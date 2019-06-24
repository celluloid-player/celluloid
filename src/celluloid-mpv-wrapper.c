/*
 * Copyright (c) 2016-2019 gnome-mpv
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

#include "celluloid-mpv-wrapper.h"
#include "celluloid-mpv-private.h"

gint
celluloid_mpv_command(CelluloidMpv *mpv, const gchar **cmd)
{
	CelluloidMpvPrivate *priv = get_private(mpv);
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

gint
celluloid_mpv_command_async(CelluloidMpv *mpv, const gchar **cmd)
{
	CelluloidMpvPrivate *priv = get_private(mpv);
	gint rc = MPV_ERROR_UNINITIALIZED;

	if(priv->mpv_ctx)
	{
		rc = mpv_command_async(priv->mpv_ctx, 0, cmd);
	}

	if(rc < 0)
	{
		gchar *cmd_str = g_strjoinv(" ", (gchar **)cmd);

		g_warning(	"Failed to dispatch async mpv command \"%s\". "
				"Reason: %s.",
				cmd_str,
				mpv_error_string(rc) );

		g_free(cmd_str);
	}

	return rc;
}

gint
celluloid_mpv_command_string(CelluloidMpv *mpv, const gchar *cmd)
{
	CelluloidMpvPrivate *priv = get_private(mpv);
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

gint
celluloid_mpv_set_option_string(	CelluloidMpv *mpv,
					const gchar *name,
					const gchar *value )
{
	return mpv_set_option_string(get_private(mpv)->mpv_ctx, name, value);
}

gint
celluloid_mpv_get_property(	CelluloidMpv *mpv,
				const gchar *name,
				mpv_format format,
				void *data )
{
	CelluloidMpvPrivate *priv = get_private(mpv);
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

gchar *
celluloid_mpv_get_property_string(CelluloidMpv *mpv, const gchar *name)
{
	CelluloidMpvPrivate *priv = get_private(mpv);
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

gboolean
celluloid_mpv_get_property_flag(CelluloidMpv *mpv, const gchar *name)
{
	CelluloidMpvPrivate *priv = get_private(mpv);
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

gint
celluloid_mpv_set_property(	CelluloidMpv *mpv,
				const gchar *name,
				mpv_format format,
				void *data )
{
	CelluloidMpvPrivate *priv = get_private(mpv);
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

gint
celluloid_mpv_set_property_string(	CelluloidMpv *mpv,
					const gchar *name,
					const char *data )
{
	CelluloidMpvPrivate *priv = get_private(mpv);
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

gint
celluloid_mpv_set_property_flag(	CelluloidMpv *mpv,
					const gchar *name,
					gboolean value )
{
	CelluloidMpvPrivate *priv = get_private(mpv);
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

void
celluloid_mpv_set_render_update_callback(	CelluloidMpv *mpv,
						mpv_render_update_fn func,
						void *data )
{
	CelluloidMpvPrivate *priv = get_private(mpv);

	priv->render_update_callback = func;
	priv->render_update_callback_data = data;

	if(priv->render_ctx)
	{
		mpv_render_context_set_update_callback
			(priv->render_ctx, func, data);
	}
}

guint64
celluloid_mpv_render_context_update(CelluloidMpv *mpv)
{
	return mpv_render_context_update(get_private(mpv)->render_ctx);
}

gint
celluloid_mpv_load_config_file(CelluloidMpv *mpv, const gchar *filename)
{
	return mpv_load_config_file(get_private(mpv)->mpv_ctx, filename);
}

gint
celluloid_mpv_observe_property(	CelluloidMpv *mpv,
				guint64 reply_userdata,
				const gchar *name,
				mpv_format format )
{
	return mpv_observe_property(	get_private(mpv)->mpv_ctx,
					reply_userdata,
					name,
					format );
}

gint
celluloid_mpv_request_log_messages(CelluloidMpv *mpv, const gchar *min_level)
{
	return mpv_request_log_messages(get_private(mpv)->mpv_ctx, min_level);
}
