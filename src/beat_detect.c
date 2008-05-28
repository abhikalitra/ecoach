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

/* This modules */
#include "beat_detect.h"

/* System */
#if (BEAT_DETECTOR_SIMULATE_HEARTBEAT)
#include <stdlib.h>
#endif

/* OSEA */
#include "osea/bdac.h"
#include "osea/ecgcodes.h"

/* Other modules */
#include "util.h"

#include "debug.h"

/*****************************************************************************
 * Private function prototypes                                               *
 *****************************************************************************/

static gboolean _beat_detector_initialized = FALSE;

static void beat_detector_reset(BeatDetector *self);

/**
 * @brief Analyze ECG data arriving from #EcgData
 *
 * @param ecg_data Pointer to #EcgData
 * @param samples Samples that have arrived
 * @param len Length of data
 * @param user_data Pointer to #BeatDetector
 */
static void beat_detector_analyze(
		EcgData *ecg_data,
		guint8 *samples,
		guint len,
		gpointer user_data);

/**
 * @brief Calculate mean heart rate and invoke the callbacks.
 *
 * @param self Pointer to #BeatDetector
 * @param beat_interval Beat interval (in samples)
 * @param beat_type (as defined by OSEA library)
 */
static void beat_detector_invoke_callbacks(
		BeatDetector *self,
		gint beat_interval,
		gint beat_type);

#if (BEAT_DETECTOR_SIMULATE_HEARTBEAT)
static void beat_detector_start_simulating_heartbeat(BeatDetector *self);
static void beat_detector_stop_simulating_heartbeat(BeatDetector *self);
static gboolean beat_detector_simulated_heartbeat(gpointer user_data);
#endif

/*****************************************************************************
 * Function declarations                                                     *
 *****************************************************************************/

/*===========================================================================*
 * Public functions                                                          *
 *===========================================================================*/

BeatDetector *beat_detector_new(EcgData *ecg_data)
{
	BeatDetector *self = NULL;

	g_return_val_if_fail(ecg_data != NULL, NULL);

	DEBUG_BEGIN();

	if(_beat_detector_initialized)
	{
		g_critical("Only one beat detector can be created at the\n"
				"time because of library constraints");
		return NULL;
	}

	self = g_new0(BeatDetector, 1);
	if(!self)
	{
		g_critical("Not enough memory");
		return NULL;
	}

	self->ecg_data = ecg_data;

	beat_detector_set_beat_interval_mean_count(self, 20);

	_beat_detector_initialized = TRUE;

	self->beat_found = FALSE;
	self->previous_beat_distance = 0;

	/* Reset the beat detector and classifier */
	ResetBDAC();

	DEBUG_END();
	return self;
}

void beat_detector_set_beat_interval_mean_count(
		BeatDetector *self,
		guint count)
{
	gint i;
	g_return_if_fail(self != NULL);
	g_return_if_fail(count > 0);

	DEBUG_BEGIN();

	self->beat_interval = g_new(gint, count);
	self->beat_interval_count = count;

	for(i = 0; i < count; i++)
	{
		self->beat_interval[i] = -1;
	}

	DEBUG_END();
}

gboolean beat_detector_add_callback(
		BeatDetector *self,
		BeatDetectorFunc callback,
		gpointer user_data,
		GError **error)
{
	BeatDetectorCallbackData *cb_data = NULL;

	g_return_val_if_fail(self != NULL, FALSE);
	g_return_val_if_fail(callback != NULL, FALSE);
	g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

	DEBUG_BEGIN();

	if(self->callbacks == NULL)
	{
		DEBUG_LONG("First callback added. Connecting to EcgData");
#if (BEAT_DETECTOR_SIMULATE_HEARTBEAT)
		beat_detector_start_simulating_heartbeat(self);
#else
		if(!ecg_data_add_callback_ecg(
					self->ecg_data,
					beat_detector_analyze,
					self,
					error))
		{
			g_assert(error == NULL || *error != NULL);
			return FALSE;
		}
#endif
	}

	cb_data = g_new0(BeatDetectorCallbackData, 1);
	cb_data->callback = callback;
	cb_data->user_data = user_data;

	self->callbacks = g_slist_append(self->callbacks, cb_data);

	DEBUG_END();
	return TRUE;
}

