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

#include <glib/gi18n.h>
#include <adwaita.h>

#include "celluloid-plugins-manager.h"
#include "celluloid-plugins-manager-item.h"
#include "celluloid-file-chooser.h"
#include "celluloid-common.h"

enum
{
	PROP_0,
	PROP_PARENT,
	N_PROPERTIES
};

struct _CelluloidPluginsManager
{
	AdwPreferencesPage parent;
	GtkWindow *parent_window;
	AdwPreferencesGroup *pref_group;
	GtkWidget *placeholder;
	GFileMonitor *monitor;
	gchar *directory;
};

struct _CelluloidPluginsManagerClass
{
	AdwPreferencesPage parent_class;
};

G_DEFINE_TYPE(CelluloidPluginsManager, celluloid_plugins_manager, ADW_TYPE_PREFERENCES_PAGE)

static void
celluloid_plugins_manager_constructed(GObject *object);

static void
celluloid_plugins_manager_finalize(GObject *object);

static void
celluloid_plugins_manager_set_property(	GObject *object,
					guint property_id,
					const GValue *value,
					GParamSpec *pspec );

static void
celluloid_plugins_manager_get_property(	GObject *object,
					guint property_id,
					GValue *value,
					GParamSpec *pspec );

static void
response_handler(GtkNativeDialog *self, gint response_id, gpointer data);

static void
add_handler(GtkButton *button, gpointer data);

static gboolean
drop_handler(	GtkDropTarget *self,
		GValue *value,
		gdouble x,
		gdouble y,
		gpointer data );

static void
changed_handler(	GFileMonitor *monitor,
			GFile *file,
			GFile *other_file,
			GFileMonitorEvent event_type,
			gpointer data );

static void
copy_file_to_directory(CelluloidPluginsManager *pmgr, GFile *src);

static void
celluloid_plugins_manager_constructed(GObject *object)
{
	CelluloidPluginsManager *self = CELLULOID_PLUGINS_MANAGER(object);
	gchar *scripts_dir = get_scripts_dir_path();

	celluloid_plugins_manager_set_directory(self, scripts_dir);

	g_free(scripts_dir);

	G_OBJECT_CLASS(celluloid_plugins_manager_parent_class)
		->constructed(object);
}

static void
celluloid_plugins_manager_finalize(GObject *object)
{
	CelluloidPluginsManager *pmgr = CELLULOID_PLUGINS_MANAGER(object);

	g_clear_object(&pmgr->monitor);
	g_free(pmgr->directory);

	G_OBJECT_CLASS(celluloid_plugins_manager_parent_class)
		->finalize(object);
}

static void
celluloid_plugins_manager_set_property(	GObject *object,
					guint property_id,
					const GValue *value,
					GParamSpec *pspec )
{
	CelluloidPluginsManager *self = CELLULOID_PLUGINS_MANAGER(object);

	if(property_id == PROP_PARENT)
	{
		self->parent_window = g_value_get_pointer(value);
	}
	else
	{
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void
celluloid_plugins_manager_get_property(	GObject *object,
					guint property_id,
					GValue *value,
					GParamSpec *pspec )
{
	CelluloidPluginsManager *self = CELLULOID_PLUGINS_MANAGER(object);

	if(property_id == PROP_PARENT)
	{
		g_value_set_pointer(value, self->parent_window);
	}
	else
	{
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void
response_handler(GtkNativeDialog *self, gint response_id, gpointer data)
{
	GFile *src = NULL;

	if(response_id == GTK_RESPONSE_ACCEPT)
	{
		src = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(self));
		copy_file_to_directory(CELLULOID_PLUGINS_MANAGER(data), src);
	}

	g_clear_object(&src);
	celluloid_file_chooser_destroy(CELLULOID_FILE_CHOOSER(self));
}

static void
add_handler(GtkButton *button, gpointer data)
{
	CelluloidPluginsManager *pmgr = data;
	CelluloidFileChooser *dialog = NULL;
	GtkFileFilter *filter;
	GtkFileChooser *chooser;

	dialog = celluloid_file_chooser_new(	_("Add Plugin"),
						pmgr->parent_window,
						GTK_FILE_CHOOSER_ACTION_OPEN );
	filter = NULL;
	chooser = GTK_FILE_CHOOSER(dialog);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("All Files"));
	gtk_file_filter_add_pattern(filter, "*");
	gtk_file_chooser_add_filter(chooser, filter);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("Lua Plugins"));
	gtk_file_filter_add_mime_type(filter, "text/x-lua");
	gtk_file_chooser_add_filter(chooser, filter);
	gtk_file_chooser_set_filter(chooser, filter);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("JavaScript Plugins"));
	gtk_file_filter_add_mime_type(filter, "application/javascript");
	gtk_file_chooser_add_filter(chooser, filter);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("C Plugins"));
	gtk_file_filter_add_mime_type(filter, "application/x-sharedlib");
	gtk_file_chooser_add_filter(chooser, filter);

	g_signal_connect(	dialog,
				"response",
				G_CALLBACK(response_handler),
				pmgr );

	gtk_native_dialog_show(GTK_NATIVE_DIALOG(dialog));
}

static gboolean
drop_handler(	GtkDropTarget *self,
		GValue *value,
		gdouble x,
		gdouble y,
		gpointer data )
{
	g_assert(G_VALUE_HOLDS(value, G_TYPE_FILE));

	GFile *file = g_value_get_object(value);
	copy_file_to_directory(CELLULOID_PLUGINS_MANAGER(data), file);

	return TRUE;
}

