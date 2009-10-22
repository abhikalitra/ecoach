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
#ifndef _ANALYZER_VIEW_H
#define _ANALYZER_VIEW_H

/*****************************************************************************
 * Includes                                                                  *
 *****************************************************************************/

/* Configuration */
#include "config.h"

/* Gtk */
#include <gtk/gtk.h>

/* Other modules */
#include "gpx_parser.h"
#include "gconf_helper.h"

/*****************************************************************************
 * Enumerations                                                              *
 *****************************************************************************/

typedef enum _AnalyzerViewType {
	ANALYZER_VIEW_INFO,
	ANALYZER_VIEW_GRAPHS,
	ANALYZER_VIEW_COUNT
} AnalyzerViewType;

typedef enum _AnalyzerViewInfoLabel {
	ANALYZER_VIEW_INFO_LABEL_NAME,
	ANALYZER_VIEW_INFO_LABEL_TIME_START,
	ANALYZER_VIEW_INFO_LABEL_TIME_END,
	ANALYZER_VIEW_INFO_LABEL_DURATION,
	ANALYZER_VIEW_INFO_LABEL_DISTANCE,
	ANALYZER_VIEW_INFO_LABEL_SPEED_AVG,
	ANALYZER_VIEW_INFO_LABEL_SPEED_MAX,
	ANALYZER_VIEW_INFO_LABEL_MIN_PER_KM,
	ANALYZER_VIEW_INFO_LABEL_HEART_RATE_AVG,
	ANALYZER_VIEW_INFO_LABEL_HEART_RATE_MAX,
	ANALYZER_VIEW_INFO_LABEL_COUNT
} AnalyzerViewInfoLabel;

/*****************************************************************************
 * Data structures                                                           *
 *****************************************************************************/

typedef struct _AnalyzerView {
	/* Following members are initialized with #analyzer_view_new() */
	GtkWidget *main_widget;

	GtkWindow *parent_window;

	GConfHelperData *gconf_helper;

	GtkWidget *btn_back;
  
	GtkWidget *map_win;
	gchar *default_folder_name;

	/* Following members are created when the view is shown and destroyed
	 * when the view is hidden */

	GtkWidget *win;
	GtkWidget *vbox;
	GtkWidget *main_table;
	GtkWidget *btn_map;
	GtkWidget *btn_open;
	GtkWidget *btn_track_prev;
	GtkWidget *btn_track_next;
	GtkWidget *lbl_track_number;
	GtkWidget *lbl_track_details;

	GtkWidget *scrolled;
	GtkWidget *pannable;
	/* Container for the different views */
	GtkWidget *fixed_view;

	GtkWidget *views[ANALYZER_VIEW_COUNT];
	gint current_view;
	gboolean is_scrolling;

	GtkWidget *btn_view_prev;
	GtkWidget *btn_view_next;
	GtkWidget *lbl_view_name;

	GtkWidget *pan;
	GtkWidget *data;
	
	/* Data for info view */
	GtkWidget *info_table;

	/*	Each statistics has the description and the comment */
	GtkWidget *info_labels[ANALYZER_VIEW_INFO_LABEL_COUNT][2];

	/* Data for graph view */
	gboolean graphs_update_data;

	GdkPixbuf *graphs_pixbuf;

	GtkWidget *graphs_table;

	GtkWidget *graphs_btn_speed;
	GtkWidget *graphs_btn_altitude;
	GtkWidget *graphs_btn_heart_rate;
	GtkWidget *graphs_drawing_area;

	gboolean show_speed;
	gboolean show_altitude;
	gboolean show_heart_rate;

	gboolean metric;
	
	 /* Activity state */
	gint activity_state;
	
	/*map */
	const char *friendly_name;
	char *cachedir;
	gboolean fullpath;
	gint map_provider;
	GtkWidget *map;
	gdouble lat,lon;
	GdkPixbuf *zoom_in;
	GdkPixbuf *zoom_out;
	/* Data that is parsed from tracks */

	/**
	 * @brief A list of pointers of type AnalyzerViewTrack (defined
	 * in the source file as it is not needed here)
	 */
	GSList *tracks;

	gint current_track_number;
} AnalyzerView;

typedef struct _AnalyzerViewScrollData {
	AnalyzerView *analyzer_view;
	gint start_x;
	gint end_x;
	gint current_step;
	GtkWidget *start_widget;
	GtkWidget *end_widget;
} AnalyzerViewScrollData;

/*****************************************************************************
 * Function prototypes                                                       *
 *****************************************************************************/

AnalyzerView *analyzer_view_new(
		GtkWindow *parent_window,
		GConfHelperData *gconf_helper);

void analyzer_view_set_default_folder(
		AnalyzerView *self,
		const gchar *folder);

void analyzer_view_show(AnalyzerView *self);

void analyzer_view_hide(AnalyzerView *self);

#endif /* _ANALYZER_VIEW_H */
