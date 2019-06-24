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

#include "celluloid-metadata-cache.h"
#include "celluloid-mpv.h"
#include "celluloid-mpv-wrapper.h"

struct _CelluloidMetadataCache
{
	GObject parent;
	GHashTable *table;
	CelluloidMpv *fetcher;
	GQueue *fetch_queue;
	guint fetch_timeout_id;
};

struct _CelluloidMetadataCacheClass
{
	GObjectClass parent_class;
};

static CelluloidMetadataCacheEntry *
celluloid_metadata_cache_entry_new(void);

static void
celluloid_metadata_cache_entry_free(CelluloidMetadataCacheEntry *entry);

static void
dispose(GObject *object);

static void
finalize(GObject *object);

static void
metadata_to_ptr_array(mpv_node metadata, GPtrArray *array);

static void
mpv_event_notify(	CelluloidMpv *mpv,
			gint event_id,
			gpointer event_data,
			gpointer data );

static gboolean
fetch_metadata(CelluloidMetadataCache *cache);

G_DEFINE_TYPE(CelluloidMetadataCache, celluloid_metadata_cache, G_TYPE_OBJECT)

static CelluloidMetadataCacheEntry *
celluloid_metadata_cache_entry_new(void)
{
	CelluloidMetadataCacheEntry *entry =
		g_new0(CelluloidMetadataCacheEntry, 1);

	entry->tags =	g_ptr_array_new_with_free_func
			((GDestroyNotify)celluloid_metadata_entry_free);

	return entry;
}

static void
celluloid_metadata_cache_entry_free(CelluloidMetadataCacheEntry *entry)
{
	if(entry)
	{
		g_free(entry->title);
		g_ptr_array_free(entry->tags, TRUE);
		g_free(entry);
	}
}

static void
dispose(GObject *object)
{
	CelluloidMetadataCache *cache = CELLULOID_METADATA_CACHE(object);

	g_source_clear(&cache->fetch_timeout_id);
	g_clear_object(&CELLULOID_METADATA_CACHE(object)->fetcher);
}

static void
finalize(GObject *object)
{
	CelluloidMetadataCache *cache = CELLULOID_METADATA_CACHE(object);

	g_hash_table_unref(cache->table);
	g_queue_free_full(cache->fetch_queue, g_free);
}

static void
metadata_to_ptr_array(mpv_node metadata, GPtrArray *array)
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
				CelluloidMetadataEntry *entry;

				entry =	celluloid_metadata_entry_new
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

static void
mpv_event_notify(	CelluloidMpv *mpv,
			gint event_id,
			gpointer event_data,
			gpointer data )
{
	if(event_id == MPV_EVENT_FILE_LOADED)
	{
		CelluloidMetadataCache *cache = data;
		CelluloidMetadataCacheEntry *entry = NULL;
		gchar *path = NULL;

		celluloid_mpv_get_property
			(mpv, "path", MPV_FORMAT_STRING, &path);

		g_debug("Fetched metadata for %s", path);

		entry =	g_hash_table_lookup(cache->table, path);

		if(entry)
		{
			const gchar *cmd[] = {"playlist-next", "force", NULL};
			gchar *media_title = NULL;
			mpv_node metadata;

			celluloid_mpv_get_property(	mpv,
						"duration",
						MPV_FORMAT_DOUBLE,
						&entry->duration );
			celluloid_mpv_get_property(	mpv,
						"media-title",
						MPV_FORMAT_STRING,
						&media_title );
			celluloid_mpv_get_property(	mpv,
						"metadata",
						MPV_FORMAT_NODE,
						&metadata );

			if(!entry->title)
			{
				entry->title = g_strdup(media_title);
			}

			metadata_to_ptr_array(metadata, entry->tags);
			celluloid_mpv_command(mpv, cmd);

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

static void
shutdown_handler(CelluloidMpv *mpv, gpointer data)
{
	CelluloidMetadataCache *cache = data;

	g_clear_object(&cache->fetcher);

	if(!g_queue_is_empty(cache->fetch_queue))
	{
		g_source_clear(&cache->fetch_timeout_id);
		cache->fetch_timeout_id
			= g_idle_add((GSourceFunc)fetch_metadata, cache);
	}
}

static gboolean
fetch_metadata(CelluloidMetadataCache *cache)
{
	g_assert(!cache->fetcher);
	cache->fetcher = celluloid_mpv_new(0);

	g_signal_connect(	cache->fetcher,
				"mpv-event-notify",
				G_CALLBACK(mpv_event_notify),
				cache );
	g_signal_connect(	cache->fetcher,
				"shutdown",
				G_CALLBACK(shutdown_handler),
				cache );

	celluloid_mpv_set_option_string(cache->fetcher, "ao", "null");
	celluloid_mpv_set_option_string(cache->fetcher, "vo", "null");
	celluloid_mpv_set_option_string(cache->fetcher, "idle", "once");
	celluloid_mpv_set_option_string(cache->fetcher, "ytdl", "yes");
	celluloid_mpv_initialize(cache->fetcher);

	for(	gchar *uri = g_queue_pop_tail(cache->fetch_queue);
		uri;
		uri = g_queue_pop_tail(cache->fetch_queue) )
	{
		g_debug("Queuing %s for metadata fetch", uri);
		celluloid_mpv_load_file(cache->fetcher, uri, TRUE);
		g_free(uri);
	}

	cache->fetch_timeout_id = 0;

	return G_SOURCE_REMOVE;
}

static void
celluloid_metadata_cache_class_init(CelluloidMetadataCacheClass *klass)
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

static void
celluloid_metadata_cache_init(CelluloidMetadataCache *cache)
{
	cache->table =	g_hash_table_new_full
			(	g_str_hash,
				g_str_equal,
				g_free,
				(GDestroyNotify)
				celluloid_metadata_cache_entry_free );
	cache->fetcher = NULL;
	cache->fetch_queue = g_queue_new();
	cache->fetch_timeout_id = 0;
}

CelluloidMetadataCache *
celluloid_metadata_cache_new(void)
{
	return g_object_new(celluloid_metadata_cache_get_type(), NULL);
}

void
celluloid_metadata_cache_ref_entry(	CelluloidMetadataCache *cache,
					const gchar *uri )
{
	celluloid_metadata_cache_lookup(cache, uri)->references++;
}

void
celluloid_metadata_cache_unref_entry(	CelluloidMetadataCache *cache,
					const gchar *uri )
{
	CelluloidMetadataCacheEntry *entry =
		g_hash_table_lookup(cache->table, uri);

	if(entry && --entry->references == 0)
	{
		g_hash_table_remove(cache->table, uri);
	}
}

void
celluloid_metadata_cache_load_playlist(	CelluloidMetadataCache *cache,
					const GPtrArray *playlist )
{
	CelluloidMetadataCacheEntry *entry = NULL;
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
		CelluloidPlaylistEntry *entry = g_ptr_array_index(playlist, i);
		celluloid_metadata_cache_ref_entry(cache, entry->filename);
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

CelluloidMetadataCacheEntry *
celluloid_metadata_cache_lookup(	CelluloidMetadataCache *cache,
					const gchar *uri )
{
	CelluloidMetadataCacheEntry *entry =
		g_hash_table_lookup(cache->table, uri);

	if(!entry)
	{
		entry = celluloid_metadata_cache_entry_new();

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
