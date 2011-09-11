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

/*****************************************************************************
 * Includes                                                                  *
 *****************************************************************************/

/* This module */
#include "target_heart_rate.h"

/* Hildon */
#include <hildon/hildon-caption.h>
#include <hildon/hildon-note.h>
#include <hildon/hildon-wizard-dialog.h>

/* Other modules */
#include "interface.h"

/* i18n */
#include <glib/gi18n.h>

#include "debug.h"


/*****************************************************************************
 * Private function prototypes                                               *
 *****************************************************************************/

/*===========================================================================*
 * HeartRateControl functions                                                *
 *===========================================================================*/

static HeartRateControl *heart_rate_control_new(
		const gchar *label,
		guint start_value,
		GtkSizeGroup *size_group);

/*===========================================================================*
 * HeartRateRangePage functions                                              *
 *===========================================================================*/

static HeartRateRangePage *heart_rate_range_page_new(
		HeartRateSettings *heart_rate_settings,
		EcExerciseDescription *exercise_description,
		GtkSizeGroup *size_group,
		GtkNotebook *notebook);

static void heart_rate_range_page_btn_calculate_clicked(
		GtkWidget *button,
		gpointer user_data);

static void heart_rate_range_page_recalculate(HeartRateRangePage *self);

static void heart_rate_range_page_spin_low_changed(
		GtkSpinButton *btn,
		gpointer user_data);

static void heart_rate_range_page_spin_high_changed(
		GtkSpinButton *btn,
		gpointer user_data);

/*===========================================================================*
 * TargetHeartRate functions                                                 *
 *===========================================================================*/

static void target_heart_rate_create_wizard(TargetHeartRate *self);

static void target_heart_rate_spin_rest_changed(
		GtkSpinButton *btn,
		gpointer user_data);

static void target_heart_rate_spin_max_changed(
		GtkSpinButton *btn,
		gpointer user_data);

static void target_heart_rate_wizard_page_changed(
		GtkNotebook *notebook,
		GtkNotebookPage *page,
		guint page_num,
		gpointer user_data);

static void target_heart_rate_revert_settings(TargetHeartRate *self);
static void target_heart_rate_save_settings(TargetHeartRate *self);

/*****************************************************************************
 * Function declarations                                                     *
 *****************************************************************************/

/*===========================================================================*
 * HeartRateControl functions                                                *
 *===========================================================================*/

/*---------------------------------------------------------------------------*
 * Private functions                                                         *
 *---------------------------------------------------------------------------*/

static HeartRateControl *heart_rate_control_new(
		const gchar *label,
		guint start_value,
		GtkSizeGroup *size_group)
{
	HeartRateControl *self = NULL;

	g_return_val_if_fail(label != NULL, NULL);
	g_return_val_if_fail(size_group != NULL, NULL);
	DEBUG_BEGIN();

	self = g_new0(HeartRateControl, 1);

	self->hbox = gtk_hbox_new(FALSE, 5);

	self->spin_button = gtk_spin_button_new_with_range(0, 400, 1);

	/* Set a fixed size for the spin button so that it doees not
	 * resize when maximum value for it is less than 100
	 */
	gtk_widget_set_size_request(self->spin_button, 100, -1);

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(self->spin_button),
			start_value);

	gtk_box_pack_start(GTK_BOX(self->hbox), self->spin_button,
			FALSE, FALSE, 0);

	self->lbl_units = gtk_label_new(_("bpm"));

	gtk_box_pack_start(GTK_BOX(self->hbox), self->lbl_units,
			FALSE, FALSE, 0);

	self->caption = hildon_caption_new(
			size_group,
			label,
			self->hbox,
			NULL,
			HILDON_CAPTION_OPTIONAL);

	DEBUG_END();
	return self;
}

/*===========================================================================*
 * HeartRateRangePage functions                                              *
 *===========================================================================*/

/*---------------------------------------------------------------------------*
 * Private functions                                                         *
 *---------------------------------------------------------------------------*/