void beat_detector_remove_callback(
		BeatDetector *self,
		BeatDetectorFunc callback,
		gpointer user_data)
{
	GSList *temp = NULL;
	GSList *indices = NULL;
	GSList *to_remove = NULL;

	gboolean match = TRUE;
	BeatDetectorCallbackData *cb_data = NULL;
	gint index = 0;

	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	for(temp = self->callbacks; temp; temp = g_slist_next(temp))
	{
		match = TRUE;
		cb_data = (BeatDetectorCallbackData *)temp->data;
		if(callback)
		{
			if(callback != cb_data->callback)
			{
				match = FALSE;
			}
		}
		if(user_data)
		{
			if(user_data != cb_data->user_data)
			{
				match = FALSE;
			}
		}
		if(match)
		{
			/* We can't just remove the item, because
			 * the list may be relocated, which would break the
			 * looping. Instead, append the index to another
			 * list */
			indices = g_slist_append(indices,
					GINT_TO_POINTER(index));
			
		}
		index++;
	}

	/* Reverse the list of indices, as we must go from end to begin
	 * to avoid shifting in the list */
	indices = g_slist_reverse(indices);

	for(temp = indices; temp; temp = g_slist_next(temp))
	{
		index = GPOINTER_TO_INT(temp->data);
		DEBUG_LONG("Removing callback %d", index);
		to_remove = g_slist_nth(self->callbacks, index);
		cb_data = (BeatDetectorCallbackData *)to_remove->data;
		g_free(cb_data);
		self->callbacks = g_slist_delete_link(
				self->callbacks,
				to_remove);
	}

	if(self->callbacks == NULL)
	{
		DEBUG_LONG("Last callback removed. Removing callback from"
				"EcgData");

#if (BEAT_DETECTOR_SIMULATE_HEARTBEAT)
		beat_detector_stop_simulating_heartbeat(self);
#else
		ecg_data_remove_callback_ecg(
				self->ecg_data,
				beat_detector_analyze,
				self);

		/* Reset the beat detector, as there will be a gap in the
		 * data, or it might come even from a different person */
		beat_detector_reset(self);
#endif
	}

	DEBUG_END();
}

void beat_detector_destroy(BeatDetector *self)
{
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	g_free(self);
	_beat_detector_initialized = FALSE;
	DEBUG_END();
}

/*===========================================================================*
 * Private functions                                                         *
 *===========================================================================*/

static void beat_detector_reset(BeatDetector *self)
{
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	self->parameters_configured = FALSE;
	self->previous_beat_distance = 0;
	self->beat_found = FALSE;
	ResetBDAC();

	DEBUG_END();
}

static void beat_detector_analyze(
		EcgData *ecg_data,
		guint8 *samples,
		guint len,
		gpointer user_data)
{
	gint converted_value = 0;
	guint i = 0;
	gint delay = 0;
	gint beat_type = 0;
	gint beat_match = 0;

	gint beat_interval = 0;

	BeatDetector *self = (BeatDetector *)user_data;

	g_return_if_fail(self != NULL);
	g_return_if_fail(samples != NULL);

	DEBUG_BEGIN();

	if(!self->parameters_configured)
	{
		self->sample_rate = ecg_data_get_sample_rate(self->ecg_data);
		self->units_per_mv = ecg_data_get_units_per_mv(self->ecg_data);
		self->zero_level = ecg_data_get_zero_level(self->ecg_data);
		self->parameters_configured = TRUE;
	}

	for(i = 0; i < len; i++)
	{
		/* Osea requires voltage baseline to be at zero, and resolution
		 * 5 uV per least significant type */
		converted_value = samples[i];
		converted_value = converted_value - self->zero_level;
		converted_value = converted_value * 200 / self->units_per_mv;	

		/* Distance to previous beat grows by one */
		self->previous_beat_distance++;

		if(self->beat_found) {
			self->sample_count_since_offset_time++;
		}

		delay = BeatDetectAndClassify(
				converted_value,
				&beat_type,
				&beat_match);

		if(delay != 0)
		{
			if(self->beat_found)
			{
				beat_interval = self->previous_beat_distance
					- delay;
				DEBUG("Beat interval: %d", beat_interval);
				DEBUG("In secs: %f",
						(gdouble)beat_interval /
						self->sample_rate);
				beat_detector_invoke_callbacks(
						self,
						beat_interval,
						beat_type);
			} else {
				DEBUG("First beat at: %d",
						self->previous_beat_distance);
				self->beat_found = TRUE;
				gettimeofday(&self->offset_time, NULL);
				beat_detector_invoke_callbacks(
						self,
						-1,
						beat_type);
			}
			self->previous_beat_distance = delay;

			if(beat_type == NORMAL)
			{
				DEBUG("Detected a NORMAL beat");
			} else if (beat_type == PVC) {
				DEBUG("Detected a PVC beat");
			} else {
				DEBUG("Detected UNKNOWN beat: %d",
						beat_type);
			}
			DEBUG("Delay was: %d", delay);
		}
	}

	DEBUG_END();
}

