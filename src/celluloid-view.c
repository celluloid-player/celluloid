/*
 * Copyright (c) 2017-2025 gnome-mpv
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

#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <adwaita.h>

#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/wayland/gdkwayland.h>
#endif

#include "celluloid-view.h"
#include "celluloid-file-dialog.h"
#include "celluloid-open-location-dialog.h"
#include "celluloid-preferences-dialog.h"
#include "celluloid-shortcuts-window.h"
#include "celluloid-authors.h"
#include "celluloid-marshal.h"
#include "celluloid-menu.h"
#include "celluloid-common.h"
#include "celluloid-def.h"

enum
{
	PROP_0,
	PROP_PLAYLIST_COUNT,
	PROP_PAUSE,
	PROP_IDLE_ACTIVE,
	PROP_VOLUME,
	PROP_VOLUME_MAX,
	PROP_DURATION,
	PROP_PLAYLIST_POS,
	PROP_CHAPTER_LIST,
	PROP_TRACK_LIST,
	PROP_DISC_LIST,
	PROP_SKIP_ENABLED,
	PROP_LOOP_FILE,
	PROP_LOOP,
	PROP_SHUFFLE,
	PROP_MEDIA_TITLE,
	PROP_DISPLAY_FPS,
	PROP_SEARCHING,
	N_PROPERTIES
};

struct _CelluloidView
{
	CelluloidMainWindow parent;
	gboolean disposed;
	gboolean has_dialog;

	/* Properties */
	gint playlist_count;
	gboolean pause;
	gboolean idle_active;
	gdouble volume;
	gdouble volume_max;
	gdouble duration;
	gint playlist_pos;
	GPtrArray *chapter_list;
	GPtrArray *track_list;
	GPtrArray *disc_list;
	gboolean skip_enabled;
	gboolean control_box_enabled;
	gboolean loop_file;
	gboolean loop;
	gboolean shuffle;
	gchar *media_title;
	gdouble display_fps;
	gboolean searching;
};

struct _CelluloidViewClass
{
	CelluloidMainWindowClass parent_class;
};

G_DEFINE_TYPE(CelluloidView, celluloid_view, CELLULOID_TYPE_MAIN_WINDOW)

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
load_css(CelluloidView *view);

static void
load_settings(CelluloidView *view);

static void
save_playlist(CelluloidView *view, GFile *file, GError **error);

static void
update_title(CelluloidView *view);

static void
show_message_dialog(CelluloidView *view, const gchar *prefix, const gchar *msg);

static void
show_open_track_dialog(CelluloidView  *view, TrackType type);

/* Dialog responses */
static void
open_dialog_callback(	GObject *source_object,
			GAsyncResult *res,
			gpointer data );

static void
open_location_dialog_response_handler(	GtkDialog *dialog,
					gint response_id,
					gpointer data );

static void
open_track_dialog_callback(	GObject *source_object,
				GAsyncResult *res,
				gpointer data );

static void
save_playlist_callback(	GObject *source_object,
			GAsyncResult *res,
			gpointer data );

static gboolean
mpv_reset_request_handler(AdwPreferencesWindow *dialog, gpointer data);

static void
error_handler(	CelluloidPreferencesDialog *dialog,
		const gchar *message,
		gpointer data );

static void
realize_handler(GtkWidget *widget, gpointer data);

static void
notify_mapped_handler(GdkSurface *surface, GParamSpec *pspec,  gpointer data);

static void
resize_handler(	CelluloidVideoArea *video_area,
		gint width,
		gint height,
		gpointer data );

static void
render_handler(CelluloidVideoArea *area, gpointer data);

static gboolean
drop_handler(	GtkDropTarget *self,
		GValue *value,
		gdouble x,
		gdouble y,
		gpointer data );

static gboolean
close_request_handler(GtkWidget *widget, gpointer data);

static void
playlist_row_activated_handler(	CelluloidPlaylistWidget *playlist,
				gint64 pos,
				gpointer data );

static void
playlist_row_inserted_handler(	CelluloidPlaylistWidget *widget,
				gint pos,
				gpointer data );

static void
playlist_row_deleted_handler(	CelluloidPlaylistWidget *widget,
				gint pos,
				gpointer data );

static void
playlist_row_reordered_handler(	CelluloidPlaylistWidget *widget,
				gint src,
				gint dest,
				gpointer data );

