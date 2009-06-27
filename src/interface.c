/*
 *  eCoach
 *
 *  Copyright (C) 2008  Jukka Alasalmi, Kai Skiftesvik, Sampo Savola
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
#include "interface.h"

/* Hildon */
#include <hildon/hildon-caption.h>
#include <hildon/hildon-note.h>
#include <hildon/hildon-text-view.h>
#include <hildon/hildon-pannable-area.h>
/* Other modules */
#include "config.h"
#include "calculate_bmi.h"
#include "beat_detect.h"
#include "gconf_keys.h"
#include "navigation_menu.h"
#include "hrm_settings.h"
#include "hrm_shared.h"
#include "ec_error.h"
#include "settings.h"
#include "general_settings.h"
#include "target_heart_rate.h"
#include "calculate_maxheartrate.h"
#include "util.h"

#include "debug.h"

/*****************************************************************************
 * Definitions                                                               *
 *****************************************************************************/

#define RCFILE_PATH DATADIR "/" PACKAGE_NAME "/ec_style.rc"
#define GFXDIR DATADIR "/pixmaps/" PACKAGE_NAME "/"

const gchar * MAPTILELOADER_DBUS_SERVICE = "org.ecoach.maptileloader";
const gchar * MAPTILELOADER_DBUS_OBJECT_PATH = "/org/ecoach/maptileloader";
const gchar * MAPTILELOADER_DBUS_INTERFACE = "org.ecoach.maptileloader";

const gchar * HILDON_DBUS_SERVICE = "com.nokia.hildon_desktop";
const gchar * HIDLON_DBUS_OBJECT_PATH = "/com/nokia/hildon_desktop";
const gchar * HILDON_DBUS_INTERFACE = "com.nokia.hildon_desktop";
/*****************************************************************************
 * Private function prototypes                                               *
 *****************************************************************************/

static void interface_create_menu(AppData *app_data);
static void interface_initialize_gconf(AppData *app_data);

static void interface_default_folder_changed(
		const GConfEntry *entry,
		gpointer user_data,
		gpointer user_data_2);
static void interface_measuring_units_changed(
		const GConfEntry *entry,
		gpointer user_data,
		gpointer user_data_2);
static void interface_display_state_changed(
		const GConfEntry *entry,
		gpointer user_data,
		gpointer user_data_2);

static void interface_minimize(GtkWidget *btn, gpointer user_data);
static void interface_confirm_close(GtkWidget *btn, gpointer user_data);

#ifdef ENABLE_ECG_VIEW
static void interface_show_ecg(
		NavigationMenu *menu,
		GtkTreePath *path,
		gpointer user_data);

static void interface_hide_ecg(GtkWidget *widget, gpointer user_data);
#endif

static void interface_show_map_view(
		NavigationMenu *menu,
		GtkTreePath *path,
		gpointer user_data);

static void interface_show_analyzer_view(
		NavigationMenu *menu,
		GtkTreePath *path,
		gpointer user_data);

static void interface_hide_map_view(GtkWidget *widget, gpointer user_data);

static void interface_hide_analyzer_view(GtkWidget *widget, gpointer user_data);

static void interface_show_about_dialog(
		NavigationMenu *menu,
		GtkTreePath *path,
		gpointer user_data);

static void interface_start_activity(
		NavigationMenu *menu,
		GtkTreePath *path,
		gpointer user_data);

/*****************************************************************************
 * Function declarations                                                     *
 *****************************************************************************/

/*===========================================================================*
 * Public functions                                                          *
 *===========================================================================*/

