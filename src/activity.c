/*
 *  eCoach
 *
 *  Copyright (C) 2009 Sampo Savola, Jukka Alasalmi
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
#include "activity.h"

/* System */
#include <string.h>

/* i18n */
#include <glib/gi18n.h>

/* Hildon */
#include <hildon/hildon-file-chooser-dialog.h>
#include <hildon/hildon-file-system-model.h>
#include <hildon/hildon-caption.h>
#include <hildon/hildon-file-selection.h>
/* Other modules */
#include "gconf_keys.h"
#include "ec_error.h"
#include "util.h"

#include "debug.h"

/*****************************************************************************
 * Private function prototypes                                               *
 *****************************************************************************/

/*===========================================================================*
 * ActivityDescription                                                       *
 *===========================================================================*/

/*===========================================================================*
 * ActivityChooser                                                           *
 *===========================================================================*/

static void activity_default_range_changed(
        const GConfEntry *entry,
        gpointer user_data,
        gpointer user_data_2);

static void activity_default_name_changed(
        const GConfEntry *entry,
        gpointer user_data,
        gpointer user_data_2);

static ActivityChooserDialog *activity_chooser_dialog_new(
        ActivityChooser *self);

static void activity_chooser_dialog_destroy(
        ActivityChooserDialog *chooser_dialog);

/**
 * @brief Return a file name for saving, or NULL if Cancel was pressed
 *
 * @param self Pointer to #ActivityChooser
 * @param chooser_dialog Pointer to #ActivityChooserDialog
 *
 * @return The file name, or NULL if Cancel was pressed. The return value
 * must be freed with g_free.
 */
static gchar *activity_chooser_dialog_choose_file_name(
        ActivityChooser *self,
        ActivityChooserDialog *chooser_dialog);

/*****************************************************************************
 * Function declarations                                                     *
 *****************************************************************************/

/*===========================================================================*
 * ActivityDescription                                                       *
 *===========================================================================*/

/*---------------------------------------------------------------------------*
 * Public functions                                                          *
 *---------------------------------------------------------------------------*/

ActivityDescription *activity_description_new(const gchar *activity_name)
{
    ActivityDescription *self = NULL;
    DEBUG_BEGIN();

    self = g_new0(ActivityDescription, 1);
    self->activity_name = g_strdup(activity_name);

    DEBUG_END();
    return self;
}

void activity_description_free(ActivityDescription *self)
{
    g_return_if_fail(self != NULL);
    DEBUG_BEGIN();

    g_free(self->file_name);
    g_free(self->activity_name);
    g_free(self);

    DEBUG_END();
}

/*---------------------------------------------------------------------------*
 * Private functions                                                         *
 *---------------------------------------------------------------------------*/

/*===========================================================================*
 * ActivityChooser                                                           *
 *===========================================================================*/

/*---------------------------------------------------------------------------*
 * Public functions                                                          *
 *---------------------------------------------------------------------------*/

ActivityChooser *activity_chooser_new(
        GConfHelperData *gconf_helper,
        HeartRateSettings *heart_rate_settings,
        MapView *map_view,
        GtkWindow *parent_window)
{
    ActivityChooser *self = NULL;

    g_return_val_if_fail(gconf_helper != NULL, NULL);
    g_return_val_if_fail(heart_rate_settings != NULL, NULL);
    g_return_val_if_fail(parent_window != NULL, NULL);
    DEBUG_BEGIN();

    self = g_new0(ActivityChooser, 1);
    self->gconf_helper = gconf_helper;
    self->heart_rate_settings = heart_rate_settings;
    self->map_view = map_view;
    self->parent_window = parent_window;

    /* Setup necessary GConf settings */
    gconf_helper_add_key_int(
            self->gconf_helper,
            ECGC_EXERC_DFL_NAME,
            0,
            activity_default_name_changed,
            self,
            NULL);

    gconf_helper_add_key_int(
            self->gconf_helper,
            ECGC_EXERC_DFL_RANGE_ID,
            0,
            activity_default_range_changed,
            self,
            NULL);

    DEBUG_END();
    return self;
}

