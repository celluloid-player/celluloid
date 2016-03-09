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
#include "playlist.h"
#include "def.h"

enum
{
	PROP_0,
	PROP_STORE,
	N_PROPERTIES
};

static gboolean mouse_press_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data );

G_DEFINE_TYPE(PlaylistWidget, playlist_widget, GTK_TYPE_SCROLLED_WINDOW)

static void playlist_widget_constructed(GObject *object)
{
	PlaylistWidget *self = PLAYLIST_WIDGET(object);

	self->tree_view
		= gtk_tree_view_new_with_model
			(GTK_TREE_MODEL(playlist_get_store(self->store)));

	g_signal_connect(	self->tree_view,
				"button-press-event",
				G_CALLBACK(mouse_press_handler),
				NULL );

	gtk_widget_set_can_focus(GTK_WIDGET(self->tree_view), FALSE);
	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(self->tree_view), TRUE);

	gtk_tree_view_append_column
		(GTK_TREE_VIEW(self->tree_view), self->indicator_column);

	gtk_tree_view_append_column
		(GTK_TREE_VIEW(self->tree_view), self->title_column);

	gtk_container_add(GTK_CONTAINER(self), self->tree_view);

	G_OBJECT_CLASS(playlist_widget_parent_class)->constructed(object);
}

static void playlist_widget_set_property(	GObject *object,
						guint property_id,
						const GValue *value,
						GParamSpec *pspec )
{
	PlaylistWidget *self = PLAYLIST_WIDGET(object);

	if(property_id == PROP_STORE)
	{
		self->store = g_value_get_pointer(value);

	}
	else
	{
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void playlist_widget_get_property(	GObject *object,
						guint property_id,
						GValue *value,
						GParamSpec *pspec )
{
	PlaylistWidget *self = PLAYLIST_WIDGET(object);

	if(property_id == PROP_STORE)
	{
		g_value_set_pointer(value, self->store);
	}
	else
	{
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

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
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec = NULL;

	obj_class->constructed = playlist_widget_constructed;
	obj_class->set_property = playlist_widget_set_property;
	obj_class->get_property = playlist_widget_get_property;

	pspec = g_param_spec_pointer
		(	"store",
			"Store",
			"Playlist object used to store playlist items",
			G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE );

	g_object_class_install_property(obj_class, PROP_STORE, pspec);
}

static void playlist_widget_init(PlaylistWidget *wgt)
{
	GtkTargetEntry targets[] = DND_TARGETS;

	wgt->indicator_renderer = gtk_cell_renderer_text_new();
	wgt->title_renderer = gtk_cell_renderer_text_new();

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

	gtk_drag_dest_set(	GTK_WIDGET(wgt),
				GTK_DEST_DEFAULT_ALL,
				targets,
				G_N_ELEMENTS(targets),
				GDK_ACTION_COPY );
	gtk_drag_dest_add_uri_targets(GTK_WIDGET(wgt));

	gtk_widget_set_size_request
		(GTK_WIDGET(wgt), PLAYLIST_MIN_WIDTH, -1);
	gtk_tree_view_column_set_sizing
		(wgt->title_column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
}

GtkWidget *playlist_widget_new(Playlist *store)
{
	return GTK_WIDGET(g_object_new(	playlist_widget_get_type(),
					"store", store,
					NULL ));
}

void playlist_widget_remove_selected(PlaylistWidget *wgt)
{
	GtkTreePath *path;

	gtk_tree_view_get_cursor
		(	GTK_TREE_VIEW(wgt->tree_view),
			&path,
			NULL );

	if(path)
	{
		gint index;
		gchar *index_str;

		index = gtk_tree_path_get_indices(path)[0];
		index_str = g_strdup_printf("%d", index);

		playlist_remove(wgt->store, index);

		g_free(index_str);
	}
}

