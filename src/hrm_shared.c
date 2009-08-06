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

/* This module */
#include "hrm_shared.h"

/* Custom modules */
#include "debug.h"
#include "gconf_keys.h"

static void hrm_bluetooth_address_changed(
		const GConfEntry *entry,
		gpointer user_data,
		gpointer user_data_2);

static void hrm_bluetooth_name_changed(
		const GConfEntry *entry,
		gpointer user_data,
		gpointer user_data_2);

HRMData *hrm_initialize(GConfHelperData *gconf_helper)
{
	HRMData *hrm_data = NULL;

	g_return_val_if_fail(gconf_helper != NULL, NULL);
	DEBUG_BEGIN();

	hrm_data = g_new0(HRMData, 1);

	if(!hrm_data)
	{
		g_critical("Not enough memory");
		return NULL;
	}

	gconf_helper_add_key_string(
			gconf_helper,
			ECGC_BLUETOOTH_ADDRESS,
			"",
			hrm_bluetooth_address_changed,
			hrm_data,
			NULL);

	gconf_helper_add_key_string(
			gconf_helper,
			ECGC_BLUETOOTH_NAME,
			"none",
			hrm_bluetooth_name_changed,
			hrm_data,
			NULL);
			
	/** @todo Load the bluetooth address from gconf */

	/* Other variables are initialized when they are needed */
	return hrm_data;
}

static void hrm_bluetooth_address_changed(
		const GConfEntry *entry,
		gpointer user_data,
		gpointer user_data_2)
{
	GConfValue *value = NULL;
	HRMData *hrm_data = (HRMData *)user_data;
	const gchar *bt_address = NULL;

	g_return_if_fail(hrm_data != NULL);
	g_return_if_fail(entry != NULL);
	DEBUG_BEGIN();

	value = gconf_entry_get_value(entry);
	bt_address = gconf_value_get_string(value);

	if(hrm_data->bluetooth_address)
	{
		g_free(hrm_data->bluetooth_address);
	}
	hrm_data->bluetooth_address = g_strdup(bt_address);

	DEBUG_END();
}

static void hrm_bluetooth_name_changed(
		const GConfEntry *entry,
		gpointer user_data,
		gpointer user_data_2)
{
	GConfValue *value = NULL;
	HRMData *hrm_data = (HRMData *)user_data;
	const gchar *bt_name = NULL;

	g_return_if_fail(hrm_data != NULL);
	g_return_if_fail(entry != NULL);
	DEBUG_BEGIN();

	value = gconf_entry_get_value(entry);
	bt_name = gconf_value_get_string(value);

	if(hrm_data->bluetooth_name)
	{
		g_free(hrm_data->bluetooth_name);
	}
	hrm_data->bluetooth_name = g_strdup(bt_name);

	DEBUG_END();
}
