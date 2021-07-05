/*
 * Copyright (c) 2014-2021 gnome-mpv
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

#include "celluloid-def.h"
#include "celluloid-marshal.h"
#include "celluloid-menu.h"
#include "celluloid-application.h"
#include "celluloid-playlist-widget.h"
#include "celluloid-main-window.h"
#include "celluloid-header-bar.h"
#include "celluloid-control-box.h"
#include "celluloid-video-area.h"

#define get_private(window) \
	((CelluloidMainWindowPrivate *)celluloid_main_window_get_instance_private(CELLULOID_MAIN_WINDOW(window)))

typedef struct _CelluloidMainWindowPrivate CelluloidMainWindowPrivate;

enum
{
	PROP_0,
	PROP_ALWAYS_FLOATING,
	N_PROPERTIES
};

struct _CelluloidMainWindowPrivate
{
	GtkApplicationWindow parent;
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
	const GPtrArray *disc_list;
	GtkWidget *header_bar;
	GtkWidget *main_box;
	GtkWidget *video_area_paned;
	GtkWidget *video_area;
	GtkWidget *control_box;
	GtkWidget *playlist;
};

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
				gint width,
				gint height,
				gpointer data );

static gboolean
resize_to_target(gpointer data);

G_DEFINE_TYPE_WITH_PRIVATE(CelluloidMainWindow, celluloid_main_window, GTK_TYPE_APPLICATION_WINDOW)

static void
constructed(GObject *object)
{
	CelluloidMainWindowPrivate *priv = get_private(object);

	priv->playlist = celluloid_playlist_widget_new();

	gtk_widget_hide(priv->playlist);
	gtk_widget_hide(priv->control_box);

	gtk_paned_set_start_child
		(GTK_PANED(priv->video_area_paned), priv->video_area);
	gtk_paned_set_end_child
		(GTK_PANED(priv->video_area_paned), priv->playlist);

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
				gint width,
				gint height,
				gpointer data )
{
	CelluloidMainWindow *wnd = data;
	CelluloidMainWindowPrivate *priv = get_private(wnd);
	GdkSurface *surface = gtk_widget_get_surface(data);
	GdkDisplay *display = gdk_display_get_default();
	GdkMonitor *monitor = gdk_display_get_monitor_at_surface(display, surface);
	GdkRectangle monitor_geom = {0};
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

	gtk_window_set_default_size(	GTK_WINDOW(wnd),
					target_width + priv->width_offset,
					target_height + priv->height_offset );

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
	CelluloidControlBox *video_area_control_box = NULL;
	CelluloidHeaderBar *video_area_header_bar = NULL;
	GSettings *settings = g_settings_new(CONFIG_WIN_STATE);

	priv->csd = FALSE;
	priv->always_floating = FALSE;
	priv->use_floating_controls = FALSE;
	priv->fullscreen = FALSE;
	priv->playlist_visible = FALSE;
	priv->pre_fs_playlist_visible = FALSE;
	priv->playlist_width = PLAYLIST_DEFAULT_WIDTH;
	priv->resize_tag = 0;
	priv->track_list = NULL;
	priv->disc_list = NULL;
	priv->header_bar = celluloid_header_bar_new();
	priv->main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	priv->video_area_paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
	priv->video_area = celluloid_video_area_new();
	priv->control_box = celluloid_control_box_new();

	priv->playlist_first_toggle = TRUE;
	priv->width_offset = 0;
	priv->height_offset = 0;

	video_area_control_box =
		celluloid_video_area_get_control_box
		(CELLULOID_VIDEO_AREA(priv->video_area));
	video_area_header_bar =
		celluloid_video_area_get_header_bar
		(CELLULOID_VIDEO_AREA(priv->video_area));

	g_settings_bind(	settings, "loop-playlist",
				priv->control_box, "loop",
				G_SETTINGS_BIND_DEFAULT );

	g_object_bind_property(	priv->header_bar, "open-button-active",
				video_area_header_bar, "open-button-active",
				G_BINDING_DEFAULT );
	g_object_bind_property(	priv->header_bar, "menu-button-active",
				video_area_header_bar, "menu-button-active",
				G_BINDING_DEFAULT );

	g_object_bind_property(	priv->control_box, "duration",
				video_area_control_box, "duration",
				G_BINDING_DEFAULT );
	g_object_bind_property(	priv->control_box, "pause",
				video_area_control_box, "pause",
				G_BINDING_DEFAULT );
	g_object_bind_property(	priv->control_box, "skip-enabled",
				video_area_control_box, "skip-enabled",
				G_BINDING_DEFAULT );
	g_object_bind_property(	priv->control_box, "show-fullscreen-button",
				video_area_control_box, "show-fullscreen-button",
				G_BINDING_DEFAULT );
	g_object_bind_property(	priv->control_box, "time-position",
				video_area_control_box, "time-position",
				G_BINDING_DEFAULT );
	g_object_bind_property(	priv->control_box, "loop",
				video_area_control_box, "loop",
				G_BINDING_BIDIRECTIONAL );
	g_object_bind_property(	priv->control_box, "volume",
				video_area_control_box, "volume",
				G_BINDING_BIDIRECTIONAL );
	g_object_bind_property(	priv->control_box, "volume-popup-visible",
				video_area_control_box, "volume-popup-visible",
				G_BINDING_BIDIRECTIONAL );

	g_signal_connect(	priv->control_box,
				"seek",
				G_CALLBACK(seek_handler),
				wnd );
	g_signal_connect(	priv->control_box,
				"button-clicked",
				G_CALLBACK(button_clicked_handler),
				wnd );
	g_signal_connect(	video_area_control_box,
				"seek",
				G_CALLBACK(seek_handler),
				wnd );
	g_signal_connect(	video_area_control_box,
				"button-clicked",
				G_CALLBACK(button_clicked_handler),
				wnd );

	gtk_window_set_title(GTK_WINDOW(wnd), g_get_application_name());

	gtk_paned_set_position(	GTK_PANED(priv->video_area_paned),
				MAIN_WINDOW_DEFAULT_WIDTH
				-PLAYLIST_DEFAULT_WIDTH );
	gtk_paned_set_resize_end_child(GTK_PANED(priv->video_area_paned), FALSE);

	gtk_window_set_default_size(	GTK_WINDOW(wnd),
					MAIN_WINDOW_DEFAULT_WIDTH,
					MAIN_WINDOW_DEFAULT_HEIGHT );

	gtk_widget_set_hexpand(priv->header_bar, TRUE);
	gtk_widget_set_hexpand(priv->control_box, TRUE);
	gtk_widget_set_vexpand(priv->video_area_paned, TRUE);

	gtk_box_append(GTK_BOX(priv->main_box), priv->video_area_paned);
	gtk_box_append(GTK_BOX(priv->main_box), priv->control_box);
	gtk_window_set_child(GTK_WINDOW(wnd), priv->main_box);
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
	return CELLULOID_VIDEO_AREA(get_private(wnd)->video_area);
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
			(CELLULOID_VIDEO_AREA(priv->video_area), floating);

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
		CelluloidVideoArea *video_area =
			CELLULOID_VIDEO_AREA(priv->video_area);
		GSettings *settings =
			g_settings_new(CONFIG_WIN_STATE);
		gboolean floating =
			priv->always_floating || fullscreen;
		gboolean playlist_visible =
			!fullscreen && priv->pre_fs_playlist_visible;
		gboolean show_controls = g_settings_get_boolean
			(settings, "show-controls");

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

		celluloid_video_area_set_fullscreen_state
			(video_area, fullscreen);
		celluloid_main_window_set_use_floating_controls
			(wnd, floating && show_controls);
		gtk_widget_set_visible
			(priv->playlist, playlist_visible);

		priv->fullscreen = fullscreen;

		g_object_unref(settings);
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
	gboolean maximized;
	gint handle_pos;
	gdouble volume;

	settings = g_settings_new(CONFIG_WIN_STATE);
	priv = get_private(wnd);
	maximized = gtk_window_is_maximized(GTK_WINDOW(wnd));
	handle_pos = gtk_paned_get_position(GTK_PANED(priv->video_area_paned));

	g_object_get(priv->control_box, "volume", &volume, NULL);
	gtk_window_get_default_size(GTK_WINDOW(wnd), &width, &height);

	// Controls visibility does not need to be saved here since
	// celluloid_main_window_set_controls_visible() already updates the
	// associated GSettings key when it is called.
	g_settings_set_boolean(settings, "maximized", maximized);
	g_settings_set_double(settings, "volume", volume/100.0);
	g_settings_set_boolean(settings, "show-playlist", priv->playlist_visible);

	if(!maximized)
	{
		g_settings_set_int(settings, "width", width - 52);
		g_settings_set_int(settings, "height", height - 52);
	}

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
		gboolean maximized = g_settings_get_boolean(settings, "maximized");
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

		g_object_set(priv->control_box, "volume", volume, NULL);

		gtk_widget_set_visible(priv->control_box, controls_visible);
		gtk_widget_set_visible(priv->playlist, priv->playlist_visible);
		gtk_window_set_default_size(GTK_WINDOW(wnd), width, height);
		gtk_paned_set_position
			(GTK_PANED(priv->video_area_paned), handle_pos);

		if(maximized)
		{
			gtk_window_maximize(GTK_WINDOW(wnd));
		}

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

	celluloid_video_area_update_track_list
		(CELLULOID_VIDEO_AREA(priv->video_area), track_list);

	if(celluloid_main_window_get_csd_enabled(wnd))
	{
		celluloid_header_bar_update_track_list
			(CELLULOID_HEADER_BAR(priv->header_bar), track_list);
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

			celluloid_menu_build_full
				(menu, track_list, priv->disc_list);
		}
	}
}

void
celluloid_main_window_update_disc_list(	CelluloidMainWindow *wnd,
					const GPtrArray *disc_list )
{
	CelluloidMainWindowPrivate *priv = get_private(wnd);

	priv->disc_list = disc_list;

	if(celluloid_main_window_get_csd_enabled(wnd))
	{
		celluloid_header_bar_update_disc_list
			(CELLULOID_HEADER_BAR(priv->header_bar), disc_list);
		celluloid_video_area_update_disc_list
			(CELLULOID_VIDEO_AREA(priv->video_area), disc_list);
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

			celluloid_menu_build_full
				(menu, priv->track_list, disc_list);
		}
	}
}

void
celluloid_main_window_resize_video_area(	CelluloidMainWindow *wnd,
						gint width,
						gint height )
{
	/* As of GNOME 3.36, attempting to resize the window while it is
	 * maximized will cause the UI to stop rendering. Resizing while
	 * fullscreen is unaffected, but it doesn't make sense to resize there
	 * either.
	 */
	if(	!get_private(wnd)->fullscreen &&
		!gtk_window_is_maximized(GTK_WINDOW(wnd)) )
	{
		CelluloidMainWindowPrivate *priv = get_private(wnd);

		g_signal_connect(	priv->video_area,
					"resize",
					G_CALLBACK(resize_video_area_finalize),
					wnd );

		priv->resize_target[0] = width;
		priv->resize_target[1] = height;
		resize_to_target(wnd);

		/* The size may not change, so this is needed to ensure that
		 * resize_video_area_finalize() will be called so that the event handler
		 * will be disconnected.
		 */
		gtk_widget_queue_allocate(priv->video_area);
	}
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
		gboolean resize;
		gint handle_pos;
		gint width;
		gint height;

		resize = gtk_window_get_resizable(GTK_WINDOW(wnd));
		handle_pos =	gtk_paned_get_position
				(GTK_PANED(priv->video_area_paned));

		gtk_window_get_default_size(GTK_WINDOW(wnd), &width, &height);

		if(priv->playlist_first_toggle && visible)
		{
			gint new_pos = width - (resize ? 0 : priv->playlist_width);

			gtk_paned_set_position
				(GTK_PANED(priv->video_area_paned), new_pos);
		}
		else if(!visible)
		{
			priv->playlist_width = width - handle_pos;
		}

		priv->playlist_visible = visible;
		gtk_widget_set_visible(priv->playlist, visible);

		if(resize)
		{
			gint new_width;

			new_width =	visible?
					width + priv->playlist_width:
					handle_pos;

			gtk_window_set_default_size
				(GTK_WINDOW(wnd), new_width, height);
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
	const gboolean fullscreen = priv->fullscreen;

	gtk_widget_set_visible
		(GTK_WIDGET(priv->control_box), visible && !fullscreen && !floating);
	celluloid_video_area_set_control_box_visible
		(CELLULOID_VIDEO_AREA(priv->video_area), visible && fullscreen);
	g_settings_set_boolean
		(settings, "show-controls", visible);

	g_clear_object(&settings);
}

gboolean
celluloid_main_window_get_controls_visible(CelluloidMainWindow *wnd)
{
	CelluloidMainWindowPrivate *priv = get_private(wnd);
	CelluloidVideoArea *video_area = CELLULOID_VIDEO_AREA(priv->video_area);

	return	priv->fullscreen ?
		celluloid_video_area_get_control_box_visible(video_area) :
		gtk_widget_get_visible(GTK_WIDGET(priv->control_box));
}
