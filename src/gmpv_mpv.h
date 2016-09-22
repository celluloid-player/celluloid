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
#include "gmpv_geometry.h"

G_BEGIN_DECLS

#define GMPV_TYPE_MPV_OBJ (gmpv_mpv_get_type())

G_DECLARE_FINAL_TYPE(GmpvMpv, gmpv_mpv, GMPV, MPV_OBJ, GObject)

typedef struct _GmpvMpvState GmpvMpvState;

struct _GmpvMpvState
{
	gboolean ready;
	gboolean paused;
	gboolean loaded;
	gboolean new_file;
	gboolean init_load;
};

GmpvMpv *gmpv_mpv_new(GmpvPlaylist *playlist, gint64 wid);
gint gmpv_mpv_command(GmpvMpv *mpv, const gchar **cmd);
gint gmpv_mpv_command_string(GmpvMpv *mpv, const gchar *cmd);
gint gmpv_mpv_get_property(	GmpvMpv *mpv,
				const gchar *name,
				mpv_format format,
				void *data );
gchar *gmpv_mpv_get_property_string(GmpvMpv *mpv, const gchar *name);
gboolean gmpv_mpv_get_property_flag(GmpvMpv *mpv, const gchar *name);
gint gmpv_mpv_set_property(	GmpvMpv *mpv,
				const gchar *name,
				mpv_format format,
				void *data );
gint gmpv_mpv_set_property_flag(	GmpvMpv *mpv,
					const gchar *name,
					gboolean value);
gint gmpv_mpv_set_property_string(	GmpvMpv *mpv,
					const gchar *name,
					const char *data );
void gmpv_mpv_set_event_callback(	GmpvMpv *mpv,
					void (*func)(mpv_event *, void *),
					void *data );
void gmpv_mpv_set_opengl_cb_callback(	GmpvMpv *mpv,
					mpv_opengl_cb_update_fn func,
					void *data );
const GmpvMpvState *gmpv_mpv_get_state(GmpvMpv *mpv);
GmpvGeometry *gmpv_mpv_get_geometry(GmpvMpv *mpv);
gdouble gmpv_mpv_get_autofit_ratio(GmpvMpv *mpv);
GmpvPlaylist *gmpv_mpv_get_playlist(GmpvMpv *mpv);
mpv_handle *gmpv_mpv_get_mpv_handle(GmpvMpv *mpv);
mpv_opengl_cb_context *gmpv_mpv_get_opengl_cb_context(GmpvMpv *mpv);
void gmpv_mpv_initialize(GmpvMpv *mpv);
void gmpv_mpv_init_gl(GmpvMpv *mpv);
void gmpv_mpv_reset(GmpvMpv *mpv);
void gmpv_mpv_quit(GmpvMpv *mpv);
void gmpv_mpv_load(	GmpvMpv *mpv,
			const gchar *uri,
			gboolean append,
			gboolean update );
void gmpv_mpv_load_list(	GmpvMpv *mpv,
				const gchar **uri_list,
				gboolean append,
				gboolean update );
void gmpv_mpv_free(gpointer data);

G_END_DECLS

#endif
