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

/* This module header */
#include "gconf_helper.h"

/* System */
#include <string.h>

/* GConf */
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>

/* Custom modules */
#include "debug.h"

struct _GConfHelperData {
	GConfClient *gconf_client;
	GSList *items;
	guint notify_id;
};

static void gconf_helper_create_entry(
		GConfHelperData *self,
		const gchar *key,
		GConfValue *value,
		GConfHelperFunc callback,
		gpointer user_data,
		gpointer user_data_2);

static void gconf_helper_invoke_callback(
		GConfHelperData *self,
		GConfHelperItem *item);

static void gconf_helper_value_changed(
		GConfClient *client,
		guint notify_id,
		GConfEntry *entry,
		gpointer user_data);

static void gconf_helper_check_and_invoke(
		GConfHelperData *self,
		GConfEntry *entry,
		GConfHelperItem *item);

static gboolean gconf_helper_values_are_equal(
		GConfValue *value1,
		GConfValue *value2);

static void gconf_helper_set_value(
		GConfHelperData *self,
		const gchar *key,
		GConfValue *gconf_value);

static void gconf_helper_set_value_for_item(
		GConfHelperData *self,
		GConfHelperItem *item,
		GConfValue *gconf_value);

GConfHelperData *gconf_helper_new(const gchar *base_dir)
{
	GConfHelperData *self = NULL;
	GError *error = NULL;

	DEBUG_BEGIN();

	self = g_new0(GConfHelperData, 1);
	if(!self)
	{
		g_critical("Not enough memory");
		DEBUG_END();
		return NULL;
	}

	self->gconf_client = gconf_client_get_default();
	if(!self->gconf_client)
	{
		g_critical("GConf client initialization failed");
		g_free(self);
		DEBUG_END();
		return NULL;
	}

	gconf_client_add_dir(self->gconf_client,
			base_dir,
			GCONF_CLIENT_PRELOAD_ONELEVEL,
			&error);

	if(error)
	{
		g_warning("Failed to add GConf directory: %s",
				error->message);
		g_error_free(error);
		error = NULL;
	}

	self->notify_id = gconf_client_notify_add(
			self->gconf_client,
			base_dir,
			gconf_helper_value_changed,
			self,
			NULL,
			&error);

	if(error)
	{
		g_warning("Failed to add GConf notification: %s",
				error->message);
		g_error_free(error);
		error = NULL;
	}

	DEBUG_END();
	return self;
}

gboolean gconf_helper_get_value(
		GConfHelperData *self,
		const gchar *key,
		GConfValueType type,
		GConfValue **value)
{
	GError *error = NULL;

	g_return_val_if_fail(self != NULL, FALSE);
	g_return_val_if_fail(key != NULL, FALSE);
	g_return_val_if_fail(value != NULL, FALSE);
	g_return_val_if_fail(*value == NULL, FALSE);

	DEBUG_BEGIN();

	*value = gconf_client_get_without_default(
			self->gconf_client,
			key,
			&error);

	if(error)
	{
		g_warning("Getting value from gconf failed: %s",
				error->message);
		g_error_free(error);
		DEBUG_END();
		return FALSE;
	}

	if(*value == NULL)
	{
		g_message("GConf returned NULL value. The key probably "
				"does not (yet) exist");
		DEBUG_END();
		return FALSE;
	}

	if(type != (*value)->type)
	{
		g_warning("Unexpected GConf value type");
		gconf_value_free(*value);
		*value = NULL;
		DEBUG_END();
		return FALSE;
	}

	DEBUG_END();
	return TRUE;
}

void gconf_helper_add_key_string(
		GConfHelperData *self,
		const gchar *key,
		const gchar *dfl_value,
		GConfHelperFunc callback,
		gpointer user_data,
		gpointer user_data_2)
{
	GConfValue *value = NULL;

	g_return_if_fail(self != NULL);
	g_return_if_fail(key != NULL);
	g_return_if_fail(callback != NULL);

	DEBUG_BEGIN();

	/* First, get the value from GConf */
	if(!gconf_helper_get_value(self, key, GCONF_VALUE_STRING, &value))
	{
		/* Getting value failed. Use default value. */
		value = gconf_value_new(GCONF_VALUE_STRING);
		gconf_value_set_string(value, dfl_value);
		gconf_client_set(self->gconf_client,
				key,
				value,
				NULL);
	}

	gconf_helper_create_entry(
			self,
			key,
			value,
			callback,
			user_data,
			user_data_2);

	DEBUG_END();
}

