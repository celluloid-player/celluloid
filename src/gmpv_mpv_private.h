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

#ifndef MPV_OBJ_PRIVATE_H
#define MPV_OBJ_PRIVATE_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _GmpvMpvPrivate GmpvMpvPrivate;

enum
{
	PROP_0,
	PROP_WID,
	PROP_READY,
	N_PROPERTIES
};

struct _GmpvMpvPrivate
{
	mpv_handle *mpv_ctx;
	mpv_opengl_cb_context *opengl_ctx;
	GPtrArray *playlist;
	gboolean ready;
	gboolean loaded;
	gboolean new_file;
	gchar *tmp_input_file;
	GSList *log_level_list;
	gboolean init_vo_config;
	gboolean force_opengl;
	gboolean use_opengl;
	gint64 wid;
	void *opengl_cb_callback_data;
	void (*opengl_cb_callback)(void *data);
};

#define get_private(mpv) \
	G_TYPE_INSTANCE_GET_PRIVATE(mpv, GMPV_TYPE_MPV, GmpvMpvPrivate)

void mpv_check_error(int status);

G_END_DECLS

#endif
