/*
 * Copyright (c) 2014-2018 gnome-mpv
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

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <gdk/gdk.h>

#include "gmpv_def.h"
#include "gmpv_marshal.h"
#include "gmpv_menu.h"
#include "gmpv_application.h"
#include "gmpv_playlist_widget.h"
#include "gmpv_main_window.h"
#include "gmpv_header_bar.h"
#include "gmpv_control_box.h"
#include "gmpv_video_area.h"

enum
{
	PROP_0,
	PROP_ALWAYS_FLOATING,
	N_PROPERTIES
};

struct _GmpvMainWindow
{
	GtkApplicationWindow parent_instance;
	gint width_offset;
	gint height_offset;
	gint resize_target[2];
	gboolean csd;
	gboolean always_floating;
	gboolean use_floating_controls;
	gboolean fullscreen;
	gboolean playlist_visible;
	gboolean playlist_first_toggle;
	gboolean pre_fs_playlist_visible;
	gint playlist_width;
	guint resize_tag;
	const GPtrArray *track_list;
	GtkWidget *header_bar;
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

static void constructed(GObject *object);
static void dispose(GObject *object);
static void set_property(	GObject *object,
				guint property_id,
				const GValue *value,
				GParamSpec *pspec );
static void get_property(	GObject *object,
				guint property_id,
				GValue *value,
				GParamSpec *pspec );
static void seek_handler(GtkWidget *widget, gdouble value, gpointer data);
static void button_clicked_handler(	GmpvControlBox *control_box,
					const gchar *button,
					gpointer data );
static void notify(GObject *object, GParamSpec *pspec);
static void resize_video_area_finalize(	GtkWidget *widget,
					GdkRectangle *allocation,
					gpointer data );
static gboolean resize_to_target(gpointer data);

G_DEFINE_TYPE(GmpvMainWindow, gmpv_main_window, GTK_TYPE_APPLICATION_WINDOW)

static void constructed(GObject *object)
{
	GmpvMainWindow *self = GMPV_MAIN_WINDOW(object);

	self->playlist = gmpv_playlist_widget_new();

	gtk_widget_show_all(self->playlist);
	gtk_widget_hide(self->playlist);
	gtk_widget_set_no_show_all(self->playlist, TRUE);

	gtk_widget_show_all(self->control_box);
	gtk_widget_hide(self->control_box);
	gtk_widget_set_no_show_all(self->control_box, TRUE);

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

static void dispose(GObject *object)
{
	g_source_clear(&GMPV_MAIN_WINDOW(object)->resize_tag);

	G_OBJECT_CLASS(gmpv_main_window_parent_class)->dispose(object);
}

static void set_property(	GObject *object,
				guint property_id,
				const GValue *value,
				GParamSpec *pspec )
{
	GmpvMainWindow *self = GMPV_MAIN_WINDOW(object);

	if(property_id == PROP_ALWAYS_FLOATING)
	{
		self->always_floating = g_value_get_boolean(value);
	}
	else
	{
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void get_property(	GObject *object,
				guint property_id,
				GValue *value,
				GParamSpec *pspec )
{
	GmpvMainWindow *self = GMPV_MAIN_WINDOW(object);

	if(property_id == PROP_ALWAYS_FLOATING)
	{
		g_value_set_boolean(value, self->always_floating);
	}
	else
	{
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void seek_handler(GtkWidget *widget, gdouble value, gpointer data)
{
	g_signal_emit_by_name(data, "seek", value);
}

static void button_clicked_handler(	GmpvControlBox *control_box,
					const gchar *button,
					gpointer data )
{
	g_signal_emit_by_name(data, "button-clicked", button);
}

static void notify(GObject *object, GParamSpec *pspec)
{
	if(g_strcmp0(pspec->name, "always-use-floating-controls") == 0)
	{
		GmpvMainWindow *wnd = GMPV_MAIN_WINDOW(object);
		gboolean floating = wnd->always_floating || wnd->fullscreen;

		gmpv_main_window_set_use_floating_controls(wnd, floating);
	}
}

static void resize_video_area_finalize(	GtkWidget *widget,
					GdkRectangle *allocation,
					gpointer data )
{
	GmpvMainWindow *wnd = data;
	GdkWindow *gdk_window = gtk_widget_get_window(data);
	GdkDisplay *display = gdk_display_get_default();
	GdkMonitor *monitor =	gdk_display_get_monitor_at_window
				(display, gdk_window);
	GdkRectangle monitor_geom = {0};
	gint width = allocation->width;
	gint height = allocation->height;
	gint target_width = wnd->resize_target[0];
	gint target_height = wnd->resize_target[1];

	g_signal_handlers_disconnect_by_func
		(widget, resize_video_area_finalize, data);

	gdk_monitor_get_geometry(monitor, &monitor_geom);

	/* Adjust resize offset */
	if((width != target_width || height != target_height)
	&& (	target_width < monitor_geom.width &&
		target_height < monitor_geom.height )
	&& !gtk_window_is_maximized(GTK_WINDOW(wnd))
	&& !wnd->fullscreen)
	{
		wnd->width_offset += target_width-width;
		wnd->height_offset += target_height-height;

		g_source_clear(&wnd->resize_tag);
		wnd->resize_tag = g_idle_add_full(	G_PRIORITY_HIGH_IDLE,
							resize_to_target,
							wnd,
							NULL );
	}
}