void gconf_helper_add_key_int(
		GConfHelperData *self,
		const gchar *key,
		gint dfl_value,
		GConfHelperFunc callback,
		gpointer user_data,
		gpointer user_data_2)
{
	GConfValue *value = NULL;

	g_return_if_fail(self != NULL);
	g_return_if_fail(key != NULL);
	g_return_if_fail(callback != NULL);

	DEBUG_BEGIN();

	/* First, get the value from GConf */
	if(!gconf_helper_get_value(self, key, GCONF_VALUE_INT, &value))
	{
		/* Getting value failed. Use default value. */
		value = gconf_value_new(GCONF_VALUE_INT);
		gconf_value_set_int(value, dfl_value);
		gconf_client_set(self->gconf_client,
				key,
				value,
				NULL);
	}

	gconf_helper_create_entry(
			self,
			key,
			value,
			callback,
			user_data,
			user_data_2);

	DEBUG_END();
}

void gconf_helper_add_key_float(
		GConfHelperData *self,
		const gchar *key,
		gdouble dfl_value,
		GConfHelperFunc callback,
		gpointer user_data,
		gpointer user_data_2)
{
	GConfValue *value = NULL;

	g_return_if_fail(self != NULL);
	g_return_if_fail(key != NULL);
	g_return_if_fail(callback != NULL);

	DEBUG_BEGIN();

	/* First, get the value from GConf */
	if(!gconf_helper_get_value(self, key, GCONF_VALUE_FLOAT, &value))
	{
		/* Getting value failed. Use default value. */
		value = gconf_value_new(GCONF_VALUE_FLOAT);
		gconf_value_set_float(value, dfl_value);
		gconf_client_set(self->gconf_client,
				key,
				value,
				NULL);
	}

	gconf_helper_create_entry(
			self,
			key,
			value,
			callback,
			user_data,
			user_data_2);

	DEBUG_END();
}

void gconf_helper_add_key_bool(
		GConfHelperData *self,
		const gchar *key,
		gboolean dfl_value,
		GConfHelperFunc callback,
		gpointer user_data,
		gpointer user_data_2)
{
	GConfValue *value = NULL;

	g_return_if_fail(self != NULL);
	g_return_if_fail(key != NULL);
	g_return_if_fail(callback != NULL);

	DEBUG_BEGIN();

	/* First, get the value from GConf */
	if(!gconf_helper_get_value(self, key, GCONF_VALUE_BOOL, &value))
	{
		/* Getting value failed. Use default value. */
		value = gconf_value_new(GCONF_VALUE_BOOL);
		gconf_value_set_bool(value, dfl_value);
		gconf_client_set(self->gconf_client,
				key,
				value,
				NULL);
	}

	gconf_helper_create_entry(
			self,
			key,
			value,
			callback,
			user_data,
			user_data_2);

	DEBUG_END();
}

void gconf_helper_set_value_string(
		GConfHelperData *self,
		const gchar *key,
		const gchar *value)
{
	GConfValue *gconf_value = NULL;

	g_return_if_fail(self != NULL);
	g_return_if_fail(key != NULL);

	DEBUG_BEGIN();

	gconf_value = gconf_value_new(GCONF_VALUE_STRING);
	gconf_value_set_string(gconf_value, value);

	gconf_helper_set_value(
			self,
			key,
			gconf_value);

	gconf_value_free(gconf_value);
	
	DEBUG_END();
}

void gconf_helper_set_value_int(
		GConfHelperData *self,
		const gchar *key,
		gint value)
{
	GConfValue *gconf_value = NULL;

	g_return_if_fail(self != NULL);
	g_return_if_fail(key != NULL);

	DEBUG_BEGIN();

	gconf_value = gconf_value_new(GCONF_VALUE_INT);
	gconf_value_set_int(gconf_value, value);

	gconf_helper_set_value(
			self,
			key,
			gconf_value);

	gconf_value_free(gconf_value);
	
	DEBUG_END();
}

