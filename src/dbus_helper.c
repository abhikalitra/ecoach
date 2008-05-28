/*
 *  eCoach
 *
 *  Copyright (C) 2008  Jukka Alasalmi
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  See the file COPYING
 */

/* DBus */
#include <dbus/dbus-glib.h>

/* This module */
#include "dbus_helper.h"

gboolean dbus_helper_initialize_dbus(AppData *app_data)
{
	GError *error = NULL;

	dbus_g_thread_init();

	g_return_val_if_fail(app_data != NULL, FALSE);

	app_data->dbus_system = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);

	if(!app_data->dbus_system)
	{
		g_message("Failed to get DBus system bus: %s\n",
				error->message);
		g_error_free(error);
		return FALSE;
	}

	return TRUE;
}
