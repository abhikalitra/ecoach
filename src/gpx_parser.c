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
#include "gpx_parser.h"

/* System */
#include <errno.h>
#include <string.h>

/* LibXML2 */
#include <libxml/parser.h>

/* Other modules */
#include "gpx_defs.h"
#include "ec_error.h"
#include "util.h"

#include "debug.h"

/*****************************************************************************
 * Enumerations                                                              *
 *****************************************************************************/

typedef enum _GpxParserState {
	GPX_PARSER_STATE_DOC_START,
	GPX_PARSER_STATE_IN_ROOT,
	GPX_PARSER_STATE_IN_TRACK,
	GPX_PARSER_STATE_IN_ROUTE,
	GPX_PARSER_STATE_IN_TRACK_NAME,
	GPX_PARSER_STATE_IN_TRACK_COMMENT,
	GPX_PARSER_STATE_IN_TRACK_NUMBER,
	GPX_PARSER_STATE_IN_TRACK_SEGMENT_EXTENSIONS,
	GPX_PARSER_STATE_IN_TRACK_SEGMENT,
	GPX_PARSER_STATE_IN_TRACK_WAYPOINT,
	GPX_PARSER_STATE_IN_TRACK_WAYPOINT_ALTITUDE,
	GPX_PARSER_STATE_IN_TRACK_WAYPOINT_TIME,
	GPX_PARSER_STATE_IN_ROUTE_WAYPOINT,
	GPX_PARSER_STATE_IN_HEART_RATE_LIST,
	GPX_PARSER_STATE_IN_HEART_RATE,
	GPX_PARSER_STATE_IN_UNKNOWN,
	GPX_PARSER_STATE_UNRECOVERABLE_ERROR,
	GPX_PARSER_STATE_FINISHED
} GpxParserState;

/*****************************************************************************
 * Data structures                                                           *
 *****************************************************************************/

typedef struct _GpxParserPriv {
	GpxParserCallback callback;
	gpointer user_data;
	GpxParserState state;
	GpxParserState last_known;
	guint unknown_depth;
	GpxParserStatus retval;
	GpxParserData data;
	GString *buffer;
	gboolean metadata_sent;
	GpxStoragePointType next_point_type;
} GpxParserPriv;

typedef struct _GpxParserSAX2Attribute {
	const xmlChar *name;
	const xmlChar *prefix;
	const xmlChar *URI;
	const xmlChar *value_start;
	const xmlChar *value_end;
} GpxParserSAX2Attribute;

/*****************************************************************************
 * Private function prototypes                                               *
 *****************************************************************************/

static void gpx_parser_unknown_node(GpxParserPriv *self);

static xmlEntityPtr gpx_parser_sax_get_entity(void *ctx, const xmlChar *name)
{
	xmlEntityPtr entity;
	DEBUG_BEGIN();

	entity = xmlGetPredefinedEntity(name);

	DEBUG_END();
	return entity;
}

static void gpx_parser_sax_attribute_decl(void *ctx,
		const xmlChar *elem,
		const xmlChar *name,
		int type,
		int def,
		const xmlChar *defaultValue,
		xmlEnumerationPtr tree)
{
	GpxParserPriv *self = (GpxParserPriv *)ctx;
	g_return_if_fail(self != NULL);

	DEBUG_END();
	DEBUG_END();
}

static void gpx_parser_sax_start_document(void *ctx)
{
	GpxParserPriv *self = (GpxParserPriv *)ctx;
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	self->state = GPX_PARSER_STATE_DOC_START;
	self->last_known = GPX_PARSER_STATE_DOC_START;
	self->unknown_depth = 0;
	self->retval = GPX_PARSER_STATUS_OK;
	self->buffer = g_string_new("");
	self->next_point_type = GPX_STORAGE_POINT_TYPE_TRACK_START;

	DEBUG_END();
}

static void gpx_parser_sax_end_document(void *ctx)
{
	GpxParserPriv *self = (GpxParserPriv *)ctx;
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	g_string_free(self->buffer, TRUE);

	DEBUG_END();
}

static void gpx_parser_sax_start_element_ns(
		void *ctx,
		const xmlChar *name,
		const xmlChar *prefix,
		const xmlChar *URI,
		int nb_namespaces,
		const xmlChar **namespaces,
		int nb_attributes,
		int nb_defaulted,
		const xmlChar **attributes);

