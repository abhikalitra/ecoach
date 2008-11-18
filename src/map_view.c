/*
 *  eCoach
 *
 *  Copyright (C) 2008  Jukka Alasalmi
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
#include "map_widget/map_widget.h"

#include "config.h"
#include "debug.h"
#include <gdk/gdkkeysyms.h>
/*****************************************************************************
 * Definitions                                                               *
 *****************************************************************************/

#define MAPTILE_LOADER_EXEC_NAME "maptile-loader"
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

	self->map_widget_state = MAP_VIEW_MAP_WIDGET_STATE_NOT_CONFIGURED;
	self->has_gps_fix = FALSE;
	
	if(gconf_helper_get_value_bool_with_default(self->gconf_helper,
	   USE_METRIC,TRUE))
	{
		self->metric = TRUE;
		g_print("true \n");
	}
	else
	{
		self->metric = FALSE;
		g_print("False \n");
	}


	/* Main layout item		*/
	self->main_widget = gtk_fixed_new();
	gdk_color_parse("#000", &color);
	gtk_widget_modify_bg(self->main_widget, GTK_STATE_NORMAL, &color);
	g_signal_connect(G_OBJECT(self->main_widget), "expose-event",
			G_CALLBACK(fixed_expose_event),
			self);

	/* Map buttons			*/
	/*	Zoom in			*/

	self->btn_zoom_in = ec_button_new();

	ec_button_set_bg_image(
			EC_BUTTON(self->btn_zoom_in),
			EC_BUTTON_STATE_RELEASED,
			GFXDIR "ec_widget_map_background_topleft.png");

	gtk_widget_set_size_request(self->btn_zoom_in, 30, 30);

	gtk_fixed_put(GTK_FIXED(self->main_widget), self->btn_zoom_in,
			20, 20);

	g_signal_connect(G_OBJECT(self->btn_zoom_in), "clicked",
			G_CALLBACK(map_view_zoom_in), self);

	/*	Zoom out		*/
	self->btn_zoom_out = ec_button_new();

	ec_button_set_bg_image(
			EC_BUTTON(self->btn_zoom_out),
			EC_BUTTON_STATE_RELEASED,
			GFXDIR "ec_widget_map_background_bottomleft.png");

	gtk_widget_set_size_request(self->btn_zoom_out, 30, 30);

	gtk_fixed_put(GTK_FIXED(self->main_widget), self->btn_zoom_out,
			20, 376);

	g_signal_connect(G_OBJECT(self->btn_zoom_out), "clicked",
			G_CALLBACK(map_view_zoom_out), self);

	/*	Auto-center		*/
	self->btn_autocenter = ec_button_new();

	ec_button_set_bg_image(
			EC_BUTTON(self->btn_autocenter),
			EC_BUTTON_STATE_RELEASED,
			GFXDIR "ec_widget_map_background_topright.png");

	self->pxb_autocenter_on = map_view_load_image(
			GFXDIR "ec_icon_autocenter_on.png");
	self->pxb_autocenter_off = map_view_load_image(
			GFXDIR "ec_icon_autocenter_off.png");

	ec_button_set_center_text_vertically(EC_BUTTON(self->btn_autocenter),
			TRUE);

	gtk_widget_set_size_request(self->btn_autocenter, 30, 30);

	gtk_fixed_put(GTK_FIXED(self->main_widget),
			self->btn_autocenter, 376, 20);

	g_signal_connect(G_OBJECT(self->btn_autocenter), "clicked",
			G_CALLBACK(map_view_btn_autocenter_clicked), self);

	/*	Fill (no function at the moment)	*/
	self->btn_fill = ec_button_new();

	ec_button_set_bg_image(
			EC_BUTTON(self->btn_fill),
			EC_BUTTON_STATE_RELEASED,
			GFXDIR "ec_widget_map_background_bottomright.png");

	gtk_widget_set_size_request(self->btn_fill, 30, 30);

	gtk_fixed_put(GTK_FIXED(self->main_widget),
			self->btn_fill, 376, 376);

	/* Move map up			*/
	self->btn_map_scrl_up = ec_button_new();

	g_signal_connect(G_OBJECT(self->btn_map_scrl_up), "clicked",
			G_CALLBACK(map_view_scroll_up), self);

	ec_button_set_bg_image(
			EC_BUTTON(self->btn_map_scrl_up),
			EC_BUTTON_STATE_RELEASED,
			GFXDIR "ec_widget_map_background_top.png");

	ec_button_set_icon(
			EC_BUTTON(self->btn_map_scrl_up),
			GFXDIR "ec_widget_map_up_btn.png");

	gtk_widget_set_size_request(
			self->btn_map_scrl_up,
			326, 30);

	gtk_fixed_put(GTK_FIXED(self->main_widget),
			self->btn_map_scrl_up,
			50, 20);

	/* Move map left		*/
	self->btn_map_scrl_left = ec_button_new();

	g_signal_connect(G_OBJECT(self->btn_map_scrl_left), "clicked",
			G_CALLBACK(map_view_scroll_left), self);

	ec_button_set_bg_image(
			EC_BUTTON(self->btn_map_scrl_left),
			EC_BUTTON_STATE_RELEASED,
			GFXDIR "ec_widget_map_background_left.png");

	ec_button_set_icon(
			EC_BUTTON(self->btn_map_scrl_left),
			GFXDIR "ec_widget_map_left_btn.png");

	ec_button_set_center_vertically(
			EC_BUTTON(self->btn_map_scrl_left),
			TRUE);

	gtk_widget_set_size_request(
			self->btn_map_scrl_left,
			30, 326);

	gtk_fixed_put(GTK_FIXED(self->main_widget),
			self->btn_map_scrl_left,
			20, 50);

	/* Move map right		*/
	self->btn_map_scrl_right = ec_button_new();

	g_signal_connect(G_OBJECT(self->btn_map_scrl_right), "clicked",
			G_CALLBACK(map_view_scroll_right), self);

	ec_button_set_bg_image(
			EC_BUTTON(self->btn_map_scrl_right),
			EC_BUTTON_STATE_RELEASED,
			GFXDIR "ec_widget_map_background_right.png");

	ec_button_set_icon(
			EC_BUTTON(self->btn_map_scrl_right),
			GFXDIR "ec_widget_map_right_btn.png");

	ec_button_set_center_vertically(
			EC_BUTTON(self->btn_map_scrl_right),
			TRUE);

	gtk_widget_set_size_request(
			self->btn_map_scrl_right,
			30, 326);

	gtk_fixed_put(GTK_FIXED(self->main_widget),
			self->btn_map_scrl_right,
			376, 50);

	/* Move map down		*/
	self->btn_map_scrl_down = ec_button_new();

	g_signal_connect(G_OBJECT(self->btn_map_scrl_down), "clicked",
			G_CALLBACK(map_view_scroll_down), self);

	ec_button_set_bg_image(
			EC_BUTTON(self->btn_map_scrl_down),
			EC_BUTTON_STATE_RELEASED,
			GFXDIR "ec_widget_map_background_bottom.png");

	ec_button_set_icon(
			EC_BUTTON(self->btn_map_scrl_down),
			GFXDIR "ec_widget_map_down_btn.png");

	gtk_widget_set_size_request(
			self->btn_map_scrl_down,
			326, 30);

	gtk_fixed_put(GTK_FIXED(self->main_widget),
			self->btn_map_scrl_down,
			50, 376);

	/* Create the container for the map widget and GPS status */
	self->notebook_map = gtk_notebook_new();
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(self->notebook_map), FALSE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(self->notebook_map), FALSE);

	self->gps_status = ec_button_new();
	ec_button_set_label_text(EC_BUTTON(self->gps_status),
			_("Searching\nsatellites..."));
	ec_button_set_center_vertically(EC_BUTTON(self->gps_status), TRUE);
	ec_button_set_center_text_vertically(EC_BUTTON(self->gps_status),
			TRUE);
	ec_button_set_bg_image(EC_BUTTON(self->gps_status),
			EC_BUTTON_STATE_RELEASED,
			GFXDIR "ec_widget_generic_small_background.png");

	PangoFontDescription *desc;
	desc = pango_font_description_new();
	pango_font_description_set_family(desc, "Nokia Sans");
	pango_font_description_set_absolute_size(desc, 22 * PANGO_SCALE);
	ec_button_set_font_description_label(EC_BUTTON(self->gps_status), desc);

	gtk_widget_set_size_request(self->gps_status, 326, 326);

	self->page_id_gps_status = gtk_notebook_append_page(
			GTK_NOTEBOOK(self->notebook_map),
			self->gps_status, NULL);
	gtk_fixed_put(GTK_FIXED(self->main_widget),
			self->notebook_map,
			50, 50);

	/* Indicator bars		*/
	self->indicator_1 = ec_progress_new_with_label(_("<span font_desc=\"Nokia sans 16\">Speed</span>"));
	ec_progress_set_use_markup(EC_PROGRESS(self->indicator_1), TRUE);
	gtk_widget_set_size_request(self->indicator_1, 350, 78);
	gtk_fixed_put(GTK_FIXED(self->main_widget),
			self->indicator_1, 427 + 180 * 0, 20 + 80 * 3);

	map_view_setup_progress_indicator(EC_PROGRESS(self->indicator_1));
	gtk_widget_set_name(self->indicator_1, "indicator_1");

	/*
	self->indicator_2 = ec_progress_new_with_label("(Heart rate)");
	gtk_widget_set_size_request(self->indicator_2, 350, 78);
	gtk_fixed_put(GTK_FIXED(self->main_widget), self->indicator_2,
			427, 285);
	map_view_setup_progress_indicator(EC_PROGRESS(self->indicator_2));
	gdk_color_parse("#999", &color);
	ec_progress_set_fg_color(EC_PROGRESS(self->indicator_2), &color);
	gtk_widget_set_name(self->indicator_2, "indicator_2");
	*/

	/* Start/pause button		*/
	self->btn_start_pause = ec_button_new();
	ec_button_set_bg_image(EC_BUTTON(self->btn_start_pause),
			EC_BUTTON_STATE_RELEASED,
			GFXDIR "ec_button_rec.png");
	ec_button_set_btn_down_offset(EC_BUTTON(self->btn_start_pause), 2);
	ec_button_set_center_vertically(
			EC_BUTTON(self->btn_start_pause),
			TRUE);
	gtk_widget_set_size_request(self->btn_start_pause, 116, 74);
	gtk_fixed_put(GTK_FIXED(self->main_widget),
			self->btn_start_pause, 425, 334);
	g_signal_connect(G_OBJECT(self->btn_start_pause), "clicked",
			G_CALLBACK(map_view_btn_start_pause_clicked), self);

	/* Stop button			*/
	self->btn_stop = ec_button_new();
	ec_button_set_bg_image(EC_BUTTON(self->btn_stop),
			EC_BUTTON_STATE_RELEASED,
			GFXDIR "ec_button_stop.png");
	ec_button_set_btn_down_offset(EC_BUTTON(self->btn_stop), 2);
	ec_button_set_center_vertically(
			EC_BUTTON(self->btn_stop),
			TRUE);
	gtk_widget_set_size_request(self->btn_stop, 116, 74);
	gtk_fixed_put(GTK_FIXED(self->main_widget),
			self->btn_stop, 545, 334);
	g_signal_connect(G_OBJECT(self->btn_stop), "clicked",
			G_CALLBACK(map_view_btn_stop_clicked), self);

	/* Back button			*/
	self->btn_back = ec_button_new();
	ec_button_set_bg_image(EC_BUTTON(self->btn_back),
			EC_BUTTON_STATE_RELEASED,
			GFXDIR "ec_button_generic.png");
	ec_button_set_btn_down_offset(EC_BUTTON(self->btn_back), 2);
	ec_button_set_label_text(EC_BUTTON(self->btn_back), _("Back"));
	ec_button_set_center_vertically(
			EC_BUTTON(self->btn_back),
			TRUE);
	ec_button_set_center_text_vertically(EC_BUTTON(self->btn_back), TRUE);
	gtk_widget_set_size_request(self->btn_back, 116, 74);
	gtk_fixed_put(GTK_FIXED(self->main_widget),
			self->btn_back, 665, 334);
	g_signal_connect(G_OBJECT(self->btn_back), "clicked",
			G_CALLBACK(map_view_btn_back_clicked), self);

	/* Elapsed time label		*/
	self->info_time = map_view_create_info_button(
			self,
			_("Time"),
			_("00:00:00"),
			0, 0);

	/* Traveled distance		*/
	if(self->metric)
	{
	self->info_distance = map_view_create_info_button(
			self,
			_("Distance"),
			_("0 m"),
			1, 0);
	}
	else
	{
		self->info_distance = map_view_create_info_button(
				self,
				_("Distance"),
				_("0 ft"),
				1, 0);	
	}

	/* Speed			*/
	if(self->metric)
	{
	self->info_speed = map_view_create_info_button(
			self,
			_("Speed"),
			_("0.0 km/h"),
			0, 1);
	}
	else
	{
	self->info_speed = map_view_create_info_button(
				self,
				_("Speed"),
				_("0.0 mph"),0, 1);
	}
	

	/* Average speed		*/
	if(self->metric)
	{
	self->info_avg_speed = map_view_create_info_button(
			self,
			_("Avg. speed"),
			_("0.0 km/h"),
			1, 1);
	}
	else
	{
	self->info_avg_speed = map_view_create_info_button(
			self,
    			_("Avg. speed"),
			_("0.0 mph"),
			1, 1);	
	}

	/* Heart rate			*/
	self->info_heart_rate = map_view_create_info_button(
			self,
			_("Heart rate"),
			_("Wait..."),
			0, 2);

	self->pxb_hrm_status[MAP_VIEW_HRM_STATUS_LOW] =
		map_view_load_image(GFXDIR "ec_icon_heart_red.png");

	self->pxb_hrm_status[MAP_VIEW_HRM_STATUS_GOOD] =
		map_view_load_image(GFXDIR "ec_icon_heart_green.png");

	self->pxb_hrm_status[MAP_VIEW_HRM_STATUS_HIGH] =
		map_view_load_image(GFXDIR "ec_icon_heart_yellow.png");

	self->pxb_hrm_status[MAP_VIEW_HRM_STATUS_NOT_CONNECTED] =
		map_view_load_image(GFXDIR "ec_icon_heart_grey.png");

	ec_button_set_icon_pixbuf(EC_BUTTON(self->info_heart_rate),
			self->pxb_hrm_status[MAP_VIEW_HRM_STATUS_NOT_CONNECTED]
			);
	/* Distance unit */
	/*
	self->info_units = map_view_create_info_button(self,
			 _("Units"), _("Metric"), 1, 2);
	g_signal_connect(G_OBJECT(self->info_units), "clicked",
			 G_CALLBACK(map_view_units_clicked), self);
	*/
	/* GPS device			*/
