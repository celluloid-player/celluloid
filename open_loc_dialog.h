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

#ifndef OPEN_LOC_DIALOG_H
#define OPEN_LOC_DIALOG_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define OPEN_LOC_DIALOG_TYPE (open_loc_dialog_get_type ())

#define	OPEN_LOC_DIALOG(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), OPEN_LOC_DIALOG_TYPE, OpenLocDialog))

#define	OPEN_LOC_DIALOG_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST \
		((klass), OPEN_LOC_DIALOG_TYPE, OpenLocDialogClass))

#define	IS_OPEN_LOC_DIALOG(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), OPEN_LOC_DIALOG_TYPE))

#define	IS_OPEN_LOC_DIALOG_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), OPEN_LOC_DIALOG_TYPE))

typedef struct OpenLocDialog OpenLocDialog;
typedef struct OpenLocDialogClass OpenLocDialogClass;

struct OpenLocDialog
{
	GtkDialog dialog;
	GtkWidget *content_area;
	GtkWidget *content_box;
	GtkWidget *loc_label;
	GtkWidget *loc_entry;
};

struct OpenLocDialogClass
{
	GtkDialogClass parent_class;
};

GtkWidget *open_loc_dialog_new(GtkWindow *parent);
GType open_loc_dialog_get_type(void);
const gchar *open_loc_dialog_get_string(OpenLocDialog *dlg);
guint64 open_loc_dialog_get_string_length(OpenLocDialog *dlg);

G_END_DECLS

#endif
