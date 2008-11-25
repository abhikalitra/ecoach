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

#include "general_settings.h"
#include "gconf_helper.h"
#include "gconf_keys.h"

void show_general_settings(NavigationMenu *menu, GtkTreePath *path,
			gpointer user_data)
{
	
	AppData *app_data = (AppData *)user_data;
	GtkWidget *dialog;
	GtkWidget *table;
	GtkWidget *radio_units_metric;
	GtkWidget *radio_units_english;
	GtkWidget *radio_display_on;
	GtkWidget *radio_display_off;
	GtkWidget *label;
	GConfValue *value = NULL;
	gint result;

// for debugging only
// gconf_helper_set_value_bool(app_data->gconf_helper, DISPLAY_ON, FALSE);


	/* Create the widgets */
	
	dialog = gtk_dialog_new_with_buttons ("General Settings",
						GTK_WINDOW(app_data->window),
						GTK_DIALOG_MODAL,
						GTK_STOCK_OK,
						GTK_RESPONSE_OK,GTK_STOCK_CANCEL,
						GTK_RESPONSE_REJECT,NULL);

	gtk_widget_set_size_request(dialog, 300,240);

	table = gtk_table_new (4, 2, TRUE);
	gtk_table_set_homogeneous (GTK_TABLE (table), FALSE);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),
			table);

	label = gtk_label_new ("Units:");
	gtk_misc_set_alignment( GTK_MISC(label), 1.0, 0.5 );
	gtk_table_attach_defaults(GTK_TABLE (table), label, 0, 1, 0, 1);

	radio_units_metric = gtk_radio_button_new_with_label (NULL, "Metric");
	gtk_table_attach_defaults(GTK_TABLE (table), radio_units_metric, 1, 2, 0, 1);
	
	radio_units_english = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON 
							(radio_units_metric), "English");
	gtk_table_attach_defaults(GTK_TABLE (table), radio_units_english, 1, 2, 1, 2);

	/*Check which unit-radio button should be set */
	if(gconf_helper_get_value_bool_with_default(app_data->gconf_helper,
						    USE_METRIC,TRUE)) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_units_metric), TRUE);
	} else {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_units_english), TRUE);
	}

	label = gtk_label_new ("Display:");
	gtk_misc_set_alignment( GTK_MISC(label), 1.0, 0.5 );
	gtk_table_attach_defaults(GTK_TABLE (table), label, 0, 1, 2, 3);
	
	radio_display_on = gtk_radio_button_new_with_label (NULL, "On");
	gtk_table_attach_defaults(GTK_TABLE (table), radio_display_on, 1, 2, 2, 3);

	radio_display_off = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON 
							(radio_display_on), "Off");
	gtk_table_attach_defaults(GTK_TABLE (table), radio_display_off, 1, 2, 3, 4);

	/*Check which display dimm-radio button should be set */
	if(gconf_helper_get_value_bool_with_default(app_data->gconf_helper,
						    DISPLAY_ON,TRUE)) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_display_on), TRUE);
	} else {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_display_off), TRUE);
	}

	gtk_widget_show_all (dialog);
	result = gtk_dialog_run(GTK_DIALOG(dialog));
	if(result == GTK_RESPONSE_OK)
	{
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio_units_metric)))
		{
			gconf_helper_set_value_bool(app_data->gconf_helper,
						    USE_METRIC,TRUE);
		}
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio_units_english)))
		{
			gconf_helper_set_value_bool(app_data->gconf_helper,
					USE_METRIC,FALSE);
		}
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio_display_on)))
		{
			gconf_helper_set_value_bool(app_data->gconf_helper,
					DISPLAY_ON,TRUE);
		}
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio_display_off)))
		{
			gconf_helper_set_value_bool(app_data->gconf_helper,
					DISPLAY_ON,FALSE);
		}
		gtk_widget_destroy(dialog);
	}
	else
	{
		gtk_widget_destroy(dialog);	
	}


}