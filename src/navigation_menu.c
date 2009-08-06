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
#include "navigation_menu.h"

/* GLib */
#include <glib/gi18n.h>

/* Other modules */
#include "config.h"

#include "ec-button.h"
#include "ec_error.h"
#include "util.h"

#include "debug.h"

/*****************************************************************************
 * Definitions                                                               *
 *****************************************************************************/

#define GFXDIR DATADIR "/pixmaps/" PACKAGE_NAME "/"

/*****************************************************************************
 * Private function prototypes                                               *
 *****************************************************************************/

static gboolean navigation_menu_create_layout(NavigationMenu *self);
/* static void navigation_menu_create_navigation(NavigationMenu *self); */
static void navigation_menu_clear_current_level(NavigationMenu *self);

static NavigationMenuButton *navigation_menu_button_new(
		NavigationMenu *menu,
		GtkTreeIter *iter,
		const gchar *text,
		NavigationMenuFunc callback,
		gpointer user_data);

static void navigation_menu_button_destroy(NavigationMenuButton *button);

static void navigation_menu_button_clicked(
		GtkWidget *widget,
		NavigationMenuButton *button);

static void navigation_menu_up(GtkButton *button, gpointer user_data);

static void navigation_menu_update_location_widget(NavigationMenu *self);

static gboolean navigation_menu_navbar_expose_event(GtkWidget *widget,
		GdkEventExpose *event,
		NavigationMenu *menu);

static NavigationMenuButton *navigation_menu_button_new(
		NavigationMenu *menu,
		GtkTreeIter *iter,
		const gchar *text,
		NavigationMenuFunc callback,
		gpointer user_data)
{
	NavigationMenuButton *button = NULL;

	g_return_val_if_fail(menu != NULL, NULL);
	g_return_val_if_fail(text != NULL, NULL);
	g_return_val_if_fail(iter != NULL, NULL);

	DEBUG_BEGIN();

	button = g_new0(NavigationMenuButton, 1);
	if(!button)
	{
		g_critical("Not enough memory");
		return NULL;
	}

	button->menu = menu;

	button->button = ec_button_new();
	if(!button->button)
	{
		g_free(button);
		g_critical("Not enough memory");
		return NULL;
	}
	ec_button_set_label_text(EC_BUTTON(button->button), text);

	ec_button_set_btn_down_offset(EC_BUTTON(button->button), 2);

	if(menu->font_description)
	{
		ec_button_set_font_description_label(
				EC_BUTTON(button->button),
				menu->font_description);
	}
	ec_button_set_alignment(EC_BUTTON(button->button), menu->alignment);

#if 0
	/* Make the button word-wrap */
	for(child_widgets = gtk_container_get_children(GTK_CONTAINER(button->button));
			child_widgets;
			child_widgets = g_list_next(child_widgets))
	{
		if(GTK_IS_LABEL(child_widgets->data))
		{
			label = GTK_LABEL(child_widgets->data);
			gtk_label_set_line_wrap(label, TRUE);
			gtk_label_set_justify(label, GTK_JUSTIFY_CENTER);
			gtk_widget_set_size_request(GTK_WIDGET(label),
					200, 100);
		}
	}
	g_list_free(child_widgets);
#endif

	button->user_data = user_data;
	button->callback = callback;

	button->tree_path = gtk_tree_model_get_path(
			GTK_TREE_MODEL(menu->tree_store),
			iter);

	return button;
}

static void navigation_menu_button_destroy(NavigationMenuButton *button)
{
	gtk_widget_destroy(button->button);
	gtk_tree_path_free(button->tree_path);
	g_free(button);
}

static void navigation_menu_button_clicked(
		GtkWidget *widget,
		NavigationMenuButton *button)
{
	GtkTreeIter iter;

	DEBUG_BEGIN();

	/* First, invoke the callback, if there is any */
	if(button->callback)
	{
		button->callback(button->menu,
				button->tree_path,
				button->user_data);
	}

	/* Then check if there is a submenu */
	if(!gtk_tree_model_get_iter(
				GTK_TREE_MODEL(button->menu->tree_store),
				&iter,
				button->tree_path))
	{
		g_critical("Invalid GtkTreePath in NavigationMenuButton");
		DEBUG_END();
		return;
	}

	if(gtk_tree_model_iter_has_child(
				GTK_TREE_MODEL(button->menu->tree_store),
				&iter))
	{
		DEBUG("There is a submenu");
		if(button->menu->current_location)
		{
			gtk_tree_path_free(button->menu->current_location);
		}
		button->menu->current_location = gtk_tree_path_copy(
				button->tree_path);

		navigation_menu_refresh(button->menu);
	} else if(!button->callback) {
		/* There is no submenu nor a callback. This is an
		 * unimplemented feature. */
		ec_error_show_message_error(_("This feature is not yet "
					"implemented."));
	}

	DEBUG_END();
}