void gconf_helper_set_value_float(
		GConfHelperData *self,
		const gchar *key,
		gdouble value)
{
	GConfValue *gconf_value = NULL;

	g_return_if_fail(self != NULL);
	g_return_if_fail(key != NULL);

	DEBUG_BEGIN();

	gconf_value = gconf_value_new(GCONF_VALUE_FLOAT);
	gconf_value_set_float(gconf_value, value);

	gconf_helper_set_value(
			self,
			key,
			gconf_value);

	gconf_value_free(gconf_value);
	
	DEBUG_END();
}

void gconf_helper_set_value_bool(
		GConfHelperData *self,
		const gchar *key,
		gboolean value)
{
	GConfValue *gconf_value = NULL;

	g_return_if_fail(self != NULL);
	g_return_if_fail(key != NULL);

	DEBUG_BEGIN();

	gconf_value = gconf_value_new(GCONF_VALUE_BOOL);
	gconf_value_set_bool(gconf_value, value);

	gconf_helper_set_value(
			self,
			key,
			gconf_value);

	gconf_value_free(gconf_value);
	
	DEBUG_END();
}

static void gconf_helper_set_value(
		GConfHelperData *self,
		const gchar *key,
		GConfValue *gconf_value)
{
	GConfHelperItem *item = NULL;
	GSList *tmp_list = NULL;
	const gchar *key2;

	g_return_if_fail(self != NULL);
	g_return_if_fail(key != NULL);
	g_return_if_fail(gconf_value != NULL);

	DEBUG_BEGIN();

	for(tmp_list = self->items; tmp_list; tmp_list = g_slist_next(tmp_list))
	{
		item = (GConfHelperItem *)tmp_list->data;
		key2 = gconf_entry_get_key(item->entry);
		if(strcmp(key, key2) == 0)
		{
			DEBUG_LONG("Found matching GConf entry");
			gconf_helper_set_value_for_item(
					self,
					item,
					gconf_value);
		}
	}

	DEBUG_END();
}

static void gconf_helper_set_value_for_item(
		GConfHelperData *self,
		GConfHelperItem *item,
		GConfValue *gconf_value)
{
	GConfValue *old_gconf_value = NULL;
	g_return_if_fail(self != NULL);
	g_return_if_fail(item != NULL);
	g_return_if_fail(gconf_value != NULL);

	DEBUG_BEGIN();

	old_gconf_value = gconf_entry_get_value(item->entry);

	if(old_gconf_value->type != gconf_value->type)
	{
		g_warning("Unable to set GConf value: wrong type");
		DEBUG_END();
		return;
	}

	gconf_client_set(self->gconf_client,
			gconf_entry_get_key(item->entry),
			gconf_value,
			NULL);

	DEBUG_END();
}

static void gconf_helper_create_entry(
		GConfHelperData *self,
		const gchar *key,
		GConfValue *value,
		GConfHelperFunc callback,
		gpointer user_data,
		gpointer user_data_2)
{
	GConfEntry *entry = NULL;
	GConfHelperItem *item = NULL;

	g_return_if_fail(self != NULL);
	g_return_if_fail(key != NULL);
	g_return_if_fail(value != NULL);

	DEBUG_BEGIN();

	entry = gconf_entry_new(key, value);
	item = g_new0(GConfHelperItem, 1);
	if(!item)
	{
		g_critical("Not enough memory");
		gconf_entry_unref(entry);
		DEBUG_END();
		return;
	}

	item->entry = entry;
	item->callback = callback;
	item->user_data = user_data;
	item->user_data_2 = user_data_2;

	self->items = g_slist_append(self->items, item);

	gconf_helper_invoke_callback(self, item);

	DEBUG_END();
}

static void gconf_helper_invoke_callback(
		GConfHelperData *self,
		GConfHelperItem *item)
{
	g_return_if_fail(self != NULL);
	g_return_if_fail(item != NULL);

	DEBUG_BEGIN();

	if(item->callback)
	{
		item->callback(item->entry, item->user_data, item->user_data_2);
	}

	DEBUG_END();
}

