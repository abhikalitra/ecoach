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
#include "xml_util.h"

/* System */
#include <string.h>

/* Other modules */
#include "debug.h"

/*****************************************************************************
 * Private function prototypes                                               *
 *****************************************************************************/

/**
 * @brief Tells if namespaces are equal
 *
 * @param ns_1 Namespace 1 to compare
 * @param ns_2 Namespace 2 to compare
 *
 * @return TRUE if the namespaces are equal (or both don't exist), FALSE
 * otherwise
 *
 * @note Namespaces are assumed to be the same if their URIs are the same
 */
gboolean xml_util_namespaces_match(xmlNsPtr ns_1, xmlNsPtr ns_2);

/*****************************************************************************
 * Function declarations                                                     *
 *****************************************************************************/

/*===========================================================================*
 * Public functions                                                          *
 *===========================================================================*/

xmlNodePtr xml_util_find_or_create_child(
		xmlNodePtr parent,
		const gchar *name,
		xmlNsPtr ns,
		gboolean create_child)
{
	xmlNodePtr temp = NULL;
	xmlNodePtr found = NULL;
	g_return_val_if_fail(parent != NULL, NULL);
	g_return_val_if_fail(name != NULL, NULL);
	DEBUG_BEGIN();

	for(temp = parent->children; temp; temp = temp->next)
	{
		if(temp->type == XML_ELEMENT_NODE)
		{
			if(strcmp(temp->name, name) == 0)
			{
				if(xml_util_namespaces_match(
							temp->ns,
							ns))
				{
					found = temp;
					break;
				}
			}
		}
	}

	if(!found && create_child)
	{
		found = xmlNewChild(parent, ns, name, NULL);
	}

	DEBUG_END();
	return found;
}

xmlNodePtr xml_util_find_first_child_from_list(
		xmlNodePtr parent,
		xmlNsPtr ns,
		const gchar *name1, ...)
{
	xmlNodePtr found = NULL;
	const gchar *cur_name = NULL;
	va_list args;

	g_return_val_if_fail(parent != NULL, NULL);
	g_return_val_if_fail(name1 != NULL, NULL);
	DEBUG_BEGIN();

	cur_name = name1;
	va_start(args, name1);

	do {
		DEBUG("Searching for node %s", cur_name);
		found = xml_util_find_or_create_child(
				parent,
				cur_name,
				ns,
				FALSE);
		if(found)
		{
			break;
		}
		cur_name = va_arg(args, const gchar *);
	} while(cur_name);
	va_end(args);

	DEBUG_END();
	return found;
}

xmlNodePtr xml_util_find_or_create_child_ordered(
		xmlNodePtr parent,
		const gchar *name,
		xmlNsPtr ns,
		const gchar *name1, ...)
{
	xmlNodePtr found = NULL;
	xmlNodePtr new_node = NULL;
	const gchar *cur_name = NULL;
	va_list args;

	g_return_val_if_fail(parent != NULL, NULL);
	g_return_val_if_fail(name1 != NULL, NULL);
	DEBUG_BEGIN();

	cur_name = name1;
	va_start(args, name1);

	/* Create the node */
	new_node = xmlNewNode(ns, name);
	if(!new_node)
	{
		g_warning("Unable to create XML node");
		DEBUG_END();
		return NULL;
	}

	if(!parent->children)
	{
		DEBUG("Parent node has no children, creating child node");
		xmlAddChild(parent, new_node);
		return new_node;
	}

	found = xml_util_find_or_create_child(
			parent,
			name,
			ns,
			FALSE);
	if(found)
	{
		DEBUG("Node already exists.");
		DEBUG_END();
		return found;
	}

	do {
		DEBUG("Searching for node %s", cur_name);
		found = xml_util_find_or_create_child(
				parent,
				cur_name,
				ns,
				FALSE);
		if(found)
		{
			DEBUG("Node found, adding as the next sibling");
			break;
		}
		cur_name = va_arg(args, const gchar *);
	} while(cur_name);
	va_end(args);

	if(found)
	{
		xmlAddNextSibling(found, new_node);
	} else {
		DEBUG("No matching nodes found, adding as the first child");
		xmlAddPrevSibling(parent->children, new_node);
	}

	DEBUG_END();
	return new_node;
}

/*===========================================================================*
 * Private functions                                                         *
 *===========================================================================*/

gboolean xml_util_namespaces_match(xmlNsPtr ns_1, xmlNsPtr ns_2)
{
	DEBUG_BEGIN();
	if(ns_1 == ns_2)
	{
		DEBUG_END();
		return TRUE;
	}

	if(ns_1 == NULL || ns_2 == NULL)
	{
		/* Only either one is NULL */
		DEBUG_END();
		return FALSE;
	}

	if(ns_1->href == NULL)
	{
		if(ns_2->href == NULL)
		{
			DEBUG_END();
			return TRUE;
		} else {
			DEBUG_END();
			return FALSE;
		}
	}

	if(ns_2->href == NULL)
	{
		DEBUG_END();
		return FALSE;
	}

	if(strcmp(ns_1->href, ns_2->href) == 0)
	{
		DEBUG_END();
		return TRUE;
	}

	DEBUG_END();
	return FALSE;
}
