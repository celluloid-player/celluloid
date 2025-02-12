/*
 * Copyright (c) 2016-2024 gnome-mpv
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

#include "celluloid-controller-private.h"
#include "celluloid-controller.h"
#include "celluloid-controller-input.h"
#include "celluloid-mpv.h"
#include "celluloid-main-window.h"
#include "celluloid-video-area.h"
#include "celluloid-def.h"

static gchar *
keyval_to_keystr(guint keyval);

static gchar *
get_modstr(guint state);

static gchar *
get_full_keystr(guint keyval, GdkModifierType state);

static void
mouse_up(CelluloidController *controller, const guint button);

static gboolean
key_pressed_handler(	GtkEventControllerKey *key_controller,
			guint keyval,
			guint keycode,
			GdkModifierType state,
			gpointer data );

static void
key_released_handler(	GtkEventControllerKey *key_controller,
			guint keyval,
			guint keycode,
			GdkModifierType state,
			gpointer data );

static void
button_pressed_handler(	GtkGestureSingle *gesture,
			gint n_press,
			gdouble x,
			gdouble y,
			gpointer data );

static void
button_released_handler(	GtkGestureSingle *gesture,
				gint n_press,
				double x,
				gdouble y,
				gpointer data );

static void
button_stopped_handler(GtkGestureSingle *gesture, gpointer data);

static gboolean
mouse_move_handler(	GtkEventControllerMotion *motion_controller,
			gdouble x,
			gdouble y,
			gpointer data );

static gboolean
scroll_handler(	GtkEventControllerScroll *scroll_controller,
		gdouble dx,
		gdouble dy,
		gpointer data );

static gchar *
keyval_to_keystr(guint keyval)
{
	const gchar *keystrmap[] = KEYSTRING_MAP;
	gboolean found = FALSE;
	gchar key_utf8[7] = {0}; // 6 bytes for output and 1 for NULL terminator
	const gchar *result = key_utf8;

	g_unichar_to_utf8(gdk_keyval_to_unicode(keyval), key_utf8);
	result = result[0] ? result : gdk_keyval_name(keyval);
	found = !result;

	for(gint i = 0; !found && keystrmap[i]; i += 2)
	{
		const gint rc = g_ascii_strncasecmp
				(result, keystrmap[i+1], KEYSTRING_MAX_LEN);

		if(rc == 0)
		{
			result = keystrmap[i][0] ? keystrmap[i] : NULL;
			found = TRUE;
		}
	}

	return result ? g_strdup(result) : NULL;
}

static gchar *
get_modstr(guint state)
{
	const struct
	{
		guint mask;
		gchar *str;
	}
	mod_map[] = {	{GDK_SHIFT_MASK, "Shift+"},
			{GDK_CONTROL_MASK, "Ctrl+"},
			{GDK_ALT_MASK, "Alt+"},
			{GDK_SUPER_MASK, "Meta+"}, // Super is Meta in mpv
			{0, NULL} };

	const gsize max_len = G_N_ELEMENTS("Ctrl+Alt+Shift+Meta+") + 1;
	gchar *result = g_malloc0(max_len);

	for(gint i = 0; mod_map[i].str; i++)
	{
		if(state & mod_map[i].mask)
		{
			g_strlcat(result, mod_map[i].str, max_len);
		}
	}

	return result;
}

static gchar *
get_full_keystr(guint keyval, GdkModifierType state)
{
	gchar *modstr = get_modstr(state);
	gchar *keystr = keyval_to_keystr(keyval);
	char *result = keystr ? g_strconcat(modstr, keystr, NULL) : NULL;

	g_free(keystr);
	g_free(modstr);

	return result;
}

static void
mouse_up(CelluloidController *controller, const guint button)
{
	gchar *name = g_strdup_printf("MOUSE_BTN%u", button - 1);

	celluloid_model_key_up(controller->model, name);
	g_free(name);
}

static gboolean
key_pressed_handler(	GtkEventControllerKey *key_controller,
			guint keyval,
			guint keycode,
			GdkModifierType state,
			gpointer data )

{
	CelluloidController *controller = data;
	gchar *keystr = get_full_keystr(keyval, state);
	gboolean playlist_visible = celluloid_view_get_playlist_visible(controller->view);
	gboolean in_mpv_keymap = celluloid_model_input_binding_exists(controller->model, keystr);

	if(keystr && !playlist_visible)
	{
		celluloid_model_key_down(controller->model, keystr);
		g_free(keystr);
	}

	return keystr && !playlist_visible && in_mpv_keymap;
}

static void
key_released_handler(	GtkEventControllerKey *key_controller,
			guint keyval,
			guint keycode,
			GdkModifierType state,
			gpointer data )
{
	CelluloidController *controller = data;
	gchar *keystr = get_full_keystr(keyval, state);
	gboolean playlist_visible = celluloid_view_get_playlist_visible(controller->view);

	if(keystr && !playlist_visible)
	{
		celluloid_model_key_up(controller->model, keystr);
		g_free(keystr);
	}
}

static void
button_pressed_handler(	GtkGestureSingle *gesture,
			gint n_press,
			gdouble x,
			gdouble y,
			gpointer data )
{
	CelluloidController *controller =
		data;
	const guint button_number =
		gtk_gesture_single_get_current_button(gesture);

	if(button_number > 0)
	{
		gchar *button_name =
			g_strdup_printf("MOUSE_BTN%u", button_number - 1);

		celluloid_model_key_down(controller->model, button_name);
		g_free(button_name);

		CelluloidView *view = celluloid_controller_get_view(controller);
		gtk_window_set_focus(GTK_WINDOW(view), NULL);
	}
}

static void
button_stopped_handler(GtkGestureSingle *gesture, gpointer data)
{
	const guint button = gtk_gesture_single_get_current_button(gesture);

	if(button > 0)
	{
		mouse_up(CELLULOID_CONTROLLER(data), button);
	}
}

static void
button_released_handler(	GtkGestureSingle *gesture,
				gint n_press,
				double x,
				gdouble y,
				gpointer data )
{
	const guint button = gtk_gesture_single_get_current_button(gesture);

	if(button > 0)
	{
		mouse_up(CELLULOID_CONTROLLER(data), button);
	}
}

static gboolean
mouse_move_handler(	GtkEventControllerMotion *motion_controller,
			gdouble x,
			gdouble y,
			gpointer data )
{
	CelluloidController *controller = data;

	if(controller->model)
	{
		celluloid_model_mouse(controller->model, (gint)x, (gint)y);
	}

	return FALSE;
}

static gboolean
scroll_handler(	GtkEventControllerScroll *scroll_controller,
		gdouble dx,
		gdouble dy,
		gpointer data )
{
	guint button = 0;
	gint count = 0;

	/* Only one axis will be used at a time to prevent accidental activation
	 * of commands bound to buttons associated with the other axis.
	 */
	if(ABS(dx) > ABS(dy))
	{
		count = (gint)ABS(dx);

		if(dx <= -1)
		{
			button = 6;
		}
		else if(dx >= 1)
		{
			button = 7;
		}
	}
	else
	{
		count = (gint)ABS(dy);

		if(dy <= -1)
		{
			button = 4;
		}
		else if(dy >= 1)
		{
			button = 5;
		}
	}

	if(button > 0)
	{
		gchar *button_name = g_strdup_printf("MOUSE_BTN%u", button - 1);
		CelluloidModel *model = CELLULOID_CONTROLLER(data)->model;

		while(count-- > 0)
		{
			celluloid_model_key_press(model, button_name);
		}

		g_free(button_name);
	}

	return TRUE;
}

