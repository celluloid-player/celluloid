/*
 * Copyright (c) 2015 gnome-mpv
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

#include <stdio.h>
#include <stdlib.h>

#include "keybind.h"
#include "def.h"

keybind *keybind_parse_config_line(const gchar *line)
{
	keybind *result = NULL;
	gchar **tokens;
	gchar **keys;
	gchar *linebuf;
	gint offset;
	gint index;

	linebuf = g_strdup(line);

	if(linebuf)
	{
		offset = -1;

		/* Remove comments */
		while(linebuf[++offset] != '#' && linebuf[offset] != '\0');

		if(linebuf[offset] == '#')
		{
			linebuf[offset] = '\0';
		}

		g_strstrip(linebuf);

		/* Fail if the command require Property Expansion other than
		 * "$$" which translates to just "$".
		 */
		if(!g_regex_match_simple("\\$[^\\$]", linebuf, 0, 0))
		{
			GRegex *regex = g_regex_new("\\$\\$", 0, 0, NULL);
			gchar *old_linebuf = linebuf;

			linebuf = g_regex_replace_literal
				(regex, old_linebuf, -1, 0, "$", 0, NULL);

			g_free(old_linebuf);

			result = g_malloc(sizeof(keybind));
			tokens = g_strsplit_set(linebuf, " \t", -1);
			keys = g_strsplit(tokens[0], "+", -1);

			result->modifier = 0x0;
			result->keyval = 0x0;
			result->command = NULL;
		}
		else
		{
			result = NULL;
		}
	}

	index = -1;

	/* Parse key */
	while(result && keys[++index])
	{
		gint keyval;

		if(g_ascii_strncasecmp(keys[index], "shift", 5) == 0)
		{
			keyval = GDK_SHIFT_MASK;
		}
		else if(g_ascii_strncasecmp(keys[index], "ctrl", 4) == 0)
		{
			keyval = GDK_CONTROL_MASK;
		}
		else if(g_ascii_strncasecmp(keys[index], "alt", 3) == 0)
		{
			keyval = GDK_MOD1_MASK;
		}
		else
		{
			keyval = gdk_keyval_from_name(keys[index]);
		}

		/* Invalid key */
		if(keyval == GDK_KEY_VoidSymbol)
		{
			free(result);

			result = NULL;
		}
		else if(keyval == GDK_SHIFT_MASK
		|| keyval == GDK_CONTROL_MASK
		|| keyval == GDK_MOD1_MASK)
		{
			result->modifier |= keyval;
		}
		else
		{
			result->keyval = keyval;
		}
	}

	if(result)
	{
		result->command = g_strdupv(tokens+1);

		g_strfreev(tokens);
		g_free(linebuf);
	}

	return result;
}

GSList *keybind_parse_config(const gchar *config_path, gboolean* has_ignore)
{
	GSList *result;
	FILE *config_file;
	char *linebuf;
	size_t linebuf_size;

	if(has_ignore)
	{
		*has_ignore = FALSE;
	}

	result = NULL;
	config_file = fopen(config_path, "r");
	linebuf = NULL;
	linebuf_size = 0;

	while(	config_file
		&& getline(&linebuf, &linebuf_size, config_file) != -1 )
	{
		gint offset = -1;

		/* Ignore comments */
		while(++offset < linebuf_size && linebuf[offset] != '#');

		if(offset != 0)
		{
			keybind *keybind = keybind_parse_config_line(linebuf);

			if(keybind)
			{
				result = g_slist_append(result, keybind);
			}
			else if(has_ignore)
			{
				*has_ignore = TRUE;
			}
		}

		g_free(linebuf);

		linebuf = NULL;
		linebuf_size = 0;
	}

	if(!config_file || !feof(config_file))
	{
		perror("Failed to read mpv key bindings");

		result = NULL;
	}
	else
	{
		fclose(config_file);
	}

	return result;
}

gchar **keybind_get_command(gmpv_handle *ctx, gint modifier, gint keyval)
{
	GSList *iter = ctx->keybind_list;
	keybind *kb = iter->data;

	while(kb && (kb->modifier != modifier || kb->keyval != keyval))
	{
		iter = g_slist_next(iter);
		kb = iter?iter->data:NULL;
	}

	if(kb)
	{
		mpv_command(ctx->mpv_ctx, (const gchar **)kb->command);
	}

	return kb?kb->command:NULL;
}

GSList *keybind_parse_config_with_defaults(	const gchar *config_path,
						gboolean *has_ignore )
{
	const gchar *default_kb[] = DEFAULT_KEYBINDS;
	gint index;
	GSList *default_kb_list;
	GSList *config_kb_list;

	index = -1;
	default_kb_list = NULL;

	config_kb_list =	config_path?
				keybind_parse_config(config_path, has_ignore):
				NULL;

	while(default_kb[++index])
	{
		keybind *keybind = keybind_parse_config_line(default_kb[index]);

		if(keybind)
		{
			default_kb_list
				= g_slist_append(default_kb_list, keybind);
		}
		else if(has_ignore)
		{
			*has_ignore = TRUE;
		}
	}

	/* Key bindings from configuration file gets prority over default ones
	 */
	return	config_kb_list?
		g_slist_concat(config_kb_list, default_kb_list):
		default_kb_list;
}
