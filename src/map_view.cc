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
static gboolean map_view_load_map_idle(gpointer user_data);
static void map_view_setup_progress_indicator(EcProgress *prg);
static void map_view_zoom_in(GtkWidget *widget, gpointer user_data);
static void map_view_zoom_out(GtkWidget *widget, gpointer user_data);

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

static void map_view_show_map_widget(
		LocationGPSDevice *device,
		gpointer user_data);

static void map_view_location_changed(
		LocationGPSDevice *device,
		gpointer user_data);

static void map_view_check_and_add_route_point(
		MapView *self,
		MapPoint *point,
		LocationGPSDeviceFix *fix);

static void map_view_scroll(MapView *self, MapViewDirection direction);
static void map_view_scroll_up(GtkWidget *btn, gpointer user_data);
static void map_view_scroll_down(GtkWidget *btn, gpointer user_data);
static void map_view_scroll_left(GtkWidget *btn, gpointer user_data);
static void map_view_scroll_right(GtkWidget *btn, gpointer user_data);
static void map_view_btn_back_clicked(GtkWidget *button, gpointer user_data);
static void map_view_btn_start_pause_clicked(GtkWidget *button,
		gpointer user_data);
static void map_view_btn_stop_clicked(GtkWidget *button, gpointer user_data);
static void map_view_btn_autocenter_clicked(GtkWidget *button,
		gpointer user_data);
static void map_view_start_activity(MapView *self);
static gboolean map_view_update_stats(gpointer user_data);
static void map_view_set_elapsed_time(MapView *self, struct timeval *tv);
static void map_view_pause_activity(MapView *self);
static void map_view_continue_activity(MapView *self);
static void map_view_set_travelled_distance(MapView *self, gdouble distance);
static void map_view_update_autocenter(MapView *self);
static void map_view_units_clicked(GtkWidget *button, gpointer user_data);
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
	
	self->info_speed = gtk_label_new("Speed");
	self->info_distance = gtk_label_new("Distance");;	
	self->info_time = gtk_label_new("Time");		
	self->info_avg_speed = gtk_label_new("Average Speed");
	self->info_heart_rate = gtk_label_new("HR");

	
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
	
	
	//self->map_widget_state = MAP_VIEW_MAP_WIDGET_STATE_NOT_CONFIGURED;
	//self->has_gps_fix = FALSE;
	 
	
	
	

	
	
	if((self->activity_state == MAP_VIEW_ACTIVITY_STATE_STOPPED) ||
	   (self->activity_state == MAP_VIEW_ACTIVITY_STATE_NOT_STARTED))
	{
	osm_gps_map_add_button((OsmGpsMap*)self->map,394, 346, self->rec_btn_unselected);
	osm_gps_map_add_button((OsmGpsMap*)self->map,557, 346, self->pause_btn_unselected);
	}
	else{
	osm_gps_map_add_button((OsmGpsMap*)self->map,394, 346, self->rec_btn_selected);
	
	}
	if(self->activity_state == MAP_VIEW_ACTIVITY_STATE_PAUSED)
	{
	osm_gps_map_add_button((OsmGpsMap*)self->map,557, 346, self->pause_btn_selected);
	}
		    

		    

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
	
	g_signal_connect(G_OBJECT(self->gps_device), "changed",
			G_CALLBACK(map_view_location_changed), self);
	self->show_map_widget_handler_id = g_signal_connect(
			G_OBJECT(self->gps_device), "changed",
			G_CALLBACK(map_view_show_map_widget), self);
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
		self->has_gps_fix = FALSE;
		self->map_widget_state = MAP_VIEW_MAP_WIDGET_STATE_CONFIGURING;
	//	g_idle_add(map_view_load_map_idle, self);
	} else {
#if (MAP_VIEW_SIMULATE_GPS)
		self->show_map_widget_handler_id = 1;
#else
		self->show_map_widget_handler_id = g_signal_connect(
			G_OBJECT(self->gps_device), "changed",
			G_CALLBACK(map_view_show_map_widget), self);
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

		

	//	self->has_gps_fix = FALSE;
	//	map_view_hide_map_widget(self);
	//	track_helper_clear(self->track_helper, FALSE);
	// ei	map_widget_clear_track(self->map_widget);
	//	map_view_update_stats(self);
	//	location_gpsd_control_stop(self->gpsd_control);
		
	}
	//gtk_widget_hide_all(self->win);
	DEBUG_END();
}

