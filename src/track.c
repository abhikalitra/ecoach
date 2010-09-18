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
#include "track.h"

/* System */
#include <string.h>

/* Location */
#include "location-distance-utils-fix.h"

/* Other modules */
#include "ec_error.h"
#include "util.h"
#include "xml_util.h"

#include "debug.h"

/*****************************************************************************
 * Definitions                                                               *
 *****************************************************************************/

#define TRACK_HELPER_AUTOSAVE_INTERVAL 5 * 60 * 1000

/*****************************************************************************
 * Private function prototypes                                               *
 *****************************************************************************/

/**
 * @brief This is a function to autosave data. This function needs to be
 * called every time when data has changed.
 *
 * @param self Pointer to #TrackHelper
 */
static void track_helper_data_changed(TrackHelper *self);

/**
 * @brief This function does the autosave after the time out
 *
 * @param user_data Pointer to #TrackHelper
 */
static gboolean track_helper_autosave(gpointer user_data);

/*****************************************************************************
 * Function declarations for TrackHelperPoint                                *
 *****************************************************************************/

TrackHelperPoint *track_helper_point_new()
{
	TrackHelperPoint *point = NULL;

	DEBUG_BEGIN();

	point = g_new0(TrackHelperPoint, 1);

	DEBUG_END();
	return point;
}

void track_helper_point_free(TrackHelperPoint *point)
{
	DEBUG_BEGIN();

	g_free(point);

	DEBUG_END();
}

/*****************************************************************************
 * Function declarations for TrackHelper                                    *
 *****************************************************************************/

/*===========================================================================*
 * Public functions                                                          *
 *===========================================================================*/

TrackHelper *track_helper_new()
{
	TrackHelper *self = NULL;

	DEBUG_BEGIN();

	self = g_new0(TrackHelper, 1);

	self->gpx_storage = gpx_storage_new();

	self->state = TRACK_HELPER_STOPPED;

	DEBUG_END();
	return self;
}

void track_helper_setup_track(TrackHelper *self,
		const gchar *name,
		const gchar *comment)
{
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	g_free(self->track_name);
	//g_free(self->track_comment);

	self->track_name = g_strdup(name);
	self->track_comment = g_strdup(comment);

	DEBUG_END();
}
void track_helper_set_comment(
		TrackHelper *self,
		const gchar *comment){
	g_return_if_fail(self != NULL);
		DEBUG_BEGIN();
		g_free(self->track_comment);
		self->track_comment = g_strdup(comment);

		DEBUG_END();
}
void track_helper_set_file_name(
		TrackHelper *self,
		const gchar *file_name)
{
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	g_free(self->file_name);
	self->file_name = g_strdup(file_name);
	gpx_storage_set_path(self->gpx_storage, file_name);

	DEBUG_END();
}

