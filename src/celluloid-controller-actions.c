/*
 * Copyright (c) 2015-2022 gnome-mpv
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

#include "celluloid-controller-actions.h"
#include "celluloid-controller-private.h"
#include "celluloid-file-chooser.h"
#include "celluloid-common.h"

static gboolean
boolean_to_state(	GBinding *binding,
		const GValue *from_value,
		GValue *to_value,
		gpointer user_data );

static gboolean
state_to_boolean(	GBinding *binding,
		const GValue *from_value,
		GValue *to_value,
		gpointer user_data );

static gboolean
track_id_to_state(	GBinding *binding,
			const GValue *from_value,
			GValue *to_value,
			gpointer user_data );

static gboolean
state_to_track_id(	GBinding *binding,
			const GValue *from_value,
			GValue *to_value,
			gpointer user_data );

static gboolean
loop_to_state(	GBinding *binding,
		const GValue *from_value,
		GValue *to_value,
		gpointer user_data );

static gboolean
state_to_loop(	GBinding *binding,
		const GValue *from_value,
		GValue *to_value,
		gpointer user_data );

static void
bind_properties(CelluloidController *controller);

static void
open_handler(GSimpleAction *action, GVariant *param, gpointer data);

static void
show_open_dialog_handler(GSimpleAction *action, GVariant *param, gpointer data);

static void
show_open_location_dialog_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data );

static void
toggle_loop_handler(GSimpleAction *action, GVariant *value, gpointer data);

static void
show_shortcuts_dialog_handler(	GSimpleAction *action,
				GVariant *param,
				gpointer data );

static void
toggle_controls_handler(	GSimpleAction *action,
				GVariant *param,
				gpointer data );

static void
toggle_playlist_handler(GSimpleAction *action, GVariant *param, gpointer data);

static void
save_playlist_handler(GSimpleAction *action, GVariant *param, gpointer data);

static void
search_playlist_handler(GSimpleAction *action, GVariant *param, gpointer data);

static void
stop_search_playlist_handler(GSimpleAction *action, GVariant *param, gpointer data);

static void
toggle_shuffle_playlist_handler(GSimpleAction *action, GVariant *param, gpointer data);

static void
copy_selected_playlist_item_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data );

static void
remove_selected_playlist_item_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data );

static void
show_preferences_dialog_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data );

static void
quit_handler(GSimpleAction *action, GVariant *param, gpointer data);

static void
toggle_main_menu_handler(GSimpleAction *action, GVariant *param, gpointer data);

static void
set_audio_track_handler(GSimpleAction *action, GVariant *value, gpointer data);

static void
set_video_track_handler(GSimpleAction *action, GVariant *value, gpointer data);

static void
set_subtitle_track_handler(	GSimpleAction *action,
				GVariant *value,
				gpointer data );

static void
load_track_handler(GSimpleAction *action, GVariant *param, gpointer data);

static void
toggle_fullscreen_handler(	GSimpleAction *action,
				GVariant *param,
				gpointer data );

static void
enter_fullscreen_handler(GSimpleAction *action, GVariant *param, gpointer data);

static void
leave_fullscreen_handler(GSimpleAction *action, GVariant *param, gpointer data);

static void
set_video_size_handler(GSimpleAction *action, GVariant *param, gpointer data);

static void
show_about_dialog_handler(	GSimpleAction *action,
				GVariant *param,
				gpointer data );

static gboolean
boolean_to_state(	GBinding *binding,
		const GValue *from_value,
		GValue *to_value,
		gpointer user_data )
{
	gboolean from = g_value_get_boolean(from_value);
	GVariant *to = g_variant_new("b", from);

	g_value_set_variant(to_value, to);

	return TRUE;
}

static gboolean
state_to_boolean(	GBinding *binding,
		const GValue *from_value,
		GValue *to_value,
		gpointer user_data )
{
	GVariant *from = g_value_get_variant(from_value);
	gboolean to = g_variant_get_boolean(from);

	g_value_set_boolean(to_value, to);

	return TRUE;
}

static gboolean
track_id_to_state(	GBinding *binding,
			const GValue *from_value,
			GValue *to_value,
			gpointer user_data )
{
	const gchar *id = g_value_get_string(from_value);
	gint64 value = g_strcmp0(id, "no") == 0?0:g_ascii_strtoll(id, NULL, 10);
	GVariant *to = g_variant_new("x", value);

	g_value_set_variant(to_value, to);

	return TRUE;
}

static gboolean
state_to_track_id(	GBinding *binding,
			const GValue *from_value,
			GValue *to_value,
			gpointer user_data )
{
	GVariant *from = g_value_get_variant(from_value);
	gint64 id = g_variant_get_int64(from);
	gchar *value =	id <= 0?
			g_strdup("no"):
			g_strdup_printf("%" G_GINT64_FORMAT, id);

	g_value_take_string(to_value, value);

	return TRUE;
}

static gboolean
loop_to_state(	GBinding *binding,
		const GValue *from_value,
		GValue *to_value,
		gpointer user_data )
{
	const gchar *from = g_value_get_string(from_value);
	GVariant *to = g_variant_new("b", g_strcmp0(from, "no") != 0);

	g_value_set_variant(to_value, to);

	return TRUE;
}

static gboolean
state_to_loop(	GBinding *binding,
		const GValue *from_value,
		GValue *to_value,
		gpointer user_data )
{
	GVariant *from = g_value_get_variant(from_value);
	gboolean to = g_variant_get_boolean(from);

	g_value_set_static_string(to_value, to?"inf":"no");

	return TRUE;
}

static void
bind_properties(CelluloidController *controller)
{
	CelluloidMainWindow *window =	celluloid_view_get_main_window
					(controller->view);
	GAction *action = NULL;

	action =	g_action_map_lookup_action
			(G_ACTION_MAP(window), "set-audio-track");
	g_object_bind_property_full(	controller->model, "aid",
					action, "state",
					G_BINDING_BIDIRECTIONAL,
					track_id_to_state,
					state_to_track_id,
					NULL,
					NULL );

	action =	g_action_map_lookup_action
			(G_ACTION_MAP(window), "set-video-track");
	g_object_bind_property_full(	controller->model, "vid",
					action, "state",
					G_BINDING_BIDIRECTIONAL,
					track_id_to_state,
					state_to_track_id,
					NULL,
					NULL );

	action =	g_action_map_lookup_action
			(G_ACTION_MAP(window), "set-subtitle-track");
	g_object_bind_property_full(	controller->model, "sid",
					action, "state",
					G_BINDING_BIDIRECTIONAL,
					track_id_to_state,
					state_to_track_id,
					NULL,
					NULL );

	action =	g_action_map_lookup_action
			(G_ACTION_MAP(window), "toggle-shuffle-playlist");
	g_object_bind_property_full(	controller->model, "shuffle",
					action, "state",
					G_BINDING_BIDIRECTIONAL,
					boolean_to_state,
					state_to_boolean,
					NULL,
					NULL );

	action =	g_action_map_lookup_action
			(G_ACTION_MAP(window), "toggle-loop-file");
	g_object_bind_property_full(	controller->model, "loop-file",
					action, "state",
					G_BINDING_BIDIRECTIONAL,
					loop_to_state,
					state_to_loop,
					NULL,
					NULL );

	action =	g_action_map_lookup_action
			(G_ACTION_MAP(window), "toggle-loop-playlist");
	g_object_bind_property_full(	controller->model, "loop-playlist",
					action, "state",
					G_BINDING_BIDIRECTIONAL,
					loop_to_state,
					state_to_loop,
					NULL,
					NULL );
}

static void
open_handler(GSimpleAction *action, GVariant *param, gpointer data)
{
	gchar *uri = NULL;
	gboolean append = FALSE;

	g_variant_get(param, "(sb)", &uri, &append);
	celluloid_controller_open(data, uri, append);

	g_free(uri);
}

static void
show_open_dialog_handler(GSimpleAction *action, GVariant *param, gpointer data)
{
	gboolean folder = FALSE;
	gboolean append = FALSE;

	g_variant_get(param, "(bb)", &folder, &append);

	celluloid_view_show_open_dialog
		(celluloid_controller_get_view(data), folder, append);
}

static void
show_open_location_dialog_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data )
{
	gboolean append = g_variant_get_boolean(param);

	celluloid_view_show_open_location_dialog
		(celluloid_controller_get_view(data), append);
}

static void
toggle_loop_handler(GSimpleAction *action, GVariant *value, gpointer data)
{
	g_simple_action_set_state(action, value);
}

static void
show_shortcuts_dialog_handler(	GSimpleAction *action,
				GVariant *param,
				gpointer data )
{
	celluloid_view_show_shortcuts_dialog(celluloid_controller_get_view(data));
}

static void
toggle_controls_handler(GSimpleAction *action, GVariant *param, gpointer data)
{
	CelluloidView *view = celluloid_controller_get_view(data);
	gboolean visible = celluloid_view_get_controls_visible(view);

	celluloid_view_set_controls_visible(view, !visible);
}

static void
toggle_playlist_handler(GSimpleAction *action, GVariant *param, gpointer data)
{
	CelluloidView *view = celluloid_controller_get_view(data);
	gboolean visible = celluloid_view_get_playlist_visible(view);

	celluloid_view_set_playlist_visible(view, !visible);
}

static void
save_playlist_handler(GSimpleAction *action, GVariant *param, gpointer data)
{
	celluloid_view_show_save_playlist_dialog
		(celluloid_controller_get_view(data));
}

static void
search_playlist_handler(GSimpleAction *action, GVariant *param, gpointer data)
{
	CelluloidView *view = celluloid_controller_get_view(data);

	g_object_set(view, "searching", TRUE, NULL);
}

static void
stop_search_playlist_handler(GSimpleAction *action, GVariant *param, gpointer data)
{
	CelluloidView *view = celluloid_controller_get_view(data);

	g_object_set(view, "searching", FALSE, NULL);
}

static void
toggle_shuffle_playlist_handler(GSimpleAction *action, GVariant *value, gpointer data)
{
	g_simple_action_set_state(action, value);
}

static void
copy_selected_playlist_item_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data )
{
	CelluloidView *view = celluloid_controller_get_view(data);
	CelluloidMainWindow *wnd = celluloid_view_get_main_window(view);

	if(celluloid_main_window_get_playlist_visible(wnd))
	{
		CelluloidPlaylistWidget *playlist;

		playlist = celluloid_main_window_get_playlist(wnd);

		celluloid_playlist_widget_copy_selected(playlist);
	}
}

static void
remove_selected_playlist_item_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data )
{
	CelluloidView *view = celluloid_controller_get_view(data);
	CelluloidMainWindow *wnd = celluloid_view_get_main_window(view);

	if(celluloid_main_window_get_playlist_visible(wnd))
	{
		CelluloidPlaylistWidget *playlist;

		playlist = celluloid_main_window_get_playlist(wnd);

		celluloid_playlist_widget_remove_selected(playlist);
	}
}

static void
show_preferences_dialog_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data )
{
	celluloid_view_show_preferences_dialog(celluloid_controller_get_view(data));
}

static void
quit_handler(GSimpleAction *action, GVariant *param, gpointer data)
{
	celluloid_controller_quit(data);
}

static void
toggle_main_menu_handler(GSimpleAction *action, GVariant *param, gpointer data)
{
	CelluloidView *view = celluloid_controller_get_view(data);
	CelluloidMainWindow *wnd = CELLULOID_MAIN_WINDOW(view);
	gboolean csd = celluloid_main_window_get_csd_enabled(wnd);
	gboolean visible = celluloid_view_get_main_menu_visible(view);

	if(csd)
	{
		celluloid_view_set_main_menu_visible(view, !visible);
	}
}

static void
set_audio_track_handler(GSimpleAction *action, GVariant *value, gpointer data)
{
	g_simple_action_set_state(action, value);
}

static void
set_video_track_handler(GSimpleAction *action, GVariant *value, gpointer data)
{
	g_simple_action_set_state(action, value);
}

static void
set_subtitle_track_handler(	GSimpleAction *action,
				GVariant *value,
				gpointer data )
{
	g_simple_action_set_state(action, value);
}

static void
load_track_handler(GSimpleAction *action, GVariant *param, gpointer data)
{
	CelluloidView *view = celluloid_controller_get_view(data);
	gchar *cmd_name = NULL;

	g_variant_get(param, "s", &cmd_name);

	if(g_strcmp0(cmd_name, "audio-add") == 0)
	{
		celluloid_view_show_open_audio_track_dialog(view);
	}
	else if(g_strcmp0(cmd_name, "video-add") == 0)
	{
		celluloid_view_show_open_video_track_dialog(view);
	}
	else if(g_strcmp0(cmd_name, "sub-add") == 0)
	{
		celluloid_view_show_open_subtitle_track_dialog(view);
	}
	else
	{
		g_assert_not_reached();
	}

	g_free(cmd_name);
}

static void
toggle_fullscreen_handler(GSimpleAction *action, GVariant *param, gpointer data)
{
	CelluloidView *view = celluloid_controller_get_view(data);
	const gboolean fullscreen = gtk_window_is_fullscreen(GTK_WINDOW(view));

	celluloid_view_set_fullscreen(view, !fullscreen);
}

static void
enter_fullscreen_handler(GSimpleAction *action, GVariant *param, gpointer data)
{
	celluloid_view_set_fullscreen(celluloid_controller_get_view(data), TRUE);
}

static void
leave_fullscreen_handler(GSimpleAction *action, GVariant *param, gpointer data)
{
	celluloid_view_set_fullscreen(celluloid_controller_get_view(data), FALSE);
}

static void
set_video_size_handler(GSimpleAction *action, GVariant *param, gpointer data)
{
	CelluloidController *controller = data;
	gdouble value = g_variant_get_double(param);

	g_object_set(controller->model, "window-scale", value, NULL);
}

static void
show_about_dialog_handler(GSimpleAction *action, GVariant *param, gpointer data)
{
	celluloid_view_show_about_window(celluloid_controller_get_view(data));
}

void
celluloid_controller_action_register_actions(CelluloidController *controller)
{
	const GActionEntry entries[]
		= {	{.name = "open",
			.activate = open_handler,
			.parameter_type = "(sb)"},
			{.name = "show-open-dialog",
			.activate = show_open_dialog_handler,
			.parameter_type = "(bb)"},
			{.name = "quit",
			.activate = quit_handler},
			{.name = "show-about-dialog",
			.activate = show_about_dialog_handler},
			{.name = "show-preferences-dialog",
			.activate = show_preferences_dialog_handler},
			{.name = "show-open-location-dialog",
			.activate = show_open_location_dialog_handler,
			.parameter_type = "b"},
			{.name = "toggle-loop-file",
			.state = "false",
			.change_state = toggle_loop_handler},
			{.name = "toggle-loop-playlist",
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
			{.name = "search-playlist",
			.activate = search_playlist_handler},
			{.name = "stop-search-playlist",
			.activate = stop_search_playlist_handler},
			{.name = "toggle-shuffle-playlist",
			.state = "false",
			.change_state = toggle_shuffle_playlist_handler},
			{.name = "copy-selected-playlist-item",
			.activate = copy_selected_playlist_item_handler},
			{.name = "remove-selected-playlist-item",
			.activate = remove_selected_playlist_item_handler},
			{.name = "toggle-main-menu",
			.activate = toggle_main_menu_handler},
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

	CelluloidMainWindow *window =	celluloid_view_get_main_window
					(controller->view);

	g_action_map_add_action_entries(	G_ACTION_MAP(window),
						entries,
						G_N_ELEMENTS(entries),
						controller );

	bind_properties(controller);
}