static void gpx_parser_sax_end_element_ns(
		void *ctx,
		const xmlChar *name,
		const xmlChar *prefix,
		const xmlChar *URI);

static void gpx_parser_sax_characters(void *ctx,
		const xmlChar *ch,
		int len);

/*===========================================================================*
 * Non-SAX function prototypes                                               *
 *===========================================================================*/

/**
 * @brief Parse track point data (latitude and longitude) from attributes
 *
 * @param self Pointer to #GpxParserPriv
 * @param attributes Node attributes to parse the track point data from
 */
static void gpx_parser_parse_track_point(
		GpxParserPriv *self,
		gint nb_attributes, 
		const xmlChar **attributes);

/**
 * @brief Parse heart rate data (timestamp and value) from attributes
 *
 * @param self Pointer to #GpxParserPriv
 * @param attributes Node attributes to parse the heart rate data from
 */
static void gpx_parser_parse_heart_rate(
		GpxParserPriv *self,
		gint nb_attributes,
		const xmlChar **attributes);

static void gpx_parser_free_data(GpxParserPriv *self,
		GpxParserDataType data_type);

/*****************************************************************************
 * Static variables                                                          *
 *****************************************************************************/

static xmlSAXHandler gpx_parser_sax_handler = {
	NULL,			/* internalSubset		*/
	NULL,			/* isStandalone			*/
	NULL,			/* hasInternalSubset		*/
	NULL,			/* hasExternalSubset		*/
	NULL,			/* resolveEntity		*/
	gpx_parser_sax_get_entity,
	NULL,			/* entityDecl			*/
	NULL,			/* notationDecl			*/
	gpx_parser_sax_attribute_decl,
	NULL,			/* elementDecl			*/
	NULL,			/* unparsedEntity		*/
	NULL,			/* setDocumentLocator		*/
	gpx_parser_sax_start_document,
	gpx_parser_sax_end_document,
	NULL,			/* startElement			*/
	NULL,			/* endElement			*/
	NULL,			/* reference			*/
	gpx_parser_sax_characters,
	NULL,			/* ignorableWhitespace		*/
	NULL,			/* processingInstruction	*/
	NULL,			/* comment			*/
	NULL,			/* warning			*/
	NULL,			/* error			*/
	NULL,			/* fatalError			*/
	NULL,			/* getParameterEntity		*/
	NULL,			/* cdataBlock			*/
	NULL,			/* externalSubset		*/
	XML_SAX2_MAGIC,
	NULL,			/* private			*/
	gpx_parser_sax_start_element_ns,
	gpx_parser_sax_end_element_ns,
	NULL			/* serror			*/	
};

/*****************************************************************************
 * Function declarations                                                     *
 *****************************************************************************/

/*===========================================================================*
 * Public functions                                                          *
 *===========================================================================*/

GpxParserStatus gpx_parser_parse_file(
		const gchar *file_name,
		GpxParserCallback callback,
		gpointer user_data,
		GError **error)
{
	GpxParserPriv self;

	g_return_val_if_fail(error == NULL || *error == NULL,
			GPX_PARSER_STATUS_FAILED);
	g_return_val_if_fail(file_name != NULL, GPX_PARSER_STATUS_FAILED);
	g_return_val_if_fail(callback  != NULL, GPX_PARSER_STATUS_FAILED);

	DEBUG_BEGIN();

	self.callback = callback;
	self.user_data = user_data;

	if(xmlSAXUserParseFile(&gpx_parser_sax_handler, &self, file_name) < 0)
	{
		g_set_error(error, EC_ERROR, EC_ERROR_FILE,
				"Failed to open file");
		DEBUG_END();
		return GPX_PARSER_STATUS_FAILED;
	}

	DEBUG_END();
	return self.retval;
}

/*===========================================================================*
 * Private functions                                                         *
 *===========================================================================*/

/*---------------------------------------------------------------------------*
 * SAX parser functions                                                      *
 *---------------------------------------------------------------------------*/

