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

static GtkSpinButton *spinner;
static GtkSpinButton *spinner2;
static GtkLabel *val_label;


static void get_value( GtkWidget *widget,
                       gpointer data )
{
	gint valh;
	gint valw;
	gdouble bmi;
	gchar *char_bmi;

	valh = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinner));
	valw = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinner2));
	bmi = (double)valw / ( (double)valh  * (double)valh ) * 10000;

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
			gpointer user_data)
{
	
	AppData *app_data = (AppData *)user_data;
	GtkWidget *dialog;
	GtkWidget *hbox;
	GtkWidget *main_vbox;
	GtkWidget *vbox;
	GtkWidget *vbox2;
	GtkWidget *button;
	GtkWidget *label;
	GtkAdjustment *adj;
	GtkAdjustment *adj2;
	
	/* Create the widgets */
	
	dialog = gtk_dialog_new_with_buttons ("Calculate BMI",
						GTK_WINDOW(app_data->window),
						GTK_DIALOG_MODAL,
						GTK_STOCK_OK,
						GTK_RESPONSE_NONE,
						NULL);

	gtk_widget_set_size_request(dialog, 460,200);

	main_vbox = gtk_vbox_new (FALSE, 5);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),
			main_vbox);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
	gtk_container_add (GTK_CONTAINER (main_vbox), vbox);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 5);

	vbox2 = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), vbox2, TRUE, TRUE, 5);

	label = gtk_label_new ("Height :");
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, TRUE, 0);

	label = gtk_label_new ("Weight :");
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, TRUE, 0);

	vbox2 = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), vbox2, TRUE, TRUE, 5);

	adj = (GtkAdjustment *) gtk_adjustment_new (180.0, 100.0, 250.0, 1.0,
						    5.0, 0.0);
	spinner = gtk_spin_button_new (adj, 0, 0);
	gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), TRUE);
	gtk_box_pack_start (GTK_BOX (vbox2), spinner, FALSE, TRUE, 0);

	adj2 = (GtkAdjustment *) gtk_adjustment_new (75.0, 40.0, 180.0, 1.0,
						    5.0, 0.0);
	spinner2 = gtk_spin_button_new (adj2, 0, 0);
	gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner2), TRUE);
	gtk_box_pack_start (GTK_BOX (vbox2), spinner2, FALSE, TRUE, 0);

	vbox2 = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), vbox2, TRUE, TRUE, 5);

	label = gtk_label_new ("cm");
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, TRUE, 0);

	label = gtk_label_new ("kg");
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, TRUE, 0);

	vbox2 = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), vbox2, TRUE, TRUE, 5);

	button = gtk_button_new_with_label ("Calculate");
	g_signal_connect (G_OBJECT (button), "clicked",
			    G_CALLBACK (get_value),
			    (gpointer) "");
	gtk_box_pack_start (GTK_BOX (vbox2), button, TRUE, TRUE, 5);
 
	val_label = gtk_label_new ("");
	gtk_box_pack_start (GTK_BOX (vbox), val_label, TRUE, TRUE, 0);
	gtk_label_set_text (GTK_LABEL (val_label), "");
	
	/* Ensure that the dialog box is destroyed when the user responds. */
	
	g_signal_connect_swapped (dialog,
				"response", 
				G_CALLBACK (gtk_widget_destroy),
				dialog);
	
	gtk_widget_show_all (dialog);
	
	}
