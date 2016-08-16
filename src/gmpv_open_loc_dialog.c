/*
 * Copyright (c) 2014, 2016 gnome-mpv
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

#include "gmpv_open_loc_dialog.h"
#include "gmpv_main_window.h"

struct _GmpvOpenLocDialog
{
	GtkDialog parent_instance;
	GtkWidget *content_area;
	GtkWidget *content_box;
	GtkWidget *loc_label;
	GtkWidget *loc_entry;
};

struct _GmpvOpenLocDialogClass
{
	GtkDialogClass parent_class;
};

G_DEFINE_TYPE(GmpvOpenLocDialog, gmpv_open_loc_dialog, GTK_TYPE_DIALOG)

static gboolean key_press_handler (GtkWidget *widget, GdkEventKey *event)
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

	return	GTK_WIDGET_CLASS(gmpv_open_loc_dialog_parent_class)
		->key_press_event (widget, event);
}

static void gmpv_open_loc_dialog_class_init(GmpvOpenLocDialogClass *klass)
{
	GtkWidgetClass *wid_class = GTK_WIDGET_CLASS(klass);

	wid_class->key_press_event = key_press_handler;
}

static void gmpv_open_loc_dialog_init(GmpvOpenLocDialog *dlg)
{
	GdkGeometry geom;

	geom.max_width = G_MAXINT;
	geom.max_height = 0;
	dlg->content_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	dlg->content_area = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
	dlg->loc_label = gtk_label_new(_("Location:"));
	dlg->loc_entry = gtk_entry_new();

	gtk_dialog_add_buttons(	GTK_DIALOG(dlg),
				_("_Cancel"),
				GTK_RESPONSE_REJECT,
				_("_Open"),
				GTK_RESPONSE_ACCEPT,
				NULL );

	gtk_window_set_geometry_hints(	GTK_WINDOW(dlg),
					GTK_WIDGET(dlg),
					&geom,
					GDK_HINT_MAX_SIZE );

	gtk_window_set_modal(GTK_WINDOW(dlg), 1);
	gtk_window_set_title(GTK_WINDOW(dlg), _("Open Location"));
	gtk_container_set_border_width(GTK_CONTAINER(dlg->content_area), 12);

	if(!gtk_dialog_get_header_bar(GTK_DIALOG(dlg)))
	{
		gtk_widget_set_margin_bottom(dlg->content_box, 6);
	}

	gtk_window_set_geometry_hints(	GTK_WINDOW(dlg),
					GTK_WIDGET(dlg),
					&geom,
					GDK_HINT_MAX_SIZE );

	gtk_container_add(GTK_CONTAINER(dlg->content_area), dlg->content_box);

	gtk_box_pack_start(	GTK_BOX(dlg->content_box),
				dlg->loc_label,
				FALSE,
				FALSE,
				0 );

	gtk_box_pack_start(	GTK_BOX(dlg->content_box),
				dlg->loc_entry,
				TRUE,
				TRUE,
				0 );

	gtk_window_set_default_size(GTK_WINDOW(dlg), 350, -1);
	gtk_dialog_set_default_response (GTK_DIALOG(dlg), GTK_RESPONSE_ACCEPT);
}

GtkWidget *gmpv_open_loc_dialog_new(GtkWindow *parent)
{
	GtkWidget *dlg;
	GtkWidget *header_bar;
	gboolean csd_enabled;

	csd_enabled = gmpv_main_window_get_csd_enabled(GMPV_MAIN_WINDOW(parent));

	dlg = g_object_new(	gmpv_open_loc_dialog_get_type(),
				"use-header-bar", csd_enabled,
				NULL );

	header_bar = gtk_dialog_get_header_bar(GTK_DIALOG(dlg));

	if(header_bar)
	{
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

	gtk_widget_hide_on_delete(dlg);
	gtk_window_set_transient_for(GTK_WINDOW(dlg), parent);
	gtk_widget_show_all(dlg);

	return dlg;
}

const gchar *gmpv_open_loc_dialog_get_string(GmpvOpenLocDialog *dlg)
{
	return gtk_entry_get_text(GTK_ENTRY(dlg->loc_entry));
}

guint64 gmpv_open_loc_dialog_get_string_length(GmpvOpenLocDialog *dlg)
{
	return gtk_entry_get_text_length(GTK_ENTRY(dlg->loc_entry));
}
