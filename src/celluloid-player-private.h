/*
 * Copyright (c) 2019 gnome-mpv
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

#ifndef PLAYER_PRIVATE_H
#define PLAYER_PRIVATE_H

#include <gtk/gtk.h>

#include "celluloid-metadata-cache.h"

G_BEGIN_DECLS

typedef struct _CelluloidPlayerPrivate CelluloidPlayerPrivate;

enum
{
	PROP_0,
	PROP_PLAYLIST,
	PROP_METADATA,
	PROP_TRACK_LIST,
	PROP_DISC_LIST,
	PROP_EXTRA_OPTIONS,
	N_PROPERTIES
};

struct _CelluloidPlayerPrivate
{
	CelluloidMpv parent;
	CelluloidMetadataCache *cache;
	GVolumeMonitor *monitor;
	GPtrArray *playlist;
	GPtrArray *metadata;
	GPtrArray *track_list;
	GPtrArray *disc_list;
	GHashTable *log_levels;
	gboolean loaded;
	gboolean new_file;
	gboolean init_vo_config;
	gchar *tmp_input_config;
	gchar *extra_options;
};

#define get_private(player) \
	G_TYPE_INSTANCE_GET_PRIVATE(player, CELLULOID_TYPE_PLAYER, CelluloidPlayerPrivate)

G_END_DECLS

#endif

