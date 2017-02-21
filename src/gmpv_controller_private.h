/*
 * Copyright (c) 2017 gnome-mpv
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

#ifndef CONTROLLER_PRIVATE_H
#define CONTROLLER_PRIVATE_H

#include "gmpv_model.h"
#include "gmpv_view.h"

G_BEGIN_DECLS

enum
{
	PROP_0,
	PROP_MODEL,
	PROP_VIEW,
	PROP_AID,
	PROP_VID,
	PROP_SID,
	PROP_READY,
	PROP_LOOP,
	PROP_IDLE,
	N_PROPERTIES
};

struct _GmpvController
{
	GObject parent;
	GmpvModel *model;
	GmpvView *view;
	gint aid;
	gint vid;
	gint sid;
	gboolean ready;
	gboolean loop;
	gboolean idle;
	GQueue *action_queue;
	gchar **files;
	guint inhibit_cookie;
	gint64 target_playlist_pos;
};

struct _GmpvControllerClass
{
	GObjectClass parent_class;
};

G_END_DECLS

#endif
