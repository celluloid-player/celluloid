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

#ifndef MPV_WRAPPER_H
#define MPV_WRAPPER_H

#include <glib.h>
#include <mpv/client.h>
#include <mpv/opengl_cb.h>

#include "gmpv_mpv.h"

gint gmpv_mpv_command(GmpvMpv *mpv, const gchar **cmd);
gint gmpv_mpv_command_string(GmpvMpv *mpv, const gchar *cmd);
gint gmpv_mpv_set_option_string(	GmpvMpv *mpv,
					const gchar *name,
					const gchar *value );
gint gmpv_mpv_get_property(	GmpvMpv *mpv,
				const gchar *name,
				mpv_format format,
				void *data );
gchar *gmpv_mpv_get_property_string(GmpvMpv *mpv, const gchar *name);
gboolean gmpv_mpv_get_property_flag(GmpvMpv *mpv, const gchar *name);
gint gmpv_mpv_set_property(	GmpvMpv *mpv,
				const gchar *name,
				mpv_format format,
				void *data );
gint gmpv_mpv_set_property_flag(	GmpvMpv *mpv,
					const gchar *name,
					gboolean value);
gint gmpv_mpv_set_property_string(	GmpvMpv *mpv,
					const gchar *name,
					const char *data );
void gmpv_mpv_set_event_callback(	GmpvMpv *mpv,
					void (*func)(mpv_event *, void *),
					void *data );
void gmpv_mpv_set_opengl_cb_callback(	GmpvMpv *mpv,
					mpv_opengl_cb_update_fn func,
					void *data );
gint gmpv_mpv_load_config_file(GmpvMpv *mpv, const gchar *filename);
gint gmpv_mpv_observe_property(	GmpvMpv *mpv,
				guint64 reply_userdata,
				const gchar *name,
				mpv_format format );

#endif
