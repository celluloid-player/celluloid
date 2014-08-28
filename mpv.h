/*
 * Copyright (c) 2014 gnome-mpv
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

#ifndef MPV_H
#define MPV_H

#include "common.h"

void *mpv_event_handler(void *data);
void mpv_log_handler(gmpv_handle *ctx, mpv_event_log_message* message);
void mpv_check_error(int status);
gboolean mpv_load_gui_update(gpointer data);
gboolean mpv_load_from_ctx(gpointer data);
gint mpv_apply_args(mpv_handle *mpv_ctx, char *args);
void mpv_init(gmpv_handle *ctx, gint64 vid_area_wid);

void mpv_load(	gmpv_handle *ctx,
		const gchar *uri,
		gboolean append,
		gboolean update );

#endif
