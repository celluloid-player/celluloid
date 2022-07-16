/*
 * Copyright (c) 2016-2022 gnome-mpv
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

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "celluloid-seek-bar.h"
#include "celluloid-time-label.h"
#include "celluloid-common.h"

enum
{
	PROP_0,
	PROP_DURATION,
	PROP_PAUSE,
	PROP_ENABLED,
	PROP_SHOW_LABEL,
	PROP_POPOVER_Y_OFFSET,
	N_PROPERTIES
};

struct _CelluloidSeekBar
{
	GtkBox parent_instance;
	GtkWidget *seek_bar;
	GtkWidget *label;
	GtkWidget *popover;
	GtkWidget *popover_label;
	gdouble pos;
	gdouble duration;
	gboolean pause;
	gboolean enabled;
	gboolean show_label;
	gint popover_y_offset;
	gboolean popover_visible;
	guint popover_timeout_id;
};

struct _CelluloidSeekBarClass
{
	GtkBoxClass parent_class;
};

static void
dispose(GObject *object);

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
change_value_handler(	GtkWidget *widget,
			GtkScrollType scroll,
			gdouble value,
			gpointer data );

static gboolean
enter_handler(	GtkEventControllerMotion *controller,
		gdouble x,
		gdouble y,
		gpointer data );

static gboolean
leave_handler(GtkEventControllerMotion *widget, gpointer data);

static gboolean
motion_handler(	GtkEventControllerMotion *controller,
		gdouble x,
		gdouble y,
		gpointer data );

static void
update_label(CelluloidSeekBar *bar);

static gboolean
update_popover_visibility(CelluloidSeekBar *bar);

G_DEFINE_TYPE(CelluloidSeekBar, celluloid_seek_bar, GTK_TYPE_BOX)

static void
dispose(GObject *object)
{
	g_clear_object(&CELLULOID_SEEK_BAR(object)->popover);

	G_OBJECT_CLASS(celluloid_seek_bar_parent_class)->dispose(object);
}

static void
set_property(	GObject *object,
		guint property_id,
		const GValue *value,
		GParamSpec *pspec )
{
	CelluloidSeekBar *self = CELLULOID_SEEK_BAR(object);

	switch(property_id)
	{
		case PROP_DURATION:
		self->duration = g_value_get_double(value);
		update_label(self);
		break;

		case PROP_PAUSE:
		self->pause = g_value_get_boolean(value);
		update_label(self);
		break;

		case PROP_ENABLED:
		self->enabled = g_value_get_boolean(value);
		update_label(self);
		break;

		case PROP_SHOW_LABEL:
		self->show_label = g_value_get_boolean(value);
		gtk_widget_set_visible(self->label, self->show_label);
		break;

		case PROP_POPOVER_Y_OFFSET:
		self->popover_y_offset = g_value_get_int(value);
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
	CelluloidSeekBar *self = CELLULOID_SEEK_BAR(object);

	switch(property_id)
	{
		case PROP_DURATION:
		g_value_set_double(value, self->duration);
		break;

		case PROP_PAUSE:
		g_value_set_boolean(value, self->pause);
		break;

		case PROP_ENABLED:
		g_value_set_boolean(value, self->enabled);
		break;

		case PROP_SHOW_LABEL:
		g_value_set_boolean(value, self->show_label);
		break;

		case PROP_POPOVER_Y_OFFSET:
		g_value_set_int(value, self->popover_y_offset);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
change_value_handler(	GtkWidget *widget,
			GtkScrollType scroll,
			gdouble value,
			gpointer data )
{
	CelluloidSeekBar *bar = data;

	if(bar->duration > 0)
	{
		g_object_set(	bar->label,
				"time", (gint)bar->pos,
				"duration", (gint)bar->duration,
				NULL );
		g_signal_emit_by_name(data, "seek", value);
	}
}

static gboolean
enter_handler(	GtkEventControllerMotion *controller,
		gdouble x,
		gdouble y,
		gpointer data )
{
	CelluloidSeekBar *bar = data;

	if(bar->popover_timeout_id > 0)
	{
		g_source_remove(bar->popover_timeout_id);
	}

	/* Don't show popover if duration is unknown */
	if(bar->duration > 0)
	{
		bar->popover_visible = TRUE;
		bar->popover_timeout_id =
			g_timeout_add(	100,
					(GSourceFunc)update_popover_visibility,
					bar );
	}

	return FALSE;
}