AppData *interface_create()
{
	AppData *app_data = NULL;

	app_data = g_new0(AppData, 1);

	if(!app_data)
	{
		g_critical("Not enough memory");
		return NULL;
	}

	app_data->osso = osso_initialize(EC_DBUS_SERVICE, VERSION, TRUE, NULL);

	/* Create HildonProgram and HildonWindow */
	app_data->program = HILDON_PROGRAM(hildon_program_get_instance());
	g_set_application_name(_("eCoach"));

	app_data->window = HILDON_WINDOW(hildon_window_new());
	hildon_program_add_window(app_data->program, app_data->window);
	gtk_widget_set_name(GTK_WIDGET(app_data->window), "mainwindow");

	gtk_window_fullscreen(GTK_WINDOW(app_data->window));

	/* Parse styles */
	gtk_rc_parse(RCFILE_PATH);

	/* Initialize GConf helper */
	interface_initialize_gconf(app_data);
	
	/* Initialize generic settings */
	app_data->settings = settings_initialize(app_data->gconf_helper);

	/* Initialize the heart rate settings */
	app_data->heart_rate_settings = heart_rate_settings_new(
			app_data->gconf_helper);

	/* Initialize the utility functions */
	util_initialize(app_data->settings);

	/* Set the main window for showing error messages */
	ec_error_initialize(
			(GtkWindow *)app_data->window,
			app_data->gconf_helper);

	app_data->hrm_data = hrm_initialize(app_data->gconf_helper);
	if(!app_data->hrm_data)
	{
		g_critical("hrm_initialize() failed");
		return NULL;
	}

	interface_create_menu(app_data);

	app_data->ecg_data = ecg_data_new(app_data->gconf_helper);
	if(!app_data->ecg_data)
	{
		g_critical("Could not create EcgData");
		return NULL;
	}

	app_data->beat_detector = beat_detector_new(app_data->ecg_data);
	if(!app_data->beat_detector)
	{
		g_critical("Could not create BeatDetector");
		return NULL;
	}

#ifdef ENABLE_ECG_VIEW
	app_data->ecg_view = ecg_view_new(app_data->gconf_helper,
			app_data->ecg_data);
	g_signal_connect(G_OBJECT(app_data->ecg_view->btn_close), "clicked",
			G_CALLBACK(interface_hide_ecg), app_data);

	app_data->ecg_view_tab_id = navigation_menu_append_page(
			app_data->navigation_menu,
			app_data->ecg_view->main_widget);
#endif

	/* Create map view */
	app_data->map_view = map_view_new(
			GTK_WINDOW(app_data->window),
			app_data->gconf_helper,
			app_data->beat_detector,
			app_data->osso);

	app_data->map_view_tab_id = navigation_menu_append_page(
			app_data->navigation_menu,
			app_data->map_view->main_widget);

	g_signal_connect(G_OBJECT(app_data->map_view->btn_back),
			"clicked",
			G_CALLBACK(interface_hide_map_view),
			app_data);

	/* Setup activity chooser */
	app_data->activity_chooser = activity_chooser_new(
			app_data->gconf_helper,
			app_data->heart_rate_settings,
			app_data->map_view,
			GTK_WINDOW(app_data->window));

	/* Create activity analyzer view */
	app_data->analyzer_view = analyzer_view_new(
			GTK_WINDOW(app_data->window),
			app_data->gconf_helper);

	app_data->analyzer_view_tab_id = navigation_menu_append_page(
			app_data->navigation_menu,
			app_data->analyzer_view->main_widget);

	g_signal_connect(G_OBJECT(app_data->analyzer_view->btn_back),
			"clicked",
			G_CALLBACK(interface_hide_analyzer_view),
			app_data);

	/* Get the default folder for file operations */
	gconf_helper_add_key_string(
			app_data->gconf_helper,
			ECGC_DEFAULT_FOLDER,
			"/home/user",
			interface_default_folder_changed,
			app_data,
			NULL);
	
	/* Set default measuring units */
	gconf_helper_add_key_bool(app_data->gconf_helper,USE_METRIC,TRUE,
				  interface_measuring_units_changed,app_data,
      				  NULL);

	/* Set default display state */
	gconf_helper_add_key_bool(app_data->gconf_helper,DISPLAY_ON,TRUE,
				  interface_display_state_changed,app_data,
				  NULL);
	
	/* Finalize the main window */
	gtk_container_add(GTK_CONTAINER(app_data->window),
			app_data->navigation_menu->main_widget);

	g_signal_connect(G_OBJECT(app_data->window), "delete_event",
			G_CALLBACK(gtk_main_quit), NULL);

	gtk_widget_show_all(GTK_WIDGET(app_data->window));

	return app_data;
}

/*===========================================================================*
 * Private functions                                                         *
 *===========================================================================*/

