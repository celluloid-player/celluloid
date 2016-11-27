/*
 * Copyright (c) 2016 gnome-mpv
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

#include <gdk/gdk.h>
#include <gio/gio.h>
#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>

#include "gmpv_plugins_manager_item.h"

enum
{
	PROP_0,
	PROP_PARENT,
	PROP_TITLE,
	PROP_PATH,
	N_PROPERTIES
};

struct _GmpvPluginsManagerItem
{
	GtkListBoxRow parent;
	GtkWindow *parent_window;
	gchar *title;
	gchar *path;
};

struct _GmpvPluginsManagerItemClass
{
	GtkListBoxRowClass parent_class;
};

G_DEFINE_TYPE(GmpvPluginsManagerItem, gmpv_plugins_manager_item, GTK_TYPE_LIST_BOX_ROW)

static void gmpv_plugins_manager_item_constructed(GObject *object);
static void gmpv_plugins_manager_item_finalize(GObject *object);
static void gmpv_plugins_manager_item_set_property(	GObject *object,
						guint property_id,
						const GValue *value,
						GParamSpec *pspec );
static void gmpv_plugins_manager_item_get_property(	GObject *object,
						guint property_id,
						GValue *value,
						GParamSpec *pspec );
static void remove_handler(GtkButton *button, gpointer data);

static void gmpv_plugins_manager_item_constructed(GObject *object)
{
	GmpvPluginsManagerItem *self = GMPV_PLUGINS_MANAGER_ITEM(object);
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GtkWidget *title_label = gtk_label_new(self->title);
	GtkWidget *remove_button = gtk_button_new_with_label(_("Remove"));

	g_signal_connect(	remove_button,
				"clicked",
				G_CALLBACK(remove_handler),
				self );

	gtk_widget_set_halign(title_label, GTK_ALIGN_START);
	gtk_label_set_ellipsize(GTK_LABEL(title_label), PANGO_ELLIPSIZE_END);
	gtk_list_box_row_set_selectable(GTK_LIST_BOX_ROW(self), FALSE);

	gtk_box_pack_start(GTK_BOX(box), title_label, TRUE, TRUE, 6);
	gtk_box_pack_end(GTK_BOX(box), remove_button, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(self), box);

	G_OBJECT_CLASS(gmpv_plugins_manager_item_parent_class)
	->constructed(object);
}

static void gmpv_plugins_manager_item_finalize(GObject *object)
{
	GmpvPluginsManagerItem *self = GMPV_PLUGINS_MANAGER_ITEM(object);

	g_free(self->title);
	g_free(self->path);

	G_OBJECT_CLASS(gmpv_plugins_manager_item_parent_class)->finalize(object);
}

static void gmpv_plugins_manager_item_set_property(	GObject *object,
						guint property_id,
						const GValue *value,
						GParamSpec *pspec )
{
	GmpvPluginsManagerItem *self = GMPV_PLUGINS_MANAGER_ITEM(object);

	if(property_id == PROP_PARENT)
	{
		self->parent_window = g_value_get_pointer(value);
	}
	else if(property_id == PROP_TITLE)
	{
		g_free(self->title);

		self->title = g_value_dup_string(value);
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

static void gmpv_plugins_manager_item_get_property(	GObject *object,
						guint property_id,
						GValue *value,
						GParamSpec *pspec )
{
	GmpvPluginsManagerItem *self = GMPV_PLUGINS_MANAGER_ITEM(object);

	if(property_id == PROP_PARENT)
	{
		g_value_set_pointer(value, self->parent_window);
	}
	else if(property_id == PROP_TITLE)
	{
		g_value_set_string(value, self->title);
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

static void remove_handler(GtkButton *button, gpointer data)
{
	GmpvPluginsManagerItem *item = data;
	GFile *file = g_file_new_for_path(item->path);
	GError *error = NULL;
	GtkWidget *confirm_dialog =	gtk_message_dialog_new
					(	item->parent_window,
						GTK_DIALOG_MODAL|
						GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_MESSAGE_QUESTION,
						GTK_BUTTONS_YES_NO,
						_("Are you sure you want to "
						"remove this script? This "
						"action cannot be undone."));

	if(gtk_dialog_run(GTK_DIALOG(confirm_dialog)) == GTK_RESPONSE_YES)
	{
		g_file_delete(file, NULL, &error);
	}

	gtk_widget_destroy(confirm_dialog);

	if(error)
	{
		GtkWidget *error_dialog;

		error_dialog =	gtk_message_dialog_new
				(	item->parent_window,
					GTK_DIALOG_MODAL|
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_OK,
					_("Failed to delete file '%s'. "
					"Reason: %s"),
					g_file_get_uri(file),
					error->message );

		g_warning(	"Failed to delete file '%s'. Reason: %s",
				g_file_get_uri(file),
				error->message );

		gtk_dialog_run(GTK_DIALOG(error_dialog));

		gtk_widget_destroy(error_dialog);
		g_error_free(error);
	}
}

static void gmpv_plugins_manager_item_class_init(GmpvPluginsManagerItemClass *klass)
{
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec = NULL;

	obj_class->constructed = gmpv_plugins_manager_item_constructed;
	obj_class->finalize = gmpv_plugins_manager_item_finalize;
	obj_class->set_property = gmpv_plugins_manager_item_set_property;
	obj_class->get_property = gmpv_plugins_manager_item_get_property;

	pspec = g_param_spec_pointer
		(	"parent",
			"Parent",
			"Parent window for the dialogs",
			G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE );
	g_object_class_install_property(obj_class, PROP_PARENT, pspec);

	pspec = g_param_spec_string
		(	"title",
			"Title",
			"The string to display as the title of the item",
			"",
			G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE );
	g_object_class_install_property(obj_class, PROP_TITLE, pspec);

	pspec = g_param_spec_string
		(	"path",
			"Path",
			"The path to the file that this item references",
			"",
			G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE );
	g_object_class_install_property(obj_class, PROP_PATH, pspec);
}

static void gmpv_plugins_manager_item_init(GmpvPluginsManagerItem *item)
{
	item->parent_window = NULL;
	item->title = NULL;
	item->path = NULL;
}

GtkWidget *gmpv_plugins_manager_item_new(	GtkWindow *parent,
					const gchar *title,
					const gchar *path )
{
	return GTK_WIDGET(g_object_new(	gmpv_plugins_manager_item_get_type(),
					"parent", parent,
					"title", title,
					"path", path,
					NULL));
}