static void gpx_parser_sax_start_element_ns(
		void *ctx,
		const xmlChar *name,
		const xmlChar *prefix,
		const xmlChar *URI,
		int nb_namespaces,
		const xmlChar **namespaces,
		int nb_attributes,
		int nb_defaulted,
		const xmlChar **attributes)
{
	GpxParserPriv *self = (GpxParserPriv *)ctx;
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();
	DEBUG("Beginning node %s", name);

	/* Clear the buffer from possible garbage */
	g_string_set_size(self->buffer, 0);

	if(self->state == GPX_PARSER_STATE_UNRECOVERABLE_ERROR) {
		/* Unrecoverable error */
		DEBUG_END();
		return;
	} else if(self->state == GPX_PARSER_STATE_DOC_START) {
		if(strcmp(name, EC_GPX_NODE_ROOT) == 0)
		{
			DEBUG("Found root node");
			self->state = GPX_PARSER_STATE_IN_ROOT;
		} else {
			g_warning("Wrong root node: %s", name);
			self->state = GPX_PARSER_STATE_UNRECOVERABLE_ERROR;
			self->retval = GPX_PARSER_STATUS_FAILED;
		}
	} else if(self->state == GPX_PARSER_STATE_IN_ROOT) {
		self->metadata_sent = FALSE;

		if(strcmp(name, EC_GPX_NODE_TRACK) == 0)
		{
			DEBUG("Found track node");
			self->next_point_type =
				GPX_STORAGE_POINT_TYPE_TRACK_START;
			self->state = GPX_PARSER_STATE_IN_TRACK;
			self->data.track = g_new0(GpxParserDataTrack, 1);

		} else if(strcmp(name, EC_GPX_NODE_ROUTE) == 0) {
			DEBUG("Found route node");
			self->next_point_type =
				GPX_STORAGE_POINT_TYPE_ROUTE_START;
			self->state = GPX_PARSER_STATE_IN_ROUTE;

		} else  {
			DEBUG("Unknown node under root: %s", name);
			gpx_parser_unknown_node(self);
		}

	} else if(self->state == GPX_PARSER_STATE_IN_TRACK) {
		if(strcmp(name, EC_GPX_NODE_TRACK_NAME) == 0)
		{
			if(self->metadata_sent)
			{
				g_warning("Track name is in wrong place");
				gpx_parser_unknown_node(self);
			} else {
				DEBUG("Found track name");
				self->state = GPX_PARSER_STATE_IN_TRACK_NAME;
			}
		} else if(strcmp(name, EC_GPX_NODE_TRACK_COMMENT) == 0) {
			if(self->metadata_sent)
			{
				g_warning("Track comment is in wrong place");
				gpx_parser_unknown_node(self);
			} else {
				DEBUG("Found track comment");
				self->state = GPX_PARSER_STATE_IN_TRACK_COMMENT;
			}
		} else if(strcmp(name, EC_GPX_NODE_TRACK_NUMBER) == 0) {
			if(self->metadata_sent)
			{
				g_warning("Track number is in wrong place");
				gpx_parser_unknown_node(self);
			} else {
				DEBUG("Found track number");
				self->state = GPX_PARSER_STATE_IN_TRACK_NUMBER;
			}
		} else if(strcmp(name, EC_GPX_NODE_TRACK_SEGMENT) == 0) {
			if(!self->metadata_sent)
			{
				/* Send the name and comment (GPX schema
				 * requires that they are before the track
				 * segments) */
				self->metadata_sent = TRUE;
				self->callback(
						GPX_PARSER_DATA_TYPE_TRACK,
						&self->data,
						self->user_data);
				gpx_parser_free_data(
						self,
						GPX_PARSER_DATA_TYPE_TRACK);
			}
			DEBUG("Found track segment");
			self->state = GPX_PARSER_STATE_IN_TRACK_SEGMENT;
			if(self->next_point_type !=
					GPX_STORAGE_POINT_TYPE_TRACK_START)
			{
				self->next_point_type =
				GPX_STORAGE_POINT_TYPE_TRACK_SEGMENT_START;
			}
			/* Send also the track segment */

			/* There is not any data in the track segment really,
			 * but do this for completeness */
			self->data.track_segment =
				g_new0(GpxParserDataTrackSegment, 1);
			self->callback(
					GPX_PARSER_DATA_TYPE_TRACK_SEGMENT,
					&self->data,
					self->user_data);
			gpx_parser_free_data(
					self,
					GPX_PARSER_DATA_TYPE_TRACK_SEGMENT);
		} else {
			DEBUG("Unknown node");
			gpx_parser_unknown_node(self);
		}
	} else if(self->state == GPX_PARSER_STATE_IN_TRACK_SEGMENT) {
		if(strcmp(name, EC_GPX_NODE_TRACK_POINT) == 0)
		{
			/* The point type will be automatically set to
			 * a normal track point when sending the first
			 * point in the track / track segment */
			self->state = GPX_PARSER_STATE_IN_TRACK_WAYPOINT;
			gpx_parser_parse_track_point(self, nb_attributes,
				       attributes);
		} else if(strcmp(name, EC_GPX_NODE_EXTENSIONS) == 0) {
			DEBUG("Found track segment extensions");
			self->state =
				GPX_PARSER_STATE_IN_TRACK_SEGMENT_EXTENSIONS;
		} else {
			gpx_parser_unknown_node(self);
		}
	} else if(self->state == GPX_PARSER_STATE_IN_TRACK_WAYPOINT) {
		if(strcmp(name, EC_GPX_NODE_WAYPOINT_ALTITUDE) == 0)
		{
			self->state = GPX_PARSER_STATE_IN_TRACK_WAYPOINT_ALTITUDE;
		} else if(strcmp(name, EC_GPX_NODE_WAYPOINT_TIME) == 0) {
			self->state = GPX_PARSER_STATE_IN_TRACK_WAYPOINT_TIME;
		} else {
			gpx_parser_unknown_node(self);
		}
	} else if(self->state ==
			GPX_PARSER_STATE_IN_TRACK_SEGMENT_EXTENSIONS) {
		if(strcmp(name, EC_GPX_EXT_NODE_HEART_RATE_LIST) == 0)
		{
			if(strcmp(URI, EC_GPX_EXTENSIONS_NAMESPACE) == 0)
			{
				self->state =
					GPX_PARSER_STATE_IN_HEART_RATE_LIST;
			} else {
				gpx_parser_unknown_node(self);
			}
		} else {
			gpx_parser_unknown_node(self);
		}
	} else if(self->state == GPX_PARSER_STATE_IN_HEART_RATE_LIST) {
		if(strcmp(name, EC_GPX_EXT_NODE_HEART_RATE) == 0)
		{
			if(strcmp(URI, EC_GPX_EXTENSIONS_NAMESPACE) == 0)
			{
				self->state = GPX_PARSER_STATE_IN_HEART_RATE;
				gpx_parser_parse_heart_rate(self, nb_attributes,
						attributes);
			} else {
				gpx_parser_unknown_node(self);
			}
		} else {
			gpx_parser_unknown_node(self);
		}
	} else {
		gpx_parser_unknown_node(self);
	}


	DEBUG_END();
}

