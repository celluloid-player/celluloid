/*
 * Copyright (c) 2017-2021 gnome-mpv
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

#ifndef CONTROLLER_PRIVATE_H
#define CONTROLLER_PRIVATE_H

#include "celluloid-model.h"
#include "celluloid-view.h"
#include "mpris/celluloid-mpris.h"
#include "media-keys/celluloid-media-keys.h"

G_BEGIN_DECLS

enum
{
	PROP_0,
	PROP_APP,
	PROP_READY,
	PROP_IDLE,
	PROP_USE_SKIP_BUTTONS_FOR_PLAYLIST,
	N_PROPERTIES
};

struct _CelluloidController
{
	GObject parent;

	GtkEventController *key_controller;

	CelluloidApplication *app;
	CelluloidModel *model;
	CelluloidView *view;
	gboolean ready;
	gboolean idle;
	gboolean use_skip_buttons_for_playlist;
	gint64 target_playlist_pos;
	guint update_seekbar_id;
	guint resize_timeout_tag;
	GBinding *skip_buttons_binding;
	GSettings *settings;
	CelluloidMediaKeys *media_keys;
	CelluloidMpris *mpris;
};

struct _CelluloidControllerClass
{
	GObjectClass parent_class;
};

G_END_DECLS

#endif
