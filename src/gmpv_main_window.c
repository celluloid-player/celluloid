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
#include <gdk/gdk.h>

#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/gdkwayland.h>
#endif

#include "gmpv_def.h"
#include "gmpv_menu.h"
#include "gmpv_application.h"
#include "gmpv_playlist_widget.h"
#include "gmpv_main_window.h"
#include "gmpv_control_box.h"
#include "gmpv_video_area.h"

enum
{
	PROP_0,
	PROP_PLAYLIST,
	N_PROPERTIES
};

struct _GmpvMainWindow
{
	GtkApplicationWindow parent_instance;
	GmpvPlaylist *playlist_store;
	gint width_offset;
	gint height_offset;
	gint resize_target[2];
	gboolean fullscreen;
	gboolean playlist_visible;
	gboolean playlist_first_toggle;
	gboolean pre_fs_playlist_visible;
	gint playlist_width;
	guint timeout_tag;
	GtkWidget *header_bar;
	GtkWidget *open_hdr_btn;
	GtkWidget *fullscreen_hdr_btn;
	GtkWidget *menu_hdr_btn;
	GtkWidget *main_box;
	GtkWidget *vid_area_paned;
	GtkWidget *vid_area;
	GtkWidget *control_box;
	GtkWidget *playlist;
};

struct _GmpvMainWindowClass
{
	GtkApplicationWindowClass parent_class;
};

static void gmpv_main_window_constructed(GObject *object);
static void gmpv_main_window_set_property(	GObject *object,
						guint property_id,
						const GValue *value,
						GParamSpec *pspec );
static void gmpv_main_window_get_property(	GObject *object,
						guint property_id,
						GValue *value,
						GParamSpec *pspec );
static void size_allocate_handler(	GtkWidget *widget,
					GdkRectangle *allocation,
					gpointer data );
static gboolean resize_to_target(gpointer data);

G_DEFINE_TYPE(GmpvMainWindow, gmpv_main_window, GTK_TYPE_APPLICATION_WINDOW)

static void gmpv_main_window_constructed(GObject *object)
{
	GmpvMainWindow *self = GMPV_MAIN_WINDOW(object);

	self->playlist = gmpv_playlist_widget_new(self->playlist_store);

	gtk_widget_show_all(self->playlist);
	gtk_widget_hide(self->playlist);
	gtk_widget_set_no_show_all(self->playlist, TRUE);

	gtk_paned_pack1(	GTK_PANED(self->vid_area_paned),
				self->vid_area,
				TRUE,
				TRUE );
	gtk_paned_pack2(	GTK_PANED(self->vid_area_paned),
				self->playlist,
				FALSE,
				FALSE );

	G_OBJECT_CLASS(gmpv_main_window_parent_class)->constructed(object);
}

