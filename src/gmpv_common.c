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

#include <gio/gio.h>
#include <gio/gsettingsbackend.h>
#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <string.h>

#include "gmpv_common.h"
#include "gmpv_def.h"
#include "gmpv_mpv.h"
#include "gmpv_mpv_wrapper.h"
#include "gmpv_main_window.h"
#include "gmpv_control_box.h"

GmpvPlaylistEntry *gmpv_playlist_entry_new(	const gchar *filename,
						const gchar *title )
{
	GmpvPlaylistEntry *entry = g_malloc(sizeof(GmpvPlaylistEntry));

	entry->filename = g_strdup(filename);
	entry->title = g_strdup(title);

	return entry;
}

void gmpv_playlist_entry_free(GmpvPlaylistEntry *entry)
{
	g_free(entry->filename);
	g_free(entry->title);
	g_free(entry);
}

GmpvMetadataEntry *gmpv_metadata_entry_new(const gchar *key, const gchar *value)
{
	GmpvMetadataEntry *entry = g_malloc(sizeof(GmpvMetadataEntry));

	entry->key = g_strdup(key);
	entry->value = g_strdup(value);

	return entry;
}

void gmpv_metadata_entry_free(GmpvMetadataEntry *entry)
{
	g_free(entry->key);
	g_free(entry->value);
	g_free(entry);
}

gchar *get_config_dir_path(void)
{
	return g_build_filename(	g_get_user_config_dir(),
					CONFIG_DIR,
					NULL );
}

gchar *get_scripts_dir_path(void)
{
	return g_build_filename(	g_get_user_config_dir(),
					CONFIG_DIR,
					"scripts",
					NULL );
}

gchar *get_path_from_uri(const gchar *uri)
{
	GFile *file = g_vfs_get_file_for_uri(g_vfs_get_default(), uri);
	gchar *path = g_file_get_path(file);

	if(file)
	{
		g_object_unref(file);
	}

	return path?path:g_strdup(uri);
}

gchar *get_name_from_path(const gchar *path)
{
	const gchar *scheme = g_uri_parse_scheme(path);
	gchar *basename = NULL;

	/* Check whether the given path is likely to be a local path */
	if(!scheme && path)
	{
		basename = g_path_get_basename(path);
	}

	return basename?basename:g_strdup(path);
}

void migrate_config()
{
	const gchar *keys[] = {	"dark-theme-enable",
				"csd-enable",
				"last-folder-enable",
				"mpv-options",
				"mpv-config-file",
				"mpv-config-enable",
				"mpv-input-config-file",
				"mpv-input-config-enable",
				NULL };

	GSettings *old_settings = g_settings_new("org.gnome-mpv");
	GSettings *new_settings = g_settings_new(CONFIG_ROOT);

	if(!g_settings_get_boolean(new_settings, "settings-migrated"))
	{
		g_settings_set_boolean(new_settings, "settings-migrated", TRUE);

		for(gint i = 0; keys[i]; i++)
		{
			GVariant *buf = g_settings_get_user_value
						(old_settings, keys[i]);

			if(buf)
			{
				g_settings_set_value
					(new_settings, keys[i], buf);

				g_variant_unref(buf);
			}
		}
	}

	g_object_unref(old_settings);
	g_object_unref(new_settings);
}

gboolean extension_matches(const gchar *filename, const gchar **extensions)
{
	const gchar *ext = strrchr(filename, '.');
	gboolean result = FALSE;

	/* Only start checking the extension if there is at
	 * least one character after the dot.
	 */
	if(ext && ++ext)
	{
		const gchar **iter = extensions;

		/* Check if the file extension matches one of the
		 * supported subtitle formats.
		 */
		while(*iter && g_strcmp0(ext, *(iter++)) != 0);

		result = !!(*iter);
	}

	return result;
}

void *gslist_to_array(GSList *slist)
{
	void **result = g_malloc(sizeof(void **)*(g_slist_length(slist)+1));
	gint i = 0;

	for(GSList *iter = slist; iter; iter = g_slist_next(iter))
	{
		result[i++] = iter->data;
	}

	result[i] = NULL;

	return result;
}

gchar *strnjoinv(const gchar *separator, const gchar **str_array, gsize count)
{
	gsize args_size = ((gsize)count+1)*sizeof(gchar *);
	gchar **args = g_malloc(args_size);
	gchar *result;

	memcpy(args, str_array, args_size-sizeof(gchar *));
	args[count] = NULL;
	result = g_strjoinv(separator, args);

	g_free(args);

	return result;
}

