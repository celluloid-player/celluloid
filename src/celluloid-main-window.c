/*
 * Copyright (c) 2014-2019 gnome-mpv
 *
 * This file is part of Celluloid.
 *
 * Celluloid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Celluloid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Celluloid.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <gdk/gdk.h>

#include "celluloid-main-window-private.h"
#include "celluloid-def.h"
#include "celluloid-marshal.h"
#include "celluloid-menu.h"
#include "celluloid-application.h"
#include "celluloid-playlist-widget.h"
#include "celluloid-main-window.h"
#include "celluloid-header-bar.h"
#include "celluloid-control-box.h"
#include "celluloid-video-area.h"

static void
constructed(GObject *object);

static void
dispose(GObject *object);

static void
set_property(	GObject *object,
		guint property_id,
		const GValue *value,
		GParamSpec *pspec );

static void
get_property(	GObject *object,
		guint property_id,
		GValue *value,
		GParamSpec *pspec );

static void
seek_handler(GtkWidget *widget, gdouble value, gpointer data);

static void
button_clicked_handler(	CelluloidControlBox *control_box,
			const gchar *button,
			gpointer data );

static void
notify(GObject *object, GParamSpec *pspec);

static void
resize_video_area_finalize(	GtkWidget *widget,
				GdkRectangle *allocation,
				gpointer data );

static gboolean
resize_to_target(gpointer data);

G_DEFINE_TYPE_WITH_PRIVATE(CelluloidMainWindow, celluloid_main_window, GTK_TYPE_APPLICATION_WINDOW)

static void
constructed(GObject *object)
{
	CelluloidMainWindowPrivate *priv = get_private(object);

	priv->playlist = celluloid_playlist_widget_new();

	gtk_widget_show_all(priv->playlist);
	gtk_widget_hide(priv->playlist);
	gtk_widget_set_no_show_all(priv->playlist, TRUE);

	gtk_widget_show_all(priv->control_box);
	gtk_widget_hide(priv->control_box);
	gtk_widget_set_no_show_all(priv->control_box, TRUE);

	gtk_paned_pack1(	GTK_PANED(priv->vid_area_paned),
				priv->vid_area,
				TRUE,
				TRUE );
	gtk_paned_pack2(	GTK_PANED(priv->vid_area_paned),
				priv->playlist,
				FALSE,
				FALSE );

	G_OBJECT_CLASS(celluloid_main_window_parent_class)->constructed(object);
}

static void
dispose(GObject *object)
{
	g_source_clear(&get_private(object)->resize_tag);

	G_OBJECT_CLASS(celluloid_main_window_parent_class)->dispose(object);
}

static void
set_property(	GObject *object,
		guint property_id,
		const GValue *value,
		GParamSpec *pspec )
{
	CelluloidMainWindowPrivate *priv = get_private(object);

	if(property_id == PROP_ALWAYS_FLOATING)
	{
		priv->always_floating = g_value_get_boolean(value);
	}
	else
	{
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void
get_property(	GObject *object,
		guint property_id,
		GValue *value,
		GParamSpec *pspec )
{
	CelluloidMainWindowPrivate *priv = get_private(object);

	if(property_id == PROP_ALWAYS_FLOATING)
	{
		g_value_set_boolean(value, priv->always_floating);
	}
	else
	{
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void
seek_handler(GtkWidget *widget, gdouble value, gpointer data)
{
	g_signal_emit_by_name(data, "seek", value);
}

static void
button_clicked_handler(	CelluloidControlBox *control_box,
			const gchar *button,
			gpointer data )
{
	gchar *name = g_strconcat("button-clicked::", button, NULL);

	g_signal_emit_by_name(data, name);
	g_free(name);
}

static void
notify(GObject *object, GParamSpec *pspec)
{
	if(g_strcmp0(pspec->name, "always-use-floating-controls") == 0)
	{
		CelluloidMainWindow *wnd = CELLULOID_MAIN_WINDOW(object);
		CelluloidMainWindowPrivate *priv = get_private(wnd);
		gboolean floating = priv->always_floating || priv->fullscreen;

		celluloid_main_window_set_use_floating_controls(wnd, floating);
	}
}

static void
resize_video_area_finalize(	GtkWidget *widget,
				GdkRectangle *allocation,
				gpointer data )
{
	CelluloidMainWindow *wnd = data;
	CelluloidMainWindowPrivate *priv = get_private(wnd);
	GdkWindow *gdk_window = gtk_widget_get_window(data);
	GdkDisplay *display = gdk_display_get_default();
	GdkMonitor *monitor =	gdk_display_get_monitor_at_window
				(display, gdk_window);
	GdkRectangle monitor_geom = {0};
	gint width = allocation->width;
	gint height = allocation->height;
	gint target_width = priv->resize_target[0];
	gint target_height = priv->resize_target[1];

	g_signal_handlers_disconnect_by_func
		(widget, resize_video_area_finalize, data);

	gdk_monitor_get_geometry(monitor, &monitor_geom);

	/* Adjust resize offset */
	if((width != target_width || height != target_height)
	&& (	target_width < monitor_geom.width &&
		target_height < monitor_geom.height )
	&& !gtk_window_is_maximized(GTK_WINDOW(wnd))
	&& !priv->fullscreen)
	{
		priv->width_offset += target_width-width;
		priv->height_offset += target_height-height;

		g_source_clear(&priv->resize_tag);
		priv->resize_tag = g_idle_add_full(	G_PRIORITY_HIGH_IDLE,
							resize_to_target,
							wnd,
							NULL );
	}
}

