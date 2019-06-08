/*
 * Copyright (c) 2017-2019 gnome-mpv
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

#ifndef MODEL_H
#define MODEL_H

#include <glib.h>

#include "celluloid-mpv.h"
#include "celluloid-mpv.h"
#include "celluloid-player.h"
#include "celluloid-player.h"

G_BEGIN_DECLS

#define CELLULOID_TYPE_MODEL (celluloid_model_get_type())

G_DECLARE_FINAL_TYPE(CelluloidModel, celluloid_model, CELLULOID, MODEL, CelluloidPlayer)

CelluloidModel *
celluloid_model_new(gint64 wid);

void
celluloid_model_initialize(CelluloidModel *model, const gchar *options);

void
celluloid_model_reset(CelluloidModel *model);

void
celluloid_model_quit(CelluloidModel *model);

void
celluloid_model_mouse(CelluloidModel *model, gint x, gint y);

void
celluloid_model_key_down(CelluloidModel *model, const gchar* keystr);

void
celluloid_model_key_up(CelluloidModel *model, const gchar* keystr);

void
celluloid_model_key_press(CelluloidModel *model, const gchar* keystr);

void
celluloid_model_reset_keys(CelluloidModel *model);

void
celluloid_model_play(CelluloidModel *model);

void
celluloid_model_pause(CelluloidModel *model);

void
celluloid_model_stop(CelluloidModel *model);

void
celluloid_model_forward(CelluloidModel *model);

void
celluloid_model_rewind(CelluloidModel *model);

void
celluloid_model_next_chapter(CelluloidModel *model);

void
celluloid_model_previous_chapter(CelluloidModel *model);

void
celluloid_model_next_playlist_entry(CelluloidModel *model);

void
celluloid_model_previous_playlist_entry(CelluloidModel *model);

void
celluloid_model_shuffle_playlist(CelluloidModel *model);

void
celluloid_model_seek(CelluloidModel *model, gdouble value);

void
celluloid_model_seek_offset(CelluloidModel *model, gdouble offset);

void
celluloid_model_load_audio_track(CelluloidModel *model, const gchar *filename);

void
celluloid_model_load_subtitle_track(	CelluloidModel *model,
					const gchar *filename );

gdouble
celluloid_model_get_time_position(CelluloidModel *model);

void
celluloid_model_set_playlist_position(CelluloidModel *model, gint64 position);

void
celluloid_model_remove_playlist_entry(CelluloidModel *model, gint64 position);

void
celluloid_model_move_playlist_entry(	CelluloidModel *model,
					gint64 src,
					gint64 dst );

void
celluloid_model_load_file(	CelluloidModel *model,
				const gchar *uri,
				gboolean append );

gboolean
celluloid_model_get_use_opengl_cb(CelluloidModel *model);

void
celluloid_model_initialize_gl(CelluloidModel *model);

void
celluloid_model_render_frame(CelluloidModel *model, gint width, gint height);

void
celluloid_model_get_video_geometry(	CelluloidModel *model,
					gint64 *width,
					gint64 *height );

gchar *
celluloid_model_get_current_path(CelluloidModel *model);

G_END_DECLS

#endif
