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
#include "marshal.h"
#include "def.h"

enum
{
	PROP_0,
	PROP_STORE,
	N_PROPERTIES
};

struct _PlaylistWidget
{
	GtkScrolledWindow parent_instance;
	GtkWidget *tree_view;
	Playlist *store;
	GtkTreeViewColumn *title_column;
	GtkCellRenderer *title_renderer;
};

struct _PlaylistWidgetClass
{
	GtkScrolledWindowClass parent_class;
};

static void row_activated_handler(	GtkTreeView *tree_view,
					GtkTreePath *path,
					GtkTreeViewColumn *column,
					gpointer data );
static void row_deleted_handler(Playlist *pl, gint pos, gpointer data);
static void row_reodered_handler(	Playlist *pl,
					gint src,
					gint dest,
					gpointer data );
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
	g_signal_connect(	self->tree_view,
				"row-activated",
				G_CALLBACK(row_activated_handler),
				self );
	g_signal_connect(	self->store,
				"row-deleted",
				G_CALLBACK(row_deleted_handler),
				self );
	g_signal_connect(	self->store,
				"row-reordered",
				G_CALLBACK(row_reodered_handler),
				self );

	gtk_widget_set_can_focus(GTK_WIDGET(self->tree_view), FALSE);
	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(self->tree_view), TRUE);

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

static void row_activated_handler(	GtkTreeView *tree_view,
					GtkTreePath *path,
					GtkTreeViewColumn *column,
					gpointer data )
{
	gint *indices = gtk_tree_path_get_indices(path);
	gint64 index = indices?indices[0]:-1;

	printf("FOO: %ld\n", index);

	g_signal_emit_by_name(data, "row-activated", index);
}

static void row_deleted_handler(Playlist *pl, gint pos, gpointer data)
{
	g_signal_emit_by_name(data, "row-deleted", pos);
}

static void row_reodered_handler(	Playlist *pl,
					gint src,
					gint dest,
					gpointer data )
{
	g_signal_emit_by_name(data, "row-reordered", src, dest);
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

	g_signal_new(	"row-activated",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_gen_marshal_VOID__INT64,
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

static void playlist_widget_init(PlaylistWidget *wgt)
{
	GtkTargetEntry targets[] = DND_TARGETS;

	wgt->title_renderer = gtk_cell_renderer_text_new();
	wgt->title_column
		= gtk_tree_view_column_new_with_attributes
			(	_("Playlist"),
				wgt->title_renderer,
				"text", PLAYLIST_NAME_COLUMN,
				"weight", PLAYLIST_WEIGHT_COLUMN,
				NULL );

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

Playlist *playlist_widget_get_store(PlaylistWidget *wgt)
{
	return wgt->store;
}