static void gmpv_main_window_set_property(	GObject *object,
						guint property_id,
						const GValue *value,
						GParamSpec *pspec )
{
	GmpvMainWindow *self = GMPV_MAIN_WINDOW(object);

	if(property_id == PROP_PLAYLIST)
	{
		self->playlist_store = g_value_get_pointer(value);

	}
	else
	{
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void gmpv_main_window_get_property(	GObject *object,
						guint property_id,
						GValue *value,
						GParamSpec *pspec )
{
	GmpvMainWindow *self = GMPV_MAIN_WINDOW(object);

	if(property_id == PROP_PLAYLIST)
	{
		g_value_set_pointer(value, self->playlist_store);
	}
	else
	{
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void size_allocate_handler(	GtkWidget *widget,
					GdkRectangle *allocation,
					gpointer data )
{
	GmpvMainWindow *wnd = data;
	GdkScreen *screen = gdk_screen_get_default();
	gint screen_width = gdk_screen_get_width(screen);
	gint screen_height = gdk_screen_get_height(screen);
	gint width = allocation->width;
	gint height = allocation->height;
	gint target_width = wnd->resize_target[0];
	gint target_height = wnd->resize_target[1];

	g_signal_handlers_disconnect_by_func
		(widget, size_allocate_handler, data);

	/* Adjust resize offset */
	if((width != target_width || height != target_height)
	&& (target_width < screen_width && target_height < screen_height)
	&& !gtk_window_is_maximized(GTK_WINDOW(wnd))
	&& !wnd->fullscreen)
	{
		wnd->width_offset += target_width-width;
		wnd->height_offset += target_height-height;

		g_idle_add(resize_to_target, wnd);
	}
}

static gboolean resize_to_target(gpointer data)
{
	GmpvMainWindow *wnd = data;
	gint target_width = wnd->resize_target[0];
	gint target_height = wnd->resize_target[1];

	gtk_window_resize(	GTK_WINDOW(wnd),
				target_width+wnd->width_offset,
				target_height+wnd->height_offset );

	/* Prevent graphical glitches that appear when calling
	 * gmpv_main_window_resize_video_area() with the current size as the
	 * target size.
	 */
	gmpv_playlist_widget_queue_draw(GMPV_PLAYLIST_WIDGET(wnd->playlist));

	return FALSE;
}

static void gmpv_main_window_class_init(GmpvMainWindowClass *klass)
{
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec = NULL;

	obj_class->constructed = gmpv_main_window_constructed;
	obj_class->set_property = gmpv_main_window_set_property;
	obj_class->get_property = gmpv_main_window_get_property;

	pspec = g_param_spec_pointer
		(	"playlist",
			"Playlist",
			"Playlist object used to store playlist items",
			G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE );

	g_object_class_install_property(obj_class, PROP_PLAYLIST, pspec);
}

static void gmpv_main_window_init(GmpvMainWindow *wnd)
{
	wnd->fullscreen = FALSE;
	wnd->playlist_visible = FALSE;
	wnd->pre_fs_playlist_visible = FALSE;
	wnd->playlist_width = PLAYLIST_DEFAULT_WIDTH;
	wnd->timeout_tag = 0;
	wnd->header_bar = gtk_header_bar_new();
	wnd->open_hdr_btn = NULL;
	wnd->fullscreen_hdr_btn = NULL;
	wnd->menu_hdr_btn = NULL;
	wnd->main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	wnd->vid_area_paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
	wnd->vid_area = gmpv_video_area_new();
	wnd->control_box = gmpv_control_box_new();

	wnd->playlist_first_toggle = TRUE;
	wnd->width_offset = 0;
	wnd->height_offset = 0;

	gtk_widget_add_events(	wnd->vid_area,
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

	gtk_box_pack_start
		(GTK_BOX(wnd->main_box), wnd->vid_area_paned, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(wnd->main_box), wnd->control_box);
	gtk_container_add(GTK_CONTAINER(wnd), wnd->main_box);
}

GtkWidget *gmpv_main_window_new(GmpvApplication *app, GmpvPlaylist *playlist)
{
	return GTK_WIDGET(g_object_new(	gmpv_main_window_get_type(),
					"application", app,
					"playlist", playlist,
					NULL ));
}

GmpvPlaylistWidget *gmpv_main_window_get_playlist(GmpvMainWindow *wnd)
{
	return GMPV_PLAYLIST_WIDGET(wnd->playlist);
}

GmpvControlBox *gmpv_main_window_get_control_box(GmpvMainWindow *wnd)
{
	return GMPV_CONTROL_BOX(wnd->control_box);
}

GmpvVideoArea *gmpv_main_window_get_video_area(GmpvMainWindow *wnd)
{
	return GMPV_VIDEO_AREA(wnd->vid_area);
}

void gmpv_main_window_set_fullscreen(GmpvMainWindow *wnd, gboolean fullscreen)
{
	if(fullscreen != wnd->fullscreen)
	{
		GmpvControlBox *control_box = GMPV_CONTROL_BOX(wnd->control_box);
		GmpvVideoArea *vid_area = GMPV_VIDEO_AREA(wnd->vid_area);
		GtkContainer* main_box = GTK_CONTAINER(wnd->main_box);

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
			gmpv_video_area_set_control_box
				(vid_area, wnd->control_box);
			g_object_unref(wnd->control_box);

			gmpv_video_area_set_fullscreen_state(vid_area, TRUE);
			gtk_window_fullscreen(GTK_WINDOW(wnd));
			gtk_window_present(GTK_WINDOW(wnd));

			if(gmpv_main_window_get_csd_enabled(wnd))
			{
				gmpv_control_box_set_fullscreen_btn_visible
					(control_box, TRUE);
			}
			else
			{
				gtk_application_window_set_show_menubar
					(GTK_APPLICATION_WINDOW(wnd), FALSE);
			}

			wnd->pre_fs_playlist_visible = wnd->playlist_visible;
			gtk_widget_set_visible(wnd->playlist, FALSE);
		}
		else
		{
			gtk_widget_set_halign(wnd->control_box, GTK_ALIGN_FILL);
			gtk_widget_set_valign(wnd->control_box, GTK_ALIGN_FILL);
			gtk_widget_set_size_request(wnd->control_box, -1, -1);

			g_object_ref(wnd->control_box);
			gmpv_video_area_set_control_box(vid_area, NULL);
			gtk_container_add(main_box, wnd->control_box);
			g_object_unref(wnd->control_box);

			gmpv_video_area_set_fullscreen_state(vid_area, FALSE);
			gtk_window_unfullscreen(GTK_WINDOW(wnd));

			if(gmpv_main_window_get_csd_enabled(wnd))
			{
				gmpv_control_box_set_fullscreen_btn_visible
					(control_box, FALSE);
			}
			else
			{
				gtk_application_window_set_show_menubar
					(GTK_APPLICATION_WINDOW(wnd), TRUE);
			}

			wnd->playlist_visible = wnd->pre_fs_playlist_visible;
			gtk_widget_set_visible
				(wnd->playlist, wnd->pre_fs_playlist_visible);
		}

		wnd->fullscreen = fullscreen;
	}
}

gboolean gmpv_main_window_get_fullscreen(GmpvMainWindow *wnd)
{
	return wnd->fullscreen;
}

void gmpv_main_window_toggle_fullscreen(GmpvMainWindow *wnd)
{
	gmpv_main_window_set_fullscreen(wnd, !wnd->fullscreen);
}

void gmpv_main_window_reset(GmpvMainWindow *wnd)
{
	GmpvPlaylist *store =	gmpv_playlist_widget_get_store
				(GMPV_PLAYLIST_WIDGET(wnd->playlist));

	gtk_window_set_title(GTK_WINDOW(wnd), g_get_application_name());
	gmpv_control_box_reset(GMPV_CONTROL_BOX(wnd->control_box));
	gmpv_playlist_set_indicator_pos(store, -1);
}

void gmpv_main_window_save_state(GmpvMainWindow *wnd)
{
	GSettings *settings;
	gint width;
	gint height;
	gint handle_pos;
	gdouble volume;

	settings = g_settings_new(CONFIG_WIN_STATE);
	handle_pos = gtk_paned_get_position(GTK_PANED(wnd->vid_area_paned));
	volume = gmpv_control_box_get_volume(GMPV_CONTROL_BOX(wnd->control_box));

	gtk_window_get_size(GTK_WINDOW(wnd), &width, &height);

	g_settings_set_int(settings, "width", width);
	g_settings_set_int(settings, "height", height);
	g_settings_set_double(settings, "volume", volume);
	g_settings_set_boolean(settings, "show-playlist", wnd->playlist_visible);

	if(gmpv_main_window_get_playlist_visible(wnd))
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

void gmpv_main_window_load_state(GmpvMainWindow *wnd)
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

		gmpv_control_box_set_volume
			(GMPV_CONTROL_BOX(wnd->control_box), volume);
		gtk_widget_set_visible(wnd->playlist, wnd->playlist_visible);
		gtk_window_resize(GTK_WINDOW(wnd), width, height);
		gtk_paned_set_position
			(GTK_PANED(wnd->vid_area_paned), handle_pos);

		g_clear_object(&settings);
	}
	else
	{
		g_critical(	"Attempted to call gmpv_main_window_load_state() "
				"on realized window" );
	}
}

void gmpv_main_window_update_track_list(	GmpvMainWindow *wnd,
						const GSList *audio_list,
						const GSList *video_list,
						const GSList *sub_list )
{
	if(gmpv_main_window_get_csd_enabled(wnd))
	{
		GMenu *menu = g_menu_new();

		gmpv_menu_build_menu_btn
			(menu, audio_list, video_list, sub_list);

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
			gmpv_menu_build_full
				(menu, audio_list, video_list, sub_list);
		}
	}
}

void gmpv_main_window_resize_video_area(	GmpvMainWindow *wnd,
						gint width,
						gint height )
{
	g_signal_connect(	wnd->vid_area,
				"size-allocate",
				G_CALLBACK(size_allocate_handler),
				wnd );

	wnd->resize_target[0] = width;
	wnd->resize_target[1] = height;
	resize_to_target(wnd);

	/* The size may not change, so this is needed to ensure that
	 * size_allocate_handler() will be called so that the event handler will
	 * be disconnected.
	 */
#if GTK_CHECK_VERSION(3, 20, 0)
	gtk_widget_queue_allocate(wnd->vid_area);
#else
	gtk_widget_queue_resize(wnd->vid_area);
#endif
}

void gmpv_main_window_enable_csd(GmpvMainWindow *wnd)
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
				("open-menu-symbolic");

	wnd->playlist_width = PLAYLIST_DEFAULT_WIDTH;
	wnd->open_hdr_btn = gtk_menu_button_new();
	wnd->fullscreen_hdr_btn = gtk_button_new();
	wnd->menu_hdr_btn = gtk_menu_button_new();

	gmpv_menu_build_open_btn(open_btn_menu);
	gmpv_menu_build_menu_btn(menu_btn_menu, NULL, NULL, NULL);

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

gboolean gmpv_main_window_get_csd_enabled(GmpvMainWindow *wnd)
{
	return	wnd->open_hdr_btn &&
		wnd->fullscreen_hdr_btn &&
		wnd->menu_hdr_btn;
}

void gmpv_main_window_set_playlist_visible(	GmpvMainWindow *wnd,
						gboolean visible )
{
	if(visible != wnd->playlist_visible && !wnd->fullscreen)
	{
		gboolean maximized;
		gint handle_pos;
		gint width;
		gint height;

		maximized = gtk_window_is_maximized(GTK_WINDOW(wnd));
		handle_pos =	gtk_paned_get_position
				(GTK_PANED(wnd->vid_area_paned));

		gtk_window_get_size(GTK_WINDOW(wnd), &width, &height);

		if(wnd->playlist_first_toggle && visible)
		{
			gint new_pos = width-(maximized?wnd->playlist_width:0);

#if !GTK_CHECK_VERSION(3, 20, 0)
			/* Workaround for window sizing bug affecting
			 * GTK+ < 3.20
			 */
			new_pos -= 52;
#endif

			gtk_paned_set_position(	GTK_PANED(wnd->vid_area_paned),
						new_pos );
		}
		else if(!visible)
		{
			wnd->playlist_width = width-handle_pos;
		}


		wnd->playlist_visible = visible;
		gtk_widget_set_visible(wnd->playlist, visible);

		if(!maximized)
		{
#if GTK_CHECK_VERSION(3, 20, 0)
			gtk_window_resize(	GTK_WINDOW(wnd),
						visible?
						width+wnd->playlist_width:
						handle_pos,
						height );
#else
			/* Workaround for window sizing bug affecting
			 * GTK+ < 3.20
			 */
			gint wnd_offset;

			wnd_offset = gmpv_main_window_get_csd_enabled(wnd)?52:0;

			gtk_window_resize(	GTK_WINDOW(wnd),
						visible?
						width+wnd->playlist_width-wnd_offset:
						handle_pos+wnd_offset,
						height );
#endif
		}

		wnd->playlist_first_toggle = FALSE;
	}
}

gboolean gmpv_main_window_get_playlist_visible(GmpvMainWindow *wnd)
{
	return gtk_widget_get_visible(GTK_WIDGET(wnd->playlist));
}