static HeartRateRangePage *heart_rate_range_page_new(
		HeartRateSettings *heart_rate_settings,
		EcExerciseDescription *exercise_description,
		GtkSizeGroup *size_group,
		GtkNotebook *notebook)
{
	HeartRateRangePage *self = NULL;

	g_return_val_if_fail(heart_rate_settings != NULL, NULL);
	g_return_val_if_fail(exercise_description != NULL, NULL);
	g_return_val_if_fail(size_group != NULL, NULL);
	DEBUG_BEGIN();

	self = g_new0(HeartRateRangePage, 1);

	self->heart_rate_settings = heart_rate_settings;
	self->exercise_description = exercise_description;
	self->vbox = gtk_vbox_new(FALSE, 0);

	self->title = gtk_label_new(self->exercise_description->name);
	gtk_misc_set_alignment(GTK_MISC(self->title), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(self->vbox), self->title, FALSE, FALSE, 0);

	self->h_separator = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(self->vbox), self->h_separator,
			FALSE, FALSE, 0);

	self->hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(self->vbox), self->hbox, FALSE, FALSE, 0);

	self->vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(self->hbox), self->vbox2, FALSE, FALSE, 0);
	self->hrc[0] = heart_rate_control_new(
			_("Lower limit"),
			self->exercise_description->low,
			size_group);

	g_signal_connect(G_OBJECT(self->hrc[0]->spin_button),
			"value-changed",
			G_CALLBACK(heart_rate_range_page_spin_low_changed),
			self);

	gtk_box_pack_start(GTK_BOX(self->vbox2), self->hrc[0]->caption,
			FALSE, FALSE, 0);

	self->hrc[1] = heart_rate_control_new(
			_("Upper limit"),
			self->exercise_description->high,
			size_group);

	g_signal_connect(G_OBJECT(self->hrc[1]->spin_button),
			"value-changed",
			G_CALLBACK(heart_rate_range_page_spin_high_changed),
			self);

	/* Set the ranges for the spin buttons */
	heart_rate_range_page_spin_low_changed(NULL, self);
	heart_rate_range_page_spin_high_changed(NULL, self);

	gtk_box_pack_start(GTK_BOX(self->vbox2), self->hrc[1]->caption,
			FALSE, FALSE, 0);

	self->btn_calculate = gtk_button_new_with_label(_("Calculate"));
	gtk_box_pack_start(GTK_BOX(self->hbox), self->btn_calculate,
			FALSE, FALSE, 0);

	g_signal_connect(G_OBJECT(self->btn_calculate), "clicked",
			G_CALLBACK(heart_rate_range_page_btn_calculate_clicked),
			self);

	self->explanation = gtk_label_new(self->exercise_description->
			description);
	gtk_label_set_line_wrap(GTK_LABEL(self->explanation), TRUE);
	gtk_misc_set_alignment(GTK_MISC(self->explanation), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(self->vbox), self->explanation,
			TRUE, TRUE, 10);

	self->page_id = gtk_notebook_append_page(notebook, self->vbox,
			gtk_label_new(self->exercise_description->name));

	DEBUG_END();
	return self;
}

static void heart_rate_range_page_btn_calculate_clicked(
		GtkWidget *button,
		gpointer user_data)
{
	HeartRateRangePage *self = (HeartRateRangePage *)user_data;

	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	heart_rate_range_page_recalculate(self);

	DEBUG_END();
}

static void heart_rate_range_page_recalculate(HeartRateRangePage *self)
{
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	heart_rate_settings_recalculate(self->heart_rate_settings,
			self->exercise_description);

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(self->hrc[0]->spin_button),
			self->exercise_description->low);

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(self->hrc[1]->spin_button),
			self->exercise_description->high);

	DEBUG_END();
}

