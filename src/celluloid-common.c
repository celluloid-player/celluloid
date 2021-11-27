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
#include <gio/gsettingsbackend.h>
#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <string.h>

#include "celluloid-common.h"
#include "celluloid-def.h"
#include "celluloid-mpv.h"
#include "celluloid-main-window.h"
#include "celluloid-control-box.h"

CelluloidPlaylistEntry *
celluloid_playlist_entry_new(const gchar *filename, const gchar *title)
{
	CelluloidPlaylistEntry *entry = g_malloc(sizeof(CelluloidPlaylistEntry));

	entry->filename =	g_strdup(filename);
	entry->title =		g_strdup(title);
	entry->metadata =	g_ptr_array_new_with_free_func
				((GDestroyNotify)celluloid_metadata_entry_free);

	return entry;
}

void
celluloid_playlist_entry_free(CelluloidPlaylistEntry *entry)
{
	if(entry)
	{
		g_free(entry->filename);
		g_free(entry->title);
		g_ptr_array_free(entry->metadata, TRUE);
		g_free(entry);
	}
}

CelluloidMetadataEntry *
celluloid_metadata_entry_new(const gchar *key, const gchar *value)
{
	CelluloidMetadataEntry *entry = g_malloc(sizeof(CelluloidMetadataEntry));

	entry->key = g_strdup(key);
	entry->value = g_strdup(value);

	return entry;
}

void
celluloid_metadata_entry_free(CelluloidMetadataEntry *entry)
{
	if(entry)
	{
		g_free(entry->key);
		g_free(entry->value);
		g_free(entry);
	}
}

CelluloidTrack *
celluloid_track_new(void)
{
	CelluloidTrack *entry = g_malloc(sizeof(CelluloidTrack));

	entry->type = TRACK_TYPE_INVALID;
	entry->title = NULL;
	entry->lang = NULL;
	entry->id = 0;

	return entry;
}

void
celluloid_track_free(CelluloidTrack *entry)
{
	if(entry)
	{
		g_free(entry->title);
		g_free(entry->lang);
		g_free(entry);
	}
}

CelluloidDisc *
celluloid_disc_new(void)
{
	return g_malloc0(sizeof(CelluloidDisc));
}

void
celluloid_disc_free(CelluloidDisc *disc)
{
	g_free(disc->uri);
	g_free(disc->label);
	g_free(disc);
}

gchar *
get_config_dir_path(void)
{
	return g_build_filename(	g_get_user_config_dir(),
					CONFIG_DIR,
					NULL );
}

gchar *
get_scripts_dir_path(void)
{
	return g_build_filename(	g_get_user_config_dir(),
					CONFIG_DIR,
					"scripts",
					NULL );
}

gchar *
get_watch_dir_path(void)
{
	return g_build_filename(	g_get_user_config_dir(),
					CONFIG_DIR,
					"watch_later",
					NULL );
}

gchar *
get_path_from_uri(const gchar *uri)
{
	GFile *file = g_vfs_get_file_for_uri(g_vfs_get_default(), uri);
	gchar *path = g_file_get_path(file);

	if(file)
	{
		g_object_unref(file);
	}

	return path?path:g_strdup(uri);
}

gchar *
get_name_from_path(const gchar *path)
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

gboolean
extension_matches(const gchar *filename, const gchar **extensions)
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

gboolean
g_source_clear(guint *tag)
{
	if(*tag != 0)
	{
		g_source_remove(*tag);
		*tag = 0;
	}

	return TRUE;
}

gchar *
strnjoinv(const gchar *separator, const gchar **str_array, gsize count)
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

gchar *
sanitize_utf8(const gchar *str, const gboolean label)
{
	gchar *result = NULL;
	const gchar *end = NULL;
	const gboolean valid = g_utf8_validate(str, -1, &end);

	g_assert(end >= str && (end - str) <= G_MAXINT);

	if(label && !valid)
	{
		result =	g_strdup_printf
				(	"%.*s (%s)",
					(gint)(end - str),
					str,
					_("invalid encoding") );
	}
	else
	{
		result =	g_strdup_printf
				(	"%.*s",
					(gint)(end - str),
					str );
	}

	return result;
}

char *
format_time(gint seconds, gboolean show_hour)
{
	gchar *result = NULL;

	if(show_hour)
	{
		result = g_strdup_printf(	"%02d:%02d:%02d",
						seconds/3600,
						(seconds%3600)/60,
						seconds%60 );
	}
	else
	{
		result = g_strdup_printf("%02d:%02d", seconds/60, seconds%60);
	}

	return result;
}

void
activate_action_string(GActionMap *map, const gchar *str)
{
	GAction *action = NULL;
	gchar *name = NULL;
	GVariant *param = NULL;
	gboolean param_match = FALSE;

	g_action_parse_detailed_name(str, &name, &param, NULL);

	if(name)
	{
		const GVariantType *action_ptype;
		const GVariantType *given_ptype;

		action = g_action_map_lookup_action(map, name);
		action_ptype = g_action_get_parameter_type(action);
		given_ptype = param?g_variant_get_type(param):NULL;

		param_match =	(action_ptype == given_ptype) ||
				(	given_ptype &&
					g_variant_type_is_subtype_of
					(action_ptype, given_ptype) );
	}

	if(action && param_match)
	{
		g_debug("Activating action %s", str);
		g_action_activate(action, param);
	}
	else
	{
		g_warning("Failed to activate action \"%s\"", str);
	}
}
