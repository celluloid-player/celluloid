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

#include <glib/gi18n.h>

#include "plugins_manager.h"
#include "plugins_manager_item.h"
#include "common.h"

enum
{
	PROP_0,
	PROP_PARENT,
	N_PROPERTIES
};

struct _PluginsManager
{
	GtkGrid parent;
	GtkWindow *parent_window;
	GtkWidget *list_box;
	GFileMonitor *monitor;
	gchar *directory;
};

struct _PluginsManagerClass
{
	GtkGridClass parent_class;
};

G_DEFINE_TYPE(PluginsManager, plugins_manager, GTK_TYPE_GRID)

static void plugins_manager_constructed(GObject *object);
static void plugins_manager_finalize(GObject *object);
static void plugins_manager_set_property(	GObject *object,
						guint property_id,
						const GValue *value,
						GParamSpec *pspec );
static void plugins_manager_get_property(	GObject *object,
						guint property_id,
						GValue *value,
						GParamSpec *pspec );
static void add_handler(GtkButton *button, gpointer data);
static void changed_handler(	GFileMonitor *monitor,
				GFile *file,
				GFile *other_file,
				GFileMonitorEvent event_type,
				gpointer data );

static void plugins_manager_constructed(GObject *object)
{
	PluginsManager *self = PLUGINS_MANAGER(object);
	gchar *scripts_dir = get_scripts_dir_path();

	plugins_manager_set_directory(self, scripts_dir);

	g_free(scripts_dir);

	G_OBJECT_CLASS(plugins_manager_parent_class)->constructed(object);
}

static void plugins_manager_finalize(GObject *object)
{
	PluginsManager *pmgr = PLUGINS_MANAGER(object);

	g_clear_object(&pmgr->monitor);
	g_free(pmgr->directory);

	G_OBJECT_CLASS(plugins_manager_parent_class)->finalize(object);
}

