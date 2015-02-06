/*
 * Copyright (c) 2014 gnome-mpv
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

#ifndef CONTROL_BOX_H
#define CONTROL_BOX_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define CONTROL_BOX_TYPE (control_box_get_type ())

#define	CONTROL_BOX(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), CONTROL_BOX_TYPE, ControlBox))

#define	CONTROL_BOX_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass), CONTROL_BOX_TYPE, ControlBoxClass))

#define	IS_CONTROL_BOX(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), CONTROL_BOX_TYPE))

#define	IS_CONTROL_BOX_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), CONTROL_BOX_TYPE))

typedef struct ControlBox ControlBox;
typedef struct ControlBoxClass ControlBoxClass;

struct ControlBox
{
	GtkBox box;
	gint seek_bar_length;
	GtkWidget *play_button;
	GtkWidget *stop_button;
	GtkWidget *forward_button;
	GtkWidget *rewind_button;
	GtkWidget *next_button;
	GtkWidget *previous_button;
	GtkWidget *volume_button;
	GtkWidget *fullscreen_button;
	GtkWidget *seek_bar;
};

struct ControlBoxClass
{
	GtkBoxClass parent_class;
};

GtkWidget *control_box_new(void);
GType control_box_get_type(void);
void control_box_set_enabled(ControlBox *box, gboolean enabled);
void control_box_set_chapter_enabled(ControlBox *box, gboolean enabled);
void control_box_set_seek_bar_length(ControlBox *box, gint length);
void control_box_set_volume(ControlBox *box, gboolean volume);
void control_box_set_playing_state(ControlBox *box, gboolean playing);
void control_box_set_fullscreen_state(ControlBox *box, gboolean fullscreen);
void control_box_reset_control(ControlBox *box);

G_END_DECLS

#endif

