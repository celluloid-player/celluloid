/*
 * Copyright (c) 2019 gnome-mpv
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

#include <glib.h>

#include "celluloid-option-parser.h"

static gboolean
eval_match(const GMatchInfo *match_info, GString *result, gpointer data);

static gchar *
unescape(const gchar *str);

static const gchar *
parse_key(const gchar *str, gchar **option);

static const gchar *
parse_value(const gchar *str, gchar **value);

static gboolean
eval_match(const GMatchInfo *match_info, GString *result, gpointer data)
{
	gchar *match = g_match_info_fetch(match_info, 0);
	gchar converted = '\0';

	g_assert(match && match[0] && match[1] && !match[2]);

	switch(match[1])
	{
		case 'a':
		converted = '\a';
		break;

		case 'b':
		converted = '\b';
		break;

		case 'f':
		converted = '\f';
		break;

		case 'n':
		converted = '\n';
		break;

		case 'r':
		converted = '\r';
		break;

		case 't':
		converted = '\t';
		break;

		case 'v':
		converted = '\v';
		break;

		case '\\':
		case '\'':
		case '"':
		case '?':
		converted = match[1];
		break;


		default:
		g_assert_not_reached();
		break;
	}

	g_string_append_c(result, converted);
	g_free(match);

	return FALSE;
}

static gchar *
unescape(const gchar *str)
{
	GRegex *regex = g_regex_new("\\\\.", 0, 0, NULL);
	gssize len = (gssize)strlen(str);
	gchar *result =	g_regex_replace_eval
			(regex, str, len, 0, 0, eval_match, NULL, NULL);

	g_regex_unref(regex);

	return result;
}

static const gchar *
parse_key(const gchar *str, gchar **option)
{
	if(str)
	{
		const gchar *token_start = str;

		if(strncmp(str, "--", 2) == 0)
		{
			str += 2;
		}

		token_start = str;

		while(g_ascii_isalnum(*str) || *str == '-' || *str == '_')
		{
			str++;
		}

		*option = g_strndup(token_start, (gsize)(str - token_start));
	}

	return str;
}

static const gchar *
parse_value(const gchar *str, gchar **value)
{
	if(str)
	{
		gchar *buf = NULL;
		gsize buf_len = 0;
		const gchar *token_start = str;
		gboolean escaping = FALSE;
		gchar quote = '\0';

		if(*str == '"' || *str == '\'')
		{
			quote = *str++;
		}

		token_start = str;

		while(	*str &&
			(escaping || quote != *str) &&
			(quote != '\0' || !g_ascii_isspace(*str)) )
		{
			escaping = (*str == '\\');

			str++;
		}

		if(quote != '\0')
		{
			str++;
		}

		buf_len = (gsize)(str - token_start - (quote != '\0'));
		buf = g_strndup(token_start, buf_len);
		*value = unescape(buf);

		g_free(buf);
	}

	return str;
}

const gchar *
parse_option(const gchar *str, gchar **option, gchar **value)
{
	while(str && g_ascii_isspace(*str))
	{
		str++;
	}

	str = parse_key(str, option);

	if(str && *str == '=')
	{
		str = parse_value(++str, value);
	}

	return str;
}
