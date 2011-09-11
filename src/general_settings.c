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
Copyright (C) 2009, Sampo Savola
*/

#include "general_settings.h"
#include    <hildon/hildon.h>
#include "calculate_bmi.h"
#include "util.h"
#include "gconf_keys.h"
#include "debug.h"
#include "hrm_settings.h"
#define GFXDIR DATADIR		"/pixmaps/" PACKAGE_NAME "/"

/* i18n */
#include <glib/gi18n.h>


typedef enum _EcExerciseTypes {
	EC_EXERCISE_TYPE_AEROBIC = 0,
	EC_EXERCISE_TYPE_ANAEROBIC,
	EC_EXERCISE_TYPE_WEIGHT_CONTROL,
	EC_EXERCISE_TYPE_MODERATE_ACTIVITY,
	EC_EXERCISE_TYPE_COUNT
} EcExerciseTypes;
typedef struct _EcActivityType {

    gint hrmax;
    gint hrmin;
} EcActivityType;

static void pick_weight(GtkWidget *widget, GdkEvent *event,gpointer user_data);
static void pick_age(GtkWidget *widget, GdkEvent *event,gpointer user_data);
static void pick_height(GtkWidget *widget, GdkEvent *event,gpointer user_data);
static void pick_update_interval(GtkWidget *widget, GdkEvent *event,gpointer user_data);
static void change_display(GtkWidget *widget, GdkEvent *event,gpointer user_data);
static void change_units(GtkWidget *widget, GdkEvent *event,gpointer user_data);
static void change_device(GtkWidget *widget, GdkEvent *event,gpointer user_data);
static void weight_selected(HildonTouchSelector * selector, gint column, gpointer user_data);
static void age_selected(HildonTouchSelector * selector, gint column, gpointer user_data);
static void height_selected(HildonTouchSelector * selector, gint column, gpointer user_data);
static void update_interval_selected(HildonTouchSelector * selector, gint column, gpointer user_data);
static void general_settings_destroy(GtkWidget *btn,GdkEvent  *event, gpointer user_data);
GeneralSettings* general_settings_new(
		GtkWindow *parent_window,
		GConfHelperData *gconf_helper,DBusGConnection *dbus_system){
  
  	GeneralSettings *self = NULL;
	DEBUG_BEGIN();
	
	self = g_new0(GeneralSettings, 1);
	self->parent_window = parent_window;
	self->gconf_helper = gconf_helper;
	self->fixed = gtk_fixed_new();
	self->dbus_system = dbus_system;
	
	self->hrm_data = hrm_initialize(self->gconf_helper);
	if(!self->hrm_data)
	{
		g_critical("hrm_initialize() failed");
		return NULL;
	}
	DEBUG_END();
	return self;
}
static void general_settings_destroy(GtkWidget *btn,GdkEvent  *event, gpointer user_data)
{
 GeneralSettings *self = (GeneralSettings *)user_data; 
 g_return_if_fail(self != NULL);
 gint i;
 DEBUG_BEGIN();
 /*Calculate HR levels based on User weight,age and height */
 
 DEBUG("IKA %d", self->age);
 self->maxhr = 206.3-(0.711*self->age);
 gconf_helper_set_value_int_simple(self->gconf_helper,ECGC_HR_MAX,self->maxhr);
 DEBUG("MAXHR %d",self->maxhr);

 gchar *gconf_key = NULL;
 for(i = 0; i < EC_EXERCISE_TYPE_COUNT; i++)
	{
		
	   switch(i){
	     case EC_EXERCISE_TYPE_ANAEROBIC:
	       
		gconf_key = g_strdup_printf("%s/%s",
				ECGC_EXERC_ANAEROBIC_DIR,
				ECGC_HR_LIMIT_LOW);
		gconf_helper_set_value_int(
				self->gconf_helper,
				gconf_key,
				self->maxhr*0.8);
		g_free(gconf_key);
		gconf_key = g_strdup_printf("%s/%s",
				ECGC_EXERC_ANAEROBIC_DIR,
				ECGC_HR_LIMIT_HIGH);

		gconf_helper_set_value_int(
				self->gconf_helper,
				gconf_key,
				self->maxhr*0.9);
		break;
	     case EC_EXERCISE_TYPE_AEROBIC:
	       
		gconf_key = g_strdup_printf("%s/%s",
				ECGC_EXERC_AEROBIC_DIR,
				ECGC_HR_LIMIT_LOW);
		gconf_helper_set_value_int(
				self->gconf_helper,
				gconf_key,
				self->maxhr*0.7);
		g_free(gconf_key);
		gconf_key = g_strdup_printf("%s/%s",
				ECGC_EXERC_AEROBIC_DIR,
				ECGC_HR_LIMIT_HIGH);

		gconf_helper_set_value_int(
				self->gconf_helper,
				gconf_key,
				self->maxhr*0.8);
		break;
	       case EC_EXERCISE_TYPE_WEIGHT_CONTROL:
	       
		gconf_key = g_strdup_printf("%s/%s",
				ECGC_EXERC_WEIGHT_CTRL_DIR,
				ECGC_HR_LIMIT_LOW);
		gconf_helper_set_value_int(
				self->gconf_helper,
				gconf_key,
				self->maxhr*0.6);
		g_free(gconf_key);
		gconf_key = g_strdup_printf("%s/%s",
				ECGC_EXERC_WEIGHT_CTRL_DIR,
				ECGC_HR_LIMIT_HIGH);

		gconf_helper_set_value_int(
				self->gconf_helper,
				gconf_key,
				self->maxhr*0.7);
		break;
		
	       case EC_EXERCISE_TYPE_MODERATE_ACTIVITY:
	       
		gconf_key = g_strdup_printf("%s/%s",
				ECGC_EXERC_MODERATE_DIR,
				ECGC_HR_LIMIT_LOW);
		gconf_helper_set_value_int(
				self->gconf_helper,
				gconf_key,
				self->maxhr*0.5);
		g_free(gconf_key);
		gconf_key = g_strdup_printf("%s/%s",
				ECGC_EXERC_MODERATE_DIR,
				ECGC_HR_LIMIT_HIGH);

		gconf_helper_set_value_int(
				self->gconf_helper,
				gconf_key,
				self->maxhr*0.6);
		break;
	   }
	}
 
 
 gtk_widget_destroy(self->win);
 DEBUG_END();
}
void general_settings_show(GeneralSettings *self){
  
  g_return_if_fail(self != NULL);
  DEBUG_BEGIN();
 
  gchar *hrm_key;
  gchar *hrm_device;
  PangoFontDescription *desc = NULL;

  self->win = hildon_stackable_window_new();
  gtk_window_set_title ( GTK_WINDOW (self->win), _("eCoach >Settings"));
  gtk_widget_set_name(GTK_WIDGET(self->win), "mainwindow");
  g_signal_connect(G_OBJECT(self->win), "delete-event",
			G_CALLBACK(general_settings_destroy), self);
  
  
  self->img_personal = gtk_image_new_from_file(GFXDIR "ec_button_personal.png");
  self->img_device = gtk_image_new_from_file(GFXDIR "ec_button_device.png");
  self->img_connection = gtk_image_new_from_file(GFXDIR "ec_button_connection.png");
  
 
  desc = pango_font_description_new();
  pango_font_description_set_family(desc, "Nokia Sans");
  pango_font_description_set_absolute_size(desc, 29 * PANGO_SCALE);
  
  gtk_fixed_put (GTK_FIXED (self->fixed), self->img_personal, 80, 80);
  gtk_fixed_put (GTK_FIXED (self->fixed), self->img_device, 310, 80);
  gtk_fixed_put (GTK_FIXED (self->fixed), self->img_connection, 532, 80);
  
  
  self->weight = gconf_helper_get_value_int_with_default(self->gconf_helper,USER_WEIGHT,0);
  gchar weight_str[8];
  g_sprintf(weight_str,_("%d kg"), self->weight);

  self->age = gconf_helper_get_value_int_with_default(self->gconf_helper,USER_AGE,0);
  gchar age_str[3];
  g_sprintf(age_str,"%d",self->age);
  
  self->height = gconf_helper_get_value_int_with_default(self->gconf_helper,USER_HEIGHT,0);
  gchar height_str[8];
  g_sprintf(height_str,"%d",self->height);
  
  gint update = gconf_helper_get_value_int_with_default(self->gconf_helper,GPS_INTERVAL,5);
  if(update==0){
    update = 5;}
  gchar update_str[8];
  g_sprintf(update_str,_("%d sec"),update);
  
  self->weight_label = gtk_label_new(weight_str);
  self->age_label = gtk_label_new(age_str);
  self->height_label = gtk_label_new(height_str);
  self->update_label = gtk_label_new(update_str);
  	
  gtk_widget_modify_font(self->weight_label, desc);
  gtk_widget_modify_font(self->age_label, desc);
  gtk_widget_modify_font(self->height_label, desc);
  gtk_widget_modify_font(self->update_label, desc);
  //pango_layout_set_font_description(self->weight_label,desc);
  
  if(gconf_helper_get_value_bool_with_default(self->gconf_helper,DISPLAY_ON,TRUE)) {
  self->display_label = gtk_label_new(_("On"));
  }
  else
  { self->display_label = gtk_label_new(_("Off"));}
	
  if(gconf_helper_get_value_bool_with_default(self->gconf_helper,USE_METRIC,TRUE)) {
  self->units_label = gtk_label_new(_("Metric"));
  }
  else
  { self->units_label = gtk_label_new(_("Imperial"));}
   
   gtk_widget_modify_font(self->units_label, desc);
   gtk_widget_modify_font(self->display_label, desc);
   
  hrm_key = gconf_helper_get_value_string_with_default(
		self->gconf_helper,
		ECGC_BLUETOOTH_NAME,
		_("none"));

  if(hrm_key == NULL || (strcmp(hrm_key, "") == 0))  
  {
  self->device_label = gtk_label_new(_("none"));
  }
  else
  {
    hrm_device = g_strndup(hrm_key, 4);
    self->device_label = gtk_label_new(hrm_device);
    g_free(hrm_device);
  }
  g_free(hrm_key);
  
  
  gtk_widget_modify_font(self->device_label, desc);

 
   
  self->weight_event = gtk_event_box_new();
  gtk_event_box_set_visible_window(GTK_EVENT_BOX(self->weight_event),FALSE);
  gtk_widget_set_size_request(self->weight_event,170,115);
  
  self->age_event = gtk_event_box_new ();
  gtk_event_box_set_visible_window(GTK_EVENT_BOX(self->age_event),FALSE);
  gtk_widget_set_size_request(self->age_event,170,115);
  
  self->height_event = gtk_event_box_new ();
  gtk_event_box_set_visible_window(GTK_EVENT_BOX(self->height_event),FALSE);
  gtk_widget_set_size_request(self->height_event,170,115);
  
  self->display_event = gtk_event_box_new ();
  gtk_event_box_set_visible_window(GTK_EVENT_BOX(self->display_event),FALSE);
  gtk_widget_set_size_request(self->display_event,170,140);
  
  self->units_event = gtk_event_box_new ();
  gtk_event_box_set_visible_window(GTK_EVENT_BOX(self->units_event),FALSE);
  gtk_widget_set_size_request(self->units_event,170,140);
  
  self->device_event = gtk_event_box_new ();
  gtk_event_box_set_visible_window(GTK_EVENT_BOX(self->device_event),FALSE);
  gtk_widget_set_size_request(self->device_event,170,140);
  
  self->update_event = gtk_event_box_new ();
  gtk_event_box_set_visible_window(GTK_EVENT_BOX(self->update_event),FALSE);
  gtk_widget_set_size_request(self->update_event,170,140);
  
  gtk_container_add (GTK_CONTAINER (self->weight_event),self->weight_label);
  gtk_container_add (GTK_CONTAINER (self->age_event),self->age_label);
  gtk_container_add (GTK_CONTAINER (self->height_event),self->height_label);
 
  gtk_container_add (GTK_CONTAINER (self->display_event),self->display_label);
  gtk_container_add (GTK_CONTAINER (self->units_event),self->units_label);
  
  
  gtk_container_add (GTK_CONTAINER (self->device_event),self->device_label);
  gtk_container_add (GTK_CONTAINER (self->update_event),self->update_label);
  
  gtk_fixed_put (GTK_FIXED (self->fixed),self->weight_event, 95, 90);
  gtk_fixed_put (GTK_FIXED (self->fixed),self->age_event, 85, 172);
  gtk_fixed_put (GTK_FIXED (self->fixed),self->height_event, 83, 251);

  gtk_fixed_put (GTK_FIXED (self->fixed),self->display_event, 320, 105);
  gtk_fixed_put (GTK_FIXED (self->fixed),self->units_event, 320, 220);
  
  
  gtk_fixed_put (GTK_FIXED (self->fixed),self->device_event, 541, 105);
  gtk_fixed_put (GTK_FIXED (self->fixed),self->update_event, 541, 220);

  
  gtk_widget_set_events (self->weight_event, GDK_BUTTON_PRESS_MASK);
  
  
  g_signal_connect (G_OBJECT (self->weight_event), "button_press_event",
                  G_CALLBACK (pick_weight),self);
  
  g_signal_connect (G_OBJECT (self->age_event), "button_press_event",
                  G_CALLBACK (pick_age),self);
		  
  g_signal_connect (G_OBJECT (self->height_event), "button_press_event",
                  G_CALLBACK (pick_height),self);
		  
  g_signal_connect (G_OBJECT (self->display_event), "button_press_event",
                  G_CALLBACK (change_display),self);
		  
 g_signal_connect (G_OBJECT (self->units_event), "button_press_event",
                  G_CALLBACK (change_units),self);

 g_signal_connect (G_OBJECT (self->device_event), "button_press_event",
                  G_CALLBACK(change_device),self);

 g_signal_connect (G_OBJECT (self->update_event), "button_press_event",
                  G_CALLBACK(pick_update_interval),self);
  gtk_container_add (GTK_CONTAINER (self->win),self->fixed);
  gtk_widget_show_all(self->win);
  
  DEBUG_END();
}

