/*
 * Copyright (c) 2014-2021 gnome-mpv
 *
 * This file is part of Celluloid.
 *
 * Celluloid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Celluloid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Celluloid.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <cairo.h>
#include <glib.h>
#include <gio/gio.h>
#include <gdk/gdk.h>
#include <glib/gi18n.h>

#include "celluloid-playlist-widget.h"
#include "celluloid-playlist-model.h"
#include "celluloid-playlist-item.h"
#include "celluloid-metadata-cache.h"
#include "celluloid-marshal.h"
#include "celluloid-common.h"
#include "celluloid-menu.h"
#include "celluloid-def.h"

enum
{
	PROP_0,
	PROP_PLAYLIST_COUNT,
	PROP_SEARCHING,
	N_PROPERTIES
};

enum PlaylistColumn
{
	PLAYLIST_NAME_COLUMN,
	PLAYLIST_URI_COLUMN,
	PLAYLIST_WEIGHT_COLUMN,
	PLAYLIST_N_COLUMNS
};

struct _CelluloidPlaylistWidget
{
	GtkBox parent_instance;
	gint64 playlist_count;
	gboolean searching;
	GtkListStore *store;
	GtkWidget *scrolled_window;
	GtkWidget *tree_view;
	CelluloidPlaylistModel *model;
	GtkWidget *list_box;
	gint last_selected;
	GtkTreeViewColumn *title_column;
	GtkCellRenderer *title_renderer;
	GtkWidget *search_bar;
	GtkWidget *search_entry;
	GtkWidget *placeholder;
	gint last_x;
	gint last_y;
	gboolean dnd_delete;
};

struct _CelluloidPlaylistWidgetClass
{
	GtkBoxClass parent_class;
};

static gint
get_selected_index(CelluloidPlaylistWidget *wgt);

static void
select_index(CelluloidPlaylistWidget *wgt, gint index);

static void
find_match(	CelluloidPlaylistWidget *wgt,
		gboolean match_current,
		gboolean reverse );

static void
constructed(GObject *object);

static void
set_property(	GObject *object,
		guint property_id,
		const GValue *value,
		GParamSpec *pspec );

static void
get_property(	GObject *object,
		guint property_id,
		GValue *value,
		GParamSpec *pspec );

static gboolean
is_zero(GBinding *binding, const GValue *from, GValue *to, gpointer data);

static void
row_activated_handler(	GtkListBox *list_box,
			GtkListBoxRow *row,
			gpointer data );

static void
row_selected_handler(	GtkListBox *list_box,
			GtkListBoxRow *row,
			gpointer data );

static void
items_changed_handler(	CelluloidPlaylistModel *model,
			guint position,
			guint removed,
			guint added,
			gpointer data );

static void
contents_changed_handler(	CelluloidPlaylistModel *model,
				guint position,
				guint removed,
				guint added,
				gpointer data );

static void
next_match_handler(GtkSearchEntry *entry, gpointer data);

static void
previous_match_handler(GtkSearchEntry *entry, gpointer data);

static void
search_changed_handler(GtkSearchEntry *entry, gpointer data);

static void
stop_search_handler(GtkSearchEntry *entry, gpointer data);

static gboolean
pressed_handler(	GtkGestureClick *gesture,
			gint n_press,
			gdouble x,
			gdouble y,
			gpointer data );

G_DEFINE_TYPE(CelluloidPlaylistWidget, celluloid_playlist_widget, GTK_TYPE_BOX)

static gint
get_selected_index(CelluloidPlaylistWidget *wgt)
{
	GtkListBoxRow *row =
		gtk_list_box_get_selected_row(GTK_LIST_BOX(wgt->list_box));

	return row ? gtk_list_box_row_get_index(row) : -1;
}

static void
select_index(CelluloidPlaylistWidget *wgt, gint index)
{
	GtkListBoxRow *row =
		gtk_list_box_get_row_at_index
		(GTK_LIST_BOX(wgt->list_box), index);

	gtk_list_box_select_row(GTK_LIST_BOX(wgt->list_box), row);
}

static void
find_match(	CelluloidPlaylistWidget *wgt,
		gboolean match_current,
		gboolean reverse )
{
	const gchar *term = gtk_editable_get_text(GTK_EDITABLE(wgt->search_entry));
	const guint len = g_list_model_get_n_items(G_LIST_MODEL(wgt->model));
	guint initial_index = (guint)MAX(0, get_selected_index(wgt));
	guint i = initial_index;
	gboolean found = FALSE;

	do
	{
		if(!match_current)
		{
			if(reverse)
			{
				i = (i == 0 ? len : i) - 1;
			}
			else
			{
				i = i >= len - 1 ? 0 : i + 1;
			}
		}

		CelluloidPlaylistItem *item =
			g_list_model_get_item(G_LIST_MODEL(wgt->model), i);
		const gchar *title =
			celluloid_playlist_item_get_title(item);

		found = g_str_match_string(term, title, TRUE);
		match_current = FALSE;
	}
	while(!found && i != initial_index);

	if(found)
	{
		select_index(wgt, (gint)i);
	}
}

static GtkWidget *
make_row(GObject *object, gpointer data)
{
	GtkWidget *row = gtk_list_box_row_new();
	CelluloidPlaylistItem *item = CELLULOID_PLAYLIST_ITEM(object);
	const gchar *text = celluloid_playlist_item_get_title(item);
	GtkWidget *label = gtk_label_new(text);

	if(celluloid_playlist_item_get_is_current(item))
	{
		PangoAttrList *attrs;
		PangoAttribute *weight;

		attrs = pango_attr_list_new();
		weight = pango_attr_weight_new(PANGO_WEIGHT_BOLD);

		pango_attr_list_insert(attrs, weight);
		gtk_label_set_attributes(GTK_LABEL(label), attrs);

		pango_attr_list_unref(attrs);
	}

	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_widget_set_margin_start(label, 6);
	gtk_widget_set_margin_end(label, 6);
	gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);

	return row;
}

static void
constructed(GObject *object)
{
	CelluloidPlaylistWidget *self = CELLULOID_PLAYLIST_WIDGET(object);

	self->store = gtk_list_store_new(	3,
						G_TYPE_STRING,
						G_TYPE_STRING,
						G_TYPE_INT );
	self->tree_view =	gtk_tree_view_new_with_model
				(GTK_TREE_MODEL(self->store));
	self->model = celluloid_playlist_model_new();
	self->list_box = gtk_list_box_new();
	self->last_selected = -1;

	gtk_list_box_set_selection_mode
		(GTK_LIST_BOX(self->list_box), GTK_SELECTION_BROWSE);
	gtk_list_box_set_activate_on_single_click
		(GTK_LIST_BOX(self->list_box), FALSE);
	gtk_list_box_set_placeholder
		(GTK_LIST_BOX(self->list_box), self->placeholder);

	gtk_list_box_bind_model
		(	GTK_LIST_BOX(self->list_box),
			G_LIST_MODEL(self->model),
			(GtkListBoxCreateWidgetFunc)make_row,
			NULL,
			NULL );

	GtkGesture *click_gesture = gtk_gesture_click_new();

	gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click_gesture), 0);

	gtk_widget_add_controller(	GTK_WIDGET(self->list_box),
					GTK_EVENT_CONTROLLER(click_gesture) );

	g_signal_connect(	click_gesture,
				"pressed",
				G_CALLBACK(pressed_handler),
				self );

	g_signal_connect(	self->list_box,
				"row-activated",
				G_CALLBACK(row_activated_handler),
				self );
	g_signal_connect(	self->list_box,
				"row-selected",
				G_CALLBACK(row_selected_handler),
				self );

	g_signal_connect(	self->model,
				"items-changed",
				G_CALLBACK(items_changed_handler),
				self );
	g_signal_connect(	self->model,
				"contents-changed",
				G_CALLBACK(contents_changed_handler),
				self );

	g_signal_connect(	self->search_entry,
				"next-match",
				G_CALLBACK(next_match_handler),
				self );
	g_signal_connect(	self->search_entry,
				"previous-match",
				G_CALLBACK(previous_match_handler),
				self );
	g_signal_connect(	self->search_entry,
				"search-changed",
				G_CALLBACK(search_changed_handler),
				self );
	g_signal_connect(	self->search_entry,
				"stop-search",
				G_CALLBACK(stop_search_handler),
				self );

	g_object_bind_property(	self, "searching",
				self->search_bar, "search-mode-enabled",
				G_BINDING_BIDIRECTIONAL );
	g_object_bind_property_full(	self, "playlist-count",
					self->placeholder, "visible",
					G_BINDING_DEFAULT,
					is_zero,
					NULL,
					NULL,
					NULL );

	gtk_widget_set_can_focus(self->tree_view, FALSE);
	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(self->tree_view), FALSE);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(self->tree_view), FALSE);

	gtk_tree_view_append_column
		(GTK_TREE_VIEW(self->tree_view), self->title_column);

	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(self->scrolled_window), self->list_box);
	gtk_box_append(GTK_BOX(self), self->scrolled_window);

	gtk_search_bar_set_child(GTK_SEARCH_BAR(self->search_bar), self->search_entry);
	gtk_box_append(GTK_BOX(self), self->search_bar);

	G_OBJECT_CLASS(celluloid_playlist_widget_parent_class)->constructed(object);
}

static void
set_property(	GObject *object,
		guint property_id,
		const GValue *value,
		GParamSpec *pspec )
{
	CelluloidPlaylistWidget *self = CELLULOID_PLAYLIST_WIDGET(object);

	if(property_id == PROP_PLAYLIST_COUNT)
	{
		self->playlist_count = g_value_get_int64(value);
	}
	else if(property_id == PROP_SEARCHING)
	{
		self->searching = g_value_get_boolean(value);

		if(self->searching)
		{
			gtk_widget_grab_focus(self->search_entry);
		}
	}
	else
	{
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void
get_property(	GObject *object,
		guint property_id,
		GValue *value,
		GParamSpec *pspec )
{
	CelluloidPlaylistWidget *self = CELLULOID_PLAYLIST_WIDGET(object);

	if(property_id == PROP_PLAYLIST_COUNT)
	{
		g_value_set_int64(value, self->playlist_count);
	}
	else if(property_id == PROP_SEARCHING)
	{
		g_value_set_boolean(value, self->searching);
	}
	else
	{
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static gboolean
is_zero(GBinding *binding, const GValue *from, GValue *to, gpointer data)
{
	g_value_set_boolean(to, g_value_get_int64(from) == 0);

	return TRUE;
}

static void
row_activated_handler(	GtkListBox *list_box,
			GtkListBoxRow *row,
			gpointer data )
{
	const gint index = gtk_list_box_row_get_index(row);

	g_signal_emit_by_name(data, "row-activated", index);
}

static void
row_selected_handler(	GtkListBox *list_box,
			GtkListBoxRow *row,
			gpointer data )
{
	CelluloidPlaylistWidget *self = CELLULOID_PLAYLIST_WIDGET(data);

	if(row)
	{
		self->last_selected = gtk_list_box_row_get_index(row);
	}
}

static void
items_changed_handler(	CelluloidPlaylistModel *model,
			guint position,
			guint removed,
			guint added,
			gpointer data )
{
	CelluloidPlaylistWidget *self = CELLULOID_PLAYLIST_WIDGET(data);

	select_index(self, self->last_selected);
}

static void
contents_changed_handler(	CelluloidPlaylistModel *model,
				guint position,
				guint removed,
				guint added,
				gpointer data )
{
	while(removed-- > 0)
	{
		CELLULOID_PLAYLIST_WIDGET(data)->playlist_count--;
		g_signal_emit_by_name(data, "row-deleted", position);
	}

	while(added-- > 0)
	{
		CELLULOID_PLAYLIST_WIDGET(data)->playlist_count++;
		g_signal_emit_by_name(data, "row-inserted", position);
	}

	g_object_notify(data, "playlist-count");
}

static void
next_match_handler(GtkSearchEntry *entry, gpointer data)
{
	find_match(data, FALSE, FALSE);
}

static void
previous_match_handler(GtkSearchEntry *entry, gpointer data)
{
	find_match(data, FALSE, TRUE);
}

static void
search_changed_handler(GtkSearchEntry *entry, gpointer data)
{
	find_match(data, TRUE, FALSE);
}

static void
stop_search_handler(GtkSearchEntry *entry, gpointer data)
{
	g_object_set(data, "searching", FALSE, NULL);
}

static gboolean
pressed_handler(	GtkGestureClick *gesture,
			gint n_press,
			gdouble x,
			gdouble y,
			gpointer data )
{
	CelluloidPlaylistWidget *wgt = data;

	wgt->last_x = (gint)x;
	wgt->last_y = (gint)y;

	if(	n_press == 1 &&
		gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture)) == 3 )
	{
		const CelluloidMenuEntry entries[]
			= {	CELLULOID_MENU_SEPARATOR,
				CELLULOID_MENU_ITEM(_("_Copy Location"), "win.copy-selected-playlist-item"),
				CELLULOID_MENU_ITEM(_("_Remove"), "win.remove-selected-playlist-item"),
				CELLULOID_MENU_SEPARATOR,
				CELLULOID_MENU_ITEM(_("_Add…"), "win.show-open-dialog((false, true))"),
				CELLULOID_MENU_ITEM(_("Add _Folder…"), "win.show-open-dialog((true, true))"),
				CELLULOID_MENU_ITEM(_("Add _Location…"), "win.show-open-location-dialog(true)"),
				CELLULOID_MENU_ITEM(_("_Shuffle"), "win.toggle-shuffle-playlist"),
				CELLULOID_MENU_ITEM(_("Loop _File"), "win.toggle-loop-file"),
				CELLULOID_MENU_ITEM(_("Loop _Playlist"), "win.toggle-loop-playlist"),
				CELLULOID_MENU_END };

		gsize entries_offset = 0;
		GMenu *menu = g_menu_new();

		GtkListBoxRow *row =
			gtk_list_box_get_row_at_y
			(GTK_LIST_BOX(wgt->list_box), (gint)y);

		gtk_list_box_select_row(GTK_LIST_BOX(wgt->list_box), row);

		if(!row)
		{
			/* Skip the first section which only contains item-level
			 * actions
			 */
			while(entries[++entries_offset].title);
		}

		celluloid_menu_build_menu(menu, entries+entries_offset, TRUE);
		g_menu_freeze(menu);

		GdkRectangle rect =
			{.x = (gint)x, .y = (gint)y, .width = 0, .height = 0};
		GtkWidget *ctx_menu =
			gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));

		gtk_widget_set_parent(ctx_menu, GTK_WIDGET(wgt));
		gtk_popover_set_has_arrow(GTK_POPOVER(ctx_menu), FALSE);
		gtk_popover_set_pointing_to(GTK_POPOVER(ctx_menu), &rect);
		gtk_widget_set_halign(ctx_menu, GTK_ALIGN_START);
		gtk_widget_show(ctx_menu);
	}

	return FALSE;
}

