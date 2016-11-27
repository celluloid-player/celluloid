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

#include <string.h>
#include <mpv/client.h>

#include "gmpv_mpv_opt.h"
#include "gmpv_mpv.h"
#include "gmpv_mpv_private.h"
#include "gmpv_mpv_wrapper.h"
#include "gmpv_def.h"

static gboolean parse_geom_token(const gchar **iter, GValue *value);
static gboolean parse_dim_string(const gchar *geom_str, gint64 dim[2]);
static gboolean parse_pos_token_prefix(const gchar **iter, gchar **prefix);
static gboolean parse_pos_token(	const gchar **iter,
					GValue *value,
					gboolean *flip );
static gboolean parse_pos_string(	const gchar *geom_str,
					gboolean flip[2],
					GValue pos[2] );
static void parse_geom_string(	GmpvMpv *mpv,
				const gchar *geom_str,
				GmpvGeometry **geom );

static gboolean parse_geom_token(const gchar **iter, GValue *value)
{
	gchar *end = NULL;
	gint64 token_value = g_ascii_strtoll(*iter, &end, 10);

	if(end)
	{
		if(*end == '%')
		{
			g_value_init(value, G_TYPE_DOUBLE);
			g_value_set_double(value, (gdouble)token_value/100.0);

			end++;
		}
		else
		{
			g_value_init(value, G_TYPE_INT64);
			g_value_set_int64(value, token_value);
		}

		*iter = end;
	}

	return !!end;
}

static gboolean parse_dim_string(const gchar *geom_str, gint64 dim[2])
{
	GdkScreen *screen = gdk_screen_get_default();
	gint screen_dim[2] = {0, 0};
	gdouble multiplier[2] = {-1, -1};
	gchar **tokens = g_strsplit(geom_str, "x", 2);
	gint i = -1;

	dim[0] = -1;
	dim[1] = -1;
	screen_dim[0] = gdk_screen_get_width(screen);
	screen_dim[1] = gdk_screen_get_height(screen);

	while(tokens && tokens[++i] && i < 3)
	{
		gint value = (gint)g_ascii_strtoll(tokens[i], NULL, 0);

		if((i == 0 && value > 0) || i == 1)
		{
			if(tokens[i][strnlen(tokens[i], 256)-1] == '%')
			{
				multiplier[i] = value/100.0;
			}
			else if(i == 1 && multiplier[0] != -1)
			{
				multiplier[i] = multiplier[0];
			}

			if(multiplier[i] == -1)
			{
				dim[i] = value;
			}
			else
			{
				dim[i] = (gint)(multiplier[i]*screen_dim[i]);
			}
		}
	}

	g_strfreev(tokens);

	return (dim[0] > 0 && dim[1] > 0);
}

static gboolean parse_pos_token_prefix(const gchar **iter, gchar **prefix)
{
	const gchar *start = *iter;
	const gchar *end = *iter;
	gboolean rc = FALSE;

	while(*end && (*end == '+' || *end == '-'))
	{
		end++;
	}

	rc = (end-start <= 2);

	if(rc)
	{
		if(end-start == 2)
		{
			end--;
		}

		*prefix = g_strndup(start, (gsize)(end-start));
		*iter = end;
	}

	return rc;
}

static gboolean parse_pos_token(	const gchar **iter,
					GValue *value,
					gboolean *flip )
{
	gchar *prefix = NULL;
	gboolean rc = TRUE;

	rc = rc && parse_pos_token_prefix(iter, &prefix);
	rc = rc && parse_geom_token(iter, value);
	*flip = (prefix && *prefix == '-');

	g_free(prefix);

	return rc;
}

static gboolean parse_pos_string(	const gchar *geom_str,
					gboolean flip[2],
					GValue pos[2] )
{
	gboolean rc = TRUE;

	rc = rc && parse_pos_token(&geom_str, &pos[0], &flip[0]);
	rc = rc && parse_pos_token(&geom_str, &pos[1], &flip[1]);

	return rc;
}