static gboolean
leave_handler(GtkEventControllerMotion *widget, gpointer data)
{
	CelluloidSeekBar *bar = data;

	if(bar->popover_timeout_id > 0)
	{
		g_source_remove(bar->popover_timeout_id);
	}

	bar->popover_visible = FALSE;
	bar->popover_timeout_id =
		g_timeout_add(100, (GSourceFunc)update_popover_visibility, bar);

	return FALSE;
}

static gboolean
motion_handler(	GtkEventControllerMotion *controller,
		gdouble x,
		gdouble y,
		gpointer data )
{
	CelluloidSeekBar *bar = data;
	GtkRange *range = GTK_RANGE(bar->seek_bar);
	GdkRectangle popover_rect = {0};
	GdkRectangle range_rect = {0};

	gtk_range_get_range_rect(range, &range_rect);

	// TODO: Figure out how to get the actual margin
	const gint margin = 1;
	const gint trough_start = range_rect.x + margin;
	const gint trough_length = range_rect.width - 2 * margin;

	x = CLAMP(x, trough_start, trough_start + trough_length);

	graphene_point_t popover_offset =
		{0};
	const gboolean popover_offset_computed =
		gtk_widget_compute_point
		(	GTK_WIDGET(range),
			GTK_WIDGET(bar),
			&GRAPHENE_POINT_INIT((gfloat)x, 0),
			&popover_offset );

	if(popover_offset_computed)
	{
		popover_rect.x = (gint)popover_offset.x;
	}
	else
	{
		// This seems to work for Adwaita
		popover_rect.x = (gint)x + 13;
		g_warning("Failed to determine position for timestamp popover");
	}

	popover_rect.y += bar->popover_y_offset;

	gtk_popover_set_pointing_to(GTK_POPOVER(bar->popover), &popover_rect);

	GtkAdjustment *adjustment = gtk_range_get_adjustment(range);
	const gdouble lower = gtk_adjustment_get_lower(adjustment);
	const gdouble upper = gtk_adjustment_get_upper(adjustment);
	const gdouble page_size = gtk_adjustment_get_page_size(adjustment);

	const gdouble progress =
		((gint)x - trough_start) / (gdouble)trough_length;
	const gdouble time =
		lower + progress * (upper - lower - page_size);
	gchar *text =
		format_time((gint)time, bar->duration >= 3600);

	gtk_label_set_text(GTK_LABEL(bar->popover_label), text);
	g_free(text);

	return FALSE;
}

static void
update_label(CelluloidSeekBar *bar)
{
	// When disabled, the effective duration is zero regardless of what the
	// value of the duration property is.
	const gdouble duration = bar->enabled ? bar->duration : 0;

	g_object_set(bar->label, "duration", (gint)duration, NULL);
	gtk_range_set_range(GTK_RANGE(bar->seek_bar), 0, duration);
}

static gboolean
update_popover_visibility(CelluloidSeekBar *bar)
{
	if(bar->popover_visible)
	{
		gtk_popover_popup(GTK_POPOVER(bar->popover));
	}
	else
	{
		gtk_popover_popdown(GTK_POPOVER(bar->popover));
	}

	bar->popover_timeout_id = 0;

	return G_SOURCE_REMOVE;
}