static void beat_detector_invoke_callbacks(
		BeatDetector *self,
		gint beat_interval,
		gint beat_type)
{
	gint i;
	GSList *temp = NULL;
	BeatDetectorCallbackData *cb_data = NULL;
	guint total_interval = 0;
	guint total_interval_count = 0;
	gdouble heart_rate = -1;
	struct timeval time_since_offset_time;
	struct timeval beat_time;

	DEBUG_BEGIN();

	if(self->sample_rate <= 0)
	{
		g_warning("Invalid sample rate %d defined", self->sample_rate);
		DEBUG_END();
		return;
	}

	if(beat_interval > 0)
	{
		/* Push the new beat interval to the array */
		for(i = 0; i < self->beat_interval_count - 1; i++)
		{
			self->beat_interval[i] = self->beat_interval[i + 1];
		}
		self->beat_interval[self->beat_interval_count - 1] =
			beat_interval;

		/* Calculate the total interval of beats */
		for(i = 0; i < self->beat_interval_count; i++)
		{
			if(self->beat_interval[i] > 0)
			{
				total_interval += self->beat_interval[i];
				total_interval_count++;
			}
		}

		if(total_interval_count > 0)
		{
			heart_rate = 60.0 * (gdouble)total_interval_count /
				((gdouble)total_interval /
				 (gdouble)self->sample_rate);
		} else {
			heart_rate = -1;
		}
	}

	/* Calculate the time (as whole seconds) since the offset_time */
	time_since_offset_time.tv_sec = self->sample_count_since_offset_time /
		self->sample_rate;

	/* Now, calculate the microseconds since the offset time */
	time_since_offset_time.tv_usec = (self->sample_count_since_offset_time %
		self->sample_rate) * 1000000 / self->sample_rate;

	/* The time of the beat if the offset time + time_since_offset_time */
	util_add_time(&self->offset_time, &time_since_offset_time, &beat_time);

	/* Finally, add the whole seconds to the offset time, and
	 * reduce the samples since offset time */
	self->offset_time.tv_sec += time_since_offset_time.tv_sec;
	self->sample_count_since_offset_time -= time_since_offset_time.tv_sec
		* self->sample_rate;

	for(temp = self->callbacks; temp; temp = g_slist_next(temp))
	{
		cb_data = (BeatDetectorCallbackData *)temp->data;
		if(cb_data->callback)
		{
			cb_data->callback(
					self,
					heart_rate,
					&beat_time,
					beat_type,
					cb_data->user_data);
		}
	}

	DEBUG_END();
}

#if (BEAT_DETECTOR_SIMULATE_HEARTBEAT)
static void beat_detector_start_simulating_heartbeat(BeatDetector *self)
{
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	if(!self->parameters_configured)
	{
		self->sample_rate = 300;
		self->parameters_configured = TRUE;
	}

	self->simulate_timeout_id = g_timeout_add(
			1000,
			beat_detector_simulated_heartbeat,
			self);
	DEBUG_END();
}

static void beat_detector_stop_simulating_heartbeat(BeatDetector *self)
{
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	g_source_remove(self->simulate_timeout_id);
	self->simulate_timeout_id = 0;

	DEBUG_END();
}

static gboolean beat_detector_simulated_heartbeat(gpointer user_data)
{
	static guint millisecs = 0;
	gint sample_interval;
	BeatDetector *self = (BeatDetector *)user_data;

	g_return_val_if_fail(self != NULL, FALSE);
	DEBUG_BEGIN();

	if(!self->simulate_timeout_id)
	{
		return FALSE;
	}

	/* Simulate the heartbeat */
	if(millisecs)
	{
		sample_interval = millisecs * self->sample_rate / 1000;
		self->sample_count_since_offset_time += sample_interval;
		beat_detector_invoke_callbacks(self, sample_interval, NORMAL);
	} else {
		gettimeofday(&self->offset_time, NULL);
		beat_detector_invoke_callbacks(self, -1, NORMAL);
	}

	/* Add a new timeout after a variable delay */
	g_source_remove(self->simulate_timeout_id);
	millisecs = 500 + 500 * (rand() / (RAND_MAX + 1.0));
	self->simulate_timeout_id = g_timeout_add(
			millisecs,
			beat_detector_simulated_heartbeat,
			self);

	DEBUG_END();
	return TRUE;
}
#endif
