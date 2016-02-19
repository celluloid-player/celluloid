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

#include "playlist_widget.h"
#include "def.h"

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
		add_menu_item = g_menu_item_new(_("_Add…"), "app.open(true)");
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

	gtk_widget_set_size_request
		(GTK_WIDGET(wgt), PLAYLIST_MIN_WIDTH, -1);

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

