/*
 * Copyright (c) 2015-2017 gnome-mpv
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

#ifndef MEDIA_KEYS_H
#define MEDIA_KEYS_H

#include "gmpv_controller.h"

G_BEGIN_DECLS

#define GMPV_TYPE_MEDIA_KEYS (gmpv_media_keys_get_type ())

G_DECLARE_FINAL_TYPE(GmpvMediaKeys, gmpv_media_keys, GMPV, MEDIA_KEYS, GObject)

GmpvMediaKeys *gmpv_media_keys_new(GmpvController *controller);

G_END_DECLS

#endif