static void parse_geom_string(	GmpvMpv *mpv,
				const gchar *geom_str,
				GmpvGeometry **geom )
{
	gint64 dim[2] = {0, 0};
	GValue pos[2] = {G_VALUE_INIT, G_VALUE_INIT};
	gboolean flip[2] = {FALSE, FALSE};
	gboolean dim_valid = FALSE;
	gboolean pos_valid = FALSE;

	if(!!geom_str)
	{
		*geom = g_new0(GmpvGeometry, 1);

		if(geom_str[0] != '+' && geom_str[0] != '-')
		{
			dim_valid = parse_dim_string(geom_str, dim);
		}

		/* Move the beginning of the string to the 'position' section */
		while(*geom_str && *geom_str != '+' && *geom_str != '-')
		{
			geom_str++;
		}

		pos_valid = parse_pos_string(geom_str, flip, pos);

		(*geom)->flags |= !dim_valid*GMPV_GEOMETRY_IGNORE_DIM;
		(*geom)->flags |= !pos_valid*GMPV_GEOMETRY_IGNORE_POS;
		(*geom)->flags |= flip[0]*GMPV_GEOMETRY_FLIP_X;
		(*geom)->flags |= flip[1]*GMPV_GEOMETRY_FLIP_Y;

		(*geom)->x = pos[0];
		(*geom)->y = pos[1];
		(*geom)->width = dim[0];
		(*geom)->height = dim[1];
	}
	else
	{
		*geom = NULL;

		g_warning("Failed to parse geometry string: %s", geom_str);
	}
}

void gmpv_mpv_opt_handle_geometry(GmpvMpv *mpv)
{
	gchar *opt_buf;

	opt_buf  =	mpv_get_property_string
			(mpv->mpv_ctx, "options/geometry");

	if(opt_buf)
	{
		parse_geom_string(mpv, opt_buf, &mpv->geometry);

		mpv_free(opt_buf);
	}
}

void gmpv_mpv_opt_handle_autofit(GmpvMpv *mpv)
{
	gchar *scale_str = NULL;
	gchar *autofit_str = NULL;
	gchar *larger_str = NULL;
	gchar *smaller_str = NULL;
	gboolean scale_set = FALSE;
	gboolean autofit_set = FALSE;
	gboolean larger_set = FALSE;
	gboolean smaller_set = FALSE;

	scale_str =	mpv_get_property_string
			(mpv->mpv_ctx, "options/window-scale");
	autofit_str =	mpv_get_property_string
			(mpv->mpv_ctx, "options/autofit");
	larger_str =	mpv_get_property_string
			(mpv->mpv_ctx, "options/autofit-larger");
	smaller_str =	mpv_get_property_string
			(mpv->mpv_ctx, "options/autofit-smaller");

	scale_set = scale_str && scale_str[0] != '\0';
	autofit_set = autofit_str && autofit_str[0] != '\0';
	larger_set = larger_str && larger_str[0] != '\0';
	smaller_set = smaller_str && smaller_str[0] != '\0';

	if(scale_set || autofit_set || larger_set || smaller_set)
	{
		gint64 larger_dim[2] = {G_MAXINT, G_MAXINT};
		gint64 smaller_dim[2] = {0, 0};
		gint64 autofit_dim[2] = {0, 0};
		gint64 vid_dim[2];
		gdouble ratio[2];
		gdouble scale = 1;
		gint rc = 0;

		rc |= mpv_get_property(	mpv->mpv_ctx,
					"dwidth",
					MPV_FORMAT_INT64,
					&vid_dim[0] );
		rc |= mpv_get_property(	mpv->mpv_ctx,
					"dheight",
					MPV_FORMAT_INT64,
					&vid_dim[1] );

		if(rc >= 0)
		{
			g_debug(	"Retrieved video size: "
					"%" G_GINT64_FORMAT "x"
					"%" G_GINT64_FORMAT,
					vid_dim[0], vid_dim[1] );
		}

		if(rc >= 0 && scale_set)
		{
			g_debug(	"Retrieved option --window-scale=%s",
					scale_str);

			/* This should never fail since mpv_set_option() will
			 * refuse to set invalid values.
			 */
			scale = g_ascii_strtod(scale_str, NULL);
		}

		if(rc >= 0 && larger_set)
		{
			g_debug(	"Retrieved option --autofit-larger=%s",
					larger_str);

			parse_dim_string(larger_str, larger_dim);
		}

		if(rc >= 0 && smaller_set)
		{
			g_debug(	"Retrieved option --autofit-smaller=%s",
					smaller_str);

			parse_dim_string(smaller_str, smaller_dim);
		}

		if(rc >= 0)
		{
			if(autofit_set)
			{
				g_debug(	"Retrieved option --autofit=%s",
						autofit_str );

				parse_dim_string(autofit_str, autofit_dim);
			}
			else
			{
				autofit_dim[0] = vid_dim[0];
				autofit_dim[1] = vid_dim[1];
			}

			if(scale_set)
			{
				autofit_dim[0]
					= (gint64)(scale*(gdouble)autofit_dim[0]);
				autofit_dim[1]
					= (gint64)(scale*(gdouble)autofit_dim[1]);
			}
		}

		if(rc >= 0)
		{
			autofit_dim[0] = MIN(autofit_dim[0], larger_dim[0]);
			autofit_dim[1] = MIN(autofit_dim[1], larger_dim[1]);

			autofit_dim[0] = MAX(autofit_dim[0], smaller_dim[0]);
			autofit_dim[1] = MAX(autofit_dim[1], smaller_dim[1]);

			if(!autofit_set
			&& autofit_dim[0] == vid_dim[0]
			&& autofit_dim[1] == vid_dim[1])
			{
				/* Do not resize if --autofit is not set and the
				 * video size does not exceed the limits imposed
				 * by --autofit-larger and --autofit-smaller.
				 */
				ratio[0] = scale_set?scale:0;
				ratio[1] = scale_set?scale:0;
			}
			else
			{
				g_debug(	"Target video area size: "
						"%" G_GINT64_FORMAT
						"x%" G_GINT64_FORMAT,
						autofit_dim[0], autofit_dim[1] );

				ratio[0] =	(gdouble)autofit_dim[0]/
						(gdouble)vid_dim[0];
				ratio[1] =	(gdouble)autofit_dim[1]/
						(gdouble)vid_dim[1];
			}
		}

		if(rc >= 0 && ratio[0] > 0 && ratio[1] > 0)
		{
			/* Resize the window so that it is as big as possible
			 *  while preseving the aspect ratio.
			 */
			mpv->autofit_ratio = MIN(ratio[0], ratio[1]);

			g_debug(	"Set video size multiplier to %f",
					mpv->autofit_ratio );
		}
		else
		{
			mpv->autofit_ratio = -1;
		}
	}

	mpv_free(scale_str);
	mpv_free(autofit_str);
	mpv_free(larger_str);
	mpv_free(smaller_str);
}

