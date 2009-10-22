/*
 *  eCoach
 *
 *  Copyright (C) 2009  Sampo Savola Jukka Alasalmi
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published bylocation-distance-utils-fix
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

/*****************************************************************************
 * Includes                                                                  *
 *****************************************************************************/

/* This module */

#include "map_view.h"
#include <CCalendarUtil.h>
/* System */
#include <math.h>
#include <sys/time.h>
#include <time.h>

/* GLib */
#include <glib/gi18n.h>

/* Gtk */
#include <gtk/gtkbutton.h>
#include <gtk/gtkfixed.h>
#include <gtk/gtkhseparator.h>
#include <gtk/gtkimage.h>
#include <gtk/gtklabel.h>

/* Hildon */
#include <hildon/hildon-note.h>

/* Location */

#include "location-distance-utils-fix.h"

/* Other modules */
#include "gconf_keys.h"
#include "ec_error.h"
#include "ec-button.h"
#include "ec-progress.h"
#include "util.h"

//#include "map_widget/map_widget.h"


#include <gdk/gdkx.h>

#include "config.h"
#include "debug.h"
#include <gdk/gdkkeysyms.h>

/*****************************************************************************
 * Definitions                                                               *
 *****************************************************************************/

#define MAPTILE_LOADER_EXEC_NAME "ecoach-maptile-loader"
#define GFXDIR DATADIR "/pixmaps/ecoach/"
#define MAP_VIEW_SIMULATE_GPS 0




/*****************************************************************************
 * Private function prototypes                                               *
 *****************************************************************************/
static void key_press_cb(GtkWidget * widget, GdkEventKey * event,MapView *self);
static gboolean map_view_connect_beat_detector_idle(gpointer user_data);
static void map_view_setup_progress_indicator(EcProgress *prg);

static gboolean fixed_expose_event(
		GtkWidget *widget,
		GdkEventExpose *event,
		gpointer user_data);

static void map_view_update_heart_rate_icon(MapView *self, gdouble heart_rate);

static void map_view_heart_rate_changed(
		BeatDetector *beat_detector,
		gdouble heart_rate,
		struct timeval *time,
		gint beat_type,
		gpointer user_data);

static void map_view_hide_map_widget(MapView *self);

static void map_view_location_changed(
		LocationGPSDevice *device,
		gpointer user_data);

static void map_view_check_and_add_route_point(
		MapView *self,
		MapPoint *point,
		LocationGPSDeviceFix *fix);	
static void map_view_btn_start_pause_clicked(GtkWidget *button,
		gpointer user_data);