static gboolean resize_to_target(gpointer data)
{
	GmpvMainWindow *wnd = data;
	gint target_width = wnd->resize_target[0];
	gint target_height = wnd->resize_target[1];

	g_source_clear(&wnd->resize_tag);

	/* Emulate mpv resizing behavior by using GDK_GRAVITY_CENTER */
	gtk_window_set_gravity(GTK_WINDOW(wnd), GDK_GRAVITY_CENTER);
	gtk_window_resize(	GTK_WINDOW(wnd),
				target_width+wnd->width_offset,
				target_height+wnd->height_offset );
	gtk_window_set_gravity(GTK_WINDOW(wnd), GDK_GRAVITY_NORTH_WEST);

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

	obj_class->constructed = constructed;
	obj_class->dispose = dispose;
	obj_class->set_property = set_property;
	obj_class->get_property = get_property;
	obj_class->notify = notify;

	pspec = g_param_spec_boolean
		(	"always-use-floating-controls",
			"Always use floating controls",
			"Whether or not to use floating controls in windowed mode",
			FALSE,
			G_PARAM_READWRITE );
	g_object_class_install_property(obj_class, PROP_ALWAYS_FLOATING, pspec);

	g_signal_new(	"button-clicked",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__STRING,
			G_TYPE_NONE,
			1,
			G_TYPE_STRING );
	g_signal_new(	"seek",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__DOUBLE,
			G_TYPE_NONE,
			1,
			G_TYPE_DOUBLE );
}

