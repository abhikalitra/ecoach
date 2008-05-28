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
#include "gpx.h"
#include "gpx_defs.h"

/* System */
#include <errno.h>
#include <string.h>

/* LibXML2 */
#include <libxml/xpath.h>

/* Other modules */
#include "ec_error.h"
#include "util.h"
#include "xml_util.h"
#include "osea/ecgcodes.h"

#include "debug.h"

/*****************************************************************************
 * Private function prototypes                                               *
 *****************************************************************************/

/**
 * @brief Search and return a top node of the given route or track
 *
 * @param self Pointer to #GpxStorage
 * @param is_track Whether to search for a track or a route
 * @param route_track_id ID of the route or track to search for
 *
 * @return The pointer to the top node of the track or route, NULL
 * if no node was found
 */
xmlNodePtr gpx_storage_find_route_track(
		GpxStorage *self,
		gboolean is_track,
		guint route_track_id);

/**
 * @brief Allocates a new track ID and creates an XML node for it
 *
 * @param self Pointer to #GpxStorage
 * @param id Storage location for the allocated route ID
 *
 * @return The created XML node
 */
static xmlNodePtr gpx_storage_track_new(GpxStorage *self, guint *id);

/**
 * @brief Creates a new track segmend XML node
 *
 * @param self Pointer to #GpxStorage
 * @param parent_node Parent track node to add the route segment to
 *
 * @return The created XML node
 */
static xmlNodePtr gpx_storage_track_segment_new(GpxStorage *self,
		xmlNodePtr parent_node);

/**
 * @brief Allocates a new route ID and creates an XML node for it
 *
 * @param self Pointer to #GpxStorage
 * @param id Storage location for the allocated route ID
 *
 * @return The created XML node
 */
static xmlNodePtr gpx_storage_route_new(GpxStorage *self, guint *id);

/**
 * @brief Retrieve the last route segment in the given route
 *
 * @param self Pointer to #GpxStorage
 * @param parent_node The track node to get the segment under
 *
 * @return The track segment, or NULL in case of failure
 */
static xmlNodePtr gpx_storage_get_last_track_segment(GpxStorage *self,
		xmlNodePtr parent_node);

/*****************************************************************************
 * Function declarations                                                     *
 *****************************************************************************/

/*===========================================================================*
 * Public functions                                                          *
 *===========================================================================*/

GpxStorage *gpx_storage_new()
{
	GpxStorage *self = NULL;

	DEBUG_BEGIN();

	self = g_new0(GpxStorage, 1);

	/* Create the document and the root node */
	self->xml_document = xmlNewDoc(EC_GPX_XML_VERSION);
	self->root_node = xmlNewDocNode(self->xml_document,
			NULL,
			EC_GPX_NODE_ROOT,
			NULL);
	xmlNewProp(self->root_node,
			EC_GPX_ATTR_VERSION_NAME,
			EC_GPX_ATTR_VERSION_CONTENT);
	xmlNewProp(self->root_node,
			EC_GPX_ATTR_CREATOR_NAME,
			EC_GPX_ATTR_CREATOR_CONTENT);

	/* Add the namespaces to the root node */
	xmlNewNs(self->root_node,
			EC_GPX_XML_NAMESPACE,
			NULL);

	self->xmlns_xsi = xmlNewNs(self->root_node,
			EC_XML_SCHEMA_INSTANCE,
			EC_GPX_SCHEMA_INSTANCE_PREFIX);


	self->xmlns_gpx_extensions = xmlNewNs(self->root_node,
			EC_GPX_EXTENSIONS_NAMESPACE,
			EC_GPX_EXTENSIONS_NAMESPACE_PREFIX);

	xmlNewNsProp(self->root_node, self->xmlns_xsi,
			EC_XML_ATTR_SCHEMA_LOCATION_NAME,
			EC_GPX_SCHEMA_LOCATIONS);

	xmlDocSetRootElement(self->xml_document, self->root_node);

	DEBUG_END();
	return self;
}

void gpx_storage_free(GpxStorage *self)
{
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	xmlFreeDoc(self->xml_document);
	g_free(self->file_path);
	g_slist_free(self->track_ids);
	g_slist_free(self->route_ids);
	g_free(self);

	DEBUG_END();
}

