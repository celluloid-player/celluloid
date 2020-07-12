/*
 * Copyright (c) 2014-2020 gnome-mpv
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

#include <gio/gio.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>

#include "celluloid-preferences-dialog.h"
#include "celluloid-plugins-manager.h"
#include "celluloid-main-window.h"
#include "celluloid-def.h"

typedef struct PreferencesDialogItem PreferencesDialogItem;
typedef enum PreferencesDialogItemType PreferencesDialogItemType;

struct _CelluloidPreferencesDialog
{
	GtkDialog parent_instance;
	GSettings *settings;
	GtkWidget *notebook;
};

struct _CelluloidPreferencesDialogClass
{
	GtkDialogClass parent_class;
};

enum PreferencesDialogItemType
{
	ITEM_TYPE_INVALID,
	ITEM_TYPE_GROUP,
	ITEM_TYPE_CHECK_BOX,
	ITEM_TYPE_FILE_CHOOSER,
	ITEM_TYPE_LABEL,
	ITEM_TYPE_TEXT_BOX
};

struct PreferencesDialogItem
{
	const gchar *label;
	const gchar *key;
	PreferencesDialogItemType type;
};

G_DEFINE_TYPE(CelluloidPreferencesDialog, celluloid_preferences_dialog, GTK_TYPE_DIALOG)

static void
file_set_handler(GtkFileChooserButton *widget, gpointer data)
{
	GtkWidget *toplevel = gtk_widget_get_toplevel(GTK_WIDGET(widget));
	CelluloidPreferencesDialog *dlg = CELLULOID_PREFERENCES_DIALOG(toplevel);
	GtkFileChooser *chooser = GTK_FILE_CHOOSER(widget);
	const gchar *key = data;
	gchar *filename = gtk_file_chooser_get_filename(chooser)?:g_strdup("");

	g_settings_set_string(dlg->settings, key, filename);

	g_free(filename);
}

static void
response_handler(GtkDialog *dialog, gint response_id)
{
	CelluloidPreferencesDialog *dlg = CELLULOID_PREFERENCES_DIALOG(dialog);

	if(response_id == GTK_RESPONSE_ACCEPT)
	{
		g_settings_apply(dlg->settings);
	}
	else
	{
		g_settings_revert(dlg->settings);
	}

	g_object_unref(dlg->settings);
}

static gboolean
key_press_handler(GtkWidget *widget, GdkEventKey *event)
{
	guint keyval = event->keyval;
	guint state = event->state;

	const guint mod_mask =	GDK_MODIFIER_MASK
				&~(GDK_SHIFT_MASK
				|GDK_LOCK_MASK
				|GDK_MOD2_MASK
				|GDK_MOD3_MASK
				|GDK_MOD4_MASK
				|GDK_MOD5_MASK);

	if((state&mod_mask) == 0 && keyval == GDK_KEY_Return)
	{
		gtk_dialog_response(GTK_DIALOG(widget), GTK_RESPONSE_ACCEPT);
	}

	return	GTK_WIDGET_CLASS(celluloid_preferences_dialog_parent_class)
		->key_press_event(widget, event);
}

static void
free_signal_data(gpointer data, GClosure *closure)
{
	g_free(data);
}

static GtkWidget *
build_page(const PreferencesDialogItem *items, GSettings *settings)
{
	GtkWidget *grid = gtk_grid_new();

	gtk_container_set_border_width(GTK_CONTAINER(grid), 12);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 12);

	for(gint i = 0; items[i].type != ITEM_TYPE_INVALID; i++)
	{
		const gchar *label = items[i].label;
		const gchar *key = items[i].key;
		const PreferencesDialogItemType type = items[i].type;
		GtkWidget *widget = NULL;
		gboolean separate_label = FALSE;
		gint width = 1;
		gint xpos = 0;

		if(type == ITEM_TYPE_GROUP)
		{
			widget = gtk_label_new(label);
			width = 2;

			gtk_label_set_use_markup(GTK_LABEL(widget), TRUE);
			gtk_widget_set_halign(widget, GTK_ALIGN_START);
			gtk_widget_set_margin_top(widget, 12);
		}
		else if(type == ITEM_TYPE_CHECK_BOX)
		{
			widget = gtk_check_button_new_with_label(label);
			width = 2;

			g_settings_bind(	settings,
						key,
						widget,
						"active",
						G_SETTINGS_BIND_DEFAULT );
		}
		else if(type == ITEM_TYPE_FILE_CHOOSER)
		{
			GtkFileChooser *chooser;
			GtkFileFilter *filter;
			gchar *filename;

			widget = gtk_file_chooser_button_new
					(label, GTK_FILE_CHOOSER_ACTION_OPEN);
			chooser = GTK_FILE_CHOOSER(widget);
			filter = gtk_file_filter_new();
			filename = g_settings_get_string(settings, key);
			separate_label = TRUE;
			xpos = 1;

			gtk_file_filter_add_mime_type(filter, "text/plain");
			gtk_file_chooser_set_filter(chooser, filter);

			gtk_widget_set_hexpand(widget, TRUE);
			gtk_widget_set_size_request(widget, 100, -1);
			gtk_file_chooser_set_filename(chooser, filename);

			/* For simplicity, changes made to the GSettings
			 * database externally won't be reflected immediately
			 * for this type of widget.
			 */
			g_signal_connect_data(	widget,
						"file-set",
						G_CALLBACK(file_set_handler),
						g_strdup(key),
						free_signal_data,
						0 );

			g_free(filename);
		}
		else if(type == ITEM_TYPE_TEXT_BOX)
		{
			widget = gtk_entry_new();
			separate_label = TRUE;
			xpos = 1;

			gtk_widget_set_hexpand(widget, TRUE);

			g_settings_bind(	settings,
						key,
						widget,
						"text",
						G_SETTINGS_BIND_DEFAULT );
		}
		else if(type == ITEM_TYPE_LABEL)
		{
			widget = gtk_label_new(label);

			gtk_widget_set_halign(widget, GTK_ALIGN_START);
		}

		g_assert(widget);
		g_assert(xpos == 0 || xpos == 1);

		if(i == 0)
		{
			gtk_widget_set_margin_top(widget, 0);
		}

		if(type != ITEM_TYPE_GROUP)
		{
			gtk_widget_set_margin_start(widget, 12);
		}

		/* Expand the widget to fill both columns if it usually needs a
		 * separate label but none is provided.
		 */
		if(separate_label && !label)
		{
			width = 2;
			xpos = 0;
		}

		gtk_grid_attach(GTK_GRID(grid), widget, xpos, i, width, 1);

		if(separate_label && label)
		{
			GtkWidget *label_widget = gtk_label_new(label);

			/* The grid should only have 2 columns, so the previous
			 * widget connot be wider than 1 column if it needs a
			 * seperate label.
			 */
			g_assert(width == 1);

			gtk_grid_attach(	GTK_GRID(grid),
						label_widget,
						1-xpos, i, 1, 1 );

			gtk_widget_set_halign(label_widget, GTK_ALIGN_START);
			gtk_widget_set_hexpand(label_widget, FALSE);
			gtk_widget_set_margin_start(label_widget, 12);
		}
	}

	return grid;
}