static void pick_weight(GtkWidget *widget, GdkEvent *event,gpointer user_data)
{
	
    GeneralSettings *self = (GeneralSettings *)user_data;
    g_return_if_fail(self != NULL);
    DEBUG_BEGIN();
    self->weight_dialog = gtk_dialog_new();	
    gtk_window_set_title(GTK_WINDOW(self->weight_dialog),_("Set weight"));
    gtk_widget_set_size_request(self->weight_dialog, 800, 390);
    self->weight_selector = hildon_touch_selector_new_text();
    hildon_touch_selector_set_column_selection_mode (HILDON_TOUCH_SELECTOR (self->weight_selector),
    HILDON_TOUCH_SELECTOR_SELECTION_MODE_SINGLE);
    self->weight = gconf_helper_get_value_int_with_default(self->gconf_helper,USER_WEIGHT,0);
    int j = 0;
    for(;j< 300;j++){
    gchar *number;	
    number = g_strdup_printf(_("%d kg"),j);	
    hildon_touch_selector_append_text (HILDON_TOUCH_SELECTOR (self->weight_selector),number);
    g_free(number);
    }
    hildon_touch_selector_set_active(HILDON_TOUCH_SELECTOR(self->weight_selector),0,self->weight);
    hildon_touch_selector_center_on_selected(HILDON_TOUCH_SELECTOR(self->weight_selector));
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG(self->weight_dialog)->vbox),self->weight_selector);
    g_signal_connect (G_OBJECT (self->weight_selector), _("changed"),
    G_CALLBACK(weight_selected),user_data);
    gtk_widget_show_all(self->weight_dialog);
    DEBUG_END();
}
static void pick_age(GtkWidget *widget, GdkEvent *event,gpointer user_data)
{
	
	GeneralSettings *self = (GeneralSettings *)user_data;
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();
	self->age_dialog = gtk_dialog_new();	
	gtk_window_set_title(GTK_WINDOW(self->age_dialog),_("Set age"));
	gtk_widget_set_size_request(self->age_dialog, 800, 390);
	self->age_selector = hildon_touch_selector_new_text();
	hildon_touch_selector_set_column_selection_mode (HILDON_TOUCH_SELECTOR (self->age_selector),
	HILDON_TOUCH_SELECTOR_SELECTION_MODE_SINGLE);
	self->age = gconf_helper_get_value_int_with_default(self->gconf_helper,USER_AGE,0);
	int j = 0;
	for(;j< 99;j++){
	gchar *number;	
	number = g_strdup_printf("%d",j);	
	hildon_touch_selector_append_text (HILDON_TOUCH_SELECTOR (self->age_selector),number);
	g_free(number);
	 }
	 
	hildon_touch_selector_set_active(HILDON_TOUCH_SELECTOR(self->age_selector),0,self->age);
	hildon_touch_selector_center_on_selected(HILDON_TOUCH_SELECTOR(self->age_selector));
	
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(self->age_dialog)->vbox),self->age_selector);
	
	g_signal_connect (G_OBJECT (self->age_selector), _("changed"),
	G_CALLBACK(age_selected),user_data);
	
	gtk_widget_show_all(self->age_dialog);
}
static void pick_height(GtkWidget *widget, GdkEvent *event,gpointer user_data)
{
	
	GeneralSettings *self = (GeneralSettings *)user_data;
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();
	
	
	self->height_dialog = gtk_dialog_new();	
	gtk_window_set_title(GTK_WINDOW(self->height_dialog),_("Set height"));
	gtk_widget_set_size_request(self->height_dialog, 800, 390);
	self->height_selector = hildon_touch_selector_new_text();
	hildon_touch_selector_set_column_selection_mode (HILDON_TOUCH_SELECTOR (self->height_selector),
	HILDON_TOUCH_SELECTOR_SELECTION_MODE_SINGLE);
	self->height = gconf_helper_get_value_int_with_default(self->gconf_helper,USER_HEIGHT,0);
	int j = 0;
	for(;j< 250;j++){
	gchar *number;	
	number = g_strdup_printf(_("%d cm"),j);	
	hildon_touch_selector_append_text (HILDON_TOUCH_SELECTOR (self->height_selector),number);
	g_free(number);
	 }
	 
	hildon_touch_selector_set_active(HILDON_TOUCH_SELECTOR(self->height_selector),0,self->height);
	hildon_touch_selector_center_on_selected(HILDON_TOUCH_SELECTOR(self->height_selector));
	
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(self->height_dialog)->vbox),self->height_selector);
	
	g_signal_connect (G_OBJECT (self->height_selector), _("changed"),
	G_CALLBACK(height_selected),user_data);
	gtk_widget_show_all(self->height_dialog);
}
static void pick_update_interval(GtkWidget *widget, GdkEvent *event,gpointer user_data)
{
	gint* intervals [] = {2,5,10,20,NULL};
	GeneralSettings *self = (GeneralSettings *)user_data;
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();
	
	
	self->update_dialog = gtk_dialog_new();	
	gtk_window_set_title(GTK_WINDOW(self->update_dialog),_("Set GPS update interval"));
	gtk_widget_set_size_request(self->update_dialog, 800, 300);
	self->update_selector = hildon_touch_selector_new_text();
	hildon_touch_selector_set_column_selection_mode (HILDON_TOUCH_SELECTOR (self->update_selector),
	HILDON_TOUCH_SELECTOR_SELECTION_MODE_SINGLE);
	
	
	int j = 0;
	for(;j<  intervals[j] != NULL ;j++){
	gchar *number;	
	number = g_strdup_printf(_("%d sec"),intervals[j]);
	hildon_touch_selector_append_text (HILDON_TOUCH_SELECTOR (self->update_selector),number);
	g_free(number);
	}	 
	hildon_touch_selector_set_active(HILDON_TOUCH_SELECTOR(self->update_selector),0,1);
	hildon_touch_selector_center_on_selected(HILDON_TOUCH_SELECTOR(self->update_selector));
	
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(self->update_dialog)->vbox),self->update_selector);
	
	g_signal_connect (G_OBJECT (self->update_selector), _("changed"),
	G_CALLBACK(update_interval_selected),user_data);
	gtk_widget_show_all(self->update_dialog);
}
static void change_display(GtkWidget *widget, GdkEvent *event,gpointer user_data)
{
	GeneralSettings *self = (GeneralSettings *)user_data;
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();
	
	gboolean display = gconf_helper_get_value_bool_with_default(self->gconf_helper,DISPLAY_ON,TRUE);
	if(display){
	 
	  gtk_label_set_text(GTK_LABEL(self->display_label),_("Off"));
	  gconf_helper_set_value_bool(self->gconf_helper, DISPLAY_ON, FALSE);
	}
	else{
	 gtk_label_set_text(GTK_LABEL(self->display_label),_("On"));
	 gconf_helper_set_value_bool(self->gconf_helper, DISPLAY_ON, TRUE);
	}
	DEBUG_END();
}

