/*
 * Copyright (c) 2014-2016 gnome-mpv
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

#include "pref_dialog.h"
#include "main_window.h"

#define TOGGLE_BTN_PREF_MAP(VAR, DLG, PREF) \
const struct \
{ \
	GtkToggleButton *btn; \
	gboolean *value; \
} \
VAR[] = {	{GTK_TOGGLE_BUTTON(DLG->csd_enable_check), \
		&PREF->csd_enable}, \
		{GTK_TOGGLE_BUTTON(DLG->dark_theme_enable_check), \
		&PREF->dark_theme_enable}, \
		{GTK_TOGGLE_BUTTON(DLG->last_folder_enable_check), \
		&PREF->last_folder_enable}, \
		{GTK_TOGGLE_BUTTON(DLG->mpv_input_enable_check), \
		&PREF->mpv_input_config_enable}, \
		{GTK_TOGGLE_BUTTON(DLG->mpv_conf_enable_check), \
		&PREF->mpv_config_enable}, \
		{NULL, NULL} };

struct _PrefDialog
{
	GtkDialog parent_instance;
	GtkWidget *grid;
	GtkWidget *content_area;
	GtkWidget *csd_enable_check;
	GtkWidget *dark_theme_enable_check;
	GtkWidget *last_folder_enable_check;
	GtkWidget *mpv_input_enable_check;
	GtkWidget *mpv_input_button;
	GtkWidget *mpv_conf_enable_check;
	GtkWidget *mpv_conf_button;
	GtkWidget *mpv_options_entry;
};

struct _PrefDialogClass
{
	GtkDialogClass parent_class;
};

G_DEFINE_TYPE(PrefDialog, pref_dialog, GTK_TYPE_DIALOG)

static gboolean key_press_handler(GtkWidget *widget, GdkEventKey *event)
{
	guint keyval = event->keyval;
	guint state = event->state;

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

	return GTK_WIDGET_CLASS(pref_dialog_parent_class)->key_press_event (widget, event);
}

static void pref_dialog_class_init(PrefDialogClass *klass)
{
	GtkWidgetClass *wid_class = GTK_WIDGET_CLASS(klass);

	wid_class->key_press_event = key_press_handler;
}

static void pref_dialog_init(PrefDialog *dlg)
{
	GdkGeometry geom;
	GtkWidget *mpv_conf_label;
	GtkWidget *mpv_input_label;
	GtkWidget *mpv_options_label;
	GtkWidget *general_group_label;
	GtkWidget *mpv_conf_group_label;
	GtkWidget *mpv_input_group_label;
	GtkWidget *misc_group_label;
	gint grid_row;

	mpv_conf_label = gtk_label_new(_("MPV configuration file:"));
	mpv_input_label = gtk_label_new(_("MPV input configuration file:"));
	mpv_options_label = gtk_label_new(_("Extra MPV options:"));
	general_group_label = gtk_label_new(_("<b>General</b>"));
	mpv_conf_group_label = gtk_label_new(_("<b>MPV Configuration</b>"));
	mpv_input_group_label = gtk_label_new(_("<b>Keybindings</b>"));
	misc_group_label = gtk_label_new(_("<b>Miscellaneous</b>"));
	grid_row = 0;

	/* This 'locks' the height of the dialog while allowing the width to be
	 * freely adjusted.
	 */
	geom.max_width = G_MAXINT;
	geom.max_height = 0;

	dlg->grid = gtk_grid_new();
	dlg->content_area = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
	dlg->mpv_options_entry = gtk_entry_new();

	dlg->csd_enable_check
		= gtk_check_button_new_with_label
			(_("Enable client-side decorations"));

	dlg->dark_theme_enable_check
		= gtk_check_button_new_with_label
			(_("Enable dark theme"));

	dlg->last_folder_enable_check
		= gtk_check_button_new_with_label
			(_("Remember last file's location"));

	dlg->mpv_conf_button
		= gtk_file_chooser_button_new
			(	_("MPV configuration file"),
				GTK_FILE_CHOOSER_ACTION_OPEN );

	dlg->mpv_input_button
		= gtk_file_chooser_button_new
			(	_("MPV input configuration file"),
				GTK_FILE_CHOOSER_ACTION_OPEN );

	dlg->mpv_conf_enable_check
		= gtk_check_button_new_with_label
			(_("Load MPV configuration file"));

	dlg->mpv_input_enable_check
		= gtk_check_button_new_with_label
			(_("Load MPV input configuration file"));

	gtk_label_set_use_markup(GTK_LABEL(general_group_label), TRUE);
	gtk_label_set_use_markup(GTK_LABEL(mpv_conf_group_label), TRUE);
	gtk_label_set_use_markup(GTK_LABEL(mpv_input_group_label), TRUE);
	gtk_label_set_use_markup(GTK_LABEL(misc_group_label), TRUE);

	gtk_widget_set_margin_top(mpv_conf_group_label, 12);
	gtk_widget_set_margin_top(mpv_input_group_label, 12);
	gtk_widget_set_margin_top(misc_group_label, 12);
	gtk_grid_set_row_spacing(GTK_GRID(dlg->grid), 6);
	gtk_grid_set_column_spacing(GTK_GRID(dlg->grid), 12);

	if(!gtk_dialog_get_header_bar(GTK_DIALOG(dlg)))
	{
		gtk_widget_set_margin_bottom(dlg->grid, 6);
	}

	gtk_widget_set_halign(mpv_conf_label, GTK_ALIGN_START);
	gtk_widget_set_halign(mpv_input_label, GTK_ALIGN_START);
	gtk_widget_set_halign(mpv_options_label, GTK_ALIGN_START);
	gtk_widget_set_halign(general_group_label, GTK_ALIGN_START);
	gtk_widget_set_halign(mpv_conf_group_label, GTK_ALIGN_START);
	gtk_widget_set_halign(mpv_input_group_label, GTK_ALIGN_START);
	gtk_widget_set_halign(misc_group_label, GTK_ALIGN_START);

	gtk_widget_set_hexpand(mpv_conf_label, FALSE);
	gtk_widget_set_hexpand(mpv_input_label, FALSE);
	gtk_widget_set_hexpand(mpv_options_label, FALSE);
	gtk_widget_set_hexpand(dlg->mpv_conf_button, TRUE);
	gtk_widget_set_hexpand(dlg->mpv_input_button, TRUE);
	gtk_widget_set_hexpand(dlg->mpv_options_entry, TRUE);

	gtk_widget_set_margin_start(mpv_conf_label, 12);
	gtk_widget_set_margin_start(mpv_input_label, 12);
	gtk_widget_set_margin_start(mpv_options_label, 12);
	gtk_widget_set_margin_start(dlg->csd_enable_check, 12);
	gtk_widget_set_margin_start(dlg->dark_theme_enable_check, 12);
	gtk_widget_set_margin_start(dlg->last_folder_enable_check, 12);
	gtk_widget_set_margin_start(dlg->mpv_conf_enable_check, 12);
	gtk_widget_set_margin_start(dlg->mpv_input_enable_check, 12);
	gtk_widget_set_margin_start(dlg->mpv_options_entry, 12);

	gtk_widget_set_size_request(dlg->mpv_conf_button, 100, -1);
	gtk_widget_set_size_request(dlg->mpv_input_button, 100, -1);

	gtk_window_set_geometry_hints(	GTK_WINDOW(dlg),
					GTK_WIDGET(dlg),
					&geom,
					GDK_HINT_MAX_SIZE );

	gtk_container_set_border_width(GTK_CONTAINER(dlg->content_area), 12);
	gtk_container_add(GTK_CONTAINER(dlg->content_area), dlg->grid);

	gtk_grid_attach(	GTK_GRID(dlg->grid),
				general_group_label,
				0, grid_row++, 1, 1 );

	gtk_grid_attach(	GTK_GRID(dlg->grid),
				dlg->csd_enable_check,
				0, grid_row++, 2, 1 );

	gtk_grid_attach(	GTK_GRID(dlg->grid),
	 			dlg->dark_theme_enable_check,
				0, grid_row++, 2, 1 );

	gtk_grid_attach(	GTK_GRID(dlg->grid),
	 			dlg->last_folder_enable_check,
				0, grid_row++, 2, 1 );

	gtk_grid_attach(	GTK_GRID(dlg->grid),
	 			mpv_conf_group_label,
				0, grid_row++, 1, 1 );

	gtk_grid_attach(	GTK_GRID(dlg->grid),
	 			dlg->mpv_conf_enable_check,
				0, grid_row++, 2, 1 );

	gtk_grid_attach(	GTK_GRID(dlg->grid),
	 			mpv_conf_label,
				0, grid_row, 1, 1 );

	gtk_grid_attach(	GTK_GRID(dlg->grid),
	 			dlg->mpv_conf_button,
				1, grid_row++, 1, 1 );

	gtk_grid_attach(	GTK_GRID(dlg->grid),
	 			mpv_input_group_label,
				0, grid_row++, 1, 1 );

	gtk_grid_attach(	GTK_GRID(dlg->grid),
	 			dlg->mpv_input_enable_check,
				0, grid_row++, 2, 1 );

	gtk_grid_attach(	GTK_GRID(dlg->grid),
	 			mpv_input_label,
				0, grid_row, 1, 1 );

	gtk_grid_attach(	GTK_GRID(dlg->grid),
	 			dlg->mpv_input_button,
				1, grid_row++, 1, 1 );

	gtk_grid_attach(	GTK_GRID(dlg->grid),
	 			misc_group_label,
				0, grid_row++, 1, 1 );

	gtk_grid_attach(	GTK_GRID(dlg->grid),
	 			mpv_options_label,
				0, grid_row++, 1, 1 );

	gtk_grid_attach(	GTK_GRID(dlg->grid),
				dlg->mpv_options_entry,
				0, grid_row++, 2, 1 );

	gtk_dialog_add_buttons(	GTK_DIALOG(dlg),
				_("_Cancel"),
				GTK_RESPONSE_REJECT,
				_("_Save"),
				GTK_RESPONSE_ACCEPT,
				NULL );

	gtk_dialog_set_default_response (GTK_DIALOG(dlg), GTK_RESPONSE_ACCEPT);
}

