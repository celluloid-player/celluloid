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

G_BEGIN_DECLS

struct _GmpvApplication
{
	GtkApplication parent;
	GmpvController *controller;
	GmpvModel *model;
	GmpvView *view;
	gboolean enqueue;
	gboolean no_existing_session;
	GQueue *action_queue;
	gchar **files;
	guint inhibit_cookie;
	GmpvMainWindow *gui;
};

struct _GmpvApplicationClass
{
	GtkApplicationClass parent_class;
};

G_END_DECLS

#endif
