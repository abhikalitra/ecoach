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
 * @file gconf_helper.h
 *
 * @brief GConf wrapper functions
 *
 * Programming GConf can be quite painful, because of following reasons:
 *
 * <ol>
 * 	<li>GConf does not guarantee type stability. The value type may change
 * 	at any time in the GConf database.</li>
 * 	<li>GConf uses the same callback for every configuration item. This
 * 	means that when one configuration key is changed, lots of
 * 	string comparisons need to be done manually</li>
 * 	<li>GConf does not check if value actually has changed. This may
 * 	easily lead to infinite loops.</li>
 * </ol>
 *
 * This wrapper tries to overcome these limitations. Programmer only needs
 * to give the key - value type - default value - callback - userdata pair.
 * This information will stay grouped all time. If GConf changes the value
 * type, the value will be reset to what it was previously. Also, specific
 * key change will always invoke the given callback. And of course, the
 * callback will only be called if the value was actually changed.
 * 
 * @todo Support removal of gconf keys, too
 */

#ifndef _GCONF_HELPER_H
#define _GCONF_HELPER_H

/* Configuration */
#include "config.h"

/* GConf */
#include <gconf/gconf.h>

typedef struct _GConfHelperData GConfHelperData;

typedef void (*GConfHelperFunc)
	(const GConfEntry *entry,
	 gpointer user_data,
	 gpointer user_data_2);

typedef struct _GConfHelperItem {
	GConfEntry *entry;
	GConfHelperFunc callback;
	gpointer user_data;
	gpointer user_data_2;
} GConfHelperItem;

GConfHelperData *gconf_helper_new(const gchar *base_dir);

/**
 * @brief Add a gconf key.
 *
 * If the key is not found in gconf, the default value will be set.
 * The callback is called when the key has changed, and during the
 * initialization of the key.
 *
 * @param self Pointer to #GConfHelperData
 * @param key GConf key
 * @param dfl_value Default value for the entry
 * @param func Called when the value of the key changes, and during
 * addition of the key
 * @param user_data User data to be passed to the callback
 * @param user_data Optional user data to be passed to the callback
 */
void gconf_helper_add_key_string(
		GConfHelperData *self,
		const gchar *key,
		const gchar *dfl_value,
		GConfHelperFunc func,
		gpointer user_data,
		gpointer user_data_2);

void gconf_helper_add_key_int(
		GConfHelperData *self,
		const gchar *key,
		gint dfl_value,
		GConfHelperFunc func,
		gpointer user_data,
		gpointer user_data_2);

void gconf_helper_add_key_float(
		GConfHelperData *self,
		const gchar *key,
		gdouble dfl_value,
		GConfHelperFunc func,
		gpointer user_data,
		gpointer user_data_2);

void gconf_helper_add_key_bool(
		GConfHelperData *self,
		const gchar *key,
		gboolean dfl_value,
		GConfHelperFunc func,
		gpointer user_data,
		gpointer user_data_2);

void gconf_helper_set_value_string(
		GConfHelperData *self,
		const gchar *key,
		const gchar *value);

void gconf_helper_set_value_int(
		GConfHelperData *self,
		const gchar *key,
		gint value);

void gconf_helper_set_value_float(
		GConfHelperData *self,
		const gchar *key,
		gdouble value);

void gconf_helper_set_value_bool(
		GConfHelperData *self,
		const gchar *key,
		gboolean value);

gboolean gconf_helper_get_value(
		GConfHelperData *self,
		const gchar *key,
		GConfValueType type,
		GConfValue **value);

/**
 * @brief Get a string value from gconf.
 *
 * This function gets a string value from gconf. If the value is not set,
 * or is of wrong type, it will be set to %default_value.
 *
 * @param self Pointer to #GConfHelperData
 * @param key GConf key of the value to get
 * @param default_value Default value
 *
 * @return The value from GConf, or NULL in case of error. The return value
 * must be freed with g_free()
 */
gchar *gconf_helper_get_value_string_with_default(
		GConfHelperData *self,
		const gchar *key,
		const gchar *default_value);

gboolean gconf_helper_get_value_bool_with_default(
		GConfHelperData *self,
		const gchar *key,
		gboolean default_value);

gint gconf_helper_get_value_int_with_default(
		GConfHelperData *self,
		const gchar *key,
		gint default_value);

gdouble gconf_helper_get_value_float_with_default(
		GConfHelperData *self,
		const gchar *key,
		gdouble default_value);

/**
 * @brief Set a value without having a callback for it set
 *
 * @param self Pointer to #GConfHelperData
 * @param key GConf key to set
 * @param value Value for the item
 */
void gconf_helper_set_value_string_simple(
		GConfHelperData *self,
		const gchar *key,
		const gchar *value);

void gconf_helper_set_value_bool_simple(
		GConfHelperData *self,
		const gchar *key,
		gboolean value);

void gconf_helper_set_value_int_simple(
		GConfHelperData *self,
		const gchar *key,
		gint value);

void gconf_helper_set_value_float_simple(
		GConfHelperData *self,
		const gchar *key,
		gdouble value);

#endif /* _GCONF_HELPER_H */
