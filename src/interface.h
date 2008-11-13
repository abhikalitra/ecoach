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
#ifndef _INTERFACE_H
#define _INTERFACE_H

/* Configuration */
#include "config.h"

/* Gtk / Glib */
#include <glib/gi18n.h>
#include <gtk/gtk.h>

/* Hildon */
#include <hildon/hildon-program.h>
#include <hildon/hildon-window.h>

/* Osso */
#include <libosso.h>

/* DBus */
#include <dbus/dbus-glib.h>

/* Other modules */ 
#include "activity.h"
#include "analyzer.h"
#include "beat_detect.h"
#include "ecg_data.h"
#include "gconf_helper.h"
#include "heart_rate_settings.h"
#include "hrm_shared.h"
#include "map_view.h"
#include "navigation_menu.h"
#include "settings.h"
#include "general_settings.h"
#include "calculate_bmi.h"
#include "calculate_maxheartrate.h"
#ifdef ENABLE_ECG_VIEW
#include "ecg_view.h"
#endif

#define EC_DBUS_SERVICE	"org.maemo.ecoach"
#define EC_DBUS_OBJECT_PATH	"/org/maemo/ecoach"
#define EC_DBUS_INTERFACE	"org.maemo.ecoach"

typedef struct _AppData {
	HildonProgram *program;
	HildonWindow *window;

	osso_context_t *osso;

	GtkWidget *notebook_main;

	/* Main menu widgets */
	GtkTable *table_main;

	/* Menu interface */
	gint navigation_menu_tab_id;
	NavigationMenu *navigation_menu;

	/* ECG data reader and handler */
	EcgData *ecg_data;

	/* Beat detector */
	BeatDetector *beat_detector;

#ifdef ENABLE_ECG_VIEW
	/* ECG view */
	gint ecg_view_tab_id;
	EcgViewData *ecg_view;
#endif

	/* Map view */
	gint map_view_tab_id;
	MapView *map_view;

	/* Analyzer view */
	gint analyzer_view_tab_id;
	AnalyzerView *analyzer_view;

	/* DBus connection */
	DBusGConnection *dbus_system;

	/* GConf helper */
	GConfHelperData *gconf_helper;

	/* Settings */
	Settings *settings;

	/* Heart rate monitor settings etc. */
	HRMData *hrm_data;

	/* Heart rate settings */
	HeartRateSettings *heart_rate_settings;

	/* Activity chooser */
	ActivityChooser *activity_chooser;
} AppData;

AppData *interface_create();

#endif /* _INTERFACE_H */
