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

#ifndef _TARGET_HEART_RATE_H
#define _TARGET_HEART_RATE_H

/*****************************************************************************
 * Includes                                                                  *
 *****************************************************************************/

/* Configuration */
#include "config.h"

/* Gtk */
#include <gtk/gtk.h>

/* Other modules */
#include "gconf_helper.h"
#include "heart_rate_settings.h"
#include "navigation_menu.h"

/*****************************************************************************
 * Enumerations                                                              *
 *****************************************************************************/

typedef enum _TargetHeartRateWizardPages {
	THR_WIZARD_PAGE_WELCOME = 0,
	THR_WIZARD_PAGE_REST_MAX = 1,
	THR_WIZARD_PAGE_EXPLANATION = 2,
	THR_WIZARD_PAGE_EXERCISE_FIRST = 3,
	THR_WIZARD_PAGE_EXERCISE_LAST = THR_WIZARD_PAGE_EXERCISE_FIRST +
		EC_EXERCISE_TYPE_COUNT - 1,
	THR_WIZARD_PAGE_FINISH,
	THR_WIZARD_PAGE_COUNT
} TargetHeartRateWizardPages;

/*****************************************************************************
 * Data structures                                                           *
 *****************************************************************************/

typedef struct _HeartRateControl {
	GtkWidget *caption;
	GtkWidget *spin_button;
	GtkWidget *lbl_units;
	GtkWidget *hbox;
} HeartRateControl;

typedef struct _HeartRateRangePage {
	HeartRateSettings *heart_rate_settings;
	EcExerciseDescription *exercise_description;
	GtkWidget *vbox;
	GtkWidget *vbox2;
	GtkWidget *hbox;
	GtkWidget *btn_calculate;
	GtkWidget *title;
	GtkWidget *explanation;
	GtkWidget *h_separator;
	HeartRateControl *hrc[2];
	gint page_id;
} HeartRateRangePage;

typedef struct _TargetHeartRate {
	GConfHelperData *gconf_helper;
	HeartRateSettings *heart_rate_settings;
	GtkWindow *parent_window;

	GtkWidget *wizard;
	GtkWidget *notebook;

	gint page_ids[THR_WIZARD_PAGE_COUNT];

	/* The size group is shared between the pages, so switching pages
	 * looks nicer */
	GtkSizeGroup *size_group;

	/* Welcome page */
	GtkWidget *lbl_welcome;

	/* Resting and maximum heart rate page */
	GtkWidget *vbox_rest_max_hr;
	GtkWidget *lbl_rest_max_hr;

	HeartRateControl *hrc_max;
	HeartRateControl *hrc_rest;

	/* Explanation page for the heart rate ranges */
	GtkWidget *lbl_ranges_explanation;

	/* Pages for different exercise types */
	HeartRateRangePage *hr_range_page[EC_EXERCISE_TYPE_COUNT];

	/* Finish page */
	GtkWidget *lbl_finished;
} TargetHeartRate;

/*****************************************************************************
 * Function prototypes                                                       *
 *****************************************************************************/

void target_heart_rate_dialog_show(
		NavigationMenu *menu,
		GtkTreePath *path,
		gpointer user_data);

#endif /* _TARGET_HEART_RATE_H */
