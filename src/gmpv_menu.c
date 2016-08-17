/*
 * Copyright (c) 2015-2016 gnome-mpv
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

#include "gmpv_menu.h"
#include "gmpv_track.h"

typedef struct GmpvMenuEntry GmpvMenuEntry;

struct GmpvMenuEntry
{
	gchar *title;
	gchar *action;
	GMenu *submenu;
};

static GMenu *build_menu_from_track_list(	const GSList *list,
						const gchar *action,
						const gchar *load_action );
static void build_menu(GMenu *menu, const GmpvMenuEntry *entries, gboolean flat);
static GMenu *build_video_track_menu(const GSList *list);
static GMenu *build_audio_track_menu(const GSList *list);
static GMenu *build_subtitle_track_menu(const GSList *list);

static GMenu *build_menu_from_track_list(	const GSList *list,
						const gchar *action,
						const gchar *load_action )
{
	GMenu *menu = g_menu_new();
	const GSList *iter = list;
	const glong max_len = 32;
	gchar *detailed_action;

	detailed_action = g_strdup_printf("%s(@x 0)", action);

	g_menu_append(menu, _("None"), detailed_action);
	g_free(detailed_action);

	while(iter)
	{
		GmpvTrack *entry;
		glong entry_title_len;
		gchar *entry_title;
		gchar *title;

		entry = iter->data;

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

		iter = g_slist_next(iter);

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

static void build_menu(GMenu *menu, const GmpvMenuEntry *entries, gboolean flat)
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

static GMenu *build_video_track_menu(const GSList *list)
{
	return	build_menu_from_track_list
		(list, "app.set-video-track", NULL);
}

static GMenu *build_audio_track_menu(const GSList *list)
{
	return	build_menu_from_track_list
		(	list,
			"app.set-audio-track",
			"app.load-track('audio-add')" );
}

static GMenu *build_subtitle_track_menu(const GSList *list)
{
	return	build_menu_from_track_list
		(	list,
			"app.set-subtitle-track",
			"app.load-track('sub-add')" );
}

void gmpv_menu_build_full(	GMenu *menu,
				const GSList *audio_list,
				const GSList *video_list,
				const GSList *sub_list )
{
	GMenu *video_menu = build_video_track_menu(video_list);
	GMenu *audio_menu = build_audio_track_menu(audio_list);
	GMenu *sub_menu = build_subtitle_track_menu(sub_list);

	const GmpvMenuEntry entries[]
		= {	{_("_File"), NULL, NULL},
			{_("_Open"), "app.show-open-dialog(false)", NULL},
			{_("Open _Location"), "app.show-open-location-dialog(false)", NULL},
			{_("_Save Playlist"), "app.save-playlist", NULL},
			{_("_Quit"), "app.quit", NULL},
			{_("_Edit"), NULL, NULL},
			{_("_Preferences"), "app.show-preferences-dialog", NULL},
			{_("_Video Track"), NULL, video_menu},
			{_("_Audio Track"), NULL, audio_menu},
			{_("S_ubtitle Track"), NULL, sub_menu},
			{_("_View"), NULL, NULL},
			{_("_Toggle Controls"), "app.toggle-controls", NULL},
			{_("_Toggle Playlist"), "app.toggle-playlist", NULL},
			{_("_Fullscreen"), "app.toggle-fullscreen", NULL},
			{_("_Normal Size"), "app.set-video-size(@d 1)", NULL},
			{_("_Double Size"), "app.set-video-size(@d 2)", NULL},
			{_("_Half Size"), "app.set-video-size(@d 0.5)", NULL},
			{_("_Help"), NULL, NULL},
			{_("_Keyboard Shortcuts"), "app.show-shortcuts-dialog" , NULL},
			{_("_About"), "app.show-about-dialog", NULL},
			{NULL, NULL, NULL} };

	build_menu(menu, entries, FALSE);

	g_object_unref(video_menu);
	g_object_unref(audio_menu);
	g_object_unref(sub_menu);
}

void gmpv_menu_build_menu_btn(	GMenu *menu,
				const GSList *audio_list,
				const GSList *video_list,
				const GSList *sub_list )
{
	GMenu *video_menu = build_video_track_menu(video_list);
	GMenu *audio_menu = build_audio_track_menu(audio_list);
	GMenu *sub_menu = build_subtitle_track_menu(sub_list);

	const GmpvMenuEntry entries[]
		= {	{NULL, "", NULL},
			{_("_Toggle Controls"), "app.toggle-controls", NULL},
			{NULL, "", NULL},
			{_("_Toggle Playlist"), "app.toggle-playlist", NULL},
			{_("_Save Playlist"), "app.save-playlist", NULL},
			{NULL, "", NULL},
			{_("_Video Track"), NULL, video_menu},
			{_("_Audio Track"), NULL, audio_menu},
			{_("S_ubtitle Track"), NULL, sub_menu},
			{NULL, "", NULL},
			{_("_Normal Size"), "app.set-video-size(@d 1)", NULL},
			{_("_Double Size"), "app.set-video-size(@d 2)", NULL},
			{_("_Half Size"), "app.set-video-size(@d 0.5)", NULL},
			{NULL, NULL, NULL} };

	build_menu(menu, entries, TRUE);

	g_object_unref(video_menu);
	g_object_unref(audio_menu);
	g_object_unref(sub_menu);
}

void gmpv_menu_build_open_btn(GMenu *menu)
{
	const GmpvMenuEntry entries[]
		= {	{NULL, "", NULL},
			{_("_Open"), "app.show-open-dialog(false)", NULL},
			{_("Open _Location"), "app.show-open-location-dialog(false)", NULL},
			{NULL, NULL, NULL} };

	build_menu(menu, entries, TRUE);
}

void gmpv_menu_build_app_menu(GMenu *menu)
{
	const GmpvMenuEntry entries[]
		= {	{NULL, "", NULL},
			{_("_Preferences"), "app.show-preferences-dialog", NULL},
			{NULL, "", NULL},
			{_("_Keyboard Shortcuts"), "app.show-shortcuts-dialog", NULL},
			{_("_About"), "app.show-about-dialog", NULL},
			{_("_Quit"), "app.quit", NULL},
			{NULL, NULL, NULL} };

	build_menu(menu, entries, TRUE);
}

