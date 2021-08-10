/*
 * Copyright (c) 2021 gnome-mpv
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

#include "celluloid-playlist-model.h"

struct _CelluloidPlaylistModel
{
	GObject parent_instance;
	GPtrArray *items;
};

struct _CelluloidPlaylistModelClass
{
	GObjectClass parent_class;
};

static void
celluloid_playlist_model_list_model_init(GListModelInterface *iface);

static void *
get_item(GListModel *list, guint position);

static GType
get_item_type(GListModel* list);

guint
get_n_items(GListModel* list);

G_DEFINE_TYPE_WITH_CODE
(	CelluloidPlaylistModel,
	celluloid_playlist_model,
	G_TYPE_OBJECT,
	G_IMPLEMENT_INTERFACE
	(	G_TYPE_LIST_MODEL,
		celluloid_playlist_model_list_model_init ))

static void *
get_item(GListModel *list, guint position)
{
	CelluloidPlaylistModel *self = CELLULOID_PLAYLIST_MODEL(list);

	return g_ptr_array_index(self->items, position);
}

static GType
get_item_type(GListModel* list)
{
	return G_TYPE_OBJECT;
}

guint
get_n_items(GListModel* list)
{
	CelluloidPlaylistModel *self = CELLULOID_PLAYLIST_MODEL(list);

	return self->items->len;
}

static void
celluloid_playlist_model_list_model_init(GListModelInterface *iface)
{
	iface->get_item = get_item;
	iface->get_item_type = get_item_type;
	iface->get_n_items = get_n_items;
}

static void
celluloid_playlist_model_class_init(CelluloidPlaylistModelClass *klass)
{
}

static void
celluloid_playlist_model_init(CelluloidPlaylistModel *self)
{
	self->items = g_ptr_array_new_with_free_func(g_object_unref);
}

CelluloidPlaylistModel *
celluloid_playlist_model_new(void)
{
	return g_object_new(CELLULOID_TYPE_PLAYLIST_MODEL, NULL);
}

void
celluloid_playlist_model_append(	CelluloidPlaylistModel *self,
					CelluloidPlaylistItem *item )
{
	g_ptr_array_add(self->items, item);
}

void
celluloid_playlist_model_remove(CelluloidPlaylistModel *self, guint position)
{
	g_ptr_array_remove_index(self->items, position);
}

void
celluloid_playlist_model_clear(CelluloidPlaylistModel *self)
{
	g_ptr_array_set_size(self->items, 0);
}
