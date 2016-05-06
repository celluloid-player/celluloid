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

#include "playlist.h"

G_BEGIN_DECLS

#define MPV_OBJ_TYPE (mpv_obj_get_type())

#define	MPV_OBJ(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), MPV_OBJ_TYPE, MpvObj))

#define	MPV_OBJ_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass), MPV_OBJ_TYPE, MpvObjClass))

#define	IS_MPV_OBJ(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), MPV_OBJ_TYPE))

#define	IS_MPV_OBJ_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), MPV_OBJ_TYPE))

typedef struct _MpvObjState MpvObjState;
typedef struct _MpvObj MpvObj;
typedef struct _MpvObjClass MpvObjClass;

struct _MpvObjState
{
	gboolean ready;
	gboolean paused;
	gboolean loaded;
	gboolean new_file;
	gboolean init_load;
};

GType mpv_obj_get_type(void);
MpvObj *mpv_obj_new(Playlist *playlist, gint64 wid);
gint mpv_obj_command(MpvObj *mpv, const gchar **cmd);
gint mpv_obj_command_string(MpvObj *mpv, const gchar *cmd);
gint mpv_obj_get_property(	MpvObj *mpv,
				const gchar *name,
				mpv_format format,
				void *data );
gchar *mpv_obj_get_property_string(MpvObj *mpv, const gchar *name);
gboolean mpv_obj_get_property_flag(MpvObj *mpv, const gchar *name);
gint mpv_obj_set_property(	MpvObj *mpv,
				const gchar *name,
				mpv_format format,
				void *data );
gint mpv_obj_set_property_flag(	MpvObj *mpv, const gchar *name, gboolean value);
gint mpv_obj_set_property_string(	MpvObj *mpv,
					const gchar *name,
					const char *data );
void mpv_obj_set_event_callback(	MpvObj *mpv,
					void (*func)(mpv_event *, void *),
					void *data );
void mpv_obj_set_opengl_cb_callback(	MpvObj *mpv,
					mpv_opengl_cb_update_fn func,
					void *data );
void mpv_check_error(int status);
gboolean mpv_obj_is_loaded(MpvObj *mpv);
void mpv_obj_get_state(MpvObj *mpv, MpvObjState *state);
gdouble mpv_obj_get_autofit_ratio(MpvObj *mpv);
Playlist *mpv_obj_get_playlist(MpvObj *mpv);
mpv_handle *mpv_obj_get_mpv_handle(MpvObj *mpv);
mpv_opengl_cb_context *mpv_obj_get_opengl_cb_context(MpvObj *mpv);
void mpv_obj_initialize(MpvObj *mpv);
void mpv_obj_reset(MpvObj *mpv);
void mpv_obj_quit(MpvObj *mpv);
void mpv_obj_load(	MpvObj *mpv,
			const gchar *uri,
			gboolean append,
			gboolean update );
void mpv_obj_load_list(	MpvObj *mpv,
			const gchar **uri_list,
			gboolean append,
			gboolean update );

G_END_DECLS

#endif