static gboolean
resize_to_target(gpointer data)
{
	CelluloidMainWindow *wnd = data;
	CelluloidMainWindowPrivate *priv = get_private(data);
	gint target_width = priv->resize_target[0];
	gint target_height = priv->resize_target[1];

	g_source_clear(&priv->resize_tag);

	/* Emulate mpv resizing behavior by using GDK_GRAVITY_CENTER */
	gtk_window_set_gravity(GTK_WINDOW(wnd), GDK_GRAVITY_CENTER);
	gtk_window_resize(	GTK_WINDOW(wnd),
				target_width+priv->width_offset,
				target_height+priv->height_offset );
	gtk_window_set_gravity(GTK_WINDOW(wnd), GDK_GRAVITY_NORTH_WEST);

	/* Prevent graphical glitches that appear when calling
	 * celluloid_main_window_resize_video_area() with the current size as
	 * the target size.
	 */
	celluloid_playlist_widget_queue_draw
		(CELLULOID_PLAYLIST_WIDGET(priv->playlist));

	return FALSE;
}

static void
celluloid_main_window_class_init(CelluloidMainWindowClass *klass)
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
			G_SIGNAL_RUN_FIRST|G_SIGNAL_DETAILED,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0 );
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

static void
celluloid_main_window_init(CelluloidMainWindow *wnd)
{
	CelluloidMainWindowPrivate *priv = get_private(wnd);
	CelluloidControlBox *vid_area_control_box = NULL;

	priv->csd = FALSE;
	priv->always_floating = FALSE;
	priv->use_floating_controls = FALSE;
	priv->fullscreen = FALSE;
	priv->playlist_visible = FALSE;
	priv->pre_fs_playlist_visible = FALSE;
	priv->playlist_width = PLAYLIST_DEFAULT_WIDTH;
	priv->resize_tag = 0;
	priv->track_list = NULL;
	priv->header_bar = celluloid_header_bar_new();
	priv->main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	priv->vid_area_paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
	priv->vid_area = celluloid_video_area_new();
	priv->control_box = celluloid_control_box_new();

	priv->playlist_first_toggle = TRUE;
	priv->width_offset = 0;
	priv->height_offset = 0;

	vid_area_control_box =	celluloid_video_area_get_control_box
				(CELLULOID_VIDEO_AREA(priv->vid_area));

	g_object_bind_property(	wnd, "title",
				priv->vid_area, "title",
				G_BINDING_DEFAULT );
	g_object_bind_property(	priv->control_box, "duration",
				vid_area_control_box, "duration",
				G_BINDING_DEFAULT );
	g_object_bind_property(	priv->control_box, "pause",
				vid_area_control_box, "pause",
				G_BINDING_DEFAULT );
	g_object_bind_property(	priv->control_box, "skip-enabled",
				vid_area_control_box, "skip-enabled",
				G_BINDING_DEFAULT );
	g_object_bind_property(	priv->control_box, "time-position",
				vid_area_control_box, "time-position",
				G_BINDING_DEFAULT );
	g_object_bind_property(	priv->control_box, "volume",
				vid_area_control_box, "volume",
				G_BINDING_BIDIRECTIONAL );

	g_signal_connect(	priv->control_box,
				"seek",
				G_CALLBACK(seek_handler),
				wnd );
	g_signal_connect(	priv->control_box,
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

	gtk_widget_add_events(	priv->vid_area,
				GDK_ENTER_NOTIFY_MASK
				|GDK_LEAVE_NOTIFY_MASK );

	gtk_window_set_title(GTK_WINDOW(wnd), g_get_application_name());

	gtk_paned_set_position(	GTK_PANED(priv->vid_area_paned),
				MAIN_WINDOW_DEFAULT_WIDTH
				-PLAYLIST_DEFAULT_WIDTH );

	gtk_window_set_default_size(	GTK_WINDOW(wnd),
					MAIN_WINDOW_DEFAULT_WIDTH,
					MAIN_WINDOW_DEFAULT_HEIGHT );

	gtk_box_pack_start
		(GTK_BOX(priv->main_box), priv->vid_area_paned, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(priv->main_box), priv->control_box);
	gtk_container_add(GTK_CONTAINER(wnd), priv->main_box);
}

GtkWidget *
celluloid_main_window_new(	GtkApplication *app,
				gboolean always_floating )
{
	return GTK_WIDGET(g_object_new(	celluloid_main_window_get_type(),
					"application",
					app,
					"always-use-floating-controls",
					always_floating,
					NULL ));
}

CelluloidPlaylistWidget *
celluloid_main_window_get_playlist(CelluloidMainWindow *wnd)
{
	return CELLULOID_PLAYLIST_WIDGET(get_private(wnd)->playlist);
}

CelluloidControlBox *
celluloid_main_window_get_control_box(CelluloidMainWindow *wnd)
{
	return CELLULOID_CONTROL_BOX(get_private(wnd)->control_box);
}

CelluloidVideoArea *
celluloid_main_window_get_video_area(CelluloidMainWindow *wnd)
{
	return CELLULOID_VIDEO_AREA(get_private(wnd)->vid_area);
}

void
celluloid_main_window_set_use_floating_controls(	CelluloidMainWindow *wnd,
							gboolean floating )
{
	CelluloidMainWindowPrivate *priv = get_private(wnd);

	if(floating != priv->use_floating_controls)
	{
		GSettings *settings = g_settings_new(CONFIG_WIN_STATE);
		gboolean controls_visible =	g_settings_get_boolean
						(settings, "show-controls");

		gtk_widget_set_visible
			(priv->control_box, controls_visible && !floating);
		celluloid_video_area_set_control_box_visible
			(CELLULOID_VIDEO_AREA(priv->vid_area), floating);

		priv->use_floating_controls = floating;

		g_clear_object(&settings);
	}
}

gboolean
celluloid_main_window_get_use_floating_controls(CelluloidMainWindow *wnd)
{
	return get_private(wnd)->use_floating_controls;
}

void
celluloid_main_window_set_fullscreen(CelluloidMainWindow *wnd, gboolean fullscreen)
{
	CelluloidMainWindowPrivate *priv = get_private(wnd);

	if(fullscreen != priv->fullscreen)
	{
		CelluloidVideoArea *vid_area =
			CELLULOID_VIDEO_AREA(priv->vid_area);
		gboolean floating =
			priv->always_floating || fullscreen;
		gboolean playlist_visible =
			!fullscreen && priv->pre_fs_playlist_visible;

		if(fullscreen)
		{
			gtk_window_fullscreen(GTK_WINDOW(wnd));
			gtk_window_present(GTK_WINDOW(wnd));

			priv->pre_fs_playlist_visible = priv->playlist_visible;
		}
		else
		{
			gtk_window_unfullscreen(GTK_WINDOW(wnd));

			priv->playlist_visible = priv->pre_fs_playlist_visible;
		}

		if(!celluloid_main_window_get_csd_enabled(wnd))
		{
			gtk_application_window_set_show_menubar
				(GTK_APPLICATION_WINDOW(wnd), !fullscreen);
		}

		celluloid_video_area_set_fullscreen_state(vid_area, fullscreen);
		celluloid_main_window_set_use_floating_controls(wnd, floating);
		gtk_widget_set_visible(priv->playlist, playlist_visible);

		priv->fullscreen = fullscreen;
	}
}

gboolean
celluloid_main_window_get_fullscreen(CelluloidMainWindow *wnd)
{
	return get_private(wnd)->fullscreen;
}

void
celluloid_main_window_toggle_fullscreen(CelluloidMainWindow *wnd)
{
	celluloid_main_window_set_fullscreen(wnd, !get_private(wnd)->fullscreen);
}

void
celluloid_main_window_reset(CelluloidMainWindow *wnd)
{
	gtk_window_set_title
		(GTK_WINDOW(wnd), g_get_application_name());
	celluloid_control_box_reset
		(CELLULOID_CONTROL_BOX(get_private(wnd)->control_box));
}

void
celluloid_main_window_save_state(CelluloidMainWindow *wnd)
{
	GSettings *settings;
	CelluloidMainWindowPrivate *priv;
	gint width;
	gint height;
	gint handle_pos;
	gdouble volume;
	gboolean controls_visible;

	settings = g_settings_new(CONFIG_WIN_STATE);
	priv = get_private(wnd);
	handle_pos = gtk_paned_get_position(GTK_PANED(priv->vid_area_paned));
	volume =	celluloid_control_box_get_volume
			(CELLULOID_CONTROL_BOX(priv->control_box));
	controls_visible = gtk_widget_get_visible(priv->control_box);

	gtk_window_get_size(GTK_WINDOW(wnd), &width, &height);

	g_settings_set_int(settings, "width", width);
	g_settings_set_int(settings, "height", height);
	g_settings_set_double(settings, "volume", volume);
	g_settings_set_boolean(settings, "show-controls", controls_visible);
	g_settings_set_boolean(settings, "show-playlist", priv->playlist_visible);

	if(celluloid_main_window_get_playlist_visible(wnd))
	{
		g_settings_set_int(	settings,
					"playlist-width",
					width-handle_pos );
	}
	else
	{
		g_settings_set_int(	settings,
					"playlist-width",
					priv->playlist_width );
	}

	g_clear_object(&settings);
}

void
celluloid_main_window_load_state(CelluloidMainWindow *wnd)
{
	if(!gtk_widget_get_realized(GTK_WIDGET(wnd)))
	{
		GSettings *settings = g_settings_new(CONFIG_WIN_STATE);
		CelluloidMainWindowPrivate *priv = get_private(wnd);
		gint width = g_settings_get_int(settings, "width");
		gint height = g_settings_get_int(settings, "height");
		gint handle_pos;
		gboolean controls_visible;
		gdouble volume;

		priv->playlist_width
			= g_settings_get_int(settings, "playlist-width");
		priv->playlist_visible
			= g_settings_get_boolean(settings, "show-playlist");
		controls_visible
			= g_settings_get_boolean(settings, "show-controls");
		volume = g_settings_get_double(settings, "volume");
		handle_pos = width-(priv->playlist_visible?priv->playlist_width:0);

		celluloid_control_box_set_volume
			(CELLULOID_CONTROL_BOX(priv->control_box), volume);
		gtk_widget_set_visible(priv->control_box, controls_visible);
		gtk_widget_set_visible(priv->playlist, priv->playlist_visible);
		gtk_window_resize(GTK_WINDOW(wnd), width, height);
		gtk_paned_set_position
			(GTK_PANED(priv->vid_area_paned), handle_pos);

		g_clear_object(&settings);
	}
	else
	{
		g_critical(	"Attempted to call "
				"celluloid_main_window_load_state() "
				"on realized window" );
	}
}

void
celluloid_main_window_update_track_list(	CelluloidMainWindow *wnd,
						const GPtrArray *track_list )
{
	CelluloidMainWindowPrivate *priv = get_private(wnd);

	priv->track_list = track_list;

	if(celluloid_main_window_get_csd_enabled(wnd))
	{
		celluloid_header_bar_update_track_list
			(CELLULOID_HEADER_BAR(priv->header_bar), track_list);
		celluloid_video_area_update_track_list
			(CELLULOID_VIDEO_AREA(priv->vid_area), track_list);
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
			celluloid_menu_build_full(menu, track_list);
		}
	}
}

