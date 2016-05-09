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

#include "gmpv_playlist_widget.h"
#include "gmpv_playlist.h"
#include "gmpv_marshal.h"
#include "gmpv_def.h"

enum
{
	PROP_0,
	PROP_STORE,
	N_PROPERTIES
};

struct _GmpvPlaylistWidget
{
	GtkScrolledWindow parent_instance;
	GtkWidget *tree_view;
	GmpvPlaylist *store;
	GtkTreeViewColumn *title_column;
	GtkCellRenderer *title_renderer;
	gboolean dnd_delete;
};

struct _GmpvPlaylistWidgetClass
{
	GtkScrolledWindowClass parent_class;
};

static void gmpv_playlist_widget_constructed(GObject *object);
static void gmpv_playlist_widget_set_property(	GObject *object,
						guint property_id,
						const GValue *value,
						GParamSpec *pspec );
static void gmpv_playlist_widget_get_property(	GObject *object,
						guint property_id,
						GValue *value,
						GParamSpec *pspec );
static void drag_data_get_handler(	GtkWidget *widget,
					GdkDragContext *context,
					GtkSelectionData *sel_data,
					guint info,
					guint time,
					gpointer data );
static void drag_data_received_handler(	GtkWidget *widget,
					GdkDragContext *context,
					gint x,
					gint y,
					GtkSelectionData *sel_data,
					guint info,
					guint time,
					gpointer data );
static void drag_data_delete_handler(	GtkWidget *widget,
					GdkDragContext *context,
					gpointer data );
static void row_activated_handler(	GtkTreeView *tree_view,
					GtkTreePath *path,
					GtkTreeViewColumn *column,
					gpointer data );
static void row_deleted_handler(GmpvPlaylist *pl, gint pos, gpointer data);
static void row_reodered_handler(	GmpvPlaylist *pl,
					gint src,
					gint dest,
					gpointer data );
static gboolean mouse_press_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data );
static gchar *get_uri_selected(GmpvPlaylistWidget *wgt);

G_DEFINE_TYPE(GmpvPlaylistWidget, gmpv_playlist_widget, GTK_TYPE_SCROLLED_WINDOW)

static void gmpv_playlist_widget_constructed(GObject *object)
{
	GmpvPlaylistWidget *self = GMPV_PLAYLIST_WIDGET(object);
	GtkTargetEntry targets[] = DND_TARGETS;

	self->tree_view
		= gtk_tree_view_new_with_model
			(GTK_TREE_MODEL(gmpv_playlist_get_store(self->store)));

	g_signal_connect(	self->tree_view,
				"button-press-event",
				G_CALLBACK(mouse_press_handler),
				NULL );
	g_signal_connect(	self->tree_view,
				"drag-data-get",
				G_CALLBACK(drag_data_get_handler),
				self );
	g_signal_connect(	self->tree_view,
				"drag-data-received",
				G_CALLBACK(drag_data_received_handler),
				self );
	g_signal_connect(	self->tree_view,
				"drag-data-delete",
				G_CALLBACK(drag_data_delete_handler),
				self );
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

	gtk_tree_view_enable_model_drag_source(	GTK_TREE_VIEW(self->tree_view),
						GDK_BUTTON1_MASK,
						targets,
						G_N_ELEMENTS(targets),
						GDK_ACTION_COPY|
						GDK_ACTION_LINK|
						GDK_ACTION_MOVE );

	gtk_tree_view_enable_model_drag_dest(	GTK_TREE_VIEW(self->tree_view),
						targets,
						G_N_ELEMENTS(targets),
						GDK_ACTION_MOVE );

	gtk_widget_set_can_focus(self->tree_view, FALSE);
	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(self->tree_view), FALSE);

	gtk_tree_view_append_column
		(GTK_TREE_VIEW(self->tree_view), self->title_column);

	gtk_container_add(GTK_CONTAINER(self), self->tree_view);

	G_OBJECT_CLASS(gmpv_playlist_widget_parent_class)->constructed(object);
}

