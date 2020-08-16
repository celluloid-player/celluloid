/*
 * Copyright (c) 2016-2020 gnome-mpv
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

struct _CelluloidSeekBar
{
	GtkBox parent_instance;
	GtkWidget *seek_bar;
	GtkWidget *label;
	GtkWidget *popover;
	GtkWidget *popover_label;
	gdouble pos;
	gdouble duration;
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
change_value_handler(	GtkWidget *widget,
			GtkScrollType scroll,
			gdouble value,
			gpointer data );

static gboolean
enter_handler(GtkWidget *widget, GdkEventCrossing *event, gpointer data);

static gboolean
leave_handler(GtkWidget *widget, GdkEventCrossing *event, gpointer data);

static gboolean
motion_handler(GtkWidget *widget, GdkEventMotion *event, gpointer data);

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
enter_handler(GtkWidget *widget, GdkEventCrossing *event, gpointer data)
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
leave_handler(GtkWidget *widget, GdkEventCrossing *event, gpointer data)
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
motion_handler(GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
	CelluloidSeekBar *bar = data;
	GdkRectangle rect = {	.x = (gint)event->x,
				.y = 0,
				.width = 0,
				.height = 0 };
	GdkRectangle range_rect;
	gdouble progress = 0;
	gchar *text = NULL;

	gtk_range_get_range_rect(GTK_RANGE(bar->seek_bar), &range_rect);
	rect.x = CLAMP(rect.x, range_rect.x, range_rect.x + range_rect.width);

	gtk_popover_set_pointing_to(GTK_POPOVER(bar->popover), &rect);

	progress = rect.x / (gdouble)(range_rect.x + range_rect.width);
	text = format_time(	(gint)(progress * bar->duration),
				bar->duration >= 3600 );
	gtk_label_set_text(GTK_LABEL(bar->popover_label), text);

	g_free(text);

	return FALSE;
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
	G_OBJECT_CLASS(klass)->dispose = dispose;

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
	bar->popover = g_object_ref_sink(gtk_popover_new(bar->seek_bar));
	bar->popover_label = gtk_label_new(NULL);
	bar->duration = 0;
	bar->pos = 0;
	bar->popover_visible = FALSE;
	bar->popover_timeout_id = 0;

	g_object_set(	bar->label,
			"time", (gint)bar->pos,
			"duration", (gint)bar->duration,
			NULL );
	gtk_popover_set_modal(GTK_POPOVER(bar->popover), FALSE);

	gtk_scale_set_draw_value(GTK_SCALE(bar->seek_bar), FALSE);
	gtk_range_set_increments(GTK_RANGE(bar->seek_bar), 10, 10);
	gtk_widget_set_can_focus(bar->seek_bar, FALSE);
	gtk_widget_add_events(bar->seek_bar, GDK_POINTER_MOTION_MASK);

	gtk_container_add(GTK_CONTAINER(bar->popover), bar->popover_label);
	gtk_container_set_border_width(GTK_CONTAINER(bar->popover), 6);
	gtk_widget_show(bar->popover_label);

	g_signal_connect(	bar->seek_bar,
				"change-value",
				G_CALLBACK(change_value_handler),
				bar );
	g_signal_connect(	bar->seek_bar,
				"enter-notify-event",
				G_CALLBACK(enter_handler),
				bar );
	g_signal_connect(	bar->seek_bar,
				"leave-notify-event",
				G_CALLBACK(leave_handler),
				bar );
	g_signal_connect(	bar->seek_bar,
				"motion-notify-event",
				G_CALLBACK(motion_handler),
				bar );

	gtk_box_pack_start(GTK_BOX(bar), bar->seek_bar, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(bar), bar->label, FALSE, FALSE, 0);
}

GtkWidget *
celluloid_seek_bar_new()
{
	return GTK_WIDGET(g_object_new(celluloid_seek_bar_get_type(), NULL));
}

void
celluloid_seek_bar_set_duration(CelluloidSeekBar *bar, gdouble duration)
{
	bar->duration = duration;

	g_object_set(bar->label, "duration", (gint)duration, NULL);
	gtk_range_set_range(GTK_RANGE(bar->seek_bar), 0, duration);
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