void activity_chooser_set_default_folder(
        ActivityChooser *self,
        const gchar *folder)
{
    g_return_if_fail(self != NULL);
    DEBUG_BEGIN();

    g_free(self->default_folder_name);
    self->default_folder_name = g_strdup(folder);

    DEBUG_END();
}

ActivityDescription *activity_chooser_choose_activity(ActivityChooser *self)
{
    ActivityChooserDialog *chooser_dialog = NULL;
    ActivityDescription *activity_description;
    EcExerciseDescription *exercise_description;
    gint response;
    gchar *file_name = NULL;

    g_return_val_if_fail(self != NULL, NULL);
    DEBUG_BEGIN();

    chooser_dialog = activity_chooser_dialog_new(self);

    gtk_widget_show_all(chooser_dialog->dialog);

    do {
        response = gtk_dialog_run(GTK_DIALOG(chooser_dialog->dialog));

        if(response == GTK_RESPONSE_DELETE_EVENT)
        {
            activity_chooser_dialog_destroy(chooser_dialog);
            DEBUG_END();
            return NULL;
        }
        file_name = activity_chooser_dialog_choose_file_name(
                self,
                chooser_dialog);

    } while(!file_name);

    /*	activity_description = activity_description_new(
			gtk_entry_get_text(GTK_ENTRY(chooser_dialog->
					entry_activity_name)));
*/

    activity_description = activity_description_new(hildon_touch_selector_get_current_text(chooser_dialog->sportselector));

    activity_description->activity_comment = g_strdup(
            gtk_entry_get_text(GTK_ENTRY(chooser_dialog->
                                         entry_activity_comment)));

    activity_description->file_name = file_name;


    //	  activity_description->heart_rate_range_id = gtk_combo_box_get_active(
    //			GTK_COMBO_BOX(chooser_dialog->cmb_pulse_ranges));

    activity_description->heart_rate_range_id = hildon_picker_button_get_active (chooser_dialog->button);
    DEBUG("HR RANGE ID SELECTED: %d", activity_description->heart_rate_range_id);
    exercise_description = &self->heart_rate_settings->
                           exercise_descriptions[activity_description->
                                                 heart_rate_range_id];

    activity_description->heart_rate_limit_low =
            exercise_description->low;
    activity_description->heart_rate_limit_high =
            exercise_description->high;
    DEBUG("%d",activity_description->heart_rate_limit_low);
    DEBUG("%d",activity_description->heart_rate_limit_high);
    if(hildon_check_button_get_active(HILDON_CHECK_BUTTON(
            chooser_dialog->chk_add_calendar)))
    {

	activity_description->add_calendar = TRUE;

    }
    else{

        activity_description->add_calendar = FALSE;
    }
    if(hildon_check_button_get_active(HILDON_CHECK_BUTTON(
            chooser_dialog->chk_save_dfl)))
    {

        gint selected =  hildon_touch_selector_get_active(chooser_dialog->sportselector,0);

        /* Save the default values */
        gconf_helper_set_value_int(
                self->gconf_helper,
                ECGC_EXERC_DFL_NAME,
                selected);

        gconf_helper_set_value_int(
                self->gconf_helper,
                ECGC_EXERC_DFL_RANGE_ID,
                activity_description->heart_rate_range_id);
    }

    activity_chooser_dialog_destroy(chooser_dialog);

    DEBUG_END();
    return activity_description;
}

gboolean activity_chooser_choose_and_setup_activity(ActivityChooser *self)
{
    ActivityDescription *activity_description = NULL;

    g_return_val_if_fail(self != NULL, FALSE);
    DEBUG_BEGIN();

    activity_description = activity_chooser_choose_activity(self);
    if(!activity_description)
    {
        DEBUG_END();
        return FALSE;
    }

    if(map_view_setup_activity(
            self->map_view,
            activity_description->activity_name,
            activity_description->activity_comment,
            activity_description->file_name,
            activity_description->heart_rate_limit_low,
            activity_description->heart_rate_limit_high,
            activity_description->add_calendar))
    {
        activity_description_free(activity_description);
        DEBUG_END();
        return TRUE;
    }

    activity_description_free(activity_description);
    DEBUG_END();
    return FALSE;
}