static void interface_create_menu(AppData *app_data)
{
	PangoFontDescription *desc = NULL;
	GtkTreePath *path_level_1;
	GtkTreePath *path_level_2;

	DEBUG_BEGIN();

	/* Setup the navigation menu widget */
	app_data->navigation_menu = navigation_menu_new();

	navigation_menu_set_navigation_bar_background(app_data->navigation_menu,
			GFXDIR "ec_titlebar_background.png");

	navigation_menu_set_default_button_background(app_data->navigation_menu,
			GFXDIR "menu_generic_btn.png");

	navigation_menu_set_back_button_gfx(
			app_data->navigation_menu,
			NULL,
			GFXDIR "menu_back_btn_icon.png");

	desc = pango_font_description_new();

	pango_font_description_set_family(desc,
			"Nokia Sans");
	pango_font_description_set_absolute_size(desc,
			20 * PANGO_SCALE);

	pango_font_description_set_weight(desc, PANGO_WEIGHT_SEMIBOLD);

	navigation_menu_set_font_description(
			app_data->navigation_menu,
			desc);

	pango_font_description_free(desc);

	navigation_menu_set_alignment(app_data->navigation_menu,
			PANGO_ALIGN_CENTER);

	g_signal_connect(G_OBJECT(app_data->navigation_menu->btn_close),
			"clicked",
			G_CALLBACK(interface_confirm_close), app_data);

	g_signal_connect(G_OBJECT(app_data->navigation_menu->btn_minimize),
			"clicked",
			G_CALLBACK(interface_minimize), app_data);

	/* Create the menus */
	navigation_menu_item_new_for_path(
			app_data->navigation_menu,
			NULL,
			_("New\nactivity"),
			GFXDIR "menu_new_btn",
			interface_start_activity,
			app_data,
			FALSE);

	navigation_menu_item_new_for_path(
			app_data->navigation_menu,
			NULL,
			_("Activity\nlog"),
			GFXDIR "menu_log_btn",
			interface_show_analyzer_view,
			app_data,
			FALSE);

	/* TODO: Add old activities here */

	path_level_1 = navigation_menu_item_new_for_path(
			app_data->navigation_menu,
			NULL,
			_("Performance\nanalysis"),
			GFXDIR "menu_analysis_btn",
			NULL,
			NULL,
			TRUE);

	navigation_menu_item_new_for_path(
			app_data->navigation_menu,
   			path_level_1,
			_("BMI"),
			GFXDIR "menu_bmi_btn",
			show_calculate_bmi,
			app_data,
			FALSE);

	path_level_2 = navigation_menu_item_new_for_path(
			app_data->navigation_menu,
			path_level_1,
			_("Maximum\nheart rate"),
			GFXDIR "menu_max_heart_btn",
			show_calculate_maxheartrate,
			app_data,
			TRUE);
	/*
	navigation_menu_item_new_for_path(
			app_data->navigation_menu,
			path_level_2,
			_("Calculate"),
			GFXDIR "menu_calculate_btn",
			show_calculate_maxheartrate,
			app_data,
			FALSE);*/

	/*navigation_menu_item_new_for_path(
			app_data->navigation_menu,
			path_level_2,
			_("Test"),
			NULL,
			NULL,
			NULL,
			FALSE);*/

	gtk_tree_path_free(path_level_2);

	navigation_menu_item_new_for_path(
			app_data->navigation_menu,
			path_level_1,
			_("Target\nheart rate"),
			GFXDIR "menu_target_heart_btn",
			target_heart_rate_dialog_show,
			app_data,
			FALSE);
	/*
	navigation_menu_item_new_for_path(
			app_data->navigation_menu,
			path_level_1,
			_("Analyze an\nold activity"),
			NULL,
			NULL,
			NULL,
			FALSE);
	*/
	gtk_tree_path_free(path_level_1);

	path_level_1 = navigation_menu_item_new_for_path(
			app_data->navigation_menu,
			NULL,
			_("Settings"),
			GFXDIR "menu_settings_btn",
			NULL,
			NULL,
			TRUE);

	navigation_menu_item_new_for_path(
			app_data->navigation_menu,
			path_level_1,
			_("General\nsettings"),
			GFXDIR "menu_settings_btn",
			show_general_settings,
			app_data,
			FALSE);

	navigation_menu_item_new_for_path(
			app_data->navigation_menu,
			path_level_1,
			_("Heart rate\nmonitor"),
			GFXDIR "menu_hrm_btn",
			hrm_settings_show,
			app_data,
			FALSE);

	navigation_menu_item_new_for_path(
			app_data->navigation_menu,
			path_level_1,
			_("About"),
			GFXDIR "menu_about_btn",
			interface_show_about_dialog,
			app_data,
			FALSE);

	gtk_tree_path_free(path_level_1);

#ifdef ENABLE_ECG_VIEW
	navigation_menu_item_new_for_path(
			app_data->navigation_menu,
			NULL,
			_("Show ECG"),
			NULL,
			interface_show_ecg,
			app_data,
			FALSE);
#endif

	navigation_menu_refresh(app_data->navigation_menu);

	DEBUG_END();
}