MapViewActivityState map_view_get_activity_state(MapView *self)
{
	g_return_val_if_fail(self != NULL, MAP_VIEW_ACTIVITY_STATE_NOT_STARTED);
	DEBUG_BEGIN();

	return self->activity_state;

	DEBUG_END();
}
static void map_view_create_data(MapView *self){
  
 self->data_win = hildon_stackable_window_new();
 self->data_widget = gtk_fixed_new(); 
 g_signal_connect(self->data_win, "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);
 
 gtk_fixed_put(GTK_FIXED(self->data_widget),self->info_distance,353, 100);
 gtk_fixed_put(GTK_FIXED(self->data_widget),self->info_speed,353, 120);
 gtk_fixed_put(GTK_FIXED(self->data_widget),self->info_time,353, 140);
 gtk_fixed_put(GTK_FIXED(self->data_widget),self->info_avg_speed,353, 160);
 gtk_fixed_put(GTK_FIXED(self->data_widget),self->info_heart_rate,353, 180);
 
 gtk_container_add (GTK_CONTAINER (self->data_win),self->data_widget);	
 	
  
}
static void map_view_show_data(MapView *self){

  gtk_widget_show_all( self->data_win);
}


void map_view_stop(MapView *self)
{
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	if((self->activity_state == MAP_VIEW_ACTIVITY_STATE_STOPPED) ||
	   (self->activity_state == MAP_VIEW_ACTIVITY_STATE_NOT_STARTED))
	{
		DEBUG_END();
		return;
	}
	
	//for calendar 
	
	time(&self->end);
	//gdouble distance = track_helper_get_travelled_distance(self->track_helper);
	//guint time = track_helper_get_elapsed_time(self->track_helper);
	//gdouble average = track_helper_get_average_speed(self->track_helper);
	
	//DEBUG("TRAVELED %d, %d seconds %d speed",distance,time,average);
	const gchar *time = gtk_label_get_text((GtkLabel*)self->info_time);
	const gchar *dist = gtk_label_get_text((GtkLabel*)self->info_distance);
	const gchar *avgspeed = gtk_label_get_text((GtkLabel*)self->info_avg_speed);
	gchar *max_speed = g_strdup_printf(_("%.1f km/h"), self->max_speed);
	
	CCalendarUtil *util;
	if(self->add_calendar){
	util->addEvent(self->activity_name,"",g_strdup_printf("Duration: %s\nDistance: %s\nAvg.speed %s\nMax.Speed: %s \n\
	Comment: %s \nGPX File: %s",time,dist,avgspeed,max_speed,self->activity_comment,self->file_name),self->start,self->end);
	}
	
	track_helper_stop(self->track_helper);
	
	
	
	
	
	
	
	track_helper_clear(self->track_helper, FALSE);
	self->activity_state = MAP_VIEW_ACTIVITY_STATE_STOPPED;
	self->max_speed =0;
	g_source_remove(self->activity_timer_id);
	self->activity_timer_id = 0;

	//ec_button_set_bg_image(EC_BUTTON(self->btn_start_pause),
	//		EC_BUTTON_STATE_RELEASED,
	//		GFXDIR "ec_button_rec.png");

	DEBUG_END();
}

void map_view_clear_all(MapView *self)
{
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	track_helper_clear(self->track_helper, TRUE);
	map_view_update_stats(self);
	//map_widget_clear_track(self->map_widget);

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
			ec_error_show_message_error_ask_dont_show_again(
				_("Heart rate monitor is not configured.\n"
					"To get heart rate data, please\n"
					"configure heart rate monitor in\n"
					"the settings menu."),
				ECGC_HRM_DIALOG_SHOWN,
				FALSE
				);
		} else {
			ec_error_show_message_error(error->message);
		}

	//	ec_button_set_label_text(EC_BUTTON(self->info_heart_rate),
	//			_("N/A"));
	      gtk_label_set_text(GTK_LABEL(self->info_heart_rate),"N/A");
	      
		gdk_threads_leave();
		DEBUG_END();
		return FALSE;
	}

	DEBUG_END();

	/* This only needs to be done once */
	return FALSE;
}
/*
static gboolean map_view_load_map_idle(gpointer user_data)
{
	MapView *self = (MapView *)user_data;	
	MapPoint center;

	g_return_val_if_fail(self != NULL, FALSE);
	DEBUG_BEGIN();

	center.latitude = 51.50;
	center.longitude = -.1;*/
	/** @todo Save the latitude & longitude to gconf */