/*---------------------------------------------------------------------------*
 * Private functions                                                         *
 *---------------------------------------------------------------------------*/

static void activity_default_name_changed(
        const GConfEntry *entry,
        gpointer user_data,
        gpointer user_data_2)
{
    GConfValue *value = NULL;
    gint id;
    ActivityChooser *self = (ActivityChooser *)user_data;

    g_return_if_fail(self != NULL);
    g_return_if_fail(entry != NULL);
    DEBUG_BEGIN();

    value = gconf_entry_get_value(entry);
    id = gconf_value_get_int(value);

    self->default_activity_name = id;

    DEBUG_END();
}

static void activity_default_range_changed(
        const GConfEntry *entry,
        gpointer user_data,
        gpointer user_data_2)
{
    gint id;
    GConfValue *value = NULL;
    ActivityChooser *self = (ActivityChooser *)user_data;

    g_return_if_fail(self != NULL);
    g_return_if_fail(entry != NULL);
    DEBUG_BEGIN();

    value = gconf_entry_get_value(entry);
    id = gconf_value_get_int(value);
    if(id < 0 || id >= EC_EXERCISE_TYPE_COUNT)
    {
        g_warning("Invalid heart rate range type");
        DEBUG_END();
        return;
    }
    self->default_heart_rate_range_id = id;

    DEBUG_END();
}

