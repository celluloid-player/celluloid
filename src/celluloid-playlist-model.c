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
#include "celluloid-marshal.h"

struct _CelluloidPlaylistModel
{
	GObject parent_instance;
	gint current;
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

	return g_object_ref(g_ptr_array_index(self->items, position));
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
contents_changed(	CelluloidPlaylistModel *self,
			guint position,
			guint removed,
			guint added )
{
	g_signal_emit_by_name
		(self, "contents-changed", position, removed, added);
	g_list_model_items_changed
		(G_LIST_MODEL(self), position, removed, added);
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
	// This is basically the same as items-changed, except that it doesn't
	// fire when the current position changed.
	g_signal_new(	"contents-changed",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_gen_marshal_VOID__UINT_UINT_UINT,
			G_TYPE_NONE,
			3,
			G_TYPE_UINT,
			G_TYPE_UINT,
			G_TYPE_UINT );
}

static void
celluloid_playlist_model_init(CelluloidPlaylistModel *self)
{
	self->current = -1;
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
	g_object_ref_sink(item);
	g_ptr_array_add(self->items, item);

	g_assert(self->items->len > 0);
	contents_changed(self, self->items->len - 1, 0, 1);
}

void
celluloid_playlist_model_remove(CelluloidPlaylistModel *self, guint position)
{
	g_ptr_array_remove_index(self->items, position);
	contents_changed(self, position, 1, 0);
}

void
celluloid_playlist_model_clear(CelluloidPlaylistModel *self)
{
	const guint len = self->items->len;

	self->current = -1;

	g_ptr_array_set_size(self->items, 0);
	contents_changed(self, 0, len, 0);
}

gint
celluloid_playlist_model_get_current(CelluloidPlaylistModel *self)
{
	return self->current;
}

void
celluloid_playlist_model_set_current(	CelluloidPlaylistModel *self,
					gint position)
{
	position = MIN((gint)self->items->len - 1, position);

	// Unset the is_current flag at the old position
	if(self->current >= 0)
	{
		CelluloidPlaylistItem *item;

		item = get_item(G_LIST_MODEL(self), (guint)self->current);
		celluloid_playlist_item_set_is_current(item, FALSE);

		g_list_model_items_changed
			(G_LIST_MODEL(self), (guint)self->current, 1, 1);

		g_object_unref(item);
	}

	// Set the is_current flag at the new position
	if(position >= 0)
	{
		CelluloidPlaylistItem *item;

		item = get_item(G_LIST_MODEL(self), (guint)position);
		celluloid_playlist_item_set_is_current(item, TRUE);

		g_list_model_items_changed
			(G_LIST_MODEL(self), (guint)position, 1, 1);

		g_object_unref(item);
	}

	self->current = position;
}