static void gpx_parser_sax_end_element_ns(
		void *ctx,
		const xmlChar *name,
		const xmlChar *prefix,
		const xmlChar *URI)
{
	GpxParserPriv *self = (GpxParserPriv *)ctx;
	g_return_if_fail(self != NULL);
	gchar *tmp_buffer = NULL;
	DEBUG_BEGIN();
	DEBUG("Ending node: %s", name);

	if(self->state == GPX_PARSER_STATE_IN_ROOT) {
		self->state = GPX_PARSER_STATE_FINISHED;

	} else if(self->state == GPX_PARSER_STATE_IN_TRACK) {
		self->state = GPX_PARSER_STATE_IN_ROOT;

	} else if(self->state == GPX_PARSER_STATE_IN_ROUTE) {
		self->state = GPX_PARSER_STATE_IN_ROOT;

	} else if(self->state ==
			GPX_PARSER_STATE_IN_TRACK_SEGMENT_EXTENSIONS) {
		self->state = GPX_PARSER_STATE_IN_TRACK_SEGMENT;

	} else if(self->state == GPX_PARSER_STATE_IN_TRACK_NAME) {
		self->state = GPX_PARSER_STATE_IN_TRACK;
		self->data.track->name = g_string_free(self->buffer, FALSE);
		self->buffer = g_string_new("");

	} else if(self->state == GPX_PARSER_STATE_IN_TRACK_COMMENT) {
		self->state = GPX_PARSER_STATE_IN_TRACK;
		self->data.track->comment = g_string_free(self->buffer, FALSE);
		self->buffer = g_string_new("");

	} else if(self->state == GPX_PARSER_STATE_IN_TRACK_NUMBER) {
		self->state = GPX_PARSER_STATE_IN_TRACK;
		tmp_buffer = g_string_free(self->buffer, FALSE);
		self->buffer = g_string_new("");
		errno = 0;
		self->data.track->number = strtoul(tmp_buffer, NULL, 10);
		if(errno)
		{
			self->data.track->number = ULONG_MAX;
		}
		g_free(tmp_buffer);

	} else if(self->state == GPX_PARSER_STATE_IN_TRACK_SEGMENT) {
		self->state = GPX_PARSER_STATE_IN_TRACK;

	} else if(self->state == GPX_PARSER_STATE_IN_TRACK_WAYPOINT) {
		self->state = GPX_PARSER_STATE_IN_TRACK_SEGMENT;
		/* Send the waypoint */
		self->data.waypoint->point_type = self->next_point_type;
		self->next_point_type = GPX_STORAGE_POINT_TYPE_TRACK;
		self->callback(
				GPX_PARSER_DATA_TYPE_WAYPOINT,
				&self->data,
				self->user_data);
		gpx_parser_free_data(
				self,
				GPX_PARSER_DATA_TYPE_WAYPOINT
				);

	} else if(self->state == GPX_PARSER_STATE_IN_TRACK_WAYPOINT_ALTITUDE) {
		self->state = GPX_PARSER_STATE_IN_TRACK_WAYPOINT;
		tmp_buffer = g_string_free(self->buffer, FALSE);
		self->buffer = g_string_new("");
		self->data.waypoint->altitude = g_ascii_strtod(
				tmp_buffer, NULL);
		if(errno)
		{
			g_warning("Unable to parse as a number: %s",
					tmp_buffer);
		} else {
			self->data.waypoint->altitude_is_set = TRUE;
		}
		g_free(tmp_buffer);

	} else if(self->state == GPX_PARSER_STATE_IN_TRACK_WAYPOINT_TIME) {
		self->state = GPX_PARSER_STATE_IN_TRACK_WAYPOINT;
		tmp_buffer = g_string_free(self->buffer, FALSE);
		self->buffer = g_string_new("");
		util_timeval_from_xml_date_time_string(tmp_buffer,
				&self->data.waypoint->timestamp);
		g_free(tmp_buffer);

	} else if(self->state == GPX_PARSER_STATE_IN_HEART_RATE_LIST ) {
		self->state = GPX_PARSER_STATE_IN_TRACK_SEGMENT_EXTENSIONS;

	} else if(self->state == GPX_PARSER_STATE_IN_HEART_RATE) {
		self->state = GPX_PARSER_STATE_IN_HEART_RATE_LIST;
		self->callback(
				GPX_PARSER_DATA_TYPE_HEART_RATE,
				&self->data,
				self->user_data);
		gpx_parser_free_data(
				self,
				GPX_PARSER_DATA_TYPE_HEART_RATE
				);

	} else if(self->state == GPX_PARSER_STATE_IN_ROOT) {
		if(strcmp(name, EC_GPX_NODE_ROOT) == 0)
		{
			DEBUG("Root node closed. Parsing should be done.");
			self->state = GPX_PARSER_STATE_FINISHED;
		}

	} else if(self->state == GPX_PARSER_STATE_IN_UNKNOWN) {
		self->unknown_depth--;
		if(self->unknown_depth == 0)
		{
			self->state = self->last_known;
		} else if(self->unknown_depth < 0) {
			g_warning("Probably a bug in parsing code (error "
					"1)\n[depth: %d, node: %s]",
					self->unknown_depth, name);
			self->unknown_depth = 0;
		}

	} else {
		g_warning("Probably a bug in parsing code (error 2)\n"
				"[parser state: %d, node: %s]",
				self->state, name);
	}

	DEBUG_END();
}