static void heart_rate_range_page_spin_low_changed(
		GtkSpinButton *btn,
		gpointer user_data)
{
	gdouble min, max;
	gint value;
	HeartRateRangePage *self = (HeartRateRangePage *)user_data;

	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	/* Ensure that user does not set higher upper limit than lower limit
	 * is */
	value = gtk_spin_button_get_value_as_int(
			GTK_SPIN_BUTTON(self->hrc[0]->spin_button));

	gtk_spin_button_get_range(
			GTK_SPIN_BUTTON(self->hrc[1]->spin_button),
			&min,
			&max);

	gtk_spin_button_set_range(
			GTK_SPIN_BUTTON(self->hrc[1]->spin_button),
			value,
			max);

	DEBUG_END();
}

static void heart_rate_range_page_spin_high_changed(
		GtkSpinButton *btn,
		gpointer user_data)
{
	gdouble min, max;
	gint value;
	HeartRateRangePage *self = (HeartRateRangePage *)user_data;

	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	/* Ensure that user does not set higher upper limit than lower limit
	 * is */
	value = gtk_spin_button_get_value_as_int(
			GTK_SPIN_BUTTON(self->hrc[1]->spin_button));

	gtk_spin_button_get_range(
			GTK_SPIN_BUTTON(self->hrc[0]->spin_button),
			&min,
			&max);

	gtk_spin_button_set_range(
			GTK_SPIN_BUTTON(self->hrc[0]->spin_button),
			min,
			value);

	DEBUG_END();
}

/*===========================================================================*
 * TargetHeartRate functions                                                 *
 *===========================================================================*/

/*---------------------------------------------------------------------------*
 * Public functions                                                          *
 *---------------------------------------------------------------------------*/

void target_heart_rate_dialog_show(
		NavigationMenu *menu,
		GtkTreePath *path,
		gpointer user_data)
{
	AppData *app_data = (AppData *)user_data;
	TargetHeartRate *self = NULL;
	GtkWidget *confirmation_dialog;

	gint result_wizard;
	gboolean close_dialog;

	g_return_if_fail(app_data != NULL);
	DEBUG_BEGIN();

	self = g_new0(TargetHeartRate, 1);
	self->gconf_helper = app_data->gconf_helper;
	self->heart_rate_settings = app_data->heart_rate_settings;
	self->parent_window = GTK_WINDOW(app_data->window);

	target_heart_rate_create_wizard(self);

	gtk_widget_show_all(self->wizard);

	do {
		close_dialog = FALSE;
		result_wizard = gtk_dialog_run(GTK_DIALOG(self->wizard));
		if(result_wizard == HILDON_WIZARD_DIALOG_CANCEL)
		{
			confirmation_dialog =
				hildon_note_new_confirmation_add_buttons(
					self->parent_window,
					_("Are you sure that you want to cancel the wizard?"),
					GTK_STOCK_YES,
					GTK_RESPONSE_YES,
					GTK_STOCK_NO,
					GTK_RESPONSE_NO,
					NULL);

			if(gtk_dialog_run(GTK_DIALOG(confirmation_dialog)) ==
					GTK_RESPONSE_YES)
			{
				close_dialog = TRUE;
			}
			gtk_widget_destroy(confirmation_dialog);
		} else {
			close_dialog = TRUE;
		}
	} while(!close_dialog);

	if(result_wizard == HILDON_WIZARD_DIALOG_CANCEL)
	{
		target_heart_rate_revert_settings(self);
	} else {
		target_heart_rate_save_settings(self);
	}

	gtk_widget_destroy(self->wizard);

	DEBUG_END();
}

/*---------------------------------------------------------------------------*
 * Private functions                                                         *
 *---------------------------------------------------------------------------*/