static void
preferences_dialog_constructed(GObject *obj)
{
	gboolean csd_enabled;

	g_object_get(obj, "use-header-bar", &csd_enabled, NULL);

	if(!csd_enabled)
	{
		GtkWidget *content_area;
		GtkWidget *notebook;

		content_area = gtk_dialog_get_content_area(GTK_DIALOG(obj));
		notebook = CELLULOID_PREFERENCES_DIALOG(obj)->notebook;

		gtk_widget_set_margin_bottom(content_area, 12);
		gtk_widget_set_margin_bottom(notebook, 12);
	}

	G_OBJECT_CLASS(celluloid_preferences_dialog_parent_class)->constructed(obj);
}

static void
celluloid_preferences_dialog_class_init(CelluloidPreferencesDialogClass *klass)
{
	GtkWidgetClass *wid_class = GTK_WIDGET_CLASS(klass);

	wid_class->key_press_event = key_press_handler;
	GTK_DIALOG_CLASS(klass)->response = response_handler;
	G_OBJECT_CLASS(klass)->constructed = preferences_dialog_constructed;

	gtk_widget_class_set_css_name(wid_class, "celluloid-preferences-dialog");
}

static void
celluloid_preferences_dialog_init(CelluloidPreferencesDialog *dlg)
{
	const PreferencesDialogItem interface_items[]
		= {	{_("Automatically resize window to fit video"),
			"autofit-enable",
			ITEM_TYPE_CHECK_BOX},
			{_("Enable client-side decorations"),
			"csd-enable",
			ITEM_TYPE_CHECK_BOX},
			{_("Enable dark theme"),
			"dark-theme-enable",
			ITEM_TYPE_CHECK_BOX},
			{_("Use floating controls in windowed mode"),
			"always-use-floating-controls",
			ITEM_TYPE_CHECK_BOX},
			{_("Autohide mouse cursor in windowed mode"),
			"always-autohide-cursor",
			ITEM_TYPE_CHECK_BOX},
			{_("Use skip buttons for controlling playlist"),
			"use-skip-buttons-for-playlist",
			ITEM_TYPE_CHECK_BOX},
			{_("Remember last file's location"),
			"last-folder-enable",
			ITEM_TYPE_CHECK_BOX},
			{NULL, NULL, ITEM_TYPE_INVALID} };
	const PreferencesDialogItem config_items[]
		= {	{_("Load MPV configuration file"),
			"mpv-config-enable",
			ITEM_TYPE_CHECK_BOX},
			{_("MPV configuration file:"),
			"mpv-config-file",
			ITEM_TYPE_FILE_CHOOSER},
			{_("Load MPV input configuration file"),
			"mpv-input-config-enable",
			ITEM_TYPE_CHECK_BOX},
			{_("MPV input configuration file:"),
			"mpv-input-config-file",
			ITEM_TYPE_FILE_CHOOSER},
			{NULL, NULL, ITEM_TYPE_INVALID} };
	const PreferencesDialogItem misc_items[]
		= {	{_("Always open new window"),
			"always-open-new-window",
			ITEM_TYPE_CHECK_BOX},
			{_("Ignore playback errors"),
			"ignore-playback-errors",
			ITEM_TYPE_CHECK_BOX},
			{_("Prefetch metadata"),
			"prefetch-metadata",
			ITEM_TYPE_CHECK_BOX},
			{_("Enable MPRIS support"),
			"mpris-enable",
			ITEM_TYPE_CHECK_BOX},
			{_("Enable media keys support"),
			"media-keys-enable",
			ITEM_TYPE_CHECK_BOX},
			{_("Extra MPV options:"),
			NULL,
			ITEM_TYPE_LABEL},
			{NULL,
			"mpv-options",
			ITEM_TYPE_TEXT_BOX},
			{NULL, NULL, ITEM_TYPE_INVALID} };

	GtkWidget *content_area;
	GdkGeometry geom;

	/* This 'locks' the height of the dialog while allowing the width to be
	 * freely adjusted.
	 */
	geom.max_width = G_MAXINT;
	geom.max_height = 0;

	dlg->settings = g_settings_new(CONFIG_ROOT);
	dlg->notebook = gtk_notebook_new();
	content_area = gtk_dialog_get_content_area(GTK_DIALOG(dlg));

	g_settings_delay(dlg->settings);

	gtk_window_set_geometry_hints(	GTK_WINDOW(dlg),
					GTK_WIDGET(dlg),
					&geom,
					GDK_HINT_MAX_SIZE );

	gtk_container_set_border_width(GTK_CONTAINER(content_area), 0);
	gtk_container_add(GTK_CONTAINER(content_area), dlg->notebook);

	gtk_notebook_append_page(	GTK_NOTEBOOK(dlg->notebook),
					build_page(interface_items, dlg->settings),
					gtk_label_new(_("Interface")) );
	gtk_notebook_append_page(	GTK_NOTEBOOK(dlg->notebook),
					build_page(config_items, dlg->settings),
					gtk_label_new(_("Config Files")) );
	gtk_notebook_append_page(	GTK_NOTEBOOK(dlg->notebook),
					build_page(misc_items, dlg->settings),
					gtk_label_new(_("Miscellaneous")) );
	gtk_notebook_append_page(	GTK_NOTEBOOK(dlg->notebook),
					celluloid_plugins_manager_new(GTK_WINDOW(dlg)),
					gtk_label_new(_("Plugins")) );

	gtk_dialog_add_buttons(	GTK_DIALOG(dlg),
				_("_Cancel"),
				GTK_RESPONSE_REJECT,
				_("_Save"),
				GTK_RESPONSE_ACCEPT,
				NULL );

	gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_ACCEPT);

}

