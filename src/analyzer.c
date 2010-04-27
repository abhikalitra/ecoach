/*
 *  eCoach
 *
 *  Copyright (C) 2009  Jukka Alasalmi, Sampo Savola
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

/*****************************************************************************
 * Definitions                                                               *
 *****************************************************************************/
#define GFXDIR DATADIR		"/pixmaps/" PACKAGE_NAME "/"
#define ANALYZER_VIEW_HEIGHT	325
#define ANALYZER_VIEW_WIDTH	760

/*****************************************************************************
 * Includes                                                                  *
 *****************************************************************************/

/* This module */
#include "analyzer.h"

/* System */
#include "math.h"
#include "string.h"

/* Hildon */
#include <hildon/hildon-file-chooser-dialog.h>
#include <hildon/hildon-file-system-model.h>
#include <hildon/hildon.h>
#include <hildon/hildon-pannable-area.h>

/* Location */
#include "location-distance-utils-fix.h"

/* Other modules */
#include "gconf_keys.h"
#include "gpx_parser.h"
#include "ec-button.h"
#include "ec_error.h"
#include "util.h"
/*osm-gps-map*/
#include "osm_gps_map/osm-gps-map.h"
#include "debug.h"

/*****************************************************************************
 * Enumerations                                                              *
 *****************************************************************************/

typedef enum _AnalyzerViewUnitType {
	ANALYZER_VIEW_UNIT_TYPE_NONE = -1,
	ANALYZER_VIEW_UNIT_TYPE_MINS_SECS,
	ANALYZER_VIEW_UNIT_TYPE_HOURS_MINS
} AnalyzerViewUnitType;

typedef enum
{
	METRIC,
 	ENGLISH
} MeasureUnit;
/*****************************************************************************
 * Data structures                                                           *
 *****************************************************************************/

typedef struct _AnalyzerViewTrack {
	/* These come directly from the parser */
	gchar *name;
	gchar *comment;
	gint number;
	gchar *min_per_km;

	/** @brief A list to pointers of type AnalyzerViewTrackSegment */
	GSList *track_segments;

	/* These are analyzed from the data */

	/** @brief Has the data been calculated */
	gboolean data_is_analyzed;

	/** @brief Start time of the track */
	struct timeval start_time;

	/**
	 * @brief End time of the track
	 *
	 * This will be used for two purposes: displaying the track time,
	 * and determining on which heart beat goes to which track
	 */
	struct timeval end_time;

	/**
	 * @brief Duration of the track
	 *
	 * This is the duration of the track, excluding all pauses between
	 * track segments
	 */
	struct timeval duration;

	/** @brief Travelled distance in metres */
	gdouble distance;

	/** @brief Average speed in km/h */
	gdouble speed_avg;

	/** @brief Maximum sustained speed */
	gdouble speed_max;

	/**
	 * @brief Whether or not the minimum and maximum altitude
	 * are sane
	 */
	gboolean altitude_bounds_set;

	/** @brief Maximum altitude in metres */
	gdouble altitude_max;

	/** @brief Minimum altitude in metres */
	gdouble altitude_min;

	/**
	 * @brief Whether or not the minimum and maximum heart rates
	 * are sane
	 */
	gboolean heart_rate_bounds_set;

	/** @brief Heart rate sum for calculating the average heart rate */
	guint heart_rate_sum;

	/** @brief Heart rate count for calculating the average heart rate */
	guint heart_rate_count;

	/** @brief Average heart rate */
	gint heart_rate_avg;

	/** @brief Maximum heart rate */
	gint heart_rate_max;

	/** @brief Minimum heart rate */
	gint heart_rate_min;
} AnalyzerViewTrack;

typedef struct _AnalyzerViewTrackSegment {
	/** @brief Whether or not the time values are set */
	gboolean times_set;

	struct timeval start_time;
	struct timeval end_time;
	struct timeval duration;

	/**
	 * @brief List of waypoints associated with this track segment
	 *
	 * The pointers are of type AnalyzerViewWaypoint
	 */
	GSList *track_points;

	/**
	 * @brief List of heart rates associated with this track segment
	 *
	 * The pointers are of type AnalyzerViewHeartRate
	 */
	GSList *heart_rates;
} AnalyzerViewTrackSegment;

/**
 * @brief Waypoint structure used in analyzer view
 *
 * This structure is quite similar to the #GpxStorageWaypoint, but has
 * some additional statistics
 */
typedef struct _AnalyzerViewWaypoint {
	GpxStoragePointType point_type;

	gdouble latitude;
	gdouble longitude;

	gboolean altitude_is_set;
	gdouble altitude;

	struct timeval timestamp;

	gdouble distance_to_prev;
	struct timeval time_to_prev;

	/* Slightly averaged speed in the waypoint */
	gdouble speed_averaged;
} AnalyzerViewWaypoint;

/**
 * @brief Heart rate structure used in analyzer view
 *
 * This structure is at the moment exactly like #GpxParserDataHeartRate
 */
typedef struct _AnalyzerViewHeartRate {
	struct timeval timestamp;
	gint value;
} AnalyzerViewHeartRate;

typedef struct _AnalyzerViewColor {
	double r;
	double g;
	double b;
	double a;
} AnalyzerViewColor;

typedef struct _AnalyzerViewPixbufDetails {
	gint w;
	gint h;
	AnalyzerViewTrack *track;

	/** @brief Number of the scale lines */
	gint scale_count;

	gboolean show_speed;
	AnalyzerViewColor color_speed;

	gboolean show_heart_rate;
	AnalyzerViewColor color_heart_rate;

	gboolean show_altitude;
	AnalyzerViewColor color_altitude;
} AnalyzerViewPixbufDetails;

/*****************************************************************************
 * Private function prototypes                                               *
 *****************************************************************************/

static void analyzer_view_show_last_activity(gpointer user_data,gchar* file_name);
/**
 * @brief Create the info view
 *
 * @param self Pointer to #AnalyzerView
 *
 * @return The main widget of the info view
 */
static GtkWidget *analyzer_view_create_view_info(AnalyzerView *self);

/**
 * @brief Create the speed view
 *
 * @param self Pointer to #AnalyzerView
 *
 * @return The main widget of the speed view
 */
static GtkWidget *analyzer_view_create_view_graphs(AnalyzerView *self);

/**
 * @brief Callback for changing to the next view
 *
 * @param button The button that was clicked
 * @param user_data Pointer to #AnalyzerView
 */
static void analyzer_view_next(GtkWidget *button, gpointer user_data);

/**
 * @brief Callback for changing to the previous view
 *
 * @param button The button that was clicked
 * @param user_data Pointer to #AnalyzerView
 */
static void analyzer_view_prev(GtkWidget *button, gpointer user_data);

/**
 * @brief Scroll to the next or previous view
 *
 * @param user_data Pointer to #AnalyzerViewScrollData
 *
 * @return TRUE while scrolling, FALSE after scrolling is done
 */
static gboolean analyzer_view_scroll(gpointer user_data);

static gboolean analyzer_view_scroll_idle(gpointer user_data);

/**
 * @brief Callback for open button clicked signal
 *
 * @param button Button that was clicked
 * @param user_data Pointer to #AnalyzerView
 *
 * This function opens opens a file and invokes parsing functions.
 */
static void analyzer_view_btn_open_clicked(
		GtkWidget *button,
		gpointer user_data);

/**
 * @brief Callback for previous track button clicked signal
 *
 * @param button Button that was clicked
 * @param user_data Pointer to #AnalyzerView
 */
static void analyzer_view_btn_track_prev_clicked(
		GtkWidget *button,
		gpointer user_data);

/**
 * @brief Callback for next track button clicked signal
 *
 * @param button Button that was clicked
 * @param user_data Pointer to #AnalyzerView
 */
static void analyzer_view_btn_track_next_clicked(
		GtkWidget *button,
		gpointer user_data);
		
		
/**
 * @brief Callback for map button clicked signal
 *
 * @param button Button that was clicked
 * @param user_data Pointer to #AnalyzerView
 */
static void analyzer_view_btn_map_clicked(
		GtkWidget *button,
		gpointer user_data);


/**
 * @brief Get a file name using Hildon file chooser dialog
 *
 * @param self Pointer to #AnalyzerView
 *
 * @return The file name, or NULL if Cancel was pressed. The return value
 * must be freed with g_free.
 */
static gchar *analyzer_view_choose_file_name(AnalyzerView *self);

/**
 * @brief Remove and destroy a widget
 *
 * @param self Pointer to #AnalyzerView
 * @param widget Pointer-to-pointer to a widget that is to be removed.
 * The actual pointer to the widget will be set to NULL.
 */
static void analyzer_view_destroy_widget(
		AnalyzerView *self,
		GtkWidget **widget);

/**
 * @brief Clear the data that there is from previous parsing
 *
 * @param self Pointer to #AnalyzerView
 */
static void analyzer_view_clear_data(AnalyzerView *self);

/**
 * @brief Callback for the GPX parser
 *
 * @param data_type Type of the data
 * @param data The actual data
 * @param user_data Pointer to #AnalyzerView
 */
static void analyzer_view_gpx_parser_callback(
		GpxParserDataType data_type,
		const GpxParserData *data,
		gpointer user_data);

/**
 * @brief Add a new track
 *
 * @param self Pointer to AnalyzerView
 * @param parser_track Track to be added
 */
static void analyzer_view_add_track(
		AnalyzerView *self,
		GpxParserDataTrack *parser_track);

/**
 * @brief Add a new track segment to the newset track
 *
 * @param self Pointer to AnalyzerView
 * @param parser_track Track to be added
 */
static void analyzer_view_add_track_segment(
		AnalyzerView *self,
		GpxParserDataTrackSegment *parser_track_segment);

/**
 * @brief Add a new waypoint to the newest track segment in the newest track
 *
 * @param self Pointer to #AnalyzerView
 * @param parser_waypoint Pointer to the waypoint to be added
 */
static void analyzer_view_add_track_point(
		AnalyzerView *self,
		GpxParserDataWaypoint *parser_waypoint);

/**
 * @brief Add a new heart rate to the newest track
 *
 * @param self Pointer to #AnalyzerView
 * @param heart_rate Pointer to the heart rate to be added
 */
static void analyzer_view_add_heart_rate(
		AnalyzerView *self,
		GpxParserDataHeartRate *parser_heart_rate);

/**
 * @brief Analyze details of a track
 *
 * @param self Pointer to #AnalyzerView
 * @param track Pointer to the track to be analyzed
 */
static void analyzer_view_analyze_track(
		AnalyzerView *self,
		AnalyzerViewTrack *track);

/**
 * @brief Analyze details of a track segment
 *
 * @param self Pointer to #AnalyzerView
 * @param track Pointer to the track where the track segment is
 * @param track_segment Pointer to the track segment to analyze
 */
static void analyzer_view_analyze_track_segment(
		AnalyzerView *self,
		AnalyzerViewTrack *track,
		AnalyzerViewTrackSegment *track_segment);

/**
 * @brief Analyze heart rate information of a given track segment
 *
 * @param self Pointer to #AnalyzerView
 * @param track Pointer to the track where the track segment is
 * @param track_segment Pointer to the track segment to analyze
 */
static void analyzer_view_analyze_track_segment_heart_rates(
		AnalyzerView *self,
		AnalyzerViewTrack *track,
		AnalyzerViewTrackSegment *track_segment);

/**
 * @brief Display information of a given track
 *
 * @param self Pointer to #AnalyzerView
 * @param track Track of which the information will be shown
 */
static void analyzer_view_show_track_information(
		AnalyzerView *self,
		AnalyzerViewTrack *track);

/**
 * @brief Callback for speed button clicks
 *
 * @param button Button that was clicked
 * @param user_data Pointer to #AnalyzerView
 */
static void analyzer_view_graphs_btn_speed_clicked(
		GtkWidget *button,
		gpointer user_data);

/**
 * @brief Callback for altitude button clicks
 *
 * @param button Button that was clicked
 * @param user_data Pointer to #AnalyzerView
 */
static void analyzer_view_graphs_btn_altitude_clicked(
		GtkWidget *button,
		gpointer user_data);

/**
 * @brief Callback for heart rate button clicks
 *
 * @param button Button that was clicked
 * @param user_data Pointer to #AnalyzerView
 */
static void analyzer_view_graphs_btn_heart_rate_clicked(
		GtkWidget *button,
		gpointer user_data);

/**
 * @brief Change a graph state to either be drawn or not to be drawn
 *
 * @param self Pointer to #AnalyzerView
 * @param button Button that was clicked
 * @param state Pointer to the state of the button
 */
static void analyzer_view_graphs_toggle_button(
		AnalyzerView *self,
		GtkWidget *button,
		gboolean *state);


static gboolean analyzer_view_graphs_drawing_area_exposed(
		GtkWidget *widget,
		GdkEventExpose *event,
		gpointer user_data);

static GdkPixbuf *analyzer_view_create_pixbuf(
		AnalyzerView *self,
		AnalyzerViewPixbufDetails *details);

static void analyzer_view_destroy_pixbuf_data(
		guchar *pixels,
		gpointer user_data);

static gint analyzer_view_create_scale(
		AnalyzerView *self,
		cairo_t *cr,
		AnalyzerViewPixbufDetails *details,
		AnalyzerViewColor *color,
		gboolean left_edge,
		gdouble value_min,
		gdouble value_max,
		gint scale_height,
		GdkRectangle *graph,
		gdouble *pixels_per_unit);

static AnalyzerViewUnitType analyzer_view_create_scale_time(
		AnalyzerView *self,
		cairo_t *cr,
		struct timeval *duration,
		GdkRectangle *graph_area);

static void analyzer_view_draw_speed(
		AnalyzerView *self,
		cairo_t *cr,
		AnalyzerViewPixbufDetails *details,
		GdkRectangle *graph_area,
		gdouble pixels_per_unit);

static void analyzer_view_draw_altitude(
		AnalyzerView *self,
		cairo_t *cr,
		AnalyzerViewPixbufDetails *details,
		GdkRectangle *graph_area,
		gdouble pixels_per_unit);