static void
constructed(GObject *object)
{
	G_OBJECT_CLASS(celluloid_view_parent_class)->constructed(object);

	CelluloidView *view =
		CELLULOID_VIEW(object);
	CelluloidMainWindow *wnd =
		CELLULOID_MAIN_WINDOW(view);
	CelluloidVideoArea *video_area =
		celluloid_main_window_get_video_area(wnd);
	CelluloidPlaylistWidget *playlist =
		celluloid_main_window_get_playlist(wnd);
	CelluloidControlBox *control_box =
		celluloid_main_window_get_control_box(wnd);

	g_object_bind_property(	view, "chapter-list",
				control_box, "chapter-list",
				G_BINDING_DEFAULT );
	g_object_bind_property(	view, "duration",
				control_box, "duration",
				G_BINDING_DEFAULT );
	g_object_bind_property(	view, "pause",
				control_box, "pause",
				G_BINDING_DEFAULT );
	g_object_bind_property(	view, "skip-enabled",
				control_box, "skip-enabled",
				G_BINDING_DEFAULT );
	g_object_bind_property(	view, "volume",
				control_box, "volume",
				G_BINDING_BIDIRECTIONAL );
	g_object_bind_property(	view, "volume-max",
				control_box, "volume-max",
				G_BINDING_DEFAULT );
	g_object_bind_property(	playlist, "playlist-count",
				view, "playlist-count",
				G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE );
	g_object_bind_property(	playlist, "searching",
				view, "searching",
				G_BINDING_BIDIRECTIONAL );
	g_object_bind_property(	view, "loop-file",
				playlist, "loop-file",
				G_BINDING_BIDIRECTIONAL );
	g_object_bind_property(	view, "loop",
				playlist, "loop-playlist",
				G_BINDING_BIDIRECTIONAL );
	g_object_bind_property(	view, "shuffle",
				playlist, "shuffle",
				G_BINDING_BIDIRECTIONAL );

	celluloid_main_window_load_state(wnd);
	load_settings(view);

	g_signal_connect(	video_area,
				"resize",
				G_CALLBACK(resize_handler),
				view );
	g_signal_connect(	video_area,
				"render",
				G_CALLBACK(render_handler),
				view );

	GType types[] = {GDK_TYPE_FILE_LIST, G_TYPE_STRING};

	GtkDropTarget *video_area_drop_target =
		gtk_drop_target_new(G_TYPE_INVALID, GDK_ACTION_COPY);

	gtk_drop_target_set_gtypes
		(video_area_drop_target, types, G_N_ELEMENTS(types));
	gtk_widget_add_controller
		(	GTK_WIDGET(video_area),
			GTK_EVENT_CONTROLLER(video_area_drop_target) );
	g_signal_connect(	video_area_drop_target,
				"drop",
				G_CALLBACK(drop_handler),
				view );

	GtkDropTarget *playlist_drop_target =
		gtk_drop_target_new(G_TYPE_INVALID, GDK_ACTION_COPY);
	gtk_drop_target_set_gtypes
		(playlist_drop_target, types, G_N_ELEMENTS(types));
	gtk_widget_add_controller
		(	GTK_WIDGET(playlist),
			GTK_EVENT_CONTROLLER(playlist_drop_target) );
	g_signal_connect(	playlist_drop_target,
				"drop",
				G_CALLBACK(drop_handler),
				view );

	g_signal_connect(	wnd,
				"close-request",
				G_CALLBACK(close_request_handler),
				view );
	g_signal_connect(	playlist,
				"row-activated",
				G_CALLBACK(playlist_row_activated_handler),
				view );
	g_signal_connect(	playlist,
				"row-inserted",
				G_CALLBACK(playlist_row_inserted_handler),
				view );
	g_signal_connect(	playlist,
				"row-deleted",
				G_CALLBACK(playlist_row_deleted_handler),
				view );
	g_signal_connect(	playlist,
				"rows-reordered",
				G_CALLBACK(playlist_row_reordered_handler),
				view );
}

static void
dispose(GObject *object)
{
	CelluloidView *view = CELLULOID_VIEW(object);

	if(!view->disposed)
	{
		celluloid_main_window_save_state(CELLULOID_MAIN_WINDOW(object));
		view->disposed = TRUE;
	}

	G_OBJECT_CLASS(celluloid_view_parent_class)->dispose(object);
}

