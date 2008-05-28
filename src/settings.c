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

/*****************************************************************************
 * Includes                                                                  *
 *****************************************************************************/

/* This module */
#include "settings.h"

/* Other modules */
#include "gconf_keys.h"

#include "debug.h"

/*****************************************************************************
 * Data structures                                                           *
 *****************************************************************************/

struct _Settings
{
	GConfHelperData *gconf_helper;
	gboolean ignore_time_zones;
};

/*****************************************************************************
 * Private function prototypes                                               *
 *****************************************************************************/

static void settings_ignore_time_zones_changed(
		const GConfEntry *entry,
		gpointer user_data,
		gpointer user_data_2);

/*****************************************************************************
 * Function declarations                                                     *
 *****************************************************************************/

/*===========================================================================*
 * Public functions                                                          *
 *===========================================================================*/

Settings *settings_initialize(GConfHelperData *gconf_helper)
{
	Settings *self = NULL;

	g_return_val_if_fail(gconf_helper != NULL, NULL);
	DEBUG_BEGIN();

	self = g_new0(Settings, 1);
	self->gconf_helper = gconf_helper;

	gconf_helper_add_key_bool(
			self->gconf_helper,
			ECGC_IGNORE_TIME_ZONES,
			TRUE,
			settings_ignore_time_zones_changed,
			self,
			NULL);

	DEBUG_END();
	return self;
}

gboolean settings_get_ignore_time_zones(Settings *self)
{
	g_return_val_if_fail(self != NULL, FALSE);
	return self->ignore_time_zones;
}

void settings_set_ignore_time_zones(Settings *self, gboolean ignore_time_zones)
{
	g_return_if_fail(self != NULL);
	gconf_helper_set_value_bool(
			self->gconf_helper,
			ECGC_IGNORE_TIME_ZONES,
			ignore_time_zones);
}

/*===========================================================================*
 * Private functions                                                         *
 *===========================================================================*/

static void settings_ignore_time_zones_changed(
		const GConfEntry *entry,
		gpointer user_data,
		gpointer user_data_2)
{
	GConfValue *value = NULL;
	Settings *self = (Settings *)user_data;

	g_return_if_fail(self != NULL);
	g_return_if_fail(entry != NULL);
	DEBUG_BEGIN();

	value = gconf_entry_get_value(entry);
	self->ignore_time_zones = gconf_value_get_bool(value);

	DEBUG_END();
}
