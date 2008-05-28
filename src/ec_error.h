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

#ifndef _EC_ERROR_H
#define _EC_ERROR_H

/*****************************************************************************
 * Includes                                                                  *
 *****************************************************************************/

/* Gtk */
#include <gtk/gtkwindow.h>

/* Other modules */
#include "gconf_helper.h"

/*****************************************************************************
 * Definitions                                                               *
 *****************************************************************************/

#define EC_ERROR ec_error_quark()

/*****************************************************************************
 * Enumerations                                                              *
 *****************************************************************************/

typedef enum
{
	EC_ERROR_BLUETOOTH,
	EC_ERROR_HRM_NOT_CONFIGURED,
	EC_ERROR_PIPE,
	EC_ERROR_FILE,
	EC_ERROR_FILE_FORMAT,
	EC_ERROR_UNKNOWN
} EcError;

/*****************************************************************************
 * Function prototypes                                                       *
 *****************************************************************************/

GQuark ec_error_quark(void);

/**
 * @brief Set a window in order to be able to show modal error dialogs
 *
 * @param window Main window
 * @param gconf_helper Pointer to #GConfHelperData
 */
void ec_error_initialize(GtkWindow *window, GConfHelperData *gconf_helper);

/**
 * @brief Show an error message dialog
 *
 * @param message Error message to show
 */
void ec_error_show_message_error(const gchar *message);

/**
 * @brief Show an error message dialog using printf format for the error
 * message
 *
 * @param format Format of the message (printf style)
 * @param ... Parameters for the message
 */
void ec_error_show_message_error_printf(const gchar *format, ...)
	__attribute__((format(printf, 1, 2)));

/**
 * @brief Show an error message and ask whether to show the same message
 * again. If user has chosen not to show the error again, the error will
 * not be shown.
 *
 * @param format Message to show (in printf format)
 * @param gconf_key GConf key where to store the value whether or not
 * to show the message again
 * @param default_value Default value of whether to show the message again
 * or not (if TRUE, don't show again, if FALSE, show again by default)
 * @param ... Printf style parameter list for the message
 *
 * @todo This function is used also to show information messages etc.,
 * so this should be renamed and probably moved to util module
 */
void ec_error_show_message_error_ask_dont_show_again(
		const gchar *format,
		const gchar *gconf_key,
		gboolean default_value,
		...)
	__attribute__((format(printf, 1, 4)));

#endif /* _EC_ERROR_H */
