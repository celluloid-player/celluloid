/*
 * Copyright (c) 2014-2022, 2024 gnome-mpv
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
#include <adwaita.h>

#include "celluloid-view.h"
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
	PROP_LOOP_FILE,
	PROP_LOOP_PLAYLIST,
	PROP_SHUFFLE,
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
	AdwBin parent_instance;
	gint64 playlist_count;
	gboolean searching;
	CelluloidPlaylistModel *model;
	gint last_selected;
	gchar *drag_uri;
	gint last_x;
	gint last_y;
	gboolean loop_file;
	gboolean loop_playlist;
	gboolean shuffle;
	GtkCssProvider *css_provider;
	GtkWidget *search_bar;
	GtkWidget *search_entry;
	GtkWidget *placeholder;
	GtkWidget *scrolled_window;
	GtkWidget *list_box;
	GtkWidget *toolbar_view;
	GtkWidget *header_box;
	GtkWidget *top_bar;
	GtkWidget *bottom_bar;
	GtkWidget *search_button;
	GtkWidget *select_button;
	GtkWidget *collapse_button;
	GtkWidget *title_box;
	GtkWidget *item_count;
	GtkWidget *playlist_consecutive;
	GtkWidget *loop1_btn;
	GtkWidget *loop_list_btn;
	GtkWidget *shuffle_btn;
};

struct _CelluloidPlaylistWidgetClass
{
	AdwBin parent_class;
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

G_DEFINE_TYPE(CelluloidPlaylistWidget, celluloid_playlist_widget, ADW_TYPE_BIN)

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

	if(len > 0)
	{
		gboolean done = FALSE;

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
				g_list_model_get_item
				(G_LIST_MODEL(wgt->model), i);
			const gchar *title =
				celluloid_playlist_item_get_title(item);

			found = g_str_match_string(term, title, TRUE);
			done = found || (!match_current && i == initial_index);
			match_current = FALSE;
		}
		while(!done);
	}

	if(found)
	{
		select_index(wgt, (gint)i);
	}
}

static gchar * 
get_length(const gchar *filename)
{
    AVFormatContext *fmt_ctx = NULL;
    gint64 duration;
    gint hours, mins, secs;
    static gchar duration_str[50];

    if (avformat_open_input(&fmt_ctx, filename, NULL, NULL) != 0) {
        return "Error: Cannot open file";
    }

    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        avformat_close_input(&fmt_ctx);
        return "Error: Cannot read stream info";
    }

    duration = (gint64)(fmt_ctx->duration / AV_TIME_BASE);
    hours = (gint)(duration / 3600);
    mins = (gint)((duration % 3600) / 60);
    secs = (gint)(duration % 60);

    snprintf(duration_str, sizeof(duration_str), "%02d:%02d:%02d", hours, mins, secs);

    avformat_close_input(&fmt_ctx);
    return duration_str;
}

static GtkWidget *
make_row(GObject *object, gpointer data)
{
	GtkWidget *row = gtk_list_box_row_new();
	CelluloidPlaylistItem *item = CELLULOID_PLAYLIST_ITEM(object);
	const gchar *title = celluloid_playlist_item_get_title(item);
	const gchar *uri = celluloid_playlist_item_get_uri(item);
	GtkWidget *label = NULL;
	GtkWidget *length_label = NULL;
	GtkWidget *spacer = NULL;
	GtkWidget *hbox = NULL;
	gchar *text = NULL;
	gchar *length = NULL;

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
	gtk_widget_set_tooltip_text(label, text);
	g_free(text);

	g_assert(label);

	if(celluloid_playlist_item_get_is_current(item))
	{
		gtk_widget_add_css_class(label, "heading");
	}

	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_widget_set_margin_top(label, 12);
	gtk_widget_set_margin_bottom(label, 12);
	gtk_label_set_max_width_chars(GTK_LABEL(label), 40);
	gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_MIDDLE);
	
	length = get_length(uri);
	length_label = gtk_label_new(length);
	gtk_widget_set_tooltip_text(length_label, length);

	g_assert(length_label);

	gtk_widget_set_halign(length_label, GTK_ALIGN_START);
	gtk_widget_set_margin_top(length_label, 12);
	gtk_widget_set_margin_bottom(length_label, 12);
	gtk_label_set_max_width_chars(GTK_LABEL(length_label), 40);
	gtk_label_set_ellipsize(GTK_LABEL(length_label), PANGO_ELLIPSIZE_MIDDLE);
	
	spacer = gtk_label_new(NULL);
	gtk_widget_set_hexpand(spacer, TRUE);
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_box_append(GTK_BOX(hbox), label);
	gtk_box_append(GTK_BOX(hbox), spacer);
	gtk_box_append(GTK_BOX(hbox), length_label);
	gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), hbox);
	return row;
}

static void
constructed(GObject *object)
{
	CelluloidPlaylistWidget *self = CELLULOID_PLAYLIST_WIDGET(object);

	self->model = celluloid_playlist_model_new();
	self->list_box = gtk_list_box_new();
	self->last_selected = -1;
	self->drag_uri = NULL;
	self->toolbar_view = adw_toolbar_view_new();

	gtk_list_box_set_selection_mode
		(GTK_LIST_BOX(self->list_box), GTK_SELECTION_BROWSE);
	gtk_list_box_set_activate_on_single_click
		(GTK_LIST_BOX(self->list_box), TRUE);
	gtk_list_box_set_placeholder
		(GTK_LIST_BOX(self->list_box), self->placeholder);
	gtk_widget_add_css_class
		(self->list_box, "navigation-sidebar");

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

	GtkViewport *viewport = GTK_VIEWPORT(gtk_viewport_new(NULL, NULL));
	gtk_viewport_set_child(viewport, self->list_box);
	gtk_viewport_set_scroll_to_focus(viewport, FALSE);

	gtk_scrolled_window_set_child
		(	GTK_SCROLLED_WINDOW(self->scrolled_window),
			 GTK_WIDGET(viewport) );
	gtk_search_bar_set_child(GTK_SEARCH_BAR
		(self->search_bar), self->search_entry);

	G_OBJECT_CLASS(celluloid_playlist_widget_parent_class)
		->constructed(object);
}

static void
set_property(	GObject *object,
		guint property_id,
		const GValue *value,
		GParamSpec *pspec )
{
	CelluloidPlaylistWidget *self = CELLULOID_PLAYLIST_WIDGET(object);
	switch(property_id)
	{
		case PROP_PLAYLIST_COUNT:
		self->playlist_count = g_value_get_int64(value);
		break;

		case PROP_SEARCHING:
		self->searching = g_value_get_boolean(value);
		if(self->searching)
		{
			gtk_widget_grab_focus(self->search_entry);
		}
		break;

		case PROP_LOOP_FILE:
		self->loop_file = g_value_get_boolean(value);
		break;

		case PROP_LOOP_PLAYLIST:
		self->loop_playlist = g_value_get_boolean(value);
		break;

		case PROP_SHUFFLE:
		self->shuffle = g_value_get_boolean(value);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
get_property(	GObject *object,
		guint property_id,
		GValue *value,
		GParamSpec *pspec )
{
	CelluloidPlaylistWidget *self = CELLULOID_PLAYLIST_WIDGET(object);
	switch(property_id)
	{
		case PROP_PLAYLIST_COUNT:
		g_value_set_int64(value, self->playlist_count);
		break;

		case PROP_SEARCHING:
		g_value_set_boolean(value, self->searching);
		break;

		case PROP_LOOP_FILE:
		g_value_set_boolean(value, self->loop_file);
		break;

		case PROP_LOOP_PLAYLIST:
		g_value_set_boolean(value, self->loop_playlist);
		break;

		case PROP_SHUFFLE:
		g_value_set_boolean(value, self->shuffle);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
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
				CELLULOID_MENU_ITEM(_("Loop File"), "win.toggle-loop-file"),
				CELLULOID_MENU_ITEM(_("Loop Playlist"), "win.toggle-loop-playlist"),
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
		gtk_widget_set_visible(ctx_menu, TRUE);
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

	gtk_widget_class_set_css_name(GTK_WIDGET_CLASS(klass), "playlist");

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

	pspec = g_param_spec_boolean
		(	"loop-file",
			"Loop file",
			"Loop a single media file",
			FALSE,
			G_PARAM_READWRITE );
	g_object_class_install_property(obj_class, PROP_LOOP_FILE, pspec);

	pspec = g_param_spec_boolean
		(	"loop-playlist",
			"Loop playlist",
			"Loop current playlist",
			FALSE,
			G_PARAM_READWRITE );
	g_object_class_install_property(obj_class, PROP_LOOP_PLAYLIST, pspec);

	pspec = g_param_spec_boolean
		(	"shuffle",
			"Shuffle playlist",
			"Shuffle current playlist",
			FALSE,
			G_PARAM_READWRITE );
	g_object_class_install_property(obj_class, PROP_SHUFFLE, pspec);

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
	wgt->playlist_count = 0;
	wgt->searching = FALSE;
	wgt->loop_file = FALSE;
	wgt->loop_playlist = FALSE;
	wgt->shuffle = FALSE;
	wgt->scrolled_window = gtk_scrolled_window_new();
	wgt->search_bar = gtk_search_bar_new();
	wgt->search_entry = gtk_search_entry_new();
	wgt->placeholder = adw_status_page_new();
	wgt->css_provider = gtk_css_provider_new();
	wgt->toolbar_view = adw_toolbar_view_new();
	wgt->top_bar = adw_header_bar_new();
	wgt->collapse_button = gtk_button_new_from_icon_name("go-next-symbolic");
	wgt->title_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	wgt->search_button = gtk_toggle_button_new();
	wgt->bottom_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	wgt->playlist_consecutive = gtk_toggle_button_new();
	wgt->item_count = gtk_label_new(_("0 files"));
	wgt->loop1_btn = gtk_toggle_button_new();

	adw_header_bar_set_show_end_title_buttons
		(ADW_HEADER_BAR(wgt->top_bar), FALSE);
	adw_header_bar_pack_start
		(ADW_HEADER_BAR(wgt->top_bar), wgt->collapse_button);

	gtk_actionable_set_action_name
		(GTK_ACTIONABLE(wgt->collapse_button), "win.toggle-playlist");
	gtk_widget_set_tooltip_text
		(wgt->collapse_button, _("Hide playlist"));

	gtk_box_append
		(GTK_BOX(wgt->title_box), gtk_label_new(_("Playlist")));
	gtk_widget_add_css_class
		(gtk_widget_get_first_child(wgt->title_box), "heading");

	gtk_widget_set_halign(wgt->item_count, GTK_ALIGN_START);
	gtk_box_append(GTK_BOX(wgt->title_box), wgt->item_count);
	gtk_widget_add_css_class(wgt->item_count, "caption");

	adw_header_bar_pack_start
		(ADW_HEADER_BAR(wgt->top_bar), wgt->title_box);
	adw_header_bar_pack_end
		(ADW_HEADER_BAR(wgt->top_bar), wgt->search_button);
	adw_header_bar_set_show_title
		(ADW_HEADER_BAR(wgt->top_bar), FALSE);

	gtk_widget_set_margin_top(wgt->top_bar, 6);
	gtk_widget_set_margin_bottom(wgt->top_bar, 6);
	gtk_widget_set_margin_start(wgt->top_bar, 6);
	gtk_widget_set_margin_end(wgt->top_bar, 6);

	gtk_button_set_icon_name
		(GTK_BUTTON(wgt->search_button), "system-search");
	g_object_bind_property
		(	wgt->search_bar, "search-mode-enabled",
			wgt->search_button, "active",
			G_BINDING_BIDIRECTIONAL );
	gtk_widget_set_tooltip_text
		(wgt->search_button, _("Search in playlist"));

	gtk_search_entry_set_placeholder_text
		(GTK_SEARCH_ENTRY(wgt->search_entry), _("Search in playlist…"));

	gtk_widget_set_halign(wgt->bottom_bar, GTK_ALIGN_CENTER);
	gtk_widget_add_css_class(wgt->bottom_bar, "toolbar");

	gtk_button_set_icon_name
		(	GTK_BUTTON(wgt->playlist_consecutive),
			"media-playlist-consecutive-symbolic" );

	gtk_box_append(GTK_BOX(wgt->bottom_bar), wgt->playlist_consecutive);

	gtk_toggle_button_set_active
		(GTK_TOGGLE_BUTTON(wgt->playlist_consecutive),TRUE);
	gtk_widget_set_tooltip_text
		(wgt->playlist_consecutive, _("Disable repeat"));

	gtk_button_set_icon_name
		(GTK_BUTTON(wgt->loop1_btn), "media-playlist-repeat-song-symbolic");
	gtk_box_append
		(GTK_BOX(wgt->bottom_bar), wgt->loop1_btn);
	gtk_toggle_button_set_group
		(	GTK_TOGGLE_BUTTON(wgt->loop1_btn),
			GTK_TOGGLE_BUTTON(wgt->playlist_consecutive) );
	gtk_widget_set_tooltip_text
		(wgt->loop1_btn, _("Repeat file"));

	wgt->loop_list_btn = gtk_toggle_button_new();
	gtk_button_set_icon_name
		(GTK_BUTTON(wgt->loop_list_btn), "media-playlist-repeat-symbolic");
	gtk_box_append
		(GTK_BOX(wgt->bottom_bar), wgt->loop_list_btn);
	gtk_toggle_button_set_group
		(	GTK_TOGGLE_BUTTON(wgt->loop_list_btn),
			GTK_TOGGLE_BUTTON(wgt->playlist_consecutive) );
	gtk_widget_set_tooltip_text
		(wgt->loop_list_btn, _("Repeat playlist"));

	wgt->shuffle_btn = gtk_toggle_button_new();
	gtk_button_set_icon_name
		(GTK_BUTTON(wgt->shuffle_btn), "media-playlist-shuffle-symbolic");
	gtk_box_append
		(GTK_BOX(wgt->bottom_bar), wgt->shuffle_btn);
	gtk_widget_set_tooltip_text
		(wgt->shuffle_btn, _("Shuffle playlist"));

	wgt->scrolled_window = gtk_scrolled_window_new();
	wgt->header_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_append(GTK_BOX(wgt->header_box), wgt->top_bar);
	gtk_box_append(GTK_BOX(wgt->header_box), wgt->search_bar);

	adw_bin_set_child
		(ADW_BIN(wgt), wgt->toolbar_view);
	adw_toolbar_view_add_top_bar
		(ADW_TOOLBAR_VIEW(wgt->toolbar_view), wgt->header_box);
	adw_toolbar_view_add_bottom_bar
		(ADW_TOOLBAR_VIEW(wgt->toolbar_view), wgt->bottom_bar);
	adw_toolbar_view_set_content
		(ADW_TOOLBAR_VIEW(wgt->toolbar_view), wgt->scrolled_window);

	gchar *css_data =
		"playlist .icon { -gtk-icon-size: 64px; }\n"
		"playlist .title { font-weight: normal; font-size: medium; }\n";

	gtk_css_provider_load_from_data(wgt->css_provider, css_data, -1);
	gtk_widget_add_css_class(wgt->placeholder, "dim-label");

	adw_status_page_set_title
		(	ADW_STATUS_PAGE(wgt->placeholder),
			_("Playlist is Empty") );
	adw_status_page_set_icon_name
		(	ADW_STATUS_PAGE(wgt->placeholder),
			"applications-multimedia-symbolic" );

	g_object_bind_property(	wgt, "loop-file",
				wgt->loop1_btn, "active",
				G_BINDING_BIDIRECTIONAL );
	g_object_bind_property(	wgt, "loop-playlist",
				wgt->loop_list_btn, "active",
				G_BINDING_BIDIRECTIONAL );
	g_object_bind_property(	wgt, "shuffle",
				wgt->shuffle_btn, "active",
				G_BINDING_BIDIRECTIONAL );
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
	select_index
		(wgt, MIN(wgt->last_selected, (gint)playlist->len -1));

	wgt->playlist_count = playlist->len;
	g_object_notify(G_OBJECT(wgt), "playlist-count");

	const gulong playlist_count =
		(gulong)wgt->playlist_count;
	gchar *label_text =
		g_strdup_printf
		(	ngettext("%ld file", "%ld files", playlist_count),
			playlist_count );

	gtk_label_set_label(GTK_LABEL(wgt->item_count), label_text);

	g_free(label_text);
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