NavigationMenu *navigation_menu_new()
{
	NavigationMenu *self = NULL;

	DEBUG_BEGIN();

	self = g_new0(NavigationMenu, 1);
	if(!self)
	{
		g_critical("Not enough memory");
		DEBUG_END();
		return NULL;
	}
	/**
	 * @todo This needs to be updated every time when the state count
	 * of the EcButton changes. Possibly here should be used
	 * gtk_tree_store_newv function.
	 */
	self->tree_store = gtk_tree_store_new(
			6,
			G_TYPE_STRING,
			G_TYPE_POINTER,
			G_TYPE_POINTER,
			GDK_TYPE_PIXBUF,
			GDK_TYPE_PIXBUF,
			GDK_TYPE_PIXBUF);

	if(!self->tree_store)
	{
		g_free(self);
		g_critical("Not enough memory");
		DEBUG_END();
		return NULL;
	}

	self->buttons_per_row = 4;
	self->rows_per_display = 2;

	/* These are just rough numbers, and will be calculated
	 * when the size of the widget is known
	 */
	self->button_width = 200;
	self->button_height = 205;

	if(!navigation_menu_create_layout(self))
	{
		g_free(self);
		g_critical("Layout creation failed");
		DEBUG_END();
		return NULL;
	}

	/*self->current_location = gtk_tree_path_new_from_indices(0, -1);
	gchar *string = gtk_tree_path_to_string(self->current_location);
	printf("Tree path is: %s\n", string);*/

	/* navigation_menu_create_navigation(self); */

	DEBUG_END();
	return self;
}

gint navigation_menu_append_page(NavigationMenu *self, GtkWidget *widget)
{
	gint retval;

	g_return_val_if_fail(self != NULL, -1);
	g_return_val_if_fail(widget != NULL, -1);

	DEBUG_BEGIN();

	retval = gtk_notebook_append_page(
			GTK_NOTEBOOK(self->notebook_main),
			widget,
			NULL);

	DEBUG_END();
	return retval;
}

void navigation_menu_set_current_page(NavigationMenu *self, gint page_num)
{
	g_return_if_fail(self != NULL);

	DEBUG_BEGIN();

	gtk_notebook_set_current_page(
			GTK_NOTEBOOK(self->notebook_main),
			page_num);

	DEBUG_END();
}

void navigation_menu_return_to_menu(NavigationMenu *self)
{
	g_return_if_fail(self != NULL);

	DEBUG_BEGIN();

	navigation_menu_set_current_page(self, self->menu_page_id);

	DEBUG_END();
}

void navigation_menu_set_navigation_bar_background(
		NavigationMenu *self,
		const gchar *path)
{
	GdkPixbuf *new_pixbuf = NULL;
	GError *error = NULL;

	g_return_if_fail(self != NULL);
	g_return_if_fail(path != NULL);

	DEBUG_BEGIN();

	if(self->navbar_bg)
	{
		gdk_pixbuf_unref(self->navbar_bg);
	}
	if(path)
	{
		new_pixbuf = gdk_pixbuf_new_from_file(path, &error);
		if(error)
		{
			g_warning("Unable to load image %s: %s", path,
					error->message);
			g_error_free(error);
		} else {
			gdk_pixbuf_ref(new_pixbuf);
		}
	}
	self->navbar_bg = new_pixbuf;

	gtk_widget_set_size_request(self->hbox_navigation,
			-1,
			gdk_pixbuf_get_height(new_pixbuf));

	gtk_widget_queue_draw(GTK_WIDGET(self->hbox_navigation));

	DEBUG_END();
}

