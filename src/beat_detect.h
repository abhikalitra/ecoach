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
#ifndef _BEAT_DETECT_H
#define _BEAT_DETECT_H

/*****************************************************************************
 * Includes                                                                  *
 *****************************************************************************/

/* Configuration */
#include "config.h"

/* System */
#include <sys/time.h>
#include <time.h>

/* GLib */
#include <glib.h>

/* Other modules */
#include "ecg_data.h"

/*****************************************************************************
 * Definitions                                                               *
 *****************************************************************************/

/**
 * @def BEAT_DETECTOR_SIMULATE_HEARTBEAT
 *
 * @brief Whether to simulate heart beat
 *
 * If defined to 1, heart beat will be simulated. If defined to 0, heart
 * beat will be taken from a heart rate monitor.
 *
 * Normally this should be defined to 0, and defined as 1 only for testing
 * and debugging purposes.
 */
#define BEAT_DETECTOR_SIMULATE_HEARTBEAT 0

/*****************************************************************************
 * Type definitions                                                          *
 *****************************************************************************/

typedef struct _BeatDetector BeatDetector;

/**
 * @brief Type definition for beat detector callback
 *
 * @param self Pointer to #BeatDetector
 * @param heart_rate Heart rate (beats per minute); may be -1 if the
 * heart rate cannot be yet calculated
 * @param time Time when the beat occurred
 * @param user_data User data pointer to be passed to the callback
 * @param beat_type Type of the heart beat, as defined by OSEA library
 *
 * @note Do not store the pointer of the time parameter, as it will be
 * invalid almost immediately after all callbacks have returned.
 *
 * @note The beats may be detected sometimes in bursts (because
 * ECG data is received in bursts). Also, there is some delay because
 * of the buffering in the Ecg monitor, and because of the delay in the
 * beat detector. This causes also the time parameter to be slightly
 * off, but the intervals derived from consequent time values will
 * be correct.
 */
typedef void (*BeatDetectorFunc)
	(BeatDetector *self,
	 gdouble heart_rate,
	 struct timeval *time,
	 gint beat_type,
	 gpointer user_data);

/*****************************************************************************
 * Data structures                                                           *
 *****************************************************************************/

typedef struct _BeatDetectorCallbackData {
	BeatDetectorFunc callback;
	gpointer user_data;
} BeatDetectorCallbackData;

struct _BeatDetector
{
	/** @brief Pointer to #EcgData */
	EcgData *ecg_data;

	/** @brief List of callbacks */
	GSList *callbacks;

	/** @brief Whether or not the sample rate etc. are configured */
	gboolean parameters_configured;

	/** @brief Sample rate in Hz */
	guint sample_rate;

	/** @brief How many units per mV */
	guint units_per_mv;

	/** @brief Level of zero voltage */
	guint zero_level;

	/** @brief Whether or not a beat has been found */
	gboolean beat_found;

	/**
	 * @brief Distance to previous beat (in samples) from current
	 * position
	 */
	gint previous_beat_distance;

	/**
	 * @brief The beat intervals in samples
	 */
	gint beat_interval_count;
	gint *beat_interval;

	/** @brief The time stamp from which the sample count is calculated */
	struct timeval offset_time;

	/**
	 * @brief Count of samples since the offset_time
	 */
	guint sample_count_since_offset_time;

#if (BEAT_DETECTOR_SIMULATE_HEARTBEAT)
	/**
	 * @brief G source ID for heart beat simulator
	 */
	guint simulate_timeout_id;
#endif
};

/*****************************************************************************
 * Function prototypes                                                       *
 *****************************************************************************/

BeatDetector *beat_detector_new(EcgData *ecg_data);

void beat_detector_destroy(BeatDetector *self);

/**
 * @brief Add a callback that will receive beat intervals.
 *
 * @note A connection to EcgData is automatically established when the
 * first callback is added. EcgData will automatically connect to the
 * ECG monitor when its first callback is added. Therefore, adding a
 * callback might sometimes a considerable amount of time, and might even
 * fail if the connection to the ECG monitor fails.
 * 
 * @param self Pointer to #BeatDetector
 * @param callback Callback to be invoked when a beat is detected
 * @param user_data Optional user data pointer to be passed to the
 * 	callback
 * @param error Return location for possible error
 *
 * @return TRUE on success, FALSE on failure.
 *
 * @note If you set the user_data to be NULL, when you remove that callback
 * with #beat_detector_remove_callback, <em>all callbacks registered to that
 * callback function will be removed</em>. That is because a NULL
 * user_data acts as a wildcard for the #beat_detector_remove_callback.
 */
gboolean beat_detector_add_callback(
		BeatDetector *self,
		BeatDetectorFunc callback,
		gpointer user_data,
		GError **error);

/**
 * @brief Remove a callback
 *
 * If a parameter is NULL, it is considered to be a wildcard.
 * Calling this function with callback and user_data as NULL will remove all
 * callbacks.
 *
 * @note When the last callback is removed, the callback from #EcgData is
 * removed, and it will automatically disconnect from the ECG monitor when
 * its last callback is removed.
 *
 * @param self Pointer to #BeatDetector (must not be NULL)
 * @param callback Callback function
 * @param user_data User data that was passed to the callback
 */
void beat_detector_remove_callback(
		BeatDetector *self,
		BeatDetectorFunc callback,
		gpointer user_data);

/**
 * @brief Set the number of beat intervals used to calculate the mean
 * count
 *
 * @param self Pointer to #BeatDetector
 * @param count Number of beat intervals
 */
void beat_detector_set_beat_interval_mean_count(
		BeatDetector *self,
		guint count);

#endif /* _BEAT_DETECT_H */