void gmpv_mpv_opt_handle_fs(GmpvMpv *mpv)
{
	gchar *optbuf;

	optbuf = gmpv_mpv_get_property_string(mpv, "options/fs");

	if(g_strcmp0(optbuf, "yes") == 0)
	{
		gmpv_mpv_command_string
			(mpv, "script-message gmpv-action toggle-fullscreen");
	}

	mpv_free(optbuf);
}

void gmpv_mpv_opt_handle_msg_level(GmpvMpv *mpv)
{
	const struct
	{
		gchar *name;
		mpv_log_level level;
	}
	level_map[] = {	{"no", MPV_LOG_LEVEL_NONE},
			{"fatal", MPV_LOG_LEVEL_FATAL},
			{"error", MPV_LOG_LEVEL_ERROR},
			{"warn", MPV_LOG_LEVEL_WARN},
			{"info", MPV_LOG_LEVEL_INFO},
			{"v", MPV_LOG_LEVEL_V},
			{"debug", MPV_LOG_LEVEL_DEBUG},
			{"trace", MPV_LOG_LEVEL_TRACE},
			{NULL, MPV_LOG_LEVEL_NONE} };

	gchar *optbuf = NULL;
	gchar **tokens = NULL;
	mpv_log_level min_level = DEFAULT_LOG_LEVEL;
	gint i;

	optbuf = mpv_get_property_string(mpv->mpv_ctx, "options/msg-level");

	if(optbuf)
	{
		tokens = g_strsplit(optbuf, ",", 0);
	}

	if(mpv->log_level_list)
	{
		g_slist_free_full(mpv->log_level_list, g_free);

		mpv->log_level_list = NULL;
	}

	for(i = 0; tokens && tokens[i]; i++)
	{
		gchar **pair = g_strsplit(tokens[i], "=", 2);
		module_log_level *level = g_malloc(sizeof(module_log_level));
		gboolean found = FALSE;
		gint j;

		level->prefix = g_strdup(pair[0]);

		for(j = 0; level_map[j].name && !found; j++)
		{
			if(g_strcmp0(pair[1], level_map[j].name) == 0)
			{
				level->level = level_map[j].level;
				found = TRUE;
			}
		}


		/* Ignore if the given level is invalid */
		if(found)
		{
			/* Lower log levels have higher values */
			if(level->level > min_level)
			{
				min_level = level->level;
			}

			if(g_strcmp0(level->prefix, "all") != 0)
			{
				mpv->log_level_list
					= g_slist_append
						(mpv->log_level_list, level);
			}
		}

		g_strfreev(pair);
	}

	for(i = 0; level_map[i].level != min_level; i++);

	mpv_check_error
		(mpv_request_log_messages(mpv->mpv_ctx, level_map[i].name));

	mpv_free(optbuf);
	g_strfreev(tokens);
}