void gpx_storage_set_path(
		GpxStorage *self,
		const gchar *path)
{
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	if(self->file_path)
	{
		g_free(self->file_path);
	}
	self->file_path = g_strdup(path);

	DEBUG_END();
}

gboolean gpx_storage_write(
		GpxStorage *self,
		GError **error)
{
	/** @todo Do autosave every now and then */
	g_return_val_if_fail(error != NULL || *error == NULL, FALSE);
	g_return_val_if_fail(self != NULL, FALSE);
	DEBUG_BEGIN();

	if(self->file_path == NULL)
	{
		g_set_error(error, EC_ERROR, EC_ERROR_FILE,
				"File name was not specified");
		return FALSE;
	}

	xmlIndentTreeOutput = 1;

	/**
	 * @todo Make configurable whether or not to use indentation
	 */
	if(xmlSaveFormatFile(self->file_path,
				self->xml_document,
				1) < 0)
	{
		g_set_error(error, EC_ERROR, EC_ERROR_FILE,
				"File saving failed");
		return FALSE;
	}

	DEBUG_END();
	return TRUE;
}

void gpx_storage_add_waypoint(
		GpxStorage *self,
		GpxStorageWaypoint *waypoint)
{
	xmlNodePtr parent_node = NULL;
	xmlNodePtr waypoint_node = NULL;
	gboolean is_track = FALSE;
	gchar *buf = NULL;
	gchar dbuf[G_ASCII_DTOSTR_BUF_SIZE];

	g_return_if_fail(self != NULL);
	g_return_if_fail(waypoint != NULL);
	DEBUG_BEGIN();

	switch(waypoint->point_type)
	{
		case GPX_STORAGE_POINT_TYPE_TRACK_START:
		case GPX_STORAGE_POINT_TYPE_TRACK_SEGMENT_START:
		case GPX_STORAGE_POINT_TYPE_TRACK:
			is_track = TRUE;
			break;
		default:
			is_track = FALSE;
	}

	if(waypoint->point_type == GPX_STORAGE_POINT_TYPE_TRACK_START)
	{
		DEBUG("Creating a new track");
		/* Create a new track */
		parent_node = gpx_storage_track_new(self,
				&waypoint->route_track_id);
	} else if(waypoint->point_type == GPX_STORAGE_POINT_TYPE_ROUTE_START) {
		DEBUG("Creating a new route");
		parent_node = gpx_storage_route_new(self,
				&waypoint->route_track_id);
	} else {
		parent_node = gpx_storage_find_route_track(
				self, is_track, waypoint->route_track_id);
	}

	if(!parent_node)
	{
		g_warning("Unable to add point to track or route");
		DEBUG_END();
		return;
	}

	if(waypoint->point_type == GPX_STORAGE_POINT_TYPE_TRACK_SEGMENT_START ||
			waypoint->point_type ==
			GPX_STORAGE_POINT_TYPE_TRACK_START)
	{
		DEBUG("Creating a new track segment");
		parent_node = gpx_storage_track_segment_new(self, parent_node);
	} else if(waypoint->point_type == GPX_STORAGE_POINT_TYPE_TRACK) {
		/* Tracks have trkseg nodes that have the actual data */
		parent_node = gpx_storage_get_last_track_segment(self,
				parent_node);
	}

	if(!parent_node)
	{
		g_warning("Unable to add point to track or route");
		DEBUG_END();
		return;
	}

	if(is_track)
	{
		waypoint_node = xmlNewChild(parent_node,
				NULL,
				EC_GPX_NODE_TRACK_POINT,
				NULL);
	} else {
		waypoint_node = xmlNewChild(parent_node,
				NULL,
				EC_GPX_NODE_ROUTE_POINT,
				NULL);
	}

	g_ascii_dtostr(dbuf, G_ASCII_DTOSTR_BUF_SIZE, waypoint->latitude);
	xmlNewProp(waypoint_node,
			EC_GPX_NODE_WAYPOINT_ATTR_LATITUDE_NAME,
			dbuf);

	g_ascii_dtostr(dbuf, G_ASCII_DTOSTR_BUF_SIZE, waypoint->longitude);
	xmlNewProp(waypoint_node,
			EC_GPX_NODE_WAYPOINT_ATTR_LONGITUDE_NAME,
			dbuf);

	if(waypoint->altitude_is_set)
	{
		g_ascii_dtostr(dbuf,
				G_ASCII_DTOSTR_BUF_SIZE, waypoint->altitude);
		xmlNewChild(waypoint_node,
				NULL,
				EC_GPX_NODE_WAYPOINT_ALTITUDE,
				dbuf);
	}

	buf = util_xml_date_time_string_from_timeval(&waypoint->timestamp);
	xmlNewChild(waypoint_node,
			NULL,
			EC_GPX_NODE_WAYPOINT_TIME,
			buf);
	g_free(buf);

	DEBUG_END();
}

