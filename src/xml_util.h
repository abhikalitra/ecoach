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

#ifndef _XML_UTIL_H
#define _XML_UTIL_H

/* Configuration */
#include "config.h"

/* System */
#include <sys/time.h>
#include <time.h>

/* GLib */
#include <glib.h>

/* LibXML2 */
#include <libxml/tree.h>

/**
 * @brief Find or create a direct child node
 *
 * This function tries to find for a node with matching name and namespace.
 * If the node is not found and %create_child is TRUE, then the node is
 * created.
 *
 * @param parent Node under which to search
 * @param name Name of the node to search for
 * @param ns Namespace of the element
 * @param create_child If TRUE, then create the child if it is not found
 *
 * @return The searched node if found or created, NULL otherwise
 */
xmlNodePtr xml_util_find_or_create_child(
		xmlNodePtr parent,
		const gchar *name,
		xmlNsPtr ns,
		gboolean create_child);

/**
 * @brief Find a child with given name. As soon as first child matching
 * the name and namespace is found, it will be returned.
 *
 * @param parent Node under which to search
 * @param ns Namespace of the element
 * @param name1 First of the names to search
 * @param ... Further list of names to search. Last item must be NULL.
 *
 * @return The first child matching the name and namespace, or NULL
 * if no such child is found
 */
xmlNodePtr xml_util_find_first_child_from_list(
		xmlNodePtr parent,
		xmlNsPtr ns,
		const gchar *name1, ...);

/**
 * @brief Find a child with given name. If it is not found, create it
 * into the correct place between siblings, determined by the arguments.
 *
 * @param parent Node under which to search
 * @param name Name of the node to search for
 * @param ns Namespace of the element
 * @param name1 Last sibling
 * @param ... Further list of siblings in correct order. Last element
 * must be NULL.
 *
 * @return The first child matching the name and namespace, or NULL
 * if no such child is found
 *
 * @note The list of the siblings must be in reversed order.
 *
 * This functions exists, because XML schemas often use sequences, which
 * determine the order of the siblings. An XML schema might say that nodes
 * a, b, c, d  and e must be always in that order. If nodes a, b and e exist,
 * and node d is to be added, it must be added after node a, but before node c.
 * Then one would call this function for example as
 * xml_util_find_or_create_child_ordered(parent, "d", NULL, "c", "b", "a", NULL);
 * The rest of the nodes (in this case, e) don't matter, because node c will
 * be always before them. Do not list the node to be added (in the example, d)
 * to the list, nor the siblings that are supposed to be after the node you
 * insert, or you may get wrong results.
 */
xmlNodePtr xml_util_find_or_create_child_ordered(
		xmlNodePtr parent,
		const gchar *name,
		xmlNsPtr ns,
		const gchar *name1, ...);

#endif /* _XML_UTIL_H */
