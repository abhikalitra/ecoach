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

/**
 * @file hrm_shared.h
 *
 * @brief Data shared between hrm module (Heart Rate Monitor)
 */

#ifndef _HRM_SHARED_H
#define _HRM_SHARED_H

/* Configuration */
#include "config.h"

/* GLib */
#include <dbus/dbus-glib.h>

/* Custom modules */
#include "gconf_helper.h"

typedef struct _HRMSettingsPriv HRMSettingsPriv;

typedef struct _HRMData {
	/**
	 * @brief Bluetooth address of the selected heart rate monitor device.
	 *
	 * Bluetooth address of the paired and selected heart rate monitor,
	 * in XX:XX:...:XX format. If no device is selected, then this is NULL.
	 */
	gchar *bluetooth_address;

	/**
	 * @brief Friendly Bluetooth name of the selected device
	 */
	gchar *bluetooth_name;

	/** @brief GIOChannel for receiving data from rfcomm tty device */
	GIOChannel *io_channel;

	/** @brief DBus proxy for connection management */
	DBusGProxy *proxy_manager;

	/** @brief DBus proxy for Bluetooth serial port (rfcomm) management */
	DBusGProxy *proxy_serial;

	HRMSettingsPriv *settings_priv;
} HRMData;

/**
 * @brief Initialize Heart Rate Monitor
 *
 * @return Newly allocated HRMData
 */
HRMData *hrm_initialize(GConfHelperData *gconf_helper);

#endif