void navigation_menu_set_default_button_background(
		NavigationMenu *self,
		const gchar *path)
{
	GError *error = NULL;
	GdkPixbuf *new_pixbuf = NULL;

	g_return_if_fail(self != NULL);
	g_return_if_fail(path != NULL);

	if(self->default_button_bg)
	{
		gdk_pixbuf_unref(self->default_button_bg);
	}

	if(path)
	{
		new_pixbuf = gdk_pixbuf_new_from_file(path, &error);
		if(error)
		{
			g_warning("Unable to load image %s: %s", path,
					error->message);
			g_error_free(error);
		} else {
			gdk_pixbuf_ref(new_pixbuf);
		}
	}
	self->default_button_bg = new_pixbuf;
}

void navigation_menu_set_back_button_gfx(
		NavigationMenu *self,
		const gchar *path_bg,
		const gchar *path_icon)
{
	GError *error = NULL;
	GdkPixbuf *new_pixbuf;

	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	if(path_bg)
	{
		new_pixbuf = gdk_pixbuf_new_from_file(path_bg, &error);
		if(error)
		{
			g_warning("Unable to load image %s: %s",
					path_bg,
					error->message);
			g_error_free(error);
			error = NULL;
		} else {
			if(self->btn_back_bg)
			{
				gdk_pixbuf_unref(self->btn_back_bg);
			}
			self->btn_back_bg = new_pixbuf;
			gdk_pixbuf_ref(self->btn_back_bg);
		}
	}

	if(path_icon)
	{
		new_pixbuf = gdk_pixbuf_new_from_file(path_icon, &error);
		if(error)
		{
			g_warning("Unable to load image %s: %s",
					path_icon,
					error->message);
			g_error_free(error);
			error = NULL;
		} else {
			if(self->btn_back_icon)
			{
				gdk_pixbuf_unref(self->btn_back_icon);
			}
			self->btn_back_icon = new_pixbuf;
			gdk_pixbuf_ref(self->btn_back_icon);
		}
	}
	DEBUG_END();
}

void navigation_menu_set_font_description(
		NavigationMenu *self,
		const PangoFontDescription *desc)
{
	g_return_if_fail(self != NULL);

	DEBUG_BEGIN();

	if(self->font_description)
	{
		pango_font_description_free(self->font_description);
	}

	if(desc)
	{
		self->font_description = pango_font_description_copy(desc);
	} else {
		self->font_description = NULL;
	}

	DEBUG_END();
}

const PangoFontDescription *navigation_menu_get_font_description(
		NavigationMenu *menu)
{
	g_return_val_if_fail(menu != NULL, NULL);

	DEBUG_BEGIN();

	DEBUG_END();
	return menu->font_description;
}

void navigation_menu_set_alignment(NavigationMenu *menu,
		PangoAlignment alignment)
{
	g_return_if_fail(menu != NULL);

	DEBUG_BEGIN();

	menu->alignment = alignment;

	DEBUG_END();
}

PangoAlignment navigation_menu_get_alignment(NavigationMenu *menu)
{
	g_return_val_if_fail(menu != NULL, PANGO_ALIGN_LEFT);

	DEBUG_BEGIN();

	DEBUG_END();
	return menu->alignment;
}

