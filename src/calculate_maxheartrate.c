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

#include "calculate_maxheartrate.h"

static GtkSpinButton *spinner;
static GtkLabel *val_label;


static void get_value( GtkWidget *widget,
                       gpointer data )
{
	gint vala;
	gdouble maxheartrate;
	gchar *char_maxheartrate;

	vala = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinner));

	maxheartrate = 205.8 - (0.685 * (double)vala);

	char_maxheartrate = g_strdup_printf ("Your maximum heart rate is %2.0f", maxheartrate);

	gtk_label_set_text (val_label, char_maxheartrate);
}

void show_calculate_maxheartrate(NavigationMenu *menu, GtkTreePath *path,
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
	
	dialog = gtk_dialog_new_with_buttons ("Calculate Maximum Heart Rate",
						GTK_WINDOW(app_data->window),
						GTK_DIALOG_MODAL,
						GTK_STOCK_OK,
						GTK_RESPONSE_NONE,
						NULL);

	gtk_widget_set_size_request(dialog, 450,160);

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

	label = gtk_label_new ("Age:");
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, TRUE, 0);

	vbox2 = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), vbox2, TRUE, TRUE, 5);

	adj = (GtkAdjustment *) gtk_adjustment_new (30.0, 10.0, 100.0, 1.0,
						    5.0, 0.0);
	spinner = gtk_spin_button_new (adj, 0, 0);
	gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), TRUE);
	gtk_box_pack_start (GTK_BOX (vbox2), spinner, FALSE, TRUE, 0);

	vbox2 = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), vbox2, TRUE, TRUE, 5);

	label = gtk_label_new ("years");
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
