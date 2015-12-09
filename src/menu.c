/*
 * Copyright (c) 2015 gnome-mpv
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

#include "menu.h"
#include "track.h"

static void build_menu_from_track_list(	GMenu *menu,
					const GSList *list,
					const gchar *action );

void build_menu_from_track_list(	GMenu *menu,
					const GSList *list,
					const gchar *action )
{
	const GSList *iter = list;

	while(iter)
	{
		Track *entry;
		gchar *detailed_action;
		gchar *title;

		entry = iter->data;

		detailed_action
			= g_strdup_printf(	"app.%s"
						"(@x %" G_GINT64_FORMAT ")",
						action,
						entry->id );

		title = g_strdup_printf(	entry->lang?"%s (%s)":"%s",
						entry->title?:_("Unknown"),
						entry->lang );

		g_menu_append(menu, title, detailed_action);

		iter = g_slist_next(iter);

		g_free(detailed_action);
		g_free(title);
	}
}

void menu_build_full(	GMenu *menu,
			const GSList *audio_list,
			const GSList *video_list,
			const GSList *sub_list )
{
	GMenu *file_menu;
	GMenu *edit_menu;
	GMenu *view_menu;
	GMenu *help_menu;
	GMenuItem *file_menu_item;
	GMenuItem *open_menu_item;
	GMenuItem *quit_menu_item;
	GMenuItem *open_loc_menu_item;
	GMenuItem *save_playlist_menu_item;
	GMenuItem *edit_menu_item;
	GMenuItem *load_audio_menu_item;
	GMenuItem *load_sub_menu_item;
	GMenuItem *pref_menu_item;
	GMenuItem *view_menu_item;
	GMenuItem *playlist_menu_item;
	GMenuItem *fullscreen_menu_item;
	GMenuItem *normal_size_menu_item;
	GMenuItem *double_size_menu_item;
	GMenuItem *half_size_menu_item;
	GMenuItem *help_menu_item;
	GMenuItem *about_menu_item;

	/* File */
	file_menu = g_menu_new();

	file_menu_item
		= g_menu_item_new_submenu
			(_("_File"), G_MENU_MODEL(file_menu));

	open_menu_item = g_menu_item_new(_("_Open"), "app.open(false)");
	quit_menu_item = g_menu_item_new(_("_Quit"), "app.quit");

	open_loc_menu_item
		= g_menu_item_new(_("Open _Location"), "app.openloc");

	save_playlist_menu_item
		= g_menu_item_new(_("_Save Playlist"), "app.playlist_save");

	/* Edit */
	edit_menu = g_menu_new();

	edit_menu_item
		= g_menu_item_new_submenu
			(_("_Edit"), G_MENU_MODEL(edit_menu));

	load_audio_menu_item
		= g_menu_item_new
			(_("_Load External..."), "app.load_track('audio-add')");

	load_sub_menu_item
		= g_menu_item_new
			(_("_Load External..."), "app.load_track('sub-add')");

	pref_menu_item
		= g_menu_item_new(_("_Preferences"), "app.pref");

	/* View */
	view_menu = g_menu_new();

	view_menu_item
		= g_menu_item_new_submenu
			(_("_View"), G_MENU_MODEL(view_menu));

	playlist_menu_item
		= g_menu_item_new(_("_Toggle Playlist"), "app.playlist_toggle");

	fullscreen_menu_item
		= g_menu_item_new(_("_Fullscreen"), "app.fullscreen");

	normal_size_menu_item
		= g_menu_item_new(_("_Normal Size"), "app.normalsize");

	double_size_menu_item
		= g_menu_item_new(_("_Double Size"), "app.doublesize");

	half_size_menu_item
		= g_menu_item_new(_("_Half Size"), "app.halfsize");

	/* Help */
	help_menu = g_menu_new();

	help_menu_item
		= g_menu_item_new_submenu
			(_("_Help"), G_MENU_MODEL(help_menu));

	about_menu_item = g_menu_item_new(_("_About"), "app.about");

	if(video_list)
	{
		GMenu *video_menu = g_menu_new();

		build_menu_from_track_list
			(video_menu, video_list, "video_select");

		g_menu_append_submenu(	edit_menu,
					_("_Video Track"),
					G_MENU_MODEL(video_menu) );
	}

	/* If there is no track, then no file is playing and we can just leave
	 * out track-related menu items. However, if there is something playing,
	 * show both menu items for audio and subtitle tracks even if they are
	 * empty so that the users can load external ones if they want.
	 */
	if(video_list || audio_list || sub_list)
	{
		GMenu *audio_menu = g_menu_new();
		GMenu *sub_menu = g_menu_new();

		build_menu_from_track_list
			(audio_menu, audio_list, "audio_select");

		build_menu_from_track_list
			(sub_menu, sub_list, "sub_select");

		g_menu_append_submenu(	edit_menu,
					_("_Audio Track"),
					G_MENU_MODEL(audio_menu) );

		g_menu_append_submenu(	edit_menu,
					_("_Subtitle Track"),
					G_MENU_MODEL(sub_menu) );

		g_menu_append_item(audio_menu, load_audio_menu_item);
		g_menu_append_item(sub_menu, load_sub_menu_item);
	}

	g_menu_append_item(menu, file_menu_item);
	g_menu_append_item(file_menu, open_menu_item);
	g_menu_append_item(file_menu, open_loc_menu_item);
	g_menu_append_item(file_menu, save_playlist_menu_item);
	g_menu_append_item(file_menu, quit_menu_item);

	g_menu_append_item(menu, edit_menu_item);
	g_menu_append_item(edit_menu, pref_menu_item);

	g_menu_append_item(menu, view_menu_item);
	g_menu_append_item(view_menu, playlist_menu_item);
	g_menu_append_item(view_menu, fullscreen_menu_item);
	g_menu_append_item(view_menu, normal_size_menu_item);
	g_menu_append_item(view_menu, double_size_menu_item);
	g_menu_append_item(view_menu, half_size_menu_item);

	g_menu_append_item(menu, help_menu_item);
	g_menu_append_item(help_menu, about_menu_item);

	g_object_unref(file_menu_item);
	g_object_unref(open_menu_item);
	g_object_unref(open_loc_menu_item);
	g_object_unref(save_playlist_menu_item);
	g_object_unref(quit_menu_item);
	g_object_unref(edit_menu_item);
	g_object_unref(pref_menu_item);
	g_object_unref(load_audio_menu_item);
	g_object_unref(load_sub_menu_item);
	g_object_unref(view_menu_item);
	g_object_unref(playlist_menu_item);
	g_object_unref(fullscreen_menu_item);
	g_object_unref(normal_size_menu_item);
	g_object_unref(double_size_menu_item);
	g_object_unref(half_size_menu_item);
	g_object_unref(help_menu_item);
	g_object_unref(about_menu_item);
}

