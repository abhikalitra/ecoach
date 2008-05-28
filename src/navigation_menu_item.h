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

#ifndef _NAVIGATION_MENU_ITEM_H
#define _NAVIGATION_MENU_ITEM_H

#include <gtk/gtk.h>

#include "navigation_menu_priv.h"
#include "ec-button.h"

/**
 * @brief Type definition for navigation menu callback
 *
 * @param menu Navigation menu that the menu item belongs to
 * @param path Patht to the menu item that was clicked
 * @param user_data Pointer to user data
 *
 * @todo Maybe should return a boolean to tell whether the menu was changed
 * somehow (for example, the item was removed, or a submenu was added etc).
 * Or, maybe that could even be done automatically.
 */
typedef void (*NavigationMenuFunc)
	(NavigationMenu *menu,
	 GtkTreePath *path,
	 gpointer user_data);

/**
 * @brief Create a new navigation menu item
 *
 * @param menu #NavigationMenu that the new item is added to
 * @param iter GtkTreeIter pointing to the newly created item
 * @param parent Parent iterator
 * @param text Text to be displayed
 * @param gfx_prefix Image file prefix. The images will be loaded from
 * states numbered from gfx_prefix + [0 to EC_BUTTON_STATE_COUNT - 1] and
 * gfx_prefix + icon
 * @param activated_callback Callback to be called when the item is clicked.
 * @param user_data Data pointer to be passed to the callback
 *
 * @todo Should the activated_callback be called even if the menu item has
 * sub-items? Could the menu be created dynamically (e.g., when showing old
 * items)? Think how it all should work.
 */
void navigation_menu_item_new_for_iter(
		NavigationMenu *menu,
		GtkTreeIter *iter,
		GtkTreeIter *parent,
		const gchar *text,
		const gchar *gfx_prefix,
		NavigationMenuFunc callback,
		gpointer user_data);

/**
 * @brief Create a new navigation menu item.
 *
 * This function behaves exactly like #navigation_menu_item_new_for_iter(),
 * except that this takes a GtkTreePath as the pointer to the parent item.
 *
 * @param menu #NavigationMenu that the new item is added to
 * @param parent Parent path
 * @param text Text to be displayed
 * @param gfx_prefix Image file prefix. The images will be loaded from
 * states numbered from gfx_prefix + "_" + [0 to EC_BUTTON_STATE_COUNT - 1]
 * + ".png" and gfx_prefix + "_icon.png"
 * @param activated_callback Callback to be called when the item is clicked.
 * @param user_data Data pointer to be passed to the callback
 * @param return_path Whether or not to return the GtkTreePath for the created
 * item.
 *
 * @return GtkTreePath that points to the created item. This must
 * be freed with gtk_tree_path_free. If you don't need this, set
 * return_path to FALSE when calling.
 *
 * @todo Should the activated_callback be called even if the menu item has
 * sub-items? Could the menu be created dynamically (e.g., when showing old
 * items)? Think how it all should work.
 */
GtkTreePath *navigation_menu_item_new_for_path(
		NavigationMenu *menu,
		GtkTreePath *parent,
		const gchar *text,
		const gchar *gfx_prefix,
		NavigationMenuFunc callback,
		gpointer user_data,
		gboolean return_path);

#endif /* _NAVIGATION_MENU_ITEM_H */
