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

#ifndef _NAVIGATION_MENU_H
#define _NAVIGATION_MENU_H

/* Configuration */
#include "config.h"

/* Gtk */
#include <gtk/gtk.h>

/* Other modules */
#include "navigation_menu_priv.h"
#include "navigation_menu_item.h"
#include "ec-button.h"

typedef enum _NavigationMenuColumns {
	NAVIGATION_MENU_COLUMN_TEXT = 0,
	NAVIGATION_MENU_COLUMN_CALLBACK,
	NAVIGATION_MENU_COLUMN_USER_DATA,
	NAVIGATION_MENU_FIXED_COLUMN_COUNT
} NavigationMenuColumns;

typedef struct _NavigationMenuButton
{
	GtkWidget *button;
	NavigationMenu *menu;
	NavigationMenuFunc callback;
	gpointer user_data;
	GtkTreePath *tree_path;
	GdkPixbuf *bg_pixbufs[EC_BUTTON_STATE_COUNT];
	GdkPixbuf *icon_pixbuf;
} NavigationMenuButton;

struct _NavigationMenu {
	GtkWidget *main_widget; /* GtkVBox */

	GtkWidget *notebook_main;
	gint menu_page_id;

	/* Layout management */
	GtkWidget *viewport_buttons;
	GtkWidget *alignment_buttons;
	GtkWidget *scrolled_buttons;
	GtkWidget *hbox_navigation;
	GtkWidget *grid_buttons;

	gint buttons_per_row;
	gint rows_per_display;

	/* Calculated from buttons_per_row and rows_per_display */
	gint button_width;
	gint button_height;

	/* Navigation bar widgets */
	GtkWidget *label_position;
	GtkWidget *btn_minimize;
	GtkWidget *btn_close;
	GtkWidget *btn_back;

	/* List of the buttons */
	GSList *list_buttons;

	/* Navigation is a tree-like structure */
	GtkTreeStore *tree_store;

	GtkTreePath *current_location;

	GdkPixbuf *navbar_bg;
	GdkPixbuf *default_button_bg;
	GdkPixbuf *btn_back_bg;
	GdkPixbuf *btn_back_icon;

	PangoFontDescription *font_description;
	PangoAlignment alignment;
};

NavigationMenu *navigation_menu_new();

gint navigation_menu_append_page(NavigationMenu *self, GtkWidget *widget);

void navigation_menu_set_current_page(NavigationMenu *self, gint page_num);

void navigation_menu_return_to_menu(NavigationMenu *self);

void navigation_menu_set_navigation_bar_background(
		NavigationMenu *self,
		const gchar *path);

void navigation_menu_set_default_button_background(
		NavigationMenu *self,
		const gchar *path);

void navigation_menu_set_back_button_gfx(
		NavigationMenu *self,
		const gchar *path_bg,
		const gchar *path_icon);

void navigation_menu_set_font_description(
		NavigationMenu *self,
		const PangoFontDescription *desc);

const PangoFontDescription *navigation_menu_get_font_description(
		NavigationMenu *menu);

void navigation_menu_set_alignment(NavigationMenu *menu,
		PangoAlignment alignment);

PangoAlignment navigation_menu_get_alignment(NavigationMenu *menu);

/**
 * @brief Update the menu
 * 
 * @param menu Pointer to #NavigationMenu
 */
void navigation_menu_refresh(NavigationMenu *menu);

#endif /* _NAVIGATION_MENU_H */
