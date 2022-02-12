/*
 * Copyright (c) 2014-2022 gnome-mpv
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
	CelluloidPlaylistModel *model;
	GtkWidget *list_box;
	gint last_selected;
	gchar *drag_uri;
	GtkWidget *search_bar;
	GtkWidget *search_entry;
	GtkWidget *placeholder;
	gint last_x;
	gint last_y;
	GtkCssProvider *css_provider;
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

static GdkContentProvider *
prepare_handler(	GtkDragSource *source,
			gdouble x,
			gdouble y,
			gpointer data );

static void
drag_end_handler(	GtkDragSource *source,
			GdkDrag *drag,
			gboolean delete_data,
			gpointer data );

static GdkDragAction
motion_handler(	GtkDropTarget *self,
		gdouble x,
		gdouble y,
		gpointer data );

static void
leave_handler(GtkDropTarget *self, gpointer data);

static gboolean
drop_handler(	GtkDropTarget *self,
		GValue *value,
		gdouble x,
		gdouble y,
		gpointer data );

static void
realize_handler(GtkWidget *self, gpointer data);

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
	const gchar *title = celluloid_playlist_item_get_title(item);
	const gchar *uri = celluloid_playlist_item_get_uri(item);
	GtkWidget *label = NULL;
	gchar *text = NULL;

	if(title)
	{
		text = sanitize_utf8(title, TRUE);
	}
	else
	{
		gchar *basename = g_path_get_basename(uri);

		text = sanitize_utf8(basename, TRUE);
		g_free(basename);
	}

	label = gtk_label_new(text);
	g_free(text);

	g_assert(label);

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
	self->model = celluloid_playlist_model_new();
	self->list_box = gtk_list_box_new();
	self->last_selected = -1;
	self->drag_uri = NULL;

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

	GtkDragSource *drag_source = gtk_drag_source_new();
	gtk_widget_add_controller(	GTK_WIDGET(self->list_box),
					GTK_EVENT_CONTROLLER(drag_source) );
	g_signal_connect(	drag_source,
				"prepare",
				G_CALLBACK(prepare_handler),
				self );
	g_signal_connect(	drag_source,
				"drag-end",
				G_CALLBACK(drag_end_handler),
				self );

	GtkDropTarget *drop_target = gtk_drop_target_new(G_TYPE_INVALID, GDK_ACTION_ALL);
	GType types[] = {G_TYPE_INT, GDK_TYPE_FILE_LIST, G_TYPE_STRING};

	gtk_drop_target_set_gtypes(	drop_target,
					types,
					G_N_ELEMENTS(types) );
	gtk_widget_add_controller(	GTK_WIDGET(self->list_box),
					GTK_EVENT_CONTROLLER(drop_target) );
	g_signal_connect(	drop_target,
				"motion",
				G_CALLBACK(motion_handler),
				self );
	g_signal_connect(	drop_target,
				"leave",
				G_CALLBACK(leave_handler),
				self );
	g_signal_connect(	drop_target,
				"drop",
				G_CALLBACK(drop_handler),
				self );

	g_signal_connect(	self->list_box,
				"realize",
				G_CALLBACK(realize_handler),
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

	self->playlist_count = g_list_model_get_n_items(G_LIST_MODEL(model));
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

		if(row)
		{
			gtk_list_box_select_row(GTK_LIST_BOX(wgt->list_box), row);
		}
		else
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

static GdkContentProvider *
prepare_handler(	GtkDragSource *source,
			gdouble x,
			gdouble y,
			gpointer data )
{
	CelluloidPlaylistWidget *wgt =
		CELLULOID_PLAYLIST_WIDGET(data);
	GtkWidget *list_box =
		gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(source));
	GtkListBoxRow *row =
		gtk_list_box_get_row_at_y(GTK_LIST_BOX(list_box), (gint)y);
	GdkContentProvider *provider =
		NULL;

	if(row)
	{
		GdkPaintable *paintable =
			gtk_widget_paintable_new(GTK_WIDGET(row));
		gint index =
			gtk_list_box_row_get_index(row);
		CelluloidPlaylistItem *item =
			g_list_model_get_item
			(G_LIST_MODEL(wgt->model), (guint)index);
		const gchar *uri =
			celluloid_playlist_item_get_uri(item);

		wgt->drag_uri = g_strdup(uri);

		GdkContentProvider *int_provider =
			gdk_content_provider_new_typed
			(G_TYPE_INT, index);
		GdkContentProvider *string_provider =
			gdk_content_provider_new_typed
			(G_TYPE_STRING, wgt->drag_uri);
		GdkContentProvider *subproviders[] =
			{int_provider, string_provider};

		provider = gdk_content_provider_new_union(subproviders, 2);
		gtk_drag_source_set_icon(source, paintable, 100, 0);
	}

	return provider;
}

static void
drag_end_handler(	GtkDragSource *source,
			GdkDrag *drag,
			gboolean delete_data,
			gpointer data )
{
	CelluloidPlaylistWidget *wgt = CELLULOID_PLAYLIST_WIDGET(data);

	g_clear_pointer(&wgt->drag_uri, g_free);
}

static GdkDragAction
motion_handler(	GtkDropTarget *self,
		gdouble x,
		gdouble y,
		gpointer data )
{
	CelluloidPlaylistWidget *wgt = CELLULOID_PLAYLIST_WIDGET(data);
	GdkDrop *drop = gtk_drop_target_get_current_drop(self);
	GdkContentFormats *formats = gdk_drop_get_formats(drop);
	gchar *css_data = NULL;

	GtkEventController *controller =
		GTK_EVENT_CONTROLLER(self);
	GtkWidget *list_box =
		gtk_event_controller_get_widget(controller);
	GtkListBoxRow *row =
		gtk_list_box_get_row_at_y(GTK_LIST_BOX(list_box), (gint)y);

	// External files are always appended, so we should always highlight the
	// last row in that case.
	if(row && gdk_content_formats_contain_gtype(formats, G_TYPE_INT))
	{
		GtkWidget *src = gtk_event_controller_get_widget(controller);
		const gint row_h = gtk_widget_get_height(GTK_WIDGET(row));
		gdouble row_x = 0;
		gdouble row_y = 0;

		gtk_widget_translate_coordinates
			(src, GTK_WIDGET(row), x, y, &row_x, &row_y);

		const gboolean top_half = row_y < (row_h / 2);
		gint row_index = gtk_list_box_row_get_index(row);

		css_data =
			g_strdup_printf
			(	"row:nth-child(%d)		"
				"{				"
				"	%s: 1px solid white;	"
				"}				",
				row_index + 1,
				top_half ? "border-top" : "border-bottom" );
	}
	else
	{
		css_data =
			g_strdup
			(	"row:last-child				"
				"{					"
				"	border-bottom: 1px solid white;	"
				"}					");
	}

	gtk_css_provider_load_from_data(wgt->css_provider, css_data, -1);
	g_free(css_data);

	return GDK_ACTION_MOVE;
}

static void
leave_handler(GtkDropTarget *self, gpointer data)
{
	CelluloidPlaylistWidget *wgt = CELLULOID_PLAYLIST_WIDGET(data);

	gtk_css_provider_load_from_data(wgt->css_provider, "", -1);
}

static gboolean
drop_handler(	GtkDropTarget *self,
		GValue *value,
		gdouble x,
		gdouble y,
		gpointer data )
{
	CelluloidPlaylistWidget *wgt =
		CELLULOID_PLAYLIST_WIDGET(data);
	const guint n_items =
		g_list_model_get_n_items(G_LIST_MODEL(wgt->model));

	if(G_VALUE_HOLDS_INT(value))
	{
		const gint src_index =
			g_value_get_int(value);
		GtkWidget *src =
			gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(self));
		GtkListBoxRow *dst_row =
			gtk_list_box_get_row_at_y(GTK_LIST_BOX(wgt->list_box), (gint)y);

		// Set the destination index to the last position. This is used if
		// the source row is dropped on the empty space following the playlist.
		gint dst_index = (gint)n_items - 1;

		if(dst_row)
		{
			gdouble row_x = 0;
			gdouble row_y = 0;

			gtk_widget_translate_coordinates
				(src, GTK_WIDGET(dst_row), x, y, &row_x, &row_y);

			const gint row_h =
				gtk_widget_get_height(GTK_WIDGET(dst_row));
			const gboolean top_half =
				row_y < (row_h / 2);
			const gint dst_row_index =
				gtk_list_box_row_get_index(dst_row);
			const gint offset =
				(top_half ? -1 : 0) +
				(src_index - 1 < dst_row_index ? 0 : 1);

			dst_index = CLAMP(dst_row_index + offset, 0, (gint)n_items - 1);
		}

		GtkListBoxRow *new_dst_row =
			gtk_list_box_get_row_at_index
			(GTK_LIST_BOX(wgt->list_box), dst_index);

		gtk_list_box_select_row(GTK_LIST_BOX(wgt->list_box), new_dst_row);
		gtk_css_provider_load_from_data(wgt->css_provider, "", -1);

		g_signal_emit_by_name
			(	wgt,
				"rows-reordered",
				src_index + (src_index > dst_index ? 1 : 0),
				dst_index + (src_index < dst_index ? 1 : 0) );
	}
	else if(G_VALUE_HOLDS(value, GDK_TYPE_FILE_LIST))
	{
		guint position = n_items;

		for(	GSList *slist = g_value_get_boxed(value);
			slist;
			slist = slist->next )
		{
			gchar *uri = g_file_get_uri(slist->data);
			gchar *title = g_strdup(uri);

			CelluloidPlaylistItem *item =
				celluloid_playlist_item_new_take
				(title, uri, FALSE);

			celluloid_playlist_model_append(wgt->model, item);
			g_signal_emit_by_name(wgt, "row-inserted", position++);
		}
	}
	else if(G_VALUE_HOLDS_STRING(value))
	{
		const gchar *string =
			g_value_get_string(value);
		CelluloidPlaylistItem *item =
			celluloid_playlist_item_new(string, string, FALSE);

		celluloid_playlist_model_append(wgt->model, item);
		g_signal_emit_by_name(wgt, "row-inserted", n_items);
	}

	return FALSE;
}

static void
realize_handler(GtkWidget *self, gpointer data)
{
	CelluloidPlaylistWidget *wgt = CELLULOID_PLAYLIST_WIDGET(data);
	GtkNative *native = gtk_widget_get_native(self);
	GdkSurface *surface = gtk_native_get_surface(native);
	GdkDisplay *display = gdk_surface_get_display(surface);

	gtk_style_context_add_provider_for_display
		(	display,
			GTK_STYLE_PROVIDER(wgt->css_provider),
			GTK_STYLE_PROVIDER_PRIORITY_APPLICATION );
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
	wgt->scrolled_window = gtk_scrolled_window_new();
	wgt->search_bar = gtk_search_bar_new();
	wgt->search_entry = gtk_search_entry_new();
	wgt->placeholder = gtk_label_new(_("Playlist is empty"));
	wgt->css_provider = gtk_css_provider_new();

	style = gtk_widget_get_style_context(wgt->placeholder);
	gtk_style_context_add_class(style, "dim-label");

	gtk_widget_set_vexpand
		(wgt->scrolled_window, TRUE);
	gtk_label_set_ellipsize
		(GTK_LABEL(wgt->placeholder), PANGO_ELLIPSIZE_END);
	gtk_orientable_set_orientation
		(GTK_ORIENTABLE(wgt), GTK_ORIENTATION_VERTICAL);
}

GtkWidget *
celluloid_playlist_widget_new()
{
	return GTK_WIDGET(g_object_new(celluloid_playlist_widget_get_type(), NULL));
}

gboolean
celluloid_playlist_widget_empty(CelluloidPlaylistWidget *wgt)
{
	return g_list_model_get_n_items(G_LIST_MODEL(wgt->model)) == 0;
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
	GtkListBoxRow *row =
		gtk_list_box_get_selected_row(GTK_LIST_BOX(wgt->list_box));

	if(row)
	{
		const guint index =
			(guint)gtk_list_box_row_get_index(row);
		CelluloidPlaylistItem *item =
			g_list_model_get_item(G_LIST_MODEL(wgt->model), index);
		const gchar *uri =
			celluloid_playlist_item_get_uri(item);
		GdkClipboard *clipboard
			= gtk_widget_get_clipboard(GTK_WIDGET(wgt));

		gdk_clipboard_set_text(clipboard, uri);
	}
}

void
celluloid_playlist_widget_remove_selected(CelluloidPlaylistWidget *wgt)
{
	gint index = get_selected_index(wgt);

	if(index >= 0)
	{
		g_signal_emit_by_name(wgt, "row-deleted", index);
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

	wgt->playlist_count = playlist->len;
	g_object_notify(G_OBJECT(wgt), "playlist-count");
}

GPtrArray *
celluloid_playlist_widget_get_contents(CelluloidPlaylistWidget *wgt)
{
	const guint n_items = g_list_model_get_n_items(G_LIST_MODEL(wgt->model));
	GPtrArray *result =	g_ptr_array_new_full
				(	n_items,
					(GDestroyNotify)
					celluloid_playlist_entry_free );

	for(guint i = 0; i < n_items; i++)
	{
		CelluloidPlaylistItem *item =
			g_list_model_get_item(G_LIST_MODEL(wgt->model), i);
		const gchar *uri =
			celluloid_playlist_item_get_uri(item);
		CelluloidPlaylistEntry *entry =
			celluloid_playlist_entry_new(uri, uri);

		g_ptr_array_add(result, entry);
	}

	return result;
}
