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

#ifndef _ACTIVITY_H
#define _ACTIVITY_H

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
#include "map_view.h"

/*****************************************************************************
 * Data structures                                                           *
 *****************************************************************************/

typedef struct _ActivityDescription {
	gchar *activity_name;
	gchar *activity_comment;
	gchar *file_name;
	gint heart_rate_range_id;
	gint heart_rate_limit_low;
	gint heart_rate_limit_high;
	gboolean add_calendar;
} ActivityDescription;

typedef struct _ActivityChooserDialog {
	GtkWidget *dialog;

	GtkWidget *button;
	GtkWidget *selector;
	GtkWidget *sportselector;
	GtkWidget *sportbutton;
//	GtkWidget *cmb_pulse_ranges;
	GtkWidget *entry_activity_name;
	GtkWidget *entry_activity_comment;
	GtkWidget *chk_save_dfl;
	GtkWidget *chk_add_calendar;
	GtkSizeGroup *size_group;
} ActivityChooserDialog;

/**
 * @brief ActivityChooser helps to start a new activity with desired
 * parameters such as heart rate range and the name of the activity.
 */
typedef struct _ActivityChooser {
	/** @brief Pointer to #GConfHelperData */
	GConfHelperData *gconf_helper;

	/** @brief Pointer to #HeartRateSettings */
	HeartRateSettings *heart_rate_settings;

	/** @brief Pointer to #MapView */
	MapView *map_view;

	/** @brief Parent window for modal dialogs */
	GtkWindow *parent_window;

	/** @brief Default activity name (from GConf) */
	gchar *default_activity_name;

	/** @brief Default folder name */
	gchar *default_folder_name;

	/** @brief Default heart rate range id (from GConf) */
	gint default_heart_rate_range_id;
} ActivityChooser;

/*****************************************************************************
 * Function prototypes                                                       *
 *****************************************************************************/

/*===========================================================================*
 * ActivityDescription                                                       *
 *===========================================================================*/

/**
 * @brief Create a new activity description
 *
 * @param activity_name Name of the activity, or NULL
 *
 * @return Pointer to a newly allocated ActivityDescription. Free with
 * activity_description_free()
 */
ActivityDescription *activity_description_new(const gchar *activity_name);

void activity_description_free(ActivityDescription *self);

/*===========================================================================*
 * ActivityChooser                                                           *
 *===========================================================================*/

/**
 * @brief Create a new activity chooser
 *
 * @param gconf_helper Pointer to #GConfHelperData
 * @param heart_rate_settings Pointer to #HeartRateSettings
 * @param map_view Pointer to #MapView
 * @param parent_window Parent window
 *
 * @return Pointer to a newly allocated ActivityChooser
 */
ActivityChooser *activity_chooser_new(
		GConfHelperData *gconf_helper,
		HeartRateSettings *heart_rate_settings,
		MapView *map_view,
		GtkWindow *parenti_window);

/**
 * @brief Set the default folder for file operations
 *
 * @param self Pointer to #ActivityChooser
 * @param folder Path of the new folder
 */
void activity_chooser_set_default_folder(
		ActivityChooser *self,
		const gchar *folder);

/**
 * @brief Choose an activity using a dialog
 *
 * @param self Pointer to #ActivityChooser
 *
 * @return Newly allocated ActivityDescription if user clicks on OK,
 * or NULL if user clicks on Cancel. Free with %activity_description_free()
 */
ActivityDescription *activity_chooser_choose_activity(ActivityChooser *self);

/**
 * @brief Select an activity using activity_chooser_choose_activity, and
 * start it to MapView
 *
 * @param self Pointer to #ActivityChooser
 *
 * @return TRUE on success, FALSE on failure (failure might occur when
 * user clicks on Cancel, or when MapView can't be set up, for example)
 */
gboolean activity_chooser_choose_and_setup_activity(ActivityChooser *self);

#endif /* _ACTIVITY_H */
