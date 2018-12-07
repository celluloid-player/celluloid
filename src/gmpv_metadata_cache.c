/*
 * Copyright (c) 2017 gnome-mpv
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

#include "gmpv_metadata_cache.h"
#include "gmpv_mpv.h"
#include "gmpv_mpv_wrapper.h"

struct _GmpvMetadataCache
{
	GObject parent;
	GHashTable *table;
	GmpvMpv *fetcher;
	GQueue *fetch_queue;
	guint fetch_timeout_id;
};

struct _GmpvMetadataCacheClass
{
	GObjectClass parent_class;
};

static void dispose(GObject *object)
{
	GmpvMetadataCache *cache = GMPV_METADATA_CACHE(object);

	g_source_clear(&cache->fetch_timeout_id);
	g_clear_object(&GMPV_METADATA_CACHE(object)->fetcher);
}

static void finalize(GObject *object)
{
	GmpvMetadataCache *cache = GMPV_METADATA_CACHE(object);

	g_hash_table_unref(cache->table);
	g_queue_free_full(cache->fetch_queue, g_free);
}

static void metadata_to_ptr_array(mpv_node metadata, GPtrArray *array);
static void mpv_event_notify(	GmpvMpv *mpv,
				gint event_id,
				gpointer event_data,
				gpointer data );
static gboolean fetch_metadata(GmpvMetadataCache *cache);
static GmpvMetadataCacheEntry *gmpv_metadata_cache_entry_new(void);
static void gmpv_metadata_cache_entry_free(GmpvMetadataCacheEntry *entry);

G_DEFINE_TYPE(GmpvMetadataCache, gmpv_metadata_cache, G_TYPE_OBJECT)

static GmpvMetadataCacheEntry *gmpv_metadata_cache_entry_new(void)
{
	GmpvMetadataCacheEntry *entry = g_new0(GmpvMetadataCacheEntry, 1);

	entry->tags =	g_ptr_array_new_with_free_func
			((GDestroyNotify)gmpv_metadata_entry_free);

	return entry;
}

static void gmpv_metadata_cache_entry_free(GmpvMetadataCacheEntry *entry)
{
	g_free(entry->title);
	g_ptr_array_free(entry->tags, TRUE);
	g_free(entry);
}

static void metadata_to_ptr_array(mpv_node metadata, GPtrArray *array)
{
	mpv_node_list *list = metadata.u.list;

	g_ptr_array_set_size(array, 0);

	if(metadata.format == MPV_FORMAT_NODE_MAP && list->num > 0)
	{
		for(gint i = 0; i < list->num; i++)
		{
			const gchar *key = list->keys[i];
			mpv_node value = list->values[i];

			if(value.format == MPV_FORMAT_STRING)
			{
				GmpvMetadataEntry *entry;

				entry =	gmpv_metadata_entry_new
					(key, value.u.string);

				g_ptr_array_add(array, entry);
			}
			else
			{
				g_warning(	"Ignored metadata field %s "
						"with unexpected format %d",
						key,
						value.format );
			}
		}
	}
}

static void mpv_event_notify(	GmpvMpv *mpv,
				gint event_id,
				gpointer event_data,
				gpointer data )
{
	if(event_id == MPV_EVENT_FILE_LOADED)
	{
		GmpvMetadataCache *cache = data;
		GmpvMetadataCacheEntry *entry = NULL;
		gchar *path = NULL;

		gmpv_mpv_get_property(mpv, "path", MPV_FORMAT_STRING, &path);
		g_debug("Fetched metadata for %s", path);

		entry =	g_hash_table_lookup(cache->table, path);

		if(entry)
		{
			const gchar *cmd[] = {"playlist-next", "force", NULL};
			gint64 playlist_pos = 0;
			gint64 playlist_count = 0;
			gchar *media_title = NULL;
			mpv_node metadata;

			gmpv_mpv_get_property(	mpv,
						"duration",
						MPV_FORMAT_DOUBLE,
						&entry->duration );
			gmpv_mpv_get_property(	mpv,
						"playlist-pos",
						MPV_FORMAT_INT64,
						&playlist_pos );
			gmpv_mpv_get_property(	mpv,
						"playlist-count",
						MPV_FORMAT_INT64,
						&playlist_count );
			gmpv_mpv_get_property(	mpv,
						"media-title",
						MPV_FORMAT_STRING,
						&media_title );
			gmpv_mpv_get_property(	mpv,
						"metadata",
						MPV_FORMAT_NODE,
						&metadata );

			if(!entry->title)
			{
				entry->title = g_strdup(media_title);
			}

			metadata_to_ptr_array(metadata, entry->tags);
			gmpv_mpv_command(mpv, cmd);

			g_signal_emit_by_name(cache, "update", path);

			mpv_free(media_title);
			mpv_free_node_contents(&metadata);
		}

		mpv_free(path);
	}
	else if(event_id == MPV_EVENT_END_FILE)
	{
		mpv_event_end_file *event = event_data;

		if(event->reason == MPV_END_FILE_REASON_ERROR)
		{
			g_debug("Failed to fetch metadata");
		}
	}
}

static void shutdown_handler(GmpvMpv *mpv, gpointer data)
{
	GmpvMetadataCache *cache = data;

	g_clear_object(&cache->fetcher);

	if(!g_queue_is_empty(cache->fetch_queue))
	{
		g_source_clear(&cache->fetch_timeout_id);
		cache->fetch_timeout_id
			= g_idle_add((GSourceFunc)fetch_metadata, cache);
	}
}

static gboolean fetch_metadata(GmpvMetadataCache *cache)
{
	g_assert(!cache->fetcher);
	cache->fetcher = gmpv_mpv_new(0);

	g_signal_connect(	cache->fetcher,
				"mpv-event-notify",
				G_CALLBACK(mpv_event_notify),
				cache );
	g_signal_connect(	cache->fetcher,
				"shutdown",
				G_CALLBACK(shutdown_handler),
				cache );

	gmpv_mpv_set_option_string(cache->fetcher, "ao", "null");
	gmpv_mpv_set_option_string(cache->fetcher, "vo", "null");
	gmpv_mpv_set_option_string(cache->fetcher, "idle", "once");
	gmpv_mpv_set_option_string(cache->fetcher, "ytdl", "yes");
	gmpv_mpv_initialize(cache->fetcher);

	for(	gchar *uri = g_queue_pop_tail(cache->fetch_queue);
		uri;
		uri = g_queue_pop_tail(cache->fetch_queue) )
	{
		g_debug("Queuing %s for metadata fetch", uri);
		gmpv_mpv_load_file(cache->fetcher, uri, TRUE);
		g_free(uri);
	}

	cache->fetch_timeout_id = 0;

	return G_SOURCE_REMOVE;
}

static void gmpv_metadata_cache_class_init(GmpvMetadataCacheClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->dispose = dispose;
	object_class->finalize = finalize;

	g_signal_new(	"update",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__STRING,
			G_TYPE_NONE,
			1,
			G_TYPE_STRING );
}

static void gmpv_metadata_cache_init(GmpvMetadataCache *cache)
{
	cache->table = g_hash_table_new_full(	g_str_hash,
						g_str_equal,
						g_free,
						(GDestroyNotify)
						gmpv_metadata_cache_entry_free );
	cache->fetcher = NULL;
	cache->fetch_queue = g_queue_new();
	cache->fetch_timeout_id = 0;
}

GmpvMetadataCache *gmpv_metadata_cache_new(void)
{
	return g_object_new(gmpv_metadata_cache_get_type(), NULL);
}

void gmpv_metadata_cache_ref_entry(GmpvMetadataCache *cache, const gchar *uri)
{
	gmpv_metadata_cache_lookup(cache, uri)->references++;
}

void gmpv_metadata_cache_unref_entry(GmpvMetadataCache *cache, const gchar *uri)
{
	GmpvMetadataCacheEntry *entry = g_hash_table_lookup(cache->table, uri);

	if(entry && --entry->references == 0)
	{
		g_hash_table_remove(cache->table, uri);
	}
}

void gmpv_metadata_cache_load_playlist(	GmpvMetadataCache *cache,
					const GPtrArray *playlist )
{
	GmpvMetadataCacheEntry *entry = NULL;
	GHashTableIter iter;

	g_hash_table_iter_init(&iter, cache->table);

	/* First, set the refrence count for all entries to zero */
	while(g_hash_table_iter_next(&iter, NULL, (gpointer)&entry))
	{
		g_assert(entry);
		entry->references = 0;
	}

	/* Then ref all entries in the playlist. This sets the reference count
	 * to the number of times the entry appears in the playlist.
	 */
	for(guint i = 0; i < playlist->len; i++)
	{
		GmpvPlaylistEntry *entry = g_ptr_array_index(playlist, i);
		gmpv_metadata_cache_ref_entry(cache, entry->filename);
	}

	g_hash_table_iter_init(&iter, cache->table);

	/* Remove all entries that with refrence count of zero, which means that
	 * the entry no longer exists in the playlist.
	 */
	while(g_hash_table_iter_next(&iter, NULL, (gpointer)&entry))
	{
		g_assert(entry);

		if(entry->references == 0)
		{
			g_hash_table_iter_remove(&iter);
		}
	}
}

GmpvMetadataCacheEntry *gmpv_metadata_cache_lookup(	GmpvMetadataCache *cache,
							const gchar *uri )
{
	GmpvMetadataCacheEntry *entry = g_hash_table_lookup(cache->table, uri);

	if(!entry)
	{
		entry = gmpv_metadata_cache_entry_new();

		g_hash_table_insert(cache->table, g_strdup(uri), entry);

		if(!cache->fetcher && g_queue_is_empty(cache->fetch_queue))
		{
			g_source_clear(&cache->fetch_timeout_id);
			cache->fetch_timeout_id
				= g_idle_add((GSourceFunc)fetch_metadata, cache);
		}

		g_queue_push_head(cache->fetch_queue, g_strdup(uri));
	}

	return entry;
}
