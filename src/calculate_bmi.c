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
#include "interface.h"
#include "gconf_helper.h"
#include "gconf_keys.h"
#include "hildon/hildon-button.h"
#include "hildon/hildon-picker-button.h"
#include "hildon/hildon-touch-selector.h"
#include "hildon/hildon-touch-selector-entry.h"

/* i18n */
#include <glib/gi18n.h>

static void get_value( GtkWidget *widget,
                       gpointer user_data )
{
      
	AppData *app_data = (AppData *)user_data;
	

	/*gint valh;
	gint valh2;
	gint valw;
	gdouble bmi;
	
*/
/*	valh = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinner));
	valwldon/hildon-touch-selector-entry.h"

static void get_value( GtkWidget *widget,
                       gpointer user_data )
{
      
	AppData *app_data = (AppData *)user_data;
	

	gint valh;
	gint valh2;
	gint valw;
	gdouble bmi;
	
*/
/*	valh = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinner));
	valw = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinner3)); = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinner3));
*/
	if (gconf_helper_get_value_bool_with_default(app_data->gconf_helper, 
							USE_METRIC, TRUE)) {

		/* Metric Units */
//		
		gdouble weight =0;
		gdouble height =0;
		app_data->bmidata->bmi =0;
		const gchar *wchoice = hildon_button_get_value(HILDON_BUTTON (app_data->bmidata->weight_picker));
		weight = CLAMP(g_ascii_strtod (wchoice, NULL), 0, 300);
		
		const gchar *hchoice = hildon_button_get_value(HILDON_BUTTON (app_data->bmidata->height_picker));
		height = CLAMP(g_ascii_strtod (hchoice, NULL), 0, 300);
		app_data->bmidata->bmi = (weight / ( height  * height ) * 10000);
  
	} else {

		/* English Units */
//		valh2 = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinner2));
//		bmi = ( (double)valw * 703 ) / ( ( (double)valh * 12 + (double)valh2 ) * 
//			( (double)valh * 12 + (double)valh2 ) );
	}

//	app_data->bmidata->bmi = 23;
	if (app_data->bmidata->bmi <= 16.5) { 
		app_data->bmidata->char_bmi = g_strdup_printf (_("Your BMI is %2.1f = severly underweight"), app_data->bmidata->bmi);
	} else if (app_data->bmidata->bmi > 16.5 && app_data->bmidata->bmi <= 18.5) {
		app_data->bmidata->char_bmi = g_strdup_printf (_("Your BMI is %2.1f = underweight"), app_data->bmidata->bmi);
	} else if (app_data->bmidata->bmi > 18.5 && app_data->bmidata->bmi <= 25) {
		app_data->bmidata->char_bmi = g_strdup_printf (_("Your BMI is %2.1f = normal"), app_data->bmidata->bmi);
	} else if (app_data->bmidata->bmi > 25 && app_data->bmidata->bmi <= 30) {
		app_data->bmidata->char_bmi = g_strdup_printf (_("Your BMI is %2.1f = overweight"), app_data->bmidata->bmi);
	} else if (app_data->bmidata->bmi > 30) {
		app_data->bmidata->char_bmi = g_strdup_printf (_("Your BMI is %gchar *char_bmi;2.1f = obese"), app_data->bmidata->bmi);
	}

	gtk_label_set_text (GTK_LABEL(app_data->bmidata->val_label), app_data->bmidata->char_bmi);
}


