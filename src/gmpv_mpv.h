/*
 * Copyright (c) 2014, 2016-2017 gnome-mpv
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

#ifndef MPV_H
#define MPV_H

#include <glib.h>
#include <glib-object.h>
#include <mpv/client.h>
#include <mpv/opengl_cb.h>

#include "gmpv_common.h"

G_BEGIN_DECLS

#define GMPV_TYPE_MPV (gmpv_mpv_get_type())

G_DECLARE_DERIVABLE_TYPE(GmpvMpv, gmpv_mpv, GMPV, MPV, GObject)

struct _GmpvMpvClass
{
	GObjectClass parent_class;
	void (*mpv_event)(GmpvMpv *mpv, gint event_id, gpointer event_data);
	void (*mpv_property_changed)(	GmpvMpv *mpv,
					const gchar *name,
					gpointer value );
};

GmpvMpv *gmpv_mpv_new(gint64 wid);
mpv_opengl_cb_context *gmpv_mpv_get_opengl_cb_context(GmpvMpv *mpv);
gboolean gmpv_mpv_get_use_opengl_cb(GmpvMpv *mpv);
GPtrArray *gmpv_mpv_get_metadata(GmpvMpv *mpv);
GPtrArray *gmpv_mpv_get_playlist(GmpvMpv *mpv);
void gmpv_mpv_initialize(GmpvMpv *mpv);
void gmpv_mpv_init_gl(GmpvMpv *mpv);
void gmpv_mpv_reset(GmpvMpv *mpv);
void gmpv_mpv_quit(GmpvMpv *mpv);
void gmpv_mpv_load_track(GmpvMpv *mpv, const gchar *uri, TrackType type);
void gmpv_mpv_load_file(GmpvMpv *mpv, const gchar *uri, gboolean append);
void gmpv_mpv_load(GmpvMpv *mpv, const gchar *uri, gboolean append);

G_END_DECLS

#endif
