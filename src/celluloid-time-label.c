/*
 * Copyright (c) 2020-2021, 2023 gnome-mpv
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

#include <math.h>

#include "celluloid-common.h"
#include "celluloid-time-label.h"

enum
{
	PROP_INVALID,
	PROP_DURATION,
	PROP_TIME
};

struct _CelluloidTimeLabel
{
	GtkBox parent;
	GtkWidget *label;
	gint duration;
	gint time;
};

struct _CelluloidTimeLabelClass
{
	GtkBoxClass parent_class;
};

G_DEFINE_TYPE(CelluloidTimeLabel, celluloid_time_label, GTK_TYPE_BOX)

static void
set_property(	GObject *object,
		guint property_id,
		const GValue *value,
		GParamSpec *pspec );

static void
get_property(	GObject *object,
		guint property_id,
		GValue *value,
		GParamSpec *pspec );

static void
update_label(CelluloidTimeLabel *label);

static void
set_property(	GObject *object,
		guint property_id,
		const GValue *value,
		GParamSpec *pspec )
{
	CelluloidTimeLabel *self = CELLULOID_TIME_LABEL(object);

	switch(property_id)
	{
		case PROP_DURATION:
		self->duration = g_value_get_int(value);
		update_label(self);
		break;

		case PROP_TIME:
		self->time = g_value_get_int(value);
		update_label(self);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
get_property(	GObject *object,
		guint property_id,
		GValue *value,
		GParamSpec *pspec )
{
	CelluloidTimeLabel *self = CELLULOID_TIME_LABEL(object);

	switch(property_id)
	{
		case PROP_DURATION:
		g_value_set_int(value, self->duration);
		break;

		case PROP_TIME:
		g_value_set_int(value, self->time);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
update_label(CelluloidTimeLabel *label)
{
	gint sec = (gint)label->time;
	gint duration = (gint)label->duration;
	gchar *text;

	if(duration > 0)
	{
		gchar *sec_str = format_time(sec, duration >= 3600);
		gchar *duration_str = format_time(duration, duration >= 3600);

		text = g_strdup_printf("%s/%s", sec_str, duration_str);

		g_free(sec_str);
		g_free(duration_str);
	}
	else
	{
		text = g_strdup_printf("%02d:%02d", (sec%3600)/60, sec%60);
	}

	gtk_label_set_text(GTK_LABEL(label->label), text);
}

static void
celluloid_time_label_init(CelluloidTimeLabel *label)
{
	label->label = gtk_label_new(NULL);
	label->duration = 0;
	label->time = 0;

	gtk_widget_add_css_class(label->label, "numeric");
	gtk_box_append(GTK_BOX(label), label->label);
}

static void
celluloid_time_label_class_init(CelluloidTimeLabelClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec = NULL;

	object_class->set_property = set_property;
	object_class->get_property = get_property;

	pspec = g_param_spec_int
		(	"duration",
			"Duration",
			"The full duration of the media being played",
			0,
			G_MAXINT,
			0,
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_DURATION, pspec);

	pspec = g_param_spec_int
		(	"time",
			"Time",
			"Timestamp of the current position",
			0,
			G_MAXINT,
			0,
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_TIME, pspec);
}

GtkWidget *
celluloid_time_label_new()
{
	GObject *object = g_object_new(celluloid_time_label_get_type(), NULL);

	return GTK_WIDGET(object);
}

