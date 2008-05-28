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
#ifndef _GPX_PARSER_H
#define _GPX_PARSER_H

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
#include "gpx.h"	/* Some datatypes are shared with GpxStorage */

/*****************************************************************************
 * Enumerations                                                              *
 *****************************************************************************/

enum _GpxParserDataType {
	GPX_PARSER_DATA_TYPE_TRACK,
	GPX_PARSER_DATA_TYPE_ROUTE,
	GPX_PARSER_DATA_TYPE_HEART_RATE,
	GPX_PARSER_DATA_TYPE_TRACK_SEGMENT,
	GPX_PARSER_DATA_TYPE_WAYPOINT,
	GPX_PARSER_DATA_TYPE_HEART_RATE
};

enum _GpxParserStatus {
	GPX_PARSER_STATUS_OK,
	GPX_PARSER_STATUS_PARTIALLY_OK,
	GPX_PARSER_STATUS_FAILED
};

/*****************************************************************************
 * Type definitions                                                          *
 *****************************************************************************/

typedef struct _GpxParserDataRouteTrack GpxParserDataTrack;
typedef struct _GpxParserDataRouteTrack GpxParserDataRoute;
typedef struct _GpxStorageWaypoint GpxParserDataWaypoint;
typedef struct _GpxParserDataTrackSegment GpxParserDataTrackSegment;
typedef struct _GpxParserDataHeartRate GpxParserDataHeartRate;

typedef union _GpxParserData GpxParserData;

typedef enum _GpxParserDataType GpxParserDataType;
typedef enum _GpxParserStatus GpxParserStatus;

/**
 * @brief Type definition for gpx data parsing callback
 *
 * @param data_type The type of data that will be in data (which is a union)
 * @param data The actual, parsed data
 * @param user_data Optional user data
 *
 * @note The data parameter will be valid until the callback returns, after
 * that all memory used by it is freed.
 *
 * @note Some or even all of the fileds may be NULL, depending on whether
 * or not they were defined in the file that was parsed.
 */
typedef void (*GpxParserCallback)
	(GpxParserDataType data_type,
	 const GpxParserData *data,
	 gpointer user_data);

/*****************************************************************************
 * Unions                                                                    *
 *****************************************************************************/

union _GpxParserData {
	GpxParserDataTrack *track;
	GpxParserDataRoute *route;
	GpxParserDataTrackSegment *track_segment;
	GpxParserDataWaypoint *waypoint;
	GpxParserDataHeartRate *heart_rate;
};

/*****************************************************************************
 * Data structures                                                           *
 *****************************************************************************/

struct _GpxParserDataRouteTrack {
	gchar *name;
	gchar *comment;
	guint number;
};

struct _GpxParserDataTrackSegment {
	/* No actual data at the moment. Provide this for completeness. */	
};

struct _GpxParserDataHeartRate {
	struct timeval timestamp;
	gint value;
};

/*****************************************************************************
 * Function prototypes                                                       *
 *****************************************************************************/

/**
 * @brief Parse a gpx file
 *
 * @param file_name Name of the file to load from
 * @param callback Callback to be called during parsing
 * @param user_data Optional user data to be passed to the callback
 * @param error Storage location for possible error
 *
 * @return Status of the parsing
 */
GpxParserStatus gpx_parser_parse_file(
		const gchar *file_name,
		GpxParserCallback callback,
		gpointer user_data,
		GError **error);

#endif /* _GPX_PARSER_H */