static void gpx_parser_sax_characters(void *ctx,
		const xmlChar *ch,
		int len)
{
	GpxParserPriv *self = (GpxParserPriv *)ctx;
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	g_string_append_len(self->buffer, ch, len);

	DEBUG_END();
}

static void gpx_parser_unknown_node(GpxParserPriv *self)
{
	g_return_if_fail(self != NULL);
	if(self->state != GPX_PARSER_STATE_IN_UNKNOWN)
	{
		self->last_known = self->state;
		self->state = GPX_PARSER_STATE_IN_UNKNOWN;
	}
	self->unknown_depth++;
}

/*---------------------------------------------------------------------------*
 * Non-SAX functions                                                         *
 *---------------------------------------------------------------------------*/

static void gpx_parser_parse_track_point(
		GpxParserPriv *self,
		gint nb_attributes,
		const xmlChar **attributes)
{
	GpxParserSAX2Attribute *attr = NULL;
	gchar *value = NULL;
	gint i = 0;

	g_return_if_fail(self != NULL);
	g_return_if_fail(attributes != NULL);
	DEBUG_BEGIN();

	self->data.waypoint = g_new0(GpxParserDataWaypoint, 1);

	for(i = 0; i < nb_attributes; i++)
	{
		attr = (GpxParserSAX2Attribute *)(attributes + 5 * i);
		if(strcmp(attr->name,
				EC_GPX_NODE_WAYPOINT_ATTR_LATITUDE_NAME) == 0)
		{
			value = g_strndup(attr->value_start,
					attr->value_end - attr->value_start);
			self->data.waypoint->latitude =
				g_ascii_strtod(value, NULL);
			if(errno)
			{
				g_warning("Unable to parse as a number: %s",
						value);
			}
			g_free(value);
		} else if(strcmp(attr->name,
				EC_GPX_NODE_WAYPOINT_ATTR_LONGITUDE_NAME) == 0)
		{
			value = g_strndup(attr->value_start,
					attr->value_end - attr->value_start);
			self->data.waypoint->longitude =
				g_ascii_strtod(value, NULL);
			if(errno)
			{
				g_warning("Unable to parse as a number: %s",
						value);
			}
			g_free(value);
		}
	}
	DEBUG("Lat: %f, long: %f", self->data.waypoint->latitude,
			self->data.waypoint->longitude);

	DEBUG_END();
}