GtkWidget *
celluloid_preferences_dialog_new(GtkWindow *parent)
{
	GtkWidget *dlg;
	GtkWidget *header_bar;
	gboolean csd_enabled;

	csd_enabled = celluloid_main_window_get_csd_enabled(CELLULOID_MAIN_WINDOW(parent));

	dlg = g_object_new(	celluloid_preferences_dialog_get_type(),
				"title", _("Preferences"),
				"modal", TRUE,
				"transient-for", parent,
				"use-header-bar", csd_enabled,
				NULL );

	header_bar = gtk_dialog_get_header_bar(GTK_DIALOG(dlg));

	if(header_bar)
	{
		/* The defaults use PACK_END which is ugly with multiple buttons
		 */
		GtkWidget *cancel_btn = gtk_dialog_get_widget_for_response
					(GTK_DIALOG(dlg), GTK_RESPONSE_REJECT);

		gtk_container_child_set(	GTK_CONTAINER(header_bar),
						cancel_btn,
						"pack-type",
						GTK_PACK_START,
						NULL );

		gtk_header_bar_set_show_close_button
			(GTK_HEADER_BAR(header_bar), FALSE);
	}

	gtk_widget_hide_on_delete(dlg);
	gtk_widget_show_all(dlg);

	return dlg;
}
