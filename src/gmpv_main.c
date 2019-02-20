/*
 * Copyright (c) 2014-2016 gnome-mpv
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

#include <gio/gio.h>
#include <glib-object.h>
#include <glib.h>

#include "gmpv_application.h"
#include "gmpv_def.h"

int main(int argc, char **argv)
{
	GApplicationFlags flags;
	GmpvApplication *app;
	gint status;

	flags = G_APPLICATION_HANDLES_COMMAND_LINE|G_APPLICATION_HANDLES_OPEN;
	app = gmpv_application_new(APP_ID, flags);
	status = g_application_run(G_APPLICATION(app), argc, argv);

	g_object_unref(app);

	return status;
}