static void gpx_parser_parse_heart_rate(
		GpxParserPriv *self,
		gint nb_attributes,
		const xmlChar **attributes)
{
	GpxParserSAX2Attribute *attr = NULL;
	gchar *value = NULL;
	gint i = 0;

	g_return_if_fail(self != NULL);
	g_return_if_fail(attributes != NULL);
	DEBUG_BEGIN();

	self->data.heart_rate = g_new0(GpxParserDataHeartRate, 1);

	for(i = 0; i < nb_attributes; i++)
	{
		attr = (GpxParserSAX2Attribute *)(attributes + 5 * i);
		if(strcmp(attr->name, EC_GPX_EXT_ATTR_HEART_RATE_TIME) == 0)
		{
			value = g_strndup(attr->value_start,
					attr->value_end - attr->value_start);
			util_timeval_from_xml_date_time_string(
					value,
					&self->data.heart_rate->timestamp);
		} else if(strcmp(attr->name,
				EC_GPX_EXT_ATTR_HEART_RATE_VALUE) == 0)
		{
			value = g_strndup(attr->value_start,
					attr->value_end - attr->value_start);
			self->data.heart_rate->value =
				strtoul(value, NULL, 10);
			if(errno)
			{
				g_warning("Unable to parse as a number: %s",
						value);
			}
			g_free(value);
		}
	}

	DEBUG_END();
}
static void gpx_parser_free_data(GpxParserPriv *self,
		GpxParserDataType data_type)
{
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	switch(data_type)
	{
		case GPX_PARSER_DATA_TYPE_ROUTE:
			g_free(self->data.route->name);
			g_free(self->data.route->comment);
			g_free(self->data.route);
			break;
		case GPX_PARSER_DATA_TYPE_TRACK:
			g_free(self->data.track->name);
			g_free(self->data.track->comment);
			g_free(self->data.route);
			break;
		case GPX_PARSER_DATA_TYPE_TRACK_SEGMENT:
			g_free(self->data.track_segment);
			break;
		case GPX_PARSER_DATA_TYPE_WAYPOINT:
			g_free(self->data.waypoint);
			break;
		case GPX_PARSER_DATA_TYPE_HEART_RATE:
			g_free(self->data.heart_rate);
			break;
		default:
			g_warning("Unknown data type to be freed: %d",
					data_type);
	}
}

