/*
 *  eCoach
 *
 *  Copyright (C) 2008  Jukka Alasalmi
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

#ifndef _MAP_VIEW_H
#define _MAP_VIEW_H

/* Configuration */
#include "config.h"

/* System */
#include <sys/time.h>
#include <stdlib.h>
#include <time.h>

/* Gtk */
#include <gtk/gtk.h>

/* Location */
#include <location/location-gps-device.h>
#include <location/location-gpsd-control.h>

/* Osso */
#include <libosso.h>

/* Other modules */
#include "beat_detect.h"
#include "gconf_helper.h"
#include "track.h"

typedef struct _MapView MapView;
typedef struct _MapViewGpsPoint MapViewGpsPoint;

typedef enum _MapViewDirection {
	MAP_VIEW_DIRECTION_NORTH,
	MAP_VIEW_DIRECTION_SOUTH,
	MAP_VIEW_DIRECTION_WEST,
	MAP_VIEW_DIRECTION_EAST
} MapViewDirection;

typedef enum _MapViewMapWidgetState {
	MAP_VIEW_MAP_WIDGET_STATE_NOT_CONFIGURED,
	MAP_VIEW_MAP_WIDGET_STATE_CONFIGURING,
	MAP_VIEW_MAP_WIDGET_STATE_CONFIGURED
} MapViewMapWidgetState;

typedef enum _MapViewActivityState {
	MAP_VIEW_ACTIVITY_STATE_NOT_STARTED,
	MAP_VIEW_ACTIVITY_STATE_STARTED,
	MAP_VIEW_ACTIVITY_STATE_PAUSED,
	MAP_VIEW_ACTIVITY_STATE_STOPPED
} MapViewActivityState;

typedef enum _MapViewHRMStatus {
	MAP_VIEW_HRM_STATUS_NOT_CONNECTED = 0,
	MAP_VIEW_HRM_STATUS_LOW,
	MAP_VIEW_HRM_STATUS_GOOD,
	MAP_VIEW_HRM_STATUS_HIGH,
	MAP_VIEW_HRM_STATUS_COUNT
} MapViewHRMStatus;

struct _MapViewGpsPoint {
	gdouble latitude;
	gdouble longitude;
	gboolean altitude_set;
	gdouble altitude;
};

struct _MapView {
	GtkWindow *parent_window;	/**< Parent window		*/
	GtkWidget *main_widget;		/**< Main layout item		*/

	/* Buttons		*/
	/*	General		*/
	GtkWidget *btn_back;		/**< Leave view to background 	*/
	GtkWidget *btn_start_pause;	/**< Start/pause the activity	*/
	GtkWidget *btn_stop;		/**< Stop the activity 		*/
	GtkWidget *btn_minimize;	/**< Minimize the program	*/

	/*	Map		*/
	GtkWidget *btn_zoom_in;		/**< Zoom in the map		*/
	GtkWidget *btn_zoom_out;	/**< Zoom out the map		*/
	GtkWidget *btn_autocenter;	/**< Auto-center the map	*/
	GtkWidget *btn_fill;		/**< Not used at the moment	*/
	GtkWidget *btn_map_scrl_up;	/**< Scroll the map up		*/
	GtkWidget *btn_map_scrl_down;	/**< Scroll the map down	*/
	GtkWidget *btn_map_scrl_left;	/**< Scroll the map left	*/
	GtkWidget *btn_map_scrl_right;	/**< Scroll the map right	*/

	/* Info buttons		*/
	GtkWidget *info_distance;	/**< Travelled distance		*/
	GtkWidget *info_time;		/**< Elapsed time		*/
	GtkWidget *info_speed;		/**< Current speed		*/
	GtkWidget *info_avg_speed;	/**< Average speed		*/
	GtkWidget *info_heart_rate;	/**< Heart rate			*/
	GtkWidget *info_units;		/**< Distance units		*/

	/* Progress indicators	*/
	GtkWidget *indicator_1;		/**< "Progress indicator"	*/

	/* Heart rate widgets	*/
	GtkWidget *h_separator_1;	/**< Horizontal separator	*/
	GtkWidget *h_separator_2;	/**< Horizontal separator	*/
	GtkWidget *icon_heart;		/**< Heart icon			*/

