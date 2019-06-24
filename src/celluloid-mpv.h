/*
 * Copyright (c) 2014-2019 gnome-mpv
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

#ifndef MPV_H
#define MPV_H

#include <glib.h>
#include <glib-object.h>
#include <mpv/client.h>
#include <mpv/render_gl.h>

#include "celluloid-common.h"

G_BEGIN_DECLS

#define CELLULOID_TYPE_MPV (celluloid_mpv_get_type())

G_DECLARE_DERIVABLE_TYPE(CelluloidMpv, celluloid_mpv, CELLULOID, MPV, GObject)

struct _CelluloidMpvClass
{
	GObjectClass parent_class;
	void (*mpv_event_notify)(	CelluloidMpv *mpv,
					gint event_id,
					gpointer event_data );
	void (*mpv_log_message)(	CelluloidMpv *mpv,
					mpv_log_level log_level,
					const gchar *prefix,
					const gchar *text );
	void (*mpv_property_changed)(	CelluloidMpv *mpv,
					const gchar *name,
					gpointer value );
	void (*initialize)(CelluloidMpv *mpv);
	void (*load_file)(CelluloidMpv *mpv, const gchar *uri, gboolean append);
	void (*reset)(CelluloidMpv *mpv);
};

CelluloidMpv *
celluloid_mpv_new(gint64 wid);

mpv_render_context *
celluloid_mpv_get_render_context(CelluloidMpv *mpv);

gboolean
celluloid_mpv_get_use_opengl_cb(CelluloidMpv *mpv);

void
celluloid_mpv_initialize(CelluloidMpv *mpv);

void
celluloid_mpv_init_gl(CelluloidMpv *mpv);

void
celluloid_mpv_reset(CelluloidMpv *mpv);

void
celluloid_mpv_quit(CelluloidMpv *mpv);

void
celluloid_mpv_load_track(CelluloidMpv *mpv, const gchar *uri, TrackType type);

void
celluloid_mpv_load_file(CelluloidMpv *mpv, const gchar *uri, gboolean append);

void
celluloid_mpv_load(CelluloidMpv *mpv, const gchar *uri, gboolean append);

G_END_DECLS

#endif