GtkWidget *pref_dialog_new(GtkWindow *parent)
{
	GtkWidget *dlg;
	GtkWidget *header_bar;
	gboolean csd_enabled;

	csd_enabled = main_window_get_csd_enabled(MAIN_WINDOW(parent));

	dlg = g_object_new(	pref_dialog_get_type(),
				"title", _("Preferences"),
				"modal", TRUE,
				"transient-for", parent,
				"use-header-bar", csd_enabled,
				NULL );

	header_bar = gtk_dialog_get_header_bar(GTK_DIALOG(dlg));

	gtk_widget_hide_on_delete (dlg);
	gtk_widget_show_all (dlg);

	if (header_bar)
	{
		/* The defaults use PACK_END which is ugly with multiple buttons
		 */
		GtkWidget *cancel_btn = gtk_dialog_get_widget_for_response
					(GTK_DIALOG(dlg), GTK_RESPONSE_REJECT);

		gtk_container_child_set(	GTK_CONTAINER(header_bar),
						cancel_btn,
						"pack-type",
						GTK_PACK_START,
						NULL );

		gtk_header_bar_set_show_close_button
			(GTK_HEADER_BAR(header_bar), FALSE);
	}

	return dlg;
}

pref_store *pref_dialog_get_pref(PrefDialog *dlg)
{
	pref_store *pref = g_malloc(sizeof(pref_store));
	GtkFileChooser *conf_chooser = GTK_FILE_CHOOSER(dlg->mpv_conf_button);
	GtkFileChooser *input_chooser = GTK_FILE_CHOOSER(dlg->mpv_input_button);
	GtkEntry *mpv_options_entry = GTK_ENTRY(dlg->mpv_options_entry);

	TOGGLE_BTN_PREF_MAP(toggle_btn, dlg, pref)

	for(gint i = 0; toggle_btn[i].btn; i++)
	{
		*(toggle_btn[i].value)
			= gtk_toggle_button_get_active(toggle_btn[i].btn);
	}

	pref->mpv_config_file
		= gtk_file_chooser_get_filename(conf_chooser);

	pref->mpv_input_config_file
		= gtk_file_chooser_get_filename(input_chooser);

	pref->mpv_options
		= g_strdup(gtk_entry_get_text(mpv_options_entry));

	return pref;
}

void pref_dialog_set_pref(PrefDialog *dlg, pref_store *pref)
{
	GtkFileChooser *conf_chooser = GTK_FILE_CHOOSER(dlg->mpv_conf_button);
	GtkFileChooser *input_chooser = GTK_FILE_CHOOSER(dlg->mpv_input_button);
	GtkEntry *mpv_options_entry = GTK_ENTRY(dlg->mpv_options_entry);

	TOGGLE_BTN_PREF_MAP(toggle_btn, dlg, pref)

	for(gint i = 0; toggle_btn[i].btn; i++)
	{
		gtk_toggle_button_set_active
			(toggle_btn[i].btn, *(toggle_btn[i].value));
	}

	gtk_file_chooser_set_filename
		(conf_chooser, pref->mpv_config_file);

	gtk_file_chooser_set_filename
		(input_chooser, pref->mpv_input_config_file);

	gtk_entry_set_text
		(mpv_options_entry, pref->mpv_options);
}
