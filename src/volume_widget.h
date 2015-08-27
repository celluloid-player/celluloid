/*
 * Copyright (c) 2015 gnome-mpv
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

#ifndef VOLUME_WIDGET_H
#define VOLUME_WIDGET_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define VOLUME_WIDGET_TYPE (volume_widget_get_type())

#define	VOLUME_WIDGET(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), VOLUME_WIDGET_TYPE, VolumeWidget))

#define	VOLUME_WIDGET_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass), VOLUME_WIDGET_TYPE, VolumeWidgetClass))

#define	IS_VOLUME_WIDGET(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), VOLUME_WIDGET_TYPE))

#define	IS_VOLUME_WIDGET_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), VOLUME_WIDGET_TYPE))

struct _VolumeWidget
{
	GtkBox parent_instance;
	GtkWidget *ebox;
	GtkWidget *icon;
	GtkWidget *scale;
};

struct _VolumeWidgetClass
{
	GtkBoxClass parent_class;
};

typedef struct _VolumeWidget VolumeWidget;
typedef struct _VolumeWidgetClass VolumeWidgetClass;

GtkWidget *volume_widget_new(void);
GType volume_widget_get_type(void);

#endif
