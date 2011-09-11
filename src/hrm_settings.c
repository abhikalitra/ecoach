/*
 *  eCoach
 *
 *  Copyright (C) 2009 Sampo Savola,  Jukka Alasalmi
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

/* System */
#include <string.h>

/* Hildon */
#include <hildon/hildon-caption.h>

/* Bluetooth connectivity dialogs DBus interface */
#include <conbtdialogs-dbus.h>

/* Custom modules */
#include "gconf_keys.h"
#include "hrm_settings.h"
#include "debug.h"
#include "marshal.h"
#include "gconf_helper.h"

/* i18n */
#include <glib/gi18n.h>

#define RESPONSE_CHANGE_DEVICE 1
#define RESPONSE_REMOVE_DEVICE 5

static void hrm_settings_cleanup(GeneralSettings *app_data);
static void hrm_settings_create_dialog(GeneralSettings *app_data);
static void hrm_settings_choose_device(GeneralSettings *app_data);
static gboolean hrm_settings_setup_dbus(GeneralSettings *app_data);
static void hrm_settings_disconnect_dbus_signal(GeneralSettings *app_data);
static void hrm_remove(GeneralSettings *app_data);

static void hrm_settings_set_dialog_sensitive(
		HRMSettingsPriv *priv,
		gboolean sensitive);

static void hrm_settings_device_chosen(
		DBusGProxy *proxy,
		const char *address,
		const char *name,
		const char *icon,
		const char *class_major,
		const char *class_minor,
		gboolean trusted,
		const char **service_classes,
		gpointer user_data
		);


void hrm_settings_show(GeneralSettings *app_data)
{
	// *app_data = (GeneralSettings *)user_data;
	HRMSettingsPriv *priv = NULL;
	gint response = 0;
	
	g_return_if_fail(app_data != NULL);
	DEBUG_BEGIN();
	if(app_data->hrm_data->settings_priv != NULL)
	{
		g_critical("Settings structure already allocated");
		DEBUG_END();
		return;
	}

	priv = g_new0(HRMSettingsPriv, 1);
	
	if(!priv)
	{
		g_critical("Not enough memory");
		DEBUG_END();
		return;
	}
	app_data->hrm_data->settings_priv = priv;

	priv->bluetooth_address = g_strdup(
			app_data->hrm_data->bluetooth_address);

	priv->bluetooth_name = g_strdup(
			app_data->hrm_data->bluetooth_name);

	hrm_settings_create_dialog(app_data);
	
	
	
	
	/*
	response = gtk_dialog_run (GTK_DIALOG (priv->dialog));
	switch (response)
	  {
	    case GTK_RESPONSE_ACCEPT:
		do_application_specific_something ();
		break;
	    default:
		do_nothing_since_dialog_was_cancelled ();
		break;
	  }
	  
	gtk_widget_destroy (dialog);
	  */    
	
	
	/*

	do {
		
		if(response == RESPONSE_CHANGE_DEVICE)
		{
			hrm_settings_choose_device(app_data);
		}
		if(response = RESPONSE_REMOVE_DEVICE)
		{
			hrm_remove(app_data);
		}
	} while();
	*
	**/
	do{
	response = gtk_dialog_run(GTK_DIALOG(priv->dialog));
	if(response == RESPONSE_CHANGE_DEVICE)
		{
			hrm_settings_choose_device(app_data);
		}
	if(response == RESPONSE_REMOVE_DEVICE)
	{
		hrm_remove(app_data);
	}
	if(response == GTK_RESPONSE_DELETE_EVENT)
	{
		break;
	}
	if(response == GTK_RESPONSE_ACCEPT)
	{
	  if(priv->bluetooth_address !=NULL && priv->bluetooth_name !=NULL){
		
		if(strcmp(priv->bluetooth_name, "") != 0)
		{
	    	gchar* bt_name = g_strndup(priv->bluetooth_name, 4);
		gtk_label_set_text(GTK_LABEL(app_data->device_label),bt_name);
		g_free(bt_name);
		}
		app_data->hrm_data->bluetooth_address = priv->bluetooth_address;
		priv->bluetooth_address = NULL;
		app_data->hrm_data->bluetooth_name = priv->bluetooth_name;
		priv->bluetooth_name = NULL;

		gconf_helper_set_value_string(
				app_data->gconf_helper,
				ECGC_BLUETOOTH_ADDRESS,
				app_data->hrm_data->bluetooth_address);

		gconf_helper_set_value_string(
				app_data->gconf_helper,
				ECGC_BLUETOOTH_NAME,
				app_data->hrm_data->bluetooth_name);
	
	  }
	        break;
	}
	}while (1);

	hrm_settings_cleanup(app_data);

	DEBUG_END();
}

