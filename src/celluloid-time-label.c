/*
 * Copyright (c) 2020 gnome-mpv
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
	GtkLabel parent;
	gint duration;
	gint time;
};

struct _CelluloidTimeLabelClass
{
	GtkLabelClass parent_class;
};

G_DEFINE_TYPE(CelluloidTimeLabel, celluloid_time_label, GTK_TYPE_LABEL)

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
get_max_pixel_size(GtkLabel *label, gint length, gint *width, gint *height);

static void
update_size(CelluloidTimeLabel *label);

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
		{
			gint old_duration = self->duration;

			self->duration = g_value_get_int(value);

			// Only update size if we are transitioning from having
			// duration to not having one and vice versa, or if the
			// number of digits in the hours part of the duration
			// changed.
			if(	(old_duration <= 0) !=
				(self->duration <= 0) ||
				(gint)log10(old_duration / 3600) !=
				(gint)log10(self->duration / 3600) )
			{
				update_size(self);
			}

			update_label(self);
		}
		break;

		case PROP_TIME:
		{
			gint old_time = self->time;

			self->time = g_value_get_int(value);

			// Only update size if we don't have a duration, and the
			// number of digits in the hour part of the time
			// changed. We don't need to update the size if we have
			// duration since the size would have been already
			// updated when the duration was set, and the
			// resulting size will have enough space to accomodate
			// all possible time values.
			if(	self->duration <= 0 &&
				(gint)log10(old_time / 3600) !=
				(gint)log10(self->time / 3600) )
			{
				update_size(self);
			}

			update_label(self);
		}
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
get_max_pixel_size(GtkLabel *label, gint duration, gint *width, gint *height)
{
	PangoLayout *layout = pango_layout_copy(gtk_label_get_layout(label));

	// Find a number in which all digits are 1s, with the same number of
	// digits as the hour part of the duration. This will be used to
	// generate other numbers which will then be used to generate test
	// strings.
	const gint hour_digits = (gint)log10(duration/3600) + 1;
	const gint ones = ((gint)pow(10, hour_digits) - 1) / 9;

	for(gint i = 0; i < 10; i++)
	{
		gint test_width = 0;
		gint test_height = 0;

		// Numbers with repeated digits to be used to generate test
		// strings.
		const gint hour = ones * i;
		const gint min_sec = 11 * i;

		gchar *time_str =
			duration < 3600 ?
			g_strdup_printf("%02d:%02d", min_sec, min_sec) :
			g_strdup_printf(	"%0*d:%02d:%02d",
						hour_digits,
						hour,
						min_sec,
						min_sec );
		gchar *test_str =
			duration <= 0 ?
			g_strdup(time_str) :
			g_strdup_printf("%s/%s", time_str, time_str);

		pango_layout_set_text
			(layout, test_str, -1);
		pango_layout_get_pixel_size
			(layout, &test_width, &test_height);

		// Keep maximum width and height.
		if(width)
		{
			*width = *width < test_width ? test_width : *width;
		}

		if(height)
		{
			*height = *height < test_height ? test_height : *height;
		}

		g_free(test_str);
	}

	g_object_unref(layout);
}

static void
update_size(CelluloidTimeLabel *label)
{
	gint width = 0;

	get_max_pixel_size(	GTK_LABEL(label),
				label->duration,
				&width,
				NULL );
	gtk_widget_set_size_request
		(GTK_WIDGET(label), width, -1);
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

	gtk_label_set_text(GTK_LABEL(label), text);
}

static void
celluloid_time_label_init(CelluloidTimeLabel *label)
{
	label->duration = 0;
	label->time = 0;

	update_size(label);
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

