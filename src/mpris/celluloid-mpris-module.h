/*
 * Copyright (c) 2017-2019 gnome-mpv
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

#ifndef MPRIS_MODULE_H
#define MPRIS_MODULE_H

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define CELLULOID_TYPE_MPRIS_MODULE celluloid_mpris_module_get_type()
G_DECLARE_DERIVABLE_TYPE(CelluloidMprisModule, celluloid_mpris_module, CELLULOID, MPRIS_MODULE, GObject)

struct _CelluloidMprisModuleClass
{
	GObjectClass parent_interface;
	void (*register_interface)(CelluloidMprisModule *module);
	void (*unregister_interface)(CelluloidMprisModule *module);
};

void
celluloid_mpris_module_connect_signal(	CelluloidMprisModule *module,
					gpointer instance,
					const gchar *detailed_signal,
					GCallback handler,
					gpointer data );

#define celluloid_mpris_module_set_properties(module, ...)\
	celluloid_mpris_module_set_properties_full(module, TRUE, __VA_ARGS__)

void
celluloid_mpris_module_get_properties(CelluloidMprisModule *module, ...);

void
celluloid_mpris_module_set_properties_full(	CelluloidMprisModule *module,
						gboolean send_new_value,
						... );
void
celluloid_mpris_module_register(CelluloidMprisModule *module);

void
celluloid_mpris_module_unregister(CelluloidMprisModule *module);

G_END_DECLS

#endif
