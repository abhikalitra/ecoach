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

#ifndef _ACTIVITY_TREE_H
#define _ACTIVITY_TREE_H

/* Configuration */
#include "config.h"

#include <gtk/gtk.h>

#include "activity.h"

typedef enum _ActivityTreeColumns {
	ACTIVITY_TREE_COLUMN_SHORT_NAME = 0,
	ACTIVITY_TREE_COLUMN_DESCRIPTION,
	ACTIVITY_TREE_COLUMN_KEEP_TARGET_PULSE,
	ACTIVITY_TREE_COLUMN_BPM_MIN,
	ACTIVITY_TREE_COLUMN_BPM_MAX,
	ACTIVITY_TREE_COLUMN_PERCENTS_MIN,
	ACTIVITY_TREE_COLUMN_PERCENTS_MAX
} ActivityTreeColumns;

/** Create a new activity tree (which is just a GtkTreeStore) */
GtkTreeStore *activity_tree_new();

/**
 * @brief Fill in the information of an #ActivityClass from the tree store
 *
 * @param activity_tree Tree store that has the activity classes
 * @param iter Iterator pointing to the node that holds the information
 * @param activity_class Activity class to be filled
 */
void activity_tree_get_activity_class_data(
		GtkTreeStore *activity_tree,
		/* ActivityClass *activity_class, */
		GtkTreeIter *iter);

#endif /* _ACTIVITY_TREE_H */