//	gboolean is_running = FALSE;

	/* Start maptile-loader if it is not already running */
/*	if(system("pidof " MAPTILE_LOADER_EXEC_NAME) != 0)
	{
		DEBUG_LONG("Starting maptile-loader process");
		system(MAPTILE_LOADER_EXEC_NAME " &");
	} else {
		DEBUG_LONG("maptile-loader already running");
		is_running = TRUE;
	}

	gint i = 0;
	struct timespec req;
	struct timespec rem;
	while(!is_running && i < 10)
	{
		i++;
		req.tv_sec = 1;
		req.tv_nsec = 0;
		nanosleep(&req, &rem);
		if(system("pidof " MAPTILE_LOADER_EXEC_NAME) == 0)
		{
			DEBUG("maptile-loader is now running");
			is_running = TRUE;
		} else {
			DEBUG("Waiting for maptile-loader to start...");
		}
	}
	if(!is_running)
	{
		*///DEBUG("Maptile-loader did not start");
		/** @todo Handle the error */
//	}

//	gdk_threads_enter();

	
	/* Create and setup the map widget */
	/*
	gtk_notebook_set_current_page(GTK_NOTEBOOK(self->notebook_map),
			self->page_id_gps_status);
*/
	//self->map_widget = map_widget_create();
	//gtk_widget_set_size_request(GTK_WIDGET(self->map_widget), 760, 425);
/*	self->page_id_map_widget = gtk_notebook_append_page(
			GTK_NOTEBOOK(self->notebook_map),
			self->map_widget, NULL);
*/
//	gtk_fixed_put(GTK_FIXED(self->main_widget),
//			self->map_widget, 0, 0);
			
	
	
	//gtk_widget_set_size_request(self->notebook_map, 800, 410);

//	map_widget_new_from_center_zoom_type(self->map_widget, &center, 15.0,
//			MAP_ORIENTATION_NORTH, "Open Street",
//			self->osso);

	

//	self->map_widget_state = MAP_VIEW_MAP_WIDGET_STATE_CONFIGURED;
//	map_view_update_autocenter(self);
	
//	gtk_widget_show_all(self->main_widget);	

//	gdk_threads_leave();

	/* This only needs to be done once */
//	return FALSE;
//}