static void interface_initialize_gconf(AppData *app_data)
{
	g_return_if_fail(app_data != NULL);
	DEBUG_BEGIN();

	/* Initialize the GConf helper */
	app_data->gconf_helper = gconf_helper_new(ECGC_BASE_DIR);

	DEBUG_END();
}


static void interface_measuring_units_changed(
		const GConfEntry *entry,
		gpointer user_data,
		gpointer user_data_2)
{
	GConfValue *value = NULL;
	AppData *app_data = (AppData *)user_data;
	const gchar *use_metric;

	g_return_if_fail(app_data != NULL);
	g_return_if_fail(entry != NULL);
	DEBUG_BEGIN();

	value = gconf_entry_get_value(entry);
	use_metric = gconf_value_get_bool(value);
	DEBUG_END();
	
}

static void interface_display_state_changed(
		const GConfEntry *entry,
		gpointer user_data,
		gpointer user_data_2)
{
	GConfValue *value = NULL;
	AppData *app_data = (AppData *)user_data;
	const gchar *display_on;

	g_return_if_fail(app_data != NULL);
	g_return_if_fail(entry != NULL);
	DEBUG_BEGIN();

	value = gconf_entry_get_value(entry);
	display_on = gconf_value_get_bool(value);
	DEBUG_END();
}

static void interface_default_folder_changed(
		const GConfEntry *entry,
		gpointer user_data,
		gpointer user_data_2)
{
	GConfValue *value = NULL;
	AppData *app_data = (AppData *)user_data;
	const gchar *default_folder_name;

	g_return_if_fail(app_data != NULL);
	g_return_if_fail(entry != NULL);
	DEBUG_BEGIN();

	value = gconf_entry_get_value(entry);
	default_folder_name = gconf_value_get_string(value);
	
	activity_chooser_set_default_folder(
			app_data->activity_chooser,
			default_folder_name);

	analyzer_view_set_default_folder(
			app_data->analyzer_view,
			default_folder_name);

	DEBUG_END();
}

static void interface_minimize(GtkWidget *btn, gpointer user_data)
{
	AppData *app_data = (AppData *)user_data;

	g_return_if_fail(app_data != NULL);
	DEBUG_BEGIN();

	//gtk_window_iconify(GTK_WINDOW(app_data->window));
	
	/* Go to task switcher */
	DBusConnection *conn;
	conn = osso_get_dbus_connection(app_data->osso);
	DBusMessage *message = NULL;
	message = dbus_message_new_signal( HIDLON_DBUS_OBJECT_PATH,
					   HILDON_DBUS_INTERFACE,"exit_app_view");
	dbus_connection_send( conn, message,NULL);
	
	dbus_connection_flush(conn);
	dbus_message_unref(message);

	DEBUG_END();
}

static void interface_confirm_close(GtkWidget *btn, gpointer user_data)
{
	GtkWidget *dialog;
	LocationGPSDControl *control;
	
	AppData *app_data = (AppData *)user_data;
	gint result;

	g_return_if_fail(app_data != NULL);
	DEBUG_BEGIN();

	dialog = hildon_note_new_confirmation(
			GTK_WINDOW(app_data->window),
			_("Do you really want to close the application?"));

	result = gtk_dialog_run(GTK_DIALOG(dialog));
	
	gtk_widget_destroy(dialog);

	if(result == GTK_RESPONSE_OK)
	{
		DBusConnection *conn;
		conn = osso_get_dbus_connection(app_data->osso);
		DBusMessage *message = NULL;
		message = dbus_message_new_method_call( MAPTILELOADER_DBUS_SERVICE,
                                            MAPTILELOADER_DBUS_OBJECT_PATH,
                                            MAPTILELOADER_DBUS_INTERFACE,
                                            "MaptileLoaderTerminate");
		dbus_connection_send( conn, message,NULL);
		dbus_message_unref(message);

		control = location_gpsd_control_get_default ();
		location_gpsd_control_stop(control);
		map_view_stop(app_data->map_view);
		osso_deinitialize(app_data->osso);
		gtk_main_quit();
		
	}

	DEBUG_END();
}