static gboolean navigation_menu_create_layout(
		NavigationMenu *self)
{
	GtkWidget *spacer;

	DEBUG_BEGIN();

	self->main_widget = gtk_vbox_new(FALSE, 0);
	if(!self->main_widget)
	{
		goto layout_creation_failed;
	}
	gtk_widget_set_name(self->main_widget, "navmenu_main");

	/* Create the navigation bar */
	self->hbox_navigation = gtk_hbox_new(FALSE, 0);
	if(!self->hbox_navigation)
	{
		goto layout_creation_failed;
	}
	gtk_widget_set_name(self->hbox_navigation, "navmenu_navbar");

	g_signal_connect(G_OBJECT(self->hbox_navigation), "expose_event",
				G_CALLBACK(navigation_menu_navbar_expose_event),
				self);

	gtk_box_pack_start(GTK_BOX(self->main_widget),
			self->hbox_navigation,
			FALSE, FALSE, 0);

	self->label_position = gtk_label_new(_("Main menu"));
	if(!self->label_position)
	{
		goto layout_creation_failed;
	}
	gtk_widget_set_name(self->label_position, "navmenu_lbllocation");

	gtk_misc_set_alignment(GTK_MISC(self->label_position),
			0, 0.5);

	gtk_box_pack_start(GTK_BOX(self->hbox_navigation),
			self->label_position,
			FALSE, FALSE, 10);

	/* Create the spacer */
	spacer = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(self->hbox_navigation),
			spacer,
			TRUE, TRUE, 0);

	/* Create the minimize button */
	self->btn_minimize = ec_button_new();
	ec_button_set_bg_image(EC_BUTTON(self->btn_minimize),
			EC_BUTTON_STATE_RELEASED,
			GFXDIR "ec_titlebar_minimize_btn.png");

	ec_button_set_bg_image(EC_BUTTON(self->btn_minimize),
			EC_BUTTON_STATE_DOWN,
			GFXDIR "ec_titlebar_minimize_btn.png");

	ec_button_set_center_vertically(EC_BUTTON(self->btn_minimize), TRUE);

	ec_button_set_btn_down_offset(EC_BUTTON(self->btn_minimize), 2);

	gtk_widget_set_size_request(self->btn_minimize, 50, 50);

	//gtk_box_pack_start(GTK_BOX(self->hbox_navigation),
	//		self->btn_minimize, FALSE, FALSE, 0);

	/* Create the close button */
	self->btn_close = ec_button_new();
	ec_button_set_bg_image(EC_BUTTON(self->btn_close),
			EC_BUTTON_STATE_RELEASED,
			GFXDIR "ec_titlebar_close_btn.png");

	ec_button_set_bg_image(EC_BUTTON(self->btn_close),
			EC_BUTTON_STATE_DOWN,
			GFXDIR "ec_titlebar_close_btn.png");

	ec_button_set_center_vertically(EC_BUTTON(self->btn_close), TRUE);

	ec_button_set_btn_down_offset(EC_BUTTON(self->btn_close), 2);

	gtk_widget_set_size_request(self->btn_close, 50, 50);

//	gtk_box_pack_start(GTK_BOX(self->hbox_navigation),
//			self->btn_close, FALSE, FALSE, 0);

	/* Create the main notebook */
	self->notebook_main = gtk_notebook_new();
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(self->notebook_main), FALSE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(self->notebook_main), FALSE);
	gtk_box_pack_start(GTK_BOX(self->main_widget),
			self->notebook_main, TRUE, TRUE, 0);

	self->alignment_buttons = gtk_alignment_new(.5, .5, 0, 0);

	/* Create the button grid */
	self->scrolled_buttons = gtk_scrolled_window_new(NULL, NULL);
	if(!self->scrolled_buttons)
	{
		goto layout_creation_failed;
	}

	gtk_container_add(GTK_CONTAINER(self->alignment_buttons),
			self->scrolled_buttons);

	/*
	self->menu_page_id = navigation_menu_append_page(self,
			self->scrolled_buttons);
			*/
	self->menu_page_id = navigation_menu_append_page(self,
			self->alignment_buttons);

	/* The scroll bars are controlled manually */
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(
				self->scrolled_buttons),
			GTK_POLICY_NEVER,
			GTK_POLICY_NEVER);

	self->grid_buttons = gtk_fixed_new();
	if(!self->grid_buttons)
	{
		goto layout_creation_failed;
	}
	gtk_scrolled_window_add_with_viewport(
			GTK_SCROLLED_WINDOW(self->scrolled_buttons),
			self->grid_buttons);


	DEBUG_END();
	return TRUE;

layout_creation_failed:
	gtk_widget_destroy(self->main_widget);
	g_critical("Not enough memory");
	DEBUG_END();
	return FALSE;
}