void menu_build_menu_btn(	GMenu *menu,
				const GSList *audio_list,
				const GSList *video_list,
				const GSList *sub_list )
{
	GMenu *playlist;
	GMenu *track;
	GMenu *view;
	GMenuItem *playlist_section;
	GMenuItem *track_section;
	GMenuItem *view_section;
	GMenuItem *playlist_toggle_menu_item;
	GMenuItem *playlist_save_menu_item;
	GMenuItem *normal_size_menu_item;
	GMenuItem *double_size_menu_item;
	GMenuItem *half_size_menu_item;

	playlist = g_menu_new();
	track = g_menu_new();
	view = g_menu_new();

	playlist_section
		= g_menu_item_new_section(NULL, G_MENU_MODEL(playlist));

	track_section
		= g_menu_item_new_section(NULL, G_MENU_MODEL(track));

	view_section
		= g_menu_item_new_section(NULL, G_MENU_MODEL(view));

	playlist_toggle_menu_item
		= g_menu_item_new
			(_("_Toggle Playlist"), "app.playlist_toggle");

	playlist_save_menu_item
		= g_menu_item_new
			(_("_Save Playlist"), "app.playlist_save");

	normal_size_menu_item
		= g_menu_item_new
			(_("_Normal Size"), "app.normalsize");

	double_size_menu_item
		= g_menu_item_new
			(_("_Double Size"), "app.doublesize");

	half_size_menu_item
		= g_menu_item_new
			(_("_Half Size"), "app.halfsize");

	if(video_list)
	{
		GMenu *video = g_menu_new();
		GMenuItem *video_menu_item;

		video_menu_item = g_menu_item_new_submenu
					(_("Video Track"), G_MENU_MODEL(video));

		build_menu_from_track_list(video, video_list, "video_select");
		g_menu_append_item(track, video_menu_item);
		g_object_unref(video_menu_item);
	}

	if(video_list || audio_list || sub_list)
	{
		GMenu *audio = g_menu_new();
		GMenu *subtitle = g_menu_new();
		GMenuItem *audio_menu_item;
		GMenuItem *subtitle_menu_item;
		GMenuItem *load_audio_menu_item;
		GMenuItem *load_sub_menu_item;

		audio_menu_item
			= g_menu_item_new_submenu
				(_("Audio Track"), G_MENU_MODEL(audio));

		subtitle_menu_item
			= g_menu_item_new_submenu
				(_("Subtitle Track"), G_MENU_MODEL(subtitle));

		build_menu_from_track_list(audio, audio_list, "audio_select");
		build_menu_from_track_list(subtitle, sub_list, "sub_select");

		load_audio_menu_item
			= g_menu_item_new
				(	_("_Load External..."),
					"app.load_track('audio-add')" );

		load_sub_menu_item
			= g_menu_item_new
				(	_("_Load External..."),
					"app.load_track('sub-add')" );

		g_menu_append_item(track, audio_menu_item);
		g_menu_append_item(track, subtitle_menu_item);
		g_menu_append_item(audio, load_audio_menu_item);
		g_menu_append_item(subtitle, load_sub_menu_item);

		g_object_unref(audio_menu_item);
		g_object_unref(subtitle_menu_item);
		g_object_unref(load_audio_menu_item);
		g_object_unref(load_sub_menu_item);
	}

	g_menu_append_item(playlist, playlist_toggle_menu_item);
	g_menu_append_item(playlist, playlist_save_menu_item);
	g_menu_append_item(view, normal_size_menu_item);
	g_menu_append_item(view, double_size_menu_item);
	g_menu_append_item(view, half_size_menu_item);
	g_menu_append_item(menu, playlist_section);
	g_menu_append_item(menu, track_section);
	g_menu_append_item(menu, view_section);

	g_object_unref(playlist_toggle_menu_item);
	g_object_unref(playlist_save_menu_item);
	g_object_unref(normal_size_menu_item);
	g_object_unref(double_size_menu_item);
	g_object_unref(half_size_menu_item);
	g_object_unref(playlist_section);
	g_object_unref(track_section);
	g_object_unref(view_section);
}

void menu_build_open_btn(GMenu *menu)
{
	GMenuItem *open_menu_item;
	GMenuItem *open_loc_menu_item;

	open_menu_item
		= g_menu_item_new(_("_Open"), "app.open(false)");

	open_loc_menu_item
		= g_menu_item_new(_("Open _Location"), "app.openloc");

	g_menu_append_item(menu, open_menu_item);
	g_menu_append_item(menu, open_loc_menu_item);

	g_object_unref(open_menu_item);
	g_object_unref(open_loc_menu_item);
}

