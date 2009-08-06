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
	self->map_provider = gconf_helper_get_value_int_with_default(self->gconf_helper,MAP_SOURCE,1);
	if(self->map_provider==0)
	{
	  self->map_provider = 1;
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
	self->map = g_object_new (OSM_TYPE_GPS_MAP,
                        "map-source",(OsmGpsMapSource_t)self->map_provider,
                        "tile-cache",self->cachedir,
                        "tile-cache-is-full-path",self->fullpath,
                        "proxy-uri",g_getenv("http_proxy"),
                        NULL);

	g_free(self->cachedir);
	
	
	self->is_auto_center = TRUE;
	/* Main layout 		*/
	self->main_widget = gtk_fixed_new();
	gdk_color_parse("#000", &color);
	
	gtk_widget_modify_bg(self->main_widget, GTK_STATE_NORMAL, &color);
	
	self->zoom_in = gdk_pixbuf_new_from_file(GFXDIR "ec_map_zoom_in.png",NULL);
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
	
	osm_gps_map_add_button(self->map,0, 150, self->zoom_out);
	osm_gps_map_add_button(self->map,720, 150, self->zoom_in);
	osm_gps_map_add_button(self->map,50, 346, self->map_btn);
	osm_gps_map_add_button(self->map,213, 346, self->data_btn);
	
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
	osm_gps_map_add_button(self->map,394, 346, self->rec_btn_unselected);
	osm_gps_map_add_button(self->map,557, 346, self->pause_btn_unselected);
	}
	else{
	osm_gps_map_add_button(self->map,394, 346, self->rec_btn_selected);
	
	}
	if(self->activity_state == MAP_VIEW_ACTIVITY_STATE_PAUSED)
	{
	osm_gps_map_add_button(self->map,557, 346, self->pause_btn_selected);
	}
		    

		    

       #if (MAP_VIEW_SIMULATE_GPS)
	self->show_map_widget_handler_id = 1;
#else
	self->gps_device = g_object_new(LOCATION_TYPE_GPS_DEVICE, NULL);
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
		gint heart_rate_limit_high)
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

	track_helper_stop(self->track_helper);
	track_helper_clear(self->track_helper, FALSE);
	self->activity_state = MAP_VIEW_ACTIVITY_STATE_STOPPED;
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

	zoom = map_widget_get_current_zoom_level(self->map_widget);
	if(zoom >= 17)
	{
		DEBUG_END();
		return;
	}
	hildon_banner_show_information(GTK_WIDGET(self->parent_window), NULL, "Zoom in");
	zoom = zoom + 1;
	map_widget_set_zoom(self->map_widget, zoom);
	
	DEBUG_END();
}

