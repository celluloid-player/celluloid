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

#include <gtk/gtk.h>
#include <string.h>

#include "playbackctl.h"
#include "control_box.h"
#include "common.h"
#include "mpv.h"

static void play_handler(GtkWidget *widget, gpointer data);
static void stop_handler(GtkWidget *widget, gpointer data);
static void seek_handler(	GtkWidget *widget,
				GtkScrollType scroll,
				gdouble value,
				gpointer data );
static void forward_handler(GtkWidget *widget, gpointer data);
static void rewind_handler(GtkWidget *widget, gpointer data);
static void chapter_previous_handler(GtkWidget *widget, gpointer data);
static void chapter_next_handler(GtkWidget *widget, gpointer data);
static void volume_handler(GtkWidget *widget, gdouble value, gpointer data);
static void fullscreen_handler(GtkWidget *button, gpointer data);
static void loop_handler(GtkWidget *button, gpointer data);

static void play_handler(GtkWidget *widget, gpointer data)
{
	gmpv_handle *ctx = data;

	ctx->paused = !ctx->paused;

	mpv_check_error(mpv_set_property(	ctx->mpv_ctx,
						"pause",
						MPV_FORMAT_FLAG,
						&ctx->paused ));
}

static void stop_handler(GtkWidget *widget, gpointer data)
{
	gmpv_handle *ctx = data;
	const gchar *cmd[] = {"stop", NULL};

	mpv_command(ctx->mpv_ctx, cmd);
}

static void seek_handler(	GtkWidget *widget,
				GtkScrollType scroll,
				gdouble value,
				gpointer data )
{
	seek(data, value);
}

static void forward_handler(GtkWidget *widget, gpointer data)
{
	gmpv_handle *ctx = data;
	const gchar *cmd[] = {"seek", "10", NULL};

	mpv_command(ctx->mpv_ctx, cmd);
}

static void rewind_handler(GtkWidget *widget, gpointer data)
{
	gmpv_handle *ctx = data;
	const gchar *cmd[] = {"seek", "-10", NULL};

	mpv_command(ctx->mpv_ctx, cmd);
}

static void chapter_previous_handler(GtkWidget *widget, gpointer data)
{
	gmpv_handle *ctx = data;
	const gchar *cmd[] = {"osd-msg", "cycle", "chapter", "down", NULL};

	mpv_command(ctx->mpv_ctx, cmd);
}

static void chapter_next_handler(GtkWidget *widget, gpointer data)
{
	gmpv_handle *ctx = data;
	const gchar *cmd[] = {"osd-msg", "cycle", "chapter", NULL};

	mpv_command(ctx->mpv_ctx, cmd);
}

static void volume_handler(GtkWidget *widget, gdouble value, gpointer data)
{
	gmpv_handle *ctx = data;

	value *= 100;

	mpv_set_property(ctx->mpv_ctx, "volume", MPV_FORMAT_DOUBLE, &value);
}

static void fullscreen_handler(GtkWidget *widget, gpointer data)
{
	toggle_fullscreen(data);
}

static void loop_handler(GtkWidget *widget, gpointer data)
{
	GtkToggleButton *loop_button = (GtkToggleButton *)widget;
	gmpv_handle *ctx = data;
	gchar *loop = mpv_get_property_string(ctx->mpv_ctx, "loop");

	if(loop) {
		if(strcmp(loop, "no") == 0) {
			mpv_check_error(mpv_set_property_string(ctx->mpv_ctx,
								"loop",
								"inf"	));
		}
		else if(strcmp(loop, "inf") == 0) {
			mpv_check_error(mpv_set_property_string(ctx->mpv_ctx,
								"loop",
								"no"	));
		}
		gtk_toggle_button_toggled(loop_button);
	}

	mpv_free(loop);
}

void playbackctl_connect_signals(gmpv_handle *ctx)
{
	ControlBox *control_box = CONTROL_BOX(ctx->gui->control_box);

	g_signal_connect(	control_box->play_button,
				"clicked",
				G_CALLBACK(play_handler),
				ctx );

	g_signal_connect(	control_box->stop_button,
				"clicked",
				G_CALLBACK(stop_handler),
				ctx );

	g_signal_connect(	control_box->seek_bar,
				"change-value",
				G_CALLBACK(seek_handler),
				ctx );

	g_signal_connect(	control_box->forward_button,
				"clicked",
				G_CALLBACK(forward_handler),
				ctx );

	g_signal_connect(	control_box->rewind_button,
				"clicked",
				G_CALLBACK(rewind_handler),
				ctx );

	g_signal_connect(	control_box->previous_button,
				"clicked",
				G_CALLBACK(chapter_previous_handler),
				ctx );

	g_signal_connect(	control_box->next_button,
				"clicked",
				G_CALLBACK(chapter_next_handler),
				ctx );

	g_signal_connect(	control_box->volume_button,
				"value-changed",
				G_CALLBACK(volume_handler),
				ctx );

	g_signal_connect(	control_box->fullscreen_button,
				"clicked",
				G_CALLBACK(fullscreen_handler),
				ctx );

	g_signal_connect(	control_box->loop_button,
				"clicked",
				G_CALLBACK(loop_handler),
				ctx );
}
