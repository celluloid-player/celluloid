/*
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

#include "playlist_widget.h"

static void playlist_widget_init(PlaylistWidget *wgt);

static void playlist_widget_init(PlaylistWidget *wgt)
{
	wgt->list_store = gtk_list_store_new(1, G_TYPE_STRING);
	wgt->cell_renderer = gtk_cell_renderer_text_new();

	wgt->tree_view
		= gtk_tree_view_new_with_model(GTK_TREE_MODEL(wgt->list_store));

	wgt->tree_column
		= gtk_tree_view_column_new_with_attributes
			("Playlist", wgt->cell_renderer, "text", 0, NULL);

	gtk_container_add(GTK_CONTAINER(wgt), wgt->tree_view);

	gtk_tree_view_append_column
		(GTK_TREE_VIEW(wgt->tree_view), wgt->tree_column);

	gtk_tree_view_set_activate_on_single_click
		(GTK_TREE_VIEW(wgt->tree_view), TRUE);
}

GtkWidget *playlist_widget_new()
{
	return GTK_WIDGET(g_object_new(playlist_widget_get_type(), NULL));
}

GType playlist_widget_get_type()
{
	static GType wgt_type = 0;

	if(wgt_type == 0)
	{
		const GTypeInfo wgt_info
			= {	sizeof(PlaylistWidgetClass),
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				sizeof(PlaylistWidget),
				0,
				(GInstanceInitFunc)playlist_widget_init };

		wgt_type = g_type_register_static
				(	GTK_TYPE_SCROLLED_WINDOW,
					"PlaylistWidget",
					&wgt_info,
					0 );
	}

	return wgt_type;
}

void playlist_widget_append(PlaylistWidget *wgt, const gchar *entry)
{
	gtk_list_store_append(wgt->list_store, &wgt->tree_iter);
	gtk_list_store_set(wgt->list_store, &wgt->tree_iter, 0, entry, -1);
}

void playlist_widget_clear(PlaylistWidget *wgt)
{
	gtk_list_store_clear(wgt->list_store);
}

void playlist_widget_set_cursor_pos(PlaylistWidget *wgt, gint pos)
{
	GtkTreePath *path = gtk_tree_path_new_from_indices(pos, -1);

	gtk_tree_view_set_cursor(	GTK_TREE_VIEW(wgt->tree_view),
					path,
					NULL,
					FALSE );

	gtk_tree_path_free(path);
}