	/* Miscellaneous	*/
	GtkWidget *notebook_map;	/**< For map widget/information	*/
	GtkWidget *map_widget;		/**< The map widget		*/
	gint page_id_map_widget;	/**< Page id for map widget	*/
	GtkWidget *gps_status;		/**< GPS status			*/
	gint page_id_gps_status;	/**< Page id for the GPS status	*/
	gulong show_map_widget_handler_id;
					/**< Signal handler id		*/

	GConfHelperData *gconf_helper;	/**< GConf helper		*/
	BeatDetector *beat_detector;	/**< Beat detector		*/
	osso_context_t *osso;		/**< Osso context		*/
	LocationGPSDevice *gps_device;	/**< GPS device connection	*/
	LocationGPSDControl
		*gpsd_control;		/**< GPSD control		*/

	MapViewMapWidgetState
		map_widget_state;	/**< State of map widget	*/

	gboolean has_gps_fix;		/**< Is there already a GPS fix	*/

	gboolean beat_detector_connected;
					/**< Is beat detector connected	*/

	MapViewActivityState
		activity_state;		/**< State of the activity	*/

	struct timeval elapsed_time;	/**< Elapsed time		*/
	struct timeval start_time;	/**< Time since previous start
					      or continue		*/
	guint activity_timer_id;	/**< Source id for g_timeout	*/

	TrackHelper *track_helper;	/**< Track management		*/

	gchar *activity_name;
	gchar *activity_comment;
	gchar *file_name;

	gint heart_rate_count;		/**< Count of heart rate values	*/
	gint heart_rate_limit_low;	/**< Heart rate lower range	*/
	gint heart_rate_limit_high;	/**< Heart rate upper range	*/

	/* Icons */
	GdkPixbuf *pxb_autocenter_on;	/**< Autocenter on		*/
	GdkPixbuf *pxb_autocenter_off;	/**< Autocenter off		*/

	GdkPixbuf *pxb_hrm_status[MAP_VIEW_HRM_STATUS_COUNT];
					/**< Heart rate status		*/

	/*distance unit*/
	
	gboolean metric;
	
	
	/* Temporary stuff	*/
	gboolean point_added;
	MapViewGpsPoint previous_added_point;
	gdouble travelled_distance;
};

/**
 * @brief Create a new map view.
 *
 * Use the main_widget of the returned struct to create a notebook page,
 * and connect the "clicked" signal of following buttons:
 * 	btn_back
 * 	btn_close
 * 
 * @param gconf_helper Pointer to #GConfHelperData
 * @param beat_detector Pointer to #BeatDetector
 */
MapView *map_view_new(
		GtkWindow *parent_window,
		GConfHelperData *gconf_helper,
		BeatDetector *beat_detector,
		osso_context_t *osso);

/**
 * @brief Get the state of the current activity
 *
 * @param self Pointer to #MapView
 *
 * @return The state of the current activity
 */
MapViewActivityState map_view_get_activity_state(MapView *self);

/**
 * @brief Setup an activity
 *
 * @param self Pointer to #MapView
 * @param activity_name Name of the activity
 * @param activity_comment Comment for the activity
 * @param file_name Name of file to save in
 * @param heart_rate_limit_low Lower heart rate limit
 * @param heart_rate_limit_high Upper heart rate limit
 *
 * @return TRUE on success, FALSE on failure (for example, if there is an
 * on-going acitivity that user does not want to stop)
 */
gboolean map_view_setup_activity(
		MapView *self,
		const gchar *activity_name,
		const gchar *activity_comment,
		const gchar *file_name,
		gint heart_rate_limit_low,
		gint heart_rate_limit_high);

/**
 * @brief Call this when map view is shown.
 *
 * @param self Pointer to #MapView
 */
void map_view_show(MapView *self);

/**
 * @brief Saves and stops the current activity
 *
 * @param self Pointer to #MapView
 */
void map_view_stop(MapView *self);

/**
 * @brief Clears all data and resets statistics
 *
 * @param self Pointer to #MapView
 */
void map_view_clear_all(MapView *self);

#endif /* _MAP_VIEW_H */
