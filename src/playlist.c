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
#include "common.h"
#include "mpv.h"
#include "control_box.h"
#include "playlist_widget.h"

void playlist_append(	playlist *pl,
			const gchar *name,
			const gchar *uri )
{
	GtkTreeIter iter;

	gtk_list_store_append(pl, &iter);
	gtk_list_store_set(pl, &iter, PLAYLIST_NAME_COLUMN, name, -1);
	gtk_list_store_set(pl, &iter, PLAYLIST_URI_COLUMN, uri, -1);
}

void playlist_remove(playlist *pl, gint pos)
{
	GtkTreeIter iter;
	gboolean rc;

	rc = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pl), &iter);

	while(rc && --pos >= 0)
	{
		rc = gtk_tree_model_iter_next
			(GTK_TREE_MODEL(pl), &iter);
	}

	if(rc)
	{
		gtk_list_store_remove(pl, &iter);
	}
}

void playlist_clear(playlist *pl)
{
	gtk_list_store_clear(pl);
}

gboolean playlist_empty(playlist *pl)
{
	GtkTreeIter iter;

	return !gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pl), &iter);
}

void playlist_set_indicator_pos(playlist *pl, gint pos)
{
	GtkTreeIter iter;
	gboolean rc;

	rc = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pl), &iter);

	/* Put UTF-8 'right-pointing triangle' at requested row and clear other
	 * rows.
	 */
	while(rc)
	{
		gchar const *const str = (pos-- == 0)?"\xe2\x96\xb6":"";

		gtk_list_store_set(pl, &iter, 0, str, -1);

		rc = gtk_tree_model_iter_next(GTK_TREE_MODEL(pl), &iter);
	}
}

void playlist_remove_current_entry(gmpv_handle *ctx)
{
	const gchar *cmd[] = {"playlist_remove", NULL, NULL};
	PlaylistWidget *playlist;
	GtkTreePath *path;

	playlist = PLAYLIST_WIDGET(ctx->gui->playlist);

	gtk_tree_view_get_cursor
		(	GTK_TREE_VIEW(playlist->tree_view),
			&path,
			NULL );

	if(path)
	{
		gint index;
		gchar *index_str;

		index = gtk_tree_path_get_indices(path)[0];
		index_str = g_strdup_printf("%d", index);
		cmd[1] = index_str;

		g_signal_handlers_block_matched
			(	playlist->list_store,
				G_SIGNAL_MATCH_DATA,
				0,
				0,
				NULL,
				NULL,
				ctx );

		playlist_remove(playlist->list_store, index);

		if(ctx->loaded)
		{
			mpv_check_error(mpv_command(ctx->mpv_ctx, cmd));
		}

		if(playlist_empty(playlist->list_store))
		{
			control_box_set_enabled
				(CONTROL_BOX(ctx->gui->control_box), FALSE);
		}

		g_signal_handlers_unblock_matched
			(	playlist->list_store,
				G_SIGNAL_MATCH_DATA,
				0,
				0,
				NULL,
				NULL,
				ctx );

		g_free(index_str);
	}
}

void playlist_reset(playlist *pl)
{
	playlist_set_indicator_pos(pl, 0);
}
