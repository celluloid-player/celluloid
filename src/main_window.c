/*
 * Copyright (c) 2014-2016 gnome-mpv
 *
 * This file is part of GNOME MPV.
 *
 * GNOME MPV is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GNOME MPV is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNOME MPV.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib/gi18n.h>
#include <gdk/gdkwayland.h>

#include "def.h"
#include "menu.h"
#include "application.h"
#include "playlist_widget.h"
#include "main_window.h"
#include "control_box.h"

enum
{
	PROP_0,
	PROP_PLAYLIST,
	PROP_USE_OPENGL,
	N_PROPERTIES
};

struct _MainWindowPrivate
{
	Playlist *playlist;
	gboolean use_opengl;
};

static void main_window_set_property(	GObject *object,
					guint property_id,
					const GValue *value,
					GParamSpec *pspec );
static void main_window_get_property(	GObject *object,
					guint property_id,
					GValue *value,
					GParamSpec *pspec );
static gboolean fs_control_enter_handler(	GtkWidget *widget,
						GdkEvent *event,
						gpointer data );
static gboolean fs_control_leave_handler(	GtkWidget *widget,
						GdkEvent *event,
						gpointer data );
static gboolean motion_notify_handler(GtkWidget *widget, GdkEventMotion *event);
static void vid_area_init(MainWindow *wnd, gboolean use_opengl);
static GtkWidget *vid_area_new(gboolean use_opengl);
static gboolean timeout_handler(gpointer data);

G_DEFINE_TYPE_WITH_PRIVATE(MainWindow, main_window, GTK_TYPE_APPLICATION_WINDOW)

static void main_window_constructed(GObject *object)
{
	MainWindow *self = MAIN_WINDOW(object);

	self->playlist = playlist_widget_new(self->priv->playlist);

	gtk_widget_show_all(self->playlist);
	gtk_widget_hide(self->playlist);
	gtk_widget_set_no_show_all(self->playlist, TRUE);

	gtk_paned_pack2(	GTK_PANED(self->vid_area_paned),
				self->playlist,
				FALSE,
				FALSE );

	vid_area_init(self, self->priv->use_opengl);

	G_OBJECT_CLASS(main_window_parent_class)->constructed(object);
}

static void main_window_set_property(	GObject *object,
					guint property_id,
					const GValue *value,
					GParamSpec *pspec )
{
	MainWindow *self = MAIN_WINDOW(object);

	if(property_id == PROP_PLAYLIST)
	{
		self->priv->playlist = g_value_get_pointer(value);

	}
	else if(property_id == PROP_USE_OPENGL)
	{
		self->priv->use_opengl = g_value_get_boolean(value);
	}
	else
	{
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void main_window_get_property(	GObject *object,
					guint property_id,
					GValue *value,
					GParamSpec *pspec )
{
	MainWindow *self = MAIN_WINDOW(object);

	if(property_id == PROP_PLAYLIST)
	{
		g_value_set_pointer(value, self->priv->playlist);
	}
	else if(property_id == PROP_USE_OPENGL)
	{
		g_value_set_boolean(value, self->priv->use_opengl);
	}
	else
	{
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static gboolean fs_control_enter_handler(	GtkWidget *widget,
						GdkEvent *event,
						gpointer data )
{
	MAIN_WINDOW(data)->fs_control_hover = TRUE;

	return FALSE;
}

static gboolean fs_control_leave_handler(	GtkWidget *widget,
						GdkEvent *event,
						gpointer data )
{
	MAIN_WINDOW(data)->fs_control_hover = FALSE;

	return FALSE;
}

static gboolean motion_notify_handler(GtkWidget *widget, GdkEventMotion *event)
{
	MainWindow *wnd = MAIN_WINDOW(widget);
	GdkCursor *cursor;

	cursor = gdk_cursor_new_from_name(gdk_display_get_default(), "default");
	gdk_window_set_cursor
		(gtk_widget_get_window(GTK_WIDGET(wnd->vid_area)), cursor);

	if(wnd->fullscreen)
	{
		gtk_revealer_set_reveal_child
			(GTK_REVEALER(wnd->fs_revealer), TRUE);
	}

	if(wnd->timeout_tag > 0)
	{
		g_source_remove(wnd->timeout_tag);
	}

	wnd->timeout_tag = g_timeout_add_seconds(	FS_CONTROL_HIDE_DELAY,
							timeout_handler,
							wnd );

	return	GTK_WIDGET_CLASS(main_window_parent_class)
		->motion_notify_event(widget, event);
}

static void vid_area_init(MainWindow *wnd, gboolean use_opengl)
{
	/* vid_area cannot be initialized more than once */
	if(!wnd->vid_area)
	{
		GtkTargetEntry targets[] = DND_TARGETS;
		GtkStyleContext *style_context;

		wnd->vid_area =	vid_area_new(use_opengl);
		style_context = gtk_widget_get_style_context(wnd->vid_area);

		gtk_style_context_add_class(style_context, "gmpv-vid-area");

		gtk_drag_dest_set(	wnd->vid_area,
					GTK_DEST_DEFAULT_ALL,
					targets,
					G_N_ELEMENTS(targets),
					GDK_ACTION_LINK );
		gtk_drag_dest_add_uri_targets(wnd->vid_area);

		/* GDK_BUTTON_RELEASE_MASK is needed so that GtkMenuButtons can
		 * hide their menus when vid_area is clicked.
		 */
		gtk_widget_add_events(	wnd->vid_area,
					GDK_BUTTON_PRESS_MASK|
					GDK_BUTTON_RELEASE_MASK );

		gtk_container_add(	GTK_CONTAINER(wnd->vid_area_overlay),
					wnd->vid_area );
		gtk_paned_pack1(	GTK_PANED(wnd->vid_area_paned),
					wnd->vid_area_overlay,
					TRUE,
					TRUE );
	}
}

