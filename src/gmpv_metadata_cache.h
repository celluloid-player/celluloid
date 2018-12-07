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

#ifndef METADATA_CACHE
#define METADATA_CACHE

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _GmpvMetadataCacheEntry GmpvMetadataCacheEntry;

struct _GmpvMetadataCacheEntry
{
	gint references;
	gchar *title;
	gdouble duration;
	GPtrArray *tags;
};

#define GMPV_TYPE_METADATA_CACHE (gmpv_metadata_cache_get_type())

G_DECLARE_FINAL_TYPE(GmpvMetadataCache, gmpv_metadata_cache, GMPV, METADATA_CACHE, GObject)

GmpvMetadataCache *gmpv_metadata_cache_new(void);
void gmpv_metadata_cache_ref_entry(GmpvMetadataCache *cache, const gchar *uri);
void gmpv_metadata_cache_unref_entry(GmpvMetadataCache *cache, const gchar *uri);
void gmpv_metadata_cache_load_playlist(	GmpvMetadataCache *cache,
					const GPtrArray *playlist );
GmpvMetadataCacheEntry *gmpv_metadata_cache_lookup(	GmpvMetadataCache *cache,
							const gchar *uri );

G_END_DECLS

#endif
