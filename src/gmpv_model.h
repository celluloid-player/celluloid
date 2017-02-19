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

#ifndef MODEL_H
#define MODEL_H

#include <glib.h>

#include "gmpv_mpv.h"

G_BEGIN_DECLS

#define GMPV_TYPE_MODEL (gmpv_model_get_type())

G_DECLARE_FINAL_TYPE(GmpvModel, gmpv_model, GMPV, MODEL, GObject)

GmpvModel *gmpv_model_new(GmpvMpv *mpv);
void gmpv_model_initialize(GmpvModel *model);
void gmpv_model_quit(GmpvModel *model);
void gmpv_model_mouse(GmpvModel *model, gint x, gint y);
void gmpv_model_key_down(GmpvModel *model, const gchar* keystr);
void gmpv_model_key_up(GmpvModel *model, const gchar* keystr);
void gmpv_model_key_press(GmpvModel *model, const gchar* keystr);
void gmpv_model_reset_keys(GmpvModel *model);
void gmpv_model_play(GmpvModel *model);
void gmpv_model_pause(GmpvModel *model);
void gmpv_model_stop(GmpvModel *model);
void gmpv_model_forward(GmpvModel *model);
void gmpv_model_rewind(GmpvModel *model);
void gmpv_model_next_chapter(GmpvModel *model);
void gmpv_model_previous_chapter(GmpvModel *model);
void gmpv_model_seek(GmpvModel *model, gdouble value);
void gmpv_model_load_audio_track(GmpvModel *model, const gchar *filename);
void gmpv_model_load_subtitle_track(GmpvModel *model, const gchar *filename);
void gmpv_model_remove_playlist_entry(GmpvModel *model, gint64 position);
void gmpv_model_move_playlist_entry(GmpvModel *model, gint64 src, gint64 dst);
void gmpv_model_load_file(GmpvModel *model, const gchar *uri, gboolean append);
gboolean gmpv_model_get_use_opengl_cb(GmpvModel *model);
void gmpv_model_initialize_gl(GmpvModel *model);
void gmpv_model_render_frame(GmpvModel *model, gint width, gint height);
void gmpv_model_get_video_geometry(	GmpvModel *model,
					gint64 *width,
					gint64 *height );
GmpvPlaylist *gmpv_model_get_playlist(GmpvModel *model);

G_END_DECLS

#endif
