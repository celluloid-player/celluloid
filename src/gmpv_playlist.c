/*
 * Copyright (c) 2014-2017 gnome-mpv
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

#include <glib.h>
#include <glib-object.h>

#include "gmpv_playlist.h"
#include "gmpv_marshal.h"

enum
{
	PROP_0,
	PROP_ROW_COUNT,
	N_PROPERTIES
};

struct _GmpvPlaylist
{
	GObject parent;
	GtkListStore *store;
	gboolean move_dest;
	gint indicator_pos;
	gint row_count;
};

struct _GmpvPlaylistClass
{
	GObjectClass parent_class;
};

static void set_property(	GObject *object,
				guint property_id,
				const GValue *value,
				GParamSpec *pspec );
static void get_property(	GObject *object,
				guint property_id,
				GValue *value,
				GParamSpec *pspec );
static void row_inserted_handler(	GtkTreeModel *tree_model,
					GtkTreePath *path,
					GtkTreeIter *iter,
					gpointer data );
static void row_deleted_handler(	GtkTreeModel *tree_model,
					GtkTreePath *path,
					gpointer data );

G_DEFINE_TYPE(GmpvPlaylist, gmpv_playlist, G_TYPE_OBJECT)

static void set_property(	GObject *object,
				guint property_id,
				const GValue *value,
				GParamSpec *pspec )
{
	GmpvPlaylist *self = GMPV_PLAYLIST(object);

	if(property_id == PROP_ROW_COUNT)
	{
		self->row_count = g_value_get_int(value);
	}
	else
	{
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void get_property(	GObject *object,
				guint property_id,
				GValue *value,
				GParamSpec *pspec )
{
	GmpvPlaylist *self = GMPV_PLAYLIST(object);

	if(property_id == PROP_ROW_COUNT)
	{
		g_value_set_int(value, self->row_count);
	}
	else
	{
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void row_inserted_handler(	GtkTreeModel *tree_model,
					GtkTreePath *path,
					GtkTreeIter *iter,
					gpointer data )
{
	GmpvPlaylist *pl = data;
	const gint dest = gtk_tree_path_get_indices(path)[0];

	pl->move_dest = dest;

	g_object_set(pl, "row-count", pl->row_count+1, NULL);
	g_signal_emit_by_name(pl, "row-inserted", dest);
}

static void row_deleted_handler(	GtkTreeModel *tree_model,
					GtkTreePath *path,
					gpointer data )
{
	GmpvPlaylist *pl = data;
	const gint src = gtk_tree_path_get_indices(path)[0];
	const gint dest = pl->move_dest;

	gmpv_playlist_set_indicator_pos(pl, pl->indicator_pos);

	if(dest >= 0)
	{
		g_signal_emit_by_name(pl, "row-reordered", src, dest);

		pl->move_dest = -1;
	}
	else
	{
		g_object_set(pl, "row-count", pl->row_count-1, NULL);
		g_signal_emit_by_name(pl, "row-deleted", src);
	}
}

static void gmpv_playlist_class_init(GmpvPlaylistClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec = NULL;

	object_class->set_property = set_property;
	object_class->get_property = get_property;

	pspec = g_param_spec_int
		(	"row-count",
			"Row count",
			"The number of rows",
			0,
			G_MAXINT,
			0,
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_ROW_COUNT, pspec);

	g_signal_new(	"row-inserted",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__INT,
			G_TYPE_NONE,
			1,
			G_TYPE_INT );
	g_signal_new(	"row-deleted",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__INT,
			G_TYPE_NONE,
			1,
			G_TYPE_INT );
	g_signal_new(	"row-reordered",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_gen_marshal_VOID__INT_INT,
			G_TYPE_NONE,
			2,
			G_TYPE_INT,
			G_TYPE_INT );
}

static void gmpv_playlist_init(GmpvPlaylist *pl)
{
	pl->store = gtk_list_store_new(	3,
					G_TYPE_STRING,
					G_TYPE_STRING,
					G_TYPE_INT );
	pl->move_dest = -1;
	pl->row_count = 0;

	g_signal_connect_after(	pl->store,
				"row-inserted",
				G_CALLBACK(row_inserted_handler),
				pl );
	g_signal_connect_after(	pl->store,
				"row-deleted",
				G_CALLBACK(row_deleted_handler),
				pl );
}

GmpvPlaylist *gmpv_playlist_new()
{
	return GMPV_PLAYLIST(g_object_new(gmpv_playlist_get_type(), NULL));
}

GtkListStore *gmpv_playlist_get_store(GmpvPlaylist *pl)
{
	return pl->store;
}

void gmpv_playlist_append(GmpvPlaylist *pl, const gchar *name, const gchar *uri)
{
	GtkTreeIter iter;

	gtk_list_store_append(pl->store, &iter);
	gtk_list_store_set(pl->store, &iter, PLAYLIST_NAME_COLUMN, name, -1);
	gtk_list_store_set(pl->store, &iter, PLAYLIST_URI_COLUMN, uri, -1);

	pl->move_dest = -1;
}

void gmpv_playlist_remove(GmpvPlaylist *pl, gint pos)
{
	GtkTreeIter iter;
	gboolean rc;

	rc = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pl->store), &iter);

	while(rc && --pos >= 0)
	{
		rc = gtk_tree_model_iter_next
			(GTK_TREE_MODEL(pl->store), &iter);
	}

	if(rc)
	{
		gtk_list_store_remove(pl->store, &iter);
	}
}

void gmpv_playlist_clear(GmpvPlaylist *pl)
{
	gtk_list_store_clear(pl->store);
}

gboolean gmpv_playlist_empty(GmpvPlaylist *pl)
{
	GtkTreeIter iter;

	return !gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pl->store), &iter);
}

void gmpv_playlist_set_indicator_pos(GmpvPlaylist *pl, gint pos)
{
	GtkTreeIter iter;
	gboolean rc;

	pl->indicator_pos = pos;
	rc = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pl->store), &iter);

	while(rc)
	{
		const PangoWeight weight =	(pos-- == 0)?
						PANGO_WEIGHT_BOLD:
						PANGO_WEIGHT_NORMAL;

		gtk_list_store_set(	pl->store,
					&iter,
					PLAYLIST_WEIGHT_COLUMN, weight,
					-1 );

		rc = gtk_tree_model_iter_next(GTK_TREE_MODEL(pl->store), &iter);
	}
}

void gmpv_playlist_reset(GmpvPlaylist *pl)
{
	gmpv_playlist_set_indicator_pos(pl, 0);
}
