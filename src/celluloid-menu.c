/*
 * Copyright (c) 2015-2020 gnome-mpv
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

#include <gio/gio.h>
#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>

#include "celluloid-menu.h"
#include "celluloid-common.h"

static void
split_track_list(	const GPtrArray *track_list,
			GPtrArray **audio_tracks,
			GPtrArray **video_tracks,
			GPtrArray **subtitle_tracks );

static GMenu *
build_menu_from_track_list(	const GPtrArray *list,
				const gchar *action,
				const gchar *load_action );

static GMenu *
build_video_track_menu(const GPtrArray *list);

static GMenu *
build_audio_track_menu(const GPtrArray *list);

static GMenu *
build_subtitle_track_menu(const GPtrArray *list);

static GMenu *
build_disc_menu(const GPtrArray *disc_list);

static void
split_track_list(	const GPtrArray *track_list,
			GPtrArray **audio_tracks,
			GPtrArray **video_tracks,
			GPtrArray **subtitle_tracks )
{
	guint track_list_len = track_list?track_list->len:0;

	g_assert(audio_tracks && video_tracks && subtitle_tracks);

	/* The contents of these array are shallow-copied from track_list and
	 * therefore only the container should be freed.
	 */
	*audio_tracks = g_ptr_array_new();
	*video_tracks = g_ptr_array_new();
	*subtitle_tracks = g_ptr_array_new();

	for(guint i = 0; i < track_list_len; i++)
	{
		CelluloidTrack *track = g_ptr_array_index(track_list, i);

		switch(track->type)
		{
			case TRACK_TYPE_AUDIO:
			g_ptr_array_add(*audio_tracks, track);
			break;

			case TRACK_TYPE_VIDEO:
			g_ptr_array_add(*video_tracks, track);
			break;

			case TRACK_TYPE_SUBTITLE:
			g_ptr_array_add(*subtitle_tracks, track);
			break;

			default:
			g_assert_not_reached();
			break;
		}
	}
}

static GMenu *
build_menu_from_track_list(	const GPtrArray *list,
				const gchar *action,
				const gchar *load_action )
{
	GMenu *menu = g_menu_new();
	const glong max_len = 32;
	gchar *detailed_action;

	g_assert(list);

	detailed_action = g_strdup_printf("%s(@x 0)", action);

	g_menu_append(menu, _("None"), detailed_action);
	g_free(detailed_action);

	for(guint i = 0; i < list->len; i++)
	{
		CelluloidTrack *entry;
		glong entry_title_len;
		gchar *entry_title;
		gchar *title;

		entry = g_ptr_array_index(list, i);

		/* For simplicity, also dup the default string used when the
		 * track has no title.
		 */
		entry_title = g_strdup(entry->title?:_("Unknown"));

		/* Maximum number of bytes per UTF-8 character is 4 */
		entry_title_len = g_utf8_strlen(entry_title, 4*(max_len+1));

		if(entry_title_len > max_len)
		{
			/* Truncate the string */
			*(g_utf8_offset_to_pointer(entry_title, max_len)) = '\0';
		}

		detailed_action
			= g_strdup_printf(	"%s(@x %" G_GINT64_FORMAT ")",
						action,
						entry->id );

		/* Ellipsize the title if it's longer than max_len */
		title = g_strdup_printf(	entry->lang?
						"%s%s (%s)":"%s%s",
						entry_title,
						(entry_title_len > max_len)?
						"…":"",
						entry->lang );

		g_menu_append(menu, title, detailed_action);

		g_free(detailed_action);
		g_free(entry_title);
		g_free(title);
	}

	if(load_action)
	{
		g_menu_append(menu, _("_Load External…"), load_action);
	}

	return menu;
}

static GMenu *
build_video_track_menu(const GPtrArray *list)
{
	return	build_menu_from_track_list
		(	list,
			"win.set-video-track",
			"win.load-track('video-add')" );
}

static GMenu *
build_audio_track_menu(const GPtrArray *list)
{
	return	build_menu_from_track_list
		(	list,
			"win.set-audio-track",
			"win.load-track('audio-add')" );
}

