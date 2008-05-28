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

#ifndef _GPX_H
#define _GPX_H

/* Configuration */
#include "config.h"

/* System */
#include <sys/time.h>
#include <time.h>

/* GLib */
#include <glib.h>

/* LibXML2 */
#include <libxml/tree.h>

typedef struct _GpxStorage GpxStorage;

typedef enum _GpxStoragePointType {
	/**
	 * @brief Start a track. Implies a track segment start
	 * (often a resume from a pause) */
	GPX_STORAGE_POINT_TYPE_TRACK_START,

	/**
	 * @brief Starts a new track segment (used for resuming from
	 * a pause)
	 */
	GPX_STORAGE_POINT_TYPE_TRACK_SEGMENT_START,

	/** @brief Regular track point */
	GPX_STORAGE_POINT_TYPE_TRACK,

	/** @brief First point of a route */
	GPX_STORAGE_POINT_TYPE_ROUTE_START,

	/** @brief Regular route point */
	GPX_STORAGE_POINT_TYPE_ROUTE
} GpxStoragePointType;

typedef struct _GpxStorageWaypoint {
	GpxStoragePointType point_type;

	/**
	 * @brief ID of the track or route.
	 *
	 * This will be allocated, if the
	 * point_type parameter is GPX_STORAGE_POINT_TYPE_TRACK_START
	 * ort GPX_STORAGE_POINT_TYPE_ROUTE_START, when adding a waypoint
	 * to a track or route.
	 */
	guint route_track_id;
	gdouble latitude;
	gdouble longitude;
	gboolean altitude_is_set;
	gdouble altitude;

	/**
	 * @brief Timestamp of the waypoint.
	 */
	struct timeval timestamp;
} GpxStorageWaypoint;

struct _GpxStorage {
	/** @brief Pointer to the XML document */
	xmlDocPtr xml_document;

	/** @brief Pointer to the root node of the XML document */
	xmlNodePtr root_node;

	/** @brief The XML Schema Instance namespace */
	xmlNsPtr xmlns_xsi;

	/** @brief The custom extensions namespace */
	xmlNsPtr xmlns_gpx_extensions;

	/** @brief Whether or not the data has chagned since last save */
	gboolean has_changed;

	/** @brief The name of current file, or NULL if not any */
	gchar *file_path;

	/** @brief List of track IDs that are in use */
	GSList *track_ids;

	/** @brief List of route IDs that are in use */
	GSList *route_ids;
};

/*****************************************************************************
 * Function prototypes                                                       *
 *****************************************************************************/

/**
 * @brief Create a new, empty #GpxStorage
 */
GpxStorage *gpx_storage_new();

/**
 * @brief Frees memory used by GpxStorage
 *
 * @param self Pointer to #GpxStorage
 *
 * @warning This function does NOT save the document
 */
void gpx_storage_free(GpxStorage *self);

/**
 * @brief Adds a waypoint to a track or a route
 *
 * @param self Pointer to #GpxStorage
 * @param waypoint Waypoint to add
 *
 * @note Points can only be added to the end of the current track or route.
 */
void gpx_storage_add_waypoint(
		GpxStorage *self,
		GpxStorageWaypoint *waypoint);

/**
 * @todo A cool extension would be to define heart beat for given
 * waypoints in a route so that given sections on a predefined route
 * could have different target heart rates.
 */

/**
 * @brief Adds a heart rate to the given track
 *
 * @param self pointer to #GpxStorage
 * @param point_type Point type determines whether the point is added
 * to a new track, new track segment, or to the latest track segment
 * in an existing track
 * @param track_id ID of the track to add the heart beat to; if %point_type
 * is either GPX_STORAGE_POINT_TYPE_TRACK_START or
 * GPX_STORAGE_POINT_TYPE_TRACK_SEGMENT_START, then the ID of the created
 * track will be stored into this parameter
 * @param time Time when the heart rate was detected
 * @param heart_rate The heart rate to be added (in beats per minute)
 */
void gpx_storage_add_heart_rate(
		GpxStorage *self,
		GpxStoragePointType point_type,
		guint *track_id,
		struct timeval *time,
		gint heart_rate);

/**
 * @brief Setup some details to a route or a track
 *
 * @param self Pointer to #GpxStorage
 * @param is_route Whether setting a track (TRUE) or a route (FALSE)
 * @param name Name of the track
 * @param comment Comment for the track
 */
void gpx_storage_set_route_or_track_details(
		GpxStorage *self,
		gboolean is_track,
		guint route_track_id,
		const gchar *name,
		const gchar *comment);

/**
 * @brief Set a path to save the file to
 *
 * @param self Pointer to #GpxStorage
 * @param path Path to save the file to (including the file name and extension)
 */
void gpx_storage_set_path(
		GpxStorage *self,
		const gchar *path);

/**
 * @brief Write data to a file.
 *
 * The file name is determined by using the function gpx_storage_set_path()
 * function. If the file exists, it will be overwritten.
 *
 * @param self Pointer to #GpxStorage
 * @param error Storage location for possible error
 *
 * @return TRUE on success, FALSE on failure
 */
gboolean gpx_storage_write(
		GpxStorage *self,
		GError **error);

#endif /* _GPX_H */
