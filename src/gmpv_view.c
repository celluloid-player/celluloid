/*
 * Copyright (c) 2017 gnome-mpv
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

#include "gmpv_view.h"
#include "gmpv_file_chooser.h"
#include "gmpv_open_location_dialog.h"
#include "gmpv_preferences_dialog.h"
#include "gmpv_shortcuts_window.h"
#include "gmpv_authors.h"
#include "gmpv_marshal.h"
#include "gmpv_menu.h"
#include "gmpv_common.h"
#include "gmpv_def.h"

enum
{
	PROP_0,
	PROP_WINDOW,
	PROP_PLAYLIST_COUNT,
	PROP_PAUSE,
	PROP_TITLE,
	PROP_VOLUME,
	PROP_DURATION,
	PROP_PLAYLIST_POS,
	PROP_TRACK_LIST,
	PROP_SKIP_ENABLED,
	PROP_FULLSCREEN,
	N_PROPERTIES
};

struct _GmpvView
{
	GObject parent;
	GmpvMainWindow *wnd;

	/* Properties */
	gint playlist_count;
	gboolean pause;
	gchar *title;
	gdouble volume;
	gdouble duration;
	gint playlist_pos;
	GPtrArray *track_list;
	gboolean skip_enabled;
	gboolean control_box_enabled;
	gboolean fullscreen;
};

struct _GmpvViewClass
{
	GObjectClass parent_class;
};

G_DEFINE_TYPE(GmpvView, gmpv_view, G_TYPE_OBJECT)

static void constructed(GObject *object);
static void set_property(	GObject *object,
				guint property_id,
				const GValue *value,
				GParamSpec *pspec );
static void get_property(	GObject *object,
				guint property_id,
				GValue *value,
				GParamSpec *pspec );
static void dispose(GObject * object);
static void finalize(GObject * object);
static void load_css(GmpvView *view);
static void save_playlist(GmpvView *view, GFile *file, GError **error);
static void show_message_dialog(	GmpvMainWindow *wnd,
					GtkMessageType type,
					const gchar *title,
					const gchar *prefix,
					const gchar *msg );

/* Control box signals */
static void button_clicked_handler(	GtkWidget *widget,
					const gchar *button,
					gpointer data );
static void seek_handler(GtkWidget *widget, gdouble value, gpointer data);

/* Dialog responses */
static void open_dialog_response_handler(	GtkDialog *dialog,
						gint response_id,
						gpointer data );
static void open_location_dialog_response_handler(	GtkDialog *dialog,
							gint response_id,
							gpointer data );
static void open_audio_track_dialog_response_handler(	GtkDialog *dialog,
							gint response_id,
							gpointer data );
static void open_subtitle_track_dialog_response_handler(	GtkDialog *dialog,
								gint response_id,
								gpointer data );
static void preferences_dialog_response_handler(	GtkDialog *dialog,
							gint response_id,
							gpointer data );

/* User inputs */
static void key_press_handler(	GmpvMainWindow *wnd,
				GdkEvent *event,
				gpointer data );
static void key_release_handler(	GmpvMainWindow *wnd,
					GdkEvent *event,
					gpointer data );
static void button_press_handler(	GmpvMainWindow *wnd,
					GdkEvent *event,
					gpointer data );
static void button_release_handler(	GmpvMainWindow *wnd,
					GdkEvent *event,
					gpointer data );
static void motion_notify_handler(	GmpvMainWindow *wnd,
					GdkEvent *event,
					gpointer data );
static void scroll_handler(	GmpvMainWindow *wnd,
				GdkEvent *event,
				gpointer data );

static void render_handler(GmpvVideoArea *area, gpointer data);
static gboolean draw_handler(GtkWidget *widget, cairo_t *cr, gpointer data);
static void drag_data_handler(	GtkWidget *widget,
				GdkDragContext *context,
				gint x,
				gint y,
				GtkSelectionData *sel_data,
				guint info,
				guint time,
				gpointer data );