static void hrm_settings_create_dialog(GeneralSettings *app_data)
{
	HRMSettingsPriv *priv = NULL;
	GtkBox *vbox = NULL;

	g_return_if_fail(app_data != NULL);

	priv = app_data->hrm_data->settings_priv;

	DEBUG_BEGIN();

	priv->dialog = gtk_dialog_new_with_buttons(
			_("Heart Rate Monitor settings"),
			GTK_WINDOW(app_data->win),
			GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR,
			_("OK"), GTK_RESPONSE_ACCEPT,
			_("Change device"), RESPONSE_CHANGE_DEVICE,
			_("Remove device"), RESPONSE_REMOVE_DEVICE,
			NULL);

	vbox = GTK_BOX(GTK_DIALOG(priv->dialog)->vbox);

	priv->lbl_current_device = gtk_label_new(NULL);

	gtk_label_set_markup(
			GTK_LABEL(priv->lbl_current_device),
			_("<b>Current device</b>"));

	gtk_misc_set_alignment(GTK_MISC(priv->lbl_current_device),
				0, 0);

	gtk_box_pack_start(vbox, priv->lbl_current_device, TRUE, TRUE, 0);

	priv->h_separator = gtk_hseparator_new();
	gtk_box_pack_start(vbox, priv->h_separator, TRUE, TRUE, 0);

	priv->size_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	priv->lbl_name = gtk_label_new(NULL);

	if(priv->bluetooth_name == NULL ||
			strcmp(priv->bluetooth_name, "") == 0)
	{
		gtk_label_set_text(
				GTK_LABEL(priv->lbl_name),
				_("<no name>"));
	} else {
		gtk_label_set_text(
				GTK_LABEL(priv->lbl_name),
				priv->bluetooth_name);
	}

	gtk_misc_set_alignment(GTK_MISC(priv->lbl_name), 0, 0.5);
	priv->caption_name = hildon_caption_new(
			priv->size_group,
			_("Device name"),
			priv->lbl_name,
			NULL,
			HILDON_CAPTION_OPTIONAL);
	gtk_box_pack_start(vbox, priv->caption_name, FALSE, FALSE, 0);

	priv->lbl_bt_addr = gtk_label_new(NULL);

	if(priv->bluetooth_address == NULL ||
			strcmp(priv->bluetooth_address, "") == 0)
	{
		gtk_label_set_text(
				GTK_LABEL(priv->lbl_bt_addr),
				_("<no device>"));
	  gtk_dialog_set_response_sensitive(GTK_DIALOG(priv->dialog),
					    RESPONSE_REMOVE_DEVICE,
					    FALSE);
	} else {
		gtk_label_set_text(
				GTK_LABEL(priv->lbl_bt_addr),
				priv->bluetooth_address);
	}
	gtk_misc_set_alignment(GTK_MISC(priv->lbl_bt_addr), 0, 0.5);
	priv->caption_bt_addr = hildon_caption_new(
			priv->size_group,
			_("Bluetooth address"),
			priv->lbl_bt_addr,
			NULL,
			HILDON_CAPTION_OPTIONAL);
	gtk_box_pack_start(vbox, priv->caption_bt_addr, FALSE, FALSE, 0);

	gtk_widget_show_all(priv->dialog);

	DEBUG_END();
}