static void analyzer_view_draw_heart_rate(
		AnalyzerView *self,
		cairo_t *cr,
		AnalyzerViewPixbufDetails *details,
		GdkRectangle *graph_area,
		gdouble pixels_per_unit);

static void analyzer_view_set_units(AnalyzerView *self);
static void create_map(gpointer user_data);

/**
 * @brief Free memory used by a #AnalyzerViewTrack
 *
 * @param track Pointer to a track to be freed
 */
static void analyzer_view_track_destroy(AnalyzerViewTrack *track);
gboolean map_button_press_cb(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
gboolean map_button_release_cb (GtkWidget *widget, GdkEventButton *event, gpointer user_data);
/*****************************************************************************
 * Function declarations                                                     *
 *****************************************************************************/

/*===========================================================================*
 * Public functions                                                          *
 *===========================================================================*/

AnalyzerView *analyzer_view_new(
		GtkWindow *parent_window,
		GConfHelperData *gconf_helper)
{
	AnalyzerView *self = NULL;
	DEBUG_BEGIN();

	self = g_new0(AnalyzerView, 1);
	self->parent_window = parent_window;
	self->gconf_helper = gconf_helper;



	self->main_widget = gtk_fixed_new();


	self->btn_back = ec_button_new();
	ec_button_set_label_text(EC_BUTTON(self->btn_back), _("Back"));

	ec_button_set_bg_image(EC_BUTTON(self->btn_back),
			EC_BUTTON_STATE_RELEASED,
			GFXDIR "ec_button_small_bg.png");

	ec_button_set_center_vertically(EC_BUTTON(self->btn_back),
			TRUE);

	ec_button_set_center_text_vertically(EC_BUTTON(self->btn_back), TRUE);
	ec_button_set_btn_down_offset(EC_BUTTON(self->btn_back), 2);

	gtk_widget_set_size_request(self->btn_back, 74, 59);
	//gtk_fixed_put(GTK_FIXED(self->main_widget), self->btn_back,
	//		708, 18);

	DEBUG_END();
	return self;
}

void analyzer_view_set_default_folder(
		AnalyzerView *self,
		const gchar *folder)
{
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	g_free(self->default_folder_name);
	self->default_folder_name = g_strdup(folder);

	DEBUG_END();
}

static GtkWidget *create_table ()
{

  GtkWidget *hbox;
  GtkWidget *table;
  GtkWidget *button;
  char buffer[32];
  int i, j;

  /* create a table of 10 by 10 squares. */
  table = gtk_table_new (20, 20, FALSE);

  /* set the spacing to 10 on x and 10 on y */
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);

  gtk_widget_show (table);

  /* this simply creates a grid of toggle buttons on the table
   * to demonstrate the scrolled window. */
  for (i = 1; i < 20; i++)
    for (j = 1; j < 20; j++) {
      sprintf (buffer, "button (%d,%d)\n", i, j);
      button = gtk_toggle_button_new_with_label (buffer);

      gtk_table_attach_defaults (GTK_TABLE (table), button,
                                 i, i+1, j, j+1);
    }

  return table;
}


void analyzer_view_show(AnalyzerView *self)
{
	gint i;
	PangoFontDescription *desc;
	GtkWidget * menu_button;
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	self->win = hildon_stackable_window_new ();
	gtk_window_set_title ( GTK_WINDOW (self->win), "eCoach >Activity log");
	gtk_widget_set_name(GTK_WIDGET(self->win), "mainwindow");
	self->menu = hildon_app_menu_new ();
	menu_button = hildon_gtk_button_new (HILDON_SIZE_AUTO);
	gtk_button_set_label (GTK_BUTTON (menu_button),"Upload to service");

 	hildon_app_menu_append (self->menu, GTK_BUTTON (menu_button));
	hildon_window_set_app_menu (HILDON_WINDOW (self->win), self->menu);

	gtk_widget_show_all(self->menu);
	g_signal_connect (self->win, "destroy", G_CALLBACK (analyzer_view_hide), self);
	

	self->main_table = gtk_table_new(5,2,TRUE);
	
	
	/* Check units to use */
	if (gconf_helper_get_value_bool_with_default(self->gconf_helper,
	    USE_METRIC, TRUE))
	{
		self->metric=TRUE;
	}
	else
	{
		self->metric=FALSE;
	}
	

	/* Open button */
	self->btn_open = ec_button_new();
	ec_button_set_bg_image(EC_BUTTON(self->btn_open),
			EC_BUTTON_STATE_RELEASED,
			GFXDIR "ec_button_small_bg.png");
	ec_button_set_icon(EC_BUTTON(self->btn_open),
			GFXDIR "ec_icon_open.png");
	ec_button_set_center_vertically(EC_BUTTON(self->btn_open), TRUE);
	ec_button_set_btn_down_offset(EC_BUTTON(self->btn_open), 2);
	gtk_widget_set_size_request(self->btn_open, 74, 59);
	gtk_fixed_put(GTK_FIXED(self->main_widget), self->btn_open,
			18, 18);
	//gtk_table_attach_defaults (GTK_TABLE (self->main_table), self->btn_open, 0, 1, 0, 1);


	g_signal_connect(G_OBJECT(self->btn_open), "clicked",
			G_CALLBACK(analyzer_view_btn_open_clicked), self);

	/* Previous track button */
	self->btn_track_prev = ec_button_new();
	ec_button_set_bg_image(EC_BUTTON(self->btn_track_prev),
			EC_BUTTON_STATE_RELEASED,
			GFXDIR "ec_button_small_bg.png");
	ec_button_set_icon(EC_BUTTON(self->btn_track_prev),
			GFXDIR "ec_icon_track_prev.png");
	ec_button_set_center_vertically(EC_BUTTON(self->btn_track_prev),
			TRUE);
	ec_button_set_btn_down_offset(EC_BUTTON(self->btn_track_prev), 2);

	gtk_widget_set_size_request(self->btn_track_prev, 74, 59);
	gtk_fixed_put(GTK_FIXED(self->main_widget), self->btn_track_prev,
			98, 18);
	//gtk_table_attach_defaults (GTK_TABLE (self->main_table), self->btn_track_prev, 1, 2, 0, 1);
	g_signal_connect(G_OBJECT(self->btn_track_prev), "clicked",
			G_CALLBACK(analyzer_view_btn_track_prev_clicked),
			self);

	/* Next track button */
	self->btn_track_next = ec_button_new();
	ec_button_set_bg_image(EC_BUTTON(self->btn_track_next),
			EC_BUTTON_STATE_RELEASED,
			GFXDIR "ec_button_small_bg.png");
	ec_button_set_icon(EC_BUTTON(self->btn_track_next),
			GFXDIR "ec_icon_track_next.png");
	ec_button_set_center_vertically(EC_BUTTON(self->btn_track_next),
			TRUE);
	ec_button_set_btn_down_offset(EC_BUTTON(self->btn_track_next), 2);

	gtk_widget_set_size_request(self->btn_track_next, 74, 59);
	gtk_fixed_put(GTK_FIXED(self->main_widget), self->btn_track_next,
			178, 18);
	//gtk_table_attach_defaults (GTK_TABLE (self->main_table), self->btn_track_next, 2, 3, 0, 1);
	g_signal_connect(G_OBJECT(self->btn_track_next), "clicked",
			G_CALLBACK(analyzer_view_btn_track_next_clicked),
			self);

	/* Track number */
	self->lbl_track_number = gtk_label_new(_("No tracks"));
	gtk_fixed_put(GTK_FIXED(self->main_widget), self->lbl_track_number,
			260, 20);
	gtk_widget_set_size_request(self->lbl_track_number,
			150, 55);
	//gtk_table_attach_defaults (GTK_TABLE (self->main_table), self->lbl_track_number, 3, 4, 0, 1);
	/* Track details */
	self->lbl_track_details = gtk_label_new(_("No track information "
				"available "));
	gtk_fixed_put(GTK_FIXED(self->main_widget), self->lbl_track_details,
			420, 20);
	//gtk_table_attach_defaults (GTK_TABLE (self->main_table), self->lbl_track_details, 4, 5, 0, 1);
	gtk_widget_set_size_request(self->lbl_track_details, 300, 55);
	desc = pango_font_description_from_string("Nokia Sans 14");
	gtk_widget_modify_font(self->lbl_track_details, desc);
	pango_font_description_free(desc);


	
	/* Map button */
	
	self->btn_map = ec_button_new();
	ec_button_set_bg_image(EC_BUTTON(self->btn_map),
			EC_BUTTON_STATE_RELEASED,
			GFXDIR "ec_button_map.png");
	ec_button_set_btn_down_offset(EC_BUTTON(self->btn_map), 2);
	gtk_widget_set_size_request(self->btn_map, 74, 59);
	gtk_fixed_put(GTK_FIXED(self->main_widget), self->btn_map,
			708, 18);
	g_signal_connect(G_OBJECT(self->btn_map), "clicked",
			G_CALLBACK(analyzer_view_btn_map_clicked),
			self);
			

	self->data = gtk_table_new(1,2,TRUE);
      /* Create the views */
	self->views[ANALYZER_VIEW_INFO] =
		analyzer_view_create_view_info(self);

	self->views[ANALYZER_VIEW_GRAPHS] =
		analyzer_view_create_view_graphs(self);
	
	create_map(self);
		
	for(i = 0; i < ANALYZER_VIEW_COUNT; i++)
	{
		gtk_widget_show_all(self->views[i]);
		g_object_ref(G_OBJECT(self->views[i]));
		gtk_widget_set_size_request(self->views[i],
				ANALYZER_VIEW_WIDTH, ANALYZER_VIEW_HEIGHT);
	}

	gtk_table_attach_defaults (GTK_TABLE (self->data), self->views[ANALYZER_VIEW_INFO], 0, 1, 0, 1);
	gtk_table_attach_defaults (GTK_TABLE (self->data), self->views[ANALYZER_VIEW_GRAPHS], 1, 2, 0, 1);

	self->pannable = hildon_pannable_area_new ();
	gtk_widget_set_name(GTK_WIDGET(self->pannable), "mainwindow");
	g_object_set (G_OBJECT(self->pannable),"mov-mode", HILDON_MOVEMENT_MODE_HORIZ );

	//g_object_set (G_OBJECT (self->pannable),"low-friction-mode", TRUE);

//	g_object_set (G_OBJECT (self->pannable),
//               "deceleration", 0.01);

	hildon_pannable_area_add_with_viewport (HILDON_PANNABLE_AREA (self->pannable), self->data);

	self->pan = gtk_table_new(1,1,TRUE);
	GtkWidget *weight_event;;
	weight_event = gtk_event_box_new ();


	GdkColor black;
	black.red = 0;
	black.green = 0;
	black.blue = 0;

	gtk_widget_modify_bg(weight_event,0,&black);

	gtk_widget_set_size_request(self->pannable,
			762,
			340);
	gtk_container_add (GTK_CONTAINER (self->pan), self->pannable);
	gtk_container_add (GTK_CONTAINER (weight_event), self->pan);

	gtk_fixed_put(GTK_FIXED(self->main_widget), weight_event,
			18, 85);

	/*Open last activity if there is a one */
	
	gtk_container_add (GTK_CONTAINER (self->win),self->main_widget);	
	gtk_widget_show_all(self->win);
	self->current_view = 0;
	gchar *file_name = gconf_helper_get_value_string_with_default(self->gconf_helper,LAST_ACTIVITY,"");
	DEBUG("%s",file_name);
	if(self->activity_state ==3)
	{
	analyzer_view_show_last_activity(self,file_name);
	}
	DEBUG_END();
}

void analyzer_view_hide(AnalyzerView *self)
{
	gint i;
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	//analyzer_view_clear_data(self);

	analyzer_view_destroy_widget(self, &self->btn_open);
	analyzer_view_destroy_widget(self, &self->btn_track_prev);
	analyzer_view_destroy_widget(self, &self->btn_track_next);
	analyzer_view_destroy_widget(self, &self->lbl_track_number);
	analyzer_view_destroy_widget(self, &self->lbl_track_details);
	analyzer_view_destroy_widget(self, &self->btn_view_prev);
	analyzer_view_destroy_widget(self, &self->btn_view_next);

	/* The views are easy to destroy, since destroying the top level
	 * widget is enough */
	for(i = 0; i < ANALYZER_VIEW_COUNT; i++)
	{
		//gtk_widget_destroy(self->views[i]);
		self->views[i] = NULL;
	}

	//analyzer_view_destroy_widget(self, &self->scrolled);

	DEBUG_END();
}

/*===========================================================================*
 * Private functions                                                         *
 *===========================================================================*/

static GtkWidget *analyzer_view_create_view_info(AnalyzerView *self)
{
	gint i;




	g_return_val_if_fail(self != NULL, NULL);
	DEBUG_BEGIN();

	self->info_table = gtk_table_new(8, 2, TRUE);
	gtk_table_set_col_spacings(GTK_TABLE(self->info_table), 10);

	self->info_labels[ANALYZER_VIEW_INFO_LABEL_NAME][0] =
		gtk_label_new(_("Activity name"));

/*	self->info_labels[ANALYZER_VIEW_INFO_LABEL_COMMENT][0] =
		gtk_label_new(_("Comment"));*/

	self->info_labels[ANALYZER_VIEW_INFO_LABEL_TIME_START][0] =
		gtk_label_new(_("Start time"));

	self->info_labels[ANALYZER_VIEW_INFO_LABEL_TIME_END][0] =
		gtk_label_new(_("End time"));

	self->info_labels[ANALYZER_VIEW_INFO_LABEL_DURATION][0] =
		gtk_label_new(_("Duration"));

	self->info_labels[ANALYZER_VIEW_INFO_LABEL_DISTANCE][0] =
		gtk_label_new(_("Distance"));

	self->info_labels[ANALYZER_VIEW_INFO_LABEL_SPEED_AVG][0] =
		gtk_label_new(_("Average speed"));

	self->info_labels[ANALYZER_VIEW_INFO_LABEL_SPEED_MAX][0] =
		gtk_label_new(_("Maximum speed"));
		
	self->info_labels[ANALYZER_VIEW_INFO_LABEL_MIN_PER_KM][0] =
		gtk_label_new(_("Min/km"));

	self->info_labels[ANALYZER_VIEW_INFO_LABEL_HEART_RATE_AVG][0] =
		gtk_label_new(_("Average heart rate"));

	self->info_labels[ANALYZER_VIEW_INFO_LABEL_HEART_RATE_MAX][0] =
		gtk_label_new(_("Maximum heart rate"));

	for(i = 0; i < ANALYZER_VIEW_INFO_LABEL_COUNT; i++)
	{
		self->info_labels[i][1] = gtk_label_new(_("N/A"));

		gtk_misc_set_alignment(
				GTK_MISC(self->info_labels[i][0]),
				1, 0.5);

		gtk_misc_set_alignment(
				GTK_MISC(self->info_labels[i][1]),
				0, 0.5);

		gtk_table_attach_defaults(GTK_TABLE(self->info_table),
				self->info_labels[i][0],
				0, 1,
				i, i + 1);

		gtk_table_attach_defaults(GTK_TABLE(self->info_table),
				self->info_labels[i][1],
				1, 2,
				i, i + 1);
	}

	DEBUG_END();
	return self->info_table;
}

