/*
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

    ---
    Copyright (C) 2008, Sampo Savola, Kai Skiftesvik 
*/

#include "calculate_bmi.h"

void show_calculate_bmi(NavigationMenu *menu, GtkTreePath *path,
			gpointer user_data)
{
	/*TODO
	Dialog for asking info and functions for calculating BMI
	
	*/
	AppData *app_data = (AppData *)user_data;
	GtkWidget *dialog = NULL;
	dialog = gtk_dialog_new_with_buttons(
					     _("Calculate BMI"),
					     GTK_WINDOW(app_data->window),
					     GTK_DIALOG_MODAL,
	  				     GTK_STOCK_CANCEL,
	    				     GTK_RESPONSE_REJECT,GTK_STOCK_OK,
					     GTK_RESPONSE_OK,NULL);
	gtk_widget_show_all(dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}
