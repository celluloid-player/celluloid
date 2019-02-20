/*
 * Copyright (c) 2016-2017 gnome-mpv
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
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <string.h>

#include "gmpv_controller_private.h"
#include "gmpv_controller.h"
#include "gmpv_controller_input.h"
#include "gmpv_mpv.h"
#include "gmpv_mpv_wrapper.h"
#include "gmpv_main_window.h"
#include "gmpv_video_area.h"
#include "gmpv_def.h"

static gchar *get_full_keystr(guint keyval, guint state);
static gboolean key_press_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data );
static gboolean key_release_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data );
static gboolean mouse_button_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data );
static gboolean mouse_move_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data );
static gboolean scroll_handler(	GtkWidget *widget,
				GdkEvent *event,
				gpointer data );

static gchar *get_full_keystr(guint keyval, guint state)
{
	/* strlen("Ctrl+Alt+Shift+Meta+")+1 == 21 */
	const gsize max_modstr_len = 21;
	gchar modstr[max_modstr_len];
	gboolean found = FALSE;
	const gchar *keystr = gdk_keyval_name(keyval);
	const gchar *keystrmap[] = KEYSTRING_MAP;
	modstr[0] = '\0';

	if((state&GDK_SHIFT_MASK) != 0)
	{
		g_strlcat(modstr, "Shift+", max_modstr_len);
	}

	if((state&GDK_CONTROL_MASK) != 0)
	{
		g_strlcat(modstr, "Ctrl+", max_modstr_len);
	}

	if((state&GDK_MOD1_MASK) != 0)
	{
		g_strlcat(modstr, "Alt+", max_modstr_len);
	}

	/* Super is Meta in mpv */
	if((state&GDK_SUPER_MASK) != 0)
	{
		g_strlcat(modstr, "Meta+", max_modstr_len);
	}

	/* Translate GDK key name to mpv key name */
	for(gint i = 0; !found && keystrmap[i]; i += 2)
	{
		gint rc = g_ascii_strncasecmp(	keystr,
						keystrmap[i+1],
						KEYSTRING_MAX_LEN );

		if(rc == 0)
		{
			keystr = keystrmap[i];
			found = TRUE;
		}
	}

	return (strlen(keystr) > 0)?g_strconcat(modstr, keystr, NULL):NULL;
}

static gboolean key_press_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data )
{
	GmpvController *controller = data;
	guint keyval = ((GdkEventKey*)event)->keyval;
	guint state = ((GdkEventKey*)event)->state;
	gchar *keystr = get_full_keystr(keyval, state);

	if(keystr)
	{
		gmpv_model_key_down(controller->model, keystr);
		g_free(keystr);
	}

	return FALSE;
}

static gboolean key_release_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data )
{
	GmpvController *controller = data;
	guint keyval = ((GdkEventKey*)event)->keyval;
	guint state = ((GdkEventKey*)event)->state;
	gchar *keystr = get_full_keystr(keyval, state);

	if(keystr)
	{
		gmpv_model_key_up(controller->model, keystr);
		g_free(keystr);
	}

	return FALSE;
}

static gboolean mouse_button_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data )
{
	GdkEventButton *btn_event = (GdkEventButton *)event;

	if(btn_event->type == GDK_BUTTON_PRESS
	|| btn_event->type == GDK_BUTTON_RELEASE
	|| btn_event->type == GDK_SCROLL)
	{
		GmpvController *controller = data;
		gchar *btn_str =	g_strdup_printf
					("MOUSE_BTN%u", btn_event->button-1);
		void (*func)(GmpvModel *, const gchar *)
			=	(btn_event->type == GDK_SCROLL)?
				gmpv_model_key_press:
				(btn_event->type == GDK_BUTTON_PRESS)?
				gmpv_model_key_down:gmpv_model_key_up;

		func(controller->model, btn_str);

		g_free(btn_str);
	}

	return TRUE;
}

static gboolean mouse_move_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data )
{
	GmpvController *controller = data;
	GdkEventMotion *motion_event = (GdkEventMotion *)event;

	if(controller->model)
	{
		gmpv_model_mouse(	controller->model,
					(gint)motion_event->x,
					(gint)motion_event->y );
	}

	return FALSE;
}

static gboolean scroll_handler(	GtkWidget *widget,
				GdkEvent *event,
				gpointer data )
{
	GdkEventScroll *scroll_event = (GdkEventScroll *)event;
	guint button = 0;
	gint count = 0;

	/* Only one axis will be used at a time to prevent accidental activation
	 * of commands bound to buttons associated with the other axis.
	 */
	if(ABS(scroll_event->delta_x) > ABS(scroll_event->delta_y))
	{
		count = (gint)ABS(scroll_event->delta_x);

		if(scroll_event->delta_x <= -1)
		{
			button = 6;
		}
		else if(scroll_event->delta_x >= 1)
		{
			button = 7;
		}
	}
	else
	{
		count = (gint)ABS(scroll_event->delta_y);

		if(scroll_event->delta_y <= -1)
		{
			button = 4;
		}
		else if(scroll_event->delta_y >= 1)
		{
			button = 5;
		}
	}

	if(button > 0)
	{
		GdkEventButton btn_event;

		btn_event.type = scroll_event->type;
		btn_event.window = scroll_event->window;
		btn_event.send_event = scroll_event->send_event;
		btn_event.time = scroll_event->time;
		btn_event.x = scroll_event->x;
		btn_event.y = scroll_event->y;
		btn_event.axes = NULL;
		btn_event.state = scroll_event->state;
		btn_event.button = button;
		btn_event.device = scroll_event->device;
		btn_event.x_root = scroll_event->x_root;
		btn_event.y_root = scroll_event->y_root;

		for(gint i = 0; i < count; i++)
		{
			/* Not used */
			gboolean rc;

			g_signal_emit_by_name
				(widget, "button-press-event", &btn_event, &rc);
		}
	}

	return TRUE;
}

void gmpv_controller_input_connect_signals(GmpvController *controller)
{
	g_signal_connect(	controller->view,
				"key-press-event",
				G_CALLBACK(key_press_handler),
				controller );
	g_signal_connect(	controller->view,
				"key-release-event",
				G_CALLBACK(key_release_handler),
				controller );
	g_signal_connect(	controller->view,
				"button-press-event",
				G_CALLBACK(mouse_button_handler),
				controller );
	g_signal_connect(	controller->view,
				"button-release-event",
				G_CALLBACK(mouse_button_handler),
				controller );
	g_signal_connect(	controller->view,
				"motion-notify-event",
				G_CALLBACK(mouse_move_handler),
				controller );
	g_signal_connect(	controller->view,
				"scroll-event",
				G_CALLBACK(scroll_handler),
				controller );
}
