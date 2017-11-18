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

#include <string.h>
#include <cairo.h>
#include <glib.h>
#include <gio/gio.h>
#include <gdk/gdk.h>
#include <glib/gi18n.h>

#include "gmpv_playlist_widget.h"
#include "gmpv_metadata_cache.h"
#include "gmpv_marshal.h"
#include "gmpv_common.h"
#include "gmpv_def.h"

enum
{
	PROP_0,
	PROP_PLAYLIST_COUNT,
	N_PROPERTIES
};

enum PlaylistColumn
{
	PLAYLIST_NAME_COLUMN,
	PLAYLIST_URI_COLUMN,
	PLAYLIST_WEIGHT_COLUMN,
	PLAYLIST_N_COLUMNS
};

struct _GmpvPlaylistWidget
{
	GtkScrolledWindow parent_instance;
	gint64 playlist_count;
	GtkListStore *store;
	GtkWidget *tree_view;
	GtkTreeViewColumn *title_column;
	GtkCellRenderer *title_renderer;
	gint last_x;
	gint last_y;
	gboolean dnd_delete;
};

struct _GmpvPlaylistWidgetClass
{
	GtkScrolledWindowClass parent_class;
};

static void constructed(GObject *object);
static void set_property(	GObject *object,
				guint property_id,
				const GValue *value,
				GParamSpec *pspec );
static void get_property(	GObject *object,
				guint property_id,
				GValue *value,
				GParamSpec *pspec );
static void drag_begin_handler(	GtkWidget *widget,
				GdkDragContext *context,
				gpointer data );
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
static void row_inserted_handler(	GtkTreeModel *tree_model,
					GtkTreePath *path,
					GtkTreeIter *iter,
					gpointer data );
static void row_deleted_handler(	GtkTreeModel *tree_model,
					GtkTreePath *path,
					gpointer data );
static gboolean mouse_press_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data );
static gchar *get_uri_selected(GmpvPlaylistWidget *wgt);

G_DEFINE_TYPE(GmpvPlaylistWidget, gmpv_playlist_widget, GTK_TYPE_SCROLLED_WINDOW)