static gboolean fixed_expose_event(
		GtkWidget *widget,
		GdkEventExpose *event,
		gpointer user_data)
{
	MapView *self = (MapView *)user_data;
	gint i, count;
	GdkRectangle *rectangle;

	g_return_val_if_fail(self != NULL, FALSE);

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
		text = g_strdup_printf(_("%d bpm"), (gint)heart_rate);
	//	ec_button_set_label_text(
	//			EC_BUTTON(self->info_heart_rate),
	  //			text);
		gtk_label_set_text(GTK_LABEL(self->info_heart_rate),text);
		g_free(text);

		map_view_update_heart_rate_icon(self, heart_rate);

		if(self->activity_state == MAP_VIEW_ACTIVITY_STATE_STARTED)
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

static void map_view_zoom_in(GtkWidget *widget, gpointer user_data)
{
	guint zoom;
	MapView *self = (MapView *)user_data;

	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();
/*
	zoom = map_widget_get_current_zoom_level(self->map_widget);
	if(zoom >= 17)
	{
		DEBUG_END();
		return;
	}
*/	hildon_banner_show_information(GTK_WIDGET(self->parent_window), NULL, "Zoom in");
	zoom = zoom + 1;
//	map_widget_set_zoom(self->map_widget, zoom);
	
	DEBUG_END();
}

static void map_view_zoom_out(GtkWidget *widget, gpointer user_data)
{
	guint zoom;
	MapView *self = (MapView *)user_data;

	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

//	zoom = map_widget_get_current_zoom_level(self->map_widget);
	if(zoom <= 1)
	{
		DEBUG_END();
		return;
	}
	zoom = zoom - 1;
	hildon_banner_show_information(GTK_WIDGET(self->parent_window), NULL, "Zoom out");
//	map_widget_set_zoom(self->map_widget, zoom);
	
	DEBUG_END();
}

static void map_view_hide_map_widget(MapView *self)
{
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

//	gtk_notebook_set_current_page(GTK_NOTEBOOK(self->notebook_map),
//			self->page_id_gps_status);

	DEBUG_END();
}

static void map_view_show_map_widget(
		LocationGPSDevice *device,
		gpointer user_data)
{
	MapView *self = (MapView *)user_data;

	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

#if (MAP_VIEW_SIMULATE_GPS == 0)
	g_signal_handler_disconnect(self->gps_device,
			self->show_map_widget_handler_id);
#endif

	self->show_map_widget_handler_id = 0;

//	gtk_notebook_set_current_page(GTK_NOTEBOOK(self->notebook_map),
//			self->page_id_map_widget);

	DEBUG_END();
}

static void map_view_location_changed(
		LocationGPSDevice *device,
		gpointer user_data)
{
	MapPoint point;
	MapView *self = (MapView *)user_data;
	//gboolean map_widget_ready = TRUE;
	
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	/* Keep display on */
	if(gconf_helper_get_value_bool_with_default(self->gconf_helper,
						    DISPLAY_ON,TRUE)) {
		osso_display_state_on(self->osso);
		osso_display_blanking_pause(self->osso);
	}

	DEBUG("Latitude: %.2f - Longitude: %.2f\nAltitude: %.2f\n",
			device->fix->latitude,
			device->fix->longitude,
			device->fix->altitude);
/*
	if(self->map_widget_state != MAP_VIEW_MAP_WIDGET_STATE_CONFIGURED)
	{
		DEBUG("MapWidget not yet ready");
		map_widget_ready = FALSE;
	}
*/

	if(device->fix->fields & LOCATION_GPS_DEVICE_LATLONG_SET)
	{
		DEBUG("Latitude and longitude are valid");
		point.latitude = device->fix->latitude;
		point.longitude = device->fix->longitude;
	/*
		if(!self->has_gps_fix)
		{
			self->has_gps_fix = TRUE;
			if(map_widget_ready)
			{
				map_widget_set_auto_center(self->map_widget,
						TRUE);
				map_view_update_autocenter(self);
			}
		}
*/
		if(self->activity_state == MAP_VIEW_ACTIVITY_STATE_STARTED)
		{
			map_view_check_and_add_route_point(self, &point,
					device->fix);
			osm_gps_map_draw_gps(OSM_GPS_MAP(self->map),device->fix->latitude,device->fix->longitude,0);
		}
		if(self->activity_state == MAP_VIEW_ACTIVITY_STATE_STOPPED || self->activity_state == MAP_VIEW_ACTIVITY_STATE_NOT_STARTED )
		{
		  osm_gps_map_clear_gps(OSM_GPS_MAP(self->map));
		  osm_gps_map_draw_gps(OSM_GPS_MAP(self->map),device->fix->latitude,device->fix->longitude,0);
		}
		
/*
		if(map_widget_ready)
		{
			map_widget_set_current_location(
					self->map_widget,
					&point);
		}
		*/
	} else {
		DEBUG("Latitude and longitude are not valid");
	} 


	/*
	if(device->status == LOCATION_GPS_DEVICE_STATUS_NO_FIX) {
		//hildon_banner_show_information(GTK_WIDGET(self->parent_window), NULL, "No GPS fix!");
	}
*/	
	
	if(self->is_auto_center)
	{
	osm_gps_map_set_center(OSM_GPS_MAP(self->map),device->fix->latitude,device->fix->longitude);
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
/*
	if(self->map_widget_state != MAP_VIEW_MAP_WIDGET_STATE_CONFIGURED)
	{
		DEBUG("MapWidget not yet ready");
		map_widget_ready = FALSE;
	}
*/
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

	//	map_view_set_travelled_distance(
	//			self,
	//			self->travelled_distance + distance);
	/*
		if(map_widget_ready)
		{
			map_widget_add_point_to_track(self->map_widget,
					point_copy);
			map_widget_show_track(self->map_widget,
					TRUE);
		}
		
		*/
	}

	DEBUG_END();
}

static void map_view_set_travelled_distance(MapView *self, gdouble distance)
{
#if 0
	gchar *lbl_text = NULL;

	g_return_if_fail(self != NULL);
	g_return_if_fail(distance >= 0.0);
	DEBUG_BEGIN();

	self->travelled_distance = distance;
	/** @todo Usage of different units? (Feet/yards, miles) */
	if(distance < 1000)
	{
		lbl_text = g_strdup_printf(_("%.0f m"), distance);
	} else {
		lbl_text = g_strdup_printf(_("%.1f km"), distance / 1000.0);
	}

	gtk_label_set_text(GTK_LABEL(self->info_distance), lbl_text);

	DEBUG_END();
#endif
}

static void map_view_scroll_up(GtkWidget *btn, gpointer user_data)
{
	MapView *self = (MapView *)user_data;

	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	map_view_scroll(self, MAP_VIEW_DIRECTION_NORTH);

	DEBUG_END();
}

static void map_view_scroll_down(GtkWidget *btn, gpointer user_data)
{
	MapView *self = (MapView *)user_data;

	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	map_view_scroll(self, MAP_VIEW_DIRECTION_SOUTH);

	DEBUG_END();
}

static void map_view_scroll_left(GtkWidget *btn, gpointer user_data)
{
	MapView *self = (MapView *)user_data;

	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	map_view_scroll(self, MAP_VIEW_DIRECTION_WEST);

	DEBUG_END();
}

static void map_view_scroll_right(GtkWidget *btn, gpointer user_data)
{
	MapView *self = (MapView *)user_data;

	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	map_view_scroll(self, MAP_VIEW_DIRECTION_EAST);

	DEBUG_END();
}

static void map_view_scroll(MapView *self, MapViewDirection direction)
{
	/* This code is somewhat hackish, because it uses the internals
	 * of the MapWidget, but seems like that is the only reasonable
	 * way to do so, because there is no public coordinate translation
	 * API for it
	 */
	gint x;
	gint y;

	g_return_if_fail(self != NULL);

	switch(direction)
	{
		case MAP_VIEW_DIRECTION_NORTH:
			x = self->map_widget->allocation.width / 2;
			y = 0;
			break;
		case MAP_VIEW_DIRECTION_SOUTH:
			x = self->map_widget->allocation.width / 2;
			y = self->map_widget->allocation.height;
			break;
		case MAP_VIEW_DIRECTION_WEST:
			x = 0;
			y = self->map_widget->allocation.height / 2;
			break;
		case MAP_VIEW_DIRECTION_EAST:
			x = self->map_widget->allocation.width;
			y = self->map_widget->allocation.height / 2;
			break;
		default:
			g_warning("Unknown map direction: %d", direction);
			DEBUG_END();
			return;
	}

	//map_widget_center_onscreen_coords(self->map_widget, x, y);
}

static void map_view_btn_back_clicked(GtkWidget *button, gpointer user_data)
{
	MapView *self = (MapView *)user_data;

	g_return_if_fail(user_data != NULL);
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

	map_view_hide(self);
	DEBUG_END();
}


static void map_view_units_clicked(GtkWidget *button, gpointer user_data)
{
	MapView *self = (MapView *)user_data;
	if(self->metric)
	{
	ec_button_set_label_text(EC_BUTTON(self->info_units), _("English"));
	self->metric = FALSE;
	}
	else
	{
	ec_button_set_label_text(EC_BUTTON(self->info_units), _("Metric"));
	self->metric = TRUE;
	}
		
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
			osm_gps_map_remove_button((OsmGpsMap*)self->map,394, 346);
			osm_gps_map_add_button((OsmGpsMap*)self->map,394, 346, self->rec_btn_selected);
			break;
		case MAP_VIEW_ACTIVITY_STATE_STARTED:
			map_view_pause_activity(self);
			osm_gps_map_remove_button((OsmGpsMap*)self->map,557, 346);
			osm_gps_map_add_button((OsmGpsMap*)self->map,557, 345, self->pause_btn_selected);
			break;
		case MAP_VIEW_ACTIVITY_STATE_PAUSED:
			map_view_continue_activity(self);
			osm_gps_map_remove_button((OsmGpsMap*)self->map,557, 345);
			osm_gps_map_add_button((OsmGpsMap*)self->map,557, 346, self->pause_btn_unselected);
			break;
		case MAP_VIEW_ACTIVITY_STATE_STOPPED:
			map_view_start_activity(self);
			osm_gps_map_remove_button((OsmGpsMap*)self->map,394, 346);
			osm_gps_map_add_button((OsmGpsMap*)self->map,394, 346, self->rec_btn_selected);
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
	osm_gps_map_remove_button((OsmGpsMap*)self->map,394, 346);
	osm_gps_map_add_button((OsmGpsMap*)self->map,394, 346, self->rec_btn_unselected);
	osm_gps_map_remove_button((OsmGpsMap*)self->map,557, 345);
	osm_gps_map_add_button((OsmGpsMap*)self->map,557, 346, self->pause_btn_unselected);
	map_view_stop(self);
	DEBUG_END();
}


static void map_view_btn_autocenter_clicked(GtkWidget *button,
		gpointer user_data)
{
	gboolean is_auto_center;
	MapView *self = (MapView *)user_data;

	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	if(self->map_widget_state != MAP_VIEW_MAP_WIDGET_STATE_CONFIGURED)
	{
		DEBUG_END();
		return;
	}

//	is_auto_center = map_widget_get_auto_center_status((OsmGpsMap*)self->map_widget);

	if(is_auto_center)
	{
//		map_widget_set_auto_center(self->map_widget, FALSE);
		ec_button_set_icon_pixbuf(EC_BUTTON(self->btn_autocenter),
				self->pxb_autocenter_off);
		hildon_banner_show_information(GTK_WIDGET(self->parent_window), NULL, "Map Centering off");
	} else {
//		map_widget_set_auto_center(self->map_widget, TRUE);
		ec_button_set_icon_pixbuf(EC_BUTTON(self->btn_autocenter),
				self->pxb_autocenter_on);
		hildon_banner_show_information(GTK_WIDGET(self->parent_window), NULL, "Map Centering on");
	}

	DEBUG_END();
}

static void map_view_update_autocenter(MapView *self)
{
	gboolean is_auto_center;
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

//	is_auto_center = map_widget_get_auto_center_status(self->map_widget);

	if(is_auto_center)
	{
		ec_button_set_icon_pixbuf(EC_BUTTON(self->btn_autocenter),
				self->pxb_autocenter_on);
		
	} else {
		ec_button_set_icon_pixbuf(EC_BUTTON(self->btn_autocenter),
				self->pxb_autocenter_off);
		
	}

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
	
	osm_gps_map_remove_button((OsmGpsMap*)self->map,394, 346);
	osm_gps_map_add_button((OsmGpsMap*)self->map,394, 346, self->rec_btn_selected);
	}

	gettimeofday(&self->start_time, NULL);

	self->activity_timer_id = g_timeout_add(
			1000,
			map_view_update_stats,
			self);

	self->activity_state = MAP_VIEW_ACTIVITY_STATE_STARTED;

	
//	ec_button_set_bg_image(EC_BUTTON(self->btn_start_pause),
//			EC_BUTTON_STATE_RELEASED,
//			GFXDIR "ec_button_pause.png");

	DEBUG_END();
}