static void map_view_btn_stop_clicked(GtkWidget *button, gpointer user_data);
static void map_view_start_activity(MapView *self);
static gboolean map_view_update_stats(gpointer user_data);
static void map_view_set_elapsed_time(MapView *self, struct timeval *tv);
static void map_view_pause_activity(MapView *self);
static void map_view_continue_activity(MapView *self);
gboolean map_button_press_cb(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
gboolean map_button_release_cb(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
void select_map_source_cb (HildonButton *button, gpointer user_data);
void map_source_selected(HildonTouchSelector * selector, gint column, gpointer user_data);
static HildonAppMenu *create_menu (MapView *self);
#if (MAP_VIEW_SIMULATE_GPS)
static void map_view_simulate_gps(MapView *self);
static gboolean map_view_simulate_gps_timeout(gpointer user_data);
#endif

static GtkWidget *map_view_create_info_button(
		MapView *self,
		const gchar *title,
		const gchar *label,
		gint x, gint y);

static GdkPixbuf *map_view_load_image(const gchar *path);
static gboolean _hide_buttons_timeout(gpointer user_data);
static void add_hide_buttons_timeout(gpointer user_data);
static void toggle_map_centering(HildonCheckButton *button, gpointer user_data);
static void map_view_show_data(MapView *self);
static void map_view_create_data(MapView *self);
static void map_view_hide(MapView *self);
static void about_dlg(HildonButton *button, gpointer user_data);
static void personal_data_dlg(HildonButton *button, gpointer user_data);
static void hide_data(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static void data_rec_btn_press_cb(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static void data_pause_btn_press_cb(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static void add_note_cb(HildonButton *button, gpointer user_data);
static void info_speed_clicked(GtkWidget *button, gpointer user_data);
static void destroy_personal_data_dlg (GtkWidget *widget,GdkEvent  *event,gpointer user_data);
/*****************************************************************************
 * Function declarations                                                     *
 *****************************************************************************/

/*===========================================================================*
 * Public functions                                                          *
 *===========================================================================*/

MapView *map_view_new(
		GtkWindow *parent_window,
		GConfHelperData *gconf_helper,
		BeatDetector *beat_detector,
		osso_context_t *osso)
{
	MapView *self = NULL;
	GdkColor color;

	g_return_val_if_fail(parent_window != NULL, NULL);
	g_return_val_if_fail(gconf_helper != NULL, NULL);
	g_return_val_if_fail(beat_detector != NULL, NULL);
	g_return_val_if_fail(osso != NULL, NULL);
	DEBUG_BEGIN();

	self = g_new0(MapView, 1);
	self->parent_window = parent_window;
	self->gconf_helper = gconf_helper;
	self->beat_detector = beat_detector;
	self->osso = osso;
	self->track_helper = track_helper_new();
	self->first_location_point_added = FALSE;



	self->win = hildon_stackable_window_new();

	g_signal_connect(self->win, "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);
	g_signal_connect( G_OBJECT(self->win), "key-press-event",
                    G_CALLBACK(key_press_cb), self);

	self->gps_update_interval = gconf_helper_get_value_int_with_default(self->gconf_helper,GPS_INTERVAL,5);
	self->map_provider = (OsmGpsMapSource_t)gconf_helper_get_value_int_with_default(self->gconf_helper,MAP_SOURCE,1);

	if(self->map_provider==0)
	{
	  self->map_provider = (OsmGpsMapSource_t)1;
	}
	self->menu = HILDON_APP_MENU (hildon_app_menu_new ());
	self->menu = create_menu(self);
	hildon_window_set_app_menu (HILDON_WINDOW (self->win), self->menu);

	self->friendly_name = osm_gps_map_source_get_friendly_name(self->map_provider);
	self->cachedir = g_build_filename(
                        g_get_user_cache_dir(),
                        "osmgpsmap",
                        self->friendly_name,
                        NULL);
        self->fullpath = TRUE;

	self->buttons_hide_timeout = 8000;
	self->map = (GtkWidget*)g_object_new (OSM_TYPE_GPS_MAP,
                        "map-source",(OsmGpsMapSource_t)self->map_provider,
                        "tile-cache",self->cachedir,
                        "tile-cache-is-full-path",self->fullpath,
                        "proxy-uri",g_getenv("http_proxy"),
                        NULL);

	g_free(self->cachedir);

	osm_gps_map_set_zoom((OsmGpsMap*)self->map,15);

	self->is_auto_center = TRUE;
	self->gps_initialized = FALSE;
	/* Main layout 		*/
	self->main_widget = gtk_fixed_new();
	gdk_color_parse("#000", &color);

	gtk_widget_modify_bg(self->main_widget, GTK_STATE_NORMAL, &color);

	self->zoom_in =   gdk_pixbuf_new_from_file(GFXDIR "ec_map_zoom_in.png",NULL);
	self->zoom_out = gdk_pixbuf_new_from_file(GFXDIR "ec_map_zoom_out.png",NULL);
	self->map_btn = gdk_pixbuf_new_from_file(GFXDIR "ec_button_map_selected.png",NULL);
	self->data_btn = gdk_pixbuf_new_from_file(GFXDIR "ec_button_data_unselected.png",NULL);


	self->rec_btn_unselected = gdk_pixbuf_new_from_file(GFXDIR "ec_button_rec_unselected.png",NULL);
	self->rec_btn_selected = gdk_pixbuf_new_from_file(GFXDIR "ec_button_rec_selected.png",NULL);


	self->pause_btn_unselected = gdk_pixbuf_new_from_file(GFXDIR "ec_button_pause_unselected.png",NULL);
	self->pause_btn_selected = gdk_pixbuf_new_from_file(GFXDIR "ec_button_pause_selected.png",NULL);

	gtk_fixed_put(GTK_FIXED(self->main_widget),self->map, 0, 0);
	gtk_widget_set_size_request(self->map, 800, 420);

	osm_gps_map_add_button((OsmGpsMap*)self->map,0, 150, self->zoom_out);
	osm_gps_map_add_button((OsmGpsMap*)self->map,720, 150, self->zoom_in);
	osm_gps_map_add_button((OsmGpsMap*)self->map,50, 346, self->map_btn);
	osm_gps_map_add_button((OsmGpsMap*)self->map,213, 346, self->data_btn);

	gtk_container_add (GTK_CONTAINER (self->win),self->main_widget);
	g_signal_connect (self->map, "button-press-event",
                      G_CALLBACK (map_button_press_cb),self);
	g_signal_connect (self->map, "button-release-event",
                    G_CALLBACK (map_button_release_cb), self);

		/* Createa data view*/
	map_view_create_data(self);

	DEBUG_END();
	return self;
}



void map_view_show(MapView *self)
{

	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();
	
	self->display_on = gconf_helper_get_value_bool_with_default(self->gconf_helper, DISPLAY_ON,TRUE);
	if((self->activity_state == MAP_VIEW_ACTIVITY_STATE_STOPPED) ||
	   (self->activity_state == MAP_VIEW_ACTIVITY_STATE_NOT_STARTED))
	{
	osm_gps_map_add_button((OsmGpsMap*)self->map,421, 346, self->rec_btn_unselected);
	osm_gps_map_add_button((OsmGpsMap*)self->map,584, 346, self->pause_btn_unselected);
	}
	else{
	osm_gps_map_add_button((OsmGpsMap*)self->map,421, 346, self->rec_btn_selected);

	}
	if(self->activity_state == MAP_VIEW_ACTIVITY_STATE_PAUSED)
	{
	osm_gps_map_add_button((OsmGpsMap*)self->map,584, 346, self->pause_btn_selected);
	}
	self->fullscreen = FALSE;
if(!self->gps_initialized)
{
      DEBUG("ALUSTETAAN GPS ASETUKSET!!");
       #if (MAP_VIEW_SIMULATE_GPS)
	self->show_map_widget_handler_id = 1;
#else
	self->gps_device = (LocationGPSDevice*)g_object_new(LOCATION_TYPE_GPS_DEVICE, NULL);
	self->gpsd_control = location_gpsd_control_get_default();

	switch(self->gps_update_interval)
	{
	  case 0:
	    g_object_set(G_OBJECT(self->gpsd_control), "preferred-interval", LOCATION_INTERVAL_5S, NULL);
	    break;

	  case 2:
	    g_object_set(G_OBJECT(self->gpsd_control), "preferred-interval", LOCATION_INTERVAL_2S, NULL);
	    break;

	  case 5:
	    g_object_set(G_OBJECT(self->gpsd_control), "preferred-interval", LOCATION_INTERVAL_5S, NULL);
	    break;

	  case 10:
	    g_object_set(G_OBJECT(self->gpsd_control), "preferred-interval", LOCATION_INTERVAL_10S, NULL);
	  break;

	  case 20:
	       g_object_set(G_OBJECT(self->gpsd_control), "preferred-interval", LOCATION_INTERVAL_20S, NULL);
	  break;
	}
//	g_object_set(G_OBJECT(self->gpsd_control), "preferred-method", LOCATION_METHOD_AGNSS, NULL);

	g_signal_connect(G_OBJECT(self->gps_device), "changed",
			G_CALLBACK(map_view_location_changed), self);
#endif

	if(gconf_helper_get_value_bool_with_default(self->gconf_helper,
	   USE_METRIC,TRUE))
	{
		self->metric = TRUE;
	}
	else
	{
		self->metric = FALSE;
	}

	if(!self->beat_detector_connected)
	{
		self->beat_detector_connected = TRUE;
		g_idle_add(map_view_connect_beat_detector_idle, self);
	}

	if(self->map_widget_state == MAP_VIEW_MAP_WIDGET_STATE_NOT_CONFIGURED)
	{
		//self->has_gps_fix = FALSE;
		self->map_widget_state = MAP_VIEW_MAP_WIDGET_STATE_CONFIGURING;
	} else {
#if (MAP_VIEW_SIMULATE_GPS)
		self->show_map_widget_handler_id = 1;
#else

#endif
	}

	/* Connect to GPS */
	/** @todo: Request the status and act according to it */

#if (MAP_VIEW_SIMULATE_GPS)
	map_view_simulate_gps(self);
#else
	location_gps_device_reset_last_known(self->gps_device);
	location_gpsd_control_start(self->gpsd_control);
#endif

	self->gps_initialized = TRUE;
}

	gtk_widget_show_all(self->win);
	add_hide_buttons_timeout(self);

	unsigned char value = 1;

	Atom hildon_zoom_key_atom =
	gdk_x11_get_xatom_by_name("_HILDON_ZOOM_KEY_ATOM"),integer_atom = gdk_x11_get_xatom_by_name("INTEGER");

	Display *dpy = GDK_DISPLAY_XDISPLAY(gdk_drawable_get_display(self->win->window));

	Window w = GDK_WINDOW_XID(self->win->window);

	XChangeProperty(dpy, w, hildon_zoom_key_atom, integer_atom, 8, PropModeReplace, &value, 1);
	DEBUG_END();
}

gboolean map_view_setup_activity(
		MapView *self,
		const gchar *activity_name,
		const gchar *activity_comment,
		const gchar *file_name,
		gint heart_rate_limit_low,
		gint heart_rate_limit_high,gboolean add_calendar)
{
	g_return_val_if_fail(self != NULL, FALSE);
	DEBUG_BEGIN();

	if((self->activity_state != MAP_VIEW_ACTIVITY_STATE_STOPPED) &&
	   (self->activity_state != MAP_VIEW_ACTIVITY_STATE_NOT_STARTED))
	{
		/**
		 * @todo: Ask if to stop the current activity and start
		 * saving to a different file
		 */
		DEBUG("Activity already started. Not starting a new one");
		return FALSE;
	}
	
	/* GLib takes care of the NULL values */
	g_free(self->activity_name);
	self->activity_name = g_strdup(activity_name);
	
	g_free(self->activity_comment);
	self->activity_comment = g_strdup(activity_comment);
	
	g_free(self->file_name);
	self->file_name = g_strdup(file_name);
	
	track_helper_setup_track(self->track_helper,
			activity_name,
			activity_comment);

	track_helper_set_file_name(self->track_helper, file_name);

	self->heart_rate_limit_low = heart_rate_limit_low;
	self->heart_rate_limit_high = heart_rate_limit_high;
	DEBUG("HR LIMIT LOW %d", self->heart_rate_limit_low);
	DEBUG("HR LIMIT HIGH %d", self->heart_rate_limit_high);
	self->add_calendar = add_calendar;

	DEBUG_END();
	return TRUE;
}


static void map_view_hide(MapView *self)
{
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();


	if((self->activity_state == MAP_VIEW_ACTIVITY_STATE_STARTED) ||
	   (self->activity_state == MAP_VIEW_ACTIVITY_STATE_PAUSED))
	{
		ec_error_show_message_error_ask_dont_show_again(
				_("When activity is in started or paused "
					"state, it is not stopped when going "
					"back. You can return to activity by "
					"clicking on the New activity button."
					"\n\nTo start a new activity, please "
					"press on the stop button first, "
					"then return to menu and click on the "
					"New activity button."),
				ECGC_MAP_VIEW_STOP_DIALOG_SHOWN,
				FALSE);
	}

	if(self->activity_state == MAP_VIEW_ACTIVITY_STATE_STOPPED)
	{
		if(self->beat_detector_connected)
		{
			self->beat_detector_connected = FALSE;
			beat_detector_remove_callback(
					self->beat_detector,
					map_view_heart_rate_changed,
					self);
		}
	}
	DEBUG_END();
}

MapViewActivityState map_view_get_activity_state(MapView *self)
{
	g_return_val_if_fail(self != NULL, MAP_VIEW_ACTIVITY_STATE_NOT_STARTED);
	DEBUG_BEGIN();

	return self->activity_state;

	DEBUG_END();
}

static void hide_data(GtkWidget *widget, GdkEventButton *event, gpointer user_data){

  MapView *self = (MapView *)user_data;
  DEBUG_BEGIN();
  gtk_widget_hide_all( self->data_win);
  add_hide_buttons_timeout(user_data);
 DEBUG_END();
}

static void data_rec_btn_press_cb(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	MapView *self = (MapView *)user_data;
	  DEBUG_BEGIN();

	    if((self->activity_state == MAP_VIEW_ACTIVITY_STATE_STOPPED) ||
		(self->activity_state == MAP_VIEW_ACTIVITY_STATE_NOT_STARTED))
	    {
		map_view_btn_start_pause_clicked(widget,user_data);
		gtk_widget_hide(self->data_rec_unselected_event);
		gtk_widget_show_all(self->data_rec_selected_event);

	    }
	     else
	     {
	      map_view_btn_stop_clicked(widget,user_data);
	      gtk_widget_hide(self->data_rec_selected_event);
	      gtk_widget_show_all(self->data_rec_unselected_event);
	     }
	 DEBUG_END();
}
static void data_pause_btn_press_cb(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	MapView *self = (MapView *)user_data;
	DEBUG_BEGIN();

	 if(self->activity_state == MAP_VIEW_ACTIVITY_STATE_STARTED || self->activity_state == MAP_VIEW_ACTIVITY_STATE_PAUSED){
	 	    map_view_btn_start_pause_clicked(widget,user_data);
	 	   if(self->activity_state == MAP_VIEW_ACTIVITY_STATE_STARTED) {
	 	 		 gtk_widget_hide(self->data_pause_selected_event);
	 		 		gtk_widget_show_all(self->data_pause_unselected_event);

	 	   }
	 	  if(self->activity_state == MAP_VIEW_ACTIVITY_STATE_PAUSED){

	 		gtk_widget_hide(self->data_pause_unselected_event);
	 						gtk_widget_show_all(self->data_pause_selected_event);

	 	  }

	 	   }


	DEBUG_END();
}

static void map_view_create_data(MapView *self){
  DEBUG_BEGIN();
 self->data_win = hildon_stackable_window_new();
 self->data_widget = gtk_fixed_new();

    self->data_map_btn = gtk_image_new_from_file(GFXDIR "ec_button_map_unselected.png");
    self->data_data_btn = gtk_image_new_from_file(GFXDIR "ec_button_data_selected.png");
    self->data_map_event = gtk_event_box_new();
    self->data_rec_selected_event = gtk_event_box_new();
    self->data_pause_selected_event = gtk_event_box_new();
    self->data_rec_unselected_event = gtk_event_box_new();
    self->data_pause_unselected_event = gtk_event_box_new();



    self->data_rec_btn_unselected = gtk_image_new_from_file(GFXDIR "ec_button_rec_unselected.png");
    self->data_rec_btn_selected = gtk_image_new_from_file(GFXDIR "ec_button_rec_selected.png");


    self->data_pause_btn_unselected =gtk_image_new_from_file(GFXDIR "ec_button_pause_unselected.png");
    self->data_pause_btn_selected = gtk_image_new_from_file(GFXDIR "ec_button_pause_selected.png");


    gtk_container_add (GTK_CONTAINER (self->data_map_event),self->data_map_btn);
    gtk_container_add (GTK_CONTAINER ( self->data_rec_selected_event),self->data_rec_btn_selected);
    gtk_container_add (GTK_CONTAINER ( self->data_rec_unselected_event),self->data_rec_btn_unselected);
    gtk_container_add (GTK_CONTAINER ( self->data_pause_selected_event),self->data_pause_btn_selected);
    gtk_container_add (GTK_CONTAINER ( self->data_pause_unselected_event),self->data_pause_btn_unselected);

 g_signal_connect(self->data_win, "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);


 self->info_time = map_view_create_info_button(
			self,
			_("Distance"),
			_("Duration"),
			0, 0);

  self->info_speed = map_view_create_info_button(
			self,
			_("Speed"),
			_("Avg. Speed"),
			0, 1);

  self->info_heart_rate = map_view_create_info_button(
  			self,
  			_("Heart rate"),
  			_("Wait..."),
  			0, 2);

  self->pxb_hrm_status[MAP_VIEW_HRM_STATUS_LOW] =
  map_view_load_image(GFXDIR "ec_icon_heart_yellow.png");

  self->pxb_hrm_status[MAP_VIEW_HRM_STATUS_GOOD] =
  map_view_load_image(GFXDIR "ec_icon_heart_green.png");

  self->pxb_hrm_status[MAP_VIEW_HRM_STATUS_HIGH] =
  map_view_load_image(GFXDIR "ec_icon_heart_red.png");

  self->pxb_hrm_status[MAP_VIEW_HRM_STATUS_NOT_CONNECTED] =
  map_view_load_image(GFXDIR "ec_icon_heart_grey.png");

  ec_button_set_icon_pixbuf(EC_BUTTON(self->info_heart_rate),
  self->pxb_hrm_status[MAP_VIEW_HRM_STATUS_NOT_CONNECTED]);

			
  g_signal_connect(G_OBJECT(self->data_map_event), "button-press-event",
                      G_CALLBACK (hide_data),self);

  g_signal_connect(G_OBJECT(self->data_rec_selected_event), "button-press-event",
                      G_CALLBACK (data_rec_btn_press_cb),self);

  g_signal_connect(G_OBJECT(self->data_rec_unselected_event), "button-press-event",
                        G_CALLBACK (data_rec_btn_press_cb),self);

  g_signal_connect(G_OBJECT(self->data_pause_unselected_event), "button-press-event",
                        G_CALLBACK (data_pause_btn_press_cb),self);

  g_signal_connect(G_OBJECT(self->data_pause_selected_event), "button-press-event",
                         G_CALLBACK (data_pause_btn_press_cb),self);
			 
  g_signal_connect(self->info_speed, "clicked", G_CALLBACK (info_speed_clicked), self);

 gtk_fixed_put(GTK_FIXED(self->data_widget),self->data_map_event,50,346);
 gtk_fixed_put(GTK_FIXED(self->data_widget),self->data_data_btn,213, 346);
 gtk_fixed_put(GTK_FIXED(self->data_widget),self->data_rec_unselected_event,421, 346);
 gtk_fixed_put(GTK_FIXED(self->data_widget),self->data_rec_selected_event,421, 346);
 gtk_fixed_put(GTK_FIXED(self->data_widget),self->data_pause_unselected_event,584, 346);
 gtk_fixed_put(GTK_FIXED(self->data_widget),self->data_pause_selected_event,584, 346);

 gtk_fixed_put(GTK_FIXED(self->data_widget),self->info_heart_rate,80, 75);
 gtk_fixed_put(GTK_FIXED(self->data_widget),self->info_speed,303, 75);
 gtk_fixed_put(GTK_FIXED(self->data_widget),self->info_time,525, 75);

 gtk_container_add (GTK_CONTAINER (self->data_win),self->data_widget);
 DEBUG_END();
}
static void map_view_show_data(MapView *self){


gtk_widget_show(self->data_win);
gtk_widget_show(self->data_widget);
gtk_widget_show(self->info_heart_rate);
gtk_widget_show(self->info_speed);
gtk_widget_show(self->info_time);
gtk_widget_show(self->data_map_btn);
gtk_widget_show(self->data_map_event);
gtk_widget_show(self->data_data_btn);

if((self->activity_state == MAP_VIEW_ACTIVITY_STATE_STOPPED) ||
	   (self->activity_state == MAP_VIEW_ACTIVITY_STATE_NOT_STARTED))
	{
	gtk_widget_show_all(self->data_rec_unselected_event);
	gtk_widget_show_all(self->data_pause_unselected_event);
	
	  ec_button_set_title_text(EC_BUTTON(self->info_speed),
		  "Speed");
		  
	  ec_button_set_label_text(EC_BUTTON(self->info_speed),
		  "Avg. Speed");

	}

if(self->activity_state == MAP_VIEW_ACTIVITY_STATE_STARTED)
		{
	gtk_widget_show_all(self->data_rec_selected_event);
	gtk_widget_show_all(self->data_pause_unselected_event);
		}
if(self->activity_state == MAP_VIEW_ACTIVITY_STATE_PAUSED)
		{
	gtk_widget_show_all(self->data_rec_selected_event);
	gtk_widget_show_all(self->data_pause_selected_event);
		}
}
void map_view_stop(MapView *self)
{
	g_return_if_fail(self != NULL);
	gdouble	travelled_distance = 0.0;
	gdouble avg_speed = 0.0;
	struct timeval time_now;
	struct timeval result;
	gchar *dist_text;
	gchar *avg_text;
	DEBUG_BEGIN();

	if((self->activity_state == MAP_VIEW_ACTIVITY_STATE_STOPPED) ||
	   (self->activity_state == MAP_VIEW_ACTIVITY_STATE_NOT_STARTED))
	{
		DEBUG_END();
		return;
	}
	//for calendar
	
	if(self->add_calendar){
	CCalendarUtil *util;
	time(&self->end);
	travelled_distance = track_helper_get_travelled_distance(self->track_helper);
	if(self->metric)
	{
		if(travelled_distance < 1000)
		{
			dist_text = g_strdup_printf(_("%.0f m"), travelled_distance);
		} else {
			dist_text = g_strdup_printf(_("%.1f km"),
					travelled_distance / 1000.0);
		}
	}
	else
	{
		if(travelled_distance < 1609)
		{
			travelled_distance = travelled_distance * 3.28;
			dist_text = g_strdup_printf(_("%.0f ft"), travelled_distance);
		} else {
			travelled_distance = travelled_distance * 0.621;
			dist_text = g_strdup_printf(_("%.1f mi"),
			travelled_distance / 1000.0);
		}
	}
	
	avg_speed = track_helper_get_average_speed(self->track_helper);
	if(avg_speed > 0.0)
	{
		if(self->metric)
		{
		avg_text = g_strdup_printf(_("%.1f km/h"), avg_speed);
		
		}
		else
		{
		avg_speed = avg_speed *	0.621;
		avg_text = g_strdup_printf(_("%.1f mph"), avg_speed);
		}
	
	}
	else
	{
	  avg_text = g_strdup_printf(_("0"));
	}
	
	
	gettimeofday(&time_now, NULL);
	util_subtract_time(&time_now, &self->start_time, &result);
	util_add_time(&self->elapsed_time, &result, &result);
	map_view_set_elapsed_time(self, &result);
	const gchar *time  =  ec_button_get_label_text((EcButton*)self->info_time);
	if(self->activity_comment ==NULL)
	{
	  self->activity_comment =  g_strdup_printf("");  
	}
	 /*Calculate min/km  */
	
	DEBUG("KULUNEET SEKUNTIT %d", result.tv_sec);
	DEBUG("KULUNUT MATKA %f", travelled_distance);
	gdouble minkm = (result.tv_sec / (travelled_distance/1000)/60);
	self->secs = modf(minkm,&self->mins);
	gchar *min_per_km = g_strdup_printf(" %02.f:%02.f",self->mins,(60*self->secs));
	
	

	util->addEvent(self->activity_name,"",g_strdup_printf("Duration: %s\nDistance: %s\nAvg.speed: %s\nMin/km: %s\nComment: %s \nGPX File: %s"
	,time,dist_text,avg_text,min_per_km,self->activity_comment,self->file_name),self->start,self->end);
	
	g_free(dist_text);
	g_free(avg_text);
	}
	track_helper_stop(self->track_helper);
	track_helper_clear(self->track_helper, FALSE);
	self->activity_state = MAP_VIEW_ACTIVITY_STATE_STOPPED;
	g_source_remove(self->activity_timer_id);
	self->activity_timer_id = 0;
	DEBUG_END();
}

void map_view_clear_all(MapView *self)
{
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();
	track_helper_clear(self->track_helper, TRUE);
	map_view_update_stats(self);
	DEBUG_END();
}

/*===========================================================================*
 * Private functions                                                         *
 *===========================================================================*/

/**
 * @brief Setup a progress indicator: set colors, images, margins and font
 *
 * @param prg EcProgress to modify
 */
static void map_view_setup_progress_indicator(EcProgress *prg)
{
	GdkColor color;

	g_return_if_fail(EC_IS_PROGRESS(prg));
	DEBUG_BEGIN();

	ec_progress_set_bg_image(prg, GFXDIR
			"ec_widget_generic_big_background.png");

	ec_progress_set_progress_bg_image(prg, GFXDIR
			"ec_widget_generic_big_progressbar_background.png");

	ec_progress_set_progress_image(prg, GFXDIR
			"ec_widget_generic_big_progressbar_fill.png");
	ec_progress_set_progress_margin_y(prg, 4);

	gdk_color_parse("#000", &color);
	ec_progress_set_bg_color(prg, &color);
	gdk_color_parse("#FFF", &color);
	ec_progress_set_fg_color(prg, &color);

	ec_progress_set_margin_x(EC_PROGRESS(prg), 14);
	ec_progress_set_margin_y(EC_PROGRESS(prg), 18);

	DEBUG_END();
}

static gboolean map_view_connect_beat_detector_idle(gpointer user_data)
{
	GError *error = NULL;
	GtkWidget *note;
	gint retcode;
	gboolean first_boot;
	MapView *self = (MapView *)user_data;

	g_return_val_if_fail(self != NULL, FALSE);
	DEBUG_BEGIN();

	if(!beat_detector_add_callback(
			self->beat_detector,
			map_view_heart_rate_changed,
			self,
			&error))
	{
		self->beat_detector_connected = FALSE;

		gdk_threads_enter();

		if(g_error_matches(error, ec_error_quark(),
					EC_ERROR_HRM_NOT_CONFIGURED))
		{
			first_boot = gconf_helper_get_value_bool_with_default(self->gconf_helper,FIRST_BOOT,FALSE);

			if(!first_boot){
			 note = hildon_note_new_information(GTK_WINDOW(self->win), "Heart rate monitor is not configured.\n"
						"To get heart rate data, please\n"
						"configure heart rate monitor in\n"
						"the settings menu.");
			 retcode = gtk_dialog_run (GTK_DIALOG (note));
			 gtk_widget_destroy (note);
			 gconf_helper_set_value_bool_simple(self->gconf_helper,FIRST_BOOT,TRUE);
			}
		} else {
			ec_error_show_message_error(error->message);
		}

			ec_button_set_label_text(EC_BUTTON(self->info_heart_rate),
			_("N/A"));


		gdk_threads_leave();
		DEBUG_END();
		return FALSE;
	}

	DEBUG_END();

	/* This only needs to be done once */
	return FALSE;
}
static gboolean fixed_expose_event(
		GtkWidget *widget,
		GdkEventExpose *event,
		gpointer user_data)
{
	MapView *self = (MapView *)user_data;
	gint i, count;
	GdkRectangle *rectangle;

	g_return_val_if_fail(self != NULL, FALSE);
	DEBUG_BEGIN();
	gdk_region_get_rectangles(event->region, &rectangle, &count);

	for(i = 0; i < count; i++)
	{
		gdk_draw_rectangle(
				widget->window,
				widget->style->bg_gc[GTK_STATE_NORMAL],
				TRUE,
				rectangle[i].x, rectangle[i].y,
				rectangle[i].width, rectangle[i].height);
	}

	DEBUG_END();
	return FALSE;
}

static void map_view_update_heart_rate_icon(MapView *self, gdouble heart_rate)
{
	DEBUG_BEGIN();
	if(heart_rate == -1)
	{
		ec_button_set_icon_pixbuf(
			EC_BUTTON(self->info_heart_rate),
			self->pxb_hrm_status[
			MAP_VIEW_HRM_STATUS_NOT_CONNECTED]);
		DEBUG_END();
		return;
	}

	if(heart_rate < self->heart_rate_limit_low)
	{
		ec_button_set_icon_pixbuf(
				EC_BUTTON(self->info_heart_rate),
				self->pxb_hrm_status[
				MAP_VIEW_HRM_STATUS_LOW]);
	} else if(heart_rate > self->heart_rate_limit_high)
	{
		ec_button_set_icon_pixbuf(
				EC_BUTTON(self->info_heart_rate),
				self->pxb_hrm_status[
				MAP_VIEW_HRM_STATUS_HIGH]);
	} else {
		ec_button_set_icon_pixbuf(
				EC_BUTTON(self->info_heart_rate),
				self->pxb_hrm_status[
				MAP_VIEW_HRM_STATUS_GOOD]);
	}
	DEBUG_END();
}
static void map_view_heart_rate_changed(
		BeatDetector *beat_detector,
		gdouble heart_rate,
		struct timeval *time,
		gint beat_type,
		gpointer user_data)
{
	gchar *text;
	MapView *self = (MapView *)user_data;

	g_return_if_fail(self != NULL);
	g_return_if_fail(time != NULL);
	DEBUG_BEGIN();

	if(heart_rate >= 0)
	{
	if(gtk_window_is_active(GTK_WINDOW(self->data_win)))
	{
		text = g_strdup_printf(_("%d bpm"), (gint)heart_rate);
		
		
		ec_button_set_label_text(
				EC_BUTTON(self->info_heart_rate),
	 			text);
		//gtk_label_set_text(GTK_LABEL(self->info_heart_rate),text);
		g_free(text);

		map_view_update_heart_rate_icon(self, heart_rate);
	}
		if(self->activity_state == MAP_VIEW_ACTIVITY_STATE_STARTED && self->first_location_point_added)
		{
			self->heart_rate_count++;
			if(self->heart_rate_count == 10)
			{
				self->heart_rate_count = 0;
				track_helper_add_heart_rate(
						self->track_helper,
						time,
						heart_rate);
			}
		}
	}

	DEBUG_END();
}
static void map_view_location_changed(
		LocationGPSDevice *device,
		gpointer user_data)
{
	MapPoint point;
	MapView *self = (MapView *)user_data;
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();
	/* Keep display on */
	
	if(self->display_on) {
		osso_display_state_on(self->osso);
		osso_display_blanking_pause(self->osso);
	}

	DEBUG("Latitude: %.2f - Longitude: %.2f\nAltitude: %.2f\n",
			device->fix->latitude,
			device->fix->longitude,
			device->fix->altitude);
			
	
	if(device->fix->fields & LOCATION_GPS_DEVICE_LATLONG_SET)	
	{
		if(!self->has_gps_fix && (device->fix->eph < 9000)){
		
		  GtkWidget *ban = hildon_banner_show_information(GTK_WIDGET(self->win),NULL,"Got GPS Fix ");
		  self->has_gps_fix = TRUE;
		}
		
		DEBUG("Latitude and longitude are valid");
		point.latitude = device->fix->latitude;
		point.longitude = device->fix->longitude;

		gchar *lbl_text = NULL;
		DEBUG("SPEED ACCURACY %.5f",device->fix->eps);
		if(self->activity_state == MAP_VIEW_ACTIVITY_STATE_STARTED)
		{
			
			DEBUG("HORIZONTAL ACCURACY %.5f",device->fix->eph);
			if(device->fix->eph < 9000){
			self->curr_speed = device->fix->speed;
			map_view_check_and_add_route_point(self, &point,
					device->fix);
			self->first_location_point_added = TRUE;
			DEBUG("SPEED ACCURACY %.5f",device->fix->eps);
			
			osm_gps_map_draw_gps(OSM_GPS_MAP(self->map),device->fix->latitude,device->fix->longitude,0);
			
			  if(self->is_auto_center)
			  {
			     if(gtk_window_is_active(GTK_WINDOW(self->win))){
			  osm_gps_map_set_center(OSM_GPS_MAP(self->map),device->fix->latitude,device->fix->longitude);
			     }
			  }
			 
			}
		}
		if(self->activity_state == MAP_VIEW_ACTIVITY_STATE_NOT_STARTED )
		{
		  
		  if(gtk_window_is_active(GTK_WINDOW(self->win))){
		  osm_gps_map_clear_gps(OSM_GPS_MAP(self->map));
		  osm_gps_map_draw_gps(OSM_GPS_MAP(self->map),device->fix->latitude,device->fix->longitude,0);
		  }
		  DEBUG("HORIZONTAL ACCURACY %.5f",device->fix->eph);
		}
	} else {
		DEBUG("Latitude and longitude are not valid");
		  self->has_gps_fix = FALSE;
		  GtkWidget *banner = hildon_banner_show_information(GTK_WIDGET(self->win),NULL,"Waiting for GPS...");
	}



	DEBUG_END();
}

static void map_view_check_and_add_route_point(
		MapView *self,
		MapPoint *point,
		LocationGPSDeviceFix *fix)
{
	gdouble distance = 0;
	gboolean add_point_to_track = FALSE;
	MapPoint *point_copy = NULL;
	TrackHelperPoint track_helper_point;
	gboolean map_widget_ready = TRUE;

	g_return_if_fail(self != NULL);
	g_return_if_fail(fix != NULL);
	DEBUG_BEGIN();

	if(self->activity_state != MAP_VIEW_ACTIVITY_STATE_STARTED)
	{
		DEBUG("Activity is not in started state. Returning");
		DEBUG_END();
		return;
	}
	if(!self->point_added)
	{
	//	DEBUG("First point. Adding to track.");
		/* Always add the first point to the route */
		add_point_to_track = TRUE;
		self->point_added = TRUE;
	} else {
		distance = location_distance_between(
				self->previous_added_point.latitude,
				self->previous_added_point.longitude,
				fix->latitude,
				fix->longitude) * 1000.0;
		DEBUG("Distance: %f meters", distance);

		/* Only add additional points if distance is at
		 * least 10 meters, or if elevation has changed at least
		 * 2 meters */
		/* @todo: Are the 10 m / 2 m values sane? */

		if(self->previous_added_point.altitude_set &&
				fix->fields & LOCATION_GPS_DEVICE_ALTITUDE_SET
				&& fabs(self->previous_added_point.altitude -
					fix->altitude) >= 2)
		{
			DEBUG("Altitude has changed at least 2 meters. "
					"Adding point to track.");
			add_point_to_track = TRUE;
		} else if(distance >= 10) {
			DEBUG("Distance has changed at least 10 meters. "
					"Adding point to track");
			add_point_to_track = TRUE;
		}
	}
	if(add_point_to_track)
	{
		self->previous_added_point.latitude = fix->latitude;
		self->previous_added_point.longitude = fix->longitude;

		track_helper_point.latitude = fix->latitude;
		track_helper_point.longitude = fix->longitude;

		if(fix->fields & LOCATION_GPS_DEVICE_ALTITUDE_SET)
		{
			self->previous_added_point.altitude_set = TRUE;
			self->previous_added_point.altitude = fix->altitude;
			track_helper_point.altitude_is_set = TRUE;
			track_helper_point.altitude = fix->altitude;
		} else {
			self->previous_added_point.altitude_set = FALSE;
			self->previous_added_point.altitude = 0;
			track_helper_point.altitude_is_set = FALSE;
		}

		gettimeofday(&track_helper_point.timestamp, NULL);
		track_helper_add_track_point(self->track_helper,
				&track_helper_point);

		/* Map widget wants to own the point that is added to the
		 * route */
		point_copy = g_new0(MapPoint, 1);
		point_copy->latitude = point->latitude;
		point_copy->longitude = point->longitude;
		
	}

	DEBUG_END();
}

static void map_view_btn_start_pause_clicked(GtkWidget *button,
		gpointer user_data)
{
	MapView *self = (MapView *)user_data;

	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	switch(self->activity_state)
	{
		case MAP_VIEW_ACTIVITY_STATE_NOT_STARTED:
			map_view_start_activity(self);
			osm_gps_map_remove_button((OsmGpsMap*)self->map,421, 346);
			osm_gps_map_add_button((OsmGpsMap*)self->map,421, 346, self->rec_btn_selected);
			break;
		case MAP_VIEW_ACTIVITY_STATE_STARTED:
			map_view_pause_activity(self);
			osm_gps_map_remove_button((OsmGpsMap*)self->map,584, 346);
			osm_gps_map_add_button((OsmGpsMap*)self->map,584, 346, self->pause_btn_selected);
			break;
		case MAP_VIEW_ACTIVITY_STATE_PAUSED:
			map_view_continue_activity(self);
			osm_gps_map_remove_button((OsmGpsMap*)self->map,584, 345);
			osm_gps_map_add_button((OsmGpsMap*)self->map,584, 346, self->pause_btn_unselected);
			break;
		case MAP_VIEW_ACTIVITY_STATE_STOPPED:
			map_view_start_activity(self);
			osm_gps_map_remove_button((OsmGpsMap*)self->map,421, 346);
			osm_gps_map_add_button((OsmGpsMap*)self->map,421, 346, self->rec_btn_selected);
			osm_gps_map_clear_gps(OSM_GPS_MAP(self->map));
			self->first_location_point_added = FALSE;
			break;
	}

	DEBUG_END();
}


static void map_view_btn_stop_clicked(GtkWidget *button, gpointer user_data)
{
	GtkWidget *dialog = NULL;
	gint result;

	MapView *self = (MapView *)user_data;

	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	if((self->activity_state == MAP_VIEW_ACTIVITY_STATE_STOPPED) ||
	   (self->activity_state == MAP_VIEW_ACTIVITY_STATE_NOT_STARTED))
	{
		DEBUG_END();
		return;
	}

	dialog = hildon_note_new_confirmation_add_buttons(
		self->parent_window,
		_("Do you really want to stop the current activity?\n\n"
		"Statistics such as distance and elapsed time\n"
		"will be cleared when you press Record button\n"
		"again."),
		GTK_STOCK_YES, GTK_RESPONSE_YES,
		GTK_STOCK_NO, GTK_RESPONSE_NO,
		NULL);

	gtk_widget_show_all(dialog);
	result = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);

	if(result == GTK_RESPONSE_NO)
	{
		DEBUG_END();
		return;
	}

	osm_gps_map_remove_button((OsmGpsMap*)self->map,421, 346);
	osm_gps_map_add_button((OsmGpsMap*)self->map,421, 346, self->rec_btn_unselected);
	osm_gps_map_remove_button((OsmGpsMap*)self->map,584, 345);
	osm_gps_map_add_button((OsmGpsMap*)self->map,584, 346, self->pause_btn_unselected);
	map_view_update_stats(self);
	map_view_stop(self);
	DEBUG_END();
}

static void map_view_start_activity(MapView *self)
{
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	self->elapsed_time.tv_sec = 0;
	self->elapsed_time.tv_usec = 0;

	time(&self->start);

	/* Clear the track helper */
	if(self->activity_state == MAP_VIEW_ACTIVITY_STATE_STOPPED)
	{
		track_helper_clear(self->track_helper, FALSE);
		map_view_update_stats(self);

	osm_gps_map_remove_button((OsmGpsMap*)self->map,421, 346);
	osm_gps_map_add_button((OsmGpsMap*)self->map,421, 346, self->rec_btn_selected);
	}

	gettimeofday(&self->start_time, NULL);

	self->activity_timer_id = g_timeout_add(
			3000,
			map_view_update_stats,
			self);

	self->activity_state = MAP_VIEW_ACTIVITY_STATE_STARTED;
	
	
	if(self->metric)
	{
	ec_button_set_title_text(EC_BUTTON(self->info_speed),
		"0 km/h");
		
	ec_button_set_label_text(EC_BUTTON(self->info_speed),
		"0 km/h");
	}
	else
	{
	  ec_button_set_title_text(EC_BUTTON(self->info_speed),
		"0 mph");
		
	ec_button_set_label_text(EC_BUTTON(self->info_speed),
		"0 mph");
	  
	}
	DEBUG_END();
}

static void map_view_continue_activity(MapView *self)
{
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	gettimeofday(&self->start_time, NULL);

	self->activity_timer_id = g_timeout_add(
			3000,
			map_view_update_stats,
			self);

	/* Force adding the current location */
	self->point_added = FALSE;

	self->activity_state = MAP_VIEW_ACTIVITY_STATE_STARTED;

	ec_button_set_bg_image(EC_BUTTON(self->btn_start_pause),
			EC_BUTTON_STATE_RELEASED,
			GFXDIR "ec_button_pause.png");

	DEBUG_END();
}

static void map_view_pause_activity(MapView *self)
{
	struct timeval time_now;
	struct timeval result;

	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	gettimeofday(&time_now, NULL);

	track_helper_pause(self->track_helper);

	/* Get the difference between now and previous start time */
	util_subtract_time(&time_now, &self->start_time, &result);

	/* Add to the elapsed time */
	util_add_time(&self->elapsed_time, &result, &self->elapsed_time);

	map_view_set_elapsed_time(self, &self->elapsed_time);
//	map_view_update_stats(self);

	g_source_remove(self->activity_timer_id);
	self->activity_timer_id = 0;
	self->activity_state = MAP_VIEW_ACTIVITY_STATE_PAUSED;
	ec_button_set_bg_image(EC_BUTTON(self->btn_start_pause),
			EC_BUTTON_STATE_RELEASED,
			GFXDIR "ec_button_rec.png");

	DEBUG_END();
}

static gboolean map_view_update_stats(gpointer user_data)
{
	MapView *self = (MapView *)user_data;
	gdouble travelled_distance = 0.0;
	gdouble avg_speed = 0.0;
	gdouble curr_speed = self->curr_speed;
	gchar *lbl_text = NULL;
	struct timeval time_now;
	struct timeval result;

	g_return_val_if_fail(self != NULL, FALSE);
	DEBUG_BEGIN();

	
	if(gtk_window_is_active(GTK_WINDOW(self->data_win))){
	/** @todo Usage of different units? (Feet/yards, miles) */

	/* Travelled distance */
	travelled_distance = track_helper_get_travelled_distance(
			self->track_helper);
	if(self->metric)
	{
		if(travelled_distance < 1000)
		{
			lbl_text = g_strdup_printf(_("%.0f m"), travelled_distance);
		} else {
			lbl_text = g_strdup_printf(_("%.1f km"),
					travelled_distance / 1000.0);
		}

		ec_button_set_title_text(EC_BUTTON(self->info_time), lbl_text);
		g_free(lbl_text);
	}
	else
	{

		if(travelled_distance < 1609)
		{
			travelled_distance = travelled_distance * 3.28;
			lbl_text = g_strdup_printf(_("%.0f ft"), travelled_distance);
		} else {
			travelled_distance = travelled_distance * 0.621;
			lbl_text = g_strdup_printf(_("%.1f mi"),
					travelled_distance / 1000.0);
		}

		ec_button_set_title_text(EC_BUTTON(self->info_time), lbl_text);
		g_free(lbl_text);
	}
	/* Elapsed time */

	/* Don't use the track_helper_get_elapsed_time(), because it
	 * does not take into account time elapsed since previous
	 * added track point */
	if(self->activity_state == MAP_VIEW_ACTIVITY_STATE_STARTED)
	{
		gettimeofday(&time_now, NULL);
		util_subtract_time(&time_now, &self->start_time, &result);
		util_add_time(&self->elapsed_time, &result, &result);
		map_view_set_elapsed_time(self, &result);
	} else if(self->activity_state == MAP_VIEW_ACTIVITY_STATE_STOPPED) {
		result.tv_sec = 0;
		result.tv_usec = 0;
		map_view_set_elapsed_time(self, &result);
	}

	/* Average speed */
	avg_speed = track_helper_get_average_speed(self->track_helper);
	if(avg_speed > 0.0)
	{
		if(self->metric)
		{
		lbl_text = g_strdup_printf(_("%.1f km/h"), avg_speed);
		ec_button_set_label_text(EC_BUTTON(self->info_speed),
			lbl_text);

		g_free(lbl_text);
		}
		else
		{
		avg_speed = avg_speed *	0.621;
		lbl_text = g_strdup_printf(_("%.1f mph"), avg_speed);
		ec_button_set_label_text(EC_BUTTON(self->info_speed),
					lbl_text);
		g_free(lbl_text);
		}
	} else {
		if(self->metric)
		{
		//ec_button_set_label_text(EC_BUTTON(self->info_avg_speed),
	//			_("0 km/h"));
		//gtk_label_set_text(GTK_LABEL(self->info_avg_speed),"O km/h");
		}
		else
		{
	//	gtk_label_set_text(GTK_LABEL(self->info_avg_speed),"O mph");
	//	ec_button_set_label_text(EC_BUTTON(self->info_avg_speed),
	//				_("0 mph"));
		}
	}

	/* Current speed */

	if(!self->show_min_per_km){
	if(curr_speed > 0.0)
	{
		if(self->metric)
		{
		lbl_text = g_strdup_printf(_("%.1f km/h"), curr_speed);
	
		ec_button_set_title_text(EC_BUTTON(self->info_speed),
		lbl_text);
		g_free(lbl_text);
		}
		else
		{
			curr_speed = curr_speed*0.621;
			lbl_text = g_strdup_printf(_("%.1f mph"), curr_speed);
			//ec_button_set_label_text(EC_BUTTON(self->info_speed),
			//		lbl_text);

		//	gtk_label_set_text(GTK_LABEL(self->info_speed),lbl_text);
			g_free(lbl_text);
		}
	} else {
		if(self->metric)
		{
			//ec_button_set_label_text(EC_BUTTON(self->info_speed),
			//			 _("0 km/h"));
		//	gtk_label_set_text(GTK_LABEL(self->info_speed),"0 km/h");
		}
		else
		{
			//ec_button_set_label_text(EC_BUTTON(self->info_speed),
			//		_("0 mph"));
		//	gtk_label_set_text(GTK_LABEL(self->info_speed),"0 mph");
		}
	}
	}
	else{
	 
	  /* Speed minutes per km  */
	
	DEBUG("KULUNEET SEKUNTIT %d", result.tv_sec);
	DEBUG("KULUNUT MATKA %f", travelled_distance);
	gdouble minkm = (result.tv_sec / (travelled_distance/1000)/60);
	self->secs = modf(minkm,&self->mins);
	DEBUG("MIN / KM  %02.f:%02.f ",self->mins,(60*self->secs));
	  
	ec_button_set_title_text(EC_BUTTON(self->info_speed),"Min/km");
	lbl_text = g_strdup_printf(_("%02.f:%02.f"),self->mins,(60*self->secs));
	
	ec_button_set_label_text(EC_BUTTON(self->info_speed),
	lbl_text);
	g_free(lbl_text);
	}
	  
	
	
	}
	DEBUG_END();
	return TRUE;
}

static void map_view_set_elapsed_time(MapView *self, struct timeval *tv)
{
	g_return_if_fail(self != NULL);
	g_return_if_fail(tv != NULL);

	gchar *lbl_text = NULL;

	guint hours, minutes, seconds;
	seconds = tv->tv_sec;
	if(tv->tv_usec > 500000)
	{
		seconds++;
	}

	minutes = seconds / 60;
	seconds = seconds % 60;

	hours = minutes / 60;
	minutes = minutes % 60;

	lbl_text = g_strdup_printf("%02d:%02d:%02d", hours, minutes, seconds);
	ec_button_set_label_text(EC_BUTTON(self->info_time), lbl_text);
	g_free(lbl_text);
}

#if (MAP_VIEW_SIMULATE_GPS)
static void map_view_simulate_gps(MapView *self)
{
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	g_timeout_add(3000, map_view_simulate_gps_timeout, self);

	DEBUG_END();
}

static gboolean map_view_simulate_gps_timeout(gpointer user_data)
{
	static int counter = 0;
	static gdouble coordinates[3];
	gdouble random_movement;

	LocationGPSDevice device;
	LocationGPSDeviceFix fix;

	MapView *self = (MapView *)user_data;

	g_return_val_if_fail(self != NULL, FALSE);
	DEBUG_BEGIN();

	device.fix = &fix;

	if(counter == 0)
	{
		coordinates[0] = 65.013;
		coordinates[1] = 25.509;
		coordinates[2] = 100;
	} else {
		random_movement = -5.0 + (10.0 * (rand() / (RAND_MAX + 1.0)));
		coordinates[0] = coordinates[0] + (random_movement / 5000.0);

		random_movement = -5.0 + (10.0 * (rand() / (RAND_MAX + 1.0)));
		coordinates[1] = coordinates[1] + (random_movement / 5000.0);

		random_movement = -5.0 + (10.0 * (rand() / (RAND_MAX + 1.0)));
		coordinates[2] = coordinates[2] + (random_movement);
	}

	fix.fields = LOCATION_GPS_DEVICE_LATLONG_SET |
		LOCATION_GPS_DEVICE_ALTITUDE_SET;
	fix.latitude = coordinates[0];
	fix.longitude = coordinates[1];
	fix.altitude = coordinates[2];
	map_view_location_changed(&device, self);

	counter++;
	if(counter > 100)
	{
		DEBUG_END();
		return FALSE;
	}
	DEBUG_END();
	return TRUE;
}
#endif

static GtkWidget *map_view_create_info_button(
		MapView *self,
		const gchar *title,
		const gchar *label,
		gint x, gint y)
{
	GtkWidget *button = NULL;
	PangoFontDescription *desc = NULL;

	g_return_val_if_fail(self != NULL, NULL);
	g_return_val_if_fail(title != NULL, NULL);
	g_return_val_if_fail(label != NULL, NULL);

	button = ec_button_new();
	ec_button_set_title_text(EC_BUTTON(button), title);
	ec_button_set_label_text(EC_BUTTON(button), label);
	ec_button_set_bg_image(EC_BUTTON(button), EC_BUTTON_STATE_RELEASED,
			GFXDIR "ec_info_button_generic.png");

	gtk_widget_set_size_request(button, 195, 268);
	ec_button_set_btn_down_offset(EC_BUTTON(button), 2);

	desc = pango_font_description_new();
	pango_font_description_set_family(desc, "Nokia Sans");
	pango_font_description_set_absolute_size(desc, 29 * PANGO_SCALE);
	ec_button_set_font_description_label(EC_BUTTON(button), desc);

	pango_font_description_set_absolute_size(desc, 29 * PANGO_SCALE);
	ec_button_set_font_description_title(EC_BUTTON(button), desc);

	return button;
}

static GdkPixbuf *map_view_load_image(const gchar *path)
{
	GError *error = NULL;
	GdkPixbuf *pxb = NULL;
	pxb = gdk_pixbuf_new_from_file(path, &error);
	if(error)
	{
		g_warning("Unable to load image %s: %s",
				path, error->message);
		g_error_free(error);
		return NULL;
	}

	gdk_pixbuf_ref(pxb);
	return pxb;
}
static void key_press_cb(GtkWidget * widget, GdkEventKey * event, MapView *self){

	switch (event->keyval) {
		g_return_if_fail(self != NULL);
		DEBUG_BEGIN();
		gint zoom;
		case HILDON_HARDKEY_INCREASE: /* Zoom in */
		  DEBUG("ZOOM IN");
		  g_object_get(self->map, "zoom", &zoom, NULL);
		  osm_gps_map_set_zoom((OsmGpsMap*)self->map, zoom+1);
		  break;

		case HILDON_HARDKEY_DECREASE: /* Zoom in */
		  DEBUG("ZOOM OUT");
		  g_object_get(self->map, "zoom", &zoom, NULL);
		  osm_gps_map_set_zoom((OsmGpsMap*)self->map, zoom-1);
		  break;
	DEBUG_END();
	}
    }

gboolean map_button_press_cb(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
      MapView *self = (MapView *)user_data;
      if(user_data ==NULL){
	return false;}
      DEBUG_BEGIN();
     coord_t coord;

    
    OsmGpsMap *map = OSM_GPS_MAP(widget);
if(!self->fullscreen){
    if ( (event->button == 1) && (event->type == GDK_BUTTON_PRESS) )
    {
	   int zoom = 0;
	coord = osm_gps_map_get_co_ordinates(map, (int)event->x, (int)event->y);




	if((event->x > 213 && event->x <380) && (event->y > 346 && event->y < 480)){


	  DEBUG("DATA BUTTON");
	//  gtk_widget_hide(self->map);
	  map_view_show_data(self);
	  DEBUG_END();
	 return FALSE;
	 
	}

	if(event->x < 80 && (event->y > 150 && event->y < 240)){

	  DEBUG("ZOOM OUT");
	    g_object_get(self->map, "zoom", &zoom, NULL);
	    osm_gps_map_set_zoom((OsmGpsMap*)self->map, zoom-1);
	}

	 if(event->x > 720 && (event->y > 150 && event->y < 240)){
	DEBUG("ZOOM IN");
	g_object_get(self->map, "zoom", &zoom, NULL);
	osm_gps_map_set_zoom((OsmGpsMap*)self->map, zoom+1);
	}

	 if((event->x < 584 && event->x > 421) && (event->y > 346 && event->y < 480)){

		DEBUG("REC BUTTON PRESS");

	    if((self->activity_state == MAP_VIEW_ACTIVITY_STATE_STOPPED) ||
		(self->activity_state == MAP_VIEW_ACTIVITY_STATE_NOT_STARTED))
	    {
		//  osm_gps_map_remove_button(self->map,394, 346);
		  //osm_gps_map_add_button(self->map,394, 346, self->rec_btn_selected);
		map_view_btn_start_pause_clicked(widget,user_data);
	     }
	     else
	     {

	      map_view_btn_stop_clicked(widget,user_data);

	     }
	 }
	 if((event->x < 800 && event->x > 584) && (event->y > 346 && event->y < 480)){


	  DEBUG("PAUSE BUTTON");
	   if(self->activity_state == MAP_VIEW_ACTIVITY_STATE_STARTED || self->activity_state == MAP_VIEW_ACTIVITY_STATE_PAUSED){
	    map_view_btn_start_pause_clicked(widget,user_data);
	   }
	 }

    }
}
else{
    gtk_window_unfullscreen ((GtkWindow*)self->win);
    gtk_widget_set_size_request(self->map, 800, 420);
    self->fullscreen = FALSE;
  
}


    add_hide_buttons_timeout(self);

    DEBUG_END();

 return FALSE;


}

gboolean
map_button_release_cb (GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{

      MapView *self = (MapView *)user_data;
     if(user_data == NULL){ return FALSE;}
      DEBUG_BEGIN();
    float lat,lon;
   // GtkEntry *entry = GTK_ENTRY(user_data);
    OsmGpsMap *map = OSM_GPS_MAP(widget);

    g_object_get(map, "latitude", &lat, "longitude", &lon, NULL);
   // gchar *msg = g_strdup_printf("%f,%f",lat,lon);
    //gtk_entry_set_text(entry, msg);
    //g_free(msg);
DEBUG_END();
    return FALSE;


    }

/** Hildon menu example **/
static HildonAppMenu *create_menu (MapView *self)
{

   const gchar* maps [] = {
  "Open Street Map",
  "Open Street Map Renderer",
  "Open Aerial Map",
  "Maps For Free",
  "Google Street",
  "Google Satellite",
  "Google Hybrid",
  "Virtual Earth Street",
  "Virtual Earth Satellite",
  "Virtual Earth Hybrid",
  NULL
};

  int i;
  gchar *command_id;
  GtkWidget *button;
  GtkWidget *filter;
  GtkWidget *center_button;
  GtkWidget *about_button;
  GtkWidget *help_button;
  GtkWidget *personal_button;
  GtkWidget *note_button;
  HildonAppMenu *menu = HILDON_APP_MENU (hildon_app_menu_new ());

  button = hildon_button_new_with_text((HildonSizeType)(HILDON_SIZE_AUTO_WIDTH | HILDON_SIZE_FINGER_HEIGHT),
			HILDON_BUTTON_ARRANGEMENT_VERTICAL,
				    "Map Source",
				      maps[self->map_provider-1]);

  g_signal_connect_after (button, "clicked", G_CALLBACK (select_map_source_cb), self);

  /* Add entry to the view menu */
  hildon_app_menu_append (menu, (GtkButton*)(button));


  center_button = hildon_check_button_new((HildonSizeType)(HILDON_SIZE_AUTO_WIDTH | HILDON_SIZE_FINGER_HEIGHT));

  gtk_button_set_label (GTK_BUTTON (center_button), "Auto Centering");
  hildon_check_button_set_active(HILDON_CHECK_BUTTON(center_button),TRUE);
  hildon_app_menu_append (menu, GTK_BUTTON(center_button));
  g_signal_connect_after(center_button, "toggled", G_CALLBACK (toggle_map_centering), self);

  personal_button = gtk_button_new_with_label ("Personal Data");
  g_signal_connect_after (personal_button, "clicked", G_CALLBACK (personal_data_dlg),self);
  hildon_app_menu_append (menu, GTK_BUTTON (personal_button));

  note_button = gtk_button_new_with_label ("Add Note");
  g_signal_connect_after (note_button, "clicked", G_CALLBACK (add_note_cb),self);
  hildon_app_menu_append (menu, GTK_BUTTON (note_button));


  about_button = gtk_button_new_with_label ("About");

  hildon_app_menu_append (menu, GTK_BUTTON (about_button));
  g_signal_connect_after (about_button, "clicked", G_CALLBACK (about_dlg), self);
  /*help_button = gtk_button_new_with_label ("Help");
  hildon_app_menu_append (menu, GTK_BUTTON (help_button));*/
  gtk_widget_show_all (GTK_WIDGET (menu));

  return menu;
}

static void add_note_cb(HildonButton *button, gpointer user_data)
{
	MapView *self = (MapView *)user_data;
	 GtkWidget *dialog;
	 GtkWidget *text_view;
	 GtkWidget *content;
	 GtkTextBuffer* buffer;
	 GtkTextIter start, end;
	 gint response;
	 DEBUG_BEGIN();

	 dialog = gtk_dialog_new_with_buttons ("Add Note",
	                                         self->parent_window,
	                                         GTK_DIALOG_DESTROY_WITH_PARENT,
	                                         "OK",
	                                         GTK_RESPONSE_ACCEPT,
	                                         NULL);

	 content = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
	 text_view = hildon_text_view_new();

	 buffer = hildon_text_view_get_buffer (HILDON_TEXT_VIEW(text_view));
	 gtk_text_buffer_set_text(buffer,self->activity_comment,g_utf8_strlen(self->activity_comment,1000));

	 gtk_container_add (GTK_CONTAINER (content), text_view);
	 gtk_widget_set_size_request(dialog,800,250);
	 gtk_widget_show_all (dialog);

	 response = gtk_dialog_run(GTK_DIALOG(dialog));

	 		if(response == GTK_RESPONSE_ACCEPT)
	 		{
	 			g_free(self->activity_comment);
	 			gtk_text_buffer_get_start_iter (buffer, &start);
	 			gtk_text_buffer_get_end_iter (buffer, &end);
	 			self->activity_comment = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
	 			track_helper_set_comment(self->track_helper, self->activity_comment);
	 			gtk_widget_destroy(dialog);

	 		}
	 		gtk_widget_destroy(dialog);
	DEBUG_END();
}


void
select_map_source_cb (HildonButton *button, gpointer user_data)
{
    MapView *self = (MapView *)user_data;
       const gchar* maps [] = {
	"Open Street Map",
	"Open Street Map Renderer",
	"Open Aerial Map",
	"Maps For Free",
	"Google Street",
	"Google Satellite",
	"Google Hybrid",
	"Virtual Earth Street",
	"Virtual Earth Satellite",
	"Virtual Earth Hybrid"
	};
    DEBUG_BEGIN();


    GtkWidget *selector;
    GtkWidget *dialog = gtk_dialog_new();


    gtk_window_set_title(GTK_WINDOW(dialog),"Select map source");
    gtk_widget_set_size_request(dialog, 800, 390);
    selector = hildon_touch_selector_new_text();
    hildon_touch_selector_set_column_selection_mode (HILDON_TOUCH_SELECTOR (selector),
    HILDON_TOUCH_SELECTOR_SELECTION_MODE_SINGLE);


    for( int j = 0;j<10;j++){
    hildon_touch_selector_append_text (HILDON_TOUCH_SELECTOR (selector),maps[j]);
    }
    hildon_touch_selector_set_active(HILDON_TOUCH_SELECTOR(selector),0,self->map_provider-1);
    hildon_touch_selector_center_on_selected(HILDON_TOUCH_SELECTOR(selector));
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),selector);
    g_signal_connect (G_OBJECT (selector), "changed", G_CALLBACK(map_source_selected),user_data);
    gtk_widget_show_all(dialog);
    DEBUG_END();
}

void map_source_selected(HildonTouchSelector * selector, gint column, gpointer user_data){

    MapView *self = (MapView *)user_data;
    g_return_if_fail(self != NULL);
    DEBUG_BEGIN();
    gint active = hildon_touch_selector_get_active(selector,column);
    active++;
    DEBUG("SELECTED MAP INDEX %d",active);
    gconf_helper_set_value_int_simple(self->gconf_helper,MAP_SOURCE,active);
    hildon_banner_show_information(GTK_WIDGET(self->parent_window), NULL, "Selected map source will be used next time you start eCoach");
    DEBUG_END();
}

static gboolean _hide_buttons_timeout(gpointer user_data)
{
	MapView *self = (MapView *)user_data;
	self->hide_buttons_timeout_id = 0;
       DEBUG_BEGIN();
	/* Piilota napit */
      DEBUG("HIDE BUTTONS TIMEOUT");
      osm_gps_map_hide_buttons(OSM_GPS_MAP(self->map));
      gtk_window_fullscreen ((GtkWindow*)self->win);
      gtk_widget_set_size_request(self->map,800,480);
	/* Palauta aina FALSE -> kutsutaan vain kerran */
      self->fullscreen = TRUE;
       DEBUG_END();
      return FALSE;
}
static void add_hide_buttons_timeout(gpointer user_data)
{

    MapView *self = (MapView *)user_data;
    g_return_if_fail(self != NULL);
     DEBUG_BEGIN();

      osm_gps_map_show_buttons(OSM_GPS_MAP(self->map));
      gtk_window_unfullscreen ((GtkWindow*)self->win);
      gtk_widget_set_size_request(self->map,800,420);
    /* Poista edellinen timeout, jos se on jo asetettu */
	if(self->hide_buttons_timeout_id)
	{
		g_source_remove(self->hide_buttons_timeout_id);
	}
	self->hide_buttons_timeout_id = g_timeout_add(self->buttons_hide_timeout, _hide_buttons_timeout, self);
   DEBUG_END();
}

static void
toggle_map_centering(HildonCheckButton *button, gpointer user_data)
{
   MapView *self = (MapView *)user_data;
   DEBUG_BEGIN();
    gboolean active;

    active = hildon_check_button_get_active (button);
    if (active)
       self->is_auto_center = TRUE;
    else
       self->is_auto_center = FALSE;

    DEBUG_END();
}
static void destroy_personal_data_dlg (GtkWidget *widget,GdkEvent  *event,gpointer user_data)
{
  MapView *self = (MapView *)user_data;
  DEBUG_BEGIN();
  gtk_widget_destroy(self->personal_win);
  if(self->fullscreen){
      osm_gps_map_show_buttons(OSM_GPS_MAP(self->map));
      gtk_window_unfullscreen ((GtkWindow*)self->win);
      gtk_widget_set_size_request(self->map,800,420);
    self->fullscreen = FALSE;
  }
  DEBUG_END();
}
static void personal_data_dlg(HildonButton *button, gpointer user_data){

	
	GtkWidget *fixed;
	GtkWidget *scale;
	GtkWidget *anaerobic;
	GtkWidget *aerobic;
	GtkWidget *weight_control;
	GtkWidget *moder_ex;
	GtkWidget *lbl_anaero;
	GtkWidget *lbl_aero;
	GtkWidget *lbl_wc;
	GtkWidget *lbl_me;
	GtkWidget *lbl_age;
	GtkWidget *lbl_maxhr;
	GtkWidget *lbl_bmi;
	gint age;
	gint weight;
	gint height;
	gdouble bmi;
	gchar *str;
	PangoFontDescription *desc = NULL;
	MapView *self = (MapView *)user_data;
	g_return_if_fail(self != NULL);


DEBUG_BEGIN();

scale = gtk_image_new_from_file(GFXDIR "personal_data_bmp_scale.png");
anaerobic = gtk_image_new_from_file(GFXDIR "personal_data_button.png");
aerobic = gtk_image_new_from_file(GFXDIR "personal_data_button.png");
weight_control = gtk_image_new_from_file(GFXDIR "personal_data_button.png");
moder_ex = gtk_image_new_from_file(GFXDIR "personal_data_button.png");

self->personal_win = hildon_stackable_window_new();
g_signal_connect(self->personal_win, "delete-event", G_CALLBACK(destroy_personal_data_dlg), self);

gtk_window_set_title ( GTK_WINDOW (self->personal_win), "eCoach >Personal Data");
fixed = gtk_fixed_new();

desc = pango_font_description_new();
pango_font_description_set_family(desc, "Nokia Sans");
pango_font_description_set_absolute_size(desc, 29 * PANGO_SCALE);

age = gconf_helper_get_value_int_with_default(self->gconf_helper,USER_AGE,0);
height = gconf_helper_get_value_int_with_default(self->gconf_helper,USER_HEIGHT,0);
weight = gconf_helper_get_value_int_with_default(self->gconf_helper,USER_WEIGHT,0);

str = g_strdup_printf(_("Age: %d"), age);
lbl_age =gtk_label_new(str);
g_free(str);

gint maxhr;
if(age >0){
maxhr = 206.3-(0.711*age);
}
else
{maxhr = 0;}

str = g_strdup_printf(_("Max HR: %d"), maxhr);

lbl_maxhr =gtk_label_new(str);
g_free(str);
DEBUG("PAINO: %d",weight);
DEBUG("PITUUS: %d",height);

if(weight >0)
{

	bmi = ((gdouble)weight/((gdouble)height*(gdouble)height) *10000);
	DEBUG("BMI: %f",bmi);

}
else{

	bmi = 0;
}

str = g_strdup_printf(_("BMI: %.1f"), bmi);
lbl_bmi =gtk_label_new(str);
g_free(str);


gtk_fixed_put(GTK_FIXED(fixed),lbl_age,280, 35);
gtk_fixed_put(GTK_FIXED(fixed),lbl_maxhr,380, 35);
gtk_fixed_put(GTK_FIXED(fixed),lbl_bmi,530, 35);
gtk_fixed_put(GTK_FIXED(fixed),scale,18, 125);

gtk_fixed_put(GTK_FIXED(fixed),anaerobic,116, 85);
lbl_anaero = gtk_label_new("Anaerobic");
gtk_widget_modify_font(lbl_anaero,desc);
gtk_fixed_put(GTK_FIXED(fixed),lbl_anaero,132, 209);

gtk_fixed_put(GTK_FIXED(fixed),aerobic,290, 85);
lbl_aero =gtk_label_new("Aerobic");
gtk_widget_modify_font(lbl_aero,desc);
gtk_fixed_put(GTK_FIXED(fixed),lbl_aero,317, 209);


gtk_fixed_put(GTK_FIXED(fixed),weight_control,464, 85);
lbl_wc =gtk_label_new("Weight\ncontrol");
gtk_widget_modify_font(lbl_wc,desc);
gtk_misc_set_alignment(GTK_MISC(lbl_wc), 0.5f, 0.5f);
gtk_fixed_put(GTK_FIXED(fixed),lbl_wc,494, 196);


gtk_fixed_put(GTK_FIXED(fixed),moder_ex,638, 85);
lbl_me =gtk_label_new("Moderate\n exercise");

gtk_widget_modify_font(lbl_me,desc);
gtk_misc_set_alignment(GTK_MISC(lbl_me), 0.5f, 0.5f);
gtk_fixed_put(GTK_FIXED(fixed),lbl_me,658, 196);


pango_font_description_set_family(desc, "Nokia Sans");
pango_font_description_set_absolute_size(desc, 50 * PANGO_SCALE);


gint value = 100;
gint position = 175;
gint position_v = 155;
gchar *text;
gchar *hr_value;
for(int i=0; i<4;i++){
	GtkWidget *percent;
	GtkWidget *lbl_hr;
	gint hr;
	value = value-10;
	text = g_strdup_printf(_("%d%%"), value);
	hr_value = g_strdup_printf(_("%d"), ((value*maxhr)/100));
	percent =gtk_label_new(text);
	lbl_hr =gtk_label_new(hr_value);
	g_free(text);
	g_free(hr_value);
	gtk_widget_modify_font(lbl_hr,desc);
	gtk_fixed_put(GTK_FIXED(fixed),percent,position, 155);
	gtk_fixed_put(GTK_FIXED(fixed),lbl_hr,position_v, 100);

	text = g_strdup_printf(_("%d%%"), (value-10));
	hr_value = g_strdup_printf(_("%d"), (((value-10)*maxhr)/100));
	lbl_hr =gtk_label_new(hr_value);
	percent =gtk_label_new(text);
	gtk_widget_modify_font(lbl_hr,desc);
	gtk_fixed_put(GTK_FIXED(fixed),lbl_hr,position_v, 275);
	gtk_fixed_put(GTK_FIXED(fixed),percent,position, 330);
	g_free(hr_value);
	g_free(text);
	position = position +170;
	position_v = position_v +170;
	if(i>1)
	{
		position = position +10;
		position_v = position_v +10;
	}

}


gtk_container_add (GTK_CONTAINER (self->personal_win),fixed);
gtk_widget_show_all(self->personal_win);
	DEBUG_END();

}
static void about_dlg(HildonButton *button, gpointer user_data){

  MapView *self = (MapView *)user_data;
  DEBUG_BEGIN();
    gtk_show_about_dialog((GtkWindow*)self->win
                , "name",      "eCoach"
		, "logo-icon-name",	"ecoach"
                , "version",   "1.54"
                , "copyright", "Sampo Savola <samposav@paju.oulu.fi>\n"
		"Jukka Alasalmi\n Veli-Pekka Haajanen\n Kai Skiftesvik"
                , "website",   "http://ecoach.garage.maemo.org"
		 ,"wrap-license", TRUE
		 , "license","eCoach comes with ABSOLUTELY NO WARRANTY. This is "
		"free software, and you are welcome to redistribute it under "
		"certain conditions. Read below for details.\n\nThis software contains components from\n"
		"OSM GPS MAP\n"
		"http://nzjrs.github.com/osm-gps-map/\n"
		"Open Source ECG Analysis Software (OSEA)\n"
		"[http://eplimited.com/]\n and \nPhysioToolkit\n"
		"[http://www.physionet.org/physiotools/wfdb.shtml]\n"
		"WFDB library (ecgcodes.h)\n\n"
		"\tCopyright (C) 1999 George B. Moody\n"
		"OSEA\n"
		"\tCopywrite (C) 2000-2002 Patrick S. Hamilton\n\n"
		"                    GNU GENERAL PUBLIC LICENSE\n"
"                       Version 2, June 1991\n"
"\n"
" Copyright (C) 1989, 1991 Free Software Foundation, Inc.\n"
"     59 Temple Place, Suite 330, Boston, MA  02111-1307  USA\n"
" Everyone is permitted to copy and distribute verbatim copies\n"
" of this license document, but changing it is not allowed.\n"
"\n"
"                            Preamble\n"
"\n"
"  The licenses for most software are designed to take away your\n"
"freedom to share and change it.  By contrast, the GNU General Public\n"
"License is intended to guarantee your freedom to share and change free\n"
"software--to make sure the software is free for all its users.  This\n"
"General Public License applies to most of the Free Software\n"
"Foundation's software and to any other program whose authors commit to\n"
"using it.  (Some other Free Software Foundation software is covered by\n"
"the GNU Library General Public License instead.)  You can apply it to\n"
"your programs, too.\n"
"\n"
"  When we speak of free software, we are referring to freedom, not\n"
"price.  Our General Public Licenses are designed to make sure that you\n"
"have the freedom to distribute copies of free software (and charge for\n"
"this service if you wish), that you receive source code or can get it\n"
"if you want it, that you can change the software or use pieces of it\n"
"in new free programs; and that you know you can do these things.\n"
"\n"
"  To protect your rights, we need to make restrictions that forbid\n"
"anyone to deny you these rights or to ask you to surrender the rights.\n"
"These restrictions translate to certain responsibilities for you if you\n"
"distribute copies of the software, or if you modify it.\n"
"\n"
"  For example, if you distribute copies of such a program, whether\n"
"gratis or for a fee, you must give the recipients all the rights that\n"
"you have.  You must make sure that they, too, receive or can get the\n"
"source code.  And you must show them these terms so they know their\n"
"rights.\n"
"\n"
"  We protect your rights with two steps: (1) copyright the software, and\n"
"(2) offer you this license which gives you legal permission to copy,\n"
"distribute and/or modify the software.\n"
"\n"
"  Also, for each author's protection and ours, we want to make certain\n"
"that everyone understands that there is no warranty for this free\n"
"software.  If the software is modified by someone else and passed on, we\n"
"want its recipients to know that what they have is not the original, so\n"
"that any problems introduced by others will not reflect on the original\n"
"authors' reputations.\n"
"\n"
"  Finally, any free program is threatened constantly by software\n"
"patents.  We wish to avoid the danger that redistributors of a free\n"
"program will individually obtain patent licenses, in effect making the\n"
"program proprietary.  To prevent this, we have made it clear that any\n"
"patent must be licensed for everyone's free use or not licensed at all.\n"
"\n"
"  The precise terms and conditions for copying, distribution and\n"
"modification follow.\n"
"\n"
"                    GNU GENERAL PUBLIC LICENSE\n"
"   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION\n"
"\n"
"  0. This License applies to any program or other work which contains\n"
"a notice placed by the copyright holder saying it may be distributed\n"
"under the terms of this General Public License.  The \"Program\", below,\n"
"refers to any such program or work, and a \"work based on the Program\"\n"
"means either the Program or any derivative work under copyright law:\n"
"that is to say, a work containing the Program or a portion of it,\n"
"either verbatim or with modifications and/or translated into another\n"
"language.  (Hereinafter, translation is included without limitation in\n"
"the term \"modification\".)  Each licensee is addressed as \"you\".\n"
"\n"
"Activities other than copying, distribution and modification are not\n"
"covered by this License; they are outside its scope.  The act of\n"
"running the Program is not restricted, and the output from the Program\n"
"is covered only if its contents constitute a work based on the\n"
"Program (independent of having been made by running the Program).\n"
"Whether that is true depends on what the Program does.\n"
"\n"
"  1. You may copy and distribute verbatim copies of the Program's\n"
"source code as you receive it, in any medium, provided that you\n"
"conspicuously and appropriately publish on each copy an appropriate\n"
"copyright notice and disclaimer of warranty; keep intact all the\n"
"notices that refer to this License and to the absence of any warranty;\n"
"and give any other recipients of the Program a copy of this License\n"
"along with the Program.\n"
"\n"
"You may charge a fee for the physical act of transferring a copy, and\n"
"you may at your option offer warranty protection in exchange for a fee.\n"
"\n"
"  2. You may modify your copy or copies of the Program or any portion\n"
"of it, thus forming a work based on the Program, and copy and\n"
"distribute such modifications or work under the terms of Section 1\n"
"above, provided that you also meet all of these conditions:\n"
"\n"
"    a) You must cause the modified files to carry prominent notices\n"
"    stating that you changed the files and the date of any change.\n"
"\n"
"    b) You must cause any work that you distribute or publish, that in\n"
"    whole or in part contains or is derived from the Program or any\n"
"    part thereof, to be licensed as a whole at no charge to all third\n"
"    parties under the terms of this License.\n"
"\n"
"    c) If the modified program normally reads commands interactively\n"
"    when run, you must cause it, when started running for such\n"
"    interactive use in the most ordinary way, to print or display an\n"
"    announcement including an appropriate copyright notice and a\n"
"    notice that there is no warranty (or else, saying that you provide\n"
"    a warranty) and that users may redistribute the program under\n"
"    these conditions, and telling the user how to view a copy of this\n"
"    License.  (Exception: if the Program itself is interactive but\n"
"    does not normally print such an announcement, your work based on\n"
"    the Program is not required to print an announcement.)\n"
"\n"
"These requirements apply to the modified work as a whole.  If\n"
"identifiable sections of that work are not derived from the Program,\n"
"and can be reasonably considered independent and separate works in\n"
"themselves, then this License, and its terms, do not apply to those\n"
"sections when you distribute them as separate works.  But when you\n"
"distribute the same sections as part of a whole which is a work based\n"
"on the Program, the distribution of the whole must be on the terms of\n"
"this License, whose permissions for other licensees extend to the\n"
"entire whole, and thus to each and every part regardless of who wrote it.\n"
"\n"
"Thus, it is not the intent of this section to claim rights or contest\n"
"your rights to work written entirely by you; rather, the intent is to\n"
"exercise the right to control the distribution of derivative or\n"
"collective works based on the Program.\n"
"\n"
"In addition, mere aggregation of another work not based on the Program\n"
"with the Program (or with a work based on the Program) on a volume of\n"
"a storage or distribution medium does not bring the other work under\n"
"the scope of this License.\n"
"\n"
"  3. You may copy and distribute the Program (or a work based on it,\n"
"under Section 2) in object code or executable form under the terms of\n"
"Sections 1 and 2 above provided that you also do one of the following:\n"
"\n"
"    a) Accompany it with the complete corresponding machine-readable\n"
"    source code, which must be distributed under the terms of Sections\n"
"    1 and 2 above on a medium customarily used for software interchange; or,\n"
"\n"
"    b) Accompany it with a written offer, valid for at least three\n"
"    years, to give any third party, for a charge no more than your\n"
"    cost of physically performing source distribution, a complete\n"
"    machine-readable copy of the corresponding source code, to be\n"
"    distributed under the terms of Sections 1 and 2 above on a medium\n"
"    customarily used for software interchange; or,\n"
"\n"
"    c) Accompany it with the information you received as to the offer\n"
"    to distribute corresponding source code.  (This alternative is\n"
"    allowed only for noncommercial distribution and only if you\n"
"    received the program in object code or executable form with such\n"
"    an offer, in accord with Subsection b above.)\n"
"\n"
"The source code for a work means the preferred form of the work for\n"
"making modifications to it.  For an executable work, complete source\n"
"code means all the source code for all modules it contains, plus any\n"
"associated interface definition files, plus the scripts used to\n"
"control compilation and installation of the executable.  However, as a\n"
"special exception, the source code distributed need not include\n"
"anything that is normally distributed (in either source or binary\n"
"form) with the major components (compiler, kernel, and so on) of the\n"
"operating system on which the executable runs, unless that component\n"
"itself accompanies the executable.\n"
"\n"
"If distribution of executable or object code is made by offering\n"
"access to copy from a designated place, then offering equivalent\n"
"access to copy the source code from the same place counts as\n"
"distribution of the source code, even though third parties are not\n"
"compelled to copy the source along with the object code.\n"
"\n"
"  4. You may not copy, modify, sublicense, or distribute the Program\n"
"except as expressly provided under this License.  Any attempt\n"
"otherwise to copy, modify, sublicense or distribute the Program is\n"
"void, and will automatically terminate your rights under this License.\n"
"However, parties who have received copies, or rights, from you under\n"
"this License will not have their licenses terminated so long as such\n"
"parties remain in full compliance.\n"
"\n"
"  5. You are not required to accept this License, since you have not\n"
"signed it.  However, nothing else grants you permission to modify or\n"
"distribute the Program or its derivative works.  These actions are\n"
"prohibited by law if you do not accept this License.  Therefore, by\n"
"modifying or distributing the Program (or any work based on the\n"
"Program), you indicate your acceptance of this License to do so, and\n"
"all its terms and conditions for copying, distributing or modifying\n"
"the Program or works based on it.\n"
"\n"
"  6. Each time you redistribute the Program (or any work based on the\n"
"Program), the recipient automatically receives a license from the\n"
"original licensor to copy, distribute or modify the Program subject to\n"
"these terms and conditions.  You may not impose any further\n"
"restrictions on the recipients' exercise of the rights granted herein.\n"
"You are not responsible for enforcing compliance by third parties to\n"
"this License.\n"
"\n"
"  7. If, as a consequence of a court judgment or allegation of patent\n"
"infringement or for any other reason (not limited to patent issues),\n"
"conditions are imposed on you (whether by court order, agreement or\n"
"otherwise) that contradict the conditions of this License, they do not\n"
"excuse you from the conditions of this License.  If you cannot\n"
"distribute so as to satisfy simultaneously your obligations under this\n"
"License and any other pertinent obligations, then as a consequence you\n"
"may not distribute the Program at all.  For example, if a patent\n"
"license would not permit royalty-free redistribution of the Program by\n"
"all those who receive copies directly or indirectly through you, then\n"
"the only way you could satisfy both it and this License would be to\n"
"refrain entirely from distribution of the Program.\n"
"\n"
"If any portion of this section is held invalid or unenforceable under\n"
"any particular circuecance, the balance of the section is intended to\n"
"apply and the section as a whole is intended to apply in other\n"
"circuecances.\n"
"\n"
"It is not the purpose of this section to induce you to infringe any\n"
"patents or other property right claims or to contest validity of any\n"
"such claims; this section has the sole purpose of protecting the\n"
"integrity of the free software distribution system, which is\n"
"implemented by public license practices.  Many people have made\n"
"generous contributions to the wide range of software distributed\n"
"through that system in reliance on consistent application of that\n"
"system; it is up to the author/donor to decide if he or she is willing\n"
"to distribute software through any other system and a licensee cannot\n"
"impose that choice.\n"
"\n"
"This section is intended to make thoroughly clear what is believed to\n"
"be a consequence of the rest of this License.\n"
"\n"
"  8. If the distribution and/or use of the Program is restricted in\n"
"certain countries either by patents or by copyrighted interfaces, the\n"
"original copyright holder who places the Program under this License\n"
"may add an explicit geographical distribution limitation excluding\n"
"those countries, so that distribution is permitted only in or among\n"
"countries not thus excluded.  In such case, this License incorporates\n"
"the limitation as if written in the body of this License.\n"
"\n"
"  9. The Free Software Foundation may publish revised and/or new versions\n"
"of the General Public License from time to time.  Such new versions will\n"
"be similar in spirit to the present version, but may differ in detail to\n"
"address new problems or concerns.\n"
"\n"
"Each version is given a distinguishing version number.  If the Program\n"
"specifies a version number of this License which applies to it and \"any\n"
"later version\", you have the option of following the terms and conditions\n"
"either of that version or of any later version published by the Free\n"
"Software Foundation.  If the Program does not specify a version number of\n"
"this License, you may choose any version ever published by the Free Software\n"
"Foundation.\n"
"\n"
"  10. If you wish to incorporate parts of the Program into other free\n"
"programs whose distribution conditions are different, write to the author\n"
"to ask for permission.  For software which is copyrighted by the Free\n"
"Software Foundation, write to the Free Software Foundation; we sometimes\n"
"make exceptions for this.  Our decision will be guided by the two goals\n"
"of preserving the free status of all derivatives of our free software and\n"
"of promoting the sharing and reuse of software generally.\n"
"\n"
"                            NO WARRANTY\n"
"\n"
"  11. BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY\n"
"FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.  EXCEPT WHEN\n"
"OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES\n"
"PROVIDE THE PROGRAM \"AS IS\" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED\n"
"OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF\n"
"MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE ENTIRE RISK AS\n"
"TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU.  SHOULD THE\n"
"PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING,\n"
"REPAIR OR CORRECTION.\n"
"\n"
"  12. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING\n"
"WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY MODIFY AND/OR\n"
"REDISTRIBUTE THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES,\n"
"INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING\n"
"OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED\n"
"TO LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY\n"
"YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER\n"
"PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE\n"
"POSSIBILITY OF SUCH DAMAGES.\n"
"\n"
"                     END OF TERMS AND CONDITIONS\n"
"\n"
"            How to Apply These Terms to Your New Programs\n"
"\n"
"  If you develop a new program, and you want it to be of the greatest\n"
"possible use to the public, the best way to achieve this is to make it\n"
"free software which everyone can redistribute and change under these terms.\n"
"\n"
"  To do so, attach the following notices to the program.  It is safest\n"
"to attach them to the start of each source file to most effectively\n"
"convey the exclusion of warranty; and each file should have at least\n"
"the \"copyright\" line and a pointer to where the full notice is found.\n"
"\n"
"    <one line to give the program's name and a brief idea of what it does.>\n"
"    Copyright (C) <year>  <name of author>\n"
"\n"
"    This program is free software; you can redistribute it and/or modify\n"
"    it under the terms of the GNU General Public License as published by\n"
"    the Free Software Foundation; either version 2 of the License, or\n"
"    (at your option) any later version.\n"
"\n"
"    This program is distributed in the hope that it will be useful,\n"
"    but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"    GNU General Public License for more details.\n"
"\n"
"    You should have received a copy of the GNU General Public License\n"
"    along with this program; if not, write to the Free Software\n"
"    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA\n"
"\n"
"\n"
"Also add information on how to contact you by electronic and paper mail.\n"
"\n"
"If the program is interactive, make it output a short notice like this\n"
"when it starts in an interactive mode:\n"
"\n"
"    Gnomovision version 69, Copyright (C) year  name of author\n"
"    Gnomovision comes with ABSOLUTELY NO WARRANTY; for details type `show w'.\n"
"    This is free software, and you are welcome to redistribute it\n"
"    under certain conditions; type `show c' for details.\n"
"\n"
"The hypothetical commands `show w' and `show c' should show the appropriate\n"
"parts of the General Public License.  Of course, the commands you use may\n"
"be called something other than `show w' and `show c'; they could even be\n"
"mouse-clicks or menu items--whatever suits your program.\n"
"\n"
"You should also get your employer (if you work as a programmer) or your\n"
"school, if any, to sign a \"copyright disclaimer\" for the program, if\n"
"necessary.  Here is a sample; alter the names:\n"
"\n"
"  Yoyodyne, Inc., hereby disclaims all copyright interest in the program\n"
"  `Gnomovision' (which makes passes at compilers) written by James Hacker.\n"
"\n"
"  <signature of Ty Coon>, 1 April 1989\n"
"  Ty Coon, President of Vice\n"
"\n"
"This General Public License does not permit incorporating your program into\n"
"proprietary programs.  If your program is a subroutine library, you may\n"
"consider it more useful to permit linking proprietary applications with the\n"
"library.  If this is what you want to do, use the GNU Library General\n"
"Public License instead of this License.\n"
                , NULL );


  DEBUG_END();

}
static void info_speed_clicked(GtkWidget *button, gpointer user_data)
{
	MapView *self = (MapView *)user_data;
	DEBUG_BEGIN();
	if(self->show_min_per_km){
	self->show_min_per_km = FALSE;
	}else
	{
	  self->show_min_per_km = TRUE;
	}
	map_view_update_stats(self);  
	DEBUG_END();
}