void gpx_storage_add_heart_rate(
		GpxStorage *self,
		GpxStoragePointType point_type,
		guint *track_id,
		struct timeval *time,
		gint heart_rate)
{
	xmlNodePtr node_track = NULL;
	xmlNodePtr node_trkseg = NULL;
	xmlNodePtr node_extensions = NULL;
	xmlNodePtr node_hr_list = NULL;
	xmlNodePtr node_hr = NULL;
	gchar *buf = NULL;

	/** @todo Add caching for the xml node */

	g_return_if_fail(self != NULL);
	g_return_if_fail(time != NULL);
	DEBUG_BEGIN();

	if((point_type != GPX_STORAGE_POINT_TYPE_TRACK_START) &&
	   (point_type != GPX_STORAGE_POINT_TYPE_TRACK_SEGMENT_START) &&
	   (point_type != GPX_STORAGE_POINT_TYPE_TRACK))
	{
		g_warning("Invalid point type for heart rate");
		DEBUG_END();
		return;
	}

	if(point_type == GPX_STORAGE_POINT_TYPE_TRACK_START)
	{
		node_track = gpx_storage_track_new(self, track_id);
	} else {
		node_track = gpx_storage_find_route_track(
				self,
				TRUE,
				*track_id);
	}

	if(!node_track)
	{
		g_warning("Unable to find or create track with id %d",
				*track_id);
		DEBUG_END();
		return;
	}

	if((point_type == GPX_STORAGE_POINT_TYPE_TRACK_SEGMENT_START) ||
	   (point_type == GPX_STORAGE_POINT_TYPE_TRACK_START))
	{
		node_trkseg = gpx_storage_track_segment_new(self, node_track);
	} else {
		node_trkseg = gpx_storage_get_last_track_segment(self,
				node_track);
	}

	if(!node_trkseg)
	{
		g_warning("Unable to find or create a track segment");
		DEBUG_END();
		return;
	}

	node_extensions = xml_util_find_or_create_child_ordered(
			node_trkseg,
			EC_GPX_NODE_EXTENSIONS,
			NULL,
			EC_GPX_NODE_TRACK_SEGMENT,
			NULL);

	if(!node_extensions)
	{
		g_warning("Unable to find or create extension node");
		DEBUG_END();
		return;
	}

	node_hr_list = xml_util_find_or_create_child(
			node_extensions,
			EC_GPX_EXT_NODE_HEART_RATE_LIST,
			self->xmlns_gpx_extensions,
			TRUE);

	if(!node_hr_list)
	{
		g_warning("Unable to find or create hear rate list node");
		DEBUG_END();
		return;
	}

	node_hr = xmlNewChild(node_hr_list,
			self->xmlns_gpx_extensions,
			EC_GPX_EXT_NODE_HEART_RATE,
			NULL);
	if(!node_hr)
	{
		g_warning("Unable to create heart rate node");
		DEBUG_END();
		return;
	}

	buf = util_xml_date_time_string_from_timeval(time);

	xmlNewProp(node_hr,
			EC_GPX_EXT_ATTR_HEART_RATE_TIME,
			buf);
	g_free(buf);

	buf = g_strdup_printf("%d", heart_rate);
	xmlNewProp(node_hr,
			EC_GPX_EXT_ATTR_HEART_RATE_VALUE,
			buf);
	g_free(buf);

	DEBUG_END();
}