void track_helper_add_track_point(
		TrackHelper *self,
		const TrackHelperPoint *point)
{
	TrackHelperPoint *point_copy = NULL;
	TrackHelperPoint *prev_point = NULL;
	GpxStorageWaypoint wp;

	g_return_if_fail(self != NULL);
	g_return_if_fail(point != NULL);
	DEBUG_BEGIN();

	/* Create a copy of the point */
	point_copy = track_helper_point_new();
	point_copy->latitude		= point->latitude;
	point_copy->longitude		= point->longitude;
	point_copy->altitude_is_set	= point->altitude_is_set;
	point_copy->altitude		= point->altitude;
	memcpy(&point_copy->timestamp, &point->timestamp,
			sizeof(struct timeval));

	/* Add the point to the list. We still can operate on it after this */
	self->track_points = g_slist_prepend(self->track_points,
			point_copy);

	switch(self->state)
	{
		case TRACK_HELPER_STOPPED:
			DEBUG("TRACK START");
			wp.point_type = GPX_STORAGE_POINT_TYPE_TRACK_START;
			break;
		case TRACK_HELPER_PAUSED:
			DEBUG("SEGMENT START");
			wp.point_type =
				GPX_STORAGE_POINT_TYPE_TRACK_SEGMENT_START;
			break;
		case TRACK_HELPER_STARTED:
			wp.point_type = GPX_STORAGE_POINT_TYPE_TRACK;
			break;
	}

	wp.route_track_id = self->current_track_id;
	wp.latitude = point_copy->latitude;
	wp.longitude = point_copy->longitude;
	wp.altitude_is_set = point_copy->altitude_is_set;
	wp.altitude = point_copy->altitude;
	memcpy(&wp.timestamp, &point_copy->timestamp, sizeof(struct timeval));

	gpx_storage_add_waypoint(self->gpx_storage,
			&wp);

	if(wp.point_type == GPX_STORAGE_POINT_TYPE_TRACK_START)
	{
		self->current_track_id = wp.route_track_id;
		gpx_storage_set_path(self->gpx_storage, self->file_name);
		gpx_storage_set_route_or_track_details(
				self->gpx_storage,
				TRUE,
				self->current_track_id,
				self->track_name,
				self->track_comment);
	}

	if(self->state == TRACK_HELPER_STOPPED ||
			self->state == TRACK_HELPER_PAUSED)
	{
		self->state = TRACK_HELPER_STARTED;
		/* No previous point. Distance and elapsed time cannot
		 * be calculated. */
		point_copy->distance_to_prev = -1;
		point_copy->time_to_prev.tv_sec = 0;
		point_copy->time_to_prev.tv_usec = 0;
		DEBUG_END();
		return;
	}

	track_helper_data_changed(self);

	/* Get the previous point from the list */
	prev_point = (TrackHelperPoint *)g_slist_nth(
			self->track_points, 1)->data;

	/* Calculate distance to previous point */
	point_copy->distance_to_prev = location_distance_between(
			point_copy->latitude,
			point_copy->longitude,
			prev_point->latitude,
			prev_point->longitude
			) * 1000.0;

	/* Calculate time to previous point */
	util_subtract_time(&point_copy->timestamp,
			&prev_point->timestamp,
			&point_copy->time_to_prev);

	util_add_time(&self->elapsed_time, &point_copy->time_to_prev,
			&self->elapsed_time);

	self->travelled_distance += point_copy->distance_to_prev;

	DEBUG_END();
}

void track_helper_add_heart_rate(
		TrackHelper *self,
		struct timeval *time,
		gint heart_rate)
{
	GpxStoragePointType point_type;

	g_return_if_fail(self != NULL);
	g_return_if_fail(time != NULL);
	DEBUG_BEGIN();

	switch(self->state)
	{
		case TRACK_HELPER_STOPPED:
			point_type = GPX_STORAGE_POINT_TYPE_TRACK_START;
			break;
		case TRACK_HELPER_PAUSED:
			point_type =
				GPX_STORAGE_POINT_TYPE_TRACK_SEGMENT_START;
			break;
		case TRACK_HELPER_STARTED:
			point_type = GPX_STORAGE_POINT_TYPE_TRACK;
			break;
		default:
			g_warning("Unknown track helper state: %d",
					self->state);
			DEBUG_END();
			return;
	}

	if(self->state == TRACK_HELPER_STOPPED ||
			self->state == TRACK_HELPER_PAUSED)
	{
		self->state = TRACK_HELPER_STARTED;
	}

	gpx_storage_add_heart_rate(
			self->gpx_storage,
			point_type,
			&self->current_track_id,
			time,
			heart_rate);

	if(point_type == GPX_STORAGE_POINT_TYPE_TRACK_START)
	{
		gpx_storage_set_path(self->gpx_storage, self->file_name);
		gpx_storage_set_route_or_track_details(
				self->gpx_storage,
				TRUE,
				self->current_track_id,
				self->track_name,
				self->track_comment);
	}

	track_helper_data_changed(self);

	DEBUG_END();
}

void track_helper_pause(TrackHelper *self)
{
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	self->state = TRACK_HELPER_PAUSED;

	DEBUG_END();
}

void track_helper_stop(TrackHelper *self)
{
	GError *error = NULL;
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	self->state = TRACK_HELPER_STOPPED;
	if(!gpx_storage_write(self->gpx_storage, &error))
	{
		ec_error_show_message_error(error->message);
		g_error_free(error);
		error = NULL;
	}
}

