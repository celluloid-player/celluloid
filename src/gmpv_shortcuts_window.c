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

#include <glib/gi18n.h>

#include "gmpv_shortcuts_window.h"

struct _GmpvShortcutsWindow
{
	GtkShortcutsWindow parent;
};

struct _GmpvShortcutsWindowClass
{
	GtkShortcutsWindowClass parent_class;
};

G_DEFINE_TYPE(GmpvShortcutsWindow, gmpv_shortcuts_window, GTK_TYPE_SHORTCUTS_WINDOW)

static void gmpv_shortcuts_window_init(GmpvShortcutsWindow *wnd)
{
	const struct
	{
		const gchar *accelerator;
		const gchar *title;
	}
	shortcuts[] = {	{"<Ctrl>O", _("Open file")},
			{"<Ctrl>L", _("Open location")},
			{"<Ctrl>Q", _("Quit")},
			{"<Ctrl>P", _("Show preferences dialog")},
			{"F9", _("Toggle playlist")},
			{"<Ctrl>S", _("Save playlist")},
			{"Delete", _("Remove selected playlist item")},
			{"F11", _("Toggle fullscreen mode")},
			{"Escape", _("Leave fullscreen mode")},
			{"<Ctrl>1", _("Normal size")},
			{"<Ctrl>2", _("Double size")},
			{"<Ctrl>3", _("Half size")},
			{NULL, NULL} };
	GtkWidget *section;
	GtkWidget *group;

	section = g_object_new(	gtk_shortcuts_section_get_type(),
				"section-name", "shortcuts",
				"visible", TRUE,
				NULL );
	group = g_object_new(	gtk_shortcuts_group_get_type(),
				"title", "General",
				NULL );

	for(gint i  = 0; shortcuts[i].accelerator; i++)
	{
		GtkWidget *current;

		current = g_object_new(	gtk_shortcuts_shortcut_get_type(),
					"accelerator", shortcuts[i].accelerator,
					"title", shortcuts[i].title,
					NULL );

		gtk_container_add(GTK_CONTAINER(group), current);
	}

	/* The widgets must be added in this order or search won't work */
	gtk_container_add(GTK_CONTAINER(section), group);
	gtk_container_add(GTK_CONTAINER(wnd), section);
}

static void gmpv_shortcuts_window_class_init(GmpvShortcutsWindowClass *klass)
{
}

GtkWidget *gmpv_shortcuts_window_new(GtkWindow *parent)
{
	return GTK_WIDGET(g_object_new(	gmpv_shortcuts_window_get_type(),
					"transient-for", parent,
					"modal", TRUE,
					NULL));
}