void navigation_menu_refresh(NavigationMenu *self)
{
	GtkTreeIter iter;
	GtkTreeIter child;

	GdkPixbuf *bg_pixbuf[EC_BUTTON_STATE_COUNT] = { NULL };
	GdkPixbuf *icon_pixbuf = NULL;

	gchar *text = NULL;
	NavigationMenuButton *button = NULL;
	NavigationMenuFunc callback = NULL;
	gpointer user_data = NULL;

	gint x = 0;
	gint y = 0;
	gint i = 0;
	gint j = 0;

	g_return_if_fail(self != NULL);

	DEBUG_BEGIN();

	navigation_menu_clear_current_level(self);

	if(self->current_location)
	{
		gtk_tree_model_get_iter(GTK_TREE_MODEL(self->tree_store),
				&iter,
				self->current_location);
		if(!gtk_tree_model_iter_children(GTK_TREE_MODEL(self->tree_store),
					&child,
					&iter))
		{
			/* There are no subitems, and we should not be here */
			g_warning("Current location points to an empty menu");
			navigation_menu_up(NULL, self);
			return;
		}
	} else {
		gtk_tree_model_get_iter_first(GTK_TREE_MODEL(self->tree_store),
			       &child);
	}

	do {
		gtk_tree_model_get(GTK_TREE_MODEL(self->tree_store),
				&child,
				NAVIGATION_MENU_COLUMN_TEXT, &text,
				NAVIGATION_MENU_COLUMN_CALLBACK, &callback,
				NAVIGATION_MENU_COLUMN_USER_DATA, &user_data,
				NAVIGATION_MENU_FIXED_COLUMN_COUNT +
				EC_BUTTON_STATE_COUNT, &icon_pixbuf,
				-1);

		button = navigation_menu_button_new(
				self,
				&child,
				text,
				callback,
				user_data);

		ec_button_set_icon_pixbuf(EC_BUTTON(button->button),
				icon_pixbuf);

		for(j = 0; j < EC_BUTTON_STATE_COUNT; j++)
		{
			gtk_tree_model_get(GTK_TREE_MODEL(self->tree_store),
					&child,
					NAVIGATION_MENU_FIXED_COLUMN_COUNT + j,
					&bg_pixbuf[j],
					-1);
			ec_button_set_bg_image_pixbuf(
					EC_BUTTON(button->button),
					j,
					bg_pixbuf[j]);
		}

		gtk_widget_set_size_request(button->button,
				self->button_width,
				self->button_height);

		x = (i % self->buttons_per_row) * self->button_width;
		y = (i / self->buttons_per_row) * (self->button_height + 10)
			+ 10;

		i++;

		self->list_buttons = g_slist_append(self->list_buttons, button);

		gtk_fixed_put(GTK_FIXED(self->grid_buttons),
				button->button,
				x, y);

		g_signal_connect(G_OBJECT(button->button),
				"clicked",
				G_CALLBACK(navigation_menu_button_clicked),
				button);

	} while(gtk_tree_model_iter_next(GTK_TREE_MODEL(self->tree_store),
				&child));

	if(self->current_location)
	{
		/* Create back button */
		button = navigation_menu_button_new(
				self,
				&child,
				_("Back"),
				NULL,
				NULL);

		if(self->btn_back_bg)
		{
			ec_button_set_bg_image_pixbuf(EC_BUTTON(
						button->button),
					EC_BUTTON_STATE_RELEASED,
					self->btn_back_bg);
		} else {
			ec_button_set_bg_image_pixbuf(EC_BUTTON(
						button->button),
					EC_BUTTON_STATE_RELEASED,
					self->default_button_bg);
		}

		if(self->btn_back_icon)
		{
			ec_button_set_icon_pixbuf(EC_BUTTON(button->button),
					self->btn_back_icon);
		}
	
		gtk_widget_set_size_request(button->button,
				self->button_width,
				self->button_height);

		x = (i % self->buttons_per_row) * self->button_width;
		y = (i / self->buttons_per_row) * (self->button_height + 10)
			+ 10;

		i++;

		self->list_buttons = g_slist_append(self->list_buttons, button);

		gtk_fixed_put(GTK_FIXED(self->grid_buttons),
				button->button,
				x, y);

		g_signal_connect(G_OBJECT(button->button),
				"clicked",
				G_CALLBACK(navigation_menu_up),
				self);
	}

	if(i > self->buttons_per_row * self->rows_per_display)
	{
		/* A scroll bar will be needed */
		gtk_scrolled_window_set_policy(
				GTK_SCROLLED_WINDOW(self->scrolled_buttons),
				GTK_POLICY_NEVER,
				GTK_POLICY_ALWAYS);
		gtk_alignment_set(GTK_ALIGNMENT(self->alignment_buttons),
				0.5, 0.5,
				1, 1);
	} else {
		gtk_scrolled_window_set_policy(
				GTK_SCROLLED_WINDOW(self->scrolled_buttons),
				GTK_POLICY_NEVER,
				GTK_POLICY_NEVER);
		gtk_alignment_set(GTK_ALIGNMENT(self->alignment_buttons),
				0.5, 0.5,
				0, 0);
	}

	gtk_widget_show_all(self->grid_buttons);

	navigation_menu_update_location_widget(self);

	DEBUG_END();
}