static void gmpv_playlist_widget_set_property(	GObject *object,
						guint property_id,
						const GValue *value,
						GParamSpec *pspec )
{
	GmpvPlaylistWidget *self = GMPV_PLAYLIST_WIDGET(object);

	if(property_id == PROP_STORE)
	{
		self->store = g_value_get_pointer(value);

	}
	else
	{
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void gmpv_playlist_widget_get_property(	GObject *object,
						guint property_id,
						GValue *value,
						GParamSpec *pspec )
{
	GmpvPlaylistWidget *self = GMPV_PLAYLIST_WIDGET(object);

	if(property_id == PROP_STORE)
	{
		g_value_set_pointer(value, self->store);
	}
	else
	{
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void drag_data_get_handler(	GtkWidget *widget,
					GdkDragContext *context,
					GtkSelectionData *sel_data,
					guint info,
					guint time,
					gpointer data )
{
	gchar *type = gdk_atom_name(gtk_selection_data_get_target(sel_data));
	gchar *text = get_uri_selected(data);

	if(g_strcmp0(type, "PLAYLIST_PATH") == 0)
	{
		GmpvPlaylistWidget *wgt = data;
		GtkTreePath *path = NULL;
		gchar *path_str = NULL;
		GdkAtom atom = gdk_atom_intern_static_string("PLAYLIST_PATH");

		gtk_tree_view_get_cursor
			(GTK_TREE_VIEW(wgt->tree_view), &path, NULL);

		path_str = gtk_tree_path_to_string(path);

		gtk_selection_data_set(	sel_data,
					atom,
					8,
					(const guchar *)path_str,
					(gint)strlen(path_str) );

		g_free(path_str);
	}
	else if(g_strcmp0(type, "text/uri-list") == 0)
	{
		/* Only one URI can be selected at a time */
		gchar *uris[] = {text, NULL};

		gtk_selection_data_set_uris(sel_data, uris);
	}
	else
	{
		gtk_selection_data_set_text(sel_data, text, -1);
	}

	g_free(type);
	g_free(text);
}

static void drag_data_received_handler(	GtkWidget *widget,
					GdkDragContext *context,
					gint x,
					gint y,
					GtkSelectionData *sel_data,
					guint info,
					guint time,
					gpointer data)
{
	GmpvPlaylistWidget *wgt = data;
	gchar *type = gdk_atom_name(gtk_selection_data_get_target(sel_data));
	gboolean reorder = (widget == gtk_drag_get_source_widget(context));

	if(reorder)
	{
		GtkListStore *store;
		const guchar *raw_data;
		gint src_index, dest_index;
		GtkTreeIter src_iter, dest_iter;
		GtkTreePath *src_path, *dest_path;
		GtkTreeViewDropPosition before_mask;
		GtkTreeViewDropPosition drop_pos;
		gboolean insert_before;
		gboolean dest_row_exist;

		store = gmpv_playlist_get_store(wgt->store);
		raw_data = gtk_selection_data_get_data(sel_data);
		src_path =	gtk_tree_path_new_from_string
				((const gchar *)raw_data);
		src_index = gtk_tree_path_get_indices(src_path)[0];
		before_mask =	GTK_TREE_VIEW_DROP_BEFORE|
				GTK_TREE_VIEW_DROP_INTO_OR_BEFORE;
		dest_row_exist =	gtk_tree_view_get_dest_row_at_pos
					(	GTK_TREE_VIEW(widget),
						x, y,
						&dest_path,
						&drop_pos );

		wgt->dnd_delete = FALSE;

		g_assert(g_strcmp0(type, "PLAYLIST_PATH") == 0);
		g_assert(src_path);

		gtk_tree_model_get_iter(	GTK_TREE_MODEL(store),
						&src_iter,
						src_path );

		if(dest_row_exist)
		{
			g_assert(dest_path);

			dest_index = gtk_tree_path_get_indices(dest_path)[0];

			gtk_tree_model_get_iter(	GTK_TREE_MODEL(store),
							&dest_iter,
							dest_path );
		}
		else
		{
			/* Set dest_iter to the last child */
			GtkTreeIter iter;
			gboolean has_next;

			drop_pos = GTK_TREE_VIEW_DROP_AFTER;
			dest_index = -1;

			gtk_tree_model_get_iter_first
				(GTK_TREE_MODEL(store), &iter);

			do
			{
				dest_iter = iter;
				has_next =	gtk_tree_model_iter_next
						(GTK_TREE_MODEL(store), &iter);

				dest_index++;
			}
			while(has_next);
		}

		insert_before = (drop_pos&before_mask || drop_pos == 0);

		if(insert_before)
		{
			gtk_list_store_move_before(store, &src_iter, &dest_iter);
		}
		else
		{
			gtk_list_store_move_after(store, &src_iter, &dest_iter);
		}

		g_signal_emit_by_name(	wgt,
					"row-reordered",
					src_index+(src_index>dest_index),
					dest_index+!insert_before );

		gtk_tree_path_free(src_path);
		gtk_tree_path_free(dest_path);
	}
	else
	{
		g_signal_emit_by_name(	wgt,
					"drag-data-received",
					context,
					x, y,
					sel_data,
					info,
					time,
					data );
	}
}

static void drag_data_delete_handler(	GtkWidget *widget,
					GdkDragContext *context,
					gpointer data )
{
	GmpvPlaylistWidget *wgt = data;

	/* Prevent default handler from deleting the source row when reordering.
	 * When reordering, dnd_delete will be set to FALSE in
	 * drag_data_received_handler().
	 */
	if(!wgt->dnd_delete)
	{
		g_signal_stop_emission_by_name(widget, "drag-data-delete");
	}

	wgt->dnd_delete = TRUE;
}

static void row_activated_handler(	GtkTreeView *tree_view,
					GtkTreePath *path,
					GtkTreeViewColumn *column,
					gpointer data )
{
	gint *indices = gtk_tree_path_get_indices(path);
	gint64 index = indices?indices[0]:-1;

	g_signal_emit_by_name(data, "row-activated", index);
}

static void row_deleted_handler(GmpvPlaylist *pl, gint pos, gpointer data)
{
	g_signal_emit_by_name(data, "row-deleted", pos);
}

static void row_reodered_handler(	GmpvPlaylist *pl,
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

static gchar *get_uri_selected(GmpvPlaylistWidget *wgt)
{
	GtkTreeIter iter;
	GtkTreePath *path = NULL;
	GtkTreeModel *model = NULL;
	gchar *result = NULL;
	gboolean rc = FALSE;

	gtk_tree_view_get_cursor(GTK_TREE_VIEW(wgt->tree_view), &path, NULL);

	if(path)
	{
		model = GTK_TREE_MODEL(gmpv_playlist_get_store(wgt->store));
		rc = gtk_tree_model_get_iter(model, &iter, path);
	}

	if(rc)
	{
		gtk_tree_model_get
			(model, &iter, PLAYLIST_URI_COLUMN, &result, -1);
	}

	return result;
}

static void gmpv_playlist_widget_class_init(GmpvPlaylistWidgetClass *klass)
{
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec = NULL;

	obj_class->constructed = gmpv_playlist_widget_constructed;
	obj_class->set_property = gmpv_playlist_widget_set_property;
	obj_class->get_property = gmpv_playlist_widget_get_property;

	pspec = g_param_spec_pointer
		(	"store",
			"Store",
			"GmpvPlaylist used to store playlist items",
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

static void gmpv_playlist_widget_init(GmpvPlaylistWidget *wgt)
{
	wgt->title_renderer = gtk_cell_renderer_text_new();
	wgt->title_column
		= gtk_tree_view_column_new_with_attributes
			(	_("Playlist"),
				wgt->title_renderer,
				"text", PLAYLIST_NAME_COLUMN,
				"weight", PLAYLIST_WEIGHT_COLUMN,
				NULL );
	wgt->dnd_delete = TRUE;

	gtk_widget_set_size_request
		(GTK_WIDGET(wgt), PLAYLIST_MIN_WIDTH, -1);
	gtk_tree_view_column_set_sizing
		(wgt->title_column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
}

GtkWidget *gmpv_playlist_widget_new(GmpvPlaylist *store)
{
	return GTK_WIDGET(g_object_new(	gmpv_playlist_widget_get_type(),
					"store", store,
					NULL ));
}

void gmpv_playlist_widget_remove_selected(GmpvPlaylistWidget *wgt)
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

		gmpv_playlist_remove(wgt->store, index);

		g_free(index_str);
	}
}

GmpvPlaylist *gmpv_playlist_widget_get_store(GmpvPlaylistWidget *wgt)
{
	return wgt->store;
}