static void gconf_helper_value_changed(
		GConfClient *client,
		guint notify_id,
		GConfEntry *entry,
		gpointer user_data)
{
	GSList *tmp_list;
	GConfHelperItem *item = NULL;
	GConfHelperData *self = (GConfHelperData *)user_data;

	const gchar *key1;
	const gchar *key2;

	g_return_if_fail(self != NULL);
	g_return_if_fail(entry != NULL);

	DEBUG_BEGIN();

	key1 = gconf_entry_get_key(entry);

	for(tmp_list = self->items; tmp_list; tmp_list = g_slist_next(tmp_list))
	{
		item = (GConfHelperItem *)tmp_list->data;
		key2 = gconf_entry_get_key(item->entry);
		if(strcmp(key1, key2) == 0)
		{
			DEBUG_LONG("Found matching GConf entry");
			gconf_helper_check_and_invoke(self, entry, item);
		}
	}

	DEBUG_END();
}

static void gconf_helper_check_and_invoke(
		GConfHelperData *self,
		GConfEntry *entry,
		GConfHelperItem *item)
{
	GConfValue *old_value = NULL;
	GConfValue *new_value = NULL;
	GError *error = NULL;

	g_return_if_fail(self != NULL);
	g_return_if_fail(entry != NULL);
	g_return_if_fail(item != NULL);

	DEBUG_BEGIN();

	old_value = gconf_entry_get_value(item->entry);
	new_value = gconf_entry_get_value(entry);

	if(old_value->type != new_value->type)
	{
		g_message("GConf value type is not compatible. "
				"Resetting value.");
		gconf_client_set(self->gconf_client,
				gconf_entry_get_key(item->entry),
				gconf_entry_get_value(item->entry),
				&error);
		if(error)
		{
			g_warning("Resetting GConf value failed: %s",
					error->message);
			g_error_free(error);
		}
		/* The value was (tried to be) reset. Don't notify about
		 * the change. */
		DEBUG_END();
		return;
	}

	/* Check if the value was actually changed. */
	if(gconf_helper_values_are_equal(old_value, new_value))
	{
		DEBUG_LONG("GConf value was not changed. Ignoring.");
		DEBUG_END();
		return;
	}

	/* Set the new value */
	old_value = gconf_entry_steal_value(item->entry);
	gconf_value_free(old_value);
	gconf_entry_set_value(item->entry, new_value);

	if(item->callback)
	{
		item->callback(item->entry, item->user_data, item->user_data_2);
	}

	DEBUG_END();
}

static gboolean gconf_helper_values_are_equal(
		GConfValue *value1,
		GConfValue *value2)
{
	const gchar *str1;
	const gchar *str2;

	gint int1, int2;
	gdouble dbl1, dbl2;
	gboolean bool1, bool2;

	g_return_val_if_fail(value1 != NULL, FALSE);
	g_return_val_if_fail(value2 != NULL, FALSE);
	g_return_val_if_fail(value1->type == value2->type, FALSE);

	DEBUG_BEGIN();

	switch(value1->type)
	{
		case GCONF_VALUE_STRING:
			str1 = gconf_value_get_string(value1);
			str2 = gconf_value_get_string(value2);
			DEBUG_END();
			return(strcmp(str1, str2) == 0);
		case GCONF_VALUE_INT:
			int1 = gconf_value_get_int(value1);
			int2 = gconf_value_get_int(value2);
			DEBUG_END();
			return(int1 == int2);
		case GCONF_VALUE_FLOAT:
			dbl1 = gconf_value_get_float(value1);
			dbl2 = gconf_value_get_float(value2);
			DEBUG_END();
			return(dbl1 == dbl2);
		case GCONF_VALUE_BOOL:
			bool1 = gconf_value_get_bool(value1);
			bool2 = gconf_value_get_bool(value2);
			DEBUG_END();
			return(bool1 == bool2);
		default:
			g_warning("Unsupport GConf value type");
			DEBUG_END();
			return FALSE;
	}
}

gchar *gconf_helper_get_value_string_with_default(
		GConfHelperData *self,
		const gchar *key,
		const gchar *default_value)
{
	gchar *retval = NULL;
	GConfValue *value = NULL;

	g_return_val_if_fail(self != NULL, NULL);
	g_return_val_if_fail(key != NULL, NULL);
	g_return_val_if_fail(default_value != NULL, NULL);

	DEBUG_BEGIN();

	/* First, get the value from GConf */
	retval = gconf_client_get_string(self->gconf_client,
			key,
			NULL);
	if(!retval)
	{
		/* Getting value failed. Use default value. */
		value = gconf_value_new(GCONF_VALUE_STRING);
		gconf_value_set_string(value, default_value);
		gconf_client_set(self->gconf_client,
				key,
				value,
				NULL);
		retval = g_strdup(default_value);
	}

	DEBUG_END();
	return retval;
}

