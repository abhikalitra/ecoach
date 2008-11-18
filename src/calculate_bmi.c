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
#include "gconf_helper.h"
#include "gconf_keys.h"


static GtkSpinButton *spinner;
static GtkSpinButton *spinner2;
static GtkSpinButton *spinner3;
static GtkLabel *val_label;


static void get_value( GtkWidget *widget,
                       gpointer user_data )
{
	AppData *app_data = (AppData *)user_data;
	gint valh;
	gint valh2;
	gint valw;
	gdouble bmi;
	gchar *char_bmi;

	valh = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinner));
	valw = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinner3));

	if (gconf_helper_get_value_bool_with_default(app_data->gconf_helper, 
							USE_METRIC, TRUE)) {

		/* Metric Units */
		bmi = (double)valw / ( (double)valh  * (double)valh ) * 10000;

	} else {

		/* English Units */
		valh2 = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinner2));
		bmi = ( (double)valw * 703 ) / ( ( (double)valh * 12 + (double)valh2 ) * 
			( (double)valh * 12 + (double)valh2 ) );
	}

	if (bmi <= 16.5) { 
		char_bmi = g_strdup_printf ("Your BMI is %2.1f = severly underweight", bmi);
	} else if (bmi > 16.5 && bmi <= 18.5) {
		char_bmi = g_strdup_printf ("Your BMI is %2.1f = underweight", bmi);
	} else if (bmi > 18.5 && bmi <= 25) {
		char_bmi = g_strdup_printf ("Your BMI is %2.1f = normal", bmi);
	} else if (bmi > 25 && bmi <= 30) {
		char_bmi = g_strdup_printf ("Your BMI is %2.1f = overweight", bmi);
	} else if (bmi > 30) {
		char_bmi = g_strdup_printf ("Your BMI is %2.1f = obese", bmi);
	}

	gtk_label_set_text (val_label, char_bmi);

}


void show_calculate_bmi(NavigationMenu *menu, GtkTreePath *path,
			gpointer user_data) {
	
	AppData *app_data = (AppData *)user_data;
	GtkWidget *dialog;
	GtkWidget *table;
	GtkWidget *button;
	GtkWidget *label;
	GtkAdjustment *adj;
	GtkAdjustment *adj2;
	GtkAdjustment *adj3;
	
	/* Create the widgets */


// for debugging only
// gconf_helper_set_value_bool(app_data->gconf_helper, USE_METRIC, FALSE);

	dialog = gtk_dialog_new_with_buttons ("Calculate BMI",
						GTK_WINDOW(app_data->window),
						GTK_DIALOG_MODAL,
						GTK_STOCK_OK,
						GTK_RESPONSE_NONE,
						NULL);

	gtk_widget_set_size_request(dialog, 500, 190);
	table = gtk_table_new (6, 3, TRUE);
	gtk_table_set_row_spacings(GTK_TABLE (table), 1);
	gtk_table_set_homogeneous (GTK_TABLE (table), FALSE);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),
			table);

	label = gtk_label_new ("Height:");
	gtk_table_attach_defaults(GTK_TABLE (table), label, 0, 1, 0, 1);

	label = gtk_label_new ("Weight:");
	gtk_table_attach_defaults(GTK_TABLE (table), label, 0, 1, 1, 2);

	if (gconf_helper_get_value_bool_with_default(app_data->gconf_helper, 
							USE_METRIC, TRUE)) {

	/* Metric Units */

		adj = (GtkAdjustment *) gtk_adjustment_new (180.0, 100.0, 250.0, 1.0,
							    5.0, 0.0);

		label = gtk_label_new ("cm");
		gtk_table_attach_defaults(GTK_TABLE (table), label, 2, 3, 0, 1);

		adj3 = (GtkAdjustment *) gtk_adjustment_new (75.0, 40.0, 180.0, 1.0,
							    5.0, 0.0);

		label = gtk_label_new ("kg");
		gtk_table_attach_defaults(GTK_TABLE (table), label, 2, 3, 1, 2);

	} else {

	/* English Units */

		adj = (GtkAdjustment *) gtk_adjustment_new (5.0, 0.0, 10.0, 1.0,
						    5.0, 0.0);

		label = gtk_label_new ("ft");
		gtk_table_attach_defaults(GTK_TABLE (table), label, 2, 3, 0, 1);

		adj2 = (GtkAdjustment *) gtk_adjustment_new (11.0, 0.0, 11.0, 1.0,
						    5.0, 0.0);
		spinner2 = gtk_spin_button_new (adj2, 0, 0);
		gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner2), TRUE);
		gtk_table_attach_defaults(GTK_TABLE (table), spinner2, 3, 4, 0, 1);

		label = gtk_label_new ("in");
		gtk_table_attach_defaults(GTK_TABLE (table), label, 4, 5, 0, 1);
	
		adj3 = (GtkAdjustment *) gtk_adjustment_new (165.0, 90.0, 400.0, 1.0,
							5.0, 0.0);
	
		label = gtk_label_new ("lb");
		gtk_table_attach_defaults(GTK_TABLE (table), label, 2, 3, 1, 2);
	}


	spinner = gtk_spin_button_new (adj, 0, 0);
	gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), TRUE);
	gtk_table_attach_defaults(GTK_TABLE (table), spinner, 1, 2, 0, 1);

	spinner3 = gtk_spin_button_new (adj3, 0, 0);
	gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner3), TRUE);
	gtk_table_attach_defaults(GTK_TABLE (table), spinner3, 1, 2, 1, 2);

	button = gtk_button_new_with_label ("Calculate");
	g_signal_connect (G_OBJECT (button), "clicked",
			    G_CALLBACK (get_value),
			    (gpointer) app_data);
	gtk_table_attach_defaults(GTK_TABLE (table), button, 5, 6, 0, 2);

	val_label = gtk_label_new ("");
	gtk_table_attach_defaults(GTK_TABLE (table), val_label, 0, 6, 2, 3);


	
	/* Ensure that the dialog box is destroyed when the user responds. */
	
	g_signal_connect_swapped (dialog,
				"response", 
				G_CALLBACK (gtk_widget_destroy),
				dialog);
	
	gtk_widget_show_all (dialog);
	
	}
