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

#ifndef _HRM_SETTINGS_H
#define _HRM_SETTINGS_H

/* Configuration */
#include "config.h"

/* Gtk */
#include <gtk/gtk.h>

/* Custom modules */
#include "hrm_shared.h"
#include "general_settings.h"
#include "navigation_menu.h"

struct _HRMSettingsPriv {
	GtkWidget *dialog;

	GtkWidget *lbl_current_device;

	GtkWidget *lbl_settings;

	GtkWidget *h_separator;

	GtkWidget *lbl_name;
	GtkWidget *lbl_bt_addr;
	GtkWidget *combo_sample_rate;

	GtkWidget *caption_name;
	GtkWidget *caption_bt_addr;
	GtkWidget *caption_sample_rate;

	GtkSizeGroup *size_group;

	/* These are used for the device discovery */
	DBusGProxy *proxy_con_dialog;

	gchar *bluetooth_name;
	gchar *bluetooth_address;
};

void hrm_settings_show(GeneralSettings *app_data);

#endif /* _HRM_SETTINGS_H */
