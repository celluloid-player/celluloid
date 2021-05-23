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

#include <gio/gio.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>

#include "celluloid-preferences-dialog.h"
#include "celluloid-file-chooser-button.h"
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
file_set_handler(CelluloidFileChooserButton *button, gpointer data)
{
	GtkRoot *root = gtk_widget_get_root(GTK_WIDGET(button));
	CelluloidPreferencesDialog *self = CELLULOID_PREFERENCES_DIALOG(root);
	const gchar *key = data;
	GFile *file = celluloid_file_chooser_button_get_file(button);
	gchar *uri = g_file_get_uri(file) ?: g_strdup("");

	g_settings_set_string(self->settings, key, uri);

	g_free(uri);
	g_object_unref(file);
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
key_pressed_handler(	GtkEventControllerKey *controller,
			guint keyval,
			guint keycode,
			GdkModifierType state,
			gpointer data )
{
	const guint mod_mask =	GDK_MODIFIER_MASK
				&~(GDK_SHIFT_MASK
				|GDK_LOCK_MASK
				|GDK_ALT_MASK
				|GDK_SUPER_MASK
				|GDK_HYPER_MASK
				|GDK_META_MASK);

	if((state&mod_mask) == 0 && keyval == GDK_KEY_Return)
	{
		gtk_dialog_response(GTK_DIALOG(data), GTK_RESPONSE_ACCEPT);
	}

	return FALSE;
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
	GSettingsSchema *schema = NULL;

	gtk_widget_set_margin_end(grid, 12);
	gtk_widget_set_margin_top(grid, 12);
	gtk_widget_set_margin_bottom(grid, 12);

	gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 12);

	g_object_get(settings, "settings-schema", &schema, NULL);

	for(gint i = 0; items[i].type != ITEM_TYPE_INVALID; i++)
	{
		const gchar *key = items[i].key;
		GSettingsSchemaKey *schema_key =
			key ?
			g_settings_schema_get_key(schema, key) :
			NULL;
		const gchar *summary =
			schema_key ?
			g_settings_schema_key_get_summary(schema_key) :
			NULL;
		const gchar *label = items[i].label ?: summary;
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
			CelluloidFileChooserButton *button;
			GtkFileFilter *filter;
			gchar *uri;
			GFile *file;

			widget =	celluloid_file_chooser_button_new
					(NULL, GTK_FILE_CHOOSER_ACTION_OPEN);

			button = CELLULOID_FILE_CHOOSER_BUTTON(widget);
			filter = gtk_file_filter_new();
			uri = g_settings_get_string(settings, key);
			file = g_file_new_for_uri(uri);

			separate_label = TRUE;
			xpos = 1;

			gtk_widget_set_hexpand(widget, TRUE);
			gtk_widget_set_size_request(widget, 100, -1);

			gtk_file_filter_add_mime_type(filter, "text/plain");
			celluloid_file_chooser_button_set_filter(button, filter);

			if(g_file_query_exists(file, NULL))
			{
				celluloid_file_chooser_button_set_file
					(button, file, NULL);
			}

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

			g_free(uri);
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
		if(separate_label && label && !label[0])
		{
			width = 2;
			xpos = 0;
		}

		gtk_grid_attach(GTK_GRID(grid), widget, xpos, i, width, 1);

		if(separate_label && label && label[0])
		{
			GtkWidget *label_widget = gtk_label_new(label);

			/* The grid should only have 2 columns, so the previous
			 * widget connot be wider than 1 column if it needs a
			 * separate label.
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

	GTK_DIALOG_CLASS(klass)->response = response_handler;
	G_OBJECT_CLASS(klass)->constructed = preferences_dialog_constructed;

	gtk_widget_class_set_css_name(wid_class, "celluloid-preferences-dialog");
}

static void
celluloid_preferences_dialog_init(CelluloidPreferencesDialog *dlg)
{
	const PreferencesDialogItem interface_items[]
		= {	{NULL,
			"autofit-enable",
			ITEM_TYPE_CHECK_BOX},
			{NULL,
			"csd-enable",
			ITEM_TYPE_CHECK_BOX},
			{NULL,
			"dark-theme-enable",
			ITEM_TYPE_CHECK_BOX},
			{NULL,
			"always-use-floating-controls",
			ITEM_TYPE_CHECK_BOX},
			{NULL,
			"always-autohide-cursor",
			ITEM_TYPE_CHECK_BOX},
			{NULL,
			"use-skip-buttons-for-playlist",
			ITEM_TYPE_CHECK_BOX},
			{NULL,
			"last-folder-enable",
			ITEM_TYPE_CHECK_BOX},
			{NULL, NULL, ITEM_TYPE_INVALID} };
	const PreferencesDialogItem config_items[]
		= {	{NULL,
			"mpv-config-enable",
			ITEM_TYPE_CHECK_BOX},
			{_("mpv configuration file:"),
			"mpv-config-file",
			ITEM_TYPE_FILE_CHOOSER},
			{NULL,
			"mpv-input-config-enable",
			ITEM_TYPE_CHECK_BOX},
			{_("mpv input configuration file:"),
			"mpv-input-config-file",
			ITEM_TYPE_FILE_CHOOSER},
			{NULL, NULL, ITEM_TYPE_INVALID} };
	const PreferencesDialogItem misc_items[]
		= {	{NULL,
			"always-open-new-window",
			ITEM_TYPE_CHECK_BOX},
			{NULL,
			"always-append-to-playlist",
			ITEM_TYPE_CHECK_BOX},
			{NULL,
			"ignore-playback-errors",
			ITEM_TYPE_CHECK_BOX},
			{NULL,
			"prefetch-metadata",
			ITEM_TYPE_CHECK_BOX},
			{NULL,
			"mpris-enable",
			ITEM_TYPE_CHECK_BOX},
			{NULL,
			"media-keys-enable",
			ITEM_TYPE_CHECK_BOX},
			{_("Extra mpv options:"),
			NULL,
			ITEM_TYPE_LABEL},
			{"",
			"mpv-options",
			ITEM_TYPE_TEXT_BOX},
			{NULL, NULL, ITEM_TYPE_INVALID} };

	GtkWidget *content_area;

	dlg->settings = g_settings_new(CONFIG_ROOT);
	dlg->notebook = gtk_notebook_new();
	content_area = gtk_dialog_get_content_area(GTK_DIALOG(dlg));

	g_settings_delay(dlg->settings);

	gtk_box_append(GTK_BOX(content_area), dlg->notebook);

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
				GTK_RESPONSE_CANCEL,
				_("_Save"),
				GTK_RESPONSE_ACCEPT,
				NULL );

	gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_ACCEPT);

	GtkEventController *key_controller = gtk_event_controller_key_new();

	gtk_widget_add_controller(GTK_WIDGET(dlg), key_controller);

	g_signal_connect(	key_controller,
				"key-pressed",
				G_CALLBACK(key_pressed_handler),
				dlg );
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
		gtk_header_bar_set_show_title_buttons
			(GTK_HEADER_BAR(header_bar), FALSE);
	}

	gtk_window_set_hide_on_close(GTK_WINDOW(dlg), TRUE);
	gtk_widget_show(dlg);

	return dlg;
}