void show_calculate_bmi(NavigationMenu *menu, GtkTreePath *path,
			gpointer user_data) {
	
	AppData *app_data = (AppData *)user_data;
	GtkWidget *dialog;
	GtkWidget *table;
	GtkWidget *button;
	
	


	app_data->bmidata = g_new0(BMIData, 1);
	app_data->bmidata->height_picker = hildon_picker_button_new (HILDON_SIZE_AUTO_WIDTH | HILDON_SIZE_FINGER_HEIGHT,
                                HILDON_BUTTON_ARRANGEMENT_HORIZONTAL);

	app_data->bmidata->weight_picker = hildon_picker_button_new (HILDON_SIZE_AUTO_WIDTH | HILDON_SIZE_FINGER_HEIGHT,
                                HILDON_BUTTON_ARRANGEMENT_HORIZONTAL);
	

	hildon_button_set_title (HILDON_BUTTON (app_data->bmidata->height_picker), _("Height:"));

	hildon_button_set_title (HILDON_BUTTON (app_data->bmidata->weight_picker), _("Weight:"));

	HildonTouchSelector *height_selector = HILDON_TOUCH_SELECTOR (
                                hildon_touch_selector_entry_new_text ());
	int i = 100;
	for(;i< 240;i++){
	gchar *number;
	number = g_strdup_printf(_("%d cm"),i);
	hildon_touch_selector_append_text (height_selector, number);
	g_free(number);
	  }

	
	HildonTouchSelector *weight_selector = HILDON_TOUCH_SELECTOR (
                                hildon_touch_selector_entry_new_text ());
	int j = 0;
	for(;j< 300;j++){
	gchar *number;
	number = g_strdup_printf(_("%d kg"),j);
	hildon_touch_selector_append_text (weight_selector, number);
	g_free(number);
	  }





    hildon_picker_button_set_selector (HILDON_PICKER_BUTTON (app_data->bmidata->height_picker), height_selector);
    hildon_picker_button_set_active (HILDON_PICKER_BUTTON (app_data->bmidata->height_picker), 80);

    hildon_picker_button_set_selector (HILDON_PICKER_BUTTON (app_data->bmidata->weight_picker), weight_selector);
    hildon_picker_button_set_active (HILDON_PICKER_BUTTON (app_data->bmidata->weight_picker), 75);


  


	/* Create the widgets */


// for debugging only
// gconf_helper_set_value_bool(app_data->gconf_helper, USE_METRIC, FALSE);

//	dialog = gtk_dialog_new_with_buttons ("Calculate BMI",
			//			GTK_WINDOW(app_data->window),
			//			GTK_DIALOG_MODAL,
			//			NULL);
	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog),_("Calculate BMI"));



	gtk_widget_set_size_request(dialog, 800, 190);
	table = gtk_table_new (6, 3, TRUE);
	gtk_table_set_row_spacings(GTK_TABLE (table), 1);
	gtk_table_set_homogeneous (GTK_TABLE (table), FALSE);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),
			table);

	//label = gtk_label_new (_("Height:"));
	gtk_table_attach_defaults(GTK_TABLE (table), app_data->bmidata->height_picker, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE (table), app_data->bmidata->weight_picker, 1, 2, 0, 1);

	//label = gtk_label_new (_("Weight:"));
	//gtk_table_attach_defaults(GTK_TABLE (table), label, 0, 1, 1, 2);

	if (gconf_helper_get_value_bool_with_default(app_data->gconf_helper, 
							USE_METRIC, TRUE)) {

	/* Metric Units */

	//	adj = (GtkAdjustment *) gtk_adjustment_new (180.0, 100.0, 250.0, 1.0,
	//						    5.0, 0.0);

	//	label = gtk_label_new (_("cm"));
	//	gtk_table_attach_defaults(GTK_TABLE (table), label, 2, 3, 0, 1);

	//	adj3 = (GtkAdjustment *) gtk_adjustment_new (75.0, 40.0, 180.0, 1.0,
	//						    5.0, 0.0);

	//	label = gtk_label_new (_("kg"));
	//	gtk_table_attach_defaults(GTK_TABLE (table), label, 2, 3, 1, 2);

	} else {

	/* English Units */
      /*
		adj = (GtkAdjustment *) gtk_adjustment_new (5.0, 0.0, 10.0, 1.0,
						    5.0, 0.0);

		label = gtk_label_new (_("ft"));
		gtk_table_attach_defaults(GTK_TABLE (table), label, 2, 3, 0, 1);

		adj2 = (GtkAdjustment *) gtk_adjustment_new (11.0, 0.0, 11.0, 1.0,
						    5.0, 0.0);
		spinner2 = gtk_spin_button_new (adj2, 0, 0);
		gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner2), TRUE);
		gtk_table_attach_defaults(GTK_TABLE (table), spinner2, 3, 4, 0, 1);

		label = gtk_label_new (_("in"));
		gtk_table_attach_defaults(GTK_TABLE (table), label, 4, 5, 0, 1);
	
		adj3 = (GtkAdjustment *) gtk_adjustment_new (165.0, 90.0, 400.0, 1.0,
							5.0, 0.0);
	
		label = gtk_label_new (_("lb"));
		gtk_table_attach_defaults(GTK_TABLE (table), label, 2, 3, 1, 2);
	*/
	  }


//	spinner = gtk_spin_button_new (adj, 0, 0);
//	gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), TRUE);
//	gtk_table_attach_defaults(GTK_TABLE (table), spinner, 1, 2, 0, 1);

//	spinner3 = gtk_spin_button_new (adj3, 0, 0);
//	gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner3), TRUE);
//	gtk_table_attach_defaults(GTK_TABLE (table), spinner3, 1, 2, 1, 2);

	button = hildon_button_new (HILDON_SIZE_AUTO_WIDTH | HILDON_SIZE_FINGER_HEIGHT,
                                HILDON_BUTTON_ARRANGEMENT_VERTICAL);
	hildon_button_set_text (HILDON_BUTTON (button), _("Calculate"), NULL);


	g_signal_connect (G_OBJECT (button), "clicked",
			    G_CALLBACK (get_value),
			    (gpointer) app_data);
	gtk_table_attach_defaults(GTK_TABLE (table), button, 5, 6, 0, 2);

	app_data->bmidata->val_label = gtk_label_new ("");
	gtk_table_attach_defaults(GTK_TABLE (table), app_data->bmidata->val_label, 0, 6, 2, 3);


	
	/* Ensure that the dialog box is destroyed when the user responds. */
	
	g_signal_connect_swapped (dialog,
				"response", 
				G_CALLBACK (gtk_widget_destroy),
				dialog);
	
	gtk_widget_show_all (dialog);
	
	}