void
celluloid_controller_input_connect_signals(CelluloidController *controller)
{
	CelluloidMainWindow *wnd =	celluloid_view_get_main_window
					(controller->view);
	CelluloidVideoArea *video_area =	celluloid_main_window_get_video_area
						(wnd);

	g_signal_connect(	controller->key_controller,
				"key-pressed",
				G_CALLBACK(key_pressed_handler),
				controller );
	g_signal_connect(	controller->key_controller,
				"key-released",
				G_CALLBACK(key_released_handler),
				controller );

	GtkGesture *const click_gesture =
		controller->click_gesture;
	gtk_gesture_single_set_button
		(GTK_GESTURE_SINGLE(click_gesture), 0);
	gtk_widget_add_controller
		(GTK_WIDGET(video_area), GTK_EVENT_CONTROLLER(click_gesture));

	g_signal_connect(	click_gesture,
				"pressed",
				G_CALLBACK(button_pressed_handler),
				controller );
	g_signal_connect(	click_gesture,
				"released",
				G_CALLBACK(button_released_handler),
				controller );
	g_signal_connect(	click_gesture,
				"stopped",
				G_CALLBACK(button_stopped_handler),
				controller );

	GtkEventController *const motion_controller =
		controller->motion_controller;
	gtk_widget_add_controller
		(GTK_WIDGET(video_area), GTK_EVENT_CONTROLLER(motion_controller));

	g_signal_connect(	motion_controller,
				"motion",
				G_CALLBACK(mouse_move_handler),
				controller );

	GtkEventController *const scroll_controller =
		controller->scroll_controller;
	gtk_widget_add_controller
		(GTK_WIDGET(video_area), GTK_EVENT_CONTROLLER(scroll_controller));

	g_signal_connect(	scroll_controller,
				"scroll",
				G_CALLBACK(scroll_handler),
				controller );
}

void
celluloid_controller_input_disconnect_signals(CelluloidController *controller)
{
	CelluloidMainWindow *wnd =
		celluloid_view_get_main_window(controller->view);
	GtkWidget *video_area =
		GTK_WIDGET(celluloid_main_window_get_video_area(wnd));

	gtk_widget_remove_controller
		(video_area, controller->motion_controller);
	gtk_widget_remove_controller
		(video_area, controller->scroll_controller);
	gtk_widget_remove_controller
		(video_area, GTK_EVENT_CONTROLLER(controller->click_gesture));
}
