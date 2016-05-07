/*
 * Copyright (c) 2014, 2016 gnome-mpv
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

#ifndef MPV_OBJ_H
#define MPV_OBJ_H

#include <gtk/gtk.h>
#include <mpv/client.h>
#include <mpv/opengl_cb.h>

#include "gmpv_playlist.h"

G_BEGIN_DECLS

#define GMPV_TYPE_MPV_OBJ (gmpv_mpv_obj_get_type())

G_DECLARE_FINAL_TYPE(GmpvMpvObj, gmpv_mpv_obj, GMPV, MPV_OBJ, GObject)

typedef struct _GmpvMpvObjState GmpvMpvObjState;

struct _GmpvMpvObjState
{
	gboolean ready;
	gboolean paused;
	gboolean loaded;
	gboolean new_file;
	gboolean init_load;
};

GmpvMpvObj *gmpv_mpv_obj_new(GmpvPlaylist *playlist, gint64 wid);
gint gmpv_mpv_obj_command(GmpvMpvObj *mpv, const gchar **cmd);
gint gmpv_mpv_obj_command_string(GmpvMpvObj *mpv, const gchar *cmd);
gint gmpv_mpv_obj_get_property(	GmpvMpvObj *mpv,
				const gchar *name,
				mpv_format format,
				void *data );
gchar *gmpv_mpv_obj_get_property_string(GmpvMpvObj *mpv, const gchar *name);
gboolean gmpv_mpv_obj_get_property_flag(GmpvMpvObj *mpv, const gchar *name);
gint gmpv_mpv_obj_set_property(	GmpvMpvObj *mpv,
				const gchar *name,
				mpv_format format,
				void *data );
gint gmpv_mpv_obj_set_property_flag(	GmpvMpvObj *mpv,
					const gchar *name,
					gboolean value);
gint gmpv_mpv_obj_set_property_string(	GmpvMpvObj *mpv,
					const gchar *name,
					const char *data );
void gmpv_mpv_obj_set_event_callback(	GmpvMpvObj *mpv,
					void (*func)(mpv_event *, void *),
					void *data );
void gmpv_mpv_obj_set_opengl_cb_callback(	GmpvMpvObj *mpv,
					mpv_opengl_cb_update_fn func,
					void *data );
void mpv_check_error(int status);
gboolean gmpv_mpv_obj_is_loaded(GmpvMpvObj *mpv);
void gmpv_mpv_obj_get_state(GmpvMpvObj *mpv, GmpvMpvObjState *state);
gdouble gmpv_mpv_obj_get_autofit_ratio(GmpvMpvObj *mpv);
GmpvPlaylist *gmpv_mpv_obj_get_playlist(GmpvMpvObj *mpv);
mpv_handle *gmpv_mpv_obj_get_mpv_handle(GmpvMpvObj *mpv);
mpv_opengl_cb_context *gmpv_mpv_obj_get_opengl_cb_context(GmpvMpvObj *mpv);
void gmpv_mpv_obj_initialize(GmpvMpvObj *mpv);
void gmpv_mpv_obj_reset(GmpvMpvObj *mpv);
void gmpv_mpv_obj_quit(GmpvMpvObj *mpv);
void gmpv_mpv_obj_load(	GmpvMpvObj *mpv,
			const gchar *uri,
			gboolean append,
			gboolean update );
void gmpv_mpv_obj_load_list(	GmpvMpvObj *mpv,
			const gchar **uri_list,
			gboolean append,
			gboolean update );

G_END_DECLS

#endif