void gpx_storage_set_route_or_track_details(
		GpxStorage *self,
		gboolean is_track,
		guint route_track_id,
		const gchar *name,
		const gchar *comment)
{
	xmlNodePtr route_track = NULL;
	xmlNodePtr node_name = NULL;
	xmlNodePtr node_comment = NULL;

	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	route_track = gpx_storage_find_route_track(
			self,
			is_track,
			route_track_id);

	if(!route_track)
	{
		DEBUG("Unable to find route or track with ID %d",
				route_track_id);
		DEBUG_END();
		return;
	}

	if(name)
	{
		node_name = xml_util_find_or_create_child(route_track,
				EC_GPX_NODE_TRACK_NAME,
				NULL,
				FALSE);

		if(!node_name)
		{
			/* Create a new node; the name is always the first
			 * child node of a track */
			node_name = xmlNewNode(NULL, EC_GPX_NODE_TRACK_NAME);
			if(route_track->children)
			{
				xmlAddPrevSibling(
						route_track->children,
						node_name);
			} else {
				xmlAddChild(route_track, node_name);
			}

		} else {
			/* Remove old name */
			xmlNodeSetContent(node_name, "");
		}
		xmlNodeAddContent(node_name, name);
	}
	
	if(comment)
	{
		node_comment = xml_util_find_or_create_child(route_track,
				EC_GPX_NODE_TRACK_COMMENT,
				NULL,
				FALSE);

		if(!node_comment)
		{
			/* Create a new node; the comment is always the first
			 * child node of a track after the name */
			node_comment = xmlNewNode(
					NULL,
					EC_GPX_NODE_TRACK_COMMENT);
			if(node_name)
			{
				xmlAddNextSibling(node_name, node_comment);
			} else {
				if(route_track->children)
				{
					xmlAddPrevSibling(
							route_track->children,
							node_comment);
				} else {
					xmlAddChild(route_track, node_comment);
				}
			}
		} else {
			/* Remove old name */
			xmlNodeSetContent(node_comment, "");
		}
		xmlNodeAddContent(node_comment, comment);
	}

	DEBUG_END();
}

/*===========================================================================*
 * Private functions                                                         *
 *===========================================================================*/

xmlNodePtr gpx_storage_find_route_track(
		GpxStorage *self,
		gboolean is_track,
		guint route_track_id)
{
	xmlXPathContextPtr xpathCtx;
	xmlXPathObjectPtr xpathObj;
	xmlNodePtr retval = NULL;
	xmlNodeSetPtr nodes = NULL;

	gint i = 0;
	guint found_id;

	/**
	 * @todo It is inefficient to find the right track every time.
	 * A simple cache of the previous result would be easy to implement,
	 * also it is possible to implement a list of trackid-node -pairs
	 */

	g_return_val_if_fail(self != NULL, NULL);
	DEBUG_BEGIN();

	xpathCtx = xmlXPathNewContext(self->xml_document);
	if(xpathCtx == NULL)
	{
		g_warning("Unable to create XPath context");
		DEBUG_END();
		return NULL;
	}

	if(is_track)
	{
		xpathObj = xmlXPathEvalExpression(
				EC_GPX_XPATH_TRACK_NUMBER,
				xpathCtx);
	} else {
		xpathObj = xmlXPathEvalExpression(
				EC_GPX_XPATH_ROUTE_NUMBER,
				xpathCtx);
	}

	if(xpathObj == NULL)
	{
		xmlXPathFreeContext(xpathCtx);
		g_warning("Unable to evaluate XPath expression");
		DEBUG_END();
		return NULL;
	}

	nodes = xpathObj->nodesetval;
	for(i = 0; i < nodes->nodeNr; i++)
	{
		g_assert(nodes->nodeTab[i]);
		if(nodes->nodeTab[i]->type == XML_ELEMENT_NODE)
		{
			/* Check if the route id matches */
			errno = 0;
			found_id = strtoul(nodes->nodeTab[i]->children->content,
						NULL, 10);
			if(errno == 0 && found_id == route_track_id)
			{
				retval = nodes->nodeTab[i]->parent;
				break;
			}
		}
	}

	if(!retval)
	{
		/* No track/route was found */
		g_warning("No matching nodes found for route/track id %d",
				route_track_id);
	}

	xmlXPathFreeObject(xpathObj);
	xmlXPathFreeContext(xpathCtx);

	DEBUG_END();
	return retval;
}