static GtkWidget *vid_area_new(gboolean use_opengl)
{
	return use_opengl?gtk_gl_area_new():gtk_drawing_area_new();
}

static gboolean timeout_handler(gpointer data)
{
	MainWindow *wnd;
	ControlBox *control_box;

	wnd = data;
	control_box = CONTROL_BOX(wnd->control_box);

	if(wnd->fullscreen
	&& !wnd->fs_control_hover
	&& !control_box_get_volume_popup_visible(control_box))
	{
		GdkWindow *window;
		GdkCursor *cursor;

		window = gtk_widget_get_window(GTK_WIDGET(wnd->vid_area));
		cursor = gdk_cursor_new_for_display
				(gdk_display_get_default(), GDK_BLANK_CURSOR);

		gdk_window_set_cursor(window, cursor);
		gtk_revealer_set_reveal_child
			(GTK_REVEALER(wnd->fs_revealer), FALSE);
	}

	wnd->timeout_tag = 0;

	return FALSE;
}

static void main_window_class_init(MainWindowClass *klass)
{
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);
	GtkWidgetClass *wgt_class = GTK_WIDGET_CLASS(klass);
	GParamSpec *pspec = NULL;

	obj_class->constructed = main_window_constructed;
	obj_class->set_property = main_window_set_property;
	obj_class->get_property = main_window_get_property;
	wgt_class->motion_notify_event = motion_notify_handler;

	pspec = g_param_spec_pointer
		(	"playlist",
			"Playlist",
			"Playlist object used to store playlist items",
			G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE );

	g_object_class_install_property(obj_class, PROP_PLAYLIST, pspec);

	pspec = g_param_spec_boolean
		(	"use-opengl",
			"Use OpenGL",
			"Whether or not to set up video area for opengl-cb",
			FALSE,
			G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE );

	g_object_class_install_property(obj_class, PROP_USE_OPENGL, pspec);
}