static void target_heart_rate_create_wizard(TargetHeartRate *self)
{
	gint i;
	EcExerciseDescription *exercise_descriptions;

	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	self->notebook = gtk_notebook_new();

	/* Welcome page */
	self->lbl_welcome = gtk_label_new(
			_("Welcome to the target heart rate setup wizard. "
				"This wizard helps you to set target heart "
				"rates for different types of exercise.")
			);
	gtk_label_set_line_wrap(GTK_LABEL(self->lbl_welcome), TRUE);

	self->page_ids[THR_WIZARD_PAGE_WELCOME] =
		gtk_notebook_append_page(GTK_NOTEBOOK(self->notebook),
			self->lbl_welcome, NULL);

	/* Resting and maximum heart rate page */
	self->vbox_rest_max_hr = gtk_vbox_new(FALSE, 10);

	self->lbl_rest_max_hr = gtk_label_new(
			_("The resting heart rate and maximum heart rate "
				"are used for calculating heart rate ranges "
				"for different exercise types. You can "
				"determine these by using the Performance "
				"analysis menu."));

	gtk_misc_set_alignment(GTK_MISC(self->lbl_rest_max_hr), 0, 0.5);
	gtk_label_set_line_wrap(GTK_LABEL(self->lbl_rest_max_hr), TRUE);

	gtk_box_pack_start(GTK_BOX(self->vbox_rest_max_hr),
			self->lbl_rest_max_hr,
			FALSE, TRUE, 0);

	self->size_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	self->hrc_rest = heart_rate_control_new(
			_("Resting heart rate"),
			self->heart_rate_settings->hr_rest,
			self->size_group);

	gtk_box_pack_start(GTK_BOX(self->vbox_rest_max_hr),
			self->hrc_rest->caption,
			FALSE, FALSE, 0);

	g_signal_connect(G_OBJECT(self->hrc_rest->spin_button),
			"value-changed",
			G_CALLBACK(target_heart_rate_spin_rest_changed),
			self);

	self->hrc_max = heart_rate_control_new(
			_("Maximum heart rate"),
			self->heart_rate_settings->hr_max,
			self->size_group);

	gtk_box_pack_start(GTK_BOX(self->vbox_rest_max_hr),
			self->hrc_max->caption,
			FALSE, FALSE, 0);

	g_signal_connect(G_OBJECT(self->hrc_max->spin_button),
			"value-changed",
			G_CALLBACK(target_heart_rate_spin_max_changed),
			self);

	self->page_ids[THR_WIZARD_PAGE_REST_MAX] =
		gtk_notebook_append_page(GTK_NOTEBOOK(self->notebook),
			self->vbox_rest_max_hr,
			gtk_label_new(_("Start values")));

	/* Explanation page for the heart rate ranges */
	self->lbl_ranges_explanation = gtk_label_new(
			_("Next, you are asked to fill in different heart "
				"rate ranges. If previously defined values "
				"are found, they are used as default. "
				"Otherwise, the values will be calculated "
				"based on the resting and maximum heart rates."
				"\n\n"
				"You can recalculate the values by pressing "
				"on the Calculate button on each page."
			 ));

	gtk_label_set_line_wrap(GTK_LABEL(self->lbl_ranges_explanation), TRUE);
	gtk_misc_set_alignment(GTK_MISC(self->lbl_ranges_explanation), 0, 0.5);

	self->page_ids[THR_WIZARD_PAGE_EXPLANATION] =
		gtk_notebook_append_page(GTK_NOTEBOOK(self->notebook),
			self->lbl_ranges_explanation,
			gtk_label_new(_("Information")));

	/* Pages for different exercise types */
	exercise_descriptions = self->heart_rate_settings->
		exercise_descriptions;

	for(i = 0; i < EC_EXERCISE_TYPE_COUNT; i++)
	{
		self->hr_range_page[i] = heart_rate_range_page_new(
				self->heart_rate_settings,
				&exercise_descriptions[i],
				self->size_group,
				GTK_NOTEBOOK(self->notebook));
		self->page_ids[THR_WIZARD_PAGE_EXERCISE_FIRST + i]
			= self->hr_range_page[i]->page_id;
	}

	/* Finish page */
	self->lbl_finished = gtk_label_new(
			_("The heart rates for all different exercise types "
				"have now been set. Please click Finish to "
				"save the values."));
	gtk_label_set_line_wrap(GTK_LABEL(self->lbl_finished), TRUE);
	self->page_ids[THR_WIZARD_PAGE_FINISH] =
		gtk_notebook_append_page(GTK_NOTEBOOK(self->notebook),
			self->lbl_finished,
			gtk_label_new(_("Finished")));

	self->wizard = hildon_wizard_dialog_new(
			self->parent_window,
			_("Heart rate ranges"),
			GTK_NOTEBOOK(self->notebook));

	/* Connect the page change signal */
	g_signal_connect(G_OBJECT(self->notebook),
			"switch-page",
			G_CALLBACK(target_heart_rate_wizard_page_changed),
			self);

	DEBUG_END();
}

