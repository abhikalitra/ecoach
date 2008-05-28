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

#ifndef _HEART_RATE_SETTINGS_H
#define _HEART_RATE_SETTINGS_H

/*****************************************************************************
 * Includes                                                                  *
 *****************************************************************************/

/* Configuration */
#include "config.h"

/* GLib */
#include <glib/gi18n.h>

/* Gtk */
#include <gtk/gtk.h>

/* Other modules */
#include "gconf_helper.h"

/*****************************************************************************
 * Enumerations                                                              *
 *****************************************************************************/

typedef enum _EcExerciseTypes {
	EC_EXERCISE_TYPE_AEROBIC = 0,
	EC_EXERCISE_TYPE_ANAEROBIC,
	EC_EXERCISE_TYPE_WEIGHT_CONTROL,
	EC_EXERCISE_TYPE_MODERATE_ACTIVITY,
	EC_EXERCISE_TYPE_COUNT
} EcExerciseTypes;

/*****************************************************************************
 * Data structures                                                           *
 *****************************************************************************/

/**
 * @brief This object holds information of heart rate ranges for different
 * activity levels.
 * 
 * @todo Rename to something more logical, as this only describes a heart rate
 * range, not an exercise
 */
typedef struct _EcExerciseDescription {
	/** @brief Name of the activity level (e.g., "Aerobic") */
	gchar *name;

	/** @brief Longer explanation of the activity level */
	gchar *description;

	/** @brief GConf path for the activity class */
	gchar *gconf_key;

	/** @brief Default lower heart rate intensity limit in percents */
	gdouble low_percentage;

	/** @brief Default upper heart rate intensity limit in percents */
	gdouble high_percentage;

	/** @brief Calculated or user-set lower heart rate limit */
	gint low;

	/** @brief Back-up value of lower heart rate limit */
	gint low_copy;

	/** @brief Calculated or user-set upper heart rate limit */
	gint high;

	/** @brief Back-up value of upper heart rate limit */
	gint high_copy;

	/**
	 * @brief Whether the limits were calculated or user-set
	 *
	 * @todo Currently this only means whether the value was in GConf
	 * or not. This could be fixed by clearing this when calling
	 * heart_rate_settings_recalculate() and setting again when
	 * the value is modified more directly (needs a new function).
	 */
	gboolean user_set;
} EcExerciseDescription;

typedef struct _HeartRateSettings {
	GConfHelperData *gconf_helper;
	EcExerciseDescription *exercise_descriptions;

	/* Resting heart rate */
	gint hr_rest;
	gint hr_rest_copy;

	/* Maximum heart rate */
	gint hr_max;
	gint hr_max_copy;
} HeartRateSettings;

/*****************************************************************************
 * Function prototypes                                                       *
 *****************************************************************************/

/**
 * @biref Create a new HeartRateSettings object
 *
 * @param gcon_helper Pointer to #GConfHelperData
 * 
 * @return Newly allocated #HeartRateSettings struct
 */
HeartRateSettings *heart_rate_settings_new(GConfHelperData *gconf_helper);

/**
 * @brief Recalculate heart rate limits for a specific exercise type
 *
 * @param self Pointer to HeartRateSettings
 * @param exercise_description Exercise description for which to recalculate
 * the heart rate limits
 */
void heart_rate_settings_recalculate(HeartRateSettings *self,
		EcExerciseDescription *exercise_description);

/**
 * @brief Save the settings in GConf
 *
 * @param self Pointer to #HeartRateSettings
 */
void heart_rate_settings_save(HeartRateSettings *self);

/**
 * @brief Revert the settings
 *
 * @param self Pointer to #HeartRateSettings
 */
void heart_rate_settings_revert(HeartRateSettings *self);

#endif /* _HEART_RATE_SETTINGS_H */
