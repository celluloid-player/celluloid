/*
 * Copyright (c) 2015-2017 gnome-mpv
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

#include <gio/gio.h>
#include <glib-object.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <mpv/client.h>
#include <string.h>

#include "gmpv_actionctl.h"
#include "gmpv_file_chooser.h"
#include "gmpv_application_private.h"
#include "gmpv_controller.h"
#include "gmpv_playlist_widget.h"
#include "gmpv_def.h"
#include "gmpv_mpv.h"
#include "gmpv_mpv_wrapper.h"
#include "gmpv_main_window.h"
#include "gmpv_playlist.h"
#include "gmpv_open_loc_dialog.h"
#include "gmpv_pref_dialog.h"
#include "gmpv_common.h"
#include "gmpv_control_box.h"
#include "gmpv_authors.h"

#if GTK_CHECK_VERSION(3, 20, 0)
#include "gmpv_shortcuts_window.h"
#endif

static void set_track(GmpvMpv *mpv, const gchar *prop, gint64 id);
static void save_playlist(GmpvPlaylist *playlist, GFile *file, GError **error);
static void show_open_dialog_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data );
static void show_open_location_dialog_handler(	GSimpleAction *action,
						GVariant *param,
						gpointer data );
static void toggle_loop_handler(	GSimpleAction *action,
					GVariant *value,
					gpointer data );
static void show_shortcuts_dialog_handler(	GSimpleAction *action,
						GVariant *param,
						gpointer data );
static void toggle_controls_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data );
static void toggle_playlist_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data );
static void save_playlist_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data );
static void remove_selected_playlist_item_handler(	GSimpleAction *action,
							GVariant *param,
							gpointer data );
static void show_preferences_dialog_handler(	GSimpleAction *action,
						GVariant *param,
						gpointer data );
static void quit_handler(	GSimpleAction *action,
				GVariant *param,
				gpointer data );
static void set_audio_track_handler(	GSimpleAction *action,
					GVariant *value,
					gpointer data );
static void set_video_track_handler(	GSimpleAction *action,
					GVariant *value,
					gpointer data );
static void set_subtitle_track_handler(	GSimpleAction *action,
					GVariant *value,
					gpointer data );
static void load_track_handler(	GSimpleAction *action,
				GVariant *param,
				gpointer data );
static void toggle_fullscreen_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data );
static void enter_fullscreen_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data );
static void leave_fullscreen_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data );
static void set_video_size_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data );
static void show_about_dialog_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data );

static void set_track(GmpvMpv *mpv, const gchar *prop, gint64 id)
{
	if(id > 0)
	{
		gmpv_mpv_set_property(mpv, prop, MPV_FORMAT_INT64, &id);
	}
	else
	{
		gmpv_mpv_set_property_string(mpv, prop, "no");
	}
}

static void save_playlist(GmpvPlaylist *playlist, GFile *file, GError **error)
{
	gboolean rc = TRUE;
	GOutputStream *dest_stream = NULL;
	GtkTreeModel *model = GTK_TREE_MODEL(gmpv_playlist_get_store(playlist));
	GtkTreeIter iter;

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
		rc = gtk_tree_model_get_iter_first(model, &iter);
		rc &= !!dest_stream;
	}

	while(rc)
	{
		gchar *uri;
		gsize written;

		gtk_tree_model_get(model, &iter, PLAYLIST_URI_COLUMN, &uri, -1);

		rc &=	g_output_stream_printf
			(dest_stream, &written, NULL, error, "%s\n", uri);
		rc &= gtk_tree_model_iter_next(model, &iter);
	}

	if(dest_stream)
	{
		g_output_stream_close(dest_stream, NULL, error);
	}
}

static void show_open_dialog_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data )
{
	gboolean append = g_variant_get_boolean(param);

	gmpv_view_show_open_dialog
		(GMPV_APPLICATION(data)->view, append);
}

static void show_open_location_dialog_handler(	GSimpleAction *action,
						GVariant *param,
						gpointer data )
{
	gboolean append = g_variant_get_boolean(param);

	gmpv_view_show_open_location_dialog
		(GMPV_APPLICATION(data)->view, append);
}

static void toggle_loop_handler(	GSimpleAction *action,
					GVariant *value,
					gpointer data )
{
	GmpvApplication *app = data;
	GmpvMpv *mpv = gmpv_application_get_mpv(app);
	gboolean loop = g_variant_get_boolean(value);

	g_simple_action_set_state(action, value);
	gmpv_mpv_set_property_string(mpv, "loop", loop?"inf":"no");
}

static void show_shortcuts_dialog_handler(	GSimpleAction *action,
						GVariant *param,
						gpointer data )
{
	gmpv_view_show_shortcuts_dialog(GMPV_APPLICATION(data)->view);
}

static void toggle_controls_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data )
{
	GmpvApplication *app = data;
	GmpvMainWindow *wnd = gmpv_application_get_main_window(app);
	GmpvControlBox *ctrl = gmpv_main_window_get_control_box(wnd);
	gboolean visible = gtk_widget_get_visible(GTK_WIDGET(ctrl));

	gtk_widget_set_visible(GTK_WIDGET(ctrl), !visible);
}

static void toggle_playlist_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data )
{
	GmpvApplication *app = data;
	GmpvMainWindow *wnd = gmpv_application_get_main_window(app);
	gboolean visible = gmpv_main_window_get_playlist_visible(wnd);

	gmpv_main_window_set_playlist_visible(wnd, !visible);
}

static void save_playlist_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data )
{
	GmpvApplication *app = data;
	GmpvMpv *mpv;
	GmpvMainWindow *wnd;
	GmpvPlaylist *playlist;
	GFile *dest_file;
	GmpvFileChooser *file_chooser;
	GtkFileChooser *gtk_chooser;
	GError *error;

	mpv = gmpv_application_get_mpv(app);
	wnd = gmpv_application_get_main_window(app);
	playlist = gmpv_mpv_get_playlist(mpv);
	dest_file = NULL;
	file_chooser =	gmpv_file_chooser_new
			(	_("Save Playlist"),
				GTK_WINDOW(wnd),
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
		save_playlist(playlist, dest_file, &error);
		g_object_unref(dest_file);
	}

	if(error)
	{
		show_message_dialog(	wnd,
					GTK_MESSAGE_ERROR,
					NULL,
					error->message,
					_("Error") );

		g_error_free(error);
	}
}

static void remove_selected_playlist_item_handler(	GSimpleAction *action,
							GVariant *param,
							gpointer data )
{
	GmpvMainWindow *wnd =	gmpv_application_get_main_window
				(GMPV_APPLICATION(data));

	if(gmpv_main_window_get_playlist_visible(wnd))
	{
		GmpvPlaylistWidget *playlist;

		playlist = gmpv_main_window_get_playlist(wnd);

		gmpv_playlist_widget_remove_selected(playlist);
	}
}

static void show_preferences_dialog_handler(	GSimpleAction *action,
						GVariant *param,
						gpointer data )
{
	gmpv_view_show_preferences_dialog(GMPV_APPLICATION(data)->view);
}

static void quit_handler(	GSimpleAction *action,
				GVariant *param,
				gpointer data )
{
	gmpv_application_quit(data);
}

static void set_audio_track_handler(	GSimpleAction *action,
					GVariant *value,
					gpointer data )
{
	GmpvMpv *mpv = gmpv_application_get_mpv(data);
	gint64 id;

	g_simple_action_set_state(action, value);
	g_variant_get(value, "x", &id);
	set_track(mpv, "aid", id);
}

static void set_video_track_handler(	GSimpleAction *action,
					GVariant *value,
					gpointer data )
{
	GmpvMpv *mpv = gmpv_application_get_mpv(data);
	gint64 id;

	g_simple_action_set_state(action, value);
	g_variant_get(value, "x", &id);
	set_track(mpv, "vid", id);
}

static void set_subtitle_track_handler(	GSimpleAction *action,
					GVariant *value,
					gpointer data )
{
	GmpvMpv *mpv = gmpv_application_get_mpv(data);
	gint64 id;

	g_simple_action_set_state(action, value);
	g_variant_get(value, "x", &id);
	set_track(mpv, "sid", id);
}

static void load_track_handler(	GSimpleAction *action,
				GVariant *param,
				gpointer data )
{
	GmpvApplication *app = data;
	gchar *cmd_name = NULL;

	g_variant_get(param, "s", &cmd_name);

	if(g_strcmp0(cmd_name, "audio-add") == 0)
	{
		gmpv_view_show_open_audio_track_dialog(app->view);
	}
	else if(g_strcmp0(cmd_name, "sub-add") == 0)
	{
		gmpv_view_show_open_subtitle_track_dialog(app->view);
	}
	else
	{
		g_assert_not_reached();
	}

	g_free(cmd_name);
}

static void toggle_fullscreen_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data )
{
	GmpvView *view = GMPV_APPLICATION(data)->view;
	gboolean fullscreen = FALSE;

	g_object_get(view, "fullscreen", &fullscreen, NULL);
	gmpv_view_set_fullscreen(view, !fullscreen);
}

static void enter_fullscreen_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data )
{
	gmpv_view_set_fullscreen(GMPV_APPLICATION(data)->view, TRUE);
}

static void leave_fullscreen_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data )
{
	gmpv_view_set_fullscreen(GMPV_APPLICATION(data)->view, FALSE);
}

static void set_video_size_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data )
{
	gdouble value = g_variant_get_double(param);

	resize_window_to_fit(GMPV_APPLICATION(data), value);
}

static void show_about_dialog_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data )
{
	gmpv_view_show_about_dialog(GMPV_APPLICATION(data)->view);
}

void gmpv_actionctl_map_actions(GmpvApplication *app)
{
	const GActionEntry entries[]
		= {	{.name = "show-open-dialog",
			.activate = show_open_dialog_handler,
			.parameter_type = "b"},
			{.name = "quit",
			.activate = quit_handler},
			{.name = "show-about-dialog",
			.activate = show_about_dialog_handler},
			{.name = "show-preferences-dialog",
			.activate = show_preferences_dialog_handler},
			{.name = "show-open-location-dialog",
			.activate = show_open_location_dialog_handler,
			.parameter_type = "b"},
			{.name = "toggle-loop",
			.state = "false",
			.change_state = toggle_loop_handler},
			{.name = "show-shortcuts-dialog",
			.activate = show_shortcuts_dialog_handler},
			{.name = "toggle-controls",
			.activate = toggle_controls_handler},
			{.name = "toggle-playlist",
			.activate = toggle_playlist_handler},
			{.name = "save-playlist",
			.activate = save_playlist_handler},
			{.name = "remove-selected-playlist-item",
			.activate = remove_selected_playlist_item_handler},
			{.name = "set-audio-track",
			.change_state = set_audio_track_handler,
			.state = "@x 0",
			.parameter_type = "x"},
			{.name = "set-video-track",
			.change_state = set_video_track_handler,
			.state = "@x 0",
			.parameter_type = "x"},
			{.name = "set-subtitle-track",
			.change_state = set_subtitle_track_handler,
			.state = "@x 0",
			.parameter_type = "x"},
			{.name = "load-track",
			.activate = load_track_handler,
			.parameter_type = "s"},
			{.name = "toggle-fullscreen",
			.activate = toggle_fullscreen_handler},
			{.name = "enter-fullscreen",
			.activate = enter_fullscreen_handler},
			{.name = "leave-fullscreen",
			.activate = leave_fullscreen_handler},
			{.name = "set-video-size",
			.activate = set_video_size_handler,
			.parameter_type = "d"} };

	g_action_map_add_action_entries(	G_ACTION_MAP(app),
						entries,
						G_N_ELEMENTS(entries),
						app );
}
