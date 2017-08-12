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

#include "gmpv_player.h"
#include "gmpv_mpv_wrapper.h"

struct _GmpvPlayer
{
	GmpvMpv parent;
	GPtrArray *track_list;
};

struct _GmpvPlayerClass
{
	GmpvMpvClass parent_class;
};

static void finalize(GObject *object);
static void mpv_property_changed_handler(	GmpvMpv *mpv,
						const gchar *name,
						gpointer value );
static GmpvTrack *parse_track_entry(mpv_node_list *node);
static void update_track_list(GmpvPlayer *player);

G_DEFINE_TYPE(GmpvPlayer, gmpv_player, GMPV_TYPE_MPV)

static void finalize(GObject *object)
{
	g_ptr_array_free(GMPV_PLAYER(object)->track_list, TRUE);

	G_OBJECT_CLASS(gmpv_player_parent_class)->finalize(object);
}

static void mpv_property_changed_handler(	GmpvMpv *mpv,
						const gchar *name,
						gpointer value )
{
	if(g_strcmp0(name, "track-list") == 0)
	{
		update_track_list(GMPV_PLAYER(mpv));
	}

	GMPV_MPV_CLASS(gmpv_player_parent_class)
		->mpv_property_changed(mpv, name, value);
}

static GmpvTrack *parse_track_entry(mpv_node_list *node)
{
	GmpvTrack *entry = gmpv_track_new();

	for(gint i = 0; i < node->num; i++)
	{
		if(g_strcmp0(node->keys[i], "type") == 0)
		{
			const gchar *type = node->values[i].u.string;

			if(g_strcmp0(type, "audio") == 0)
			{
				entry->type = TRACK_TYPE_AUDIO;
			}
			else if(g_strcmp0(type, "video") == 0)
			{
				entry->type = TRACK_TYPE_VIDEO;
			}
			else if(g_strcmp0(type, "sub") == 0)
			{
				entry->type = TRACK_TYPE_SUBTITLE;
			}
		}
		else if(g_strcmp0(node->keys[i], "title") == 0)
		{
			entry->title = g_strdup(node->values[i].u.string);
		}
		else if(g_strcmp0(node->keys[i], "lang") == 0)
		{
			entry->lang = g_strdup(node->values[i].u.string);
		}
		else if(g_strcmp0(node->keys[i], "id") == 0)
		{
			entry->id = node->values[i].u.int64;
		}
	}

	return entry;
}

static void update_track_list(GmpvPlayer *player)
{
	mpv_node_list *org_list = NULL;
	mpv_node track_list;

	g_ptr_array_set_size(player->track_list, 0);
	gmpv_mpv_get_property(	GMPV_MPV(player),
				"track-list",
				MPV_FORMAT_NODE,
				&track_list );
	org_list = track_list.u.list;

	if(track_list.format == MPV_FORMAT_NODE_ARRAY)
	{
		for(gint i = 0; i < org_list->num; i++)
		{
			GmpvTrack *entry =	parse_track_entry
						(org_list->values[i].u.list);

			g_ptr_array_add(player->track_list, entry);
		}

		mpv_free_node_contents(&track_list);
	}
}

static void gmpv_player_class_init(GmpvPlayerClass *klass)
{
	GmpvMpvClass *mpv_class = GMPV_MPV_CLASS(klass);
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	mpv_class->mpv_property_changed = mpv_property_changed_handler;
	obj_class->finalize = finalize;
}

static void gmpv_player_init(GmpvPlayer *player)
{
	player->track_list = g_ptr_array_new_full(	1,
							(GDestroyNotify)
							gmpv_track_free );
}

GmpvPlayer *gmpv_player_new(gint64 wid)
{
	return GMPV_PLAYER(g_object_new(	gmpv_player_get_type(),
						"wid", wid,
						NULL ));
}

GPtrArray *gmpv_player_get_track_list(GmpvPlayer *player)
{
	return player->track_list;
}