static void map_view_continue_activity(MapView *self)
{
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	gettimeofday(&self->start_time, NULL);

	self->activity_timer_id = g_timeout_add(
			1000,
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
	gdouble curr_speed = 0.0;
	gchar *lbl_text = NULL;
	struct timeval time_now;
	struct timeval result;

	g_return_val_if_fail(self != NULL, FALSE);
	DEBUG_BEGIN();

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

		//ec_button_set_label_text(EC_BUTTON(self->info_distance), lbl_text);
		gtk_label_set_text(GTK_LABEL(self->info_distance),lbl_text);
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
		gtk_label_set_text(GTK_LABEL(self->info_distance),lbl_text);

	//	ec_button_set_label_text(EC_BUTTON(self->info_distance), lbl_text);
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
	//	ec_button_set_label_text(EC_BUTTON(self->info_avg_speed),
	//			lbl_text);
	
		gtk_label_set_text(GTK_LABEL(self->info_avg_speed),lbl_text);
		g_free(lbl_text);
		}
		else
		{
		avg_speed = avg_speed *	0.621;
		lbl_text = g_strdup_printf(_("%.1f mph"), avg_speed);
		gtk_label_set_text(GTK_LABEL(self->info_avg_speed),lbl_text);
		g_free(lbl_text);
		}
	} else {
		if(self->metric)
		{
	//	ec_button_set_label_text(EC_BUTTON(self->info_avg_speed),
	//			_("0 km/h"));
		gtk_label_set_text(GTK_LABEL(self->info_avg_speed),"O km/h");
		}
		else
		{
		gtk_label_set_text(GTK_LABEL(self->info_avg_speed),"O mph");
	//	ec_button_set_label_text(EC_BUTTON(self->info_avg_speed),
	//				_("0 mph"));		
		}
	}

	/* Current speed */
	
	curr_speed = track_helper_get_current_speed(self->track_helper);
	if(curr_speed > 0.0)
	{
		if(self->metric)
		{
		lbl_text = g_strdup_printf(_("%.1f km/h"), curr_speed);
		if(curr_speed > self->max_speed){
		 self->max_speed = curr_speed; 
		}
//		ec_button_set_label_text(EC_BUTTON(self->info_speed),
//				lbl_text);
		gtk_label_set_text(GTK_LABEL(self->info_speed),lbl_text);
		g_free(lbl_text);
		}
		else
		{
			curr_speed = curr_speed*0.621;
			lbl_text = g_strdup_printf(_("%.1f mph"), curr_speed);
			//ec_button_set_label_text(EC_BUTTON(self->info_speed),
			//		lbl_text);
					
			gtk_label_set_text(GTK_LABEL(self->info_speed),lbl_text);
			g_free(lbl_text);
		}
	} else {
		if(self->metric)
		{
			//ec_button_set_label_text(EC_BUTTON(self->info_speed),
			//			 _("0 km/h"));
			gtk_label_set_text(GTK_LABEL(self->info_speed),"0 km/h");
		}
		else
		{
			//ec_button_set_label_text(EC_BUTTON(self->info_speed),
			//		_("0 mph"));
			gtk_label_set_text(GTK_LABEL(self->info_speed),"0 mph");
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
	//ec_button_set_label_text(EC_BUTTON(self->info_distance), lbl_text);
	gtk_label_set_text(GTK_LABEL(self->info_time),lbl_text);
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

	if(self->show_map_widget_handler_id)
	{
		map_view_show_map_widget(NULL, self);
	}

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
			GFXDIR "ec_widget_generic_small_background.png");

	gtk_widget_set_size_request(button, 174, 82);
	gtk_fixed_put(GTK_FIXED(self->main_widget),
			button, 425 + 180 * x, 18 + 80 * y);

	ec_button_set_btn_down_offset(EC_BUTTON(button), 2);

	desc = pango_font_description_new();
	pango_font_description_set_family(desc, "Nokia Sans");
	pango_font_description_set_absolute_size(desc, 22 * PANGO_SCALE);
	ec_button_set_font_description_label(EC_BUTTON(button), desc);

	pango_font_description_set_absolute_size(desc, 20 * PANGO_SCALE);
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
	
	gboolean is_auto_center;
	switch (event->keyval) {
		
	//	is_auto_center = map_widget_get_auto_center_status(self->map_widget);
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
	//	case GDK_F6:
			/*if(is_auto_center)
			{
				map_widget_set_auto_center(self->map_widget, FALSE);
				//hildon_banner_show_information(GTK_WIDGET(self->parent_window), NULL, "Map Centering off");
				map_view_update_autocenter(self);
			
			} else {
				map_widget_set_auto_center(self->map_widget,
						TRUE);
				//hildon_banner_show_information(GTK_WIDGET(self->parent_window), NULL, "Map Centering on");
				map_view_update_autocenter(self);
			}
			*/
	//		map_view_btn_autocenter_clicked(NULL,self);
			
			
	//	break;
	/*	case GDK_F7:
			map_view_zoom_in(NULL,self);
			
		break;
		
		case GDK_F8:
			map_view_zoom_out(NULL,self);
	
		break;
		case GDK_Up:
			
			map_view_scroll_up(NULL,self);
			break;
		
		case GDK_Down:
			
			map_view_scroll_down(NULL,self);
			break;
		case GDK_Left:
			
			map_view_scroll_left(NULL,self);
			break;	
		case GDK_Right:
			
			map_view_scroll_right(NULL,self);
			break;
	*/	DEBUG_END();
		

		
	}

		
    }

