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

G_BEGIN_DECLS

#define MPV_OBJ_TYPE (mpv_obj_get_type ())

#define	MPV_OBJ(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), MPV_OBJ_TYPE, MpvObj))

#define	MPV_OBJ_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass), MPV_OBJ_TYPE, MpvObjClass))

#define	IS_MPV_OBJ(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), MPV_OBJ_TYPE))

#define	IS_MPV_OBJ_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), MPV_OBJ_TYPE))

typedef struct _MpvObj MpvObj;
typedef struct _MpvObjClass MpvObjClass;

struct _MpvObj
{
	GObject parent;
	mpv_handle *mpv_ctx;
	mpv_opengl_cb_context *opengl_ctx;
	GSList *log_level_list;
	gdouble autofit_ratio;
};

struct _MpvObjClass
{
	GObjectClass parent_class;
};

//TODO: Remove
typedef struct _Application Application;

GType mpv_obj_get_type(void);
MpvObj *mpv_obj_new(void);
gint mpv_obj_command(MpvObj *mpv, const gchar **cmd);
gint mpv_obj_command_string(MpvObj *mpv, const gchar *cmd);
gint mpv_obj_set_property(	MpvObj *mpv,
				const gchar *name,
				mpv_format format,
				void *data );
gint mpv_obj_set_property_string(	MpvObj *mpv,
					const gchar *name,
					const char *data );
void mpv_obj_wakeup_callback(void *data);
void mpv_obj_log_handler(MpvObj *mpv, mpv_event_log_message* message);
void mpv_check_error(int status);
gboolean  mpv_obj_handle_event(gpointer data);
void mpv_obj_update_playlist(Application *app);
void mpv_obj_load_gui_update(Application *app);
gint mpv_obj_apply_args(mpv_handle *mpv_ctx, char *args);
void mpv_obj_initialize(Application *app);
void mpv_obj_quit(Application *app);
void mpv_obj_load(	Application *app,
		const gchar *uri,
		gboolean append,
		gboolean update );

G_END_DECLS

#endif
