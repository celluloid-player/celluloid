/*
 * Copyright (c) 2021 gnome-mpv
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

enum
{
	PROP_0,
	PROP_TITLE,
	PROP_ACTION,
	N_PROPERTIES
};

struct _CelluloidFileChooserButton
{
	GtkButton parent;
	GtkWidget *dialog;
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
set_property(	GObject *object,
		guint property_id,
		const GValue *value,
		GParamSpec *pspec );

static void
get_property(	GObject *object,
		guint property_id,
		GValue *value,
		GParamSpec *pspec );

static void
finalize(GObject *object);

static void
clicked(GtkButton *button);

static void
set_file(CelluloidFileChooserButton *self, GFile *file);

static void
handle_response(GtkDialog *dialog, gint response_id, gpointer data);

G_DEFINE_TYPE(CelluloidFileChooserButton, celluloid_file_chooser_button, GTK_TYPE_BUTTON)

static void
set_property(	GObject *object,
		guint property_id,
		const GValue *value,
		GParamSpec *pspec )
{
	CelluloidFileChooserButton *self = CELLULOID_FILE_CHOOSER_BUTTON(object);

	switch(property_id)
	{
		case PROP_TITLE:
		g_clear_pointer(&self->title, g_free);
		self->title = g_value_dup_string(value);
		break;

		case PROP_ACTION:
		self->action = g_value_get_enum(value);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
get_property(	GObject *object,
		guint property_id,
		GValue *value,
		GParamSpec *pspec )
{
	CelluloidFileChooserButton *self = CELLULOID_FILE_CHOOSER_BUTTON(object);

	switch(property_id)
	{
		case PROP_TITLE:
		g_value_set_string(value, self->title);
		break;

		case PROP_ACTION:
		g_value_set_enum(value, self->action);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
finalize(GObject *object)
{
	CelluloidFileChooserButton *button = CELLULOID_FILE_CHOOSER_BUTTON(object);

	gtk_window_destroy(GTK_WINDOW(button->dialog));
	g_clear_object(&button->file);
	g_clear_object(&button->title);

	G_OBJECT_CLASS(celluloid_file_chooser_button_parent_class)
		->finalize(object);
}

static void
clicked(GtkButton *button)
{
	CelluloidFileChooserButton *self = CELLULOID_FILE_CHOOSER_BUTTON(button);
	GtkWindow *window = GTK_WINDOW(gtk_widget_get_root(GTK_WIDGET(self)));

	gtk_window_set_transient_for(GTK_WINDOW(self->dialog), window);
	gtk_widget_show(self->dialog);
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
handle_response(GtkDialog *dialog, gint response_id, gpointer data)
{
	CelluloidFileChooserButton *self = CELLULOID_FILE_CHOOSER_BUTTON(data);
	GFile *file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog));

	if(response_id == GTK_RESPONSE_ACCEPT)
	{
		set_file(self, file);
		g_object_unref(file);
	}

	gtk_window_close(GTK_WINDOW(dialog));
}

static void
celluloid_file_chooser_button_class_init(CelluloidFileChooserButtonClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	GtkButtonClass *button_class = GTK_BUTTON_CLASS(klass);

	object_class->set_property = set_property;
	object_class->get_property = get_property;
	object_class->finalize = finalize;
	button_class->clicked = clicked;

	GParamSpec *pspec = NULL;

	pspec = g_param_spec_string
		(	"title",
			"Title",
			"The title of the file chooser dialog",
			"",
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_TITLE, pspec);

	pspec = g_param_spec_enum
		(	"action",
			"Action",
			"Mode for the file chooser dialog",
			GTK_TYPE_FILE_CHOOSER_ACTION,
			GTK_FILE_CHOOSER_ACTION_OPEN,
			G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_ACTION, pspec);

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
	self->dialog =
		gtk_file_chooser_dialog_new
		(	_("Open File…"),
			NULL,
			GTK_FILE_CHOOSER_ACTION_OPEN,
			_("Open"), GTK_RESPONSE_ACCEPT,
			_("Cancel"), GTK_RESPONSE_CANCEL,
			NULL );
	self->label = gtk_label_new(_("(None)"));
	self->file = NULL;
	self->title = NULL;
	self->action = GTK_FILE_CHOOSER_ACTION_OPEN;

	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	GtkWidget *icon = gtk_image_new_from_icon_name("document-open-symbolic");

	gtk_window_set_modal
		(GTK_WINDOW(self->dialog), TRUE);
	gtk_window_set_hide_on_close
		(GTK_WINDOW(self->dialog), TRUE);
	gtk_dialog_set_default_response
		(GTK_DIALOG(self->dialog), GTK_RESPONSE_ACCEPT);

	gtk_widget_set_halign(self->label, GTK_ALIGN_START);
	gtk_widget_set_halign(icon, GTK_ALIGN_END);
	gtk_widget_set_hexpand(icon, TRUE);

	gtk_box_append(GTK_BOX(box), self->label);
	gtk_box_append(GTK_BOX(box), icon);
	gtk_button_set_child(GTK_BUTTON(self), box);

	g_object_bind_property(	self, "title",
				self->dialog, "title",
				G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE );
	g_object_bind_property(	self, "action",
				self->dialog, "action",
				G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE );

	g_signal_connect(	self->dialog,
				"response",
				G_CALLBACK(handle_response),
				self );
}

GtkWidget *
celluloid_file_chooser_button_new(	const gchar *title,
					GtkFileChooserAction action )
{
	return GTK_WIDGET(g_object_new(	celluloid_file_chooser_button_get_type(),
					"title", title,
					"action", action,
					NULL ));
}

void
celluloid_file_chooser_button_set_file(	CelluloidFileChooserButton *self,
					GFile *file,
					GError **error )
{

	GtkFileChooser *chooser = GTK_FILE_CHOOSER(self->dialog);

	set_file(self, file);
	gtk_file_chooser_set_file(chooser, file, error);
}

GFile *
celluloid_file_chooser_button_get_file(CelluloidFileChooserButton *self)
{
	GtkFileChooser *chooser = GTK_FILE_CHOOSER(self->dialog);

	return gtk_file_chooser_get_file(chooser);
}

void
celluloid_file_chooser_button_set_filter(	CelluloidFileChooserButton *self,
						GtkFileFilter *filter )
{
	GtkFileChooser *chooser = GTK_FILE_CHOOSER(self->dialog);

	gtk_file_chooser_set_filter(chooser, filter);
}