gboolean map_button_press_cb(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
      MapView *self = (MapView *)user_data;
      if(user_data ==NULL){
	return false;}
      DEBUG_BEGIN();
     coord_t coord;
     
    gtk_window_unfullscreen ((GtkWindow*)self->win);
    gtk_widget_set_size_request(self->map, 800, 420);
    OsmGpsMap *map = OSM_GPS_MAP(widget);

    if ( (event->button == 1) && (event->type == GDK_BUTTON_PRESS) )
    {
	   int zoom = 0;
	coord = osm_gps_map_get_co_ordinates(map, (int)event->x, (int)event->y);
	
	
	
	
	if((event->x > 213 && event->x <380) && (event->y > 346 && event->y < 480)){
	  
	 
	  DEBUG("DATA BUTTON");
	//  gtk_widget_hide(self->map);
	  map_view_show_data(self);
	  
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
	
	 if((event->x < 557 && event->x > 394) && (event->y > 346 && event->y < 480)){
	   
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
	 if((event->x < 800 && event->x > 558) && (event->y > 346 && event->y < 480)){
	
	  
	  DEBUG("PAUSE BUTTON"); 
	   if(self->activity_state == MAP_VIEW_ACTIVITY_STATE_STARTED || self->activity_state == MAP_VIEW_ACTIVITY_STATE_PAUSED){
	    map_view_btn_start_pause_clicked(widget,user_data);
	   }
	 }
    
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

    /* Create menu entries */
   // button = hildon_gtk_button_new (HILDON_SIZE_AUTO);
   // command_id = g_strdup_printf ("Select Map Source");
   // gtk_button_set_label (GTK_BUTTON (button), command_id);

  button = hildon_button_new_with_text((HildonSizeType)(HILDON_SIZE_AUTO_WIDTH | HILDON_SIZE_FINGER_HEIGHT),
                                HILDON_BUTTON_ARRANGEMENT_VERTICAL,
                                            "Map Source",
                                             maps[self->map_provider-1]);
  
   g_signal_connect_after (button, "clicked", G_CALLBACK (select_map_source_cb), self);
  
     /* Add entry to the view menu */
   hildon_app_menu_append (menu, (GtkButton*)(button));
  
 //  center_button = hildon_check_button_new(HILDON_SIZE_AUTO_WIDTH | HILDON_SIZE_FINGER_HEIGHT);
  center_button = hildon_check_button_new((HildonSizeType)(HILDON_SIZE_AUTO_WIDTH | HILDON_SIZE_FINGER_HEIGHT));
 
gtk_button_set_label (GTK_BUTTON (center_button), "Auto Centering");
   hildon_check_button_set_active(HILDON_CHECK_BUTTON(center_button),TRUE);
   hildon_app_menu_append (menu, GTK_BUTTON(center_button));
   g_signal_connect_after(center_button, "toggled", G_CALLBACK (toggle_map_centering), self);
   
   personal_button = gtk_button_new_with_label ("Personal Data");
    //g_signal_connect_after (button, "clicked", G_CALLBACK (button_one_clicked), userdata);
   hildon_app_menu_append (menu, GTK_BUTTON (personal_button));

    note_button = gtk_button_new_with_label ("Add Note");
    //g_signal_connect_after (button, "clicked", G_CALLBACK (button_one_clicked), userdata);
   hildon_app_menu_append (menu, GTK_BUTTON (note_button));

   
   about_button = gtk_button_new_with_label ("About");

   hildon_app_menu_append (menu, GTK_BUTTON (about_button));
  g_signal_connect_after (about_button, "clicked", G_CALLBACK (about_dlg), self);
    help_button = gtk_button_new_with_label ("Help");
    //g_signal_connect_after (button, "clicked", G_CALLBACK (button_one_clicked), userdata);
    hildon_app_menu_append (menu, GTK_BUTTON (help_button));
   
   
  // Create a filter and add it to the menu
//filter = gtk_radio_button_new_with_label (NULL, "Filter one");
//gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (filter), FALSE);
//g_signal_connect_after (filter, "clicked", G_CALLBACK (filter_one_clicked), userdata);
//hildon_app_menu_add_filter (menu, GTK_BUTTON (filter));

// Add a new filter
//filter = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (filter), "Filter two");
//gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (filter), FALSE);
//g_signal_connect_after (filter, "clicked", G_CALLBACK (filter_two_clicked), userdata);
//hildon_app_menu_add_filter (menu, GTK_BUTTON (filter));

gtk_widget_show_all (GTK_WIDGET (menu));

  return menu;
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
static void about_dlg(HildonButton *button, gpointer user_data){
  
  MapView *self = (MapView *)user_data;
  DEBUG_BEGIN();   
    gtk_show_about_dialog((GtkWindow*)self->win
                , "name",      "eCoach"
		, "logo-icon-name",	"ecoach"
                , "version",   "1.5alpha"
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