static void
celluloid_playlist_widget_class_init(CelluloidPlaylistWidgetClass *klass)
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

	pspec = g_param_spec_boolean
		(	"searching",
			"Searching",
			"Whether or not the user is searching the playlist",
			FALSE,
			G_PARAM_READWRITE );
	g_object_class_install_property(obj_class, PROP_SEARCHING, pspec);

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

static void
celluloid_playlist_widget_init(CelluloidPlaylistWidget *wgt)
{
	GtkStyleContext *style = NULL;

	wgt->playlist_count = 0;
	wgt->searching = FALSE;
	wgt->title_renderer = gtk_cell_renderer_text_new();
	wgt->title_column
		= gtk_tree_view_column_new_with_attributes
			(	_("Playlist"),
				wgt->title_renderer,
				"text", PLAYLIST_NAME_COLUMN,
				"weight", PLAYLIST_WEIGHT_COLUMN,
				NULL );
	wgt->scrolled_window = gtk_scrolled_window_new();
	wgt->search_bar = gtk_search_bar_new();
	wgt->search_entry = gtk_search_entry_new();
	wgt->placeholder = gtk_label_new(_("Playlist is empty"));
	wgt->dnd_delete = TRUE;

	gtk_widget_set_vexpand(wgt->scrolled_window, TRUE);

	style = gtk_widget_get_style_context(wgt->placeholder);
	gtk_style_context_add_class(style, "dim-label");

	gtk_orientable_set_orientation
		(GTK_ORIENTABLE(wgt), GTK_ORIENTATION_VERTICAL);
	gtk_widget_set_size_request
		(GTK_WIDGET(wgt), PLAYLIST_MIN_WIDTH, -1);
	gtk_tree_view_column_set_sizing
		(wgt->title_column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
}

GtkWidget *
celluloid_playlist_widget_new()
{
	return GTK_WIDGET(g_object_new(celluloid_playlist_widget_get_type(), NULL));
}

gboolean
celluloid_playlist_widget_empty(CelluloidPlaylistWidget *wgt)
{
	GtkTreeIter iter;

	return !gtk_tree_model_get_iter_first(GTK_TREE_MODEL(wgt->store), &iter);
}

void
celluloid_playlist_widget_set_indicator_pos(	CelluloidPlaylistWidget *wgt,
						gint pos )
{
	celluloid_playlist_model_set_current(wgt->model, pos);
}

void
celluloid_playlist_widget_copy_selected(CelluloidPlaylistWidget *wgt)
{
	GtkTreePath *path = NULL;

	gtk_tree_view_get_cursor
		(	GTK_TREE_VIEW(wgt->tree_view),
			&path,
			NULL );

	if(path)
	{
		GtkTreeIter iter;
		GtkTreeModel *model = GTK_TREE_MODEL(wgt->store);
		GValue value = G_VALUE_INIT;

		if(gtk_tree_model_get_iter(model, &iter, path))
		{
			const gchar *uri;

			gtk_tree_model_get_value
				(model, &iter, PLAYLIST_URI_COLUMN, &value);

			uri = g_value_get_string(&value);

			if(uri)
			{
				GdkClipboard *clipboard;

				clipboard = gtk_widget_get_clipboard(GTK_WIDGET(wgt));

				gdk_clipboard_set_text(clipboard, uri);
			}
		}
	}
}

void
celluloid_playlist_widget_remove_selected(CelluloidPlaylistWidget *wgt)
{
	gint index = get_selected_index(wgt);

	if(index >= 0)
	{
		celluloid_playlist_model_remove(wgt->model, (guint)index);
	}
}

void
celluloid_playlist_widget_queue_draw(CelluloidPlaylistWidget *wgt)
{
	gtk_widget_queue_draw(GTK_WIDGET(wgt));
}

void
celluloid_playlist_widget_update_contents(	CelluloidPlaylistWidget *wgt,
						GPtrArray* playlist )
{
	g_signal_handlers_block_by_func(wgt->model, contents_changed_handler, wgt);

	const gint current = celluloid_playlist_model_get_current(wgt->model);

	celluloid_playlist_model_clear(wgt->model);

	for(guint i = 0; i < playlist->len; i++)
	{
		CelluloidPlaylistEntry *entry = g_ptr_array_index(playlist, i);

		CelluloidPlaylistItem *item =
			celluloid_playlist_item_new_take
			(	g_strdup(entry->title),
				g_strdup(entry->filename),
				(gint)i == current	);

		celluloid_playlist_model_append(wgt->model, item);
	}

	celluloid_playlist_model_set_current
		(wgt->model, MIN(current, (gint)playlist->len - 1));

	g_object_notify(G_OBJECT(wgt), "playlist-count");

	g_signal_handlers_unblock_by_func(wgt->model, contents_changed_handler, wgt);
}

GPtrArray *
celluloid_playlist_widget_get_contents(CelluloidPlaylistWidget *wgt)
{
	gboolean rc = TRUE;
	GtkTreeModel *model = GTK_TREE_MODEL(wgt->store);
	GtkTreeIter iter;
	GPtrArray *result = NULL;
	rc = gtk_tree_model_get_iter_first(model, &iter);
	result = g_ptr_array_new_full(	1,
					(GDestroyNotify)
					celluloid_playlist_entry_free );

	while(rc)
	{
		gchar *uri = NULL;
		gchar *name = NULL;

		gtk_tree_model_get(	model, &iter,
					PLAYLIST_URI_COLUMN, &uri,
					PLAYLIST_NAME_COLUMN, &name,
					-1 );

		g_ptr_array_add(result, celluloid_playlist_entry_new(uri, name));

		rc = gtk_tree_model_iter_next(model, &iter);
	}

	return result;
}
