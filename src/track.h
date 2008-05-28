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

#ifndef _TRACK_H
#define _TRACK_H

/* Configuration */
#include "config.h"

/* System */
#include <sys/time.h>
#include <time.h>

/* GLib */
#include <glib.h>

/* Other modules */
#include "gpx.h"

typedef enum _TrackHelperState {
	TRACK_HELPER_STOPPED,
	TRACK_HELPER_PAUSED,
	TRACK_HELPER_STARTED
} TrackHelperState;

typedef struct _TrackHelperPoint {
	/** @brief The latitude of the point */
	gdouble latitude;

	/** @brief The longitude of the point */
	gdouble longitude;

	/** @brief Whether or not the altitude is valid */
	gboolean altitude_is_set;

	/** @brief Altitude */
	gdouble altitude;

	/** @brief Time stamp (in Unix time format, i.e., seconds from Epoch) */
	struct timeval timestamp;

	/**
	 * @brief Distance to previous track point in meters,
	 * or -1 if not defined (first point after start or resume).
	 *
	 * @note This will be automatically set by
	 * #track_helper_add_track_point()
	 */
	gdouble distance_to_prev;

	/**
	 * @brief Time elapsed since previous track point
	 * or 0 if not defined (first point after start or resume).
	 *
	 * @note This will be automatically set by
	 * #track_helper_add_track_point()
	 */
	struct timeval time_to_prev;
} TrackHelperPoint;

/**
 * @brief Allocates memory for a new #TrackHelperPoint
 *
 * @return Newly allocated #TrackHelperPoint. Free with
 * track_helper_pointe_free()
 */
TrackHelperPoint *track_helper_point_new();

/**
 * @brief Frees memory used by a #TrackHelperPoint
 *
 * @param point Pointer to #TrackHelperPoint to be freed
 */
void track_helper_point_free(TrackHelperPoint *point);

typedef struct _TrackHelper {
	/**
	 * @brief List of track points
	 *
	 * @note The list points are in reverse order (latest point first),
	 * because prepending to a linked list is faster than appending
	 */
	GSList *track_points;

	/**
	 * @brief Whether or not in started state
	 */
	TrackHelperState state;

	/** @brief The total travelled distance in meters */
	gdouble travelled_distance;

	/** @brief Elapsed time */
	struct timeval elapsed_time;

	/** @brief Pointer to #GpxStorage */
	GpxStorage *gpx_storage;

	/** @brief ID of the current track */
	guint current_track_id;

	/** @brief GLib source ID for the autosave timer */
	guint autosave_timer_id;

	/** @brief Track name */
	gchar *track_name;

	/** @brief Track comment */
	gchar *track_comment;

	/** @brief File name to save the track to */
	gchar *file_name;
} TrackHelper;

/**
 * @brief Creates a new track helper
 */
TrackHelper *track_helper_new();

/**
 * @brief Clears all data from #TrackHelper and sets state to stopped
 *
 * @param self Pointer to #TrackHelper
 * @param remove_tracks Whether or not to also remove all currently
 * saved tracks
 */
void track_helper_clear(TrackHelper *self, gboolean remove_tracks);

/**
 * @brief Sets the name and comment for the track
 *
 * @param self Pointer to #TrackHelper
 * @param name Name of the track
 * @param comment Comment for the track
 *
 * @note The parameters are set only for new tracks, not existing ones!
 */
void track_helper_setup_track(
		TrackHelper *self,
		const gchar *name,
		const gchar *comment);

/**
 * @brief Sets the file name
 *
 * @param self Pointer to #TrackHelper
 * @param file_name File name to set
 */
void track_helper_set_file_name(
		TrackHelper *self,
		const gchar *file_name);

/**
 * @brief Add a point to track
 *
 * @param self Pointer to #TrackHelper
 * @param point #TrackHelperPoint to be added to track (a copy of the
 * point is created, so a reference to a temporary object may be passed)
 *
 * Adding a point starts or resumes (on pause) the track helper
 */
void track_helper_add_track_point(
		TrackHelper *self,
		const TrackHelperPoint *point);

/**
 * @brief Add a detected heart rate to a track. The heart rate can be added
 * to a "track" even when GPS is not in use.
 *
 * @param self Pointer to #TrackHelper
 * @param time Time when the heart rate was added
 * @param heart_rate Heart rate (in beats per minute)
 */
void track_helper_add_heart_rate(
		TrackHelper *self,
		struct timeval *time,
		gint heart_rate);

/**
 * @brief Add a pause to track
 *
 * @param self Pointer to #TrackHelper
 */
void track_helper_pause(TrackHelper *self);

/**
 * @brief Stop the track recording
 *
 * @param self Pointer to #TrackHelper
 */
void track_helper_stop(TrackHelper *self);

/**
 * @brief Get the current travelled distance (in meters)
 *
 * @param self Pointer to #TrackHelper
 *
 * @return The current travelled distance (in meters)
 */
gdouble track_helper_get_travelled_distance(TrackHelper *self);

/**
 * @brief Get the current elapsed time (in milliseconds)
 *
 * @param self Pointer to #TrackHelper
 *
 * @return The current elapsed time (in milliseconds)
 */
guint track_helper_get_elapsed_time(TrackHelper *self);

/**
 * @brief Get current speed (in km/h)
 *
 * @param self Pointer to #TrackHelper
 *
 * @return Current speed, or -1 if the speed cannot be calculated
 */
gdouble track_helper_get_current_speed(TrackHelper *self);

/**
 * @brief Get the average speed
 *
 * @param self Pointer to #TrackHelper
 *
 * @return Average speed, or -1 if the speed cannot be calculated
 */
gdouble track_helper_get_average_speed(TrackHelper *self);

/**
 * @brief Get the total distance
 *
 * @param self Pointer to #TrackHelper
 *
 * @return Travelled distance in meters
 */
gdouble track_helper_get_travelled_distance(TrackHelper *self);

/**
 * @todo Functions to calculate distances and average speeds between given
 * route points
 *
 * @todo Functions to get route points by distance or elapsed time
 */

/**
 * @brief Returns a GList of track points
 *
 * @note The list items are in reverse order, because prepending is much
 * more effective with linked lists than appending
 *
 * @warning Do not modify or free the list or its elements.
 */
const GSList *track_helper_get_track_points(TrackHelper *self);

#endif /* _TRACK_H */
