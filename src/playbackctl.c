/*
 * Copyright (c) 2015-2016 gnome-mpv
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

#include <gtk/gtk.h>

#include "playbackctl.h"
#include "control_box.h"
#include "common.h"
#include "mpv_obj.h"

static void play_handler(GtkWidget *widget, gpointer data);
static void stop_handler(GtkWidget *widget, gpointer data);
static void seek_handler(GtkWidget *widget, gdouble value, gpointer data);
static void forward_handler(GtkWidget *widget, gpointer data);
static void rewind_handler(GtkWidget *widget, gpointer data);
static void chapter_previous_handler(GtkWidget *widget, gpointer data);
static void chapter_next_handler(GtkWidget *widget, gpointer data);
static void volume_handler(GtkWidget *widget, gdouble value, gpointer data);
static void fullscreen_handler(GtkWidget *button, gpointer data);

static void play_handler(GtkWidget *widget, gpointer data)
{
	Application *app = data;

	mpv_obj_set_property_flag
		(	app->mpv,
			"pause",
			!mpv_obj_get_property_flag(app->mpv, "pause") );
}

static void stop_handler(GtkWidget *widget, gpointer data)
{
	Application *app = data;
	const gchar *cmd[] = {"stop", NULL};

	mpv_obj_command(app->mpv, cmd);
}

static void seek_handler(GtkWidget *widget, gdouble value, gpointer data)
{
	seek(data, value);
}

static void forward_handler(GtkWidget *widget, gpointer data)
{
	Application *app = data;
	const gchar *cmd[] = {"seek", "10", NULL};

	mpv_obj_command(app->mpv, cmd);
}

static void rewind_handler(GtkWidget *widget, gpointer data)
{
	Application *app = data;
	const gchar *cmd[] = {"seek", "-10", NULL};

	mpv_obj_command(app->mpv, cmd);
}

static void chapter_previous_handler(GtkWidget *widget, gpointer data)
{
	Application *app = data;
	const gchar *cmd[] = {"osd-msg", "cycle", "chapter", "down", NULL};

	mpv_obj_command(app->mpv, cmd);
}

static void chapter_next_handler(GtkWidget *widget, gpointer data)
{
	Application *app = data;
	const gchar *cmd[] = {"osd-msg", "cycle", "chapter", NULL};

	mpv_obj_command(app->mpv, cmd);
}

static void volume_handler(GtkWidget *widget, gdouble value, gpointer data)
{
	Application *app = data;

	value *= 100;

	mpv_obj_set_property(app->mpv, "volume", MPV_FORMAT_DOUBLE, &value);
}

static void fullscreen_handler(GtkWidget *widget, gpointer data)
{
	main_window_toggle_fullscreen(APPLICATION(data)->gui);
}

void playbackctl_connect_signals(Application *app)
{
	ControlBox *control_box = CONTROL_BOX(app->gui->control_box);

	const struct
	{
		const gchar *name;
		GCallback handler;
	}
	signals_map[] = {	{"play-button-clicked",
				G_CALLBACK(play_handler)},
				{"stop-button-clicked",
				G_CALLBACK(stop_handler)},
				{"forward-button-clicked",
				G_CALLBACK(forward_handler)},
				{"rewind-button-clicked",
				G_CALLBACK(rewind_handler)},
				{"previous-button-clicked",
				G_CALLBACK(chapter_previous_handler)},
				{"next-button-clicked",
				G_CALLBACK(chapter_next_handler)},
				{"fullscreen-button-clicked",
				G_CALLBACK(fullscreen_handler)},
				{"seek",
				G_CALLBACK(seek_handler)},
				{"volume-changed",
				G_CALLBACK(volume_handler)},
				{NULL, NULL} };

	for(gint i = 0; signals_map[i].name; i++)
	{
		g_signal_connect(	control_box,
					signals_map[i].name,
					signals_map[i].handler,
					app );
	}
}
