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

/* System */
#include <stdlib.h>

/* Gtk */
#include <gtk/gtk.h>

/* Custom modules */
#include "dbus_helper.h"
#include "interface.h"

gint main(gint argc, gchar **argv)
{
	AppData *app_data = NULL;

	g_thread_init(NULL);

	gtk_init(&argc, &argv);

	app_data = interface_create();

	if(!app_data)
	{
		g_critical("Interface creation failed");
		exit(1);
	}

	if(!dbus_helper_initialize_dbus(app_data))
	{
		g_critical("DBus initialization failed");
		exit(1);
	}

	gtk_main();

	return 0;
}