static ActivityChooserDialog *activity_chooser_dialog_new(
        ActivityChooser *self)
{
    ActivityChooserDialog *chooser_dialog = NULL;
    GtkWidget *vbox = NULL;
    GtkWidget *caption = NULL;

    GtkSizeGroup *size_group = NULL;

    gint i;

    g_return_val_if_fail(self != NULL, NULL);
    DEBUG_BEGIN();

    chooser_dialog = g_new0(ActivityChooserDialog, 1);
    chooser_dialog->dialog = gtk_dialog_new_with_buttons(
            _("Start a new activity"),
            GTK_WINDOW(self->parent_window),
            GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR,
            _("OK"), GTK_RESPONSE_OK,
            _("Cancel"), GTK_RESPONSE_CANCEL,
            NULL);

    vbox = GTK_DIALOG(chooser_dialog->dialog)->vbox;

    size_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

    chooser_dialog->sportbutton = hildon_picker_button_new (HILDON_SIZE_FINGER_HEIGHT, HILDON_BUTTON_ARRANGEMENT_VERTICAL);
    hildon_button_set_title (HILDON_BUTTON (chooser_dialog->sportbutton), _("Sport"));

    chooser_dialog->sportselector = hildon_touch_selector_new_text ();


    hildon_touch_selector_append_text(HILDON_TOUCH_SELECTOR (chooser_dialog->sportselector), _("Running"));
    hildon_touch_selector_append_text(HILDON_TOUCH_SELECTOR (chooser_dialog->sportselector), _("Cycling"));
    hildon_touch_selector_append_text(HILDON_TOUCH_SELECTOR (chooser_dialog->sportselector), _("Walking"));
    hildon_touch_selector_append_text(HILDON_TOUCH_SELECTOR (chooser_dialog->sportselector), _("Nordic walking"));
    hildon_touch_selector_append_text(HILDON_TOUCH_SELECTOR (chooser_dialog->sportselector), _("Skiing"));
    hildon_touch_selector_append_text(HILDON_TOUCH_SELECTOR (chooser_dialog->sportselector), _("Roller skating"));
    hildon_touch_selector_append_text(HILDON_TOUCH_SELECTOR (chooser_dialog->sportselector), _("Alpine skiing"));
    hildon_touch_selector_append_text(HILDON_TOUCH_SELECTOR (chooser_dialog->sportselector), _("Rowing"));
    hildon_touch_selector_append_text(HILDON_TOUCH_SELECTOR (chooser_dialog->sportselector), _("Riding"));
    hildon_touch_selector_append_text(HILDON_TOUCH_SELECTOR (chooser_dialog->sportselector), _("Mountain biking"));
    hildon_touch_selector_append_text(HILDON_TOUCH_SELECTOR (chooser_dialog->sportselector), _("Snowboarding"));
    hildon_touch_selector_append_text(HILDON_TOUCH_SELECTOR (chooser_dialog->sportselector), _("Snowshoeing"));
    hildon_touch_selector_append_text(HILDON_TOUCH_SELECTOR (chooser_dialog->sportselector), _("Orienteering"));
    hildon_touch_selector_append_text(HILDON_TOUCH_SELECTOR (chooser_dialog->sportselector), _("Sailing"));
    hildon_touch_selector_append_text(HILDON_TOUCH_SELECTOR (chooser_dialog->sportselector), _("Hiking"));
    hildon_touch_selector_append_text(HILDON_TOUCH_SELECTOR (chooser_dialog->sportselector), _("Roller skiing"));
    hildon_touch_selector_append_text(HILDON_TOUCH_SELECTOR (chooser_dialog->sportselector), _("Telemark skiing"));
    hildon_touch_selector_append_text(HILDON_TOUCH_SELECTOR (chooser_dialog->sportselector), _("Jogging"));
    hildon_touch_selector_append_text(HILDON_TOUCH_SELECTOR (chooser_dialog->sportselector), _("Walking the dog"));
    hildon_touch_selector_append_text(HILDON_TOUCH_SELECTOR (chooser_dialog->sportselector), _("Golf"));
    hildon_touch_selector_append_text(HILDON_TOUCH_SELECTOR (chooser_dialog->sportselector), _("Frisbeegolf"));


    gint dfl_index =  gconf_helper_get_value_int_with_default(
            self->gconf_helper,
            ECGC_EXERC_DFL_NAME,
            0);
    hildon_touch_selector_set_active (HILDON_TOUCH_SELECTOR (chooser_dialog->sportselector), 0, dfl_index);
    hildon_picker_button_set_selector (HILDON_PICKER_BUTTON (chooser_dialog->sportbutton),HILDON_TOUCH_SELECTOR (chooser_dialog->sportselector));
    /*
	chooser_dialog->entry_activity_name = hildon_entry_new (HILDON_SIZE_AUTO);
	if(self->default_activity_name)
	{
		gtk_entry_set_text(GTK_ENTRY(chooser_dialog->
					entry_activity_name),
				self->default_activity_name);
		gtk_editable_select_region(GTK_EDITABLE(chooser_dialog->
					entry_activity_name),
				0, -1);
	}

	caption = hildon_caption_new(
			size_group,
			_("Activity name"),
			chooser_dialog->entry_activity_name,
			NULL,
			HILDON_CAPTION_OPTIONAL);
*/
    gtk_box_pack_start(GTK_BOX(vbox), chooser_dialog->sportbutton, FALSE, FALSE, 0);


    /*
	chooser_dialog->entry_activity_comment = hildon_entry_new (HILDON_SIZE_AUTO);

	caption = hildon_caption_new(
			size_group,
			_("Comment"),
			chooser_dialog->entry_activity_comment,
			NULL,
			HILDON_CAPTION_OPTIONAL);

	gtk_box_pack_start(GTK_BOX(vbox), caption, FALSE, FALSE, 0);
*/
    /*
	chooser_dialog->cmb_pulse_ranges = gtk_combo_box_new_text();



	gtk_combo_box_set_active(GTK_COMBO_BOX(chooser_dialog->
				cmb_pulse_ranges),
			self->default_heart_rate_range_id);

	*/

    chooser_dialog->button = hildon_picker_button_new (HILDON_SIZE_FINGER_HEIGHT, HILDON_BUTTON_ARRANGEMENT_VERTICAL);
    hildon_button_set_title (HILDON_BUTTON (chooser_dialog->button), _("Target heart rate type"));
    chooser_dialog->selector = hildon_touch_selector_new_text ();

    for(i = 0; i < EC_EXERCISE_TYPE_COUNT; i++)
    {

	hildon_touch_selector_append_text(HILDON_TOUCH_SELECTOR (chooser_dialog->selector), self->heart_rate_settings->exercise_descriptions[i].name);
        hildon_touch_selector_set_active (HILDON_TOUCH_SELECTOR (chooser_dialog->selector), 0, self->default_heart_rate_range_id);

    }
    hildon_picker_button_set_selector (HILDON_PICKER_BUTTON (chooser_dialog->button),HILDON_TOUCH_SELECTOR (chooser_dialog->selector));


    gtk_box_pack_start(GTK_BOX(vbox), chooser_dialog->button, FALSE, FALSE, 0);

    chooser_dialog->chk_save_dfl = hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);
    gtk_button_set_label (GTK_BUTTON ( chooser_dialog->chk_save_dfl), _("Save as default"));

    gtk_box_pack_start(GTK_BOX(vbox), chooser_dialog->chk_save_dfl, FALSE, FALSE, 0);

    chooser_dialog->chk_add_calendar = hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);
    gtk_button_set_label (GTK_BUTTON (chooser_dialog->chk_add_calendar), _("Add to calendar"));
    hildon_check_button_set_active(GTK_BUTTON ( chooser_dialog->chk_add_calendar),TRUE);
    gtk_box_pack_start(GTK_BOX(vbox),chooser_dialog->chk_add_calendar, FALSE, FALSE, 0);
    DEBUG_END();
    return chooser_dialog;
}

