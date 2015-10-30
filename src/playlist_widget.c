/*
 * Copyright (c) 2014-2015 gnome-mpv
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

#include "playlist_widget.h"

G_DEFINE_TYPE(PlaylistWidget, playlist_widget, GTK_TYPE_SCROLLED_WINDOW)

static gboolean mouse_press_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data )
{
	GdkEventButton *btn_event = (GdkEventButton *)event;
	gboolean handled;

	handled = (	btn_event->type == GDK_BUTTON_PRESS &&
			btn_event->button == 3 );

	if(handled)
	{
		GMenu *menu;
		GMenuItem *add_menu_item;
		GMenuItem *loop_menu_item;
		GtkWidget *ctx_menu;

		menu = g_menu_new();
		add_menu_item = g_menu_item_new(_("_Add..."), "app.open(true)");
		loop_menu_item = g_menu_item_new(_("Loop"), "app.loop");

		g_menu_append_item(menu, add_menu_item);
		g_menu_append_item(menu, loop_menu_item);
		g_menu_freeze(menu);

		ctx_menu = gtk_menu_new_from_model(G_MENU_MODEL(menu));

		gtk_menu_attach_to_widget(GTK_MENU(ctx_menu), widget, NULL);
		gtk_widget_show_all(ctx_menu);

		gtk_menu_popup(	GTK_MENU(ctx_menu), NULL, NULL, NULL, NULL,
				btn_event->button, btn_event->time );
	}

	return handled;
}

static void playlist_widget_class_init(PlaylistWidgetClass *klass)
{
}

static void playlist_widget_init(PlaylistWidget *wgt)
{
	wgt->indicator_renderer = gtk_cell_renderer_text_new();
	wgt->title_renderer = gtk_cell_renderer_text_new();

	wgt->list_store = gtk_list_store_new(	3,
						G_TYPE_STRING,
						G_TYPE_STRING,
						G_TYPE_STRING );

	wgt->tree_view
		= gtk_tree_view_new_with_model(GTK_TREE_MODEL(wgt->list_store));

	wgt->indicator_column
		= gtk_tree_view_column_new_with_attributes
			(	"\xe2\x80\xa2", /* UTF-8 bullet */
				wgt->indicator_renderer,
				"text",
				0,
				NULL );

	wgt->title_column
		= gtk_tree_view_column_new_with_attributes
			(_("Playlist"), wgt->title_renderer, "text", 1, NULL);

	g_signal_connect(	wgt->tree_view,
				"button-press-event",
				G_CALLBACK(mouse_press_handler),
				NULL );

	gtk_widget_set_can_focus(GTK_WIDGET(wgt->tree_view), FALSE);
	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(wgt->tree_view), TRUE);

	gtk_tree_view_append_column
		(GTK_TREE_VIEW(wgt->tree_view), wgt->indicator_column);

	gtk_tree_view_append_column
		(GTK_TREE_VIEW(wgt->tree_view), wgt->title_column);

	gtk_container_add(GTK_CONTAINER(wgt), wgt->tree_view);
}

GtkWidget *playlist_widget_new()
{
	return GTK_WIDGET(g_object_new(playlist_widget_get_type(), NULL));
}

void playlist_widget_append(	PlaylistWidget *wgt,
				const gchar *name,
				const gchar *uri )
{
	GtkTreeIter iter;

	gtk_list_store_append(wgt->list_store, &iter);

	gtk_list_store_set
		(wgt->list_store, &iter, PLAYLIST_NAME_COLUMN, name, -1);

	gtk_list_store_set
		(wgt->list_store, &iter, PLAYLIST_URI_COLUMN, uri, -1);

	gtk_tree_view_columns_autosize(GTK_TREE_VIEW(wgt->tree_view));
}

void playlist_widget_remove(PlaylistWidget *wgt, gint pos)
{
	GtkTreeIter iter;
	gboolean rc;

	rc = gtk_tree_model_get_iter_first
		(GTK_TREE_MODEL(wgt->list_store), &iter);

	while(rc && --pos >= 0)
	{
		rc = gtk_tree_model_iter_next
			(GTK_TREE_MODEL(wgt->list_store), &iter);
	}

	if(rc)
	{
		gtk_list_store_remove(wgt->list_store, &iter);
		gtk_tree_view_columns_autosize(GTK_TREE_VIEW(wgt->tree_view));
	}
}

void playlist_widget_clear(PlaylistWidget *wgt)
{
	gtk_list_store_clear(wgt->list_store);
}

gboolean playlist_widget_empty(PlaylistWidget *wgt)
{
	GtkTreeIter iter;
	int rc;

	rc = gtk_tree_model_get_iter_first
		(GTK_TREE_MODEL(wgt->list_store), &iter);

	return !rc;
}

void playlist_widget_set_indicator_pos(PlaylistWidget *wgt, gint pos)
{
	GtkTreeIter iter;
	gboolean rc;

	rc = gtk_tree_model_get_iter_first
		(GTK_TREE_MODEL(wgt->list_store), &iter);

	while(rc)
	{
		if(pos-- == 0)
		{
			/* Put UTF-8 'right-pointing triangle' at requested row
			 */
			gtk_list_store_set
				(wgt->list_store, &iter, 0, "\xe2\x96\xb6", -1);
		}
		else
		{
			/* Clear other rows */
			gtk_list_store_set(wgt->list_store, &iter, 0, "", -1);
		}

		rc = gtk_tree_model_iter_next
			(GTK_TREE_MODEL(wgt->list_store), &iter);
	}
}