static void navigation_menu_update_location_widget(NavigationMenu *self)
{
	GSList *stack = NULL;
	GSList *temp = NULL;
	GtkTreePath *path = NULL;
	GtkTreeIter iter;
	GString *buffer = NULL;
	gchar *text;

	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	if(self->current_location)
	{
		path = gtk_tree_path_copy(self->current_location);
	}

	do {
		if(!path)
		{
			//stack = g_slist_prepend(stack,
			//		g_strdup(_("eCoach")));
			break;
		}
		gtk_tree_model_get_iter(
				GTK_TREE_MODEL(self->tree_store),
				&iter,
				path);

		gtk_tree_model_get(
				GTK_TREE_MODEL(self->tree_store),
				&iter,
				NAVIGATION_MENU_COLUMN_TEXT, &text,
				-1);

		util_replace_chars_with_char(text, '\n', ' ');

		stack = g_slist_prepend(stack, text);

		if(!gtk_tree_path_up(path))
		{
			gtk_tree_path_free(path);
			path = NULL;
		}
		if(gtk_tree_path_get_depth(path) == 0)
		{
			gtk_tree_path_free(path);
			path = NULL;
		}
	} while(1); /* Exit with break */

	buffer = g_string_new("");
	for(temp = stack; temp; temp = g_slist_next(temp))
	{
		text = (gchar *)temp->data;
		if(g_slist_next(temp))
		{
			g_string_append_printf(
					buffer, "%s > ", text);
		} else {
			g_string_append_printf(
					buffer, "%s", text);
		}
		g_free(text);
	}

	gtk_label_set_text(GTK_LABEL(self->label_position),
			buffer->str);

	g_string_free(buffer, TRUE);
	g_slist_free(stack);
}

static void navigation_menu_clear_current_level(NavigationMenu *self)
{
	NavigationMenuButton *button = NULL;

	g_return_if_fail(self != NULL);

	DEBUG_BEGIN();

	GSList *current;
	for(current = self->list_buttons; current;
			current = g_slist_next(current))
	{
		button = (NavigationMenuButton *)(current->data);
		navigation_menu_button_destroy(button);
	}

	g_slist_free(self->list_buttons);
	self->list_buttons = NULL;

	DEBUG_END();
}

static void navigation_menu_up(GtkButton *button, gpointer user_data)
{
	NavigationMenu *self = NULL;

	g_return_if_fail(user_data != NULL);

	DEBUG_BEGIN();

	self = (NavigationMenu *)user_data;

	if(!self->current_location)
	{
		g_critical("Already at topmost level");
		return;
	}

	if(!gtk_tree_path_up(self->current_location))
	{
		/* This is a top-level item in the tree.
		 * Make current_location point to NULL so
		 * that we can browse root menu
		 */
		gtk_tree_path_free(self->current_location);
		self->current_location = NULL;
	}

	if(gtk_tree_path_get_depth(self->current_location) == 0)
	{
		gtk_tree_path_free(self->current_location);
		self->current_location = NULL;
	}

	navigation_menu_refresh(self);
}

static gboolean navigation_menu_navbar_expose_event(GtkWidget *widget,
		GdkEventExpose *event,
		NavigationMenu *self)
{
	GdkDrawable *drawable = NULL;
	GdkGC *gc = NULL;

	g_return_val_if_fail(widget != NULL, FALSE);
	g_return_val_if_fail(event != NULL, FALSE);
	g_return_val_if_fail(self != NULL, FALSE);

	if(!self->navbar_bg)
	{
		return FALSE;
	}

	drawable = GDK_DRAWABLE(widget->window);
	gc = gdk_gc_new(drawable);
	
	gdk_draw_pixbuf(drawable,
			gc,
			self->navbar_bg,
			0, 0,
			0, 0,
			widget->allocation.width,
			widget->allocation.height,
			GDK_RGB_DITHER_NONE,
			0, 0);

	gdk_gc_unref(gc);

	return FALSE;
}