static void hrm_settings_cleanup(GeneralSettings *app_data)
{
	g_return_if_fail(app_data != NULL);
	DEBUG_BEGIN();

	HRMSettingsPriv *priv = app_data->hrm_data->settings_priv;

	gtk_widget_destroy(priv->dialog);

	if(priv->proxy_con_dialog)
	{
		g_object_unref(G_OBJECT(priv->proxy_con_dialog));
	}

	g_free(priv);
	priv = NULL;
	app_data->hrm_data->settings_priv = NULL;
	DEBUG_END();
}

static void hrm_settings_choose_device(GeneralSettings *app_data)
{
	HRMSettingsPriv *priv = NULL;
	GError *error = NULL;
	gchar *service_classes[] = { "information", NULL };
	gchar **service_classes_p = service_classes;

	g_return_if_fail(app_data != NULL);
	DEBUG_BEGIN();

	priv = app_data->hrm_data->settings_priv;

	/* Disable the buttons while the discovery dialog is shown */
	hrm_settings_set_dialog_sensitive(priv, FALSE);

	if(!hrm_settings_setup_dbus(app_data))
	{
		g_critical("Unable to show device selection dialog");
		hrm_settings_set_dialog_sensitive(priv, TRUE);
		DEBUG_END();
		return;
	}

	if(!dbus_g_proxy_call(
				priv->proxy_con_dialog,
				CONBTDIALOGS_SEARCH_REQ,
				&error,

				/* Parameters */
				G_TYPE_STRING, "",
				G_TYPE_STRING, "",
				G_TYPE_STRV, service_classes_p,
				G_TYPE_STRING, "require",
				G_TYPE_INVALID,

				/* Return */
				G_TYPE_INVALID))
	{
		g_critical("DBus proxy call failed: %s\n",
				error->message);
		g_error_free(error);
		hrm_settings_set_dialog_sensitive(priv, TRUE);
		DEBUG_END();
		return;
	}

	DEBUG_END();
}

static void hrm_settings_set_dialog_sensitive(
		HRMSettingsPriv *priv,
		gboolean sensitive)
{
	DEBUG_BEGIN();
	g_return_if_fail(priv != NULL);

	gtk_dialog_set_response_sensitive(
			GTK_DIALOG(priv->dialog),
			GTK_RESPONSE_ACCEPT,
			sensitive);

	gtk_dialog_set_response_sensitive(
			GTK_DIALOG(priv->dialog),
			RESPONSE_CHANGE_DEVICE,
			sensitive);
			
	gtk_dialog_set_response_sensitive(
			GTK_DIALOG(priv->dialog),
			GTK_RESPONSE_REJECT,
			sensitive);
	if(priv->bluetooth_address == NULL || (strcmp(priv->bluetooth_address, "") == 0)){
	gtk_dialog_set_response_sensitive(
			GTK_DIALOG(priv->dialog),
			RESPONSE_REMOVE_DEVICE,
			FALSE);
	}
	else{
	  gtk_dialog_set_response_sensitive(
			GTK_DIALOG(priv->dialog),
			RESPONSE_REMOVE_DEVICE,
			TRUE);
	}

	DEBUG_END();
}

static gboolean hrm_settings_setup_dbus(GeneralSettings *app_data)
{
	HRMSettingsPriv *priv = NULL;

	g_return_val_if_fail(app_data != NULL, FALSE);
	DEBUG_BEGIN();

	priv = app_data->hrm_data->settings_priv;

	if(!priv->proxy_con_dialog)
	{
		priv->proxy_con_dialog = dbus_g_proxy_new_for_name(
				app_data->dbus_system,
				CONBTDIALOGS_DBUS_SERVICE,
				CONBTDIALOGS_DBUS_PATH,
				CONBTDIALOGS_DBUS_INTERFACE);

		if(!priv->proxy_con_dialog)
		{
			g_critical("DBus proxy creation failed");
			DEBUG_END();
			return FALSE;
		}

		dbus_g_object_register_marshaller(
				marshal_VOID__STRING_STRING_STRING_STRING_STRING_BOOLEAN_POINTER,

				/* Return value */
				G_TYPE_NONE,

				/* Parameters */
				G_TYPE_STRING,
				G_TYPE_STRING,
				G_TYPE_STRING,
				G_TYPE_STRING,
				G_TYPE_STRING,
				G_TYPE_BOOLEAN,
				G_TYPE_STRV,
				G_TYPE_INVALID);
			
		dbus_g_proxy_add_signal(
				priv->proxy_con_dialog,
				CONBTDIALOGS_SEARCH_SIG,
				G_TYPE_STRING,
				G_TYPE_STRING,
				G_TYPE_STRING,
				G_TYPE_STRING,
				G_TYPE_STRING,
				G_TYPE_BOOLEAN,
				G_TYPE_STRV,
				G_TYPE_INVALID);

	} else {
		DEBUG_LONG("DBus already initialized");
	}

	/* The signal must be connected anyway */
	dbus_g_proxy_connect_signal(
			priv->proxy_con_dialog,
			CONBTDIALOGS_SEARCH_SIG,
			G_CALLBACK(hrm_settings_device_chosen),
			app_data,
			NULL);

	DEBUG_END();
	return TRUE;
}