static void
set_property(	GObject *object,
		guint property_id,
		const GValue *value,
		GParamSpec *pspec )
{
	CelluloidView *self =
		CELLULOID_VIEW(object);
	CelluloidMainWindow *wnd =
		CELLULOID_MAIN_WINDOW(self);
	CelluloidControlBox* control_box =
		celluloid_main_window_get_control_box(wnd);

	switch(property_id)
	{
		case PROP_PLAYLIST_COUNT:
		self->playlist_count = g_value_get_int(value);

		update_title(self);

		g_object_set(	control_box,
				"enabled",
				self->playlist_count > 0,
				NULL );

		if(self->playlist_count <= 0)
		{
			celluloid_view_reset(self);
		}
		break;

		case PROP_PAUSE:
		self->pause = g_value_get_boolean(value);
		break;

		case PROP_IDLE_ACTIVE:
		self->idle_active = g_value_get_boolean(value);
		update_title(self);
		break;

		case PROP_VOLUME:
		self->volume = g_value_get_double(value);
		break;

		case PROP_VOLUME_MAX:
		self->volume_max = g_value_get_double(value);
		break;

		case PROP_DURATION:
		self->duration = g_value_get_double(value);
		break;

		case PROP_PLAYLIST_POS:
		self->playlist_pos = g_value_get_int(value);

		CelluloidPlaylistWidget *playlist =
			celluloid_main_window_get_playlist(wnd);

		celluloid_playlist_widget_set_indicator_pos
			(playlist, self->playlist_pos);
		break;

		case PROP_CHAPTER_LIST:
		self->chapter_list = g_value_get_pointer(value);
		// TODO: run updates
		break;

		case PROP_TRACK_LIST:
		self->track_list = g_value_get_pointer(value);
		celluloid_main_window_update_track_list(wnd, self->track_list);
		break;

		case PROP_DISC_LIST:
		self->disc_list = g_value_get_pointer(value);
		celluloid_main_window_update_disc_list(wnd, self->disc_list);
		break;

		case PROP_SKIP_ENABLED:
		self->skip_enabled = g_value_get_boolean(value);
		break;

		case PROP_LOOP_FILE:
		self->loop_file = g_value_get_boolean(value);
		break;

		case PROP_LOOP:
		self->loop = g_value_get_boolean(value);
		break;

		case PROP_SHUFFLE:
		self->shuffle = g_value_get_boolean(value);
		break;

		case PROP_MEDIA_TITLE:
		g_free(self->media_title);
		self->media_title = g_value_dup_string(value);
		update_title(self);
		break;

		case PROP_DISPLAY_FPS:
		self->display_fps = g_value_get_double(value);
		break;

		case PROP_SEARCHING:
		// We do not display playlist in fullscreen mode so refuse to
		// enter search mode here if we're in fullscreen mode.
		self->searching =
			!gtk_window_is_fullscreen(GTK_WINDOW(self)) &&
			g_value_get_boolean(value);

		// Show playlist if we're about to enter search mode and it
		// isn't already visible.
		if(self->searching && !celluloid_view_get_playlist_visible(self))
		{
			celluloid_view_set_playlist_visible(self, TRUE);
		}
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
get_property(	GObject *object,
		guint property_id,
		GValue *value,
		GParamSpec *pspec )
{
	CelluloidView *self = CELLULOID_VIEW(object);

	switch(property_id)
	{
		case PROP_PLAYLIST_COUNT:
		g_value_set_int(value, self->playlist_count);
		break;

		case PROP_PAUSE:
		g_value_set_boolean(value, self->pause);
		break;

		case PROP_IDLE_ACTIVE:
		g_value_set_boolean(value, self->idle_active);
		break;

		case PROP_VOLUME:
		g_value_set_double(value, self->volume);
		break;

		case PROP_VOLUME_MAX:
		g_value_set_double(value, self->volume_max);
		break;

		case PROP_DURATION:
		g_value_set_double(value, self->duration);
		break;

		case PROP_PLAYLIST_POS:
		g_value_set_int(value, self->playlist_pos);
		break;

		case PROP_CHAPTER_LIST:
		g_value_set_pointer(value, self->chapter_list);
		break;

		case PROP_TRACK_LIST:
		g_value_set_pointer(value, self->track_list);
		break;

		case PROP_DISC_LIST:
		g_value_set_pointer(value, self->disc_list);
		break;

		case PROP_SKIP_ENABLED:
		g_value_set_boolean(value, self->skip_enabled);
		break;

		case PROP_LOOP_FILE:
		g_value_set_boolean(value, self->loop_file);
		break;

		case PROP_LOOP:
		g_value_set_boolean(value, self->loop);
		break;

		case PROP_SHUFFLE:
		g_value_set_boolean(value, self->shuffle);
		break;

		case PROP_MEDIA_TITLE:
		g_value_set_static_string(value, self->media_title);
		break;

		case PROP_DISPLAY_FPS:
		g_value_set_double(value, self->display_fps);
		break;

		case PROP_SEARCHING:
		g_value_set_boolean(value, self->searching);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
load_css(CelluloidView *view)
{
	const gchar *style =
		"celluloid-video-area"
		"{"
		"	background-color: var(--view-bg-color);"
		"}"
		"celluloid-seek-bar slider"
		"{"
		"	border-radius: 100%;"
		"}"
		".dialog-action-area"
		"{"
		"	margin: -12px 12px 12px 0px;"
		"}"
		".dialog-action-area > button"
		"{"
		"	margin-left: 12px;"
		"}"
		".osd.docked"
		"{"
		"	border-radius: 0px;"
		"}"
		".control-box.osd.undocked"
		"{"
		"	margin: 12px 12px 12px 12px;"
		"}"
		".control-box.osd.docked"
		"{"
		"	margin: 0px 0px 0px 0px;"
		"}"
		".osd, .floating-header windowcontrols image"
		"{"
		"	border: 1px solid var(--border-color);"
		"}";

	GtkCssProvider *style_provider = gtk_css_provider_new();

	gtk_css_provider_load_from_string(style_provider, style);

	GdkSurface *surface = gtk_widget_get_surface(view);

	if(surface)
	{
		GdkDisplay *display = gdk_surface_get_display(surface);

		gtk_style_context_add_provider_for_display
			(	display,
				GTK_STYLE_PROVIDER(style_provider),
				GTK_STYLE_PROVIDER_PRIORITY_APPLICATION );

		g_object_unref(style_provider);
	}
	else
	{
		g_warning("Failed to load CSS: Surface not available");
	}
}

static void
load_settings(CelluloidView *view)
{
	GSettings *settings =
		g_settings_new(CONFIG_ROOT);
	CelluloidMainWindow *wnd =
		CELLULOID_MAIN_WINDOW(view);
	CelluloidControlBox *control_box =
		celluloid_main_window_get_control_box(wnd);

	g_object_set(	control_box,
			"skip-enabled", FALSE,
			NULL );

	g_object_unref(settings);
}

static void
save_playlist(CelluloidView *view, GFile *file, GError **error)
{
	CelluloidMainWindow *wnd = CELLULOID_MAIN_WINDOW(view);
	CelluloidPlaylistWidget *wgt = celluloid_main_window_get_playlist(wnd);
	GPtrArray *playlist = celluloid_playlist_widget_get_contents(wgt);
	gboolean rc = TRUE;
	GOutputStream *dest_stream = NULL;

	if(file)
	{
		GFileOutputStream *file_stream;

		file_stream = g_file_replace(	file,
						NULL,
						FALSE,
						G_FILE_CREATE_NONE,
						NULL,
						error );
		dest_stream = G_OUTPUT_STREAM(file_stream);
		rc = !!dest_stream;
	}

	for(guint i = 0; rc && i < playlist->len; i++)
	{
		gsize written;

		CelluloidPlaylistEntry *entry = g_ptr_array_index(playlist, i);

		rc = g_output_stream_printf(	dest_stream,
						&written,
						NULL,
						NULL,
						"%s\n",
						entry->filename);
	}

	if(dest_stream)
	{
		g_output_stream_close(dest_stream, NULL, error);
	}

	g_ptr_array_free(playlist, TRUE);
}

static void
update_title(CelluloidView *view)
{
	const gboolean use_media_title =
		view->media_title &&
		!view->idle_active &&
		view->playlist_count > 0;
	const gchar *title_source =
		use_media_title ?
		view->media_title :
		g_get_application_name();
	gchar *title =
		sanitize_utf8(title_source, TRUE);

	CelluloidMainWindow *wnd =
		CELLULOID_MAIN_WINDOW(view);
	CelluloidVideoArea *video_area =
		celluloid_main_window_get_video_area(wnd);
	CelluloidControlBox *control_box =
		celluloid_video_area_get_control_box
		(CELLULOID_VIDEO_AREA(video_area));

	celluloid_control_box_set_title(control_box, title);
	gtk_window_set_title(GTK_WINDOW(view), title);
	g_free(title);
}

void
show_message_dialog(CelluloidView *view, const gchar *prefix, const gchar *msg)
{
	GtkAlertDialog *dialog = NULL;

	if(view->has_dialog)
	{
		g_info("Error dialog suppressed because one is already being shown");

		if(prefix)
		{
			g_info("Content: [%s] %s", prefix, msg);
		}
		else
		{
			g_info("Content: %s", msg);
		}
	}
	else if(prefix)
	{
		gchar *prefix_escaped = g_markup_printf_escaped("%s", prefix);
		gchar *msg_escaped = g_markup_printf_escaped("%s", msg);

		dialog =	gtk_alert_dialog_new
				("[%s] %s", prefix_escaped, msg_escaped);

		g_free(prefix_escaped);
		g_free(msg_escaped);
	}
	else
	{
		dialog = gtk_alert_dialog_new("%s", msg);
	}

	if(dialog)
	{
		view->has_dialog = TRUE;

		gtk_alert_dialog_show(dialog, GTK_WINDOW(view));
	}
}

static void
show_open_track_dialog(CelluloidView  *view, TrackType type)
{
	CelluloidFileDialog *dialog = celluloid_file_dialog_new(TRUE);

	switch(type)
	{
		case TRACK_TYPE_AUDIO:
		celluloid_file_dialog_set_title
			(dialog, _("Load Audio Track…"));
		break;

		case TRACK_TYPE_VIDEO:
		celluloid_file_dialog_set_title
			(dialog, _("Load Video Track…"));
		break;

		case TRACK_TYPE_SUBTITLE:
		celluloid_file_dialog_set_title
			(dialog, _("Load Subtitle Track…"));
		break;

		default:
		g_assert_not_reached();
		break;
	}

	g_object_set_data(	G_OBJECT(dialog),
				"track-type",
				(gpointer)type );

	celluloid_file_dialog_set_default_filters
		(	dialog,
			type == TRACK_TYPE_AUDIO,
			type == TRACK_TYPE_VIDEO,
			FALSE,
			type == TRACK_TYPE_SUBTITLE );
	celluloid_file_dialog_open_multiple
		(	dialog,
			GTK_WINDOW(view),
			NULL,
			open_track_dialog_callback,
			view );
}

static void
open_dialog_callback(	GObject *source_object,
			GAsyncResult *res,
			gpointer data )
{
	CelluloidFileDialog *dialog =
		CELLULOID_FILE_DIALOG(source_object);
	CelluloidView *view =
		CELLULOID_VIEW(data);
	const gboolean append =
		GPOINTER_TO_INT(g_object_get_data(source_object, "append"));
	const gboolean select_folder =
		GPOINTER_TO_INT(g_object_get_data(source_object, "select-folder"));

	GListModel *files = NULL;

	if(select_folder)
	{
		GFile *folder =
			celluloid_file_dialog_select_folder_finish
			(dialog, res, NULL);

		if(folder)
		{
			GListStore *files_store = g_list_store_new(G_TYPE_FILE);
			g_list_store_append(files_store, folder);

			files = G_LIST_MODEL(files_store);

			g_object_unref(folder);
		}
	}
	else
	{
		files =	celluloid_file_dialog_open_multiple_finish
			(dialog, res, NULL);
	}

	if(files)
	{
		g_signal_emit_by_name(view, "file-open", files, append);
		g_object_unref(files);
	}

	g_object_unref(dialog);
}

static void
open_location_dialog_response_handler(	GtkDialog *dialog,
					gint response_id,
					gpointer data )
{
	if(response_id == GTK_RESPONSE_ACCEPT)
	{
		CelluloidOpenLocationDialog *location_dialog =
			CELLULOID_OPEN_LOCATION_DIALOG(dialog);
		const gchar *uri =
			celluloid_open_location_dialog_get_string
			(location_dialog);

		GPtrArray *args = data;
		CelluloidView *view = g_ptr_array_index(args, 0);
		gboolean *append = g_ptr_array_index(args, 1);

		GFile *file = g_file_new_for_uri(uri);
		GListStore *list = g_list_store_new(G_TYPE_OBJECT);

		g_list_store_append(list, file);

		g_signal_emit_by_name(view, "file-open", list, *append);

		g_object_unref(list);
		g_free(append);
		g_ptr_array_free(args, TRUE);
	}

	gtk_window_close(GTK_WINDOW(dialog));
}

static void
open_track_dialog_callback(	GObject *source_object,
				GAsyncResult *res,
				gpointer data )
{
	CelluloidFileDialog *dialog =
		CELLULOID_FILE_DIALOG(source_object);
	TrackType type =
		(TrackType)g_object_get_data(G_OBJECT(dialog), "track-type");
	const gchar *name =
		NULL;

	switch(type)
	{
		case TRACK_TYPE_AUDIO:
		name = "audio-track-load";
		break;

		case TRACK_TYPE_VIDEO:
		name = "video-track-load";
		break;

		case TRACK_TYPE_SUBTITLE:
		name = "subtitle-track-load";
		break;

		default:
		g_assert_not_reached();
		break;
	}

	CelluloidView *view =
		CELLULOID_VIEW(data);
	GListModel *files =
		celluloid_file_dialog_open_multiple_finish(dialog, res, NULL);

	if(files)
	{
		const guint n_items =
			g_list_model_get_n_items(files);

		for(guint i = 0; i < n_items; i++)
		{
			GFile *file = g_list_model_get_item(files, i);
			gchar *uri = g_file_get_uri(file);

			g_signal_emit_by_name(view, name, uri);
			g_free(uri);
		}

		g_object_unref(files);
	}

	g_object_unref(dialog);
}

static void
save_playlist_callback(	GObject *source_object,
			GAsyncResult *res,
			gpointer data )
{
	CelluloidFileDialog *dialog = CELLULOID_FILE_DIALOG(source_object);
	CelluloidView *view = CELLULOID_VIEW(data);
	GError *error = NULL;
	GFile *file = celluloid_file_dialog_save_finish(dialog, res, &error);

	if(file)
	{
		save_playlist(view, file, &error);
		g_object_unref(file);
	}

	if(error)
	{
		show_message_dialog(view, NULL, error->message);
		g_error_free(error);
	}
}

static gboolean
mpv_reset_request_handler(AdwPreferencesWindow *dialog, gpointer data)
{
	CelluloidMainWindow *wnd;
	GSettings *settings;
	gboolean csd_enable;

	wnd = CELLULOID_MAIN_WINDOW(data);
	settings = g_settings_new(CONFIG_ROOT);
	csd_enable = g_settings_get_boolean(settings, "csd-enable");

	if(celluloid_main_window_get_csd_enabled(wnd) != csd_enable)
	{
		show_message_dialog(	CELLULOID_VIEW(data),
					NULL,
					_("Enabling or disabling "
					"client-side decorations "
					"requires restarting to "
					"take effect.") );
	}

	gtk_widget_queue_draw(GTK_WIDGET(wnd));
	g_signal_emit_by_name(data, "mpv-reset-request");

	g_object_unref(settings);

	return FALSE;
}

static void
error_handler(	CelluloidPreferencesDialog *dialog,
		const gchar *message,
		gpointer data )
{
	CelluloidView *view = CELLULOID_VIEW(data);

	show_message_dialog(view, NULL, message);
}

static void
realize_handler(GtkWidget *widget, gpointer data)
{
	CelluloidView *view = CELLULOID_VIEW(widget);
	GdkSurface *surface = gtk_widget_get_surface(view);

	g_signal_connect(	surface,
				"notify::mapped",
				G_CALLBACK(notify_mapped_handler),
				widget );

	load_css(view);
}

static void
notify_mapped_handler(GdkSurface *surface, GParamSpec *pspec, gpointer data)
{
	g_signal_handlers_disconnect_by_func(	surface,
						notify_mapped_handler,
						data );

	g_signal_emit_by_name(CELLULOID_VIEW(data), "ready");
}

static void
resize_handler(	CelluloidVideoArea *video_area,
		gint width,
		gint height,
		gpointer data )
{
	g_signal_emit_by_name(data, "video-area-resize", width, height);
}

static void
render_handler(CelluloidVideoArea *area, gpointer data)
{
	g_signal_emit_by_name(data, "render");
}

static gboolean
drop_handler(	GtkDropTarget *self,
		GValue *value,
		gdouble x,
		gdouble y,
		gpointer data )
{
	CelluloidView *view = CELLULOID_VIEW(data);
	GtkEventController *controller = GTK_EVENT_CONTROLLER(self);
	GtkWidget *source = gtk_event_controller_get_widget(controller);
	const gboolean append = CELLULOID_IS_PLAYLIST_WIDGET(source);
	GListStore *files = NULL;

	g_debug("Received drop event with value type %s", g_type_name(G_VALUE_TYPE(value)) );

	if(G_VALUE_HOLDS_STRING(value))
	{
		const gchar *string = g_value_get_string(value);
		gchar **uris = g_uri_list_extract_uris(string);

		if(uris)
		{
			files = g_list_store_new(G_TYPE_FILE);

			for(gint i = 0; uris[i]; i++)
			{
				GFile *file = g_file_new_for_uri(uris[i]);

				g_list_store_append(files, file);
				g_object_unref(file);
			}

			g_strfreev(uris);
		}
		else
		{
			g_warning("Failed to extract any URIs from dropped string");
		}
	}
	else if(G_VALUE_HOLDS(value, GDK_TYPE_FILE_LIST))
	{
		const GSList *slist = g_value_get_boxed(value);

		files = g_list_store_new(G_TYPE_FILE);

		for(const GSList *cur = slist; cur; cur = cur->next)
		{
			g_list_store_append(files, G_FILE(cur->data));
		}
	}
	else
	{
		g_error(	"Cannot handle drop event with value type %s",
				g_type_name(G_VALUE_TYPE(value)) );
	}

	if(files)
	{
		g_signal_emit_by_name(view, "file-open", files, append);
		g_object_unref(files);
	}
	else
	{
		g_warning("Failed to open anything from drop event");
	}

	return TRUE;
}

static gboolean
close_request_handler(GtkWidget *widget, gpointer data)
{
	if(!celluloid_main_window_get_fullscreen(CELLULOID_MAIN_WINDOW(widget)))
	{
		celluloid_main_window_save_state(CELLULOID_MAIN_WINDOW(widget));
	}

	return FALSE;
}

static void
playlist_row_activated_handler(	CelluloidPlaylistWidget *playlist,
				gint64 pos,
				gpointer data )
{
	g_signal_emit_by_name(data, "playlist-item-activated", pos);
}

static void
playlist_row_inserted_handler(	CelluloidPlaylistWidget *widget,
				gint pos,
				gpointer data )
{
	g_signal_emit_by_name(data, "playlist-item-inserted", pos);
}

static void
playlist_row_deleted_handler(	CelluloidPlaylistWidget *widget,
				gint pos,
				gpointer data )
{
	if(celluloid_playlist_widget_empty(widget))
	{
		celluloid_view_reset(data);
	}

	g_signal_emit_by_name(data, "playlist-item-deleted", pos);
}

static void
playlist_row_reordered_handler(	CelluloidPlaylistWidget *widget,
				gint src,
				gint dest,
				gpointer data )
{
	g_signal_emit_by_name(data, "playlist-reordered", src, dest);
}

static void
celluloid_view_class_init(CelluloidViewClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec = NULL;

	object_class->constructed = constructed;
	object_class->dispose = dispose;
	object_class->set_property = set_property;
	object_class->get_property = get_property;

	pspec = g_param_spec_int
		(	"playlist-count",
			"Playlist count",
			"The number of items in playlist",
			0,
			G_MAXINT,
			0,
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_PLAYLIST_COUNT, pspec);

	pspec = g_param_spec_boolean
		(	"pause",
			"Pause",
			"Whether or not the player is paused",
			FALSE,
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_PAUSE, pspec);

	pspec = g_param_spec_boolean
		(	"idle-active",
			"Idle active",
			"Whether or not the player is idle",
			TRUE,
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_IDLE_ACTIVE, pspec);

	pspec = g_param_spec_double
		(	"volume",
			"Volume",
			"The volume the volume button is set to",
			0.0,
			G_MAXDOUBLE,
			0.0,
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_VOLUME, pspec);

	pspec = g_param_spec_double
		(	"volume-max",
			"Volume Max",
			"The maximum amplification level",
			0.0,
			G_MAXDOUBLE,
			100.0,
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_VOLUME_MAX, pspec);

	pspec = g_param_spec_double
		(	"duration",
			"Duration",
			"The duration the seek bar is using",
			0.0,
			G_MAXDOUBLE,
			0.0,
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_DURATION, pspec);

	pspec = g_param_spec_int
		(	"playlist-pos",
			"Playlist position",
			"The position of the current item in playlist",
			-1,
			G_MAXINT,
			-1,
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_PLAYLIST_POS, pspec);

	pspec = g_param_spec_pointer
		(	"chapter-list",
			"Chapter list",
			"The list of chapters in the playing file",
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_CHAPTER_LIST, pspec);

	pspec = g_param_spec_pointer
		(	"track-list",
			"Track list",
			"The list of tracks in the playing file",
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_TRACK_LIST, pspec);

	pspec = g_param_spec_pointer
		(	"disc-list",
			"Disc list",
			"The list of mounted discs",
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_DISC_LIST, pspec);

	pspec = g_param_spec_boolean
		(	"skip-enabled",
			"Skip enabled",
			"Whether or not skip buttons are shown",
			FALSE,
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_SKIP_ENABLED, pspec);

	pspec = g_param_spec_boolean
		(	"loop-file",
			"Loop file",
			"Whether or not the the loop file button is active",
			FALSE,
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_LOOP_FILE, pspec);

	pspec = g_param_spec_boolean
		(	"loop",
			"Loop",
			"Whether or not the the loop button is active",
			FALSE,
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_LOOP, pspec);

	pspec = g_param_spec_boolean
		(	"shuffle",
			"Shuffle",
			"Whether or not the the shuffle button is active",
			FALSE,
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_SHUFFLE, pspec);

	pspec = g_param_spec_string
		(	"media-title",
			"Media Title",
			"The title of the media being played",
			NULL,
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_MEDIA_TITLE, pspec);

	pspec = g_param_spec_boolean
		(	"searching",
			"Searching",
			"Whether or not the user is searching the playlist",
			FALSE,
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_SEARCHING, pspec);

	g_signal_new(	"video-area-resize",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST|G_SIGNAL_DETAILED,
			0,
			NULL,
			NULL,
			g_cclosure_gen_marshal_VOID__INT_INT,
			G_TYPE_NONE,
			2,
			G_TYPE_INT,
			G_TYPE_INT );

	/* Controls-related signals */
	g_signal_new(	"volume-changed",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__DOUBLE,
			G_TYPE_NONE,
			1,
			G_TYPE_DOUBLE );
	g_signal_new(	"ready",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0 );
	g_signal_new(	"render",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0 );
	g_signal_new(	"mpv-reset-request",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0 );
	g_signal_new(	"audio-track-load",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__STRING,
			G_TYPE_NONE,
			1,
			G_TYPE_STRING );
	g_signal_new(	"video-track-load",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__STRING,
			G_TYPE_NONE,
			1,
			G_TYPE_STRING );
	g_signal_new(	"subtitle-track-load",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__STRING,
			G_TYPE_NONE,
			1,
			G_TYPE_STRING );
	g_signal_new(	"file-open",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_gen_marshal_VOID__POINTER_BOOLEAN,
			G_TYPE_NONE,
			2,
			G_TYPE_POINTER,
			G_TYPE_BOOLEAN );
	g_signal_new(	"playlist-item-activated",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__INT,
			G_TYPE_NONE,
			1,
			G_TYPE_INT );
	g_signal_new(	"playlist-item-inserted",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__INT,
			G_TYPE_NONE,
			1,
			G_TYPE_INT );
	g_signal_new(	"playlist-item-deleted",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__INT,
			G_TYPE_NONE,
			1,
			G_TYPE_INT );
	g_signal_new(	"playlist-reordered",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_gen_marshal_VOID__INT_INT,
			G_TYPE_NONE,
			2,
			G_TYPE_INT,
			G_TYPE_INT );
}

static void
celluloid_view_init(CelluloidView *view)
{
	view->disposed = FALSE;
	view->has_dialog = FALSE;
	view->playlist_count = 0;
	view->pause = FALSE;
	view->idle_active = FALSE;
	view->volume = 0.0;
	view->volume_max = 100.0;
	view->duration = 0.0;
	view->playlist_pos = 0;
	view->chapter_list = NULL;
	view->track_list = NULL;
	view->disc_list = NULL;
	view->skip_enabled = FALSE;
	view->loop_file = FALSE;
	view->loop = FALSE;
	view->shuffle = FALSE;
	view->media_title = NULL;
	view->display_fps = 0;
	view->searching = FALSE;

	g_signal_connect(view, "realize", G_CALLBACK(realize_handler), NULL);
}

CelluloidView *
celluloid_view_new(CelluloidApplication *app, gboolean always_floating)
{
	GObject *obj = g_object_new(	celluloid_view_get_type(),
					"application",
					app,
					"always-use-floating-controls",
					always_floating,
					NULL );
	CelluloidView *view = CELLULOID_VIEW(obj);
	GtkApplication *gtk_app = GTK_APPLICATION(app);
	GSettings *settings = g_settings_new(CONFIG_ROOT);

	if(g_settings_get_boolean(settings, "csd-enable"))
	{
		celluloid_main_window_enable_csd(CELLULOID_MAIN_WINDOW(view));
	}
	else
	{
		GMenu *full_menu = g_menu_new();

		celluloid_menu_build_full(full_menu, NULL, NULL);
		gtk_application_set_menubar(gtk_app, G_MENU_MODEL(full_menu));
	}

	g_object_unref(settings);

	return view;
}

CelluloidMainWindow *
celluloid_view_get_main_window(CelluloidView *view)
{
	return CELLULOID_MAIN_WINDOW(view);
}

void
celluloid_view_show_open_dialog(	CelluloidView *view,
					gboolean select_folder,
					gboolean append )
{
	CelluloidFileDialog *dialog = celluloid_file_dialog_new(TRUE);

	g_object_set_data(	G_OBJECT(dialog),
				"append",
				GINT_TO_POINTER(append) );
	g_object_set_data(	G_OBJECT(dialog),
				"select-folder",
				GINT_TO_POINTER(select_folder) );

	if(select_folder)
	{
		celluloid_file_dialog_set_title
			(	dialog,
				append ?
				_("Add Folder to Playlist") :
				_("Open Folder") );
		celluloid_file_dialog_select_folder
			(	dialog,
				GTK_WINDOW(view),
				NULL,
				open_dialog_callback,
				view );
	}
	else
	{
		celluloid_file_dialog_set_default_filters
			(dialog, TRUE, TRUE, TRUE, TRUE);

		celluloid_file_dialog_set_title
			(	dialog,
				append ?
				_("Add File to Playlist") :
				_("Open File") );
		celluloid_file_dialog_open_multiple
			(	dialog,
				GTK_WINDOW(view),
				NULL,
				open_dialog_callback,
				view );
	}
}

void
celluloid_view_show_open_location_dialog(CelluloidView *view, gboolean append)
{
	GtkWidget *dlg;
	GPtrArray *args;
	gboolean *append_buf;

	dlg =	celluloid_open_location_dialog_new
		(	GTK_WINDOW(view),
			append?
			_("Add Location to Playlist"):
			_("Open Location") );
	args = g_ptr_array_sized_new(2);
	append_buf = g_malloc(sizeof(gboolean));
	*append_buf = append;

	g_ptr_array_add(args, view);
	g_ptr_array_add(args, append_buf);

	g_signal_connect
		(	dlg,
			"response",
			G_CALLBACK(open_location_dialog_response_handler),
			args );

	gtk_widget_set_visible(dlg, TRUE);
}

void
celluloid_view_show_open_audio_track_dialog(CelluloidView *view)
{
	show_open_track_dialog(view, TRACK_TYPE_AUDIO);
}

void
celluloid_view_show_open_video_track_dialog(CelluloidView *view)
{
	show_open_track_dialog(view, TRACK_TYPE_VIDEO);
}

void
celluloid_view_show_open_subtitle_track_dialog(CelluloidView *view)
{
	show_open_track_dialog(view, TRACK_TYPE_SUBTITLE);
}

void
celluloid_view_show_save_playlist_dialog(CelluloidView *view)
{
	CelluloidFileDialog *dialog = celluloid_file_dialog_new(TRUE);

	celluloid_file_dialog_set_initial_name
		(	dialog,
			"playlist.m3u" );
	celluloid_file_dialog_save
		(	dialog,
			GTK_WINDOW(view),
			NULL,
			save_playlist_callback,
			view );
}

void
celluloid_view_show_preferences_dialog(CelluloidView *view)
{
	GtkWidget *dialog = celluloid_preferences_dialog_new(GTK_WINDOW(view));

	g_signal_connect_after(	dialog,
				"mpv-reset-request",
				G_CALLBACK(mpv_reset_request_handler),
				view );
	g_signal_connect(	dialog,
				"error-raised",
				G_CALLBACK(error_handler),
				view );

	celluloid_preferences_dialog_present
		(CELLULOID_PREFERENCES_DIALOG(dialog));
}

void
celluloid_view_show_shortcuts_dialog(CelluloidView *view)
{
	GtkWidget *wnd = celluloid_shortcuts_window_new(GTK_WINDOW(view));

	gtk_widget_set_visible(wnd, TRUE);
}

void
celluloid_view_show_about_window (CelluloidView *view)
{
	const gchar *const authors[] = AUTHORS;

	adw_show_about_dialog(	GTK_WIDGET(view),
				"application-icon",
				ICON_NAME,
				"application-name",
				g_get_application_name(),
				"developer-name",
				"The Celluloid Developers",
				"version",
				VERSION,
				"website",
				"https://celluloid-player.github.io/",
				"issue-url",
				"https://celluloid-player.github.io/bug-reports.html",
				"license-type",
				GTK_LICENSE_GPL_3_0,
				"copyright",
				"\u00A9 2014-2023 The Celluloid authors",
				"developers",
				authors,
				"translator-credits",
				_("translator-credits"),
				NULL );
}

void
celluloid_view_show_message_toast(CelluloidView *view, const gchar *msg)
{
	CelluloidMainWindow *wnd =
		CELLULOID_MAIN_WINDOW(view);
	CelluloidVideoArea *video_area =
		celluloid_main_window_get_video_area(wnd);

	celluloid_video_area_show_toast_message(video_area, msg);
}

void
celluloid_view_present(CelluloidView *view)
{
	gtk_window_present(GTK_WINDOW(view));
}

void
celluloid_view_quit(CelluloidView *view)
{
	gtk_window_close(GTK_WINDOW(view));
}

void
celluloid_view_reset(CelluloidView *view)
{
	celluloid_main_window_reset(CELLULOID_MAIN_WINDOW(view));
}

void
celluloid_view_queue_render(CelluloidView *view)
{
	CelluloidMainWindow *wnd = CELLULOID_MAIN_WINDOW(view);
	CelluloidVideoArea *area = celluloid_main_window_get_video_area(wnd);

	celluloid_video_area_queue_render(area);
}

void
celluloid_view_make_gl_context_current(CelluloidView *view)
{
	CelluloidMainWindow *wnd =
		CELLULOID_MAIN_WINDOW(view);
	CelluloidVideoArea *video_area =
		celluloid_main_window_get_video_area(wnd);
	GtkGLArea *gl_area =
		celluloid_video_area_get_gl_area(video_area);

	gtk_widget_realize(GTK_WIDGET(gl_area));
	gtk_gl_area_make_current(gl_area);
}

gint
celluloid_view_get_scale_factor(CelluloidView *view)
{
	GdkSurface *surface = gtk_widget_get_surface(view);

	return gdk_surface_get_scale_factor(surface);
}

void
celluloid_view_get_video_area_geometry(	CelluloidView *view,
					gint *width,
					gint *height )
{
	CelluloidMainWindow *wnd = CELLULOID_MAIN_WINDOW(view);
	CelluloidVideoArea *area = celluloid_main_window_get_video_area(wnd);
	GtkGLArea *gl_area = celluloid_video_area_get_gl_area(area);
	*width = gtk_widget_get_width(GTK_WIDGET(gl_area));
	*height = gtk_widget_get_height(GTK_WIDGET(gl_area));
}

void
celluloid_view_move(	CelluloidView *view,
			gboolean flip_x,
			gboolean flip_y,
			GValue *x,
			GValue *y )
{
#ifdef GDK_WINDOWING_WAYLAND
	GdkDisplay *display = gdk_display_get_default();

	if (GDK_IS_WAYLAND_DISPLAY(display))
	{
		g_warning("%s: Not supported on Wayland", __func__);
		return;
	}
#endif

	g_warning("%s: Not implemented", __func__);
}

void
celluloid_view_resize_video_area(CelluloidView *view, gint width, gint height)
{
	celluloid_main_window_resize_video_area
		(CELLULOID_MAIN_WINDOW(view), width, height);
}

void
celluloid_view_set_fullscreen(CelluloidView *view, gboolean fullscreen)
{
	g_object_set(view, "fullscreened", fullscreen, NULL);
}

void
celluloid_view_set_time_position(CelluloidView *view, gdouble position)
{
	CelluloidControlBox *control_box;

	control_box =	celluloid_main_window_get_control_box
			(CELLULOID_MAIN_WINDOW(view));
	g_object_set(control_box, "time-position", position, NULL);
}

void
celluloid_view_update_playlist(CelluloidView *view, GPtrArray *playlist)
{
	CelluloidMainWindow *wnd = CELLULOID_MAIN_WINDOW(view);
	CelluloidPlaylistWidget *wgt = celluloid_main_window_get_playlist(wnd);

	celluloid_playlist_widget_update_contents(wgt, playlist);
}

void
celluloid_view_set_playlist_pos(CelluloidView *view, gint64 pos)
{
	g_object_set(view, "playlist-pos", (gint)pos, NULL);
}

void
celluloid_view_set_playlist_visible(CelluloidView *view, gboolean visible)
{
	celluloid_main_window_set_playlist_visible
		(CELLULOID_MAIN_WINDOW(view), visible);
}

gboolean
celluloid_view_get_playlist_visible(CelluloidView *view)
{
	return	celluloid_main_window_get_playlist_visible
		(CELLULOID_MAIN_WINDOW(view));
}

void
celluloid_view_set_controls_visible(CelluloidView *view, gboolean visible)
{
	celluloid_main_window_set_controls_visible
		(CELLULOID_MAIN_WINDOW(view), visible);
}

gboolean
celluloid_view_get_controls_visible(CelluloidView *view)
{
	return	celluloid_main_window_get_controls_visible
		(CELLULOID_MAIN_WINDOW(view));
}
