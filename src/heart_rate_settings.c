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
#include "heart_rate_settings.h"

/* Other modules */
#include "gconf_keys.h"

/* i18n */
#include <glib/gi18n.h>

#include "debug.h"

/*****************************************************************************
 * Static variables                                                          *
 *****************************************************************************/

static EcExerciseDescription ec_exercise_types[EC_EXERCISE_TYPE_COUNT + 1] =
{
	{
		N_("Anaerobic"),
		N_("Anaerobic exercise is a form of hardcore exercise "
				"that is not sustainable for long periods of "
				"time. It is usually done at 80-90% of the "
				"maximum intensity"),
		ECGC_EXERC_ANAEROBIC_DIR,
		.8, .9,
		0, 0,
		FALSE
	}, {
		N_("Aerobic"),
		N_("Aerobic exercise is a form of exercise that is done "
				"at intensity around the maximum sustainable "
				"range - 70-80%. It is used for cardio and "
				"endurance training."),
		ECGC_EXERC_AEROBIC_DIR,
		.7, .8,
		0, 0,
		FALSE
	}, {
		N_("Weight control"),
		N_("Weight control intensity is used for fat burning "
				"and fitness purposes. The recommended "
				"heart rate intensity for weight control "
				"is 60-70 %."),
		ECGC_EXERC_WEIGHT_CTRL_DIR,
		.6, .7,
		0, 0,
		FALSE
	}, {
		N_("Moderate exercise"),
		N_("Moderate activity is beneficial for maintenance "
				"exercise and warm up. It is usually done "
				"at around 50-60 % intensity."),
		ECGC_EXERC_MODERATE_DIR,
		.5, .6,
		0, 0,
		FALSE
	}, {
		NULL,
		NULL,
		NULL,
		0, 0,
		0, 0,
		FALSE
	}
};

/*****************************************************************************
 * Function prototypes                                                       *
 *****************************************************************************/

static void heart_rate_settings_setup_gconf(HeartRateSettings *self);

static void heart_rate_settings_limit_low_changed(
		const GConfEntry *entry,
		gpointer user_data,
		gpointer user_data_2);

static void heart_rate_settings_limit_high_changed(
		const GConfEntry *entry,
		gpointer user_data,
		gpointer user_data_2);

static void heart_rate_settings_hr_rest_changed(
		const GConfEntry *entry,
		gpointer user_data,
		gpointer user_data_2);

static void heart_rate_settings_hr_max_changed(
		const GConfEntry *entry,
		gpointer user_data,
		gpointer user_data_2);

/*****************************************************************************
 * Function declarations                                                     *
 *****************************************************************************/

/*===========================================================================*
 * Public functions                                                          *
 *===========================================================================*/

HeartRateSettings *heart_rate_settings_new(GConfHelperData *gconf_helper)
{
	HeartRateSettings *self = NULL;

	DEBUG_BEGIN();

	self = g_new0(HeartRateSettings, 1);
	self->gconf_helper = gconf_helper;
	self->exercise_descriptions = ec_exercise_types;

	heart_rate_settings_setup_gconf(self);

	DEBUG_END();
	return self;
}

void heart_rate_settings_recalculate(
		HeartRateSettings *self,
		EcExerciseDescription *exercise_description)
{
	g_return_if_fail(self != NULL);
	g_return_if_fail(exercise_description != NULL);
	DEBUG_BEGIN();

	/* Calculate the target heart rates by using the Karvonen method,
	 * where
	 *
	 * THR = ((HR[max] - HR[rest]) * intensity%) + HR[rest]
	 */
	exercise_description->low =
		(gdouble)(self->hr_max - self->hr_rest) *
		exercise_description->low_percentage
		+ (gdouble)self->hr_rest + 0.5;

	exercise_description->high =
		(gdouble)(self->hr_max - self->hr_rest) *
		exercise_description->high_percentage
		+ (gdouble)self->hr_rest + 0.5;

	DEBUG_END();
}

void heart_rate_settings_save(HeartRateSettings *self)
{
	gint i;
	gchar *gconf_key = NULL;
	EcExerciseDescription *desc = NULL;

	DEBUG_BEGIN();
	gconf_helper_set_value_int(
			self->gconf_helper,
			ECGC_HR_REST,
			self->hr_rest);

	gconf_helper_set_value_int(
			self->gconf_helper,
			ECGC_HR_MAX,
			self->hr_max);

	for(i = 0; i < EC_EXERCISE_TYPE_COUNT; i++)
	{
		desc = &self->exercise_descriptions[i];

		gconf_key = g_strdup_printf("%s/%s",
				desc->gconf_key,
				ECGC_HR_LIMIT_LOW);

		gconf_helper_set_value_int(
				self->gconf_helper,
				gconf_key,
				desc->low);

		g_free(gconf_key);

		gconf_key = g_strdup_printf("%s/%s",
				desc->gconf_key,
				ECGC_HR_LIMIT_HIGH);

		gconf_helper_set_value_int(
				self->gconf_helper,
				gconf_key,
				desc->high);
	}

	DEBUG_END();
}