static void change_units(GtkWidget *widget, GdkEvent *event,gpointer user_data)
{	
	GeneralSettings *self = (GeneralSettings *)user_data;
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();
	
	gboolean units = gconf_helper_get_value_bool_with_default(self->gconf_helper,USE_METRIC,TRUE);
	if(units){
	 
	  gtk_label_set_text(GTK_LABEL(self->units_label),_("Imperial"));
	  gconf_helper_set_value_bool(self->gconf_helper, USE_METRIC, FALSE);
	}
	else{
	 gtk_label_set_text(GTK_LABEL(self->units_label),_("Metric"));
	 gconf_helper_set_value_bool(self->gconf_helper, USE_METRIC, TRUE);
	}
	DEBUG_END();
}

static void change_device(GtkWidget *widget, GdkEvent *event,gpointer user_data)
{
	
	GeneralSettings *self = (GeneralSettings *)user_data;
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();
	hrm_settings_show(self);
	DEBUG_END();
}
void weight_selected(HildonTouchSelector * selector, gint column, gpointer user_data){
  
    GeneralSettings *self = (GeneralSettings *)user_data;
    g_return_if_fail(self != NULL);
    DEBUG_BEGIN();
    gchar *current_selection = NULL;
    current_selection = hildon_touch_selector_get_current_text (selector);
    self->weight = hildon_touch_selector_get_active(selector,column);
    gconf_helper_set_value_int_simple(self->gconf_helper,USER_WEIGHT,self->weight);
    gtk_label_set_text(GTK_LABEL(self->weight_label),current_selection);
    gtk_widget_destroy(self->weight_dialog);
    DEBUG_END();
}


