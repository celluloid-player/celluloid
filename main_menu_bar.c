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

#include "main_menu_bar.h"

static void main_menu_bar_init(MainMenuBar *menu);

static void main_menu_bar_init(MainMenuBar *menu)
{
	menu->file_menu = gtk_menu_new();
	menu->file_menu_item = gtk_menu_item_new_with_mnemonic("_File");
	menu->open_menu_item = gtk_menu_item_new_with_mnemonic("_Open");
	menu->quit_menu_item = gtk_menu_item_new_with_mnemonic("_Quit");
	menu->edit_menu = gtk_menu_new();
	menu->edit_menu_item = gtk_menu_item_new_with_mnemonic("_Edit");
	menu->pref_menu_item = gtk_menu_item_new_with_mnemonic("_Preferences");
	menu->view_menu = gtk_menu_new();
	menu->view_menu_item = gtk_menu_item_new_with_mnemonic("_View");
	menu->help_menu = gtk_menu_new();
	menu->help_menu_item = gtk_menu_item_new_with_mnemonic("_Help");
	menu->about_menu_item = gtk_menu_item_new_with_mnemonic("_About");

	menu->open_loc_menu_item
		= gtk_menu_item_new_with_mnemonic("Open _Location");

	menu->playlist_menu_item
		= gtk_menu_item_new_with_mnemonic("_Playlist");

	menu->fullscreen_menu_item
		= gtk_menu_item_new_with_mnemonic("_Fullscreen");

	menu->normal_size_menu_item
		= gtk_menu_item_new_with_mnemonic("_Normal Size");

	menu->double_size_menu_item
		= gtk_menu_item_new_with_mnemonic("_Double Size");

	menu->half_size_menu_item
		= gtk_menu_item_new_with_mnemonic("_Half Size");

	gtk_menu_shell_append(	GTK_MENU_SHELL(menu),
				menu->file_menu_item );

	gtk_menu_shell_append(	GTK_MENU_SHELL(menu),
				menu->edit_menu_item );

	gtk_menu_shell_append(	GTK_MENU_SHELL(menu),
				menu->view_menu_item );

	gtk_menu_shell_append(	GTK_MENU_SHELL(menu),
				menu->help_menu_item );

	gtk_menu_item_set_submenu(	GTK_MENU_ITEM(menu->file_menu_item),
					menu->file_menu );

	gtk_menu_item_set_submenu(	GTK_MENU_ITEM(menu->edit_menu_item),
					menu->edit_menu );

	gtk_menu_item_set_submenu(	GTK_MENU_ITEM(menu->view_menu_item),
					menu->view_menu );

	gtk_menu_item_set_submenu(	GTK_MENU_ITEM(menu->help_menu_item),
					menu->help_menu );

	gtk_menu_shell_append(	GTK_MENU_SHELL(menu->file_menu),
				menu->open_menu_item );

	gtk_menu_shell_append(	GTK_MENU_SHELL(menu->file_menu),
				menu->open_loc_menu_item );

	gtk_menu_shell_append(	GTK_MENU_SHELL(menu->file_menu),
				gtk_separator_menu_item_new() );

	gtk_menu_shell_append(	GTK_MENU_SHELL(menu->file_menu),
				menu->quit_menu_item );

	gtk_menu_shell_append(	GTK_MENU_SHELL(menu->edit_menu),
				menu->pref_menu_item );

	gtk_menu_shell_append(	GTK_MENU_SHELL(menu->view_menu),
				menu->playlist_menu_item );

	gtk_menu_shell_append(	GTK_MENU_SHELL(menu->view_menu),
				gtk_separator_menu_item_new() );

	gtk_menu_shell_append(	GTK_MENU_SHELL(menu->view_menu),
				menu->fullscreen_menu_item );

	gtk_menu_shell_append(	GTK_MENU_SHELL(menu->view_menu),
				gtk_separator_menu_item_new() );

	gtk_menu_shell_append(	GTK_MENU_SHELL(menu->view_menu),
				menu->normal_size_menu_item );

	gtk_menu_shell_append(	GTK_MENU_SHELL(menu->view_menu),
				menu->double_size_menu_item );

	gtk_menu_shell_append(	GTK_MENU_SHELL(menu->view_menu),
				menu->half_size_menu_item );

	gtk_menu_shell_append(	GTK_MENU_SHELL(menu->help_menu),
				menu->about_menu_item );
}

GtkWidget *main_menu_bar_new()
{
	return GTK_WIDGET(g_object_new(main_menu_bar_get_type(), NULL));
}

GType main_menu_bar_get_type()
{
	static GType menu_type = 0;

	if(menu_type == 0)
	{
		const GTypeInfo menu_info
			= {	sizeof(MainMenuBarClass),
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				sizeof(MainMenuBar),
				0,
				(GInstanceInitFunc)main_menu_bar_init };

		menu_type = g_type_register_static(	GTK_TYPE_MENU_BAR,
							"MainMenuBar",
							&menu_info,
							0 );
	}

	return menu_type;
}