static GtkWidget *analyzer_view_create_view_graphs(AnalyzerView *self)
{
	g_return_val_if_fail(self != NULL, NULL);
	DEBUG_BEGIN();

	self->graphs_table = gtk_table_new(2, 3, FALSE);


	/* Speed button */
	self->show_speed = TRUE;
	self->graphs_btn_speed = ec_button_new();
	ec_button_set_bg_image(EC_BUTTON(self->graphs_btn_speed),
			EC_BUTTON_STATE_RELEASED,
			GFXDIR "ec_button_small_bg_down.png");
	ec_button_set_icon(EC_BUTTON(self->graphs_btn_speed),
			GFXDIR "ec_icon_speed.png");
	ec_button_set_center_vertically(EC_BUTTON(self->graphs_btn_speed),
			TRUE);
	gtk_widget_set_size_request(self->graphs_btn_speed, 74, 59);
	ec_button_set_btn_down_offset(EC_BUTTON(self->graphs_btn_speed), 2);
	gtk_table_attach(GTK_TABLE(self->graphs_table),
			self->graphs_btn_speed,
			0, 1,
			0, 1,
			GTK_SHRINK,
			GTK_SHRINK,
			0, 0);
	g_signal_connect(G_OBJECT(self->graphs_btn_speed), "clicked",
			G_CALLBACK(analyzer_view_graphs_btn_speed_clicked),
			self);

	/* Altitude button */
	self->show_altitude = FALSE;
	self->graphs_btn_altitude = ec_button_new();
	ec_button_set_bg_image(EC_BUTTON(self->graphs_btn_altitude),
			EC_BUTTON_STATE_RELEASED,
			GFXDIR "ec_button_small_bg.png");

	ec_button_set_icon(EC_BUTTON(self->graphs_btn_altitude),
			GFXDIR "ec_icon_altitude.png");

	ec_button_set_center_vertically(EC_BUTTON(self->graphs_btn_altitude),
			TRUE);
	gtk_widget_set_size_request(self->graphs_btn_altitude, 74, 59);
	ec_button_set_btn_down_offset(EC_BUTTON(self->graphs_btn_altitude),
			2);
	gtk_table_attach(GTK_TABLE(self->graphs_table),
			self->graphs_btn_altitude,
			0, 1,
			1, 2,
			GTK_SHRINK,
			GTK_SHRINK,
			0, 0);
	g_signal_connect(G_OBJECT(self->graphs_btn_altitude), "clicked",
			G_CALLBACK(analyzer_view_graphs_btn_altitude_clicked),
			self);

	/* Heart rate button */
	self->show_heart_rate = FALSE;
	self->graphs_btn_heart_rate = ec_button_new();
	ec_button_set_bg_image(EC_BUTTON(self->graphs_btn_heart_rate),
			EC_BUTTON_STATE_RELEASED,
			GFXDIR "ec_button_small_bg.png");

	ec_button_set_icon(EC_BUTTON(self->graphs_btn_heart_rate),
			GFXDIR "ec_icon_heart_rate.png");

	ec_button_set_center_vertically(
			EC_BUTTON(self->graphs_btn_heart_rate),
			TRUE);
	gtk_widget_set_size_request(self->graphs_btn_heart_rate, 74, 59);
	ec_button_set_btn_down_offset(EC_BUTTON(self->graphs_btn_heart_rate),
			2);
	gtk_table_attach(GTK_TABLE(self->graphs_table),
			self->graphs_btn_heart_rate,
			0, 1,
			2, 3,
			GTK_SHRINK,
			GTK_SHRINK,
			0, 0);
	g_signal_connect(G_OBJECT(self->graphs_btn_heart_rate), "clicked",
			G_CALLBACK(analyzer_view_graphs_btn_heart_rate_clicked),
			self);

	/* Drawing area for the graph */
	self->graphs_drawing_area = gtk_drawing_area_new();
	gtk_table_attach(GTK_TABLE(self->graphs_table),
			self->graphs_drawing_area,
			1, 2,
			0, 3,
			GTK_EXPAND | GTK_FILL,
			GTK_EXPAND | GTK_FILL,
			0, 0);

	g_signal_connect(G_OBJECT(self->graphs_drawing_area),
			"expose-event",
			G_CALLBACK(analyzer_view_graphs_drawing_area_exposed),
			self);

	return self->graphs_table;

	DEBUG_END();
}

static void analyzer_view_next(GtkWidget *button, gpointer user_data)
{
	GtkWidget *current_view;
	GtkWidget *next_view;
	AnalyzerViewScrollData *scroll_data = NULL;

	AnalyzerView *self = (AnalyzerView *)user_data;

	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	if(self->is_scrolling)
	{
		return;
	}
	self->is_scrolling = TRUE;

	/* First, place the current view to (0, 0) */
	current_view = self->views[self->current_view];
	gtk_fixed_move(GTK_FIXED(self->fixed_view),
			current_view,
			0, 0);

	/* Then, place the next view to (ANALYZER_VIEW_WIDTH, 0) */
	if(self->current_view == ANALYZER_VIEW_COUNT - 1)
	{
		DEBUG("Going back to first view");
		next_view = self->views[0];
		self->current_view = 0;
	} else {
		DEBUG("Going to next view");
		next_view = self->views[self->current_view + 1];
		self->current_view++;
	}

	/* Create the necessary data for the scrolling */
	scroll_data = g_new0(AnalyzerViewScrollData, 1);
	scroll_data->analyzer_view = self;
	scroll_data->start_x = 0;
	scroll_data->end_x = ANALYZER_VIEW_WIDTH;
	scroll_data->start_widget = current_view;
	scroll_data->end_widget = next_view;

	/* And finally, scroll to the view */
	g_timeout_add(50, analyzer_view_scroll, scroll_data);

	DEBUG_END();
}

static void analyzer_view_prev(GtkWidget *button, gpointer user_data)
{
	GtkWidget *current_view;
	GtkWidget *next_view;
	AnalyzerViewScrollData *scroll_data = NULL;

	AnalyzerView *self = (AnalyzerView *)user_data;

	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	if(self->is_scrolling)
	{
		return;
	}
	self->is_scrolling = TRUE;

	/* First, place the current view to (ANALYZER_VIEW_WIDTH, 0)
	 * and scroll it to be visible */
	current_view = self->views[self->current_view];
	gtk_adjustment_set_value(gtk_scrolled_window_get_hadjustment(
				GTK_SCROLLED_WINDOW(self->scrolled)),
			1);
	gtk_fixed_move(GTK_FIXED(self->fixed_view),
			current_view,
			ANALYZER_VIEW_WIDTH, 0);

	/* Then, place the next view to (0, 0) */
	if(self->current_view == 0)
	{
		DEBUG("Going to last view");
		next_view = self->views[ANALYZER_VIEW_COUNT - 1];
		self->current_view = ANALYZER_VIEW_COUNT - 1;
	} else {
		DEBUG("Going to previous view");
		next_view = self->views[self->current_view - 1];
		self->current_view--;
	}

	/* Create the necessary data for the scrolling */
	scroll_data = g_new0(AnalyzerViewScrollData, 1);
	scroll_data->analyzer_view = self;
	scroll_data->start_x = ANALYZER_VIEW_WIDTH;
	scroll_data->end_x = 0;
	scroll_data->start_widget = current_view;
	scroll_data->end_widget = next_view;

	/* And finally, scroll to the view */
	g_timeout_add(50, analyzer_view_scroll, scroll_data);

	DEBUG_END();
}

static gboolean analyzer_view_scroll(gpointer user_data)
{
	AnalyzerView *self = NULL;
	AnalyzerViewScrollData *scroll_data =
		(AnalyzerViewScrollData *)user_data;

	g_return_val_if_fail(scroll_data != NULL, FALSE);

	self = scroll_data->analyzer_view;

	if(self->is_scrolling)
	{
		g_idle_add(analyzer_view_scroll_idle, user_data);
		return TRUE;
	} else {
		g_free(scroll_data);
	}
	return FALSE;
}

static gboolean analyzer_view_scroll_idle(gpointer user_data)
{
	AnalyzerView *self = NULL;
	GtkAdjustment *adjustment = NULL;
	gdouble value = 0;

	AnalyzerViewScrollData *scroll_data =
		(AnalyzerViewScrollData *)user_data;

	g_return_val_if_fail(scroll_data != NULL, FALSE);
	DEBUG_BEGIN();

	self = scroll_data->analyzer_view;

	gdk_threads_enter();

	adjustment = gtk_scrolled_window_get_hadjustment(
			GTK_SCROLLED_WINDOW(self->scrolled));

	if(scroll_data->current_step == 0)
	{
		/* This needs to be done here to prevent the next view
		 * from flashing before the scroll starts. */
		gtk_fixed_put(GTK_FIXED(self->fixed_view),
				scroll_data->end_widget,
				scroll_data->end_x, 0);
	}

	if(scroll_data->current_step < 20)
	{
		scroll_data->current_step++;
		if(scroll_data->end_x > scroll_data->start_x)
		{
			value = (gdouble)(scroll_data->end_x -
					scroll_data->start_x) *
				(gdouble)scroll_data->current_step / 20.0;
		} else {
			value = (gdouble)(scroll_data->start_x -
					scroll_data->end_x) *
				(gdouble)(20 - scroll_data->current_step) /
				20.0;
		}
		gtk_adjustment_set_value(adjustment, value);
	} else {
		/* Final step */
		DEBUG("Removing start widget");
		gtk_container_remove(GTK_CONTAINER(self->fixed_view),
				scroll_data->start_widget);

		DEBUG("Moving end widget");
		gtk_fixed_move(GTK_FIXED(self->fixed_view),
				scroll_data->end_widget,
				0, 0);
		gtk_adjustment_set_value(adjustment, 0);
		self->is_scrolling = FALSE;
		gdk_threads_leave();
		DEBUG_END();
		return FALSE;
	}
	gdk_threads_leave();
	DEBUG_END();
	return FALSE;
}

static void analyzer_view_destroy_widget(
		AnalyzerView *self,
		GtkWidget **widget)
{
	g_return_if_fail(self != NULL);
	g_return_if_fail(widget != NULL);

	DEBUG_BEGIN();

	gtk_container_remove(GTK_CONTAINER(self->main_widget), *widget);
	*widget = NULL;

	DEBUG_END();
}
static void analyzer_view_show_last_activity(gpointer user_data,gchar* file_name){
  
  	GSList *temp = NULL;
	GSList *temp2 = NULL;
	AnalyzerViewTrack *track = NULL;
	AnalyzerViewTrackSegment *track_segment = NULL;
	GpxParserStatus parser_status;
	//gchar *file_name = NULL;
	GError *error = NULL;

	AnalyzerView *self = (AnalyzerView *)user_data;

	g_return_if_fail(self != NULL);
	DEBUG_BEGIN(); 
	
	//file_name = analyzer_view_choose_file_name(self);
	if(!file_name || !g_strcmp0(file_name,""))
	{
		DEBUG_END();
		return;
	}

	analyzer_view_clear_data(self);
	osm_gps_map_clear_gps(OSM_GPS_MAP(self->map));
	parser_status = gpx_parser_parse_file(
			file_name,
			analyzer_view_gpx_parser_callback,
			self,
			&error);

	if(parser_status == GPX_PARSER_STATUS_PARTIALLY_OK)
	{
		ec_error_show_message_error_printf(
				_("Some of the data could not be parsed:\n\n"
					"%s"),
				error->message);
		g_error_free(error);
	} else if(parser_status == GPX_PARSER_STATUS_FAILED) {
		ec_error_show_message_error_printf(
				"The file could not be opened:\n\n"
					"%s",
				error->message);
		g_error_free(error);
		DEBUG_END();
		return;
	}
	if(self->tracks)
	{

		self->tracks = g_slist_reverse(self->tracks);

		for(temp = self->tracks; temp; temp = g_slist_next(temp))
		{
			track = (AnalyzerViewTrack *)temp->data;
			track->track_segments = g_slist_reverse(
					track->track_segments);
			for(temp2 = track->track_segments; temp2;
					temp2 = g_slist_next(temp2))
			{
				track_segment = (AnalyzerViewTrackSegment *)
					temp2->data;
				track_segment->track_points = g_slist_reverse(
						track_segment->track_points);
				track_segment->heart_rates = g_slist_reverse(
					track_segment->heart_rates);
			}
		}

		analyzer_view_show_track_information(
				self,
				(AnalyzerViewTrack *)self->tracks->data);
	}

	g_free(file_name);
	DEBUG_END();
  
}
static void analyzer_view_btn_open_clicked(
		GtkWidget *button,
		gpointer user_data)
{
	GSList *temp = NULL;
	GSList *temp2 = NULL;
	AnalyzerViewTrack *track = NULL;
	AnalyzerViewTrackSegment *track_segment = NULL;
	GpxParserStatus parser_status;
	gchar *file_name = NULL;
	GError *error = NULL;

	AnalyzerView *self = (AnalyzerView *)user_data;

	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	if(self->is_scrolling)
	{
		DEBUG_END();
		return;
	}

	file_name = analyzer_view_choose_file_name(self);
	if(!file_name)
	{
		DEBUG_END();
		return;
	}

	analyzer_view_clear_data(self);
	osm_gps_map_clear_gps(OSM_GPS_MAP(self->map));
	parser_status = gpx_parser_parse_file(
			file_name,
			analyzer_view_gpx_parser_callback,
			self,
			&error);

	if(parser_status == GPX_PARSER_STATUS_PARTIALLY_OK)
	{
		ec_error_show_message_error_printf(
				_("Some of the data could not be parsed:\n\n"
					"%s"),
				error->message);
		g_error_free(error);
	} else if(parser_status == GPX_PARSER_STATUS_FAILED) {
		ec_error_show_message_error_printf(
				"The file could not be opened:\n\n"
					"%s",
				error->message);
		g_error_free(error);
		DEBUG_END();
		return;
	}
	if(self->tracks)
	{

		self->tracks = g_slist_reverse(self->tracks);

		for(temp = self->tracks; temp; temp = g_slist_next(temp))
		{
			track = (AnalyzerViewTrack *)temp->data;
			track->track_segments = g_slist_reverse(
					track->track_segments);
			for(temp2 = track->track_segments; temp2;
					temp2 = g_slist_next(temp2))
			{
				track_segment = (AnalyzerViewTrackSegment *)
					temp2->data;
				track_segment->track_points = g_slist_reverse(
						track_segment->track_points);
				track_segment->heart_rates = g_slist_reverse(
					track_segment->heart_rates);
			}
		}

		analyzer_view_show_track_information(
				self,
				(AnalyzerViewTrack *)self->tracks->data);
	}

	g_free(file_name);
	DEBUG_END();
}