static GMenu *
build_subtitle_track_menu(const GPtrArray *list)
{
	return	build_menu_from_track_list
		(	list,
			"win.set-subtitle-track",
			"win.load-track('sub-add')" );
}

static GMenu *
build_disc_menu(const GPtrArray *disc_list)
{
	GMenu *menu = g_menu_new();

	for(guint i = 0; disc_list && i < disc_list->len; i++)
	{
		CelluloidDisc *disc = g_ptr_array_index(disc_list, i);
		gchar *action =	g_strdup_printf
				("win.open(('%s', false))", disc->uri);

		g_menu_append(menu, disc->label, action);
		g_free(action);
	}

	if(!disc_list || disc_list->len == 0)
	{
		/* Disable the menu item by setting the action to something
		 * invalid.
		 */
		g_menu_append(menu, _("No disc found"), "_");
	}

	return menu;
}

void
celluloid_menu_build_full(	GMenu *menu,
				const GPtrArray *track_list,
				const GPtrArray *disc_list )
{
	GPtrArray *audio_tracks = NULL;
	GPtrArray *video_tracks = NULL;
	GPtrArray *subtitle_tracks = NULL;
	GMenu *video_menu = NULL;
	GMenu *audio_menu = NULL;
	GMenu *subtitle_menu = NULL;
	GMenu *disc_menu = NULL;

	split_track_list
		(track_list, &audio_tracks, &video_tracks, &subtitle_tracks);

	video_menu = build_video_track_menu(video_tracks);
	audio_menu = build_audio_track_menu(audio_tracks);
	subtitle_menu = build_subtitle_track_menu(subtitle_tracks);
	disc_menu = build_disc_menu(disc_list);

	const CelluloidMenuEntry entries[]
		= {	CELLULOID_MENU_SUBMENU(_("_File"), NULL),
			CELLULOID_MENU_ITEM(_("_Open File…"), "win.show-open-dialog((false, false))"),
			CELLULOID_MENU_ITEM(_("Open _Folder…"), "win.show-open-dialog((true, false))"),
			CELLULOID_MENU_ITEM(_("Open _Location…"), "win.show-open-location-dialog(false)"),
			CELLULOID_MENU_SUBMENU(_("Open _Disc…"), disc_menu),
			CELLULOID_MENU_ITEM(_("_Save Playlist"), "win.save-playlist"),
			CELLULOID_MENU_ITEM(_("_New Window"), "app.new-window"),
			CELLULOID_MENU_ITEM(_("_Quit"), "win.quit"),
			CELLULOID_MENU_SUBMENU(_("_Edit"), NULL),
			CELLULOID_MENU_ITEM(_("_Preferences"), "win.show-preferences-dialog"),
			CELLULOID_MENU_SUBMENU(_("_Video Track"), video_menu),
			CELLULOID_MENU_SUBMENU(_("_Audio Track"), audio_menu),
			CELLULOID_MENU_SUBMENU(_("S_ubtitle Track"), subtitle_menu),
			CELLULOID_MENU_SUBMENU(_("_View"), NULL),
			CELLULOID_MENU_ITEM(_("_Toggle Controls"), "win.toggle-controls"),
			CELLULOID_MENU_ITEM(_("_Toggle Playlist"), "win.toggle-playlist"),
			CELLULOID_MENU_ITEM(_("_Fullscreen"), "win.toggle-fullscreen"),
			CELLULOID_MENU_SUBMENU(_("_Help"), NULL),
			CELLULOID_MENU_ITEM(_("_Keyboard Shortcuts"), "win.show-shortcuts-dialog"),
			CELLULOID_MENU_ITEM(_("_About Celluloid"), "win.show-about-dialog"),
			CELLULOID_MENU_END };

	celluloid_menu_build_menu(menu, entries, FALSE);

	g_ptr_array_free(audio_tracks, FALSE);
	g_ptr_array_free(video_tracks, FALSE);
	g_ptr_array_free(subtitle_tracks, FALSE);
	g_object_unref(video_menu);
	g_object_unref(audio_menu);
	g_object_unref(subtitle_menu);
	g_object_unref(disc_menu);
}

