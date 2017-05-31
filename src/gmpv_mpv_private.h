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

#include "gmpv_mpv_opt.h"

G_BEGIN_DECLS

typedef struct _GmpvMpvState GmpvMpvState;

struct _GmpvMpvState
{
	gboolean ready;
	gboolean loaded;
	gboolean new_file;
	gboolean init_load;
};

enum
{
	PROP_0,
	PROP_WID,
	N_PROPERTIES
};

struct _GmpvMpv
{
	GObject parent;
	GmpvMpvState state;
	mpv_handle *mpv_ctx;
	mpv_opengl_cb_context *opengl_ctx;
	GPtrArray *playlist;
	GPtrArray *metadata;
	GPtrArray *track_list;
	gchar *tmp_input_file;
	GSList *log_level_list;
	gdouble autofit_ratio;
	GmpvGeometry *geometry;
	gboolean init_vo_config;
	gboolean force_opengl;
	gboolean use_opengl;
	gint64 wid;
	void *opengl_cb_callback_data;
	void (*opengl_cb_callback)(void *data);
};

struct _GmpvMpvClass
{
	GObjectClass parent_class;
};

void mpv_check_error(int status);

G_END_DECLS

#endif