static void
changed_handler(	GFileMonitor *monitor,
			GFile *file,
			GFile *other_file,
			GFileMonitorEvent event_type,
			gpointer data )
{
	CelluloidPluginsManager *pmgr = data;
	GDir *dir = NULL;
	const gchar *filename = NULL;
	gboolean empty = TRUE;
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
		AdwPreferencesGroup *pref_group;
		GtkWidget *first_child;
		GtkWidget *list_box;

		pref_group = ADW_PREFERENCES_GROUP (pmgr->pref_group);
		// It's not possible to cleanly delete PreferenceGroup's children, hence this hack.
		list_box = gtk_widget_get_first_child( gtk_widget_get_last_child (gtk_widget_get_first_child (pref_group)));
		first_child = gtk_widget_get_first_child (list_box);

		// Empty the list box
		while(first_child)
		{
			gtk_list_box_remove(list_box, first_child);

			first_child = gtk_widget_get_first_child(list_box);
		}

		// Populate the list box with files in the plugins directory
		do
		{
			gchar *full_path;

			filename = g_dir_read_name(dir);
			full_path =	g_build_filename
					(pmgr->directory, filename, NULL);

			if(g_file_test(full_path, G_FILE_TEST_IS_REGULAR))
			{
				GtkWidget *item;

				item =	celluloid_plugins_manager_item_new
					(	pmgr->parent_window,
						filename,
						full_path );
				adw_preferences_group_add(pmgr->pref_group, item);

				empty = FALSE;
			}

			g_free(full_path);
		}
		while(filename);

		gtk_widget_set_visible(pmgr->placeholder, empty);

		g_dir_close(dir);
	}
}

static void
copy_file_to_directory(CelluloidPluginsManager *pmgr, GFile *src)
{
	gchar *dest_path = NULL;
	GFile *dest = NULL;
	GError *error = NULL;

	if(src)
	{
		g_assert(pmgr->directory);

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

	if(error)
	{
		GtkWidget *error_dialog = NULL;
		gchar *src_path = NULL;

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

		g_signal_connect(	error_dialog,
					"response",
					G_CALLBACK(gtk_window_destroy),
					NULL );

		g_warning(	"Failed to copy file from '%s' to '%s'. "
				"Reason: %s",
				src_path,
				dest_path,
				error->message );

		gtk_widget_show(error_dialog);

		g_error_free(error);
		g_free(src_path);
	}

	g_clear_object(&dest);
	g_free(dest_path);
}

static void
celluloid_plugins_manager_class_init(CelluloidPluginsManagerClass *klass)
{
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec = NULL;

	obj_class->constructed = celluloid_plugins_manager_constructed;
	obj_class->finalize = celluloid_plugins_manager_finalize;
	obj_class->set_property = celluloid_plugins_manager_set_property;
	obj_class->get_property = celluloid_plugins_manager_get_property;

	pspec = g_param_spec_pointer
		(	"parent",
			"Parent",
			"Parent window for the dialogs",
			G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE );

	g_object_class_install_property(obj_class, PROP_PARENT, pspec);
}

static void
celluloid_plugins_manager_init(CelluloidPluginsManager *pmgr)
{
	GtkWidget *add_button = gtk_button_new();
	GtkWidget *add_button_child = adw_button_content_new();

	adw_preferences_page_set_title(pmgr, _("Plugins"));
	adw_preferences_page_set_icon_name(pmgr, "application-x-addon-symbolic");

	gtk_widget_add_css_class(add_button, "flat");
	adw_button_content_set_label(add_button_child, _("Add…"));
	adw_button_content_set_icon_name(add_button_child, "list-add-symbolic");
	gtk_button_set_child(add_button, add_button_child);

	pmgr->pref_group = adw_preferences_group_new();
	pmgr->placeholder = adw_status_page_new();
	pmgr->parent_window = NULL;
	pmgr->monitor = NULL;
	pmgr->directory = NULL;

	adw_preferences_group_set_header_suffix(pmgr->pref_group, add_button);

	g_signal_connect(	add_button,
				"clicked",
				G_CALLBACK(add_handler),
				pmgr );

	GtkDropTarget *drop_target =
		gtk_drop_target_new(G_TYPE_FILE, GDK_ACTION_COPY);

	gtk_widget_add_controller
		(pmgr, GTK_EVENT_CONTROLLER(drop_target));

	g_signal_connect(	drop_target,
				"drop",
				G_CALLBACK(drop_handler),
				pmgr );

	gtk_widget_set_tooltip_text(add_button, _("Add Plugin"));

	adw_status_page_set_title(ADW_STATUS_PAGE(pmgr->placeholder),_("No Plugins Found"));
	adw_status_page_set_icon_name(ADW_STATUS_PAGE(pmgr->placeholder), "application-x-addon-symbolic");
	gtk_widget_set_vexpand(pmgr->placeholder, true);
	gtk_widget_add_css_class(pmgr->placeholder,"card");
	adw_preferences_group_add (ADW_PREFERENCES_GROUP(pmgr->pref_group), pmgr->placeholder);

	adw_preferences_page_add(pmgr, pmgr->pref_group);
}

AdwPreferencesPage *
celluloid_plugins_manager_new(GtkWindow *parent)
{
	return g_object_new(	celluloid_plugins_manager_get_type(),
				"parent", parent,
				NULL);
}

void
celluloid_plugins_manager_set_directory(	CelluloidPluginsManager *pmgr,
						const gchar *path )
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