static void map_view_zoom_out(GtkWidget *widget, gpointer user_data)
{
	guint zoom;
	MapView *self = (MapView *)user_data;

	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	zoom = map_widget_get_current_zoom_level(self->map_widget);
	if(zoom <= 1)
	{
		DEBUG_END();
		return;
	}
	zoom = zoom - 1;
	hildon_banner_show_information(GTK_WIDGET(self->parent_window), NULL, "Zoom out");
	map_widget_set_zoom(self->map_widget, zoom);
	
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
		else{
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

	map_widget_center_onscreen_coords(self->map_widget, x, y);
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
			osm_gps_map_remove_button(self->map,394, 346);
			osm_gps_map_add_button(self->map,394, 346, self->rec_btn_selected);
			break;
		case MAP_VIEW_ACTIVITY_STATE_STARTED:
			map_view_pause_activity(self);
			osm_gps_map_remove_button(self->map,557, 346);
			osm_gps_map_add_button(self->map,557, 345, self->pause_btn_selected);
			break;
		case MAP_VIEW_ACTIVITY_STATE_PAUSED:
			map_view_continue_activity(self);
			osm_gps_map_remove_button(self->map,557, 345);
			osm_gps_map_add_button(self->map,557, 346, self->pause_btn_unselected);
			break;
		case MAP_VIEW_ACTIVITY_STATE_STOPPED:
			map_view_start_activity(self);
			osm_gps_map_remove_button(self->map,394, 346);
			osm_gps_map_add_button(self->map,394, 346, self->rec_btn_selected);
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
	osm_gps_map_remove_button(self->map,394, 346);
	osm_gps_map_add_button(self->map,394, 346, self->rec_btn_unselected);
	osm_gps_map_remove_button(self->map,557, 345);
	osm_gps_map_add_button(self->map,557, 346, self->pause_btn_unselected);
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

	is_auto_center = map_widget_get_auto_center_status(self->map_widget);

	if(is_auto_center)
	{
		map_widget_set_auto_center(self->map_widget, FALSE);
		ec_button_set_icon_pixbuf(EC_BUTTON(self->btn_autocenter),
				self->pxb_autocenter_off);
		hildon_banner_show_information(GTK_WIDGET(self->parent_window), NULL, "Map Centering off");
	} else {
		map_widget_set_auto_center(self->map_widget, TRUE);
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

	is_auto_center = map_widget_get_auto_center_status(self->map_widget);

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

	/* Clear the track helper */
	if(self->activity_state == MAP_VIEW_ACTIVITY_STATE_STOPPED)
	{
		track_helper_clear(self->track_helper, FALSE);
		map_view_update_stats(self);
	
	osm_gps_map_remove_button(self->map,394, 346);
	osm_gps_map_add_button(self->map,394, 346, self->rec_btn_selected);
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
		  osm_gps_map_set_zoom(self->map, zoom+1);
		  break;
		  
		case HILDON_HARDKEY_DECREASE: /* Zoom in */
		
		  DEBUG("ZOOM OUT");
		  
		  g_object_get(self->map, "zoom", &zoom, NULL);
		  osm_gps_map_set_zoom(self->map, zoom-1);
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
      g_return_if_fail(user_data != NULL);
      DEBUG_BEGIN();
     coord_t coord;
     
    gtk_window_unfullscreen (self->win);
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
	    osm_gps_map_set_zoom(self->map, zoom-1);
	}
	
	 if(event->x > 720 && (event->y > 150 && event->y < 240)){
	DEBUG("ZOOM IN");
	g_object_get(self->map, "zoom", &zoom, NULL);
	osm_gps_map_set_zoom(self->map, zoom+1);
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
      g_return_if_fail(user_data != NULL);
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

  button = hildon_button_new_with_text     (HILDON_SIZE_AUTO_WIDTH | HILDON_SIZE_FINGER_HEIGHT,
                                HILDON_BUTTON_ARRANGEMENT_VERTICAL,
                                            "Map Source",
                                             maps[self->map_provider-1]);
  
   g_signal_connect_after (button, "clicked", G_CALLBACK (select_map_source_cb), self);
  
     /* Add entry to the view menu */
   hildon_app_menu_append (menu, HILDON_BUTTON(button));
  
   center_button = hildon_check_button_new (HILDON_SIZE_AUTO_WIDTH | HILDON_SIZE_FINGER_HEIGHT);

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
    //g_signal_connect_after (button, "clicked", G_CALLBACK (button_one_clicked), userdata);
   hildon_app_menu_append (menu, GTK_BUTTON (about_button));

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
	"Virtual Earth Hybrid",
	NULL
	};
    DEBUG_BEGIN(); 
    
    
    GtkWidget *selector;
    GtkWidget *dialog = gtk_dialog_new();	
    
    
    gtk_window_set_title(GTK_WINDOW(dialog),"Select map source");
    gtk_widget_set_size_request(dialog, 800, 390);
    selector = hildon_touch_selector_new_text();
    hildon_touch_selector_set_column_selection_mode (HILDON_TOUCH_SELECTOR (selector),
    HILDON_TOUCH_SELECTOR_SELECTION_MODE_SINGLE);
  
    int j = 0;
    for(;j<  maps[j] != NULL ;j++){
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
    hildon_banner_show_information(GTK_WIDGET(self->parent_window), NULL, "Selected map source will be used next time you load map view");
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
      gtk_window_fullscreen (self->win);
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
      gtk_window_unfullscreen (self->win);
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
