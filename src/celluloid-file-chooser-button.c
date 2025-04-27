/*
 * Copyright (c) 2021, 2024 gnome-mpv
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

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "celluloid-file-chooser-button.h"
#include "celluloid-file-dialog.h"

struct _CelluloidFileChooserButton
{
	GtkButton parent;
	CelluloidFileDialog *dialog;
	GtkWidget *label;
	GFile *file;

	gchar *title;
	GtkFileChooserAction action;
};

struct _CelluloidFileChooserButtonClass
{
	GtkButtonClass parent_class;
};

static void
finalize(GObject *object);

static void
clicked(GtkButton *button);

static void
set_file(CelluloidFileChooserButton *self, GFile *file);

static void
open_dialog_callback(GObject *source_object, GAsyncResult *res, gpointer data);

G_DEFINE_TYPE(CelluloidFileChooserButton, celluloid_file_chooser_button, GTK_TYPE_BUTTON)

static void
finalize(GObject *object)
{
	CelluloidFileChooserButton *button = CELLULOID_FILE_CHOOSER_BUTTON(object);

	g_clear_object(&button->dialog);
	g_clear_object(&button->file);
	g_clear_object(&button->title);

	G_OBJECT_CLASS(celluloid_file_chooser_button_parent_class)
		->finalize(object);
}

static void
clicked(GtkButton *button)
{
	CelluloidFileChooserButton *self =
		CELLULOID_FILE_CHOOSER_BUTTON(button);
	GtkWindow *window =
		GTK_WINDOW(gtk_widget_get_root(GTK_WIDGET(self)));

	celluloid_file_dialog_open
		(	self->dialog,
			window,
			NULL,
			open_dialog_callback,
			button );
}

static void
set_file(CelluloidFileChooserButton *self, GFile *file)
{
	const gboolean file_changed =
		(!file != !self->file) ||
		(file && self->file && !g_file_equal(file, self->file));

	g_clear_object(&self->file);
	self->file = g_object_ref(file);

	if(file_changed)
	{
		gchar *basename = g_file_get_basename(self->file);

		gtk_label_set_text(GTK_LABEL(self->label), basename);
		g_signal_emit_by_name(self, "file-set");

		g_free(basename);
	}
}

static void
open_dialog_callback(GObject *source_object, GAsyncResult *res, gpointer data)
{
	CelluloidFileChooserButton *self = CELLULOID_FILE_CHOOSER_BUTTON(data);
	CelluloidFileDialog *dialog = CELLULOID_FILE_DIALOG(source_object);
	GFile *file = celluloid_file_dialog_open_finish(dialog, res, NULL);

	if(file)
	{
		set_file(self, file);
		g_object_unref(file);
	}

	g_object_unref(dialog);
}

static void
celluloid_file_chooser_button_class_init(CelluloidFileChooserButtonClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	GtkButtonClass *button_class = GTK_BUTTON_CLASS(klass);

	object_class->finalize = finalize;
	button_class->clicked = clicked;

	g_signal_new(	"file-set",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST|G_SIGNAL_DETAILED,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0 );
}

static void
celluloid_file_chooser_button_init(CelluloidFileChooserButton *self)
{
	self->dialog = celluloid_file_dialog_new(FALSE);
	self->label = gtk_label_new(_("(None)"));
	self->file = NULL;
	self->title = NULL;
	self->action = GTK_FILE_CHOOSER_ACTION_OPEN;

	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	GtkWidget *icon = gtk_image_new_from_icon_name("document-open-symbolic");

	celluloid_file_dialog_set_title(self->dialog, _("Open File…"));

	gtk_widget_set_halign(self->label, GTK_ALIGN_START);
	gtk_widget_set_halign(icon, GTK_ALIGN_END);
	gtk_widget_set_hexpand(icon, TRUE);

	gtk_box_append(GTK_BOX(box), self->label);
	gtk_box_append(GTK_BOX(box), icon);
	gtk_button_set_child(GTK_BUTTON(self), box);
}

GtkWidget *
celluloid_file_chooser_button_new(void)
{
	return GTK_WIDGET(g_object_new(	celluloid_file_chooser_button_get_type(),
					NULL ));
}

void
celluloid_file_chooser_button_set_file(	CelluloidFileChooserButton *self,
					GFile *file,
					GError **error )
{
	set_file(self, file);
	celluloid_file_dialog_set_initial_file(self->dialog, file);
}

GFile *
celluloid_file_chooser_button_get_file(CelluloidFileChooserButton *self)
{
	return self->file;
}

void
celluloid_file_chooser_button_set_filter(	CelluloidFileChooserButton *self,
						GtkFileFilter *filter )
{
	celluloid_file_dialog_set_default_filter(self->dialog, filter);
}
