/*
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA  02110-1301, USA.

    ---
    Copyright (C) 2009, Sampo Savola, Kai Skiftesvik
 */


#ifndef _GENERAL_SETTINGS_H
#define _GENERAL_SETTINGS_H
#include <gtk/gtk.h>
#include "hildon/hildon-picker-button.h"
#include "gconf_helper.h"
#include "config.h"
#include "hrm_shared.h"

typedef struct _GeneralSettings{
   
  
  GtkWindow *parent_window;
  GConfHelperData *gconf_helper;
  DBusGConnection *dbus_system;
 

 /* Heart rate monitor settings etc. */
  HRMData *hrm_data;

  GtkWidget *dialog;
  GtkWidget *win;
  GtkWidget *table;
  GtkWidget* fixed;
  gint		weight;
  gint		age;
  gint		height;
  gint		maxhr;
  GtkWidget *img_personal;
  GtkWidget *img_device;
  GtkWidget *img_connection;
  
  GtkWidget *weight_label;
  GtkWidget *age_label;
  GtkWidget *height_label;
   
  GtkWidget *display_label;
  GtkWidget *units_label;
   
  GtkWidget *device_label;
  GtkWidget *update_label;
  
  GtkWidget *weight_event;
  GtkWidget *age_event;
  GtkWidget *height_event;
  GtkWidget *display_event;
  GtkWidget *units_event;
  GtkWidget *device_event;
  GtkWidget *update_event;
  
/*   Weight picker*/
  GtkWidget *weight_dialog;
  GtkWidget *weight_selector;
  
/*    Age picker*/    
  GtkWidget *age_dialog;
  GtkWidget *age_selector;

/*    Height picker*/    
  GtkWidget *height_dialog;
  GtkWidget *height_selector;
  
/*    GPS Update interval picker*/    
  GtkWidget *update_dialog;
  GtkWidget *update_selector;
  
  GtkWidget *test;
}GeneralSettings;


GeneralSettings *general_settings_new(GtkWindow *parent_window,GConfHelperData *gconf_helper,DBusGConnection *dbus_system);

void general_settings_show(GeneralSettings *self);

#endif /* _GENERAL_SETTINGS_H */

