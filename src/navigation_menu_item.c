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

#include "navigation_menu_item.h"
#include "navigation_menu.h"

#include "debug.h"

void navigation_menu_item_new_for_iter(
		NavigationMenu *menu,
		GtkTreeIter *iter,
		GtkTreeIter *parent,
		const gchar *text,
		const gchar *gfx_prefix,
		NavigationMenuFunc callback,
		gpointer user_data)
{
	GdkPixbuf *icon_pixbuf = NULL;
	GdkPixbuf *bg_pixbuf[EC_BUTTON_STATE_COUNT] = { NULL };
	GError *error = NULL;
	gchar *pxb_path = NULL;
	gint i = 0;

	g_return_if_fail(menu != NULL);
	g_return_if_fail(text != NULL);
	g_return_if_fail(iter != NULL);

	DEBUG_BEGIN();

	if(gfx_prefix)
	{
#if 0
		This is if state-specific pixmaps come into use
		for(i = 0; i < EC_BUTTON_STATE_COUNT; i++)
		{
			pxb_path = g_strdup_printf("%s_%d.png",
					gfx_prefix,
					i + 1);

			bg_pixbuf[i] = gdk_pixbuf_new_from_file(pxb_path,
					&error);
			if(error)
			{
				g_warning("Unable to load image %s: %s",
						pxb_path,
						error->message);
				g_error_free(error);
				error = NULL;
			} else {
				gdk_pixbuf_ref(bg_pixbuf[i]);
			}
			g_free(pxb_path);
		}
#else
		pxb_path = g_strdup_printf("%s.png",
					gfx_prefix);

		bg_pixbuf[0] = gdk_pixbuf_new_from_file(pxb_path,
					&error);
		if(error)
		{
			g_warning("Unable to load image %s: %s",
					pxb_path,
					error->message);
			g_error_free(error);
			error = NULL;
		} else {
			for(i = 1; i < EC_BUTTON_STATE_COUNT; i++)
			{
				bg_pixbuf[i] = bg_pixbuf[0];
				gdk_pixbuf_ref(bg_pixbuf[i]);
			}
		}
		g_free(pxb_path);
#endif
#if 0
		/**
		 * @todo Enable this code if/when icons are enabled
		 * for navigatio menu
		 */
		pxb_path = g_strdup_printf("%s_icon.png", gfx_prefix);
		icon_pixbuf = gdk_pixbuf_new_from_file(pxb_path, &error);
		if(error)
		{
			g_warning("Unable to load image %s: %s",
					pxb_path,
					error->message);
			g_error_free(error);
			error = NULL;
		} else {
			gdk_pixbuf_ref(icon_pixbuf);
		}
		g_free(pxb_path);
#endif
	} else {
		/* Use default button image */
		for(i = 0; i < EC_BUTTON_STATE_COUNT; i++)
		{
			bg_pixbuf[i] = menu->default_button_bg;
		}
	}

	gtk_tree_store_append(menu->tree_store,
			iter,
			parent);

	gtk_tree_store_set(menu->tree_store,
			iter,
			NAVIGATION_MENU_COLUMN_TEXT, text,
			NAVIGATION_MENU_COLUMN_CALLBACK, callback,
			NAVIGATION_MENU_COLUMN_USER_DATA, user_data,
			NAVIGATION_MENU_FIXED_COLUMN_COUNT +
			EC_BUTTON_STATE_COUNT, icon_pixbuf,
			-1);

	for(i = 0; i < EC_BUTTON_STATE_COUNT; i++)
	{
		gtk_tree_store_set(menu->tree_store,
				iter,
				NAVIGATION_MENU_FIXED_COLUMN_COUNT + i,
				bg_pixbuf[i],
				-1);
	}

	/**
	 * @todo: Create logic to update the menu view
	 */

	DEBUG_END();
}

GtkTreePath *navigation_menu_item_new_for_path(
		NavigationMenu *menu,
		GtkTreePath *parent,
		const gchar *text,
		const gchar *gfx_prefix,
		NavigationMenuFunc callback,
		gpointer user_data,
		gboolean return_path)
{
	GtkTreeIter parent_iter;
	GtkTreeIter iter;
	GtkTreePath *path = NULL;

	g_return_val_if_fail(menu != NULL, NULL);
	g_return_val_if_fail(text != NULL, NULL);

	DEBUG_BEGIN();

	if(parent)
	{
		if(!gtk_tree_model_get_iter(
					GTK_TREE_MODEL(menu->tree_store),
					&parent_iter,
					parent))
		{
			g_critical("Given GtkTreePath not found");
			DEBUG_END();
			return NULL;
		}
		navigation_menu_item_new_for_iter(
				menu,
				&iter,
				&parent_iter,
				text,
				gfx_prefix,
				callback,
				user_data);
	} else {
		navigation_menu_item_new_for_iter(
				menu,
				&iter,
				NULL,
				text,
				gfx_prefix,
				callback,
				user_data);
	}

	if(return_path)
	{
		path = gtk_tree_model_get_path(
				GTK_TREE_MODEL(menu->tree_store),
				&iter);

		DEBUG_END();
		return path;
	} else {
		DEBUG_END();
		return NULL;
	}
}