static void target_heart_rate_spin_rest_changed(
		GtkSpinButton *btn,
		gpointer user_data)
{
	gdouble min, max;
	TargetHeartRate *self = (TargetHeartRate *)user_data;

	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	self->heart_rate_settings->hr_rest = gtk_spin_button_get_value_as_int(
			GTK_SPIN_BUTTON(self->hrc_rest->spin_button));

	/* Maximum heart rate cannot be smaller than resting heart rate */
	gtk_spin_button_get_range(
			GTK_SPIN_BUTTON(self->hrc_max->spin_button),
			&min,
			&max);

	gtk_spin_button_set_range(
			GTK_SPIN_BUTTON(self->hrc_max->spin_button),
			self->heart_rate_settings->hr_rest,
			max);

	DEBUG_END();
}

static void target_heart_rate_spin_max_changed(
		GtkSpinButton *btn,
		gpointer user_data)
{
	gdouble min, max;
	TargetHeartRate *self = (TargetHeartRate *)user_data;

	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	/* Maximum heart rate cannot be smaller than resting heart rate */
	self->heart_rate_settings->hr_max = gtk_spin_button_get_value_as_int(
			GTK_SPIN_BUTTON(self->hrc_max->spin_button));

	gtk_spin_button_get_range(
			GTK_SPIN_BUTTON(self->hrc_rest->spin_button),
			&min,
			&max);

	gtk_spin_button_set_range(
			GTK_SPIN_BUTTON(self->hrc_rest->spin_button),
			min,
			self->heart_rate_settings->hr_max);

	DEBUG_END();
}

static void target_heart_rate_wizard_page_changed(
		GtkNotebook *notebook,
		GtkNotebookPage *page,
		guint page_num,
		gpointer user_data)
{
	gint i;
	HeartRateRangePage *range_page = NULL;
	TargetHeartRate *self = (TargetHeartRate *)user_data;

	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	for(i = 0; i < EC_EXERCISE_TYPE_COUNT; i++)
	{
		if(self->hr_range_page[i]->page_id == page_num)
		{
			range_page = self->hr_range_page[i];
			break;
		}
	}

	if(!range_page)
	{
		/* Not entering an exercise page */
		DEBUG_END();
		return;
	}

	if(range_page->exercise_description->user_set)
	{
		/* User has set the values.
		 * Do not change them automatically. */
		DEBUG_END();
		return;
	}

	heart_rate_range_page_recalculate(range_page);

	DEBUG_END();
}

static void target_heart_rate_revert_settings(TargetHeartRate *self)
{
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	heart_rate_settings_revert(self->heart_rate_settings);

	DEBUG_END();
}

void target_heart_rate_save_settings(TargetHeartRate *self)
{
	gint i;

	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	/* Resting and maximum heart rate are updated on the fly,
	 * so update only exercise specific heart rate values
	 */

	for(i = 0; i < EC_EXERCISE_TYPE_COUNT; i++)
	{
		self->hr_range_page[i]->exercise_description->low =
			gtk_spin_button_get_value_as_int(
					GTK_SPIN_BUTTON(
						self->hr_range_page[i]->hrc[0]->
						spin_button));
		self->hr_range_page[i]->exercise_description->high =
			gtk_spin_button_get_value_as_int(
					GTK_SPIN_BUTTON(
						self->hr_range_page[i]->hrc[1]->
						spin_button));
	}

	heart_rate_settings_save(self->heart_rate_settings);

	DEBUG_END();
}
