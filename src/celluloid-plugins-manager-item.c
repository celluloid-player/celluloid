/*
 * Copyright (c) 2016-2021 gnome-mpv
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

#include <gdk/gdk.h>
#include <gio/gio.h>
#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <adwaita.h>

#include "celluloid-plugins-manager-item.h"
#include "celluloid-common.h"

enum
{
	PROP_0,
	PROP_PARENT,
	PROP_PATH,
	N_PROPERTIES
};

struct _CelluloidPluginsManagerItem
{
	AdwActionRow parent;
	GtkWindow *parent_window;
	gchar *path;
};

struct _CelluloidPluginsManagerItemClass
{
	AdwActionRowClass parent_class;
};

G_DEFINE_TYPE(CelluloidPluginsManagerItem, celluloid_plugins_manager_item, ADW_TYPE_ACTION_ROW)

static void
celluloid_plugins_manager_item_constructed(GObject *object);

static void
celluloid_plugins_manager_item_finalize(GObject *object);

static void
celluloid_plugins_manager_item_set_property(	GObject *object,
						guint property_id,
						const GValue *value,
						GParamSpec *pspec );

static void
celluloid_plugins_manager_item_get_property(	GObject *object,
						guint property_id,
						GValue *value,
						GParamSpec *pspec );

static void
remove_handler(GtkButton *button, gpointer data);

static void
celluloid_plugins_manager_item_constructed(GObject *object)
{
	CelluloidPluginsManagerItem *self =
		CELLULOID_PLUGINS_MANAGER_ITEM(object);
	GtkWidget *remove_button =
		gtk_button_new_from_icon_name("user-trash-symbolic");

	gtk_widget_add_css_class(remove_button, "flat");
	gtk_widget_set_valign(remove_button, GTK_ALIGN_CENTER);
	gtk_widget_set_tooltip_text(remove_button, _("Remove Plugin"));

	g_signal_connect(	remove_button,
				"clicked",
				G_CALLBACK(remove_handler),
				self );

	adw_action_row_add_suffix(ADW_ACTION_ROW (self), remove_button);

	G_OBJECT_CLASS(celluloid_plugins_manager_item_parent_class)
		->constructed(object);
}

static void
celluloid_plugins_manager_item_finalize(GObject *object)
{
	CelluloidPluginsManagerItem *self = CELLULOID_PLUGINS_MANAGER_ITEM(object);

	g_free(self->path);

	G_OBJECT_CLASS(celluloid_plugins_manager_item_parent_class)
		->finalize(object);
}

static void
celluloid_plugins_manager_item_set_property(	GObject *object,
						guint property_id,
						const GValue *value,
						GParamSpec *pspec )
{
	CelluloidPluginsManagerItem *self = CELLULOID_PLUGINS_MANAGER_ITEM(object);

	if(property_id == PROP_PARENT)
	{
		self->parent_window = g_value_get_pointer(value);
	}
	else if(property_id == PROP_PATH)
	{
		g_free(self->path);

		self->path = g_value_dup_string(value);
	}
	else
	{
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void
celluloid_plugins_manager_item_get_property(	GObject *object,
						guint property_id,
						GValue *value,
						GParamSpec *pspec )
{
	CelluloidPluginsManagerItem *self = CELLULOID_PLUGINS_MANAGER_ITEM(object);

	if(property_id == PROP_PARENT)
	{
		g_value_set_pointer(value, self->parent_window);
	}
	else if(property_id == PROP_PATH)
	{
		g_value_set_string(value, self->path);
	}
	else
	{
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}
static void
remove_response_handler(	AdwMessageDialog *dialog,
				gchar* response_id,
				gpointer data )
{
	CelluloidPluginsManagerItem *item = data;
	GFile *file = g_file_new_for_path(item->path);
	GError *error = NULL;

	if(strcmp (response_id, "remove") == 0)
	{
		g_file_delete_recursive(file, NULL, &error);
	}

	gtk_window_destroy(GTK_WINDOW(dialog));

	if(error)
	{
		GtkWidget *error_dialog;

		error_dialog =	adw_message_dialog_new
				(	item->parent_window,
					NULL,
					NULL);
		adw_message_dialog_format_body
			(	ADW_MESSAGE_DIALOG (error_dialog),
				_("Failed to delete file '%s'. Reason: %s"),
				g_file_get_uri(file),
				error->message );

		g_warning(	"Failed to delete file '%s'. Reason: %s",
				g_file_get_uri(file),
				error->message );

		gtk_window_present(GTK_WINDOW(error_dialog));

		gtk_window_destroy(GTK_WINDOW(error_dialog));
		g_error_free(error);
	}

	g_object_unref(file);
}

static void
remove_handler(GtkButton *button, gpointer data)
{
	CelluloidPluginsManagerItem *item = data;
	GtkWidget *confirm_dialog =	adw_message_dialog_new
					(	item->parent_window,
						NULL,
						_("Are you sure you want to "
						"remove this script? This "
						"action cannot be undone."));

	adw_message_dialog_add_responses
		(	ADW_MESSAGE_DIALOG(confirm_dialog),
			"remove", _("_Remove"),
			"keep", _("_Keep"),
			NULL );

	adw_message_dialog_set_response_appearance
		(	ADW_MESSAGE_DIALOG(confirm_dialog),
			 "remove",
			 ADW_RESPONSE_DESTRUCTIVE );

	adw_message_dialog_set_default_response
		(	ADW_MESSAGE_DIALOG(confirm_dialog),
			"keep" );
	adw_message_dialog_set_close_response
		(	ADW_MESSAGE_DIALOG(confirm_dialog),
			"keep" );

	g_signal_connect
		(	confirm_dialog,
			"response",
			G_CALLBACK(remove_response_handler),
			item );

	gtk_window_present(GTK_WINDOW(confirm_dialog));
}

static void
celluloid_plugins_manager_item_class_init(CelluloidPluginsManagerItemClass *klass)
{
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec = NULL;

	obj_class->constructed = celluloid_plugins_manager_item_constructed;
	obj_class->finalize = celluloid_plugins_manager_item_finalize;
	obj_class->set_property = celluloid_plugins_manager_item_set_property;
	obj_class->get_property = celluloid_plugins_manager_item_get_property;

	pspec = g_param_spec_pointer
		(	"parent",
			"Parent",
			"Parent window for the dialogs",
			G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE );
	g_object_class_install_property(obj_class, PROP_PARENT, pspec);

	pspec = g_param_spec_string
		(	"path",
			"Path",
			"The path to the file that this item references",
			"",
			G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE );
	g_object_class_install_property(obj_class, PROP_PATH, pspec);
}

static void
celluloid_plugins_manager_item_init(CelluloidPluginsManagerItem *item)
{
	item->parent_window = NULL;
	item->path = NULL;
}

GtkWidget *
celluloid_plugins_manager_item_new(	GtkWindow *parent,
					const gchar *title,
					const gchar *path )
{
	return GTK_WIDGET(g_object_new(	celluloid_plugins_manager_item_get_type(),
					"parent", parent,
					"title", title,
					"path", path,
					NULL));
}
