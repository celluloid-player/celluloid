/*
 * Copyright (c) 2016-2019 gnome-mpv
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

#ifndef APPLICATION_H
#define APPLICATION_H

#include <gtk/gtk.h>

#include "gmpv_main_window.h"
#include "gmpv_mpv.h"

G_BEGIN_DECLS

#define GMPV_TYPE_APPLICATION (gmpv_application_get_type())

G_DECLARE_FINAL_TYPE(GmpvApplication, gmpv_application, GMPV, APPLICATION, GtkApplication)

GmpvApplication *gmpv_application_new(gchar *id, GApplicationFlags flags);
void gmpv_application_quit(GmpvApplication *app);
const gchar *gmpv_application_get_mpv_options(GmpvApplication *app);

G_END_DECLS

#endif
