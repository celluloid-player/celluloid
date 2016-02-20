/*
 * Copyright (c) 2015-2016 gnome-mpv
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

keybind *keybind_parse_config_line(const gchar *line, gboolean *propexp)
{
	keybind *result = NULL;
	gchar **tokens;
	gchar **keys;
	gchar *linebuf;
	gint offset;
	gint index;

	linebuf = g_strdup(line);

	if(propexp)
	{
		*propexp = FALSE;
	}

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
			gchar *old_linebuf;
			GRegex *regex;

			old_linebuf = linebuf;
			regex = g_regex_new("\\$\\$", 0, 0, NULL);

			linebuf = g_regex_replace_literal
				(regex, old_linebuf, -1, 0, "$", 0, NULL);

			g_free(old_linebuf);
			g_regex_unref(regex);

			regex = g_regex_new("\\s+", 0, 0, NULL);

			tokens = g_regex_split_full(	regex,
							linebuf,
							-1, 0, 0, 2,
							NULL );

			g_regex_unref(regex);

			keys =	(tokens && tokens[0])?
				g_strsplit(tokens[0], "+", -1):
				NULL;

			if(keys)
			{
				result = g_malloc(sizeof(keybind));

				result->mouse = FALSE;
				result->modifier = 0x0;
				result->keyval = 0x0;
				result->command = NULL;
			}
		}
		else
		{
			if(propexp)
			{
				*propexp = TRUE;
			}

			result = NULL;
		}
	}

	index = -1;

	/* Parse key */
	while(result && keys[++index])
	{
		gboolean mod = TRUE;
		guint keyval;

		/* Modifiers */
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
		else if(g_ascii_strncasecmp(keys[index], "meta", 4) == 0)
		{
			/* In mpv, meta == super */
			keyval = GDK_SUPER_MASK;
		}
		/* Mouse */
		else if(g_regex_match_simple(	"MOUSE_BTN[0-9]*(_DBL)?",
						keys[index],
						0,
						0 ))
		{
			/* Button number */
			result->mouse = TRUE;
			mod = FALSE;

			keyval = (guint)g_ascii_strtoull
					(keys[index]+9, NULL, 10)+1;

			if(g_regex_match_simple("_DBL$", keys[index], 0, 0))
			{
				/* Set modifier to indicate double click */
				result->modifier = 0x1;
			}
		}
		/* Non-modifier keys */
		else
		{
			const gchar *keystrmap[] = KEYSTRING_MAP;
			const gsize keystrlen = KEYSTRING_MAX_LEN;
			const gchar *match = NULL;
			gint i;

			mod = FALSE;

			/* Translate key string to GDK key name */
			for(i = 0; !match && keystrmap[i]; i += 2)
			{
				gint rc = g_ascii_strncasecmp(	keys[index],
								keystrmap[i],
								keystrlen );

				if(rc == 0)
				{
					match = keystrmap[i+1];
				}
			}

			keyval = gdk_keyval_from_name(match?match:keys[index]);
		}

		/* Invalid key */
		if(keyval == GDK_KEY_VoidSymbol)
		{
			free(result);

			result = NULL;
		}
		else if(mod)
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
		result->command = g_strdup(tokens[1]);

		g_strfreev(tokens);
		g_strfreev(keys);
		g_free(linebuf);
	}

	return result;
}

GSList *keybind_parse_config(const gchar *config_path, gboolean* propexp)
{
	GSList *result;
	GFile *config_file;
	GError *error;
	GFileInputStream *config_file_stream;
	GDataInputStream *config_data_stream;
	gchar *linebuf;
	gsize linebuf_size;

	if(propexp)
	{
		*propexp = FALSE;
	}

	result = NULL;
	error = NULL;
	linebuf = (gchar *)0xdeadbeef;
	linebuf_size = 0;
	config_file = g_file_new_for_path(config_path);
	config_file_stream = g_file_read(config_file, NULL, &error);

	if(!error)
	{
		config_data_stream
			= g_data_input_stream_new
				(G_INPUT_STREAM(config_file_stream));
	}

	while(!error && linebuf)
	{
		keybind *keybind;
		gboolean line_propexp;

		linebuf = g_data_input_stream_read_line
				(	config_data_stream,
					&linebuf_size,
					NULL,
					&error );

		keybind = keybind_parse_config_line(	linebuf,
							&line_propexp );

		if(keybind)
		{
			result = g_slist_append(result, keybind);
		}

		if(propexp && line_propexp)
		{
			*propexp = TRUE;
		}

		g_free(linebuf);
	}

	/* linebuf should be NULL if the read completes sucessfully */
	if(linebuf || error)
	{
		g_warning("Failed to load input configuration file");

		result = NULL;
	}
	else
	{
		g_object_unref(config_file_stream);
		g_object_unref(config_data_stream);
		g_object_unref(config_file);
	}

	/* Reverse the list so that bindings defined later in the file will have
	 * priority over the ones defined earlier.
	 */
	return g_slist_reverse(result);
}

gchar *keybind_get_command(	Application *app,
				gboolean mouse,
				guint modifier,
				guint keyval )
{
	GSList *iter = app->keybind_list;
	keybind *kb = iter->data;

	while(kb && (kb->modifier != modifier || kb->keyval != keyval))
	{
		iter = g_slist_next(iter);
		kb = iter?iter->data:NULL;
	}

	return kb?kb->command:NULL;
}

GSList *keybind_parse_config_with_defaults(	const gchar *config_path,
						gboolean *propexp )
{
	const gchar *default_kb[] = DEFAULT_KEYBINDS;
	gint index;
	GSList *default_kb_list;
	GSList *config_kb_list;

	index = -1;
	default_kb_list = NULL;

	config_kb_list =	config_path?
				keybind_parse_config(config_path, propexp):
				NULL;

	while(default_kb[++index])
	{
		keybind *keybind;
		gboolean line_propexp;

		keybind = keybind_parse_config_line(	default_kb[index],
							&line_propexp );

		if(keybind)
		{
			default_kb_list
				= g_slist_append(default_kb_list, keybind);
		}

		if(propexp && line_propexp)
		{
			*propexp = TRUE;
		}
	}

	/* Key bindings from configuration file gets prority over default ones
	 */
	return	config_kb_list?
		g_slist_concat(config_kb_list, default_kb_list):
		default_kb_list;
}
