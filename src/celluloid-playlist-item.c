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

#include "celluloid-playlist-item.h"

struct _CelluloidPlaylistItem
{
	GObject parent_instance;
	gchar *title;
	gchar *uri;
	gdouble duration;
	gboolean is_current;
};

struct _CelluloidPlaylistItemClass
{
	GObjectClass parent_class;
};

G_DEFINE_TYPE(CelluloidPlaylistItem, celluloid_playlist_item, G_TYPE_OBJECT)

static void
finalize(GObject *gobject)
{
	CelluloidPlaylistItem *self = CELLULOID_PLAYLIST_ITEM(gobject);

	g_free(self->title);
	g_free(self->uri);
}

static void
celluloid_playlist_item_init(CelluloidPlaylistItem *self)
{
	self->title = NULL;
	self->uri = NULL;
	self->duration = 0.0;
	self->is_current = FALSE;
}

static void
celluloid_playlist_item_class_init(CelluloidPlaylistItemClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->finalize = finalize;
}

CelluloidPlaylistItem *
celluloid_playlist_item_new_take(	gchar *title,
					gchar *uri,
					gdouble duration,
					gboolean is_current )
{
	CelluloidPlaylistItem *self;

	self = g_object_new(CELLULOID_TYPE_PLAYLIST_ITEM, NULL);
	self->title = title;
	self->uri = uri;
	self->duration = duration;
	self->is_current = is_current;

	return self;
}

CelluloidPlaylistItem *
celluloid_playlist_item_new(	const gchar *title,
				const gchar *uri,
				gdouble duration,
				gboolean is_current )
{
	return	celluloid_playlist_item_new_take
		(g_strdup(title), g_strdup(uri), duration, is_current);
}

CelluloidPlaylistItem *
celluloid_playlist_item_copy(CelluloidPlaylistItem *source)
{
	CelluloidPlaylistItem *self;

	self = g_object_new(CELLULOID_TYPE_PLAYLIST_ITEM, NULL);
	self->title = source->title;
	self->uri = source->uri;
	self->duration = source->duration;
	self->is_current = source->is_current;

	return self;
}

const gchar *
celluloid_playlist_item_get_title(CelluloidPlaylistItem *self)
{
	return self->title;
}

const gchar *
celluloid_playlist_item_get_uri(CelluloidPlaylistItem *self)
{
	return self->uri;
}

gdouble
celluloid_playlist_item_get_duration(CelluloidPlaylistItem *self)
{
	return self->duration;
}

gboolean
celluloid_playlist_item_get_is_current(CelluloidPlaylistItem *self)
{
	return self->is_current;
}

void
celluloid_playlist_item_set_is_current(	CelluloidPlaylistItem *self,
					gboolean value )
{
	self->is_current = value;
}

