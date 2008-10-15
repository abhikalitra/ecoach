/*
 *  eCoach
 *
 *  Copyright (C) 2008  Kai Skiftesvik
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

#ifndef _CALCULATE_BMI_H
#define _CALCULATE_BMI_H


/*****************************************************************************
 * Includes                                                                  *
 *****************************************************************************/

/* Configuration */
#include "config.h"

/* Gtk */
#include <gtk/gtk.h>

/* Other modules */
#include "gconf_helper.h"
#include "navigation_menu.h"

/*****************************************************************************
 * Function prototypes                                                       *
 *****************************************************************************/

void calculate_bmi_dialog_show(
		NavigationMenu *menu,
		GtkTreePath *path,
		gpointer user_data);

#endif /* _CALCULATE_BMI_H */