static void gmpv_main_window_init(GmpvMainWindow *wnd)
{
	GmpvControlBox *vid_area_control_box = NULL;

	wnd->csd = FALSE;
	wnd->always_floating = FALSE;
	wnd->use_floating_controls = FALSE;
	wnd->fullscreen = FALSE;
	wnd->playlist_visible = FALSE;
	wnd->pre_fs_playlist_visible = FALSE;
	wnd->playlist_width = PLAYLIST_DEFAULT_WIDTH;
	wnd->resize_tag = 0;
	wnd->track_list = NULL;
	wnd->header_bar = gmpv_header_bar_new();
	wnd->main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	wnd->vid_area_paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
	wnd->vid_area = gmpv_video_area_new();
	wnd->control_box = gmpv_control_box_new();

	wnd->playlist_first_toggle = TRUE;
	wnd->width_offset = 0;
	wnd->height_offset = 0;

	vid_area_control_box =	gmpv_video_area_get_control_box
				(GMPV_VIDEO_AREA(wnd->vid_area));

	g_object_bind_property(	wnd, "title",
				wnd->vid_area, "title",
				G_BINDING_DEFAULT );
	g_object_bind_property(	wnd->control_box, "duration",
				vid_area_control_box, "duration",
				G_BINDING_DEFAULT );
	g_object_bind_property(	wnd->control_box, "pause",
				vid_area_control_box, "pause",
				G_BINDING_DEFAULT );
	g_object_bind_property(	wnd->control_box, "skip-enabled",
				vid_area_control_box, "skip-enabled",
				G_BINDING_DEFAULT );
	g_object_bind_property(	wnd->control_box, "time-position",
				vid_area_control_box, "time-position",
				G_BINDING_DEFAULT );
	g_object_bind_property(	wnd->control_box, "volume",
				vid_area_control_box, "volume",
				G_BINDING_BIDIRECTIONAL );

	g_signal_connect(	wnd->control_box,
				"seek",
				G_CALLBACK(seek_handler),
				wnd );
	g_signal_connect(	wnd->control_box,
				"button-clicked",
				G_CALLBACK(button_clicked_handler),
				wnd );
	g_signal_connect(	vid_area_control_box,
				"seek",
				G_CALLBACK(seek_handler),
				wnd );
	g_signal_connect(	vid_area_control_box,
				"button-clicked",
				G_CALLBACK(button_clicked_handler),
				wnd );

	gtk_widget_add_events(	wnd->vid_area,
				GDK_ENTER_NOTIFY_MASK
				|GDK_LEAVE_NOTIFY_MASK );

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

GtkWidget *gmpv_main_window_new(	GtkApplication *app,
					gboolean always_floating )
{
	return GTK_WIDGET(g_object_new(	gmpv_main_window_get_type(),
					"application",
					app,
					"always-use-floating-controls",
					always_floating,
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

void gmpv_main_window_set_use_floating_controls(	GmpvMainWindow *wnd,
							gboolean floating )
{
	if(floating != wnd->use_floating_controls)
	{
		GSettings *settings = g_settings_new(CONFIG_WIN_STATE);
		gboolean controls_visible =	g_settings_get_boolean
						(settings, "show-controls");

		gtk_widget_set_visible
			(wnd->control_box, controls_visible && !floating);
		gmpv_video_area_set_control_box_visible
			(GMPV_VIDEO_AREA(wnd->vid_area), floating);

		wnd->use_floating_controls = floating;

		g_clear_object(&settings);
	}
}

gboolean gmpv_main_window_get_use_floating_controls(GmpvMainWindow *wnd)
{
	return wnd->use_floating_controls;
}

void gmpv_main_window_set_fullscreen(GmpvMainWindow *wnd, gboolean fullscreen)
{
	if(fullscreen != wnd->fullscreen)
	{
		GmpvVideoArea *vid_area = GMPV_VIDEO_AREA(wnd->vid_area);
		gboolean floating = wnd->always_floating || fullscreen;
		gboolean playlist_visible =	!fullscreen &&
						wnd->pre_fs_playlist_visible;

		if(fullscreen)
		{
			gtk_window_fullscreen(GTK_WINDOW(wnd));
			gtk_window_present(GTK_WINDOW(wnd));

			wnd->pre_fs_playlist_visible = wnd->playlist_visible;
		}
		else
		{
			gtk_window_unfullscreen(GTK_WINDOW(wnd));

			wnd->playlist_visible = wnd->pre_fs_playlist_visible;
		}

		if(!gmpv_main_window_get_csd_enabled(wnd))
		{
			gtk_application_window_set_show_menubar
				(GTK_APPLICATION_WINDOW(wnd), !fullscreen);
		}

		gmpv_video_area_set_fullscreen_state(vid_area, fullscreen);
		gmpv_main_window_set_use_floating_controls(wnd, floating);
		gtk_widget_set_visible(wnd->playlist, playlist_visible);

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
	gtk_window_set_title(GTK_WINDOW(wnd), g_get_application_name());
	gmpv_control_box_reset(GMPV_CONTROL_BOX(wnd->control_box));
}

void gmpv_main_window_save_state(GmpvMainWindow *wnd)
{
	GSettings *settings;
	gint width;
	gint height;
	gint handle_pos;
	gdouble volume;
	gboolean controls_visible;

	settings = g_settings_new(CONFIG_WIN_STATE);
	handle_pos = gtk_paned_get_position(GTK_PANED(wnd->vid_area_paned));
	volume = gmpv_control_box_get_volume(GMPV_CONTROL_BOX(wnd->control_box));
	controls_visible = gtk_widget_get_visible(wnd->control_box);

	gtk_window_get_size(GTK_WINDOW(wnd), &width, &height);

	g_settings_set_int(settings, "width", width);
	g_settings_set_int(settings, "height", height);
	g_settings_set_double(settings, "volume", volume);
	g_settings_set_boolean(settings, "show-controls", controls_visible);
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
		gboolean controls_visible;
		gdouble volume;

		wnd->playlist_width
			= g_settings_get_int(settings, "playlist-width");
		wnd->playlist_visible
			= g_settings_get_boolean(settings, "show-playlist");
		controls_visible
			= g_settings_get_boolean(settings, "show-controls");
		volume = g_settings_get_double(settings, "volume");
		handle_pos = width-(wnd->playlist_visible?wnd->playlist_width:0);

		gmpv_control_box_set_volume
			(GMPV_CONTROL_BOX(wnd->control_box), volume);
		gtk_widget_set_visible(wnd->control_box, controls_visible);
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
						const GPtrArray *track_list )
{
	wnd->track_list = track_list;

	if(gmpv_main_window_get_csd_enabled(wnd))
	{
		gmpv_header_bar_update_track_list
			(GMPV_HEADER_BAR(wnd->header_bar), track_list);
		gmpv_video_area_update_track_list
			(GMPV_VIDEO_AREA(wnd->vid_area), track_list);
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
			gmpv_menu_build_full(menu, track_list);
		}
	}
}

void gmpv_main_window_resize_video_area(	GmpvMainWindow *wnd,
						gint width,
						gint height )
{
	g_signal_connect(	wnd->vid_area,
				"size-allocate",
				G_CALLBACK(resize_video_area_finalize),
				wnd );

	wnd->resize_target[0] = width;
	wnd->resize_target[1] = height;
	resize_to_target(wnd);

	/* The size may not change, so this is needed to ensure that
	 * resize_video_area_finalize() will be called so that the event handler
	 * will be disconnected.
	 */
	gtk_widget_queue_allocate(wnd->vid_area);
}

void gmpv_main_window_enable_csd(GmpvMainWindow *wnd)
{
	wnd->csd = TRUE;
	wnd->playlist_width = PLAYLIST_DEFAULT_WIDTH;

	gtk_paned_set_position(	GTK_PANED(wnd->vid_area_paned),
				MAIN_WINDOW_DEFAULT_WIDTH
				-PLAYLIST_DEFAULT_WIDTH );

	gtk_window_set_titlebar(GTK_WINDOW(wnd), wnd->header_bar);
	gtk_window_set_title(GTK_WINDOW(wnd), g_get_application_name());
}

gboolean gmpv_main_window_get_csd_enabled(GmpvMainWindow *wnd)
{
	return wnd->csd;
}

void gmpv_main_window_set_playlist_visible(	GmpvMainWindow *wnd,
						gboolean visible )
{
	if(visible != wnd->playlist_visible && !wnd->fullscreen)
	{
		GdkWindow *gdk_window;
		GdkWindowState window_state;
		gboolean resize;
		gint handle_pos;
		gint width;
		gint height;

		gdk_window = gtk_widget_get_window(GTK_WIDGET(wnd));
		window_state = gdk_window_get_state(gdk_window);
		resize = window_state &
			(	GDK_WINDOW_STATE_TOP_RESIZABLE |
				GDK_WINDOW_STATE_RIGHT_RESIZABLE |
				GDK_WINDOW_STATE_BOTTOM_RESIZABLE |
				GDK_WINDOW_STATE_LEFT_RESIZABLE );
		handle_pos =	gtk_paned_get_position
				(GTK_PANED(wnd->vid_area_paned));

		gtk_window_get_size(GTK_WINDOW(wnd), &width, &height);

		if(wnd->playlist_first_toggle && visible)
		{
			gint new_pos = width-(resize?0:wnd->playlist_width);

			gtk_paned_set_position
				(GTK_PANED(wnd->vid_area_paned), new_pos);
		}
		else if(!visible)
		{
			wnd->playlist_width = width-handle_pos;
		}

		wnd->playlist_visible = visible;
		gtk_widget_set_visible(wnd->playlist, visible);

		if(resize)
		{
			gint new_width;

			new_width =	visible?
					width+wnd->playlist_width:
					handle_pos;

			gtk_window_resize(	GTK_WINDOW(wnd),
						new_width,
						height );
		}

		wnd->playlist_first_toggle = FALSE;
	}
}

gboolean gmpv_main_window_get_playlist_visible(GmpvMainWindow *wnd)
{
	return gtk_widget_get_visible(GTK_WIDGET(wnd->playlist));
}

void gmpv_main_window_set_controls_visible(	GmpvMainWindow *wnd,
						gboolean visible )
{
	GSettings *settings = g_settings_new(CONFIG_WIN_STATE);
	const gboolean floating = wnd->use_floating_controls;

	gtk_widget_set_visible(GTK_WIDGET(wnd->control_box), visible && !floating);
	g_settings_set_boolean(settings, "show-controls", visible);

	g_clear_object(&settings);
}

gboolean gmpv_main_window_get_controls_visible(GmpvMainWindow *wnd)
{
	return gtk_widget_get_visible(GTK_WIDGET(wnd->control_box));
}