static gboolean window_state_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data );
static void grab_handler(GtkWidget *widget, gboolean was_grabbed, gpointer data);
static gboolean delete_handler(	GtkWidget *widget,
				GdkEvent *event,
				gpointer data );
static void playlist_row_activated_handler(	GmpvPlaylistWidget *playlist,
						gint64 pos,
						gpointer data );
static void playlist_row_inserted_handler(	GmpvPlaylistWidget *widget,
						gint pos,
						gpointer data );
static void playlist_row_deleted_handler(	GmpvPlaylistWidget *widget,
						gint pos,
						gpointer data );
static void playlist_row_reordered_handler(	GmpvPlaylistWidget *widget,
						gint src,
						gint dest,
						gpointer data );

static void constructed(GObject *object)
{
	GmpvView *view = GMPV_VIEW(object);
	GmpvVideoArea *video_area = gmpv_main_window_get_video_area(view->wnd);
	GmpvPlaylistWidget *playlist = gmpv_main_window_get_playlist(view->wnd);
	GmpvControlBox *control_box;
	GSettings *settings = g_settings_new(CONFIG_ROOT);
	gboolean csd_enable;
	gboolean dark_theme_enable;

	csd_enable = g_settings_get_boolean
				(settings, "csd-enable");
	dark_theme_enable = g_settings_get_boolean
				(settings, "dark-theme-enable");
	control_box = gmpv_main_window_get_control_box(view->wnd);

	gmpv_main_window_load_state(view->wnd);
	load_css(view);
	gtk_widget_show_all(GTK_WIDGET(view->wnd));

	g_object_set(	control_box,
			"show-fullscreen-button", !csd_enable,
			"skip-enabled", FALSE,
			NULL );
	g_object_set(	gtk_settings_get_default(),
			"gtk-application-prefer-dark-theme",
			dark_theme_enable,
			NULL );

	g_object_unref(settings);

	g_object_bind_property(	view, "title",
				view->wnd, "title",
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
	g_object_bind_property(	playlist, "playlist-count",
				view, "playlist-count",
				G_BINDING_DEFAULT );

	g_signal_connect(	view->wnd,
				"button-clicked",
				G_CALLBACK(button_clicked_handler),
				view );
	g_signal_connect(	view->wnd,
				"seek",
				G_CALLBACK(seek_handler),
				view );

	g_signal_connect(	view->wnd,
				"key-press-event",
				G_CALLBACK(key_press_handler),
				view );
	g_signal_connect(	view->wnd,
				"key-release-event",
				G_CALLBACK(key_release_handler),
				view );
	g_signal_connect(	video_area,
				"render",
				G_CALLBACK(render_handler),
				view );
	g_signal_connect(	video_area,
				"button-press-event",
				G_CALLBACK(button_press_handler),
				view );
	g_signal_connect(	video_area,
				"button-release-event",
				G_CALLBACK(button_release_handler),
				view );
	g_signal_connect(	video_area,
				"motion-notify-event",
				G_CALLBACK(motion_notify_handler),
				view );
	g_signal_connect(	video_area,
				"scroll-event",
				G_CALLBACK(scroll_handler),
				view );

	g_signal_connect_after(	view->wnd,
				"draw",
				G_CALLBACK(draw_handler),
				view );
	g_signal_connect(	video_area,
				"drag-data-received",
				G_CALLBACK(drag_data_handler),
				view );
	g_signal_connect(	playlist,
				"drag-data-received",
				G_CALLBACK(drag_data_handler),
				view );
	g_signal_connect(	view->wnd,
				"window-state-event",
				G_CALLBACK(window_state_handler),
				view );
	g_signal_connect(	view->wnd,
				"grab-notify",
				G_CALLBACK(grab_handler),
				view );
	g_signal_connect(	view->wnd,
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

	G_OBJECT_CLASS(gmpv_view_parent_class)->constructed(object);
}

static void set_property(	GObject *object,
				guint property_id,
				const GValue *value,
				GParamSpec *pspec )
{
	GmpvView *self = GMPV_VIEW(object);
	GmpvControlBox* control_box = NULL;

	if(self->wnd)
	{
		 control_box = gmpv_main_window_get_control_box(self->wnd);
	}

	switch(property_id)
	{
		case PROP_WINDOW:
		self->wnd = g_value_get_pointer(value);
		break;

		case PROP_PLAYLIST_COUNT:
		self->playlist_count = g_value_get_int(value);

		g_object_set(	control_box,
				"enabled",
				self->playlist_count > 0,
				NULL );

		if(self->playlist_count <= 0)
		{
			gmpv_view_reset(self);
		}
		break;

		case PROP_PAUSE:
		self->pause = g_value_get_boolean(value);
		break;

		case PROP_TITLE:
		g_free(self->title);
		self->title = g_value_dup_string(value);
		break;

		case PROP_VOLUME:
		self->volume = g_value_get_double(value);
		break;

		case PROP_DURATION:
		self->duration = g_value_get_double(value);
		break;

		case PROP_PLAYLIST_POS:
		self->playlist_pos = g_value_get_int(value);
		GmpvPlaylistWidget *playlist = gmpv_main_window_get_playlist(self->wnd);
		gmpv_playlist_widget_set_indicator_pos(playlist, self->playlist_pos);
		break;

		case PROP_TRACK_LIST:
		self->track_list = g_value_get_pointer(value);
		gmpv_main_window_update_track_list(self->wnd, self->track_list);
		break;

		case PROP_SKIP_ENABLED:
		self->skip_enabled = g_value_get_boolean(value);
		break;

		case PROP_FULLSCREEN:
		self->fullscreen = g_value_get_boolean(value);
		gmpv_main_window_set_fullscreen(self->wnd, self->fullscreen);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void get_property(	GObject *object,
				guint property_id,
				GValue *value,
				GParamSpec *pspec )
{
	GmpvView *self = GMPV_VIEW(object);

	switch(property_id)
	{
		case PROP_WINDOW:
		g_value_set_pointer(value, self->wnd);
		break;

		case PROP_PLAYLIST_COUNT:
		g_value_set_int(value, self->playlist_count);
		break;

		case PROP_PAUSE:
		g_value_set_boolean(value, self->pause);
		break;

		case PROP_TITLE:
		g_value_set_string(value, self->title);
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

		case PROP_SKIP_ENABLED:
		g_value_set_boolean(value, self->skip_enabled);
		break;

		case PROP_FULLSCREEN:
		g_value_set_boolean(value, self->fullscreen);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void dispose(GObject *object)
{
	GmpvView *view = GMPV_VIEW(object);

	if(view->wnd)
	{
		gtk_widget_destroy(GTK_WIDGET(view->wnd));
		view->wnd = NULL;
	}

	G_OBJECT_CLASS(gmpv_view_parent_class)->dispose(object);
}

static void finalize(GObject *object)
{
	g_free(GMPV_VIEW(object)->title);

	G_OBJECT_CLASS(gmpv_view_parent_class)->finalize(object);
}

static void load_css(GmpvView *view)
{
	const gchar *style;
	GtkCssProvider *style_provider;
	gboolean css_loaded;

	style = ".gmpv-vid-area{background-color: black}";
	style_provider = gtk_css_provider_new();
	css_loaded =	gtk_css_provider_load_from_data
			(style_provider, style, -1, NULL);

	if(!css_loaded)
	{
		g_warning ("Failed to apply background color css");
	}

	gtk_style_context_add_provider_for_screen
		(	gtk_window_get_screen(GTK_WINDOW(view->wnd)),
			GTK_STYLE_PROVIDER(style_provider),
			GTK_STYLE_PROVIDER_PRIORITY_APPLICATION );

	g_object_unref(style_provider);
}

static void save_playlist(GmpvView *view, GFile *file, GError **error)
{
	GmpvPlaylistWidget *wgt = gmpv_main_window_get_playlist(view->wnd);
	GPtrArray *playlist = gmpv_playlist_widget_get_contents(wgt);
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

		GmpvPlaylistEntry *entry = g_ptr_array_index(playlist, i);

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

void show_message_dialog(	GmpvMainWindow *wnd,
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

static void button_clicked_handler(	GtkWidget *widget,
					const gchar *button,
					gpointer data )
{
	gchar *name = g_strconcat("button-clicked::", button, NULL);

	g_signal_emit_by_name(data, name);
	g_free(name);
}

static void seek_handler(GtkWidget *widget, gdouble value, gpointer data)
{
	g_signal_emit_by_name(data, "seek", value);
}

static void open_dialog_response_handler(	GtkDialog *dialog,
						gint response_id,
						gpointer data )
{
	GPtrArray *args = data;
	GmpvView *view = g_ptr_array_index(args, 0);
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

	gmpv_file_chooser_destroy(dialog);

	g_free(append);
	g_ptr_array_free(args, TRUE);
}

static void open_location_dialog_response_handler(	GtkDialog *dialog,
							gint response_id,
							gpointer data )
{
	GPtrArray *args = data;
	GmpvView *view = g_ptr_array_index(args, 0);
	gboolean *append = g_ptr_array_index(args, 1);

	if(response_id == GTK_RESPONSE_ACCEPT)
	{
		GmpvOpenLocationDialog *location_dialog;
		const gchar *uris[2];

		location_dialog = GMPV_OPEN_LOCATION_DIALOG(dialog);
		uris[0] = gmpv_open_location_dialog_get_string(location_dialog);
		uris[1] = NULL;

		g_signal_emit_by_name(view, "file-open", uris, *append);
	}

	gtk_widget_destroy(GTK_WIDGET(dialog));

	g_free(append);
	g_ptr_array_free(args, TRUE);
}

static void open_audio_track_dialog_response_handler(	GtkDialog *dialog,
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

	gmpv_file_chooser_destroy(GMPV_FILE_CHOOSER(dialog));
}

static void open_subtitle_track_dialog_response_handler(	GtkDialog *dialog,
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

	gmpv_file_chooser_destroy(GMPV_FILE_CHOOSER(dialog));
}

static void preferences_dialog_response_handler(	GtkDialog *dialog,
							gint response_id,
							gpointer data )
{
	if(response_id == GTK_RESPONSE_ACCEPT)
	{
		GmpvView *view;
		GSettings *settings;
		gboolean csd_enable;

		view = data;
		settings = g_settings_new(CONFIG_ROOT);
		csd_enable = g_settings_get_boolean(settings, "csd-enable");

		if(gmpv_main_window_get_csd_enabled(view->wnd) != csd_enable)
		{
			show_message_dialog(	view->wnd,
						GTK_MESSAGE_INFO,
						g_get_application_name(),
						NULL,
						_("Enabling or disabling "
						"client-side decorations "
						"requires restarting to "
						"take effect.") );
		}

		gtk_widget_queue_draw(GTK_WIDGET(view->wnd));
		g_signal_emit_by_name(data, "preferences-updated");

		g_object_unref(settings);
	}

	gtk_widget_destroy(GTK_WIDGET(dialog));
}

static void key_press_handler(	GmpvMainWindow *wnd,
				GdkEvent *event,
				gpointer data )
{
	g_signal_emit_by_name(data, "key-press-event", event);
}

static void key_release_handler(	GmpvMainWindow *wnd,
					GdkEvent *event,
					gpointer data )
{
	g_signal_emit_by_name(data, "key-release-event", event);
}

static void button_press_handler(	GmpvMainWindow *wnd,
					GdkEvent *event,
					gpointer data )
{
	g_signal_emit_by_name(data, "button-press-event", event);
}

static void button_release_handler(	GmpvMainWindow *wnd,
					GdkEvent *event,
					gpointer data )
{
	g_signal_emit_by_name(data, "button-release-event", event);
}

static void motion_notify_handler(	GmpvMainWindow *wnd,
					GdkEvent *event,
					gpointer data )
{
	g_signal_emit_by_name(data, "motion-notify-event", event);
}

static void scroll_handler(	GmpvMainWindow *wnd,
				GdkEvent *event,
				gpointer data )
{
	g_signal_emit_by_name(data, "scroll-event", event);
}

static void render_handler(GmpvVideoArea *area, gpointer data)
{
	g_signal_emit_by_name(data, "render");
}

static gboolean draw_handler(GtkWidget *widget, cairo_t *cr, gpointer data)
{
	GmpvView *view = data;
	GmpvPlaylistWidget *playlist = gmpv_main_window_get_playlist(view->wnd);
	guint signal_id = g_signal_lookup("draw", GMPV_TYPE_MAIN_WINDOW);

	g_signal_handlers_disconnect_matched(	widget,
						G_SIGNAL_MATCH_ID|
						G_SIGNAL_MATCH_DATA,
						signal_id,
						0,
						0,
						NULL,
						view );

	if(gmpv_playlist_widget_empty(playlist))
	{
		GmpvControlBox *control_box;

		control_box = gmpv_main_window_get_control_box(view->wnd);
		g_object_set(control_box, "enabled", FALSE, NULL);
	}

	g_signal_emit_by_name(view, "ready");

	return FALSE;
}

static void drag_data_handler(	GtkWidget *widget,
				GdkDragContext *context,
				gint x,
				gint y,
				GtkSelectionData *sel_data,
				guint info,
				guint time,
				gpointer data )
{
	GmpvView *view = data;
	gchar *type = gdk_atom_name(gtk_selection_data_get_target(sel_data));
	const guchar *raw_data = gtk_selection_data_get_data(sel_data);
	gchar **uri_list = gtk_selection_data_get_uris(sel_data);
	gboolean append = GMPV_IS_PLAYLIST_WIDGET(widget);

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

static gboolean window_state_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data )
{
	GmpvView *view = data;
	GdkEventWindowState *state = (GdkEventWindowState *)event;

	if(state->changed_mask&GDK_WINDOW_STATE_FULLSCREEN)
	{
		view->fullscreen =	state->new_window_state&
					GDK_WINDOW_STATE_FULLSCREEN;

		g_object_notify(data, "fullscreen");
	}

	return FALSE;
}

static void grab_handler(GtkWidget *widget, gboolean was_grabbed, gpointer data)
{
	g_signal_emit_by_name(data, "grab-notify", was_grabbed);
}

static gboolean delete_handler(	GtkWidget *widget,
				GdkEvent *event,
				gpointer data )
{
	if(!gmpv_main_window_get_fullscreen(GMPV_MAIN_WINDOW(widget)))
	{
		gmpv_main_window_save_state(GMPV_MAIN_WINDOW(widget));
	}

	g_signal_emit_by_name(data, "delete-notify");

	return TRUE;
}

static void playlist_row_activated_handler(	GmpvPlaylistWidget *playlist,
						gint64 pos,
						gpointer data )
{
	g_signal_emit_by_name(data, "playlist-item-activated", pos);
}

static void playlist_row_inserted_handler(	GmpvPlaylistWidget *widget,
						gint pos,
						gpointer data )
{
	g_signal_emit_by_name(data, "playlist-item-inserted", pos);
}

static void playlist_row_deleted_handler(	GmpvPlaylistWidget *widget,
						gint pos,
						gpointer data )
{
	if(gmpv_playlist_widget_empty(widget))
	{
		gmpv_view_reset(data);
	}

	g_signal_emit_by_name(data, "playlist-item-deleted", pos);
}

static void playlist_row_reordered_handler(	GmpvPlaylistWidget *widget,
						gint src,
						gint dest,
						gpointer data )
{
	g_signal_emit_by_name(data, "playlist-reordered", src, dest);
}

static void gmpv_view_class_init(GmpvViewClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec = NULL;

	object_class->constructed = constructed;
	object_class->set_property = set_property;
	object_class->get_property = get_property;
	object_class->dispose = dispose;
	object_class->finalize = finalize;

	pspec = g_param_spec_pointer
		(	"window",
			"Window",
			"The GmpvMainWindow to use",
			G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_WINDOW, pspec);

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

	pspec = g_param_spec_string
		(	"title",
			"Title",
			"The title of the window",
			_("GNOME MPV"),
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_TITLE, pspec);

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

	pspec = g_param_spec_boolean
		(	"skip-enabled",
			"Skip enabled",
			"Whether or not skip buttons are shown",
			FALSE,
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_SKIP_ENABLED, pspec);

	pspec = g_param_spec_boolean
		(	"fullscreen",
			"Fullscreen",
			"Whether or not the player is current in fullscreen mode",
			FALSE,
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_FULLSCREEN, pspec);

	/* Controls-related signals */
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

	/* User input signals */
	g_signal_new(	"key-press-event",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER);
	g_signal_new(	"key-release-event",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER );
	g_signal_new(	"button-press-event",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER );
	g_signal_new(	"button-release-event",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER );
	g_signal_new(	"motion-notify-event",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER );
	g_signal_new(	"scroll-event",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER );

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
	g_signal_new(	"grab-notify",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__BOOLEAN,
			G_TYPE_NONE,
			1,
			G_TYPE_BOOLEAN );
	g_signal_new(	"delete-notify",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0 );
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

static void gmpv_view_init(GmpvView *view)
{
	view->wnd = NULL;
	view->playlist_count = 0;
	view->pause = FALSE;
	view->title = NULL;
	view->volume = 0.0;
	view->duration = 0.0;
	view->playlist_pos = 0;
	view->skip_enabled = FALSE;
	view->fullscreen = FALSE;
}

GmpvView *gmpv_view_new(GmpvApplication *app, gboolean always_floating)
{
	GtkApplication *gtk_app = GTK_APPLICATION(app);
	GSettings *settings = g_settings_new(CONFIG_ROOT);
	GtkWidget *window = gmpv_main_window_new(gtk_app, always_floating);

	if(g_settings_get_boolean(settings, "csd-enable"))
	{
		GMenu *app_menu = g_menu_new();

		gmpv_menu_build_app_menu(app_menu);
		gtk_application_set_app_menu(gtk_app, G_MENU_MODEL(app_menu));
		gmpv_main_window_enable_csd(GMPV_MAIN_WINDOW(window));
	}
	else
	{
		GMenu *full_menu = g_menu_new();

		gmpv_menu_build_full(full_menu, NULL);
		gtk_application_set_app_menu(gtk_app, NULL);
		gtk_application_set_menubar(gtk_app, G_MENU_MODEL(full_menu));
	}

	g_object_unref(settings);

	return GMPV_VIEW(g_object_new(	gmpv_view_get_type(),
					"window", window,
					NULL ));
}

GmpvMainWindow *gmpv_view_get_main_window(GmpvView *view)
{
	return view->wnd;
}

void gmpv_view_show_open_dialog(GmpvView *view, gboolean append)
{
	GmpvFileChooser *chooser;
	GPtrArray *args;
	gboolean *append_buf;

	chooser = gmpv_file_chooser_new(	append?
						_("Add File to Playlist"):
						_("Open File"),
						GTK_WINDOW(view->wnd),
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

	gmpv_file_chooser_set_default_filters(chooser, TRUE, TRUE, TRUE, TRUE);
	gmpv_file_chooser_show(chooser);
}

void gmpv_view_show_open_location_dialog(GmpvView *view, gboolean append)
{
	GtkWidget *dlg;
	GPtrArray *args;
	gboolean *append_buf;

	dlg = gmpv_open_location_dialog_new(	GTK_WINDOW(view->wnd),
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

void gmpv_view_show_open_audio_track_dialog(GmpvView *view)
{
	GmpvFileChooser *chooser =	gmpv_file_chooser_new
					(	_("Load Audio Track…"),
						GTK_WINDOW(view->wnd),
						GTK_FILE_CHOOSER_ACTION_OPEN );

	g_signal_connect
		(	chooser,
			"response",
			G_CALLBACK(open_audio_track_dialog_response_handler),
			view );

	gmpv_file_chooser_set_default_filters
		(chooser, TRUE, FALSE, FALSE, FALSE);

	gmpv_file_chooser_show(chooser);
}

void gmpv_view_show_open_subtitle_track_dialog(GmpvView *view)
{
	GmpvFileChooser *chooser =	gmpv_file_chooser_new
					(	_("Load Subtitle Track…"),
						GTK_WINDOW(view->wnd),
						GTK_FILE_CHOOSER_ACTION_OPEN );

	g_signal_connect
		(	chooser,
			"response",
			G_CALLBACK(open_subtitle_track_dialog_response_handler),
			view );

	gmpv_file_chooser_set_default_filters
		(chooser, FALSE, FALSE, FALSE, TRUE);

	gmpv_file_chooser_show(chooser);
}

void gmpv_view_show_save_playlist_dialog(GmpvView *view)
{
	GFile *dest_file;
	GmpvFileChooser *file_chooser;
	GtkFileChooser *gtk_chooser;
	GError *error;

	dest_file = NULL;
	file_chooser =	gmpv_file_chooser_new
			(	_("Save Playlist"),
				GTK_WINDOW(view->wnd),
				GTK_FILE_CHOOSER_ACTION_SAVE );
	gtk_chooser = GTK_FILE_CHOOSER(file_chooser);
	error = NULL;

	gtk_file_chooser_set_current_name(gtk_chooser, "playlist.m3u");

	if(gmpv_file_chooser_run(file_chooser) == GTK_RESPONSE_ACCEPT)
	{
		/* There should be only one file selected. */
		dest_file = gtk_file_chooser_get_file(gtk_chooser);
	}

	gmpv_file_chooser_destroy(file_chooser);

	if(dest_file)
	{
		save_playlist(view, dest_file, &error);
		g_object_unref(dest_file);
	}

	if(error)
	{
		show_message_dialog(	view->wnd,
					GTK_MESSAGE_ERROR,
					_("Error"),
					NULL,
					error->message );

		g_error_free(error);
	}
}

void gmpv_view_show_preferences_dialog(GmpvView *view)
{
	GtkWidget *dialog = gmpv_preferences_dialog_new(GTK_WINDOW(view->wnd));

	g_signal_connect_after(	dialog,
				"response",
				G_CALLBACK(preferences_dialog_response_handler),
				view );

	gtk_widget_show_all(dialog);
}

void gmpv_view_show_shortcuts_dialog(GmpvView *view)
{
#if GTK_CHECK_VERSION(3, 20, 0)
	GtkWidget *wnd = gmpv_shortcuts_window_new(GTK_WINDOW(view->wnd));

	gtk_widget_show_all(wnd);
#endif
}

void gmpv_view_show_about_dialog(GmpvView *view)
{
	const gchar *const authors[] = AUTHORS;

	gtk_show_about_dialog(	GTK_WINDOW(view->wnd),
				"logo-icon-name",
				ICON_NAME,
				"version",
				VERSION,
				"comments",
				_("A GTK frontend for MPV"),
				"website",
				"https://github.com/gnome-mpv/gnome-mpv",
				"license-type",
				GTK_LICENSE_GPL_3_0,
				"copyright",
				"\u00A9 2014-2017 The GNOME MPV authors",
				"authors",
				authors,
				"translator-credits",
				_("translator-credits"),
				NULL );
}

void gmpv_view_show_message_dialog(	GmpvView *view,
					GtkMessageType type,
					const gchar *title,
					const gchar *prefix,
					const gchar *msg )
{
	show_message_dialog(view->wnd, type, title, prefix, msg);
}

void gmpv_view_present(GmpvView *view)
{
	gtk_window_present(GTK_WINDOW(view->wnd));
}

void gmpv_view_quit(GmpvView *view)
{
	gtk_window_close(GTK_WINDOW(view->wnd));
}

void gmpv_view_reset(GmpvView *view)
{
	gmpv_main_window_reset(view->wnd);
}

void gmpv_view_queue_render(GmpvView *view)
{
	GmpvVideoArea *area = gmpv_main_window_get_video_area(view->wnd);

	gmpv_video_area_queue_render(area);
}

void gmpv_view_make_gl_context_current(GmpvView *view)
{
	GmpvVideoArea *video_area = gmpv_main_window_get_video_area(view->wnd);
	GtkGLArea *gl_area = gmpv_video_area_get_gl_area(video_area);

	gtk_widget_realize(GTK_WIDGET(gl_area));
	gtk_gl_area_make_current(gl_area);
}

void gmpv_view_set_use_opengl_cb(GmpvView *view, gboolean use_opengl_cb)
{
	GmpvVideoArea *area = gmpv_main_window_get_video_area(view->wnd);

	gmpv_video_area_set_use_opengl(area, use_opengl_cb);
}

gint gmpv_view_get_scale_factor(GmpvView *view)
{
	GdkWindow *gdk_window = gtk_widget_get_window(GTK_WIDGET(view->wnd));

	return gdk_window_get_scale_factor(gdk_window);
}

void gmpv_view_get_video_area_geometry(GmpvView *view, gint *width, gint *height)
{
	GmpvVideoArea *area = gmpv_main_window_get_video_area(view->wnd);
	GtkAllocation allocation;

	gtk_widget_get_allocation(GTK_WIDGET(area), &allocation);

	*width = allocation.width;
	*height = allocation.height;
}

void gmpv_view_move(	GmpvView *view,
			gboolean flip_x,
			gboolean flip_y,
			GValue *x,
			GValue *y )
{
	GdkScreen *screen = gdk_screen_get_default();
	GtkWindow *window = GTK_WINDOW(view->wnd);
	gint64 x_pos = -1;
	gint64 y_pos = -1;
	gint window_width = 0;
	gint window_height = 0;
	gint space_x;
	gint space_y;

	gtk_window_get_size(window, &window_width, &window_height);
	space_x = gdk_screen_get_width(screen)-window_width;
	space_y = gdk_screen_get_height(screen)-window_height;

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

void gmpv_view_resize_video_area(GmpvView *view, gint width, gint height)
{
	gmpv_main_window_resize_video_area(view->wnd, width, height);
}

void gmpv_view_set_fullscreen(GmpvView *view, gboolean fullscreen)
{
	g_object_set(view, "fullscreen", fullscreen, NULL);
	gmpv_main_window_set_fullscreen(view->wnd, fullscreen);
}

void gmpv_view_set_time_position(GmpvView *view, gdouble position)
{
	GmpvControlBox *control_box;

	control_box = gmpv_main_window_get_control_box(view->wnd);
	g_object_set(control_box, "time-position", position, NULL);
}

void gmpv_view_update_playlist(GmpvView *view, GPtrArray *playlist)
{
	GmpvPlaylistWidget *wgt = gmpv_main_window_get_playlist(view->wnd);

	gmpv_playlist_widget_update_contents(wgt, playlist);
}

void gmpv_view_set_playlist_pos(GmpvView *view, gint64 pos)
{
	g_object_set(view, "playlist-pos", pos, NULL);
}

void gmpv_view_set_playlist_visible(GmpvView *view, gboolean visible)
{
	gmpv_main_window_set_playlist_visible(view->wnd, visible);
}

gboolean gmpv_view_get_playlist_visible(GmpvView *view)
{
	return gmpv_main_window_get_playlist_visible(view->wnd);
}

void gmpv_view_set_controls_visible(GmpvView *view, gboolean visible)
{
	gmpv_main_window_set_controls_visible(view->wnd, visible);
}

gboolean gmpv_view_get_controls_visible(GmpvView *view)
{
	return gmpv_main_window_get_controls_visible(view->wnd);
}