gboolean gconf_helper_get_value_bool_with_default(
		GConfHelperData *self,
		const gchar *key,
		gboolean default_value)
{
	gboolean retval = FALSE;
	GConfValue *value = NULL;
	GError *error = NULL;

	g_return_val_if_fail(self != NULL, default_value);
	g_return_val_if_fail(key != NULL, default_value);

	DEBUG_BEGIN();

	/* First, get the value from GConf */
	retval = gconf_client_get_bool(self->gconf_client,
			key,
			NULL);
	if(error)
	{
		g_error_free(error);
		/* Getting value failed. Use default value. */
		value = gconf_value_new(GCONF_VALUE_BOOL);
		gconf_value_set_bool(value, default_value);
		gconf_client_set(self->gconf_client,
				key,
				value,
				NULL);
		retval = default_value;
	}

	DEBUG_END();
	return retval;
}

gint gconf_helper_get_value_int_with_default(
		GConfHelperData *self,
		const gchar *key,
		gint default_value)
{
	gint retval = FALSE;
	GConfValue *value = NULL;
	GError *error = NULL;

	g_return_val_if_fail(self != NULL, default_value);
	g_return_val_if_fail(key != NULL, default_value);

	DEBUG_BEGIN();

	/* First, get the value from GConf */
	retval = gconf_client_get_int(self->gconf_client,
			key,
			NULL);
	if(error)
	{
		g_error_free(error);
		/* Getting value failed. Use default value. */
		value = gconf_value_new(GCONF_VALUE_INT);
		gconf_value_set_int(value, default_value);
		gconf_client_set(self->gconf_client,
				key,
				value,
				NULL);
		retval = default_value;
	}

	DEBUG_END();
	return retval;
}

gdouble gconf_helper_get_value_float_with_default(
		GConfHelperData *self,
		const gchar *key,
		gdouble default_value)
{
	gdouble retval = FALSE;
	GConfValue *value = NULL;
	GError *error = NULL;

	g_return_val_if_fail(self != NULL, default_value);
	g_return_val_if_fail(key != NULL, default_value);

	DEBUG_BEGIN();

	/* First, get the value from GConf */
	retval = gconf_client_get_float(self->gconf_client,
			key,
			NULL);
	if(error)
	{
		g_error_free(error);
		/* Getting value failed. Use default value. */
		value = gconf_value_new(GCONF_VALUE_FLOAT);
		gconf_value_set_float(value, default_value);
		gconf_client_set(self->gconf_client,
				key,
				value,
				NULL);
		retval = default_value;
	}

	DEBUG_END();
	return retval;
}

void gconf_helper_set_value_string_simple(
		GConfHelperData *self,
		const gchar *key,
		const gchar *value)
{
	g_return_if_fail(self != NULL);
	g_return_if_fail(key != NULL);
	g_return_if_fail(value != NULL);

	DEBUG_BEGIN();
	gconf_client_set_string(self->gconf_client,
			key,
			value,
			NULL);
	DEBUG_END();
}

void gconf_helper_set_value_bool_simple(
		GConfHelperData *self,
		const gchar *key,
		gboolean value)
{
	g_return_if_fail(self != NULL);
	g_return_if_fail(key != NULL);

	DEBUG_BEGIN();
	gconf_client_set_bool(self->gconf_client,
			key,
			value,
			NULL);
	DEBUG_END();
}

void gconf_helper_set_value_int_simple(
		GConfHelperData *self,
		const gchar *key,
		gint value)
{
	g_return_if_fail(self != NULL);
	g_return_if_fail(key != NULL);

	DEBUG_BEGIN();
	gconf_client_set_int(self->gconf_client,
			key,
			value,
			NULL);
	DEBUG_END();
}

void gconf_helper_set_value_float_simple(
		GConfHelperData *self,
		const gchar *key,
		gdouble value)
{
	g_return_if_fail(self != NULL);
	g_return_if_fail(key != NULL);

	DEBUG_BEGIN();
	gconf_client_set_float(self->gconf_client,
			key,
			value,
			NULL);
	DEBUG_END();
}