static xmlNodePtr gpx_storage_track_new(GpxStorage *self, guint *id)
{
	GSList *temp = NULL;
	guint curr = 0;
	guint prev = 0;
	xmlNodePtr retval = NULL;
	gchar *buf = NULL;
	gboolean found_gap = FALSE;
	gint counter = 0;

	g_return_val_if_fail(self != NULL, NULL);
	g_return_val_if_fail(id != NULL, NULL);
	DEBUG_BEGIN();

	/* Search for an available number */
	for(temp = self->track_ids; temp; temp = g_slist_next(temp))
	{
		counter++;
		curr = GPOINTER_TO_UINT(temp->data);

		if(curr - prev > 1)
		{
			/* There is a gap. Use that for the ID */
			DEBUG_LONG("Found a gap. Using track id %d", curr);
			found_gap = TRUE;
			prev = curr - 1;
			break;
		}
		prev = curr;
	}
	if(!found_gap)
	{
		/* There were no gaps. Add a new ID */
		*id = counter;
		self->track_ids = g_slist_append(self->track_ids,
				GUINT_TO_POINTER(prev));

	} else {
		/* There was a gap. Use it. */
		*id = prev;
		self->track_ids = g_slist_insert(
				self->track_ids,
				GUINT_TO_POINTER(prev),
				counter - 1);
	}

	DEBUG("Adding track with id %d", *id);

	/* Create the XML node */
	retval = xmlNewChild(self->root_node,
			NULL,
			EC_GPX_NODE_TRACK,
			NULL);

	/* Add the route number */
	buf = g_strdup_printf("%u", *id);
	xmlNewChild(retval,
			NULL,
			EC_GPX_NODE_TRACK_NUMBER,
			buf);
	g_free(buf);

	DEBUG_END();
	return retval;
}

static xmlNodePtr gpx_storage_track_segment_new(GpxStorage *self,
		xmlNodePtr parent_node)
{
	xmlNodePtr retval = NULL;

	g_return_val_if_fail(self != NULL, NULL);
	g_return_val_if_fail(parent_node != NULL, NULL);
	DEBUG_BEGIN();

	retval = xmlNewChild(parent_node,
			NULL,
			EC_GPX_NODE_TRACK_SEGMENT,
			NULL);

	DEBUG_END();
	return retval;
}

static xmlNodePtr gpx_storage_route_new(GpxStorage *self, guint *id)
{
	GSList *temp = NULL;
	guint curr = 0;
	guint prev = 0;
	xmlNodePtr retval = NULL;
	gchar *buf = NULL;

	g_return_val_if_fail(self != NULL, NULL);
	g_return_val_if_fail(id != NULL, NULL);
	DEBUG_BEGIN();

	/* Search for an available number */
	for(temp = self->route_ids; temp; temp = g_slist_next(temp))
	{
		curr = GPOINTER_TO_UINT(temp->data);

		if(curr - prev > 1)
		{
			/* There is a gap. Use that for the ID */
			prev = curr - 1;
			break;
		}
		prev = curr;
	}
	if(prev == curr && prev != 0)
	{
		/* There were no gaps. Add a new ID */
		prev++;
	}

	DEBUG("Adding route with id %d", prev);
	self->route_ids = g_slist_append(self->route_ids,
			GUINT_TO_POINTER(prev));

	*id = prev;

	/* Create the XML node */
	retval = xmlNewChild(self->root_node,
			NULL,
			EC_GPX_NODE_ROUTE,
			NULL);

	/* Add the route number */
	buf = g_strdup_printf("%u", *id);
	xmlNewChild(retval,
			NULL,
			EC_GPX_NODE_ROUTE_NUBMER,
			buf);
	g_free(buf);

	DEBUG_END();
	return retval;
}


static xmlNodePtr gpx_storage_get_last_track_segment(GpxStorage *self,
		xmlNodePtr parent_node)
{
	xmlNodePtr temp;
	xmlNodePtr found = NULL;

	g_return_val_if_fail(self != NULL, NULL);
	g_return_val_if_fail(parent_node != NULL, NULL);
	DEBUG_BEGIN();

	for(temp = parent_node->children; temp; temp = temp->next)
	{
		if(temp->type == XML_ELEMENT_NODE)
		{
			if(strcmp(temp->name, "trkseg") == 0)
			{
				found = temp;	
			}
		}
	}

	if(!found)
	{
		g_warning("No route segments");
	}

	DEBUG_END();
	return found;
}
