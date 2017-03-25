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

#include "gmpv_mpris_module.h"

G_DEFINE_TYPE(GmpvMprisModule, gmpv_mpris_module, G_TYPE_OBJECT)

static void gmpv_mpris_module_class_init(GmpvMprisModuleClass *klass);
static void gmpv_mpris_module_init(GmpvMprisModule *module);

static void gmpv_mpris_module_class_init(GmpvMprisModuleClass *klass)
{
}

static void gmpv_mpris_module_init(GmpvMprisModule *module)
{
}

void gmpv_mpris_module_register_interface(GmpvMprisModule *module)
{
	GmpvMprisModuleClass *klass = GMPV_MPRIS_MODULE_GET_CLASS(module);

	g_return_if_fail(GMPV_IS_MPRIS_MODULE(module));
	g_return_if_fail(klass->register_interface);
	klass->register_interface(module);
}

void gmpv_mpris_module_unregister_interface(GmpvMprisModule *module)
{
	GmpvMprisModuleClass *klass = GMPV_MPRIS_MODULE_GET_CLASS(module);

	g_return_if_fail(GMPV_IS_MPRIS_MODULE(module));
	g_return_if_fail(klass->unregister_interface);
	klass->unregister_interface(module);
}

