/*
 * Copyright (c) 2014-2015 gnome-mpv
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

#include "pref_dialog.h"

static void response_handler(	GtkDialog *dialog,
				gint response_id,
				gpointer data );

static gboolean key_press_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data );

static void pref_dialog_init(PrefDialog *dlg);

static void response_handler(GtkDialog *dialog, gint response_id, gpointer data)
{
	gtk_widget_hide(GTK_WIDGET(dialog));
}

static gboolean key_press_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data )
{
	guint keyval = ((GdkEventKey*)event)->keyval;
	guint state = ((GdkEventKey*)event)->state;

	const guint mod_mask =	GDK_MODIFIER_MASK
				&~(GDK_SHIFT_MASK
				|GDK_LOCK_MASK
				|GDK_MOD2_MASK
				|GDK_MOD3_MASK
				|GDK_MOD4_MASK
				|GDK_MOD5_MASK);

	if((state&mod_mask) == 0 && keyval == GDK_KEY_Return)
	{
		gtk_dialog_response(GTK_DIALOG(widget), GTK_RESPONSE_ACCEPT);
	}

	return FALSE;
}

static void pref_dialog_init(PrefDialog *dlg)
{
	GdkGeometry geom;
	GtkWidget *mpvconf_label;
	GtkWidget *mpvinput_label;
	GtkWidget *mpvopt_label;
	GtkWidget *mpvconf_group_label;
	GtkWidget *mpvinput_group_label;
	GtkWidget *misc_group_label;

	mpvconf_label = gtk_label_new("MPV configuration file:");
	mpvinput_label = gtk_label_new("MPV input configuration file:");
	mpvopt_label = gtk_label_new("Extra MPV options:");
	mpvconf_group_label = gtk_label_new("<b>MPV Configuration</b>");
	mpvinput_group_label = gtk_label_new("<b>Keybindings</b>");
	misc_group_label = gtk_label_new("<b>Miscellaneous</b>");

	/* This 'locks' the height of the dialog while allowing the width to be
	 * freely adjusted.
	 */
	geom.max_width = G_MAXINT;
	geom.max_height = 0;

	gtk_label_set_use_markup(GTK_LABEL(mpvconf_group_label), TRUE);
	gtk_label_set_use_markup(GTK_LABEL(mpvinput_group_label), TRUE);
	gtk_label_set_use_markup(GTK_LABEL(misc_group_label), TRUE);

	dlg->grid = gtk_grid_new();
	dlg->content_area = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
	dlg->mpvopt_entry = gtk_entry_new();

	dlg->mpvconf_button
		= gtk_file_chooser_button_new(	"MPV configuration file",
						GTK_FILE_CHOOSER_ACTION_OPEN );

	dlg->mpvinput_button
		= gtk_file_chooser_button_new(	"MPV input configuration file",
						GTK_FILE_CHOOSER_ACTION_OPEN );

	dlg->mpvconf_enable_check
		= gtk_check_button_new_with_label
			("Load MPV configuration file");

	dlg->mpvinput_enable_check
		= gtk_check_button_new_with_label
			("Load MPV input configuration file");

	gtk_widget_set_margin_top(mpvinput_group_label, 10);
	gtk_widget_set_margin_top(misc_group_label, 10);
	gtk_widget_set_margin_bottom(dlg->grid, 5);
	gtk_grid_set_row_spacing(GTK_GRID(dlg->grid), 5);
	gtk_grid_set_column_spacing(GTK_GRID(dlg->grid), 5);

	gtk_widget_set_halign(mpvconf_label, GTK_ALIGN_START);
	gtk_widget_set_halign(mpvinput_label, GTK_ALIGN_START);
	gtk_widget_set_halign(mpvopt_label, GTK_ALIGN_START);
	gtk_widget_set_halign(mpvconf_group_label, GTK_ALIGN_START);
	gtk_widget_set_halign(mpvinput_group_label, GTK_ALIGN_START);
	gtk_widget_set_halign(misc_group_label, GTK_ALIGN_START);

	gtk_widget_set_hexpand(mpvconf_label, FALSE);
	gtk_widget_set_hexpand(mpvinput_label, FALSE);
	gtk_widget_set_hexpand(mpvopt_label, FALSE);
	gtk_widget_set_hexpand(dlg->mpvconf_button, TRUE);
	gtk_widget_set_hexpand(dlg->mpvinput_button, TRUE);
	gtk_widget_set_hexpand(dlg->mpvopt_entry, TRUE);

	gtk_widget_set_margin_start(mpvconf_label, 10);
	gtk_widget_set_margin_start(mpvinput_label, 10);
	gtk_widget_set_margin_start(mpvopt_label, 10);
	gtk_widget_set_margin_start(dlg->mpvconf_enable_check, 10);
	gtk_widget_set_margin_start(dlg->mpvinput_enable_check, 10);
	gtk_widget_set_margin_start(dlg->mpvopt_entry, 10);

	gtk_widget_set_size_request(dlg->mpvconf_button, 100, -1);
	gtk_widget_set_size_request(dlg->mpvinput_button, 100, -1);

	gtk_dialog_add_buttons(	GTK_DIALOG(dlg),
				"_Save",
				GTK_RESPONSE_ACCEPT,
				"_Cancel",
				GTK_RESPONSE_REJECT,
				NULL );

	gtk_window_set_modal(GTK_WINDOW(dlg), 1);
	gtk_window_set_title(GTK_WINDOW(dlg), "Preferences");

	gtk_window_set_geometry_hints(	GTK_WINDOW(dlg),
					GTK_WIDGET(dlg),
					&geom,
					GDK_HINT_MAX_SIZE );

	g_signal_connect(	dlg,
				"response",
				G_CALLBACK(response_handler),
				NULL );

	g_signal_connect(	dlg,
				"key-press-event",
				G_CALLBACK(key_press_handler),
				NULL );

	gtk_container_set_border_width(GTK_CONTAINER(dlg->content_area), 5);
	gtk_container_add(GTK_CONTAINER(dlg->content_area), dlg->grid);

	gtk_grid_attach(GTK_GRID(dlg->grid), mpvconf_group_label, 0, 0, 1, 1);

	gtk_grid_attach
		(GTK_GRID(dlg->grid), dlg->mpvconf_enable_check, 0, 1, 2, 1);

	gtk_grid_attach(GTK_GRID(dlg->grid), mpvconf_label, 0, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(dlg->grid), dlg->mpvconf_button, 1, 2, 1, 1);

	gtk_grid_attach(GTK_GRID(dlg->grid), mpvinput_group_label, 0, 3, 1, 1);

	gtk_grid_attach
		(GTK_GRID(dlg->grid), dlg->mpvinput_enable_check, 0, 4, 2, 1);

	gtk_grid_attach(GTK_GRID(dlg->grid), mpvinput_label, 0, 5, 1, 1);
	gtk_grid_attach(GTK_GRID(dlg->grid), dlg->mpvinput_button, 1, 5, 1, 1);

	gtk_grid_attach(GTK_GRID(dlg->grid), misc_group_label, 0, 6, 1, 1);
	gtk_grid_attach(GTK_GRID(dlg->grid), mpvopt_label, 0, 7, 1, 1);
	gtk_grid_attach(GTK_GRID(dlg->grid), dlg->mpvopt_entry, 0, 8, 2, 1);
}