static void interface_show_map_view(
		NavigationMenu *menu,
		GtkTreePath *path,
		gpointer user_data)
{
	AppData *app_data = (AppData *)user_data;

	g_return_if_fail(app_data != NULL);
	DEBUG_BEGIN();

	navigation_menu_set_current_page(
			app_data->navigation_menu,
			app_data->map_view_tab_id);

	map_view_show(app_data->map_view);

	DEBUG_END();
}

static void interface_show_analyzer_view(
		NavigationMenu *menu,
		GtkTreePath *path,
		gpointer user_data)
{
	AppData *app_data = (AppData *)user_data;

	g_return_if_fail(app_data != NULL);
	DEBUG_BEGIN();

	navigation_menu_set_current_page(
			app_data->navigation_menu,
			app_data->analyzer_view_tab_id);

	analyzer_view_show(app_data->analyzer_view);

	DEBUG_END();
}

static void interface_hide_map_view(GtkWidget *widget, gpointer user_data)
{
	AppData *app_data = (AppData *)user_data;
	g_return_if_fail(app_data != NULL);

	DEBUG_BEGIN();

	navigation_menu_return_to_menu(app_data->navigation_menu);

	DEBUG_END();

}

static void interface_hide_analyzer_view(GtkWidget *widget, gpointer user_data)
{
	AppData *app_data = (AppData *)user_data;
	g_return_if_fail(app_data != NULL);

	DEBUG_BEGIN();

	navigation_menu_return_to_menu(app_data->navigation_menu);
	analyzer_view_hide(app_data->analyzer_view);

	DEBUG_END();
}

static void interface_show_about_dialog(
		NavigationMenu *menu,
		GtkTreePath *path,
		gpointer user_data)
{
	GtkWidget *lbl_about;
	GtkWidget *dialog = NULL;
	GtkWidget *vbox = NULL;
	GtkWidget *hbox = NULL;
	GtkWidget *pannable = NULL;
	GtkWidget *cont = NULL;
	GtkWidget *view = NULL;
	GtkWidget *scrollbar = NULL;
	GtkTextBuffer *buffer = NULL;
	PangoFontDescription *desc = NULL;

	AppData *app_data = (AppData *)user_data;
	gchar *text_about;
	GtkTextIter iter;

	g_return_if_fail(app_data != NULL);
	DEBUG_BEGIN();

	dialog = gtk_dialog_new_with_buttons(
			_("About eCoach"),
			GTK_WINDOW(app_data->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR,
			NULL);

	vbox = GTK_DIALOG(dialog)->vbox;

	text_about = g_strdup_printf(_(
				"eCoach version %s Copyright (C) 2009 \n"
				"Jukka Alasalmi Sampo Savola\n"
				"Kai Skiftesvik Veli-Pekka Haajanen\n"),
				PACKAGE_VERSION);
	lbl_about = gtk_label_new(text_about);
	g_free(text_about);
      
	pannable =  hildon_pannable_area_new();

	gtk_box_pack_start(GTK_BOX(vbox), lbl_about, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	//view = gtk_text_view_new();
	
	view = hildon_text_view_new();
	hildon_pannable_area_add_with_viewport (HILDON_PANNABLE_AREA (pannable), view);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view), GTK_WRAP_WORD);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
	//gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(view), FALSE);
	gtk_widget_set_size_request(dialog, 800, 390);

	desc = pango_font_description_from_string("Nokia Sans 14");
	gtk_widget_modify_font(view, desc);
	pango_font_description_free(desc);

	buffer = hildon_text_view_get_buffer(HILDON_TEXT_VIEW(view));

	gtk_text_buffer_set_text(buffer, _(
		"eCoach comes with ABSOLUTELY NO WARRANTY. This is "
		"free software, and you are welcome to redistribute it under "
		"certain conditions. Read below for details.\n\n"), -1);

	gtk_text_buffer_get_end_iter(buffer, &iter);

	gtk_text_buffer_insert(buffer, &iter, _(
		"This software contains components from MapWidget "
		"[https://garage.maemo.org/projects/mapwidget/], "
		"Open Source ECG Analysis Software (OSEA) "
		"[http://eplimited.com/] and PhysioToolkit "
		"[http://www.physionet.org/physiotools/wfdb.shtml]\n\n"), -1);

	gtk_text_buffer_get_end_iter(buffer, &iter);

	gtk_text_buffer_insert(buffer, &iter, _(
		"MapWidget\n"
		"\tCopyright (C) 2007 Pekka Rönkkö"
		"\tCopyright (C) 2006-2007 John Costigan\n\n"), -1);

	gtk_text_buffer_get_end_iter(buffer, &iter);
	gtk_text_buffer_insert(buffer, &iter, _(
		"WFDB library (ecgcodes.h)\n"
		"\tCopyright (C) 1999 George B. Moody\n"), -1);

	gtk_text_buffer_get_end_iter(buffer, &iter);

	gtk_text_buffer_insert(buffer, &iter, _(
		"OSEA\n"
		"\tCopywrite (C) 2000-2002 Patrick S. Hamilton\n\n"), -1);

	/* Add the whole GPL license */
	gtk_text_buffer_get_end_iter(buffer, &iter);

	gtk_text_buffer_insert(buffer, &iter,
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
"Public License instead of this License.\n", -1);

	cont = gtk_table_new(1,1,TRUE);
	gtk_widget_set_size_request(cont,
			800,
			240);
	gtk_container_add (GTK_CONTAINER (cont),pannable);
	gtk_box_pack_start(GTK_BOX(hbox), cont, FALSE, FALSE, 0);
	
	//scrollbar = gtk_vscrollbar_new(NULL);
	//gtk_widget_set_scroll_adjustments(view, NULL,
	//		GTK_RANGE(scrollbar)->adjustment);

	//gtk_box_pack_start(GTK_BOX(hbox), scrollbar, FALSE, FALSE, 0);

	gtk_widget_show_all(dialog);

	gtk_dialog_run(GTK_DIALOG(dialog));

	gtk_widget_destroy(dialog);

	DEBUG_END();
}