static void analyzer_view_btn_track_prev_clicked(
		GtkWidget *button,
		gpointer user_data)
{
	gint track_count = 0;
	AnalyzerViewTrack *track = NULL;

	AnalyzerView *self = (AnalyzerView *)user_data;

	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	if(self->is_scrolling)
	{
		DEBUG_END();
		return;
	}

	track_count = g_slist_length(self->tracks);
	self->current_track_number--;
	if(self->current_track_number < 0)
	{
		self->current_track_number = track_count - 1;
	}

	DEBUG("Track number: %d", self->current_track_number);
	track = (AnalyzerViewTrack *)g_slist_nth_data(self->tracks,
			self->current_track_number);

	analyzer_view_show_track_information(self, track);

	DEBUG_END();
}

static void analyzer_view_btn_track_next_clicked(
		GtkWidget *button,
		gpointer user_data)
{
	gint track_count = 0;
	AnalyzerViewTrack *track = NULL;

	AnalyzerView *self = (AnalyzerView *)user_data;

	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	if(self->is_scrolling)
	{
		DEBUG_END();
		return;
	}

	track_count = g_slist_length(self->tracks);
	self->current_track_number++;
	if(self->current_track_number >= track_count)
	{
		self->current_track_number = 0;
	}

	DEBUG("Track number: %d", self->current_track_number);
	track = (AnalyzerViewTrack *)g_slist_nth_data(self->tracks,
			self->current_track_number);

	analyzer_view_show_track_information(self, track);

	DEBUG_END();
}

