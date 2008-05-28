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
#ifndef _GPX_DEFS_H
#define _GPX_DEFS_H

/* Configuration */
#include "config.h"

#define EC_GPX_XML_VERSION		"1.0"

#define EC_GPX_XML_NAMESPACE		"http://www.topografix.com/GPX/1/1"
#define EC_GPX_XML_SCHEMA_URI		EC_GPX_XML_NAMESPACE "/gpx.xsd"
#define EC_XML_ATTR_SCHEMA_LOCATION_NAME	"schemaLocation"

#define EC_XML_SCHEMA_INSTANCE		"http://www.w3.org/2001/XMLSchema-instance"
#define EC_GPX_SCHEMA_INSTANCE_PREFIX	"xsi"

#define EC_GPX_EXTENSIONS_NAMESPACE	"eCoachGPXExtensionsV1"

/**
 * @todo The placement of the schema is a bit problematic: it should be
 * always available in the same place. Relative path (only file name) is
 * not good, because it means that the schema would need to be copied
 * with the XML file. Absolute path is not good, because it would need
 * to copied from the system to another to the same path, which is not
 * usually possible. The only real solution would be to put it to
 * Internet, but even then, the location should not change, and that would
 * be very difficult, too.
 *
 * Then, on the other hand, the schemaLocation only provides a hint to the
 * parser, and does not mandate that the parser must load and use the
 * specified URI.
 */
#define EC_GPX_EXTENSIONS_SCHEMA_LOCATION	\
	DATADIR PACKAGE_NAME "/ec_gpx_ext_v1.xsd"
#define EC_GPX_EXTENSIONS_NAMESPACE_PREFIX	"ec"

#define EC_GPX_SCHEMA_LOCATIONS	\
	EC_GPX_XML_NAMESPACE " " EC_GPX_XML_SCHEMA_URI " " \
	EC_GPX_EXTENSIONS_NAMESPACE " " EC_GPX_EXTENSIONS_SCHEMA_LOCATION

#define EC_GPX_ATTR_VERSION_NAME	"version"
#define EC_GPX_ATTR_VERSION_CONTENT	"1.1"

#define EC_GPX_ATTR_CREATOR_NAME	"creator"
#define EC_GPX_ATTR_CREATOR_CONTENT	PACKAGE_NAME

#define EC_GPX_NODE_ROOT		"gpx"

#define EC_GPX_NODE_TRACK		"trk"
#define EC_GPX_NODE_TRACK_NAME		"name"
#define EC_GPX_NODE_TRACK_COMMENT	"cmt"
#define EC_GPX_NODE_TRACK_DESCRIPTION	"desc"
#define EC_GPX_NODE_TRACK_SOURCE	"src"
#define EC_GPX_NODE_TRACK_LINK		"link"
#define EC_GPX_NODE_TRACK_NUMBER	"number"
#define EC_GPX_NODE_TRACK_TYPE		"type"
#define EC_GPX_NODE_TRACK_SEGMENT	"trkseg"
#define EC_GPX_NODE_TRACK_POINT	"trkpt"

#define EC_GPX_NODE_EXTENSIONS		"extensions"

#define EC_GPX_NODE_WAYPOINT_ATTR_LATITUDE_NAME	"lat"
#define EC_GPX_NODE_WAYPOINT_ATTR_LONGITUDE_NAME	"lon"
#define EC_GPX_NODE_WAYPOINT_ALTITUDE	"ele"
#define EC_GPX_NODE_WAYPOINT_TIME	"time"
#define EC_GPX_NODE_WAYPOINT_FIX_TYPE	"fix"
#define EC_GPX_NODE_WAYPOINT_HDOP	"hdop"
#define EC_GPX_NODE_WAYPOINT_VDOP	"vdop"

#define EC_GPX_FIX_NONE		"none"
#define EC_GPX_FIX_2D			"2d"
#define EC_GPX_FIX_3D			"3d"

#define EC_GPX_NODE_ROUTE		"rte"
#define EC_GPX_NODE_ROUTE_POINT	"rtept"

#define EC_GPX_NODE_ROUTE_NUBMER	"number"

/* Definitions for the custom extensions */
#define EC_GPX_EXT_NODE_HEART_RATE_LIST	"hbtlist"
#define EC_GPX_EXT_NODE_HEART_RATE		"hbt"
#define EC_GPX_EXT_ATTR_HEART_RATE_TIME	"time"
#define EC_GPX_EXT_ATTR_HEART_RATE_VALUE	"value"

/* XPath definitions */
#define EC_GPX_XPATH_TRACK_NUMBER	"//gpx/trk/number"
#define EC_GPX_XPATH_ROUTE_NUMBER	"//gpx/rte/number"

#endif /* _GPX_DEFS_H */