void
celluloid_main_window_resize_video_area(	CelluloidMainWindow *wnd,
						gint width,
						gint height )
{
	CelluloidMainWindowPrivate *priv = get_private(wnd);

	g_signal_connect(	priv->vid_area,
				"size-allocate",
				G_CALLBACK(resize_video_area_finalize),
				wnd );

	priv->resize_target[0] = width;
	priv->resize_target[1] = height;
	resize_to_target(wnd);

	/* The size may not change, so this is needed to ensure that
	 * resize_video_area_finalize() will be called so that the event handler
	 * will be disconnected.
	 */
	gtk_widget_queue_allocate(priv->vid_area);
}

void
celluloid_main_window_enable_csd(CelluloidMainWindow *wnd)
{
	CelluloidMainWindowPrivate *priv = get_private(wnd);

	priv->csd = TRUE;

	gtk_window_set_titlebar(GTK_WINDOW(wnd), priv->header_bar);
	gtk_window_set_title(GTK_WINDOW(wnd), g_get_application_name());
}

gboolean
celluloid_main_window_get_csd_enabled(CelluloidMainWindow *wnd)
{
	return get_private(wnd)->csd;
}

void
celluloid_main_window_set_playlist_visible(	CelluloidMainWindow *wnd,
						gboolean visible )
{
	CelluloidMainWindowPrivate *priv = get_private(wnd);

	if(visible != priv->playlist_visible && !priv->fullscreen)
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
				(GTK_PANED(priv->vid_area_paned));

		gtk_window_get_size(GTK_WINDOW(wnd), &width, &height);

		if(priv->playlist_first_toggle && visible)
		{
			gint new_pos = width-(resize?0:priv->playlist_width);

			gtk_paned_set_position
				(GTK_PANED(priv->vid_area_paned), new_pos);
		}
		else if(!visible)
		{
			priv->playlist_width = width-handle_pos;
		}

		priv->playlist_visible = visible;
		gtk_widget_set_visible(priv->playlist, visible);

		if(resize)
		{
			gint new_width;

			new_width =	visible?
					width+priv->playlist_width:
					handle_pos;

			gtk_window_resize(	GTK_WINDOW(wnd),
						new_width,
						height );
		}

		priv->playlist_first_toggle = FALSE;
	}
}

gboolean
celluloid_main_window_get_playlist_visible(CelluloidMainWindow *wnd)
{
	return gtk_widget_get_visible(GTK_WIDGET(get_private(wnd)->playlist));
}

void
celluloid_main_window_set_controls_visible(	CelluloidMainWindow *wnd,
						gboolean visible )
{
	GSettings *settings = g_settings_new(CONFIG_WIN_STATE);
	CelluloidMainWindowPrivate *priv = get_private(wnd);
	const gboolean floating = priv->use_floating_controls;

	gtk_widget_set_visible
		(GTK_WIDGET(priv->control_box), visible && !floating);
	g_settings_set_boolean
		(settings, "show-controls", visible);

	g_clear_object(&settings);
}

gboolean
celluloid_main_window_get_controls_visible(CelluloidMainWindow *wnd)
{
	return gtk_widget_get_visible(GTK_WIDGET(get_private(wnd)->control_box));
}
