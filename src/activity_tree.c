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

#include "debug.h"

#include "activity_tree.h"

GtkTreeStore *activity_tree_new()
{
	GtkTreeStore *tree_store = NULL;

	DEBUG_BEGIN();

	tree_store = gtk_tree_store_new(
			7,
			G_TYPE_STRING,
			G_TYPE_STRING,
			G_TYPE_BOOLEAN,
			G_TYPE_INT,
			G_TYPE_INT,
			G_TYPE_INT);

	if(!tree_store)
	{
		g_critical("Not enough memory");
		DEBUG_END();
		return NULL;
	}

	DEBUG_END();
	return tree_store;
}

void activity_tree_get_activity_class_data(
		GtkTreeStore *activity_tree,
		/* ActivityClass *activity_class, */
		GtkTreeIter *iter)
{
	// TODO
}