void
celluloid_menu_build_menu_btn(GMenu *menu, const GPtrArray *track_list)
{
	GPtrArray *audio_tracks = NULL;
	GPtrArray *video_tracks = NULL;
	GPtrArray *subtitle_tracks = NULL;
	GMenu *video_menu = NULL;
	GMenu *audio_menu = NULL;
	GMenu *subtitle_menu = NULL;

	split_track_list
		(track_list, &audio_tracks, &video_tracks, &subtitle_tracks);

	video_menu = build_video_track_menu(video_tracks);
	audio_menu = build_audio_track_menu(audio_tracks);
	subtitle_menu = build_subtitle_track_menu(subtitle_tracks);

	const CelluloidMenuEntry entries[]
		= {	CELLULOID_MENU_SEPARATOR,
			CELLULOID_MENU_ITEM(_("_Toggle Controls"), "win.toggle-controls"),
			CELLULOID_MENU_SEPARATOR,
			CELLULOID_MENU_ITEM(_("_Toggle Playlist"), "win.toggle-playlist"),
			CELLULOID_MENU_ITEM(_("_Save Playlist"), "win.save-playlist"),
			CELLULOID_MENU_SEPARATOR,
			CELLULOID_MENU_SUBMENU(_("_Video Track"), video_menu),
			CELLULOID_MENU_SUBMENU(_("_Audio Track"), audio_menu),
			CELLULOID_MENU_SUBMENU(_("S_ubtitle Track"), subtitle_menu),
			CELLULOID_MENU_SEPARATOR,
			CELLULOID_MENU_ITEM(_("_Preferences"), "win.show-preferences-dialog"),
			CELLULOID_MENU_ITEM(_("_Keyboard Shortcuts"), "win.show-shortcuts-dialog"),
			CELLULOID_MENU_ITEM(_("_About Celluloid"), "win.show-about-dialog"),
			CELLULOID_MENU_END };

	celluloid_menu_build_menu(menu, entries, TRUE);

	g_ptr_array_free(audio_tracks, FALSE);
	g_ptr_array_free(video_tracks, FALSE);
	g_ptr_array_free(subtitle_tracks, FALSE);
	g_object_unref(video_menu);
	g_object_unref(audio_menu);
	g_object_unref(subtitle_menu);
}

void
celluloid_menu_build_open_btn(GMenu *menu, const GPtrArray *disc_list)
{
	GMenu *disc_menu = build_disc_menu(disc_list);

	const CelluloidMenuEntry entries[]
		= {	CELLULOID_MENU_SEPARATOR,
			CELLULOID_MENU_ITEM(_("_Open…"), "win.show-open-dialog((false, false))"),
			CELLULOID_MENU_ITEM(_("Open _Folder…"), "win.show-open-dialog((true, false))"),
			CELLULOID_MENU_ITEM(_("Open _Location…"), "win.show-open-location-dialog(false)"),
			CELLULOID_MENU_SUBMENU(_("Open _Disc"), disc_menu),
			CELLULOID_MENU_SEPARATOR,
			CELLULOID_MENU_ITEM(_("_New Window"), "app.new-window"),
			CELLULOID_MENU_END };

	celluloid_menu_build_menu(menu, entries, TRUE);

	g_clear_object(&disc_menu);
}

void
celluloid_menu_build_menu(	GMenu *menu,
				const CelluloidMenuEntry *entries,
				gboolean flat )
{
	GMenu *current_submenu = NULL;

	for(	gint i = 0;
		entries[i].title || entries[i].action || entries[i].submenu;
		i++ )
	{
		const gchar *title = entries[i].title;
		const gchar *action = entries[i].action;
		const GMenu *submenu = entries[i].submenu;

		if(title && (action || submenu))
		{
			GMenuItem *item;

			if(submenu)
			{
				item =	g_menu_item_new_submenu
					(title, G_MENU_MODEL(submenu));
			}
			else
			{
				item = g_menu_item_new(title, action);
			}

			g_menu_append_item(current_submenu, item);
			g_object_unref(item);
		}
		else
		{
			GMenuModel *model;

			current_submenu = g_menu_new();
			model = G_MENU_MODEL(current_submenu);

			if(flat)
			{
				g_menu_append_section(menu, title, model);
			}
			else
			{
				g_menu_append_submenu(menu, title, model);
			}
		}
	}
}