#ifdef ENABLE_ECG_VIEW
static void interface_show_ecg(
		NavigationMenu *menu,
		GtkTreePath *path,
		gpointer user_data)
{
	AppData *app_data = (AppData *)user_data;

	g_return_if_fail(app_data != NULL);
	DEBUG_BEGIN();

	ecg_view_start_drawing(app_data->ecg_view);

	navigation_menu_set_current_page(
			app_data->navigation_menu,
			app_data->ecg_view_tab_id);

	DEBUG_END();
}

static void interface_hide_ecg(GtkWidget *widget, gpointer user_data)
{
	AppData *app_data = (AppData *)user_data;
	g_return_if_fail(app_data != NULL);

	DEBUG_BEGIN();

	/* Just for the case if this gets later called from outside the
	 * EcgView */
	ecg_view_stop_drawing(app_data->ecg_view);

	navigation_menu_return_to_menu(app_data->navigation_menu);

	DEBUG_END();
}
#endif

static void interface_start_activity(
		NavigationMenu *menu,
		GtkTreePath *path,
		gpointer user_data)
{
	GtkWidget *dialog;
	MapViewActivityState activity_state;
	AppData *app_data = (AppData *)user_data;
	gboolean choose_activity = FALSE;
	gint response;

	g_return_if_fail(app_data != NULL);
	DEBUG_BEGIN();

	activity_state = map_view_get_activity_state(app_data->map_view);

	if (activity_state == MAP_VIEW_ACTIVITY_STATE_STOPPED)
	{
		dialog = hildon_note_new_confirmation_add_buttons(
				GTK_WINDOW(app_data->window),
		_("There is an activity in stopped state.\n"
		  "Do you want to continue it into a new track or\n"
		  "start a new activity?"),
		_("New"), GTK_RESPONSE_OK,
		_("Continue"), GTK_RESPONSE_CANCEL,
		NULL);
		gtk_widget_show_all(dialog);
		response = gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		if(response == GTK_RESPONSE_OK)
		{
			map_view_clear_all(app_data->map_view);
			choose_activity = TRUE;
		} else {
			choose_activity = FALSE;
		}
	} else if(activity_state == MAP_VIEW_ACTIVITY_STATE_NOT_STARTED) {
		choose_activity = TRUE;
	} else {
		choose_activity = FALSE;
	}

	if(choose_activity)
	{
		if(activity_chooser_choose_and_setup_activity(
					app_data->activity_chooser))
		{
			interface_show_map_view(NULL,
					NULL,
					app_data);
		}
	} else {
		interface_show_map_view(NULL,
				NULL,
				app_data);
	}

	DEBUG_END();
}
