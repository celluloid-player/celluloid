/*
 * Copyright (c) 2017-2019 gnome-mpv
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

#include "celluloid-view.h"
#include "celluloid-file-chooser.h"
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
	PROP_VOLUME,
	PROP_DURATION,
	PROP_PLAYLIST_POS,
	PROP_TRACK_LIST,
	PROP_DISC_LIST,
	PROP_SKIP_ENABLED,
	PROP_LOOP,
	PROP_BORDER,
	PROP_FULLSCREEN,
	PROP_DISPLAY_FPS,
	N_PROPERTIES
};

struct _CelluloidView
{
	CelluloidMainWindow parent;

	/* Properties */
	gint playlist_count;
	gboolean pause;
	gdouble volume;
	gdouble duration;
	gint playlist_pos;
	GPtrArray *track_list;
	GPtrArray *disc_list;
	gboolean skip_enabled;
	gboolean control_box_enabled;
	gboolean loop;
	gboolean border;
	gboolean fullscreen;
	gdouble display_fps;
};

struct _CelluloidViewClass
{
	CelluloidMainWindowClass parent_class;
};

G_DEFINE_TYPE(CelluloidView, celluloid_view, CELLULOID_TYPE_MAIN_WINDOW)

static void
constructed(GObject *object);

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
show_message_dialog(	CelluloidMainWindow *wnd,
			GtkMessageType type,
			const gchar *title,
			const gchar *prefix,
			const gchar *msg );

static void
update_display_fps(CelluloidView *view);

/* Dialog responses */
static void
open_dialog_response_handler(GtkDialog *dialog, gint response_id, gpointer data);

static void
open_location_dialog_response_handler(	GtkDialog *dialog,
					gint response_id,
					gpointer data );

static void
open_audio_track_dialog_response_handler(	GtkDialog *dialog,
						gint response_id,
						gpointer data );
static void
open_subtitle_track_dialog_response_handler(	GtkDialog *dialog,
						gint response_id,
						gpointer data );

static void
preferences_dialog_response_handler(	GtkDialog *dialog,
					gint response_id,
					gpointer data );

static void
realize_handler(GtkWidget *widget, gpointer data);

static void
size_allocate_handler(	GtkWidget *widget,
			GdkRectangle *allocation,
			gpointer data );

static void
render_handler(CelluloidVideoArea *area, gpointer data);

static gboolean
draw_handler(GtkWidget *widget, cairo_t *cr, gpointer data);

static void
drag_data_handler(	GtkWidget *widget,
			GdkDragContext *context,
			gint x,
			gint y,
			GtkSelectionData *sel_data,
			guint info,
			guint time,
			gpointer data );

static gboolean
window_state_handler(GtkWidget *widget, GdkEvent *event, gpointer data);

static void
screen_changed_handler(	GtkWidget *wnd,
			GdkScreen *previous_screen,
			gpointer data );

