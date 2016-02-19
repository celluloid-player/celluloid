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

#ifndef PLAYLIST_H
#define PLAYLIST_H

#include "common.h"

#include <gtk/gtk.h>

void playlist_previous_handler(GtkWidget *widget, gpointer data);
void playlist_next_handler(GtkWidget *widget, gpointer data);
void playlist_row_handler(	GtkTreeView *tree_view,
 				GtkTreePath *path,
 				GtkTreeViewColumn *column,
 				gpointer data );
void playlist_row_inserted_handler(	GtkTreeModel *tree_model,
 					GtkTreePath *path,
 					GtkTreeIter *iter,
 					gpointer data );
void playlist_row_deleted_handler(	GtkTreeModel *tree_model,
 					GtkTreePath *path,
 					gpointer data );
void playlist_remove_current_entry(gmpv_handle *ctx);
void playlist_reset(gmpv_handle *ctx);

#endif