#if (MAP_VIEW_SIMULATE_GPS)
	self->show_map_widget_handler_id = 1;
#else
	self->gps_device = g_object_new(LOCATION_TYPE_GPS_DEVICE, NULL);
	self->gpsd_control = location_gpsd_control_get_default();
	g_signal_connect(G_OBJECT(self->gps_device), "changed",
			G_CALLBACK(map_view_location_changed), self);
	self->show_map_widget_handler_id = g_signal_connect(
			G_OBJECT(self->gps_device), "changed",
			G_CALLBACK(map_view_show_map_widget), self);
#endif

	DEBUG_END();
	return self;
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

void map_view_show(MapView *self)
{
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();
	
	

	if(!self->beat_detector_connected)
	{
		self->beat_detector_connected = TRUE;
		g_idle_add(map_view_connect_beat_detector_idle, self);
	}

	if(self->map_widget_state == MAP_VIEW_MAP_WIDGET_STATE_NOT_CONFIGURED)
	{
		self->has_gps_fix = FALSE;
		self->map_widget_state = MAP_VIEW_MAP_WIDGET_STATE_CONFIGURING;
		g_idle_add(map_view_load_map_idle, self);
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
	location_gpsd_control_start(self->gpsd_control);
#endif
	
	g_signal_connect(G_OBJECT(self->parent_window), 
			 "key_press_event", G_CALLBACK(key_press_cb), self);
	
	

	DEBUG_END();
}

void map_view_hide(MapView *self)
{
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

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

		

		self->has_gps_fix = FALSE;
		map_view_hide_map_widget(self);
		track_helper_clear(self->track_helper, FALSE);
		map_widget_clear_track(self->map_widget);
		map_view_update_stats(self);
		location_gpsd_control_stop(self->gpsd_control);
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

	ec_button_set_bg_image(EC_BUTTON(self->btn_start_pause),
			EC_BUTTON_STATE_RELEASED,
			GFXDIR "ec_button_rec.png");

	DEBUG_END();
}

void map_view_clear_all(MapView *self)
{
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	track_helper_clear(self->track_helper, TRUE);
	map_view_update_stats(self);
	map_widget_clear_track(self->map_widget);

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

static gboolean map_view_load_map_idle(gpointer user_data)
{
	MapView *self = (MapView *)user_data;	
	MapPoint center;

	g_return_val_if_fail(self != NULL, FALSE);
	DEBUG_BEGIN();

	center.latitude = 51.50;
	center.longitude = -.1;
	/** @todo Save the latitude & longitude to gconf */

	gboolean is_running = FALSE;

	/* Start maptile-loader if it is not already running */
	if(system("pidof " MAPTILE_LOADER_EXEC_NAME) != 0)
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
		DEBUG("Maptile-loader did not start");
		/** @todo Handle the error */
	}

	gdk_threads_enter();

	/* Create and setup the map widget */
	gtk_notebook_set_current_page(GTK_NOTEBOOK(self->notebook_map),
			self->page_id_gps_status);

	self->map_widget = map_widget_create();
	gtk_widget_set_size_request(GTK_WIDGET(self->map_widget), 326, 326);
	self->page_id_map_widget = gtk_notebook_append_page(
			GTK_NOTEBOOK(self->notebook_map),
			self->map_widget, NULL);

	gtk_widget_set_size_request(self->notebook_map, 326, 326);

	map_widget_new_from_center_zoom_type(self->map_widget, &center, 15.0,
			MAP_ORIENTATION_NORTH, "Open Street",
			self->osso);

	gtk_widget_show_all(self->notebook_map);

	self->map_widget_state = MAP_VIEW_MAP_WIDGET_STATE_CONFIGURED;
	map_view_update_autocenter(self);

	gdk_threads_leave();

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

	if(heart_rate > 0)
	{
		text = g_strdup_printf(_("%d bpm"), (gint)heart_rate);
		ec_button_set_label_text(
				EC_BUTTON(self->info_heart_rate),
				text);
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

	gtk_notebook_set_current_page(GTK_NOTEBOOK(self->notebook_map),
			self->page_id_gps_status);

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

	gtk_notebook_set_current_page(GTK_NOTEBOOK(self->notebook_map),
			self->page_id_map_widget);

	DEBUG_END();
}

static void map_view_location_changed(
		LocationGPSDevice *device,
		gpointer user_data)
{
	MapPoint point;
	MapView *self = (MapView *)user_data;
	gboolean map_widget_ready = TRUE;

	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	DEBUG("Latitude: %.2f - Longitude: %.2f\nAltitude: %.2f\n",
			device->fix->latitude,
			device->fix->longitude,
			device->fix->altitude);

	if(self->map_widget_state != MAP_VIEW_MAP_WIDGET_STATE_CONFIGURED)
	{
		DEBUG("MapWidget not yet ready");
		map_widget_ready = FALSE;
	}

	if(device->fix->fields & LOCATION_GPS_DEVICE_LATLONG_SET)
	{
		DEBUG("Latitude and longitude are valid");
		point.latitude = device->fix->latitude;
		point.longitude = device->fix->longitude;

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

		if(self->activity_state == MAP_VIEW_ACTIVITY_STATE_STARTED)
		{
			map_view_check_and_add_route_point(self, &point,
					device->fix);
		}

		if(map_widget_ready)
		{
			map_widget_set_current_location(
					self->map_widget,
					&point);
		}
	} else {
		DEBUG("Latitude and longitude are not valid");
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

	if(self->map_widget_state != MAP_VIEW_MAP_WIDGET_STATE_CONFIGURED)
	{
		DEBUG("MapWidget not yet ready");
		map_widget_ready = FALSE;
	}

	if(!self->point_added)
	{
		DEBUG("First point. Adding to track.");
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

		map_view_set_travelled_distance(
				self,
				self->travelled_distance + distance);

		if(map_widget_ready)
		{
			map_widget_add_point_to_track(self->map_widget,
					point_copy);
			map_widget_show_track(self->map_widget,
					TRUE);
		}
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
			break;
		case MAP_VIEW_ACTIVITY_STATE_STARTED:
			map_view_pause_activity(self);
			break;
		case MAP_VIEW_ACTIVITY_STATE_PAUSED:
			map_view_continue_activity(self);
			break;
		case MAP_VIEW_ACTIVITY_STATE_STOPPED:
			map_view_start_activity(self);
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
		"Statistics such as average speed and elapsed time\n"
		"will cleared be when you press Start button again."),
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
		map_widget_clear_track(self->map_widget);
	}

	gettimeofday(&self->start_time, NULL);

	self->activity_timer_id = g_timeout_add(
			1000,
			map_view_update_stats,
			self);

	self->activity_state = MAP_VIEW_ACTIVITY_STATE_STARTED;

	ec_button_set_bg_image(EC_BUTTON(self->btn_start_pause),
			EC_BUTTON_STATE_RELEASED,
			GFXDIR "ec_button_pause.png");

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
	map_view_update_stats(self);

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

		ec_button_set_label_text(EC_BUTTON(self->info_distance), lbl_text);
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

		ec_button_set_label_text(EC_BUTTON(self->info_distance), lbl_text);
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
		ec_button_set_label_text(EC_BUTTON(self->info_avg_speed),
				lbl_text);
		g_free(lbl_text);
		}
		else
		{
		avg_speed = avg_speed *	0.621;
		lbl_text = g_strdup_printf(_("%.1f mph"), avg_speed);
		ec_button_set_label_text(EC_BUTTON(self->info_avg_speed),
					 lbl_text);
		g_free(lbl_text);
		}
	} else {
		if(self->metric)
		{
		ec_button_set_label_text(EC_BUTTON(self->info_avg_speed),
				_("0 km/h"));
		}
		else
		{
		ec_button_set_label_text(EC_BUTTON(self->info_avg_speed),
					_("0 mph"));		
		}
	}

	/* Current speed */
	
	curr_speed = track_helper_get_current_speed(self->track_helper);
	if(curr_speed > 0.0)
	{
		if(self->metric)
		{
		lbl_text = g_strdup_printf(_("%.1f km/h"), curr_speed);
		ec_button_set_label_text(EC_BUTTON(self->info_speed),
				lbl_text);
		g_free(lbl_text);
		}
		else
		{
			curr_speed = curr_speed*0.621;
			lbl_text = g_strdup_printf(_("%.1f mph"), curr_speed);
			ec_button_set_label_text(EC_BUTTON(self->info_speed),
					lbl_text);
			g_free(lbl_text);
		}
	} else {
		if(self->metric)
		{
			ec_button_set_label_text(EC_BUTTON(self->info_speed),
						 _("0 km/h"));
		}
		else
		{
			ec_button_set_label_text(EC_BUTTON(self->info_speed),
					_("0 mph"));
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
		
		is_auto_center = map_widget_get_auto_center_status(self->map_widget);
		g_return_if_fail(self != NULL);
		DEBUG_BEGIN();
		
		case GDK_F6:
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
			map_view_btn_autocenter_clicked(NULL,self);
			
			
		break;
		case GDK_F7:
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
		DEBUG_END();
		

		
	}

		
		    }