static gboolean
delete_handler(GtkWidget *widget, GdkEvent *event, gpointer data);

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

	celluloid_main_window_load_state(wnd);
	load_css(view);
	load_settings(view);

	g_object_bind_property(	view, "duration",
				control_box, "duration",
				G_BINDING_DEFAULT );
	g_object_bind_property(	view, "pause",
				control_box, "pause",
				G_BINDING_DEFAULT );
	g_object_bind_property(	view, "skip-enabled",
				control_box, "skip-enabled",
				G_BINDING_DEFAULT );
	g_object_bind_property(	view, "loop",
				control_box, "loop",
				G_BINDING_BIDIRECTIONAL );
	g_object_bind_property(	view, "volume",
				control_box, "volume",
				G_BINDING_BIDIRECTIONAL );
	g_object_bind_property(	playlist, "playlist-count",
				view, "playlist-count",
				G_BINDING_DEFAULT );

	g_signal_connect(	video_area,
				"size-allocate",
				G_CALLBACK(size_allocate_handler),
				view );
	g_signal_connect(	video_area,
				"render",
				G_CALLBACK(render_handler),
				view );
	g_signal_connect(	video_area,
				"drag-data-received",
				G_CALLBACK(drag_data_handler),
				view );
	g_signal_connect(	playlist,
				"drag-data-received",
				G_CALLBACK(drag_data_handler),
				view );
	g_signal_connect(	wnd,
				"window-state-event",
				G_CALLBACK(window_state_handler),
				view );
	g_signal_connect(	wnd,
				"screen-changed",
				G_CALLBACK(screen_changed_handler),
				view );
	g_signal_connect(	wnd,
				"delete-event",
				G_CALLBACK(delete_handler),
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

		case PROP_VOLUME:
		self->volume = g_value_get_double(value);
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

		case PROP_LOOP:
		self->loop = g_value_get_boolean(value);
		break;

		case PROP_BORDER:
		self->border = g_value_get_boolean(value);
		gtk_window_set_decorated(GTK_WINDOW(wnd), self->border);
		break;

		case PROP_FULLSCREEN:
		self->fullscreen = g_value_get_boolean(value);
		celluloid_main_window_set_fullscreen(wnd, self->fullscreen);
		break;

		case PROP_DISPLAY_FPS:
		self->display_fps = g_value_get_double(value);
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

		case PROP_VOLUME:
		g_value_set_double(value, self->volume);
		break;

		case PROP_DURATION:
		g_value_set_double(value, self->duration);
		break;

		case PROP_PLAYLIST_POS:
		g_value_set_int(value, self->playlist_pos);
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

		case PROP_LOOP:
		g_value_set_boolean(value, self->loop);
		break;

		case PROP_BORDER:
		g_value_set_boolean(value, self->border);
		break;

		case PROP_FULLSCREEN:
		g_value_set_boolean(value, self->fullscreen);
		break;

		case PROP_DISPLAY_FPS:
		g_value_set_double(value, self->display_fps);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
load_css(CelluloidView *view)
{
	const gchar *style;
	GtkCssProvider *style_provider;
	gboolean css_loaded;

	style = ".celluloid-video-area{background-color: black}";
	style_provider = gtk_css_provider_new();
	css_loaded =	gtk_css_provider_load_from_data
			(style_provider, style, -1, NULL);

	if(!css_loaded)
	{
		g_warning ("Failed to apply background color css");
	}

	gtk_style_context_add_provider_for_screen
		(	gtk_window_get_screen(GTK_WINDOW(view)),
			GTK_STYLE_PROVIDER(style_provider),
			GTK_STYLE_PROVIDER_PRIORITY_APPLICATION );

	g_object_unref(style_provider);
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

	gboolean csd_enable;
	gboolean dark_theme_enable;

	csd_enable =	g_settings_get_boolean
			(settings, "csd-enable");
	dark_theme_enable =	g_settings_get_boolean
				(settings, "dark-theme-enable");

	g_object_set(	control_box,
			"show-fullscreen-button", !csd_enable,
			"skip-enabled", FALSE,
			NULL );
	g_object_set(	gtk_settings_get_default(),
			"gtk-application-prefer-dark-theme", dark_theme_enable,
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

void
show_message_dialog(	CelluloidMainWindow *wnd,
			GtkMessageType type,
			const gchar *title,
			const gchar *prefix,
			const gchar *msg )
{
	GtkWidget *dialog;
	GtkWidget *msg_area;
	GList *children;

	if(prefix)
	{
		gchar *prefix_escaped = g_markup_printf_escaped("%s", prefix);
		gchar *msg_escaped = g_markup_printf_escaped("%s", msg);

		dialog =	gtk_message_dialog_new_with_markup
				(	GTK_WINDOW(wnd),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					type,
					GTK_BUTTONS_OK,
					"<b>[%s]</b> %s",
					prefix_escaped,
					msg_escaped );

		g_free(prefix_escaped);
		g_free(msg_escaped);
	}
	else
	{
		dialog =	gtk_message_dialog_new
				(	GTK_WINDOW(wnd),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					type,
					GTK_BUTTONS_OK,
					"%s",
					msg );
	}

	msg_area =	gtk_message_dialog_get_message_area
			(GTK_MESSAGE_DIALOG(dialog));
	children = gtk_container_get_children(GTK_CONTAINER(msg_area));

	for(GList *iter = children; iter; iter = g_list_next(iter))
	{
		if(GTK_IS_LABEL(iter->data))
		{
			gtk_label_set_line_wrap_mode
				(GTK_LABEL(iter->data), PANGO_WRAP_WORD_CHAR);
		}
	}

	g_list_free(children);

	g_signal_connect(	dialog,
				"response",
				G_CALLBACK(gtk_widget_destroy),
				NULL );

	gtk_window_set_title(GTK_WINDOW(dialog), title);
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
	gtk_widget_show_all(dialog);
}

static void
update_display_fps(CelluloidView *view)
{
	GtkWidget *wnd = GTK_WIDGET(view);
	GdkWindow *gdkwindow = gtk_widget_get_window(wnd);
	GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(wnd));
	GdkDisplay *display = gdk_screen_get_display(screen);
	GdkMonitor *monitor =	gdk_display_get_monitor_at_window
				(display, gdkwindow);

	view->display_fps = gdk_monitor_get_refresh_rate(monitor)/1000.0;
	g_object_notify(G_OBJECT(view), "display-fps");
}

static void
open_dialog_response_handler(GtkDialog *dialog, gint response_id, gpointer data)
{
	GPtrArray *args = data;
	CelluloidView *view = g_ptr_array_index(args, 0);
	gboolean *append = g_ptr_array_index(args, 1);

	if(response_id == GTK_RESPONSE_ACCEPT)
	{
		GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
		GSList *uri_slist = gtk_file_chooser_get_filenames(chooser);

		if(uri_slist)
		{
			const gchar **uris = gslist_to_array(uri_slist);

			g_signal_emit_by_name(view, "file-open", uris, *append);
			g_free(uris);
		}

		g_slist_free_full(uri_slist, g_free);
	}

	celluloid_file_chooser_destroy(dialog);

	g_free(append);
	g_ptr_array_free(args, TRUE);
}

static void
open_location_dialog_response_handler(	GtkDialog *dialog,
					gint response_id,
					gpointer data )
{
	GPtrArray *args = data;
	CelluloidView *view = g_ptr_array_index(args, 0);
	gboolean *append = g_ptr_array_index(args, 1);

	if(response_id == GTK_RESPONSE_ACCEPT)
	{
		CelluloidOpenLocationDialog *location_dialog;
		const gchar *uris[2];

		location_dialog = CELLULOID_OPEN_LOCATION_DIALOG(dialog);
		uris[0] =	celluloid_open_location_dialog_get_string
				(location_dialog);
		uris[1] = NULL;

		g_signal_emit_by_name(view, "file-open", uris, *append);
	}

	gtk_widget_destroy(GTK_WIDGET(dialog));

	g_free(append);
	g_ptr_array_free(args, TRUE);
}

static void
open_audio_track_dialog_response_handler(	GtkDialog *dialog,
						gint response_id,
						gpointer data )
{
	if(response_id == GTK_RESPONSE_ACCEPT)
	{
		GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
		GSList *uri_list = gtk_file_chooser_get_filenames(chooser);

		for(GSList *iter = uri_list; iter; iter = g_slist_next(iter))
		{
			g_signal_emit_by_name(data, "audio-track-load", *iter);
		}

		g_slist_free_full(uri_list, g_free);
	}

	celluloid_file_chooser_destroy(CELLULOID_FILE_CHOOSER(dialog));
}

static void
open_subtitle_track_dialog_response_handler(	GtkDialog *dialog,
						gint response_id,
						gpointer data )
{
	if(response_id == GTK_RESPONSE_ACCEPT)
	{
		GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
		GSList *uri_list = gtk_file_chooser_get_filenames(chooser);

		for(GSList *iter = uri_list; iter; iter = g_slist_next(iter))
		{
			g_signal_emit_by_name(data, "subtitle-track-load", *iter);
		}

		g_slist_free_full(uri_list, g_free);
	}

	celluloid_file_chooser_destroy(CELLULOID_FILE_CHOOSER(dialog));
}

static void
preferences_dialog_response_handler(	GtkDialog *dialog,
					gint response_id,
					gpointer data )
{
	if(response_id == GTK_RESPONSE_ACCEPT)
	{
		CelluloidMainWindow *wnd;
		GSettings *settings;
		gboolean csd_enable;

		wnd = CELLULOID_MAIN_WINDOW(data);
		settings = g_settings_new(CONFIG_ROOT);
		csd_enable = g_settings_get_boolean(settings, "csd-enable");

		if(celluloid_main_window_get_csd_enabled(wnd) != csd_enable)
		{
			show_message_dialog(	wnd,
						GTK_MESSAGE_INFO,
						g_get_application_name(),
						NULL,
						_("Enabling or disabling "
						"client-side decorations "
						"requires restarting to "
						"take effect.") );
		}

		gtk_widget_queue_draw(GTK_WIDGET(wnd));
		g_signal_emit_by_name(data, "preferences-updated");

		g_object_unref(settings);
	}

	gtk_widget_destroy(GTK_WIDGET(dialog));
}

static void
realize_handler(GtkWidget *widget, gpointer data)
{
	update_display_fps(CELLULOID_VIEW(widget));
}

static void
size_allocate_handler(	GtkWidget *widget,
			GdkRectangle *allocation,
			gpointer data )
{
	g_signal_emit_by_name(	data,
				"video-area-resize",
				allocation->width,
				allocation->height );
}

static void
render_handler(CelluloidVideoArea *area, gpointer data)
{
	g_signal_emit_by_name(data, "render");
}

static gboolean
draw_handler(GtkWidget *widget, cairo_t *cr, gpointer data)
{
	CelluloidView *view =
		CELLULOID_VIEW(widget);
	CelluloidMainWindow *wnd =
		CELLULOID_MAIN_WINDOW(view);
	CelluloidPlaylistWidget *playlist =
		celluloid_main_window_get_playlist(wnd);
	const GSignalMatchType match =
		G_SIGNAL_MATCH_ID|G_SIGNAL_MATCH_DATA;
	const guint signal_id =
		g_signal_lookup("draw", CELLULOID_TYPE_VIEW);

	g_signal_handlers_disconnect_matched
		(view, match, signal_id, 0, 0, NULL, NULL);

	if(celluloid_playlist_widget_empty(playlist))
	{
		CelluloidControlBox *control_box;

		control_box = celluloid_main_window_get_control_box(wnd);
		g_object_set(control_box, "enabled", FALSE, NULL);
	}

	g_signal_emit_by_name(view, "ready");

	return FALSE;
}

static void
drag_data_handler(	GtkWidget *widget,
			GdkDragContext *context,
			gint x,
			gint y,
			GtkSelectionData *sel_data,
			guint info,
			guint time,
			gpointer data )
{
	CelluloidView *view = data;
	gchar *type = gdk_atom_name(gtk_selection_data_get_target(sel_data));
	const guchar *raw_data = gtk_selection_data_get_data(sel_data);
	gchar **uri_list = gtk_selection_data_get_uris(sel_data);
	gboolean append = CELLULOID_IS_PLAYLIST_WIDGET(widget);

	if(g_strcmp0(type, "PLAYLIST_PATH") == 0)
	{
		GtkTreePath *path =	gtk_tree_path_new_from_string
					((const gchar *)raw_data);
		gint pos = gtk_tree_path_get_indices(path)[0];

		g_assert(path);

		g_signal_emit_by_name(view, "playlist-item-activated", pos);

		gtk_tree_path_free(path);
	}
	else
	{
		if(!uri_list)
		{
			uri_list = g_malloc(2*sizeof(gchar *));
			uri_list[0] = g_strdup((gchar *)raw_data);
			uri_list[1] = NULL;
		}

		g_signal_emit_by_name(view, "file-open", uri_list, append);
	}

	g_strfreev(uri_list);
	g_free(type);
}

static gboolean
window_state_handler(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	CelluloidView *view = data;
	GdkEventWindowState *state = (GdkEventWindowState *)event;

	if(state->changed_mask&GDK_WINDOW_STATE_FULLSCREEN)
	{
		view->fullscreen =	state->new_window_state&
					GDK_WINDOW_STATE_FULLSCREEN;

		g_object_notify(data, "fullscreen");
	}

	return FALSE;
}

static void
screen_changed_handler(	GtkWidget *wnd,
			GdkScreen *previous_screen,
			gpointer data )
{
	update_display_fps(data);
}

static gboolean
delete_handler(GtkWidget *widget, GdkEvent *event, gpointer data)
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
			0,
			G_MAXINT,
			0,
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_PLAYLIST_POS, pspec);

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
		(	"loop",
			"Loop",
			"Whether or not the the loop button is active",
			FALSE,
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_LOOP, pspec);

	pspec = g_param_spec_boolean
		(	"border",
			"Border",
			"Whether or not the main window should have decorations",
			FALSE,
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_BORDER, pspec);

	pspec = g_param_spec_boolean
		(	"fullscreen",
			"Fullscreen",
			"Whether or not the player is current in fullscreen mode",
			FALSE,
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_FULLSCREEN, pspec);

	pspec = g_param_spec_double
		(	"display-fps",
			"Display FPS",
			"The display rate of the monitor the window is on",
			0.0,
			G_MAXDOUBLE,
			0.0,
			G_PARAM_READABLE );
	g_object_class_install_property(object_class, PROP_DISPLAY_FPS, pspec);

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
	g_signal_new(	"preferences-updated",
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
	view->playlist_count = 0;
	view->pause = FALSE;
	view->volume = 0.0;
	view->duration = 0.0;
	view->playlist_pos = 0;
	view->disc_list = NULL;
	view->skip_enabled = FALSE;
	view->loop = FALSE;
	view->border = FALSE;
	view->fullscreen = FALSE;
	view->display_fps = 0;

	g_signal_connect(view, "realize", G_CALLBACK(realize_handler), NULL);
	g_signal_connect_after(view, "draw", G_CALLBACK(draw_handler), NULL);
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
celluloid_view_show_open_dialog(CelluloidView *view, gboolean append)
{
	CelluloidFileChooser *chooser;
	GPtrArray *args;
	gboolean *append_buf;

	chooser = celluloid_file_chooser_new(	append?
						_("Add File to Playlist"):
						_("Open File"),
						GTK_WINDOW(view),
						GTK_FILE_CHOOSER_ACTION_OPEN );
	args = g_ptr_array_new();
	append_buf = g_malloc(sizeof(gboolean));
	*append_buf = append;

	g_ptr_array_add(args, view);
	g_ptr_array_add(args, append_buf);

	g_signal_connect(	chooser,
				"response",
				G_CALLBACK(open_dialog_response_handler),
				args );

	celluloid_file_chooser_set_default_filters(chooser, TRUE, TRUE, TRUE, TRUE);
	celluloid_file_chooser_show(chooser);
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

	gtk_widget_show_all(dlg);
}

void
celluloid_view_show_open_audio_track_dialog(CelluloidView *view)
{
	CelluloidFileChooser *chooser =	celluloid_file_chooser_new
					(	_("Load Audio Track…"),
						GTK_WINDOW(view),
						GTK_FILE_CHOOSER_ACTION_OPEN );

	g_signal_connect
		(	chooser,
			"response",
			G_CALLBACK(open_audio_track_dialog_response_handler),
			view );

	celluloid_file_chooser_set_default_filters
		(chooser, TRUE, FALSE, FALSE, FALSE);

	celluloid_file_chooser_show(chooser);
}

void
celluloid_view_show_open_subtitle_track_dialog(CelluloidView *view)
{
	CelluloidFileChooser *chooser =	celluloid_file_chooser_new
					(	_("Load Subtitle Track…"),
						GTK_WINDOW(view),
						GTK_FILE_CHOOSER_ACTION_OPEN );

	g_signal_connect
		(	chooser,
			"response",
			G_CALLBACK(open_subtitle_track_dialog_response_handler),
			view );

	celluloid_file_chooser_set_default_filters
		(chooser, FALSE, FALSE, FALSE, TRUE);

	celluloid_file_chooser_show(chooser);
}

void
celluloid_view_show_save_playlist_dialog(CelluloidView *view)
{
	GFile *dest_file;
	CelluloidFileChooser *file_chooser;
	GtkFileChooser *gtk_chooser;
	GError *error;

	dest_file = NULL;
	file_chooser =	celluloid_file_chooser_new
			(	_("Save Playlist"),
				GTK_WINDOW(view),
				GTK_FILE_CHOOSER_ACTION_SAVE );
	gtk_chooser = GTK_FILE_CHOOSER(file_chooser);
	error = NULL;

	gtk_file_chooser_set_current_name(gtk_chooser, "playlist.m3u");

	if(celluloid_file_chooser_run(file_chooser) == GTK_RESPONSE_ACCEPT)
	{
		/* There should be only one file selected. */
		dest_file = gtk_file_chooser_get_file(gtk_chooser);
	}

	celluloid_file_chooser_destroy(file_chooser);

	if(dest_file)
	{
		save_playlist(view, dest_file, &error);
		g_object_unref(dest_file);
	}

	if(error)
	{
		show_message_dialog(	CELLULOID_MAIN_WINDOW(view),
					GTK_MESSAGE_ERROR,
					_("Error"),
					NULL,
					error->message );

		g_error_free(error);
	}
}

void
celluloid_view_show_preferences_dialog(CelluloidView *view)
{
	GtkWidget *dialog = celluloid_preferences_dialog_new(GTK_WINDOW(view));

	g_signal_connect_after(	dialog,
				"response",
				G_CALLBACK(preferences_dialog_response_handler),
				view );

	gtk_widget_show_all(dialog);
}

void
celluloid_view_show_shortcuts_dialog(CelluloidView *view)
{
	GtkWidget *wnd = celluloid_shortcuts_window_new(GTK_WINDOW(view));

	gtk_widget_show_all(wnd);
}

void
celluloid_view_show_about_dialog(CelluloidView *view)
{
	const gchar *const authors[] = AUTHORS;

	gtk_show_about_dialog(	GTK_WINDOW(view),
				"logo-icon-name",
				ICON_NAME,
				"version",
				VERSION,
				"comments",
				_("A GTK frontend for MPV"),
				"website",
				"https://github.com/celluloid-player/celluloid",
				"license-type",
				GTK_LICENSE_GPL_3_0,
				"copyright",
				"\u00A9 2014-2019 The Celluloid authors",
				"authors",
				authors,
				"translator-credits",
				_("translator-credits"),
				NULL );
}

void
celluloid_view_show_message_dialog(	CelluloidView *view,
					GtkMessageType type,
					const gchar *title,
					const gchar *prefix,
					const gchar *msg )
{
	show_message_dialog
		(CELLULOID_MAIN_WINDOW(view), type, title, prefix, msg);
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

void
celluloid_view_set_use_opengl_cb(CelluloidView *view, gboolean use_opengl_cb)
{
	CelluloidMainWindow *wnd = CELLULOID_MAIN_WINDOW(view);
	CelluloidVideoArea *area = celluloid_main_window_get_video_area(wnd);

	celluloid_video_area_set_use_opengl(area, use_opengl_cb);
}

gint
celluloid_view_get_scale_factor(CelluloidView *view)
{
	GdkWindow *gdk_window = gtk_widget_get_window(GTK_WIDGET(view));

	return gdk_window_get_scale_factor(gdk_window);
}

void
celluloid_view_get_video_area_geometry(	CelluloidView *view,
					gint *width,
					gint *height )
{
	CelluloidMainWindow *wnd = CELLULOID_MAIN_WINDOW(view);
	CelluloidVideoArea *area = celluloid_main_window_get_video_area(wnd);
	GtkAllocation allocation;

	gtk_widget_get_allocation(GTK_WIDGET(area), &allocation);

	*width = allocation.width;
	*height = allocation.height;
}

void
celluloid_view_move(	CelluloidView *view,
			gboolean flip_x,
			gboolean flip_y,
			GValue *x,
			GValue *y )
{
	GtkWindow *window = GTK_WINDOW(view);
	GdkWindow *gdk_window = gtk_widget_get_window(GTK_WIDGET(window));
	GdkDisplay *display = gdk_display_get_default();
	GdkMonitor *monitor =	gdk_display_get_monitor_at_window
				(display, gdk_window);
	GdkRectangle monitor_geom = {0};
	gint64 x_pos = -1;
	gint64 y_pos = -1;
	gint window_width = 0;
	gint window_height = 0;
	gint space_x = 0;
	gint space_y = 0;

	gtk_window_get_size(window, &window_width, &window_height);
	gdk_monitor_get_geometry(monitor, &monitor_geom);
	space_x = monitor_geom.width-window_width;
	space_y = monitor_geom.height-window_height;

	if(G_VALUE_HOLDS_DOUBLE(x))
	{
		x_pos = (gint64)(g_value_get_double(x)*space_x);
	}
	else
	{
		x_pos = flip_x?space_x-g_value_get_int64(x):g_value_get_int64(x);
	}

	if(G_VALUE_HOLDS_DOUBLE(y))
	{
		y_pos = (gint64)(g_value_get_double(y)*space_y);
	}
	else
	{
		y_pos = flip_y?space_y-g_value_get_int64(y):g_value_get_int64(y);
	}

	gtk_window_move(window, (gint)x_pos, (gint)y_pos);
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
	g_object_set(view, "fullscreen", fullscreen, NULL);
	celluloid_main_window_set_fullscreen
		(CELLULOID_MAIN_WINDOW(view), fullscreen);
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
	g_object_set(view, "playlist-pos", pos, NULL);
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
	return 	celluloid_main_window_get_playlist_visible
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
