/*
 * Copyright (c) 2016-2017 gnome-mpv
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

#include "gmpv_mpv_wrapper.h"
#include "gmpv_mpv_private.h"

gint gmpv_mpv_command(GmpvMpv *mpv, const gchar **cmd)
{
	GmpvMpvPrivate *priv = get_private(mpv);
	gint rc = MPV_ERROR_UNINITIALIZED;

	if(priv->mpv_ctx)
	{
		rc = mpv_command(priv->mpv_ctx, cmd);
	}

	if(rc < 0)
	{
		gchar *cmd_str = g_strjoinv(" ", (gchar **)cmd);

		g_warning(	"Failed to run mpv command \"%s\". Reason: %s.",
				cmd_str,
				mpv_error_string(rc) );

		g_free(cmd_str);
	}

	return rc;
}

gint gmpv_mpv_command_string(GmpvMpv *mpv, const gchar *cmd)
{
	GmpvMpvPrivate *priv = get_private(mpv);
	gint rc = MPV_ERROR_UNINITIALIZED;

	if(priv->mpv_ctx)
	{
		rc = mpv_command_string(priv->mpv_ctx, cmd);
	}

	if(rc < 0)
	{
		g_warning(	"Failed to run mpv command string \"%s\". "
				"Reason: %s.",
				cmd,
				mpv_error_string(rc) );
	}

	return rc;
}

gint gmpv_mpv_set_option_string(	GmpvMpv *mpv,
					const gchar *name,
					const gchar *value )
{
	return mpv_set_option_string(get_private(mpv)->mpv_ctx, name, value);
}

gint gmpv_mpv_get_property(	GmpvMpv *mpv,
				const gchar *name,
				mpv_format format,
				void *data )
{
	GmpvMpvPrivate *priv = get_private(mpv);
	gint rc = MPV_ERROR_UNINITIALIZED;

	if(priv->mpv_ctx)
	{
		rc = mpv_get_property(priv->mpv_ctx, name, format, data);
	}

	if(rc < 0)
	{
		g_info(	"Failed to retrieve property \"%s\" "
			"using mpv format %d. Reason: %s.",
			name,
			format,
			mpv_error_string(rc) );
	}

	return rc;
}

gchar *gmpv_mpv_get_property_string(GmpvMpv *mpv, const gchar *name)
{
	GmpvMpvPrivate *priv = get_private(mpv);
	gchar *value = NULL;

	if(priv->mpv_ctx)
	{
		value = mpv_get_property_string(priv->mpv_ctx, name);
	}

	if(!value)
	{
		g_info("Failed to retrieve property \"%s\" as string.", name);
	}

	return value;
}

gboolean gmpv_mpv_get_property_flag(GmpvMpv *mpv, const gchar *name)
{
	GmpvMpvPrivate *priv = get_private(mpv);
	gboolean value = FALSE;
	gint rc = MPV_ERROR_UNINITIALIZED;

	if(priv->mpv_ctx)
	{
		rc =	mpv_get_property
			(priv->mpv_ctx, name, MPV_FORMAT_FLAG, &value);
	}

	if(rc < 0)
	{
		g_info(	"Failed to retrieve property \"%s\" as flag. "
			"Reason: %s.",
			name,
			mpv_error_string(rc) );
	}

	return value;
}

gint gmpv_mpv_set_property(	GmpvMpv *mpv,
				const gchar *name,
				mpv_format format,
				void *data )
{
	GmpvMpvPrivate *priv = get_private(mpv);
	gint rc = MPV_ERROR_UNINITIALIZED;

	if(priv->mpv_ctx)
	{
		rc = mpv_set_property(priv->mpv_ctx, name, format, data);
	}

	if(rc < 0)
	{
		g_info(	"Failed to set property \"%s\" using mpv format %d. "
			"Reason: %s.",
			name,
			format,
			mpv_error_string(rc) );
	}

	return rc;
}

gint gmpv_mpv_set_property_string(	GmpvMpv *mpv,
					const gchar *name,
					const char *data )
{
	GmpvMpvPrivate *priv = get_private(mpv);
	gint rc = MPV_ERROR_UNINITIALIZED;

	if(priv->mpv_ctx)
	{
		rc = mpv_set_property_string(priv->mpv_ctx, name, data);
	}

	if(rc < 0)
	{
		g_info(	"Failed to set property \"%s\" as string. Reason: %s.",
			name,
			mpv_error_string(rc) );
	}

	return rc;
}

gint gmpv_mpv_set_property_flag(	GmpvMpv *mpv,
					const gchar *name,
					gboolean value )
{
	GmpvMpvPrivate *priv = get_private(mpv);
	gint rc = MPV_ERROR_UNINITIALIZED;

	if(priv->mpv_ctx)
	{
		rc =	mpv_set_property
			(priv->mpv_ctx, name, MPV_FORMAT_FLAG, &value);
	}

	if(rc < 0)
	{
		g_info(	"Failed to set property \"%s\" as flag. Reason: %s.",
			name,
			mpv_error_string(rc) );
	}

	return rc;
}

void gmpv_mpv_set_opengl_cb_callback(	GmpvMpv *mpv,
					mpv_opengl_cb_update_fn func,
					void *data )
{
	GmpvMpvPrivate *priv = get_private(mpv);

	priv->opengl_cb_callback = func;
	priv->opengl_cb_callback_data = data;

	if(priv->opengl_ctx)
	{
		mpv_opengl_cb_set_update_callback(priv->opengl_ctx, func, data);
	}
}

gint gmpv_mpv_load_config_file(GmpvMpv *mpv, const gchar *filename)
{
	return mpv_load_config_file(get_private(mpv)->mpv_ctx, filename);
}

gint gmpv_mpv_observe_property(	GmpvMpv *mpv,
				guint64 reply_userdata,
				const gchar *name,
				mpv_format format )
{
	return mpv_observe_property(	get_private(mpv)->mpv_ctx,
					reply_userdata,
					name,
					format );
}
