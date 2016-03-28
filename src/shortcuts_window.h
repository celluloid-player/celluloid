/*
 * Copyright (c) 2016 gnome-mpv
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

#ifndef SHORTCUTS_WINDOW_H
#define SHORTCUTS_WINDOW_H

#include <gtk/gtk.h>

#define SHORTCUTS_WINDOW_TYPE (shortcuts_window_get_type ())

#define	SHORTCUTS_WINDOW(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), SHORTCUTS_WINDOW_TYPE, ShortcutsWindow))

#define	SHORTCUTS_WINDOW_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass), SHORTCUTS_WINDOW_TYPE, ShortcutsWindowClass))

#define	IS_SHORTCUTS_WINDOW(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), SHORTCUTS_WINDOW_TYPE))

#define	IS_SHORTCUTS_WINDOW_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), SHORTCUTS_WINDOW_TYPE))

typedef struct ShortcutsWindow ShortcutsWindow;
typedef struct ShortcutsWindowClass ShortcutsWindowClass;

GtkWidget *shortcuts_window_new(GtkWindow *parent);
GType shortcuts_window_get_type(void);

#endif