GtkWidget *pref_dialog_new(GtkWindow *parent)
{
	PrefDialog *dlg = g_object_new(pref_dialog_get_type(), NULL);

	gtk_window_set_transient_for(GTK_WINDOW(dlg), parent);
	gtk_widget_show_all(GTK_WIDGET(dlg));

	return GTK_WIDGET(dlg);
}

GType pref_dialog_get_type()
{
	static GType dlg_type = 0;

	if(dlg_type == 0)
	{
		const GTypeInfo dlg_info
			= {	sizeof(PrefDialogClass),
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				sizeof(PrefDialog),
				0,
				(GInstanceInitFunc)pref_dialog_init };

		dlg_type = g_type_register_static(	GTK_TYPE_DIALOG,
							"PrefDialog",
							&dlg_info,
							0 );
	}

	return dlg_type;
}

void pref_dialog_set_mpvconf_enable(PrefDialog *dlg, gboolean value)
{
	GtkToggleButton *button = GTK_TOGGLE_BUTTON(dlg->mpvconf_enable_check);

	gtk_toggle_button_set_active(button, value);
}

gboolean pref_dialog_get_mpvconf_enable(PrefDialog *dlg)
{
	GtkToggleButton *button = GTK_TOGGLE_BUTTON(dlg->mpvconf_enable_check);

	return gtk_toggle_button_get_active(button);
}

void pref_dialog_set_mpvconf(PrefDialog *dlg, const gchar *buffer)
{
	GtkFileChooser *chooser = GTK_FILE_CHOOSER(dlg->mpvconf_button);

	gtk_file_chooser_set_filename(chooser, buffer);
}

gchar *pref_dialog_get_mpvconf(PrefDialog *dlg)
{
	GtkFileChooser *chooser = GTK_FILE_CHOOSER(dlg->mpvconf_button);

	return gtk_file_chooser_get_filename(chooser);
}

void pref_dialog_set_mpvinput_enable(PrefDialog *dlg, gboolean value)
{
	GtkToggleButton *button = GTK_TOGGLE_BUTTON(dlg->mpvinput_enable_check);

	gtk_toggle_button_set_active(button, value);
}

gboolean pref_dialog_get_mpvinput_enable(PrefDialog *dlg)
{
	GtkToggleButton *button = GTK_TOGGLE_BUTTON(dlg->mpvinput_enable_check);

	return gtk_toggle_button_get_active(button);
}

void pref_dialog_set_mpvinput(PrefDialog *dlg, const gchar *buffer)
{
	GtkFileChooser *chooser = GTK_FILE_CHOOSER(dlg->mpvinput_button);

	gtk_file_chooser_set_filename(chooser, buffer);
}

gchar *pref_dialog_get_mpvinput(PrefDialog *dlg)
{
	GtkFileChooser *chooser = GTK_FILE_CHOOSER(dlg->mpvinput_button);

	return gtk_file_chooser_get_filename(chooser);
}

void pref_dialog_set_mpvopt(PrefDialog *dlg, gchar *buffer)
{
	gtk_entry_set_text(GTK_ENTRY(dlg->mpvopt_entry), buffer);
}

const gchar *pref_dialog_get_mpvopt(PrefDialog *dlg)
{
	return gtk_entry_get_text(GTK_ENTRY(dlg->mpvopt_entry));
}