static void main_window_init(MainWindow *wnd)
{
	/* wnd->vid_area will be initialized when use-opengl property is set */
	wnd->priv = main_window_get_instance_private(wnd);
	wnd->fullscreen = FALSE;
	wnd->playlist_visible = FALSE;
	wnd->fs_control_hover = FALSE;
	wnd->pre_fs_playlist_visible = FALSE;
	wnd->playlist_width = PLAYLIST_DEFAULT_WIDTH;
	wnd->timeout_tag = 0;
	wnd->settings = gtk_settings_get_default();
	wnd->header_bar = gtk_header_bar_new();
	wnd->open_hdr_btn = NULL;
	wnd->fullscreen_hdr_btn = NULL;
	wnd->menu_hdr_btn = NULL;
	wnd->main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	wnd->vid_area_paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
	wnd->vid_area_overlay = gtk_overlay_new();
	wnd->control_box = control_box_new();
	wnd->fs_revealer = gtk_revealer_new();

	gtk_widget_add_events(	wnd->vid_area_overlay,
				GDK_ENTER_NOTIFY_MASK
				|GDK_LEAVE_NOTIFY_MASK );

	gtk_header_bar_set_show_close_button(	GTK_HEADER_BAR(wnd->header_bar),
						TRUE );

	gtk_window_set_title(GTK_WINDOW(wnd), g_get_application_name());

	gtk_paned_set_position(	GTK_PANED(wnd->vid_area_paned),
				MAIN_WINDOW_DEFAULT_WIDTH
				-PLAYLIST_DEFAULT_WIDTH );

	gtk_window_set_default_size(	GTK_WINDOW(wnd),
					MAIN_WINDOW_DEFAULT_WIDTH,
					MAIN_WINDOW_DEFAULT_HEIGHT );

	gtk_widget_set_vexpand(wnd->fs_revealer, FALSE);
	gtk_widget_set_hexpand(wnd->fs_revealer, FALSE);
	gtk_widget_set_halign(wnd->fs_revealer, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(wnd->fs_revealer, GTK_ALIGN_END);
	gtk_widget_show(wnd->fs_revealer);
	gtk_revealer_set_reveal_child(GTK_REVEALER(wnd->fs_revealer), FALSE);

	gtk_overlay_add_overlay
		(GTK_OVERLAY(wnd->vid_area_overlay), wnd->fs_revealer);
	gtk_box_pack_start
		(GTK_BOX(wnd->main_box), wnd->vid_area_paned, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(wnd->main_box), wnd->control_box);
	gtk_container_add(GTK_CONTAINER(wnd), wnd->main_box);

	g_signal_connect(	wnd->vid_area_overlay,
				"enter-notify-event",
				G_CALLBACK(fs_control_enter_handler),
				wnd );
	g_signal_connect(	wnd->vid_area_overlay,
				"leave-notify-event",
				G_CALLBACK(fs_control_leave_handler),
				wnd );
}

GtkWidget *main_window_new(	Application *app,
				Playlist *playlist,
				gboolean use_opengl )
{
	return GTK_WIDGET(g_object_new(	main_window_get_type(),
					"application", app,
					"playlist", playlist,
					"use-opengl", use_opengl,
					NULL ));
}

void main_window_set_fullscreen(MainWindow *wnd, gboolean fullscreen)
{
	if(fullscreen != wnd->fullscreen)
	{
		ControlBox *control_box = CONTROL_BOX(wnd->control_box);
		GtkContainer* main_box = GTK_CONTAINER(wnd->main_box);
		GtkContainer* revealer = GTK_CONTAINER(wnd->fs_revealer);

		if(fullscreen)
		{
			GdkScreen *screen;
			GdkWindow *window;
			GdkRectangle monitor_geom;
			gint width;
			gint monitor;

			screen = gtk_window_get_screen(GTK_WINDOW(wnd));
			window = gtk_widget_get_window(GTK_WIDGET(wnd));
			monitor =	gdk_screen_get_monitor_at_window
					(screen, window);

			gdk_screen_get_monitor_geometry
				(screen, monitor, &monitor_geom);

			width = monitor_geom.width/2;

			gtk_widget_set_size_request
				(wnd->control_box, width, -1);

			g_object_ref(wnd->control_box);
			gtk_container_remove(main_box, wnd->control_box);
			gtk_container_add(revealer, wnd->control_box);
			g_object_unref(wnd->control_box);

			control_box_set_fullscreen_state(control_box, TRUE);
			gtk_window_fullscreen(GTK_WINDOW(wnd));
			gtk_window_present(GTK_WINDOW(wnd));
			gtk_widget_show(GTK_WIDGET(revealer));

			if(main_window_get_csd_enabled(wnd))
			{
				control_box_set_fullscreen_btn_visible
					(CONTROL_BOX(wnd->control_box), TRUE);
			}
			else
			{
				gtk_application_window_set_show_menubar
					(GTK_APPLICATION_WINDOW(wnd), FALSE);
			}

			wnd->pre_fs_playlist_visible = wnd->playlist_visible;
			gtk_widget_set_visible(wnd->playlist, FALSE);
			timeout_handler(wnd);
		}
		else
		{
			GdkCursor *cursor;
			GdkWindow *vid_area;

			gtk_widget_set_halign(wnd->control_box, GTK_ALIGN_FILL);
			gtk_widget_set_valign(wnd->control_box, GTK_ALIGN_FILL);
			gtk_widget_set_size_request(wnd->control_box, -1, -1);

			g_object_ref(wnd->control_box);
			gtk_container_remove(revealer, wnd->control_box);
			gtk_container_add(main_box, wnd->control_box);
			g_object_unref(wnd->control_box);

			control_box_set_fullscreen_state(control_box, FALSE);
			gtk_window_unfullscreen(GTK_WINDOW(wnd));
			gtk_widget_hide(GTK_WIDGET(revealer));

			if(main_window_get_csd_enabled(wnd))
			{
				control_box_set_fullscreen_btn_visible
					(CONTROL_BOX(wnd->control_box), FALSE);
			}
			else
			{
				gtk_application_window_set_show_menubar
					(GTK_APPLICATION_WINDOW(wnd), TRUE);
			}

			wnd->playlist_visible = wnd->pre_fs_playlist_visible;
			gtk_widget_set_visible
				(wnd->playlist, wnd->pre_fs_playlist_visible);

			cursor =	gdk_cursor_new_from_name
					(gdk_display_get_default(), "default");
			vid_area =	gtk_widget_get_window
					(GTK_WIDGET(wnd->vid_area));

			gdk_window_set_cursor(vid_area, cursor);
		}

		wnd->fullscreen = fullscreen;
	}
}

void main_window_toggle_fullscreen(MainWindow *wnd)
{
	main_window_set_fullscreen(wnd, !wnd->fullscreen);
}

void main_window_reset(MainWindow *wnd)
{
	gtk_window_set_title(GTK_WINDOW(wnd), g_get_application_name());
	control_box_reset(CONTROL_BOX(wnd->control_box));
	playlist_set_indicator_pos(PLAYLIST_WIDGET(wnd->playlist)->store, -1);
}

void main_window_save_state(MainWindow *wnd)
{
	GSettings *settings;
	gint width;
	gint height;
	gint handle_pos;
	gdouble volume;

	settings = g_settings_new(CONFIG_WIN_STATE);
	handle_pos = gtk_paned_get_position(GTK_PANED(wnd->vid_area_paned));
	volume = control_box_get_volume(CONTROL_BOX(wnd->control_box));

	gtk_window_get_size(GTK_WINDOW(wnd), &width, &height);

	g_settings_set_int(settings, "width", width);
	g_settings_set_int(settings, "height", height);
	g_settings_set_double(settings, "volume", volume);
	g_settings_set_boolean(settings, "show-playlist", wnd->playlist_visible);

	if(main_window_get_playlist_visible(wnd))
	{
		g_settings_set_int(	settings,
					"playlist-width",
					width-handle_pos );
	}
	else
	{
		g_settings_set_int(	settings,
					"playlist-width",
					wnd->playlist_width );
	}

	g_clear_object(&settings);
}

void main_window_load_state(MainWindow *wnd)
{
	if(!gtk_widget_get_realized(GTK_WIDGET(wnd)))
	{
		GSettings *settings = g_settings_new(CONFIG_WIN_STATE);
		gint width = g_settings_get_int(settings, "width");
		gint height = g_settings_get_int(settings, "height");
		gint handle_pos;
		gdouble volume;

		wnd->playlist_width
			= g_settings_get_int(settings, "playlist-width");
		wnd->playlist_visible
			= g_settings_get_boolean(settings, "show-playlist");
		volume = g_settings_get_double(settings, "volume");
		handle_pos = width-(wnd->playlist_visible?wnd->playlist_width:0);

		control_box_set_volume(CONTROL_BOX(wnd->control_box), volume);
		gtk_widget_set_visible(wnd->playlist, wnd->playlist_visible);
		gtk_window_resize(GTK_WINDOW(wnd), width, height);
		gtk_paned_set_position(	GTK_PANED(wnd->vid_area_paned),
					 handle_pos );

		g_clear_object(&settings);
	}
	else
	{
		g_critical(	"Attempted to call main_window_load_state() on "
				"realized window" );
	}
}

void main_window_update_track_list(	MainWindow *wnd,
					const GSList *audio_list,
					const GSList *video_list,
					const GSList *sub_list )
{
	if(main_window_get_csd_enabled(wnd))
	{
		GMenu *menu = g_menu_new();

		menu_build_menu_btn(menu, audio_list, video_list, sub_list);

		gtk_menu_button_set_menu_model
			(	GTK_MENU_BUTTON(wnd->menu_hdr_btn),
				G_MENU_MODEL(menu) );
	}
	else
	{
		GtkApplication *app;
		GMenu *menu;

		app = gtk_window_get_application(GTK_WINDOW(wnd));
		menu = G_MENU(gtk_application_get_menubar(app));

		if(menu)
		{
			g_menu_remove_all(menu);
			menu_build_full(menu, audio_list, video_list, sub_list);
		}
	}
}

gint main_window_get_width_margin(MainWindow *wnd)
{
	const gint offset = main_window_get_csd_enabled(wnd)?CSD_WIDTH_OFFSET:0;

	return	gtk_widget_get_allocated_width(GTK_WIDGET(wnd)) -
		gtk_widget_get_allocated_width(wnd->vid_area) -
		offset;
}

gint main_window_get_height_margin(MainWindow *wnd)
{
	GdkDisplay *display = gdk_display_get_default();
	gint offset = 0;

	if(main_window_get_csd_enabled(wnd))
	{
		offset = CSD_HEIGHT_OFFSET;
	}
	else if(GDK_IS_WAYLAND_DISPLAY(display))
	{
		offset = WAYLAND_NOCSD_HEIGHT_OFFSET;
	}

	return	gtk_widget_get_allocated_height(GTK_WIDGET(wnd)) -
		gtk_widget_get_allocated_height(wnd->vid_area) -
		offset;
}

gboolean main_window_get_use_opengl(MainWindow *wnd)
{
	return wnd->priv->use_opengl;
}

void main_window_enable_csd(MainWindow *wnd)
{
	GMenu *menu_btn_menu;
	GMenu *open_btn_menu;
	GIcon *open_icon;
	GIcon *fullscreen_icon;
	GIcon *menu_icon;

	open_btn_menu = g_menu_new();
	menu_btn_menu = g_menu_new();

	open_icon = g_themed_icon_new_with_default_fallbacks
				("list-add-symbolic");
	fullscreen_icon = g_themed_icon_new_with_default_fallbacks
				("view-fullscreen-symbolic");
	menu_icon = g_themed_icon_new_with_default_fallbacks
				("view-list-symbolic");

	wnd->playlist_width = PLAYLIST_DEFAULT_WIDTH;
	wnd->open_hdr_btn = gtk_menu_button_new();
	wnd->fullscreen_hdr_btn = gtk_button_new();
	wnd->menu_hdr_btn = gtk_menu_button_new();

	menu_build_open_btn(open_btn_menu);
	menu_build_menu_btn(menu_btn_menu, NULL, NULL, NULL);

	gtk_widget_set_can_focus(wnd->open_hdr_btn, FALSE);
	gtk_widget_set_can_focus(wnd->fullscreen_hdr_btn, FALSE);
	gtk_widget_set_can_focus(wnd->menu_hdr_btn, FALSE);

	gtk_button_set_image
		(	GTK_BUTTON(wnd->fullscreen_hdr_btn),
			gtk_image_new_from_gicon
				(fullscreen_icon, GTK_ICON_SIZE_MENU ));
	gtk_button_set_image
		(	GTK_BUTTON(wnd->open_hdr_btn),
			gtk_image_new_from_gicon
				(open_icon, GTK_ICON_SIZE_MENU ));
	gtk_button_set_image
		(	GTK_BUTTON(wnd->menu_hdr_btn),
			gtk_image_new_from_gicon
				(menu_icon, GTK_ICON_SIZE_MENU ));
	gtk_menu_button_set_menu_model
		(	GTK_MENU_BUTTON(wnd->open_hdr_btn),
			G_MENU_MODEL(open_btn_menu) );
	gtk_menu_button_set_menu_model
		(	GTK_MENU_BUTTON(wnd->menu_hdr_btn),
			G_MENU_MODEL(menu_btn_menu) );

	gtk_header_bar_pack_start
		(GTK_HEADER_BAR(wnd->header_bar), wnd->open_hdr_btn);
	gtk_header_bar_pack_end
		(GTK_HEADER_BAR(wnd->header_bar), wnd->menu_hdr_btn);
	gtk_header_bar_pack_end
		(GTK_HEADER_BAR(wnd->header_bar), wnd->fullscreen_hdr_btn);

	gtk_widget_set_tooltip_text(	wnd->fullscreen_hdr_btn,
					_("Toggle Fullscreen") );

	gtk_actionable_set_action_name
		(	GTK_ACTIONABLE(wnd->fullscreen_hdr_btn),
			"app.fullscreen_toggle" );

	gtk_paned_set_position(	GTK_PANED(wnd->vid_area_paned),
				MAIN_WINDOW_DEFAULT_WIDTH
				-PLAYLIST_DEFAULT_WIDTH );

	gtk_window_set_titlebar(GTK_WINDOW(wnd), wnd->header_bar);
	gtk_window_set_title(GTK_WINDOW(wnd), g_get_application_name());
}

gboolean main_window_get_csd_enabled(MainWindow *wnd)
{
	return	wnd->open_hdr_btn &&
		wnd->fullscreen_hdr_btn &&
		wnd->menu_hdr_btn;
}

void main_window_set_playlist_visible(MainWindow *wnd, gboolean visible)
{
	if(visible != wnd->playlist_visible && !wnd->fullscreen)
	{
		gint handle_pos;
		gint width;
		gint height;

		handle_pos =	gtk_paned_get_position
				(GTK_PANED(wnd->vid_area_paned));

		gtk_window_get_size(GTK_WINDOW(wnd), &width, &height);

		if(!visible)
		{
			wnd->playlist_width = width-handle_pos;
		}

		wnd->playlist_visible = visible;
		gtk_widget_set_visible(wnd->playlist, visible);

		/* For some unknown reason, width needs to be adjusted by some
		 * offset (50px) when CSD is enabled for the resulting size to
		 * be correct.
		 */
		gtk_window_resize(	GTK_WINDOW(wnd),
					visible
					?width+wnd->playlist_width:handle_pos,
					height );
	}
}

gboolean main_window_get_playlist_visible(MainWindow *wnd)
{
	return gtk_widget_get_visible(GTK_WIDGET(wnd->playlist));
}
