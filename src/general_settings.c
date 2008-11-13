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

static GtkSpinButton *spinner;
static GtkSpinButton *spinner2;
static GtkLabel *val_label;



void show_general_settings(NavigationMenu *menu, GtkTreePath *path,
			gpointer user_data)
{
	
	AppData *app_data = (AppData *)user_data;
	GtkWidget *dialog;
	GtkWidget *hbox;
	GtkWidget *main_vbox;
	GtkWidget *vbox;
	GtkWidget *vbox2;
	GtkWidget *radio1;
	GtkWidget *radio2;
	GtkWidget *entry;
	GtkWidget *label;
	GtkAdjustment *adj;
	GtkAdjustment *adj2;
	
	/* Create the widgets */
	
	dialog = gtk_dialog_new_with_buttons ("General Settings",
						GTK_WINDOW(app_data->window),
						GTK_DIALOG_MODAL,
						GTK_STOCK_OK,
						GTK_RESPONSE_NONE,
						NULL);

	gtk_widget_set_size_request(dialog, 300,150);

	main_vbox = gtk_vbox_new (FALSE, 2);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),
			main_vbox);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
	gtk_container_add (GTK_CONTAINER (main_vbox), vbox);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 2);

	vbox2 = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), vbox2, TRUE, TRUE, 2);

	label = gtk_label_new ("Units:");
	gtk_box_pack_start (GTK_BOX (vbox2), label, TRUE, TRUE, 2);

	vbox2 = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), vbox2, TRUE, TRUE, 2);

	radio1 = gtk_radio_button_new_with_label (NULL, "Metric");
	entry = gtk_entry_new ();
	gtk_container_add (GTK_CONTAINER (radio1), entry);
	gtk_box_pack_start (GTK_BOX (vbox2), radio1, TRUE, TRUE, 2);

	radio2 = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radio1),
							"English");
	gtk_box_pack_start (GTK_BOX (vbox2), radio2, TRUE, TRUE, 2);



	
	/* Ensure that the dialog box is destroyed when the user responds. */
	
	g_signal_connect_swapped (dialog,
				"response", 
				G_CALLBACK (gtk_widget_destroy),
				dialog);
	
	gtk_widget_show_all (dialog);
	
	}
