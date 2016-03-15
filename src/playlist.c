/*
 * Copyright (c) 2014-2016 gnome-mpv
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

#include "playlist.h"
#include "marshal.h"
#include "common.h"
#include "mpv_obj.h"
#include "control_box.h"
#include "playlist_widget.h"

struct _Playlist
{
	GObject parent;
	GtkListStore *store;
	gboolean move_dest;
};

struct _PlaylistClass
{
	GObjectClass parent_class;
};

static void row_inserted_handler(	GtkTreeModel *tree_model,
					GtkTreePath *path,
					GtkTreeIter *iter,
					gpointer data );
static void row_deleted_handler(	GtkTreeModel *tree_model,
					GtkTreePath *path,
					gpointer data );

G_DEFINE_TYPE(Playlist, playlist, G_TYPE_OBJECT)

static void row_inserted_handler(	GtkTreeModel *tree_model,
					GtkTreePath *path,
					GtkTreeIter *iter,
					gpointer data )
{
	Playlist *pl = data;
	const gint dest = gtk_tree_path_get_indices(path)[0];

	pl->move_dest = dest;

	g_signal_emit_by_name(pl, "row-inserted", dest);
}

static void row_deleted_handler(	GtkTreeModel *tree_model,
					GtkTreePath *path,
					gpointer data )
{
	Playlist *pl = data;
	const gint src = gtk_tree_path_get_indices(path)[0];
	const gint dest = pl->move_dest;

	if(dest >= 0)
	{
		g_signal_emit_by_name(pl, "row-reordered", src, dest);

		pl->move_dest = -1;
	}
	else
	{
		g_signal_emit_by_name(pl, "row-deleted", src);
	}
}

static void playlist_class_init(PlaylistClass *klass)
{
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

static void playlist_init(Playlist *pl)
{
	pl->store = gtk_list_store_new(	3,
					G_TYPE_STRING,
					G_TYPE_STRING,
					G_TYPE_INT );

	pl->move_dest = -1;

	g_signal_connect(	pl->store,
				"row-inserted",
				G_CALLBACK(row_inserted_handler),
				pl );

	g_signal_connect(	pl->store,
				"row-deleted",
				G_CALLBACK(row_deleted_handler),
				pl );
}

Playlist *playlist_new()
{
	return PLAYLIST(g_object_new(playlist_get_type(), NULL));
}

GtkListStore *playlist_get_store(Playlist *pl)
{
	return pl->store;
}

void playlist_append(Playlist *pl, const gchar *name, const gchar *uri)
{
	GtkTreeIter iter;

	gtk_list_store_append(pl->store, &iter);
	gtk_list_store_set(pl->store, &iter, PLAYLIST_NAME_COLUMN, name, -1);
	gtk_list_store_set(pl->store, &iter, PLAYLIST_URI_COLUMN, uri, -1);

	pl->move_dest = -1;
}

void playlist_remove(Playlist *pl, gint pos)
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

void playlist_clear(Playlist *pl)
{
	gtk_list_store_clear(pl->store);
}

gboolean playlist_empty(Playlist *pl)
{
	GtkTreeIter iter;

	return !gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pl->store), &iter);
}

void playlist_set_indicator_pos(Playlist *pl, gint pos)
{
	GtkTreeIter iter;
	gboolean rc;

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

void playlist_reset(Playlist *pl)
{
	playlist_set_indicator_pos(pl, 0);
}