void age_selected(HildonTouchSelector * selector, gint column, gpointer user_data){
  
    GeneralSettings *self = (GeneralSettings *)user_data;
    g_return_if_fail(self != NULL);
    DEBUG_BEGIN();
    gchar *current_selection = NULL;
    current_selection = hildon_touch_selector_get_current_text (selector);
    self->age = hildon_touch_selector_get_active(selector,column);
    gconf_helper_set_value_int_simple(self->gconf_helper,USER_AGE,self->age);
    gtk_label_set_text(GTK_LABEL(self->age_label),current_selection);
    gtk_widget_destroy(self->age_dialog);
    DEBUG_END();
}

void height_selected(HildonTouchSelector * selector, gint column, gpointer user_data){
  
    GeneralSettings *self = (GeneralSettings *)user_data;
    g_return_if_fail(self != NULL);
    DEBUG_BEGIN();
    gchar *current_selection = NULL;
    current_selection = hildon_touch_selector_get_current_text (selector);
    self->height = hildon_touch_selector_get_active(selector,column);
    gconf_helper_set_value_int_simple(self->gconf_helper,USER_HEIGHT,self->height);
    gtk_label_set_text(GTK_LABEL(self->height_label),current_selection);
    gtk_widget_destroy(self->height_dialog);
    DEBUG_END();
}
void update_interval_selected(HildonTouchSelector * selector, gint column, gpointer user_data){
  
    int intervals[] ={2,5,10,20};
    GeneralSettings *self = (GeneralSettings *)user_data;
    g_return_if_fail(self != NULL);
    DEBUG_BEGIN();
    gchar *current_selection = NULL;
    current_selection = hildon_touch_selector_get_current_text (selector);
    gint interval = hildon_touch_selector_get_active(selector,column);
    gconf_helper_set_value_int_simple(self->gconf_helper,GPS_INTERVAL,intervals[interval]);
    gtk_label_set_text(GTK_LABEL(self->update_label),current_selection);
    gtk_widget_destroy(self->update_dialog);
    DEBUG_END();
}