static void activity_chooser_dialog_destroy(
        ActivityChooserDialog *chooser_dialog)
{
    DEBUG_BEGIN();
    g_return_if_fail(chooser_dialog != NULL);

    gtk_widget_destroy(chooser_dialog->dialog);
    g_free(chooser_dialog);

    DEBUG_END();
}

static gchar *activity_chooser_dialog_choose_file_name(
        ActivityChooser *self,
        ActivityChooserDialog *chooser_dialog)
{
    HildonFileSystemModel *fs_model = NULL;
    GtkWidget *file_dialog = NULL;

    gchar *file_name = NULL;
    const gchar *activity_name = NULL;
    gchar *date_str = NULL;
    gchar *dfl_fname = NULL;
    gint result;
    gint i = 1;
    g_return_val_if_fail(self != NULL, NULL);
    g_return_val_if_fail(chooser_dialog != NULL, NULL);
    DEBUG_BEGIN();
    gchar* extension;
    extension = g_strdup_printf(".gpx");
    /*
	activity_name = gtk_entry_get_text(
			GTK_ENTRY(chooser_dialog->entry_activity_name));
*/
    //	activity_name = hildon_picker_button_get_active (chooser_dialog->button);
    activity_name = hildon_touch_selector_get_current_text(chooser_dialog->sportselector);
    DEBUG("%s", activity_name);
    date_str = util_date_string_from_timeval(NULL);

    if(strcmp(activity_name, "") == 0)
    {
        dfl_fname = g_strdup_printf("%s", date_str);
    } else {
        dfl_fname = g_strdup_printf("%s-%s", date_str,
                                    activity_name);
    }

    g_free(date_str);

    fs_model = HILDON_FILE_SYSTEM_MODEL(
            g_object_new(HILDON_TYPE_FILE_SYSTEM_MODEL,
                         "ref_widget", GTK_WIDGET(self->parent_window), NULL));
    GtkWidget *selection = hildon_file_selection_new_with_model(fs_model);
    hildon_file_selection_set_sort_key(selection,HILDON_FILE_SELECTION_SORT_NAME,GTK_SORT_DESCENDING);

    if(!fs_model)
    {
        ec_error_show_message_error(
                _("Unable to open File chooser dialog"));
        DEBUG_END();
        return NULL;
    }

    gtk_file_chooser_set_current_folder(
            GTK_FILE_CHOOSER(file_dialog),
            self->default_folder_name);

    gtk_file_chooser_set_current_name(
            GTK_FILE_CHOOSER(file_dialog),
            dfl_fname);

    file_name =  g_strdup_printf("%s/%s%s",self->default_folder_name,dfl_fname,extension);
    DEBUG("%s",file_name);
    while(g_file_test(file_name,G_FILE_TEST_EXISTS))
    {
	g_free(file_name);
	file_name =  g_strdup_printf("%s/%s-%d%s",self->default_folder_name,dfl_fname,i,extension);
	DEBUG("%s",file_name);
	i++;
    }
    gconf_helper_set_value_string_simple(self->gconf_helper,LAST_ACTIVITY,file_name);
    g_free(extension);
    g_free(dfl_fname);

    DEBUG_END();
    return file_name;
}