static void create_map(gpointer user_data){

 AnalyzerView *self = (AnalyzerView *)user_data;
 self->map_win = hildon_stackable_window_new ();
 g_signal_connect(self->map_win, "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);
self->zoom_in =   gdk_pixbuf_new_from_file(GFXDIR "ec_map_zoom_in.png",NULL);
self->zoom_out = gdk_pixbuf_new_from_file(GFXDIR "ec_map_zoom_out.png",NULL);
 self->map_provider = (OsmGpsMapSource_t)gconf_helper_get_value_int_with_default(self->gconf_helper,MAP_SOURCE,1);
 if(self->map_provider==0)
 {
   self->map_provider = (OsmGpsMapSource_t)1;
  }
  self->friendly_name = osm_gps_map_source_get_friendly_name(self->map_provider);
  self->cachedir = g_build_filename(
                        g_get_user_cache_dir(),
                        "osmgpsmap",
			self->friendly_name,
                        NULL);
  self->fullpath = TRUE;
  self->map = (GtkWidget*)g_object_new (OSM_TYPE_GPS_MAP,
                        "map-source",(OsmGpsMapSource_t)self->map_provider,
                        "tile-cache",self->cachedir,
                        "tile-cache-is-full-path",self->fullpath,
                        "proxy-uri",g_getenv("http_proxy"),
                        NULL);
  g_free(self->cachedir);
  osm_gps_map_add_button((OsmGpsMap*)self->map,0, 150, self->zoom_out);
  osm_gps_map_add_button((OsmGpsMap*)self->map,720, 150, self->zoom_in);
  	g_signal_connect (self->map, "button-press-event",
                      G_CALLBACK (map_button_press_cb),self);
 	g_signal_connect (self->map, "button-release-event",
                    G_CALLBACK (map_button_release_cb), self);
  gtk_container_add (GTK_CONTAINER (self->map_win),self->map);
  
}
gboolean
map_button_release_cb (GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{

     AnalyzerView *self = (AnalyzerView *)user_data;
     if(user_data == NULL){ return FALSE;}
      DEBUG_BEGIN();
    float lat,lon;
   // GtkEntry *entry = GTK_ENTRY(user_data);
    OsmGpsMap *map = OSM_GPS_MAP(widget);

    g_object_get(map, "latitude", &lat, "longitude", &lon, NULL);
    DEBUG_END();
    return FALSE;


    }

gboolean map_button_press_cb(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
      AnalyzerView *self = (AnalyzerView *)user_data;
      int zoom = 0;
      coord_t coord;
      if(user_data ==NULL){
	return FALSE;}
      DEBUG_BEGIN();
      
       OsmGpsMap *map = OSM_GPS_MAP(widget);
       
 if ( (event->button == 1) && (event->type == GDK_BUTTON_PRESS) )
    {
      coord = osm_gps_map_get_co_ordinates(map, (int)event->x, (int)event->y);
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
      
    }
 
      DEBUG_END();
       return FALSE;
}

static void analyzer_view_btn_map_clicked(
		GtkWidget *button,
		gpointer user_data)
{
  AnalyzerViewTrack *track = NULL;
  AnalyzerView *self = (AnalyzerView *)user_data;

  DEBUG_BEGIN();
  

  gtk_widget_show_all(self->map_win);
  osm_gps_map_set_mapcenter(OSM_GPS_MAP(self->map),self->lat, self->lon,14);
  DEBUG_END();
  
}
static gchar *analyzer_view_choose_file_name(AnalyzerView *self)
{
	HildonFileSystemModel *fs_model = NULL;
	GtkWidget *file_dialog = NULL;
	GtkFileFilter *file_filter = NULL;
	gchar *file_name = NULL;
	gint result;

	g_return_val_if_fail(self != NULL, NULL);
	DEBUG_BEGIN();

	fs_model = HILDON_FILE_SYSTEM_MODEL(
			g_object_new(HILDON_TYPE_FILE_SYSTEM_MODEL,
				"ref_widget", GTK_WIDGET(self->parent_window),
				NULL));

	if(!fs_model)
	{
		ec_error_show_message_error(
				_("Unable to open File chooser dialog"));
		DEBUG_END();
		return NULL;
	}

	self->default_folder_name = gconf_helper_get_value_string_with_default(self->gconf_helper,ECGC_DEFAULT_FOLDER,"/home/user/MyDocs");
	file_dialog = hildon_file_chooser_dialog_new_with_properties(
			GTK_WINDOW(self->parent_window),
			"file_system_model", fs_model,
			"action", GTK_FILE_CHOOSER_ACTION_OPEN,
			NULL);



	file_filter = gtk_file_filter_new();
	gtk_file_filter_set_name(file_filter, _("GPX files"));
	gtk_file_filter_add_pattern(file_filter, "*.gpx");

	gtk_file_chooser_add_filter(
			GTK_FILE_CHOOSER(file_dialog),
			file_filter);

	gtk_file_chooser_set_filter(
			GTK_FILE_CHOOSER(file_dialog),
			file_filter);
	gtk_file_chooser_set_current_folder(
			GTK_FILE_CHOOSER(file_dialog),
			self->default_folder_name);

	gtk_widget_show_all(file_dialog);
	result = gtk_dialog_run(GTK_DIALOG(file_dialog));

	if(result == GTK_RESPONSE_CANCEL)
	{
		DEBUG_LONG("User cancelled the operation");
		gtk_widget_destroy(file_dialog);
		DEBUG_END();
		return NULL;
	}
    /*** TODO proper maemo5 filechooser implementation ***/

/*
	gconf_helper_set_value_string(
			self->gconf_helper,
			ECGC_DEFAULT_FOLDER,
			gtk_file_chooser_get_current_folder(
				GTK_FILE_CHOOSER(file_dialog)));
*/
	file_name = gtk_file_chooser_get_filename(
			GTK_FILE_CHOOSER(file_dialog));

	gtk_widget_destroy(file_dialog);
	//analyzer_view_set_units(self);
	

	DEBUG_END();
	return file_name;
}

static void analyzer_view_clear_data(AnalyzerView *self)
{
	gint i;
	GSList *temp = NULL;

	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	/* Free memory used by the graphs */
	if(self->graphs_pixbuf)
	{
		g_object_unref(G_OBJECT(self->graphs_pixbuf));
		self->graphs_pixbuf = NULL;
	}

	/* Clear the info labels */
	for(i = 0; i < ANALYZER_VIEW_INFO_LABEL_COUNT; i++)
	{
		gtk_label_set_text(GTK_LABEL(self->info_labels[i][1]),
				_("N/A"));
	}
	gtk_label_set_text(GTK_LABEL(self->lbl_track_number),
		       _("No tracks"));
	gtk_label_set_text(GTK_LABEL(self->lbl_track_details),
		       _("No track information avail."));

	/* Clear all the tracks */
	for(temp = self->tracks; temp; temp = g_slist_next(temp))
	{
		analyzer_view_track_destroy((AnalyzerViewTrack *)temp->data);
	}

	g_slist_free(self->tracks);
	self->tracks = NULL;

	self->current_track_number = 0;

	gtk_widget_queue_draw(self->graphs_drawing_area);

	DEBUG_END();
}

static void analyzer_view_gpx_parser_callback(
		GpxParserDataType data_type,
		const GpxParserData *data,
		gpointer user_data)
{
	AnalyzerView *self = (AnalyzerView *)user_data;
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	switch(data_type)
	{
		case GPX_PARSER_DATA_TYPE_TRACK:
			analyzer_view_add_track(self, data->track);
			break;
		case GPX_PARSER_DATA_TYPE_TRACK_SEGMENT:
			analyzer_view_add_track_segment(self,
					data->track_segment);
			break;
		case GPX_PARSER_DATA_TYPE_WAYPOINT:
			switch(data->waypoint->point_type)
			{
				case GPX_STORAGE_POINT_TYPE_TRACK_START:
				case GPX_STORAGE_POINT_TYPE_TRACK_SEGMENT_START:
				case GPX_STORAGE_POINT_TYPE_TRACK:
					analyzer_view_add_track_point(
							self,
							data->waypoint);
					break;
				default:
					/* Routes are not yet supported */
					break;
			}
			break;
		case GPX_PARSER_DATA_TYPE_HEART_RATE:
			analyzer_view_add_heart_rate(self, data->heart_rate);
			break;
		default:
			break;
	}

	DEBUG_END();
}

static void analyzer_view_add_track(
		AnalyzerView *self,
		GpxParserDataTrack *parser_track)
{
	AnalyzerViewTrack *track = NULL;
	g_return_if_fail(self != NULL);
	g_return_if_fail(parser_track != NULL);
	DEBUG_BEGIN();

	track = g_new0(AnalyzerViewTrack, 1);
	self->tracks = g_slist_prepend(self->tracks, track);

	track->name = g_strdup(parser_track->name);
//	track->comment = g_strdup(parser_track->comment);
	track->number = parser_track->number;

	/* All other data is initialized correctly with g_new0 */

	DEBUG_END();
}

static void analyzer_view_add_track_segment(
		AnalyzerView *self,
		GpxParserDataTrackSegment *parser_track_segment)
{
	AnalyzerViewTrack *track = NULL;
	AnalyzerViewTrackSegment *track_segment = NULL;

	g_return_if_fail(self != NULL);
	g_return_if_fail(self->tracks != NULL);

	/* Get the newest track */
	track = (AnalyzerViewTrack *)self->tracks->data;

	track_segment = g_new0(AnalyzerViewTrackSegment, 1);
	track->track_segments = g_slist_append(track->track_segments,
			track_segment);
}

static void analyzer_view_add_track_point(
		AnalyzerView *self,
		GpxParserDataWaypoint *parser_waypoint)
{
	AnalyzerViewTrack *track = NULL;
	AnalyzerViewTrackSegment *track_segment = NULL;
	AnalyzerViewWaypoint *waypoint = NULL;

	g_return_if_fail(self != NULL);
	g_return_if_fail(parser_waypoint != NULL);
	g_return_if_fail(self->tracks != NULL);
	DEBUG_BEGIN();

	track = (AnalyzerViewTrack *)self->tracks->data;
	if(!track->track_segments)
	{
		g_warning("Track does not have any track segments");
		DEBUG_END();
		return;
	}

	track_segment = (AnalyzerViewTrackSegment *)track->track_segments->data;

	osm_gps_map_draw_gps(OSM_GPS_MAP(self->map),parser_waypoint->latitude,parser_waypoint->longitude,0);
	self->lat = parser_waypoint->latitude;
	self->lon = parser_waypoint->longitude;
	waypoint = g_new0(AnalyzerViewWaypoint, 1);

	/* Copy the data from the parser waypoint to the analyzer view
	 * waypoint */
	waypoint->point_type = parser_waypoint->point_type;
	waypoint->latitude = parser_waypoint->latitude;
	waypoint->longitude = parser_waypoint->longitude;
	waypoint->altitude_is_set = parser_waypoint->altitude_is_set;
	waypoint->altitude = parser_waypoint->altitude;

	memcpy(&waypoint->timestamp, &parser_waypoint->timestamp,
			sizeof(struct timeval));

	track_segment->track_points = g_slist_prepend(
			track_segment->track_points,
			waypoint);

	DEBUG_END();
}

static void analyzer_view_add_heart_rate(
		AnalyzerView *self,
		GpxParserDataHeartRate *parser_heart_rate)
{
	AnalyzerViewTrack *track = NULL;
	AnalyzerViewTrackSegment *track_segment = NULL;
	AnalyzerViewHeartRate *heart_rate = NULL;

	g_return_if_fail(self != NULL);
	g_return_if_fail(parser_heart_rate != NULL);
	g_return_if_fail(self->tracks != NULL);
	DEBUG_BEGIN();

	track = (AnalyzerViewTrack *)self->tracks->data;
	if(!track->track_segments)
	{
		g_warning("Track does not have any track segments");
		DEBUG_END();
		return;
	}

	track = (AnalyzerViewTrack *)self->tracks->data;
	track_segment = (AnalyzerViewTrackSegment *)track->track_segments->data;
	heart_rate = g_new0(AnalyzerViewHeartRate, 1);

	/* Copy the data from the parser heart rate to the analyzer view
	 * heart rate */
	memcpy(&heart_rate->timestamp, &parser_heart_rate->timestamp,
			sizeof(struct timeval));

	heart_rate->value = parser_heart_rate->value;

	track_segment->heart_rates = g_slist_prepend(
			track_segment->heart_rates,
			heart_rate);

	DEBUG_END();

}

static void analyzer_view_analyze_track(
		AnalyzerView *self,
		AnalyzerViewTrack *track)
{
	gboolean times_set = FALSE;
	GSList *seg_temp = NULL;
	AnalyzerViewTrackSegment *track_segment = NULL;
	gdouble secs = 0;

	g_return_if_fail(self != NULL);
	g_return_if_fail(track != NULL);
	DEBUG_BEGIN();

	track->altitude_min = G_MAXDOUBLE;
	track->altitude_max = -G_MAXDOUBLE;

	track->distance = -1;

	for(seg_temp = track->track_segments; seg_temp;
			seg_temp = g_slist_next(seg_temp))
	{
		track_segment = (AnalyzerViewTrackSegment *)seg_temp->data;
		analyzer_view_analyze_track_segment(self, track, track_segment);
		if(track_segment->times_set)
		{
			if(!times_set)
			{
				times_set = TRUE;
				memcpy(&track->start_time,
						&track_segment->start_time,
						sizeof(struct timeval));
				memcpy(&track->end_time,
						&track_segment->end_time,
						sizeof(struct timeval));
			} else {
				if(util_compare_timevals(
						&track_segment->start_time,
						&track->start_time) == -1)
				{
					memcpy(&track->start_time,
						&track_segment->start_time,
						sizeof(struct timeval));
				}
				if(util_compare_timevals(
						&track_segment->end_time,
						&track->end_time) == 1)
				{
					memcpy(&track->end_time,
						&track_segment->start_time,
						sizeof(struct timeval));
				}
			}
		}
	}

	if(track->heart_rate_count > 0)
	{
		track->heart_rate_avg = (gint)((gdouble)track->heart_rate_sum /
				(gdouble)track->heart_rate_count);
	}

	secs = (gdouble)track->duration.tv_sec +
		(gdouble)track->duration.tv_usec / 1000000.0;
	if(secs != 0 && track->distance != -1)
	{
		DEBUG("Secs: %f; distance: %f", secs, track->distance);
		track->speed_avg = track->distance / secs * 3.6;
		
//		DEBUG("KULUNEET SEKUNTIT %d", result.tv_sec);
//		DEBUG("KULUNUT MATKA %f", travelled_distance);
	gdouble mins,seconds;
	gdouble minkm = (secs / (track->distance/1000)/60);
	seconds = modf(minkm,&mins);
	DEBUG("MIN / KM  %02.f:%02.f ",mins,(60*seconds));
	
	track->min_per_km = g_strdup_printf(_("%02.f:%02.f"),mins,(60*seconds));
	}
	
	
	track->data_is_analyzed = TRUE;

	DEBUG_END();
}

static void analyzer_view_analyze_track_segment(
		AnalyzerView *self,
		AnalyzerViewTrack *track,
		AnalyzerViewTrackSegment *track_segment)
{
	AnalyzerViewWaypoint *waypoint = NULL;
	AnalyzerViewWaypoint *prev = NULL;
	gdouble elapsed_temp;
	GSList *temp = NULL;
	GSList *temp2 = NULL;

	AnalyzerViewWaypoint *waypoint_temp = NULL;

	gint i = 0;
	gdouble distance_array[5];
	gdouble time_array[5];
	gint fill_counter;
	gdouble dist_sum;
	gdouble time_sum;
	gdouble speed_temp;

	g_return_if_fail(self != NULL);
	g_return_if_fail(track != NULL);
	g_return_if_fail(track_segment != NULL);

	DEBUG_BEGIN();

	if((track_segment->track_points != NULL) && track->distance == -1)
	{
		/* The track has at least one track point, so set the distance
		 * to 0 instead of undefined (-1) */
		track->distance = 0;
	} else {
		/* Don't analyze the track points, since there aren't
		 * any... */
		temp = NULL;
	}

	prev = NULL;
	/* On the first round, calculate the distance and time to
	 * previous waypoint for each waypoint and get the maximum and
	 * minimum altitude */
	for(temp = track_segment->track_points; temp; temp = g_slist_next(temp))
	{
		waypoint = (AnalyzerViewWaypoint *)temp->data;
		if(prev)
		{
			waypoint->distance_to_prev = location_distance_between(
					waypoint->latitude,
					waypoint->longitude,
					prev->latitude,
					prev->longitude) * 1000.0;
			track->distance += waypoint->distance_to_prev;

			util_subtract_time(
					&waypoint->timestamp,
					&prev->timestamp,
					&waypoint->time_to_prev);
			prev = waypoint;
		} else {
			/* Set the track start and end times to the timestamps
			 * of first waypoint. These are not yet definite, so
			 * though, so do not yet calculate the track segment
			 * duration (or set any information to the track) */
			track_segment->times_set = TRUE;
			memcpy(&track_segment->start_time,
					&waypoint->timestamp,
					sizeof(struct timeval));
			memcpy(&track_segment->end_time,
					&waypoint->timestamp,
					sizeof(struct timeval));
		}

		if(waypoint->altitude_is_set)
		{
			if(!self->metric)
			{
				waypoint->altitude= waypoint->altitude * 3.280;
			}
			if(waypoint->altitude > track->altitude_max)
			{
				track->altitude_max = waypoint->altitude;
				track->altitude_bounds_set = TRUE;
			}
			if(waypoint->altitude < track->altitude_min)
			{
				track->altitude_min = waypoint->altitude;
			}
		}
		prev = waypoint;
	}

	if(waypoint)
	{
		/* If this is a different waypoint than from which the
		 * end time was taken earlier, it should be also later.
		 * If it is the same, it does not really matter even if
		 * it is copied again. */
		memcpy(&track_segment->end_time, &waypoint->timestamp,
				sizeof(struct timeval));
	}

	DEBUG("Total distance so far: %f metres", track->distance);

	/* Analyze the speed for each point and at the same time, derive
	 * the maximum speed. Use slight interpolation here.
	 *
	 * Because of the interpolation and some checks, this
	 * certainly is not the nicest code that I have written.
	 */

	fill_counter = 0;

	prev = NULL;
	for(temp = track_segment->track_points; temp; temp = g_slist_next(temp))
	{
		waypoint = (AnalyzerViewWaypoint *)temp->data;
		if(!prev)
		{
			prev = waypoint;
			continue;
		}
		elapsed_temp = (gdouble)waypoint->time_to_prev.tv_sec +
			(gdouble)waypoint->time_to_prev.tv_usec / 1000000.0;
		if(elapsed_temp != 0)
		{
			if(fill_counter == 5)
			{
				/* Shift the data in the array */
				for(i = 0; i < 4; i++)
				{
					distance_array[i] = distance_array[i+1];
					time_array[i] = time_array[i+1];
				}
				distance_array[4] = waypoint->distance_to_prev;
				time_array[4] = elapsed_temp;
			} else if(fill_counter == 4) {
				/* Use the current speed for the beginning of
				 * the list */
				distance_array[4] = waypoint->distance_to_prev;
				time_array[4] = elapsed_temp;

				time_sum = 0;
				dist_sum = 0;
				for(i = 0; i < 4; i++)
				{
					dist_sum += distance_array[i];
					time_sum += time_array[i];
				}
				if(self->metric)
				{
				speed_temp = dist_sum / time_sum * 3.6;
				}
				else
				{
				speed_temp = dist_sum / time_sum * 3.6;
				speed_temp = speed_temp * 0.621;
				}


				for(temp2 = track_segment->track_points;
				    temp2 != temp;
				    temp2 = g_slist_next(temp2))
				{
					waypoint_temp = (AnalyzerViewWaypoint *)
							temp2->data;
					waypoint_temp->speed_averaged
						= speed_temp;
				}
				fill_counter = 5;
				if(speed_temp > track->speed_max)
				{
					track->speed_max = speed_temp;
				}
			} else {
				distance_array[fill_counter] =
					waypoint->distance_to_prev;
				time_array[fill_counter] = elapsed_temp;
				fill_counter++;
			}
		}
		if(fill_counter == 5)
		{
			time_sum = 0;
			dist_sum = 0;
			for(i = 0; i < 5; i++)
			{
				dist_sum += distance_array[i];
				time_sum += time_array[i];
			}
			if(self->metric)
			{
			speed_temp = dist_sum / time_sum * 3.6;
			}
			else
			{
				speed_temp = dist_sum / time_sum * 3.6;
				speed_temp = speed_temp * 0.621;
			}
				waypoint->speed_averaged = speed_temp;
			if(speed_temp > track->speed_max)
			{
				track->speed_max = speed_temp;
			}
		}
		prev = waypoint;
	}

	/* If the fill_counter is less than five, it means that the speeds
	 * were not yet calculated. Use the achieved speeds for all
	 * waypoints (except if there were none, in which case, leave
	 * the speed to zero) */
	if((fill_counter < 5) && (fill_counter > 0))
	{
		time_sum = 0;
		dist_sum = 0;
		for(i = 0; i < fill_counter; i++)
		{
			dist_sum += distance_array[i];
			time_sum += time_array[i];
		}
		if(self->metric)
		{
		speed_temp = dist_sum / time_sum * 3.6;
		}
		else
		{
			speed_temp = dist_sum / time_sum * 3.6;
			speed_temp = speed_temp * 0.621;
		}

		for(temp2 = track_segment->track_points;
			temp2;
			temp2 = g_slist_next(temp2))
		{
			waypoint_temp =
				(AnalyzerViewWaypoint *)
				temp2->data;
			waypoint_temp->speed_averaged =
				speed_temp;
		}
		if(speed_temp > track->speed_max)
		{
			track->speed_max = speed_temp;
		}
	}

	analyzer_view_analyze_track_segment_heart_rates(
			self,
			track,
			track_segment);

	if(track_segment->times_set)
	{
		/* Calculate the track segment duration and add it to the
		 * track duration */
		util_subtract_time(&track_segment->end_time,
				&track_segment->start_time,
				&track_segment->duration);

		util_add_time(&track->duration, &track_segment->duration,
				&track->duration);
	}

	DEBUG_END();
}

static void analyzer_view_analyze_track_segment_heart_rates(
		AnalyzerView *self,
		AnalyzerViewTrack *track,
		AnalyzerViewTrackSegment *track_segment)
{
	GSList *temp = NULL;
	AnalyzerViewHeartRate *heart_rate = NULL;
	gboolean first = TRUE;

	g_return_if_fail(self != NULL);
	g_return_if_fail(track != NULL);
	g_return_if_fail(track_segment != NULL);
	DEBUG_BEGIN();

	if(!track_segment->heart_rates)
	{
		DEBUG("Empty heart rate list");
		DEBUG_END();
		return;
	}

	track->heart_rate_min = G_MAXINT;
	track->heart_rate_max = G_MININT;

	for(temp = track_segment->heart_rates; temp; temp = g_slist_next(temp))
	{
		heart_rate = (AnalyzerViewHeartRate *)temp->data;

		if(first)
		{
			first = FALSE;
			track->heart_rate_bounds_set = TRUE;
			track->heart_rate_max = heart_rate->value;
			track->heart_rate_min = heart_rate->value;
			if(!track_segment->times_set)
			{
				track_segment->times_set = TRUE;
				memcpy(&track_segment->start_time,
						&heart_rate->timestamp,
						sizeof(struct timeval));
				memcpy(&track_segment->end_time,
						&heart_rate->timestamp,
						sizeof(struct timeval));
			} else if(util_compare_timevals(&heart_rate->timestamp,
						&track_segment->start_time)
					== -1)
			{
				/* This is earlier than the track start.
				 * Set it to this */
				track_segment->times_set = TRUE;
				memcpy(&track_segment->start_time,
						&heart_rate->timestamp,
						sizeof(struct timeval));
			}
		}

		if(heart_rate->value > track->heart_rate_max)
		{
			track->heart_rate_max = heart_rate->value;
		} else if(heart_rate->value < track->heart_rate_min) {
			track->heart_rate_min = heart_rate->value;
		}

		track->heart_rate_sum += heart_rate->value;
		track->heart_rate_count++;
	}

	if(util_compare_timevals(&heart_rate->timestamp,
				&track_segment->start_time) == 1)
	{
		/* This is later than the track segment end.
		 * Set it to this */
		memcpy(&track_segment->end_time, &heart_rate->timestamp,
				sizeof(struct timeval));
	}

	DEBUG_END();
}

static void analyzer_view_show_track_information(
		AnalyzerView *self,
		AnalyzerViewTrack *track)
{
	time_t time_src;
	struct tm time_dest;
	struct tm time_dest_2;
	gchar *buffer = NULL;
	gchar *buffer2 = NULL;
	gchar *buffer_times = NULL;
	guint temp;
	gboolean has_name = FALSE;
	gboolean has_comment = FALSE;
	gdouble temp_average;
	gdouble temp_distance;


	g_return_if_fail(self != NULL);
	g_return_if_fail(track != NULL);

	DEBUG_BEGIN();

	buffer = g_strdup_printf(_("Track %d"),
			track->number);
	gtk_label_set_text(GTK_LABEL(self->lbl_track_number), buffer);
	g_free(buffer);

	if(!track->data_is_analyzed)
	{
		analyzer_view_analyze_track(self, track);
	}

	if((track->name != NULL) && (strcmp(track->name, "") != 0))
	{
		has_name = TRUE;
		gtk_label_set_text(GTK_LABEL(self->info_labels
				[ANALYZER_VIEW_INFO_LABEL_NAME][1]),
				track->name);
	gtk_window_set_title ( GTK_WINDOW (self->map_win), track->name);	
	} else {
		gtk_label_set_text(GTK_LABEL(self->info_labels
				[ANALYZER_VIEW_INFO_LABEL_NAME][1]),
				_("N/A"));
	}
/*
	if((track->comment != NULL) && (strcmp(track->comment, "") != 0))
	{
		has_comment = TRUE;
		gtk_label_set_text(GTK_LABEL(self->info_labels
				[ANALYZER_VIEW_INFO_LABEL_COMMENT][1]),
				track->comment);
	} else {
		gtk_label_set_text(GTK_LABEL(self->info_labels
				[ANALYZER_VIEW_INFO_LABEL_COMMENT][1]),
				_("N/A"));
	}
*/

	if(has_name && has_comment)
	{
		buffer2 = g_strdup_printf(_("%s (%s)"),
				track->name,
				track->comment);
	} else if(has_name) {
		buffer2 = g_strdup(track->name);
	} else if(has_comment) {
		buffer2 = g_strdup(track->comment);
	} else {
		buffer2 = g_strdup("Untitled track");
	}

	if((track->start_time.tv_sec != 0) && (track->end_time.tv_sec != 0))
	{
		gmtime_r(&track->start_time.tv_sec, &time_dest);
		gmtime_r(&track->end_time.tv_sec, &time_dest_2);
		buffer_times = g_strdup_printf(
				_("%04d-%02d-%02d %02d:%02d - %02d:%02d"),
				time_dest.tm_year + 1900,
				time_dest.tm_mon + 1,
				time_dest.tm_mday,
				time_dest.tm_hour,
				time_dest.tm_min,
				time_dest_2.tm_hour,
				time_dest_2.tm_min);

		buffer = g_strdup_printf("%s\n%s",
				buffer2, buffer_times);
		g_free(buffer2);
		g_free(buffer_times);
	} else {
		buffer = buffer2;
	}

	gtk_label_set_text(GTK_LABEL(self->lbl_track_details), buffer);

	g_free(buffer);

	if(track->start_time.tv_sec != 0)
	{
		time_src = track->start_time.tv_sec;
		gmtime_r(&time_src, &time_dest);
		buffer = g_strdup_printf(
				_("%04d-%02d-%02d %02d:%02d"),
				time_dest.tm_year + 1900,
				time_dest.tm_mon + 1,
				time_dest.tm_mday,
				time_dest.tm_hour,
				time_dest.tm_min);
		gtk_label_set_text(GTK_LABEL(self->info_labels
				[ANALYZER_VIEW_INFO_LABEL_TIME_START][1]),
				buffer);
		g_free(buffer);
	} else {
		gtk_label_set_text(GTK_LABEL(self->info_labels
				[ANALYZER_VIEW_INFO_LABEL_TIME_START][1]),
				_("N/A"));
	}

	if(track->end_time.tv_sec != 0)
	{
		time_src = track->end_time.tv_sec;
		gmtime_r(&time_src, &time_dest);
		buffer = g_strdup_printf(
				_("%04d-%02d-%02d %02d:%02d"),
				time_dest.tm_year + 1900,
				time_dest.tm_mon + 1,
				time_dest.tm_mday,
				time_dest.tm_hour,
				time_dest.tm_min);
	} else {
		buffer = g_strdup("N/A");
	}
	gtk_label_set_text(GTK_LABEL(self->info_labels
				[ANALYZER_VIEW_INFO_LABEL_TIME_END][1]),
			buffer);
	g_free(buffer);

	if((track->duration.tv_sec != 0) || (track->duration.tv_usec != 0))
	{
		DEBUG("%d seconds",(gint) track->duration.tv_sec);
		time_src = track->duration.tv_sec;
		temp = track->duration.tv_usec / 10000;
		gmtime_r(&time_src, &time_dest);
		buffer = g_strdup_printf(
				_("%02d:%02d:%02d.%02d"),
				time_dest.tm_hour,
				time_dest.tm_min,
				time_dest.tm_sec,
				temp);
		gtk_label_set_text(GTK_LABEL(self->info_labels
				[ANALYZER_VIEW_INFO_LABEL_DURATION][1]),
				buffer);
		g_free(buffer);
	} else {
		gtk_label_set_text(GTK_LABEL(self->info_labels
				[ANALYZER_VIEW_INFO_LABEL_DURATION][1]),
				_("N/A"));
	}

	if(track->distance >= 10000.0)
	{

		if(self->metric)
		{
			buffer = g_strdup_printf(
					_("%.1f km"),
					  track->distance / 1000.0);
		}
		else
		{

			buffer = g_strdup_printf(
					_("%.1f mi"),
					  (track->distance / 1000.0)*0.621);
		}

	} else if(track->distance >= 1000.0) {

		if(self->metric)
		{
		buffer = g_strdup_printf(_("%.2f km"),
				track->distance / 1000.0);
		}
		else
		{
		buffer = g_strdup_printf(_("%.2f mi"),
					 (track->distance / 1000.0)*0.621);
		}

	} else if(track->distance >= 0) {

		if(self->metric)
		{
		buffer = g_strdup_printf(_("%d m"), (gint)track->distance);
		}
		else
		{
		temp_distance = track->distance * 3.280;
		buffer = g_strdup_printf(_("%.0f ft"), temp_distance);
		}

	} else {
		buffer = g_strdup_printf(_("N/A"));
	}

	gtk_label_set_text(GTK_LABEL(self->info_labels
				[ANALYZER_VIEW_INFO_LABEL_DISTANCE][1]),
			buffer);
	g_free(buffer);

	if(track->speed_avg > 0.0)
	{
		if(self->metric)
		{
		buffer = g_strdup_printf("%.1f km/h", track->speed_avg);
		}
		else
		{
		temp_average= track->speed_avg * 0.621;
		buffer = g_strdup_printf("%.1f mph", temp_average);
		}
	} else {
		buffer = g_strdup("N/A");
	}
	gtk_label_set_text(GTK_LABEL(self->info_labels
				[ANALYZER_VIEW_INFO_LABEL_SPEED_AVG][1]),
			buffer);
	g_free(buffer);

	
	if(track->min_per_km != NULL){
	  
	  gtk_label_set_text(GTK_LABEL(self->info_labels
				[ANALYZER_VIEW_INFO_LABEL_MIN_PER_KM][1]),
			track->min_per_km);
	}
	else
	{
	    gtk_label_set_text(GTK_LABEL(self->info_labels
				[ANALYZER_VIEW_INFO_LABEL_MIN_PER_KM][1]),
			"N/A");
	}
	
	
	if(track->speed_max > 0.0)
	{
		if(self->metric)
		{
		buffer = g_strdup_printf("%.1f km/h", track->speed_max);
		}
		else
		{
		buffer = g_strdup_printf("%.1f mph", track->speed_max);
		}
	} else {
		buffer = g_strdup("N/A");
	}
	gtk_label_set_text(GTK_LABEL(self->info_labels
				[ANALYZER_VIEW_INFO_LABEL_SPEED_MAX][1]),
			buffer);
	g_free(buffer);

	if(track->heart_rate_bounds_set)
	{
		buffer = g_strdup_printf(_("%d bpm"), track->heart_rate_avg);
	} else {
		buffer = g_strdup(_("N/A"));
	}
	gtk_label_set_text(GTK_LABEL(self->info_labels
				[ANALYZER_VIEW_INFO_LABEL_HEART_RATE_AVG][1]),
			buffer);
	g_free(buffer);

	if(track->heart_rate_bounds_set)
	{
		buffer = g_strdup_printf(_("%d bpm"), track->heart_rate_max);
	} else {
		buffer = g_strdup(_("N/A"));
	}
	gtk_label_set_text(GTK_LABEL(self->info_labels
				[ANALYZER_VIEW_INFO_LABEL_HEART_RATE_MAX][1]),
			buffer);
	g_free(buffer);

	self->graphs_update_data = TRUE;
	if(self->current_view == ANALYZER_VIEW_GRAPHS)
	{
		gtk_widget_queue_draw(self->graphs_drawing_area);
	}

	DEBUG_END();
}

static void analyzer_view_graphs_btn_speed_clicked(
		GtkWidget *button,
		gpointer user_data)
{
	AnalyzerView *self = (AnalyzerView *)user_data;
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	analyzer_view_graphs_toggle_button(self, button, &self->show_speed);

	DEBUG_END();
}

static void analyzer_view_graphs_btn_altitude_clicked(
		GtkWidget *button,
		gpointer user_data)
{
	AnalyzerView *self = (AnalyzerView *)user_data;
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	analyzer_view_graphs_toggle_button(self, button, &self->show_altitude);

	DEBUG_END();
}

static void analyzer_view_graphs_btn_heart_rate_clicked(
		GtkWidget *button,
		gpointer user_data)
{
	AnalyzerView *self = (AnalyzerView *)user_data;
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	analyzer_view_graphs_toggle_button(self, button,
			&self->show_heart_rate);

	DEBUG_END();
}

static void analyzer_view_graphs_toggle_button(
		AnalyzerView *self,
		GtkWidget *button,
		gboolean *state)
{
	gint graph_count = 0;

	g_return_if_fail(self != NULL);
	g_return_if_fail(button != NULL);
	g_return_if_fail(state != NULL);

	DEBUG_BEGIN();

	if(*state)
	{
		*state = FALSE;
		ec_button_set_bg_image(EC_BUTTON(button),
				EC_BUTTON_STATE_RELEASED,
				GFXDIR "ec_button_small_bg.png");
	} else {
		if(self->show_altitude)
		{
			graph_count++;
		}
		if(self->show_speed)
		{
			graph_count++;
		}
		if(self->show_heart_rate)
		{
			graph_count++;
		}
		*state = TRUE;
		ec_button_set_bg_image(EC_BUTTON(button),
				EC_BUTTON_STATE_RELEASED,
				GFXDIR "ec_button_small_bg_down.png");
	}

	self->graphs_update_data = TRUE;
	gtk_widget_queue_draw(self->graphs_drawing_area);

	DEBUG_END();
}

static gboolean analyzer_view_graphs_drawing_area_exposed(
		GtkWidget *widget,
		GdkEventExpose *event,
		gpointer user_data)
{
	AnalyzerViewTrack *track = NULL;

	AnalyzerView *self = (AnalyzerView *)user_data;

	GdkGC *gc = NULL;
	GdkDrawable *drawable = NULL;
	GdkRectangle *rects = NULL;

	gint n_rects = 0;
	gint i = 0;

	AnalyzerViewPixbufDetails details;

	g_return_val_if_fail(self != NULL, FALSE);

	if(!self->tracks)
	{
		return FALSE;
	}
	track = (AnalyzerViewTrack *)
		g_slist_nth_data(self->tracks, self->current_track_number);

	if(!self->graphs_pixbuf || self->graphs_update_data)
	{
		self->graphs_update_data = FALSE;
		if(self->graphs_pixbuf)
		{
			g_object_unref(G_OBJECT(self->graphs_pixbuf));
			self->graphs_pixbuf = NULL;
		}
		details.w = widget->allocation.width;
		details.h = widget->allocation.height;

		details.track = track;

		details.show_speed = self->show_speed;

		details.show_altitude = (self->show_altitude &&
				track->altitude_bounds_set);

		details.show_heart_rate = (self->show_heart_rate &&
				track->heart_rate_bounds_set);

		details.color_speed.r = 0;
		details.color_speed.g = .6;
		details.color_speed.b = 1;
		details.color_speed.a = 1;

		details.color_altitude.r = .4;
		details.color_altitude.g = .9;
		details.color_altitude.b = 0;
		details.color_altitude.a = 1;

		details.color_heart_rate.r = 1;
		details.color_heart_rate.g = .2;
		details.color_heart_rate.b = .2;
		details.color_heart_rate.a = 1;

		details.scale_count = 5;

		self->graphs_pixbuf = analyzer_view_create_pixbuf(
				self,
				&details);
	}

	drawable = GDK_DRAWABLE(widget->window);
	gc = gdk_gc_new(drawable);

	gdk_region_get_rectangles(event->region, &rects, &n_rects);

	for(i = 0; i < n_rects; i++)
	{
		gdk_draw_pixbuf(drawable,
				gc,
				self->graphs_pixbuf,
				rects[i].x, rects[i].y,
				rects[i].x, rects[i].y,
				rects[i].width,
				rects[i].height,
				GDK_RGB_DITHER_NONE,
				0, 0);
	}

	g_free(rects);
	gdk_gc_unref(gc);

	return FALSE;
}

static GdkPixbuf *analyzer_view_create_pixbuf(
		AnalyzerView *self,
		AnalyzerViewPixbufDetails *details)
{
	/* Cairo and Gdk have the colors in different order. What a big
	 * surprise. Work around it switching the blue and red during
	 * drawing so that no switch operations need to be done. */

	GdkPixbuf *pixbuf = NULL;

	cairo_text_extents_t extents;
	cairo_text_extents_t extents2;
	cairo_surface_t *surface = NULL;
	cairo_t *cr = NULL;

	guchar *data = NULL;

	gint i, j;
	gint scale_height;
	gint scale_width;
	gint time_scale_height;

	GdkRectangle graph;

	gchar *scale_text;
	gboolean draw_scale_to_left = TRUE;

	gdouble speed_pixels_per_unit = 0;
	gdouble altitude_pixels_per_unit = 0;
	gdouble heart_rate_pixels_per_unit = 0;

	gdouble descr_x = 0;

	AnalyzerViewUnitType unit_type;

	g_return_val_if_fail(details != NULL, NULL);

	DEBUG_BEGIN();

	surface = cairo_image_surface_create(
			CAIRO_FORMAT_ARGB32,
			details->w,
			details->h);

	cr = cairo_create(surface);

	/* Do the actual drawing */
	graph.x = 0;
	graph.width = details->w;
	graph.y = 0;
	graph.height = details->h;

	/* Calculate the height for the time scale */
	cairo_select_font_face(cr, "Nokia Sans",
			CAIRO_FONT_SLANT_NORMAL,
			CAIRO_FONT_WEIGHT_NORMAL);

	cairo_set_font_size(cr, 20.0);

	/* The scale extent calculations go as follows:
	 *
	 * The time scale height only depends on the height of the text,
	 * which consists only of the characters "0123456789:" (or other
	 * characters, depending on the locale, but anyhow, a small, defined
	 * set of characters).
	 *
	 * The height of the speed/altitude/etc depends on  the height of
	 * the time scale.
	 *
	 * Finally, the width of the time scale depends on the width of the
	 * speed/altitude/etc scales.
	 */
	cairo_set_source_rgb(cr, 1, 1, 1);

	scale_text = _("0123456789:");
	cairo_text_extents(cr, scale_text, &extents);
	time_scale_height = extents.height;
	graph.height -= time_scale_height + 5;

	/* First, draw the titles of the scales */
	if(self->metric)
	{
	scale_text = _("Speed (km/h) Altitude (m) Heart rate (bpm)"
		       "Time (hh:mm) (mm:ss)");
	}
	else
	{
	scale_text = _("Speed (mph) Altitude (ft) Heart rate (bpm)"
			"Time (hh:mm) (mm:ss)");
	}

	cairo_text_extents(cr, scale_text, &extents);

	graph.height -= extents.height + 15;
	graph.y += extents.height + 15;

	if(details->track->speed_max == 0)
	{
		details->show_speed = FALSE;
	}

	if(!details->track->altitude_bounds_set)
	{
		details->show_speed = FALSE;
	}

	if(details->track->heart_rate_max == 0)
	{
		details->show_heart_rate = FALSE;
	}

	if(details->show_speed)
	{
		if(self->metric)
		{
		scale_text = _("Speed (km/h)");
		}
		else
		{
		scale_text = _("Speed (mph)");
		}
		cairo_text_extents(cr, scale_text, &extents2);

		cairo_set_source_rgba(cr,
				details->color_speed.b,
				details->color_speed.g,
				details->color_speed.r,
				details->color_speed.a);

		cairo_arc(cr, extents.height / 2.0, extents.height / 2.0,
				extents.height / 2.0,
				0, 2 * M_PI);
		cairo_fill(cr);

		cairo_set_source_rgba(cr, 1, 1, 1, 1);
		cairo_move_to(cr, extents.height + 5,
				extents.height / 2.0 - extents.y_bearing / 2.0);
		cairo_show_text(cr, scale_text);
		descr_x = extents.height + extents2.width + 20;
	}

	if(details->show_altitude)
	{
		if(self->metric)
		{
		scale_text = _("Altitude (m)");
		}
		else
		{
		scale_text = _("Altitude (ft)");
		}
		cairo_text_extents(cr, scale_text, &extents2);

		cairo_set_source_rgba(cr,
				details->color_altitude.b,
				details->color_altitude.g,
				details->color_altitude.r,
				details->color_altitude.a);

		cairo_arc(cr, descr_x + extents.height / 2.0,
				extents.height / 2.0,
				extents.height / 2.0,
				0, 2 * M_PI);
		cairo_fill(cr);

		cairo_set_source_rgba(cr, 1, 1, 1, 1);
		cairo_move_to(cr, descr_x + extents.height + 5,
				extents.height / 2.0 - extents.y_bearing / 2.0);
		cairo_show_text(cr, scale_text);
		descr_x += extents.height + extents2.width + 20;
	}

	if(details->show_heart_rate)
	{
		scale_text = _("Heart rate (bpm)");
		cairo_text_extents(cr, scale_text, &extents2);

		cairo_set_source_rgba(cr,
				details->color_heart_rate.b,
				details->color_heart_rate.g,
				details->color_heart_rate.r,
				details->color_heart_rate.a);

		cairo_arc(cr, descr_x + extents.height / 2.0,
				extents.height / 2.0,
				extents.height / 2.0,
				0, 2 * M_PI);
		cairo_fill(cr);

		cairo_set_source_rgba(cr, 1, 1, 1, 1);

		cairo_move_to(cr, descr_x + extents.height + 5,
				extents.height / 2.0 - extents.y_bearing / 2.0);
		cairo_show_text(cr, scale_text);
		descr_x += extents.height + extents2.width + 20;
	}

	scale_height = graph.height / ((gdouble)details->scale_count);

	/* Draw the unit scales and at the same time, keep record on the
	 * width of the graph area */
	if(details->show_speed)
	{
		scale_width = analyzer_view_create_scale(
				self,
				cr,
				details,
				&details->color_speed,
				draw_scale_to_left,
				0, details->track->speed_max,
				scale_height,
				&graph,
				&speed_pixels_per_unit);

		if(scale_width > 0)
		{
			if(draw_scale_to_left)
			{
				graph.x += scale_width;
				draw_scale_to_left = FALSE;
			}
			graph.width -= scale_width;
		} else {
			details->show_speed = FALSE;
		}
	}

	if(details->show_altitude)
	{
		scale_width = analyzer_view_create_scale(
				self,
				cr,
				details,
				&details->color_altitude,
				draw_scale_to_left,
				details->track->altitude_min,
				details->track->altitude_max,
				scale_height,
				&graph,
				&altitude_pixels_per_unit);

		if(scale_width > 0)
		{
			if(draw_scale_to_left)
			{
				graph.x += scale_width;
				draw_scale_to_left = FALSE;
			}
			graph.width -= scale_width;
		} else {
			details->show_altitude = FALSE;
		}
	}

	if(details->show_heart_rate)
	{
		scale_width = analyzer_view_create_scale(
				self,
				cr,
				details,
				&details->color_heart_rate,
				draw_scale_to_left,
				details->track->heart_rate_min,
				details->track->heart_rate_max,
				scale_height,
				&graph,
				&heart_rate_pixels_per_unit);

		if(scale_width > 0)
		{
			if(draw_scale_to_left)
			{
				graph.x += scale_width;
				draw_scale_to_left = FALSE;
			}
			graph.width -= scale_width;
		} else {
			details->show_heart_rate = FALSE;
		}
	}

	/* Draw the horizontal lines */
	cairo_translate(cr, graph.x, graph.y);

	j = graph.height - 1;
	for(i = 0; i <= details->scale_count; i++)
	{
		cairo_move_to(cr, 0, j);
		cairo_line_to(cr, graph.width, j);
		j -= scale_height;
		cairo_stroke(cr);
	}
	cairo_move_to(cr, 0, 0);
	cairo_line_to(cr, 0, graph.height);

	cairo_move_to(cr, graph.width, 0);
	cairo_line_to(cr, graph.width, graph.height);

	cairo_stroke(cr);

	cairo_translate(cr, -graph.x, -graph.y);

	graph.x++;
	graph.width -= 2;

	/* Draw the time scale */
	unit_type = analyzer_view_create_scale_time(self, cr,
			&details->track->duration,
			&graph);

	scale_text = "";
	switch(unit_type)
	{
		case ANALYZER_VIEW_UNIT_TYPE_MINS_SECS:
			scale_text = _("Time (mm:ss)");
			break;
		case ANALYZER_VIEW_UNIT_TYPE_HOURS_MINS:
			scale_text = _("Time (hh:mm)");
			break;
		case ANALYZER_VIEW_UNIT_TYPE_NONE:
			g_warning("Time scale creation failed");
			break;
		default:
			g_warning("Unknown unit type: %d", unit_type);
			break;
	}

	cairo_text_extents(cr, scale_text, &extents2);

	cairo_set_source_rgba(cr, 1, 1, 1, 1);

	cairo_move_to(cr, descr_x, extents.height / 2.0);
	cairo_line_to(cr, descr_x + extents.height, extents.height / 2.0);

	cairo_stroke(cr);

	cairo_move_to(cr, descr_x + extents.height + 5,
			extents.height / 2.0 - extents.y_bearing / 2.0 + 1.0);
	cairo_show_text(cr, scale_text);

	/* Limit the graphs to the graph area (they might exceed it by
	 * a couple of pixels because of the line width etc)*/
	cairo_rectangle(cr, graph.x, graph.y, graph.width, graph.height);
	cairo_clip(cr);


	if(details->show_speed)
	{
		analyzer_view_draw_speed(
				self,
				cr,
				details,
				&graph,
				speed_pixels_per_unit);
	}

	if(details->show_altitude)
	{
		analyzer_view_draw_altitude(
				self,
				cr,
				details,
				&graph,
				altitude_pixels_per_unit);
	}

	if(details->show_heart_rate)
	{
		analyzer_view_draw_heart_rate(
				self,
				cr,
				details,
				&graph,
				heart_rate_pixels_per_unit);
	}

	/* Make a pixbuf from the data */
	data = cairo_image_surface_get_data(surface);
	pixbuf = gdk_pixbuf_new_from_data(
			data,
			GDK_COLORSPACE_RGB,
			TRUE,
			8,
			details->w,
			details->h,
			cairo_image_surface_get_stride(surface),
			analyzer_view_destroy_pixbuf_data,
			cr);

	DEBUG_END();
	return pixbuf;
}

static void analyzer_view_destroy_pixbuf_data(
		guchar *pixels,
		gpointer user_data)
{
	cairo_surface_t *surface = NULL;
	cairo_t *cr = (cairo_t *)user_data;

	g_return_if_fail(pixels != NULL);
	DEBUG_BEGIN();

	surface = cairo_get_target(cr);
	cairo_surface_destroy(surface);
	cairo_destroy(cr);

	DEBUG_END();
}

static gint analyzer_view_create_scale(
		AnalyzerView *self,
		cairo_t *cr,
		AnalyzerViewPixbufDetails *details,
		AnalyzerViewColor *color,
		gboolean left_edge,
		gdouble value_min,
		gdouble value_max,
		gint scale_height,
		GdkRectangle *graph,
		gdouble *pixels_per_unit)
{
	gint units_per_scale = 0;
	gchar *scale_text = NULL;
	gint i = 0;
	gint scale_width = 0;
	cairo_text_extents_t extents;
	gdouble y = 0;
	gdouble x = 0;

	g_return_val_if_fail(self != NULL, -1);
	g_return_val_if_fail(cr != NULL, -1);
	g_return_val_if_fail(details != NULL, -1);
	g_return_val_if_fail(color != NULL, -1);
	g_return_val_if_fail(pixels_per_unit != NULL, -1);

	DEBUG_BEGIN();

	units_per_scale = (value_max - value_min)
		/ details->scale_count + 1;
	*pixels_per_unit = (gdouble)scale_height / (gdouble)units_per_scale;

	cairo_save(cr);

	cairo_set_source_rgba(cr, color->b, color->g, color->r, color->a);

	/* First, calculate the maximum scale size */
	for(i = 0; i <= details->scale_count; i++)
	{
		scale_text = g_strdup_printf("%d",
				i * units_per_scale +
				       (gint)value_min);

		cairo_text_extents(cr, scale_text, &extents);
		if(extents.width > scale_width)
		{
			scale_width = extents.width;
		}

		g_free(scale_text);
	}

	if(!left_edge)
	{
		cairo_translate(cr, graph->width + graph->x - scale_width,
				0);
	}

	/* Then, draw the scale texts */
	for(i = 0; i <= details->scale_count; i++)
	{
		scale_text = g_strdup_printf("%d",
				i * units_per_scale +
				(gint)value_min);

		cairo_text_extents(cr, scale_text, &extents);

		y = graph->y + graph->height - ((gdouble) i *
				(gdouble)scale_height +
				(extents.height / 2.0) + extents.y_bearing);
		if(left_edge)
		{
			x = scale_width - extents.width;
		}

		cairo_move_to(cr, x, y);
		cairo_show_text(cr, scale_text);

		g_free(scale_text);
	}

	cairo_restore(cr);

	DEBUG_END();
	return scale_width + 5;
}

static AnalyzerViewUnitType analyzer_view_create_scale_time(
		AnalyzerView *self,
		cairo_t *cr,
		struct timeval *duration,
		GdkRectangle *graph_area)
{
	gdouble dur_seconds = 0;

	gchar *units_1 = NULL;
	gchar *time_separator = NULL;
	gchar *units_2 = NULL;

	gchar *buffer = NULL;

	gdouble secs_per_segment_min = 0;
	gboolean ready = FALSE;
	gint secs_per_segment = 0;
	gint i = 0;

	AnalyzerViewUnitType unit_type = ANALYZER_VIEW_UNIT_TYPE_MINS_SECS;

	gint seconds_counter;

	cairo_text_extents_t extents;

	gdouble text_x, text_y;

	g_return_val_if_fail(self != NULL, ANALYZER_VIEW_UNIT_TYPE_NONE);
	g_return_val_if_fail(cr != NULL, ANALYZER_VIEW_UNIT_TYPE_NONE);
	g_return_val_if_fail(duration != NULL, ANALYZER_VIEW_UNIT_TYPE_NONE);
	g_return_val_if_fail(graph_area != NULL, ANALYZER_VIEW_UNIT_TYPE_NONE);

	DEBUG_BEGIN();

	dur_seconds = (gdouble)duration->tv_sec +
		(gdouble)duration->tv_usec / 1000000.0;
	gdouble marker_pos = 0;

	secs_per_segment_min = dur_seconds / 6.0;

	DEBUG("Seconds per segment: %.1f", secs_per_segment_min);

	/* Draw the scale so that five time markers are shown. Zero and the
	 * last time are not shown so that graph does not need to be scaled
	 * down because of the time markers.
	 *
	 * The segment duration can only be rounded down; otherwise they would
	 * not fit correctly. The segment duration only affects the width
	 * of each segment, not the width of the graph.
	 */

	secs_per_segment = 1;

	do {
		if(secs_per_segment > secs_per_segment_min)
		{
			ready = TRUE;
		} else {
			if(secs_per_segment < 15)
			{
				secs_per_segment += 1;
			} else if(secs_per_segment < 60) {
				/* 15, 30, 60 seconds */
				secs_per_segment = secs_per_segment * 2;
			} else if(secs_per_segment < 5 * 60) {
				/* 1, 2, 3, 4, 5 minutes */
				secs_per_segment += 60;
			} else if(secs_per_segment < 30 * 60) {
				/* 10, 15, ..., 30 minutes */
				secs_per_segment += 5 * 60;
			} else if(secs_per_segment < 2 * 60 * 60) {
				/* 30, 40, ..., 1 h 50 minutes, 2 h */
				secs_per_segment += 10 * 60;
			} else if(secs_per_segment < 5 * 60 * 60) {
				/* 2.5 h, 3 h, ..., 5 h */
				secs_per_segment += 30 * 60;
			} else {
				/* 5 h and up */
				secs_per_segment += 60 * 60;
			}
		}
	} while(!ready);
	if(secs_per_segment >= 60 * 60)
	{
		unit_type = ANALYZER_VIEW_UNIT_TYPE_HOURS_MINS;
	}
	DEBUG("Secs per segment: %d", secs_per_segment);

	time_separator = _(":");
	seconds_counter = 0;

	for(i = 1; i <= 5; i++)
	{
		/* First, convert to either minutes and seconds or
		 * hours and minutes */
		seconds_counter += secs_per_segment;

		if(seconds_counter > dur_seconds)
		{
			/* Stop drawing the markers */
			i = 5;
		}

		/* Then, draw the text so that the time separator is where
		 * the marker is
		 */
		if(i < 5)
		{
			marker_pos = (gdouble)seconds_counter / dur_seconds *
				(gdouble)graph_area->width + graph_area->x;
			cairo_move_to(cr, marker_pos, graph_area->y);
			cairo_line_to(cr, marker_pos,
					graph_area->y + graph_area->height);
		} else {
			/* Don't draw the last marker, instead,
			 * draw the duration text */
			seconds_counter = dur_seconds;
		}

		if(unit_type == ANALYZER_VIEW_UNIT_TYPE_MINS_SECS)
		{
			units_1 = g_strdup_printf("%02d", seconds_counter / 60);
			units_2 = g_strdup_printf("%02d", seconds_counter % 60);
		} else {
			units_1 = g_strdup_printf("%02d",
					seconds_counter / 3600);
			units_2 = g_strdup_printf("%02d",
					(seconds_counter % 3600) / 60);
		}

		/**
		 * @todo Draw the text so that the text is centered
		 * according to time separator, not the whole text
		 */

		buffer = g_strdup_printf("%s%s%s",
				units_1, time_separator, units_2);

		cairo_text_extents(cr, buffer, &extents);

		if(i < 5)
		{

			text_x = marker_pos - extents.width / 2.0;
		} else {
			text_x = graph_area->x + graph_area->width -
				extents.width;
		}

		text_y = graph_area->y + graph_area->height -
				extents.y_bearing + 5;

		cairo_move_to(cr, text_x, text_y);
		cairo_show_text(cr, buffer);

		g_free(buffer);
		g_free(units_1);
		g_free(units_2);
	}

	cairo_stroke(cr);

	DEBUG_END();
	return unit_type;
}

static void analyzer_view_draw_speed(
		AnalyzerView *self,
		cairo_t *cr,
		AnalyzerViewPixbufDetails *details,
		GdkRectangle *graph_area,
		gdouble pixels_per_unit)
{
	/* Pixels per units ("how many pixels wide is a second") */
	gdouble pixels_per_sec = 0;

	gdouble mile_speed = 0;

	gdouble duration_secs = 0;
	guint counter = 0;

	gdouble x = 0;
	gdouble y = 0;

	GSList *temp = NULL;
	GSList *temp2 = NULL;

	AnalyzerViewTrack *track = NULL;
	AnalyzerViewTrackSegment *track_segment = NULL;
	AnalyzerViewWaypoint *waypoint = NULL;

	g_return_if_fail(self != NULL);
	g_return_if_fail(cr != NULL);
	g_return_if_fail(details != NULL);
	g_return_if_fail(graph_area != NULL);
	DEBUG_BEGIN();

	track = details->track;

	duration_secs = (gdouble)track->duration.tv_sec +
		(gdouble)track->duration.tv_usec / 1000000.0;

	if(duration_secs == 0)
	{
		DEBUG_END();
		return;
	}
	pixels_per_sec = (gdouble)graph_area->width / (gdouble)duration_secs;

	/* Start drawing */

	cairo_save(cr);
	cairo_translate(cr, graph_area->x, graph_area->y);

	cairo_set_source_rgba(cr,
			details->color_speed.b,
			details->color_speed.g,
			details->color_speed.r,
			details->color_speed.a);

	cairo_set_line_width(cr, 3.0);

	for(temp = track->track_segments; temp; temp = g_slist_next(temp))
	{
		track_segment = (AnalyzerViewTrackSegment *)temp->data;
		for(temp2 = track_segment->track_points; temp2;
				temp2 = g_slist_next(temp2))
		{
			waypoint = (AnalyzerViewWaypoint *)temp2->data;
			if(self->metric)
			{
			y = graph_area->height - waypoint->speed_averaged *
				pixels_per_unit;
			}
			else
			{
				mile_speed  = 	waypoint->speed_averaged;
			y = graph_area->height - mile_speed *
				pixels_per_unit;

			}

			if(counter == 0)
			{
				cairo_move_to(cr, x, y);
			} else {
				x += pixels_per_sec *
				((gdouble)waypoint->time_to_prev.tv_sec +
				 (gdouble)waypoint->time_to_prev.tv_usec /
					 1000000.0);
				cairo_line_to(cr, x, y);
			}
			counter++;
		}
	}
	cairo_stroke(cr);
	cairo_restore(cr);

	DEBUG_END();
}

static void analyzer_view_draw_altitude(
		AnalyzerView *self,
		cairo_t *cr,
		AnalyzerViewPixbufDetails *details,
		GdkRectangle *graph_area,
		gdouble pixels_per_unit)
{
	/* Pixels per units ("how many pixels wide is a second") */
	gdouble pixels_per_sec = 0;

	gdouble duration_secs = 0;
	gboolean first_point = TRUE;
	gboolean draw_line = FALSE;

	gdouble x = 0;
	gdouble y = 0;

	GSList *temp = NULL;
	GSList *temp2 = NULL;

	AnalyzerViewTrack *track = NULL;
	AnalyzerViewTrackSegment *track_segment = NULL;
	AnalyzerViewWaypoint *waypoint = NULL;

	g_return_if_fail(self != NULL);
	g_return_if_fail(cr != NULL);
	g_return_if_fail(details != NULL);
	g_return_if_fail(graph_area != NULL);
	DEBUG_BEGIN();

	track = details->track;

	duration_secs = (gdouble)track->duration.tv_sec +
		(gdouble)track->duration.tv_usec / 1000000.0;

	if(duration_secs == 0)
	{
		DEBUG_END();
		return;
	}
	pixels_per_sec = (gdouble)graph_area->width / (gdouble)duration_secs;

	/* Start drawing */

	cairo_save(cr);
	cairo_translate(cr, graph_area->x, graph_area->y);

	cairo_set_source_rgba(cr,
			details->color_altitude.b,
			details->color_altitude.g,
			details->color_altitude.r,
			details->color_altitude.a);

	cairo_set_line_width(cr, 3.0);

	for(temp = track->track_segments; temp; temp = g_slist_next(temp))
	{
		track_segment = (AnalyzerViewTrackSegment *)temp->data;
		for(temp2 = track_segment->track_points; temp2;
				temp2 = g_slist_next(temp2))
		{
			waypoint = (AnalyzerViewWaypoint *)temp2->data;

			if(!first_point)
			{
				x += pixels_per_sec *
				((gdouble)waypoint->time_to_prev.tv_sec +
				 (gdouble)waypoint->time_to_prev.tv_usec /
					 1000000.0);
			}

			if(!waypoint->altitude_is_set)
			{
				draw_line = FALSE;
				continue;
			}

			y = graph_area->height -
				(waypoint->altitude - track->altitude_min) *
				pixels_per_unit;

			if(!draw_line)
			{
				cairo_move_to(cr, x, y);
			} else {
				cairo_line_to(cr, x, y);
			}
			first_point = FALSE;
			draw_line = TRUE;
		}
	}
	cairo_stroke(cr);
	cairo_restore(cr);

	DEBUG_END();
}

static void analyzer_view_draw_heart_rate(
		AnalyzerView *self,
		cairo_t *cr,
		AnalyzerViewPixbufDetails *details,
		GdkRectangle *graph_area,
		gdouble pixels_per_unit)
{
	/* Pixels per units ("how many pixels wide is one second") */
	gdouble pixels_per_sec = 0;

	gdouble hr_secs = 0;
	gdouble start_secs = 0;

	gdouble duration_secs = 0;
	gboolean first_point = TRUE;

	gdouble x = 0;
	gdouble y = 0;

	GSList *temp = NULL;
	GSList *temp2 = NULL;

	AnalyzerViewTrack *track = NULL;
	AnalyzerViewTrackSegment *track_segment = NULL;
	AnalyzerViewHeartRate *heart_rate = NULL;

	g_return_if_fail(self != NULL);
	g_return_if_fail(cr != NULL);
	g_return_if_fail(details != NULL);
	g_return_if_fail(graph_area != NULL);
	DEBUG_BEGIN();

	track = details->track;

	start_secs = (gdouble)track->start_time.tv_sec +
		(gdouble)track->duration.tv_usec / 1000000.0;

	duration_secs = (gdouble)track->duration.tv_sec +
		(gdouble)track->duration.tv_usec / 1000000.0;

	if(duration_secs == 0)
	{
		DEBUG_END();
		return;
	}
	pixels_per_sec = (gdouble)graph_area->width / (gdouble)duration_secs;

	/* Start drawing */

	cairo_save(cr);
	cairo_translate(cr, graph_area->x, graph_area->y);

	cairo_set_source_rgba(cr,
			details->color_heart_rate.b,
			details->color_heart_rate.g,
			details->color_heart_rate.r,
			details->color_heart_rate.a);

	cairo_set_line_width(cr, 3.0);

	for(temp = track->track_segments; temp; temp = g_slist_next(temp))
	{
		track_segment = (AnalyzerViewTrackSegment *)temp->data;
		for(temp2 = track_segment->heart_rates; temp2;
				temp2 = g_slist_next(temp2))
		{
			heart_rate = (AnalyzerViewHeartRate *)temp2->data;

			hr_secs = (gdouble)heart_rate->timestamp.tv_sec +
				(gdouble)heart_rate->timestamp.tv_usec /
				1000000.0;

			x = pixels_per_sec * (hr_secs - start_secs);

			y = graph_area->height -
				(heart_rate->value - track->heart_rate_min) *
				pixels_per_unit;

			if(first_point)
			{
				cairo_move_to(cr, 0, y);
			}
			cairo_line_to(cr, x, y);
			first_point = FALSE;
		}
	}

	cairo_stroke(cr);
	cairo_restore(cr);

	DEBUG_END();

}

static void analyzer_view_track_destroy(AnalyzerViewTrack *track)
{
	AnalyzerViewTrackSegment *track_segment = NULL;
	AnalyzerViewWaypoint *waypoint = NULL;
	AnalyzerViewHeartRate *heart_rate = NULL;

	GSList *temp = NULL;
	GSList *temp2 = NULL;

	g_return_if_fail(track != NULL);
	DEBUG_BEGIN();

	g_free(track->name);
	g_free(track->min_per_km);
//	g_free(track->comment);

	for(temp = track->track_segments; temp; temp = g_slist_next(temp))
	{
		track_segment = (AnalyzerViewTrackSegment *)temp->data;
		for(temp2 = track_segment->track_points; temp2;
				temp2 = g_slist_next(temp2))
		{
			waypoint = (AnalyzerViewWaypoint *)temp2->data;
			g_free(waypoint);
		}
		g_slist_free(track_segment->track_points);

		for(temp2 = track_segment->heart_rates; temp2;
				temp2 = g_slist_next(temp2))
		{
			heart_rate = (AnalyzerViewHeartRate *)temp2->data;
			g_free(heart_rate);
		}
		g_slist_free(track_segment->heart_rates);
		g_free(track_segment);
	}

	g_slist_free(track->track_segments);

	g_free(track);

	DEBUG_END();
}
static void analyzer_view_set_units(AnalyzerView *self)
{
	GtkWidget *dialog;
	gint result = 0;
	gboolean loop = TRUE;
	dialog = gtk_dialog_new_with_buttons("Choose units",
					     self->parent_window,
	  				     GTK_DIALOG_MODAL,"Metric",
					     METRIC,
					     "English",
	  				     ENGLISH,NULL);


	do{

	result = gtk_dialog_run (GTK_DIALOG (dialog));

		switch (result)
		{
			case METRIC:
			self->metric = TRUE;
			loop = FALSE;
				break;
			case ENGLISH:
			self->metric = FALSE;
			loop = FALSE;
				break;
		}
	}while(loop);
	gtk_widget_destroy (dialog);

}
