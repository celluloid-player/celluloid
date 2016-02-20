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

#ifndef MEDIA_KEYS_H
#define MEDIA_KEYS_H

#include "common.h"

typedef struct media_keys media_keys;

struct media_keys
{
	Application *gmpv_ctx;
	gulong g_signal_sig_id;
	gulong shutdown_sig_id;
	GDBusProxy *proxy;
	GDBusConnection *session_bus_conn;
};

void media_keys_init(Application *gmpv_ctx);

#endif
