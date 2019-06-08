/*
 * Copyright (c) 2016-2019 gnome-mpv
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
#include "celluloid-seek-bar.h"

struct _CelluloidSeekBar
{
	GtkBox parent_instance;
	GtkWidget *seek_bar;
	GtkWidget *label;
	gdouble pos;
	gdouble duration;
};

struct _CelluloidSeekBarClass
{
	GtkBoxClass parent_class;
};

static void
change_value_handler(	GtkWidget *widget,
			GtkScrollType scroll,
			gdouble value,
			gpointer data );

static void
update_label(CelluloidSeekBar *bar);

G_DEFINE_TYPE(CelluloidSeekBar, celluloid_seek_bar, GTK_TYPE_BOX)

static void
change_value_handler(	GtkWidget *widget,
			GtkScrollType scroll,
			gdouble value,
			gpointer data )
{
	CelluloidSeekBar *bar = data;

	if(bar->duration > 0)
	{
		update_label(data);
		g_signal_emit_by_name(data, "seek", value);
	}
}

static void
update_label(CelluloidSeekBar *bar)
{
	gint sec = (gint)bar->pos;
	gint duration = (gint)bar->duration;
	gchar *output;

	/* Longer than one hour */
	if(duration > 3600)
	{
		output = g_strdup_printf(	"%02d:%02d:%02d/"
						"%02d:%02d:%02d",
						sec/3600,
						(sec%3600)/60,
						sec%60,
						duration/3600,
						(duration%3600)/60,
						duration%60 );
	}
	else if(duration > 0)
	{
		output = g_strdup_printf(	"%02d:%02d/"
						"%02d:%02d",
						(sec%3600)/60,
						sec%60,
						(duration%3600)/60,
						duration%60 );
	}
	else
	{
		output = g_strdup_printf("%02d:%02d", (sec%3600)/60, sec%60);
	}

	gtk_label_set_text(GTK_LABEL(bar->label), output);
}

static void
celluloid_seek_bar_class_init(CelluloidSeekBarClass *klass)
{
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
	bar->label = gtk_label_new("");
	bar->duration = 0;
	bar->pos = 0;

	update_label(bar);
	gtk_scale_set_draw_value(GTK_SCALE(bar->seek_bar), FALSE);
	gtk_range_set_increments(GTK_RANGE(bar->seek_bar), 10, 10);
	gtk_widget_set_can_focus(bar->seek_bar, FALSE);

	g_signal_connect(	bar->seek_bar,
				"change-value",
				G_CALLBACK(change_value_handler),
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

	update_label(bar);
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
		update_label(bar);
	}
}
