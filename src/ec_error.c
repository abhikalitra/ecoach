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

/* This module */
#include "ec_error.h"

/* GLib */
#include <glib/gi18n.h>

/* Gtk */
#include <gtk/gtkbox.h>
#include <gtk/gtkcheckbutton.h>

/* Hildon */
#include <hildon/hildon-note.h>

/* Other modules */
#include "debug.h"
#include "gconf_helper.h"

static GtkWindow *ec_error_main_window = NULL;
static GConfHelperData *ec_error_gconf_helper = NULL;

GQuark ec_error_quark(void)
{
	return g_quark_from_static_string("ec-error-quark");
}

void ec_error_initialize(GtkWindow *window, GConfHelperData *gconf_helper)
{
	g_return_if_fail(window != NULL);
	g_return_if_fail(gconf_helper != NULL);
	ec_error_main_window = window;
	ec_error_gconf_helper = gconf_helper;
}

void ec_error_show_message_error(const gchar *message)
{
	GtkWidget *note = NULL;

	DEBUG_BEGIN();
	if(!ec_error_main_window)
	{
		g_warning("Main window not set for ec_error.c.\n");
	}

	note = hildon_note_new_information(ec_error_main_window,
			message);

	gtk_dialog_run(GTK_DIALOG(note));
	gtk_widget_destroy(note);

	DEBUG_END();
}

void ec_error_show_message_error_printf(const gchar *format, ...)
{
	gchar *message = NULL;
	va_list args;

	g_return_if_fail(format != NULL);
	DEBUG_BEGIN();

	va_start(args, format);
	message = g_strdup_vprintf(format, args);
	va_end(args);
	ec_error_show_message_error(message);

	g_free(message);
	DEBUG_END();
}

void ec_error_show_message_error_ask_dont_show_again(
		const gchar *format,
		const gchar *gconf_key,
		gboolean default_value,
		...)
{
	gchar *message = NULL;
	va_list args;
	GtkWidget *note = NULL;
	GtkWidget *chk_dont_show_again = NULL;
	gboolean dont_show_again;

	g_return_if_fail(format != NULL);
	g_return_if_fail(gconf_key != NULL);
	g_return_if_fail(ec_error_gconf_helper != NULL);

	DEBUG_BEGIN();

	if(ec_error_main_window == NULL)
	{
		g_warning("Main window not set for ec_error.c.\n");
	}

	dont_show_again = gconf_helper_get_value_bool_with_default(
			ec_error_gconf_helper,
			gconf_key,
			default_value);

	if(dont_show_again)
	{
		return;
	}

	/* Prepare the error message */
	va_start(args, default_value);
	message = g_strdup_vprintf(format, args);
	va_end(args);

	/* Create the dialog */
	note = hildon_note_new_information(ec_error_main_window,
			message);

	chk_dont_show_again = gtk_check_button_new_with_label(
			_("Do not show this message again"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chk_dont_show_again),
			default_value);
	gtk_box_pack_end(GTK_BOX(GTK_DIALOG(note)->vbox),
				chk_dont_show_again,
				FALSE, FALSE, 10);
	gtk_widget_show(chk_dont_show_again);

	gtk_dialog_run(GTK_DIALOG(note));

	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chk_dont_show_again))
			!= default_value)
	{
		gconf_helper_set_value_bool_simple(
				ec_error_gconf_helper,
				gconf_key,
				gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
						chk_dont_show_again)));
	}

	gtk_widget_destroy(note);

	DEBUG_END();
}

