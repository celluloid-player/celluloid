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

#ifndef MPV_WRAPPER_H
#define MPV_WRAPPER_H

#include <glib.h>
#include <mpv/client.h>
#include <mpv/render_gl.h>

#include "celluloid-mpv.h"
#include "celluloid-mpv.h"

gint
celluloid_mpv_command(CelluloidMpv *mpv, const gchar **cmd);

gint
celluloid_mpv_command_async(CelluloidMpv *mpv, const gchar **cmd);

gint
celluloid_mpv_command_string(CelluloidMpv *mpv, const gchar *cmd);

gint
celluloid_mpv_set_option_string(	CelluloidMpv *mpv,
					const gchar *name,
					const gchar *value );

gint
celluloid_mpv_get_property(	CelluloidMpv *mpv,
				const gchar *name,
				mpv_format format,
				void *data );

gchar *
celluloid_mpv_get_property_string(CelluloidMpv *mpv, const gchar *name);

gboolean
celluloid_mpv_get_property_flag(CelluloidMpv *mpv, const gchar *name);

gint
celluloid_mpv_set_property(	CelluloidMpv *mpv,
				const gchar *name,
				mpv_format format,
				void *data );

gint
celluloid_mpv_set_property_flag(	CelluloidMpv *mpv,
					const gchar *name,
					gboolean value);

gint
celluloid_mpv_set_property_string(	CelluloidMpv *mpv,
					const gchar *name,
					const char *data );

void
celluloid_mpv_set_event_callback(	CelluloidMpv *mpv,
					void (*func)(mpv_event *, void *),
					void *data );

void
celluloid_mpv_set_render_update_callback(	CelluloidMpv *mpv,
						mpv_render_update_fn func,
						void *data );

guint64
celluloid_mpv_render_context_update(CelluloidMpv *mpv);

gint
celluloid_mpv_load_config_file(CelluloidMpv *mpv, const gchar *filename);

gint
celluloid_mpv_observe_property(	CelluloidMpv *mpv,
				guint64 reply_userdata,
				const gchar *name,
				mpv_format format );

gint
celluloid_mpv_request_log_messages(CelluloidMpv *mpv, const gchar *min_level);

#endif