void track_helper_clear(TrackHelper *self, gboolean remove_tracks)
{
	GSList *temp = NULL;
	TrackHelperPoint *point;

	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	if(self->state != TRACK_HELPER_STOPPED)
	{
		track_helper_stop(self);
	}

	/* Clear all the current track points and reset statistics */
	for(temp = self->track_points; temp; temp = g_slist_next(temp))
	{
		point = (TrackHelperPoint *)temp->data;
		track_helper_point_free(point);
	}
	g_slist_free(self->track_points);
	self->track_points = NULL;

	self->travelled_distance = 0;
	self->elapsed_time.tv_sec = 0;
	self->elapsed_time.tv_usec = 0;

	g_source_remove(self->autosave_timer_id);
	self->autosave_timer_id = 0;

	if(remove_tracks)
	{
		gpx_storage_free(self->gpx_storage);
		self->gpx_storage = gpx_storage_new();
	}

	DEBUG_END();
}

gdouble track_helper_get_travelled_distance(TrackHelper *self)
{
	g_return_val_if_fail(self != NULL, 0);
	DEBUG_BEGIN();

	DEBUG_END();
	return self->travelled_distance;
}

guint track_helper_get_elapsed_time(TrackHelper *self)
{
	guint elapsed_msec = 0;

	g_return_val_if_fail(self != NULL, 0);
	DEBUG_BEGIN();

	DEBUG_END();
	elapsed_msec = self->elapsed_time.tv_sec * 1000;
	elapsed_msec += self->elapsed_time.tv_usec / 1000;

	DEBUG_END();
	return elapsed_msec;
}

gdouble track_helper_get_current_speed(TrackHelper *self)
{
	gdouble distance_sum = 0;
	struct timeval time_sum;
	gdouble elapsed_secs;
	gint i = 0;
	GSList *temp = NULL;
	TrackHelperPoint *point = NULL;

	time_sum.tv_sec = 0;
	time_sum.tv_usec = 0;

	g_return_val_if_fail(self != NULL, -1);
	DEBUG_BEGIN();

	if(self->state != TRACK_HELPER_STARTED)
	{
		DEBUG("Activity is not in started state");
		return -1;
	}

	if(g_slist_length(self->track_points) < 6)
	{
		/* Less than five points, return the average speed of all
		 * points so far */
		return track_helper_get_average_speed(self);
	}

	temp = self->track_points;
	for(i = 0; i < 5; i++)
	{
		point = (TrackHelperPoint *)temp->data;
		if(point->distance_to_prev == -1)
		{
			DEBUG_END();
			return -1;
		}
		distance_sum += point->distance_to_prev;
		util_add_time(&time_sum, &point->time_to_prev, &time_sum);
		temp = g_slist_next(temp);
	}

	elapsed_secs = (gdouble)time_sum.tv_sec +
		(gdouble)time_sum.tv_usec / 1000000.0;

	if(elapsed_secs == 0)
	{
		DEBUG_END();
		return -1;
	}

	DEBUG_END();
	/* Calculate the speed and convert to km/h */
	return distance_sum / elapsed_secs * 3.6;
}

gdouble track_helper_get_average_speed(TrackHelper *self)
{
	gdouble elapsed_secs;

	DEBUG_BEGIN();

	elapsed_secs = (gdouble)self->elapsed_time.tv_sec +
		(gdouble)self->elapsed_time.tv_usec / 1000000.0;
	if(elapsed_secs <= 0)
	{
		return -1;
	}

	/* Calculate the average speed and convert to km/h */
	DEBUG_END();
	return self->travelled_distance / elapsed_secs * 3.6;
}

/*===========================================================================*
 * Private functions                                                         *
 *===========================================================================*/

static void track_helper_data_changed(TrackHelper *self)
{
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	if(self->autosave_timer_id != 0)
	{
		/* Time is already active */
		DEBUG_END();
		return;
	}

	self->autosave_timer_id = g_timeout_add(
			TRACK_HELPER_AUTOSAVE_INTERVAL,
			track_helper_autosave,
			self);

	DEBUG_END();
}

static gboolean track_helper_autosave(gpointer user_data)
{
	GError *error = NULL;
	TrackHelper *self = (TrackHelper *)user_data;

	g_return_val_if_fail(self != NULL, FALSE);
	DEBUG_BEGIN();

	if(!gpx_storage_write(self->gpx_storage, &error))
	{
		ec_error_show_message_error_printf(
				"Unable to autosave track data:\n%s",
				error->message);
		g_error_free(error);
		DEBUG_END();
		return FALSE;
	}

	self->autosave_timer_id = 0;

	DEBUG_END();
	/* Always return FALSE, because autosave will be activated again
	 * if data changes again */
	return FALSE;
}