static void plugins_manager_set_property(	GObject *object,
						guint property_id,
						const GValue *value,
						GParamSpec *pspec )
{
	PluginsManager *self = PLUGINS_MANAGER(object);

	if(property_id == PROP_PARENT)
	{
		self->parent_window = g_value_get_pointer(value);
	}
	else
	{
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void plugins_manager_get_property(	GObject *object,
						guint property_id,
						GValue *value,
						GParamSpec *pspec )
{
	PluginsManager *self = PLUGINS_MANAGER(object);

	if(property_id == PROP_PARENT)
	{
		g_value_set_pointer(value, self->parent_window);
	}
	else
	{
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void add_handler(GtkButton *button, gpointer data)
{
	PluginsManager *pmgr = data;
	GtkWidget *dialog = NULL;
	GFile *src = NULL;
	GFile *dest = NULL;
	gchar *src_path = NULL;
	gchar *dest_path = NULL;
	GError *error = NULL;

	dialog = gtk_file_chooser_dialog_new(	"Add Lua Script",
						pmgr->parent_window,
						GTK_FILE_CHOOSER_ACTION_OPEN,
						_("Cancel"),
						GTK_RESPONSE_CANCEL,
						_("Open"),
						GTK_RESPONSE_ACCEPT,
						NULL );

	g_assert(pmgr->directory);

	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);

		src = gtk_file_chooser_get_file(chooser);
		dest_path = g_build_filename(	pmgr->directory,
						g_file_get_basename(src),
						NULL );
		dest = g_file_new_for_path(dest_path);

		g_file_copy(	src,
				dest,
				G_FILE_COPY_NONE,
				NULL, NULL, NULL,
				&error );
	}

	gtk_widget_destroy(dialog);

	if(error)
	{
		GtkWidget *error_dialog;

		src_path = g_file_get_path(src)?:g_file_get_uri(src);

		error_dialog =	gtk_message_dialog_new
				(	pmgr->parent_window,
					GTK_DIALOG_MODAL|
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_OK,
					_("Failed to copy file from '%s' "
					"to '%s'. Reason: %s"),
					src_path,
					dest_path,
					error->message );

		g_warning(	"Failed to copy file from '%s' to '%s'. "
				"Reason: %s",
				src_path,
				dest_path,
				error->message );

		gtk_dialog_run(GTK_DIALOG(error_dialog));

		gtk_widget_destroy(error_dialog);
		g_error_free(error);
	}

	g_clear_object(&src);
	g_clear_object(&dest);
	g_free(src_path);
	g_free(dest_path);
}

static void changed_handler(	GFileMonitor *monitor,
				GFile *file,
				GFile *other_file,
				GFileMonitorEvent event_type,
				gpointer data )
{
	PluginsManager *pmgr = data;
	GDir *dir = NULL;
	const gchar *filename = NULL;
	const GFileMonitorEvent mask =	G_FILE_MONITOR_EVENT_CREATED|
					G_FILE_MONITOR_EVENT_DELETED|
					G_FILE_MONITOR_EVENT_UNMOUNTED;

	if(event_type&mask)
	{
		g_assert(pmgr->directory);

		dir = g_dir_open(pmgr->directory, 0, NULL);
	}

	if(dir)
	{
		GtkContainer *list_box;
		GList *iter;

		list_box = GTK_CONTAINER(pmgr->list_box);
		iter = gtk_container_get_children(list_box);

		while(iter)
		{
			gtk_widget_destroy(iter->data);

			iter = g_list_next(iter);
		}

		do
		{
			gchar *full_path;

			filename = g_dir_read_name(dir);
			full_path =	g_build_filename
					(pmgr->directory, filename, NULL);

			if(g_file_test(full_path, G_FILE_TEST_IS_REGULAR))
			{
				GtkWidget *item;

				item =	plugins_manager_item_new
					(	pmgr->parent_window,
						filename,
						full_path );

				gtk_container_add
					(GTK_CONTAINER(pmgr->list_box), item);
			}

			g_free(full_path);
		}
		while(filename);

		gtk_widget_show_all(pmgr->list_box);

		g_dir_close(dir);
	}
}

static void plugins_manager_class_init(PluginsManagerClass *klass)
{
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec = NULL;

	obj_class->constructed = plugins_manager_constructed;
	obj_class->finalize = plugins_manager_finalize;
	obj_class->set_property = plugins_manager_set_property;
	obj_class->get_property = plugins_manager_get_property;

	pspec = g_param_spec_pointer
		(	"parent",
			"Parent",
			"Parent window for the dialogs",
			G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE );

	g_object_class_install_property(obj_class, PROP_PARENT, pspec);
}

static void plugins_manager_init(PluginsManager *pmgr)
{
	GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	GtkWidget *add_button = gtk_button_new_with_label("+");

	pmgr->list_box = gtk_list_box_new();
	pmgr->parent_window = NULL;
	pmgr->monitor = NULL;
	pmgr->directory = NULL;

	g_signal_connect(	add_button,
				"clicked",
				G_CALLBACK(add_handler),
				pmgr );

	gtk_widget_set_hexpand(GTK_WIDGET(scrolled_window), TRUE);
	gtk_widget_set_vexpand(GTK_WIDGET(scrolled_window), TRUE);

	gtk_grid_attach(	GTK_GRID(pmgr),
				scrolled_window,
				0, 0, 1, 1 );
	gtk_grid_attach(	GTK_GRID(pmgr),
				add_button,
				0, 1, 1, 1 );

	gtk_container_add(GTK_CONTAINER(scrolled_window), pmgr->list_box);
}

GtkWidget *plugins_manager_new(GtkWindow *parent)
{
	return g_object_new(	plugins_manager_get_type(),
				"parent", parent,
				NULL);
}

void plugins_manager_set_directory(PluginsManager *pmgr, const gchar *path)
{
	GFile *directory = g_file_new_for_path(path);

	g_clear_object(&pmgr->monitor);
	g_free(pmgr->directory);

	pmgr->monitor = g_file_monitor_directory(	directory,
							G_FILE_MONITOR_NONE,
							NULL,
							NULL );
	pmgr->directory = g_strdup(path);

	if(pmgr->monitor)
	{
		g_signal_connect(	pmgr->monitor,
					"changed",
					G_CALLBACK(changed_handler),
					pmgr );

		changed_handler(	pmgr->monitor, NULL, NULL,
					G_FILE_MONITOR_EVENT_CREATED,
					pmgr );
	}
	else
	{
		g_warning("Failed to monitor directory %s", path);
	}

	g_object_unref(directory);
}