static void hrm_settings_disconnect_dbus_signal(GeneralSettings *app_data)
{
	HRMSettingsPriv *priv = NULL;

	g_return_if_fail(app_data != NULL);
	DEBUG_BEGIN();

	priv = app_data->hrm_data->settings_priv;

	dbus_g_proxy_disconnect_signal(
			priv->proxy_con_dialog,
			CONBTDIALOGS_SEARCH_SIG,
			G_CALLBACK(hrm_settings_device_chosen),
			app_data);

	DEBUG_END();
}

static void hrm_settings_device_chosen(
		DBusGProxy *proxy,
		const char *address,
		const char *name,
		const char *icon,
		const char *class_major,
		const char *class_minor,
		gboolean trusted,
		const char **service_classes,
		gpointer user_data
		)
{
	HRMSettingsPriv *priv = NULL;
	GeneralSettings *app_data = (GeneralSettings *)user_data;

	g_return_if_fail(app_data != NULL);
	DEBUG_BEGIN();

	DEBUG("Device name: %s\n", name);
	DEBUG("Device address: %s\n", address);

	priv = app_data->hrm_data->settings_priv;

	if(address == NULL || (strcmp(address, "") == 0))
	{
		DEBUG_LONG("Device selection was cancelled");
		goto device_selection_finished;
	}

	if(priv->bluetooth_address)
	{
		g_free(priv->bluetooth_address);
	}
	priv->bluetooth_address = g_strdup(address);

	if(priv->bluetooth_name)
	{
		g_free(priv->bluetooth_name);
	}
	priv->bluetooth_name = g_strdup(name);

	gtk_label_set_text(
			GTK_LABEL(priv->lbl_name),
			priv->bluetooth_name);

	gtk_label_set_text(
			GTK_LABEL(priv->lbl_bt_addr),
			priv->bluetooth_address);

	
			

device_selection_finished:
	hrm_settings_disconnect_dbus_signal(app_data);
	/* Disable the buttons while the discovery dialog is shown */
	hrm_settings_set_dialog_sensitive(
			priv,
			TRUE);

	DEBUG_END();
}
static void hrm_remove(GeneralSettings *app_data){
  
    DEBUG_BEGIN();
    
    gconf_helper_set_value_string(
				app_data->gconf_helper,
				ECGC_BLUETOOTH_ADDRESS,
				"");

    gconf_helper_set_value_string(
		    app_data->gconf_helper,
		    ECGC_BLUETOOTH_NAME,
		    "");
  app_data->hrm_data->settings_priv->bluetooth_address = NULL;	    
  app_data->hrm_data->settings_priv->bluetooth_name = NULL;
  gtk_label_set_text(GTK_LABEL(app_data->hrm_data->settings_priv->lbl_name),_("<no name>"));
  gtk_label_set_text(GTK_LABEL(app_data->hrm_data->settings_priv->lbl_bt_addr),_("<no device>"));
  gtk_dialog_set_response_sensitive(GTK_DIALOG(app_data->hrm_data->settings_priv->dialog),
				    RESPONSE_REMOVE_DEVICE,FALSE);
  gtk_label_set_text(GTK_LABEL(app_data->device_label),_("none"));
  
  DEBUG_END();
}
