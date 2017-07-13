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

#ifndef APPLICATION_PRIVATE_H
#define APPLICATION_PRIVATE_H

#include "gmpv_controller.h"
#include "gmpv_model.h"
#include "gmpv_view.h"
#include "mpris/gmpv_mpris.h"
#include "media_keys/gmpv_media_keys.h"

G_BEGIN_DECLS

struct _GmpvApplication
{
	GtkApplication parent;
	GmpvController *controller;
	gboolean enqueue;
	gboolean no_existing_session;
	GQueue *action_queue;
	guint inhibit_cookie;
	GSettings *settings;
	GmpvMpris *mpris;
	GmpvMediaKeys *media_keys;
};

struct _GmpvApplicationClass
{
	GtkApplicationClass parent_class;
};

G_END_DECLS

#endif