void heart_rate_settings_revert(HeartRateSettings *self)
{
	gint i;

	DEBUG_BEGIN();
	self->hr_rest = self->hr_rest_copy;
	self->hr_max = self->hr_max_copy;
	for(i = 0; i < EC_EXERCISE_TYPE_COUNT; i++)
	{
		self->exercise_descriptions[i].low =
			self->exercise_descriptions[i].low_copy;
		self->exercise_descriptions[i].high =
			self->exercise_descriptions[i].high_copy;
	}

	DEBUG_END();
}

/*===========================================================================*
 * Private functions                                                         *
 *===========================================================================*/

static void heart_rate_settings_setup_gconf(HeartRateSettings *self)
{
	EcExerciseDescription *desc = NULL;
	gchar *gconf_key = NULL;
	gint i;

	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	for(i = 0; i < EC_EXERCISE_TYPE_COUNT; i++)
	{
		desc = &self->exercise_descriptions[i];

		gconf_key = g_strdup_printf("%s/%s",
				desc->gconf_key,
				ECGC_HR_LIMIT_LOW);

		gconf_helper_add_key_int(
				self->gconf_helper,
				gconf_key,
				-1,
				heart_rate_settings_limit_low_changed,
				self,
				desc);

		g_free(gconf_key);

		gconf_key = g_strdup_printf("%s/%s",
				desc->gconf_key,
				ECGC_HR_LIMIT_HIGH);

		gconf_helper_add_key_int(
				self->gconf_helper,
				gconf_key,
				-1,
				heart_rate_settings_limit_high_changed,
				self,
				desc);

		g_free(gconf_key);
	}

	gconf_helper_add_key_int(
			self->gconf_helper,
			ECGC_HR_REST,
			70,
			heart_rate_settings_hr_rest_changed,
			self,
			desc);

	gconf_helper_add_key_int(
			self->gconf_helper,
			ECGC_HR_MAX,
			200,
			heart_rate_settings_hr_max_changed,
			self,
			desc);

	DEBUG_END();
}

static void heart_rate_settings_limit_low_changed(
		const GConfEntry *entry,
		gpointer user_data,
		gpointer user_data_2)
{
	GConfValue *value = NULL;
	HeartRateSettings *self = (HeartRateSettings *)user_data;
	EcExerciseDescription *desc = (EcExerciseDescription *)user_data_2;

	g_return_if_fail(self != NULL);
	g_return_if_fail(desc != NULL);
	g_return_if_fail(entry != NULL);
	DEBUG_BEGIN();

	value = gconf_entry_get_value(entry);
	desc->low = gconf_value_get_int(value);
	desc->low_copy = desc->low;
	if(desc->low != -1)
	{
		desc->user_set = TRUE;
	}

	DEBUG_END();
}

static void heart_rate_settings_limit_high_changed(
		const GConfEntry *entry,
		gpointer user_data,
		gpointer user_data_2)
{
	GConfValue *value = NULL;
	HeartRateSettings *self = (HeartRateSettings *)user_data;
	EcExerciseDescription *desc = (EcExerciseDescription *)user_data_2;

	g_return_if_fail(self != NULL);
	g_return_if_fail(desc != NULL);
	g_return_if_fail(entry != NULL);
	DEBUG_BEGIN();

	value = gconf_entry_get_value(entry);
	desc->high = gconf_value_get_int(value);
	desc->high_copy = desc->high;
	if(desc->high != -1)
	{
		desc->user_set = TRUE;
	}

	DEBUG_END();
}

static void heart_rate_settings_hr_rest_changed(
		const GConfEntry *entry,
		gpointer user_data,
		gpointer user_data_2)
{
	GConfValue *value = NULL;
	HeartRateSettings *self = (HeartRateSettings *)user_data;

	g_return_if_fail(self != NULL);
	g_return_if_fail(entry != NULL);
	DEBUG_BEGIN();

	value = gconf_entry_get_value(entry);
	self->hr_rest = gconf_value_get_int(value);
	self->hr_rest_copy = self->hr_rest;

	DEBUG_END();
}

static void heart_rate_settings_hr_max_changed(
		const GConfEntry *entry,
		gpointer user_data,
		gpointer user_data_2)
{
	GConfValue *value = NULL;
	HeartRateSettings *self = (HeartRateSettings *)user_data;

	g_return_if_fail(self != NULL);
	g_return_if_fail(entry != NULL);
	DEBUG_BEGIN();

	value = gconf_entry_get_value(entry);
	self->hr_max = gconf_value_get_int(value);
	self->hr_max_copy = self->hr_max;

	DEBUG_END();
}
