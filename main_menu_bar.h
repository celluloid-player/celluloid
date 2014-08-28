/*
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

#ifndef MAIN_MENU_H
#define MAIN_MENU_H

#include <gtk/gtk.h>

#include "def.h"

G_BEGIN_DECLS

#define MAIN_MENU_TYPE (main_menu_bar_get_type ())

#define	MAIN_MENU(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), MAIN_MENU_TYPE, MainMenuBar))

#define	MAIN_MENU_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass), MAIN_MENU_TYPE, MainMenuBarClass))

#define	IS_MAIN_MENU(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), MAIN_MENU_TYPE))

#define	IS_MAIN_MENU_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), MAIN_MENU_TYPE))

typedef struct _MainMenuBar MainMenuBar;
typedef struct _MainMenuBarClass MainMenuBarClass;

struct _MainMenuBar
{
	GtkMenuBar menu_bar;
	GtkWidget *file_menu;
	GtkWidget *file_menu_item;
	GtkWidget *open_menu_item;
	GtkWidget *open_loc_menu_item;
	GtkWidget *quit_menu_item;
	GtkWidget *edit_menu;
	GtkWidget *edit_menu_item;
	GtkWidget *pref_menu_item;
	GtkWidget *view_menu;
	GtkWidget *view_menu_item;
	GtkWidget *playlist_menu_item;
	GtkWidget *fullscreen_menu_item;
	GtkWidget *normal_size_menu_item;
	GtkWidget *double_size_menu_item;
	GtkWidget *half_size_menu_item;
	GtkWidget *help_menu;
	GtkWidget *help_menu_item;
	GtkWidget *about_menu_item;
};

struct _MainMenuBarClass
{
	GtkMenuBarClass parent_class;
};

GtkWidget *main_menu_bar_new(void);
GType main_menu_bar_get_type(void);

G_END_DECLS

#endif
