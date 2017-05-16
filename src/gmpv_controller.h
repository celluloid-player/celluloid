/*
 * Copyright (c) 2017 gnome-mpv
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

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <glib.h>

#include "gmpv_model.h"
#include "gmpv_view.h"

G_BEGIN_DECLS

#define GMPV_TYPE_CONTROLLER (gmpv_controller_get_type())

G_DECLARE_FINAL_TYPE(GmpvController, gmpv_controller, GMPV, CONTROLLER, GObject)

GmpvController *gmpv_controller_new(GmpvModel *model, GmpvView *view);
void gmpv_controller_quit(GmpvController *controller);
void gmpv_controller_autofit(GmpvController *controller, gdouble multiplier);
void gmpv_controller_present(GmpvController *controller);
void gmpv_controller_open(	GmpvController *controller,
				const gchar *urii,
				gboolean append );

G_END_DECLS

#endif