static void
celluloid_seek_bar_class_init(CelluloidSeekBarClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec = NULL;

	object_class->dispose = dispose;
	object_class->set_property = set_property;
	object_class->get_property = get_property;

	pspec = g_param_spec_double
		(	"duration",
			"Duration",
			"The duration the seek bar is using",
			0.0,
			G_MAXDOUBLE,
			0.0,
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_DURATION, pspec);

	pspec = g_param_spec_boolean
		(	"pause",
			"Pause",
			"Whether there is a file playing",
			TRUE,
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_PAUSE, pspec);

	pspec = g_param_spec_boolean
		(	"enabled",
			"Enabled",
			"Whether or not the widget is enabled",
			TRUE,
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_ENABLED, pspec);

	pspec = g_param_spec_boolean
		(	"show-label",
			"Shoe label",
			"Whether or not the timestamp/duration label is shown",
			TRUE,
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_SHOW_LABEL, pspec);

	pspec = g_param_spec_int
		(	"popover-y-offset",
			"Popover y-offset",
			"The offset to add to the y-position of the popover",
			G_MININT,
			G_MAXINT,
			0,
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_POPOVER_Y_OFFSET, pspec);

	g_signal_new(	"seek",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__DOUBLE,
			G_TYPE_NONE,
			1,
			G_TYPE_DOUBLE );
}

static void
celluloid_seek_bar_init(CelluloidSeekBar *bar)
{
	bar->seek_bar = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, NULL);
	bar->label = celluloid_time_label_new();
	bar->popover = g_object_ref_sink(gtk_popover_new());
	bar->popover_label = gtk_label_new(NULL);
	bar->duration = 0;
	bar->pause = TRUE;
	bar->enabled = TRUE;
	bar->show_label = TRUE;
	bar->popover_y_offset = 0;
	bar->pos = 0;
	bar->popover_visible = FALSE;
	bar->popover_timeout_id = 0;

	g_object_set(	bar->label,
			"time", (gint)bar->pos,
			"duration", (gint)bar->duration,
			NULL );
	gtk_popover_set_autohide(GTK_POPOVER(bar->popover), FALSE);

	gtk_scale_set_draw_value(GTK_SCALE(bar->seek_bar), FALSE);
	gtk_range_set_increments(GTK_RANGE(bar->seek_bar), 10, 10);
	gtk_widget_set_can_focus(bar->seek_bar, FALSE);

	gtk_popover_set_position(GTK_POPOVER(bar->popover), GTK_POS_TOP);
	gtk_popover_set_child(GTK_POPOVER(bar->popover), bar->popover_label);
	gtk_widget_show(bar->popover_label);

	g_signal_connect(	bar->seek_bar,
				"change-value",
				G_CALLBACK(change_value_handler),
				bar );

	GtkEventController *motion_controller =
		gtk_event_controller_motion_new();

	gtk_widget_add_controller(GTK_WIDGET(bar->seek_bar), motion_controller);

	g_signal_connect(	motion_controller,
				"enter",
				G_CALLBACK(enter_handler),
				bar );
	g_signal_connect(	motion_controller,
				"leave",
				G_CALLBACK(leave_handler),
				bar );
	g_signal_connect(	motion_controller,
				"motion",
				G_CALLBACK(motion_handler),
				bar );

	gtk_widget_set_hexpand(bar->seek_bar, TRUE);

	gtk_box_append(GTK_BOX(bar), bar->seek_bar);
	gtk_box_append(GTK_BOX(bar), bar->label);
	gtk_box_append(GTK_BOX(bar), bar->popover);
}

GtkWidget *
celluloid_seek_bar_new()
{
	return GTK_WIDGET(g_object_new(celluloid_seek_bar_get_type(), NULL));
}

void
celluloid_seek_bar_set_duration(CelluloidSeekBar *bar, gdouble duration)
{
	g_object_set(bar, "duration", duration, NULL);
}

void
celluloid_seek_bar_set_pos(CelluloidSeekBar *bar, gdouble pos)
{
	gdouble old_pos = bar->pos;

	bar->pos = pos;

	gtk_range_set_value(GTK_RANGE(bar->seek_bar), pos);

	if((gint)old_pos != (gint)pos)
	{
		g_object_set(bar->label, "time", (gint)pos, NULL);
	}
}
