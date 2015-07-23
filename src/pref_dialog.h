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

#ifndef PREF_DIALOG_H
#define PREF_DIALOG_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PREF_DIALOG_TYPE (pref_dialog_get_type ())

#define	PREF_DIALOG(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), PREF_DIALOG_TYPE, PrefDialog))

#define	PREF_DIALOG_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass), PREF_DIALOG_TYPE, PrefDialogClass))

#define	IS_PREF_DIALOG(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), PREF_DIALOG_TYPE))

#define	IS_PREF_DIALOG_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), PREF_DIALOG_TYPE))

typedef struct PrefDialog PrefDialog;
typedef struct PrefDialogClass PrefDialogClass;

struct PrefDialog
{
	GtkDialog dialog;
	GtkWidget *grid;
	GtkWidget *content_area;
	GtkWidget *dark_theme_enable_check;
	GtkWidget *csd_enable_check;
	GtkWidget *mpvinput_enable_check;
	GtkWidget *mpvinput_button;
	GtkWidget *mpvconf_enable_check;
	GtkWidget *mpvconf_button;
	GtkWidget *mpvopt_entry;
	GtkWidget *headerbar;
	GtkWidget *cancel_button;
	GtkWidget *save_button;
};

struct PrefDialogClass
{
	GtkDialogClass parent_class;
};

GtkWidget *pref_dialog_new(GtkWindow *parent);
GType pref_dialog_get_type(void);
void pref_dialog_set_dark_theme_enable(PrefDialog *dlg, gboolean value);
gboolean pref_dialog_get_dark_theme_enable(PrefDialog *dlg);
void pref_dialog_set_csd_enable(PrefDialog *dlg, gboolean value);
gboolean pref_dialog_get_csd_enable(PrefDialog *dlg);
void pref_dialog_set_mpvconf_enable(PrefDialog *dlg, gboolean value);
gboolean pref_dialog_get_mpvconf_enable(PrefDialog *dlg);
void pref_dialog_set_mpvconf(PrefDialog *dlg, const gchar *buffer);
gchar *pref_dialog_get_mpvconf(PrefDialog *dlg);
void pref_dialog_set_mpvinput_enable(PrefDialog *dlg, gboolean value);
gboolean pref_dialog_get_mpvinput_enable(PrefDialog *dlg);
void pref_dialog_set_mpvinput(PrefDialog *dlg, const gchar *buffer);
gchar *pref_dialog_get_mpvinput(PrefDialog *dlg);
void pref_dialog_set_mpvopt(PrefDialog *dlg, gchar *buffer);
const gchar *pref_dialog_get_mpvopt(PrefDialog *dlg);

G_END_DECLS

#endif