static void constructed(GObject *object)
{
	GmpvPlaylistWidget *self = GMPV_PLAYLIST_WIDGET(object);
	GtkTargetEntry targets[] = DND_TARGETS;

	self->store = gtk_list_store_new(	3,
						G_TYPE_STRING,
						G_TYPE_STRING,
						G_TYPE_INT );
	self->tree_view =	gtk_tree_view_new_with_model
				(GTK_TREE_MODEL(self->store));

	g_signal_connect(	self->tree_view,
				"button-press-event",
				G_CALLBACK(mouse_press_handler),
				self );
	g_signal_connect_after(	self->tree_view,
				"drag-begin",
				G_CALLBACK(drag_begin_handler),
				self );
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
				"row-inserted",
				G_CALLBACK(row_inserted_handler),
				self );
	g_signal_connect(	self->store,
				"row-deleted",
				G_CALLBACK(row_deleted_handler),
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

static void set_property(	GObject *object,
				guint property_id,
				const GValue *value,
				GParamSpec *pspec )
{
	GmpvPlaylistWidget *self = GMPV_PLAYLIST_WIDGET(object);

	if(property_id == PROP_PLAYLIST_COUNT)
	{
		self->playlist_count = g_value_get_int64(value);
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
	GmpvPlaylistWidget *self = GMPV_PLAYLIST_WIDGET(object);

	if(property_id == PROP_PLAYLIST_COUNT)
	{
		g_value_set_int64(value, self->playlist_count);
	}
	else
	{
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void drag_begin_handler(	GtkWidget *widget,
				GdkDragContext *context,
				gpointer data )
{
	const gint padding = 6;
	const gint border_width = 1;
	GmpvPlaylistWidget *wgt = data;
	GtkTreeView *tree_view = GTK_TREE_VIEW(widget);
	GtkStyleContext *style = gtk_widget_get_style_context(widget);
	GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
	GtkTreePath *path = NULL;
	GtkTreeIter iter;
	GdkWindow *window = gtk_widget_get_window(widget);
	PangoContext *pango_ctx = gtk_widget_get_pango_context(widget);
	PangoLayout *layout = pango_layout_new(pango_ctx);
	gint width = 0, height = 0;
	gint text_width = 0, text_height = 0;
	gint cell_x = 0, cell_y = 0;
	gchar *name = NULL;
	GdkRGBA *border_color = NULL;
	GdkRGBA *fg_color = NULL;
	cairo_surface_t *surface = NULL;
	cairo_t *cr = NULL;

	gtk_tree_view_get_path_at_pos(	tree_view,
					wgt->last_x, wgt->last_y,
					NULL,
					NULL,
					&cell_x, &cell_y );

	gtk_tree_view_get_cursor(tree_view, &path, NULL);
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter, PLAYLIST_NAME_COLUMN, &name, -1);
	pango_layout_set_text(layout, name, (gint)strlen(name));
	pango_layout_get_pixel_size(layout, &text_width, &text_height);

	gtk_cell_renderer_get_preferred_height(	wgt->title_renderer,
						widget,
						NULL,
						&height );

	width = text_width+2*(padding+border_width);
	height += 2*border_width;

	surface =	gdk_window_create_similar_surface
			(window, CAIRO_CONTENT_COLOR, width, height);
	cr = cairo_create(surface);

	gtk_style_context_get(	style,
				GTK_STATE_FLAG_NORMAL,
				GTK_STYLE_PROPERTY_COLOR, &fg_color,
				GTK_STYLE_PROPERTY_BORDER_COLOR, &border_color,
				NULL);

	gtk_render_background(style, cr, 0, 0, width, height);

	/* Draw border */
	gdk_cairo_set_source_rgba(cr, border_color);
	cairo_set_line_width(cr, border_width);
	cairo_rectangle(	cr,
				border_width/2.0, border_width/2.0,
				width-border_width, height-border_width );
	cairo_stroke(cr);

	/* Draw text */
	gdk_cairo_set_source_rgba(cr, fg_color);
	cairo_translate(cr, padding, (height-text_height)/2.0);
	pango_cairo_show_layout(cr, layout);

	cairo_surface_set_device_offset
		(	surface,
			(-1)*(MIN(cell_x+border_width, width-border_width)),
			(-1)*(cell_y+border_width) );
	gtk_drag_set_icon_surface(context, surface);

	gdk_rgba_free(fg_color);
	gdk_rgba_free(border_color);
	cairo_destroy(cr);
	g_object_unref(layout);
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
	gboolean reorder = (widget == gtk_drag_get_source_widget(context));
#ifndef G_DISABLE_ASSERT /* Only used in an assertion below */
	gchar *type = gdk_atom_name(gtk_selection_data_get_target(sel_data));
#endif

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

		store = wgt->store;
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
					"rows-reordered",
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

static void row_inserted_handler(	GtkTreeModel *tree_model,
					GtkTreePath *path,
					GtkTreeIter *iter,
					gpointer data )
{
	const gint pos = gtk_tree_path_get_indices(path)[0];

	GMPV_PLAYLIST_WIDGET(data)->playlist_count++;
	g_signal_emit_by_name(data, "row-inserted", pos);
	g_object_notify(data, "playlist-count");
}

static void row_deleted_handler(	GtkTreeModel *tree_model,
					GtkTreePath *path,
					gpointer data )
{
	const gint pos = gtk_tree_path_get_indices(path)[0];

	GMPV_PLAYLIST_WIDGET(data)->playlist_count--;
	g_signal_emit_by_name(data, "row-deleted", pos);
	g_object_notify(data, "playlist-count");
}

static gboolean mouse_press_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data )
{
	GmpvPlaylistWidget *wgt = data;
	GdkEventButton *btn_event = (GdkEventButton *)event;
	gboolean handled;

	wgt->last_x = (gint)btn_event->x;
	wgt->last_y = (gint)btn_event->y;

	handled = (	btn_event->type == GDK_BUTTON_PRESS &&
			btn_event->button == 3 );

	if(handled)
	{
		GMenu *menu;
		GMenuItem *add_menu_item;
		GMenuItem *add_loc_menu_item;
		GMenuItem *shuffle_menu_item;
		GMenuItem *loop_file_menu_item;
		GMenuItem *loop_playlist_menu_item;
		GtkWidget *ctx_menu;

		menu = g_menu_new();
		add_menu_item
			=	g_menu_item_new
				(	_("_Add…"),
					"win.show-open-dialog(true)" );
		add_loc_menu_item
			=	g_menu_item_new
				(	_("Add _Location…"),
					"win.show-open-location-dialog(true)" );
		shuffle_menu_item
			=	g_menu_item_new
				(	_("_Shuffle"),
					"win.shuffle-playlist" );
		loop_file_menu_item
			=	g_menu_item_new
				(	_("Loop _File"),
					"win.toggle-loop-file" );
		loop_playlist_menu_item
			=	g_menu_item_new
				(	_("Loop _Playlist"),
					"win.toggle-loop-playlist" );

		g_menu_append_item(menu, add_menu_item);
		g_menu_append_item(menu, add_loc_menu_item);
		g_menu_append_item(menu, shuffle_menu_item);
		g_menu_append_item(menu, loop_file_menu_item);
		g_menu_append_item(menu, loop_playlist_menu_item);
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
	gchar *result = NULL;
	gboolean rc = FALSE;

	gtk_tree_view_get_cursor(GTK_TREE_VIEW(wgt->tree_view), &path, NULL);

	if(path)
	{
		rc =	gtk_tree_model_get_iter
			(GTK_TREE_MODEL(wgt->store), &iter, path);
	}

	if(rc)
	{
		gtk_tree_model_get(	GTK_TREE_MODEL(wgt->store),
					&iter,
					PLAYLIST_URI_COLUMN, &result,
					-1 );
	}

	return result;
}

static void gmpv_playlist_widget_class_init(GmpvPlaylistWidgetClass *klass)
{
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec = NULL;

	obj_class->constructed = constructed;
	obj_class->set_property = set_property;
	obj_class->get_property = get_property;

	pspec = g_param_spec_int64
		(	"playlist-count",
			"Playlist count",
			"The number of items in the playlist",
			0,
			G_MAXINT64,
			0,
			G_PARAM_READABLE );
	g_object_class_install_property(obj_class, PROP_PLAYLIST_COUNT, pspec);

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
	g_signal_new(	"rows-reordered",
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
	wgt->playlist_count = 0;
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

GtkWidget *gmpv_playlist_widget_new()
{
	return GTK_WIDGET(g_object_new(gmpv_playlist_widget_get_type(), NULL));
}

gboolean gmpv_playlist_widget_empty(GmpvPlaylistWidget *wgt)
{
	GtkTreeIter iter;

	return !gtk_tree_model_get_iter_first(GTK_TREE_MODEL(wgt->store), &iter);
}

void gmpv_playlist_widget_set_indicator_pos(	GmpvPlaylistWidget *wgt,
							gint pos )
{
	GtkTreeIter iter;
	gboolean rc;

	rc = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(wgt->store), &iter);

	while(rc)
	{
		const PangoWeight weight =	(pos-- == 0)?
						PANGO_WEIGHT_BOLD:
						PANGO_WEIGHT_NORMAL;

		gtk_list_store_set(	wgt->store,
					&iter,
					PLAYLIST_WEIGHT_COLUMN, weight,
					-1 );

		rc = gtk_tree_model_iter_next(GTK_TREE_MODEL(wgt->store), &iter);
	}
}

void gmpv_playlist_widget_remove_selected(GmpvPlaylistWidget *wgt)
{
	GtkTreePath *path = NULL;

	gtk_tree_view_get_cursor
		(	GTK_TREE_VIEW(wgt->tree_view),
			&path,
			NULL );

	if(path)
	{
		GtkTreeIter iter;
		GtkTreeModel *model;

		model = GTK_TREE_MODEL(wgt->store);

		if(gtk_tree_model_get_iter(model, &iter, path))
		{
			gtk_list_store_remove(wgt->store, &iter);
		}
	}
}

void gmpv_playlist_widget_queue_draw(GmpvPlaylistWidget *wgt)
{
	gtk_widget_queue_draw(GTK_WIDGET(wgt));
	gtk_widget_queue_draw(wgt->tree_view);
}

void gmpv_playlist_widget_update_contents(	GmpvPlaylistWidget *wgt,
						GPtrArray* playlist )
{
	GtkListStore *store = wgt->store;
	gboolean iter_end = FALSE;
	GtkTreeIter iter;

	g_assert(playlist);

	g_signal_handlers_block_by_func(wgt->store, row_inserted_handler, wgt);
	g_signal_handlers_block_by_func(wgt->store, row_deleted_handler, wgt);

	iter_end = !gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);

	for(guint i = 0; i < playlist->len; i++)
	{
		GmpvPlaylistEntry *entry= g_ptr_array_index(playlist, i);
		gchar *uri = entry->filename;
		gchar *title = entry->title;
		gchar *name = title?g_strdup(title):get_name_from_path(uri);

		/* Overwrite current entry if it doesn't match the new value */
		if(!iter_end)
		{
			gchar *old_name =	NULL;
			gchar *old_uri =	NULL;
			gboolean name_update =	FALSE;
			gboolean uri_update =	FALSE;

			gtk_tree_model_get(	GTK_TREE_MODEL(store),
						&iter,
						PLAYLIST_NAME_COLUMN, &old_name,
						PLAYLIST_URI_COLUMN, &old_uri,
						-1 );

			name_update =	(g_strcmp0(name, old_name) != 0);
			uri_update =	(g_strcmp0(uri, old_uri) != 0);

			/* Only set the name if either the title can be
			 * retrieved or the name is unset. This preserves the
			 * correct title if it becomes unavailable later such as
			 * when restarting mpv.
			 */
			if(name_update && (!old_name || title || uri_update))
			{
				gtk_list_store_set(	store,
							&iter,
							PLAYLIST_NAME_COLUMN,
							name,
							-1 );
			}

			if(uri_update)
			{
				gtk_list_store_set(	store,
							&iter,
							PLAYLIST_URI_COLUMN,
							uri,
							-1 );
			}

			iter_end =	!gtk_tree_model_iter_next
					(GTK_TREE_MODEL(store), &iter);

			g_free(old_name);
			g_free(old_uri);
		}
		/* Append entries to the playlist if there are fewer entries in
		 * the playlist widget than given playlist.
		 */
		else
		{
			gtk_list_store_append(store, &iter);
			gtk_list_store_set(	store,
						&iter,
						PLAYLIST_NAME_COLUMN,
						name,
						-1 );
			gtk_list_store_set(	store,
						&iter,
						PLAYLIST_URI_COLUMN,
						uri,
						-1 );

			wgt->playlist_count++;
		}

		g_free(name);
	}

	/* If there are more entries in the playlist widget than given playlist,
	 * remove the excess entries from the playlist widget.
	 */
	if(!iter_end)
	{
		while(gtk_list_store_remove(store, &iter))
		{
			wgt->playlist_count--;
		}
	}

	g_signal_handlers_unblock_by_func(wgt->store, row_inserted_handler, wgt);
	g_signal_handlers_unblock_by_func(wgt->store, row_deleted_handler, wgt);
	g_object_notify(G_OBJECT(wgt), "playlist-count");
}

GPtrArray *gmpv_playlist_widget_get_contents(GmpvPlaylistWidget *wgt)
{
	gboolean rc = TRUE;
	GtkTreeModel *model = GTK_TREE_MODEL(wgt->store);
	GtkTreeIter iter;
	GPtrArray *result = NULL;

	rc = gtk_tree_model_get_iter_first(model, &iter);
	result = g_ptr_array_new_full(	1,
					(GDestroyNotify)
					gmpv_playlist_entry_free );

	while(rc)
	{
		gchar *uri = NULL;
		gchar *name = NULL;

		gtk_tree_model_get(	model, &iter,
					PLAYLIST_URI_COLUMN, &uri,
					PLAYLIST_NAME_COLUMN, &name,
					-1 );

		g_ptr_array_add(result, gmpv_playlist_entry_new(uri, name));

		rc = gtk_tree_model_iter_next(model, &iter);
	}

	return result;
}
