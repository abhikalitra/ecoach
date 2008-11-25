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

#ifndef _GCONF_KEYS_H
#define _GCONF_KEYS_H

#define ECGC_BASE_DIR		"/apps/ecoach"

#define ECGC_BLUETOOTH_ADDRESS	ECGC_BASE_DIR "/bluetooth_address"
#define ECGC_BLUETOOTH_NAME	ECGC_BASE_DIR "/bluetooth_name"

#define ECGC_HRM_DIALOG_SHOWN	ECGC_BASE_DIR "/hrm_dialog_shown"

#define ECGC_HRM_RANGES_DIALOG_SHOWN		ECGC_BASE_DIR \
	"/hrm_ranges_dialog_shown"

#define ECGC_MAP_VIEW_STOP_DIALOG_SHOWN	ECGC_BASE_DIR \
	"/map_view_stop_dialog_shown"

#define ECGC_ANALYZER_VIEW_GRAPH_DIALOG_SHOWN	ECGC_BASE_DIR \
	"/analyzer_view_graph_dialog_shown"

#define ECGC_EXERC_ANAEROBIC_DIR	ECGC_BASE_DIR "/exercise_anaerobic"
#define ECGC_EXERC_AEROBIC_DIR		ECGC_BASE_DIR "/exercise_aerobic"
#define ECGC_EXERC_WEIGHT_CTRL_DIR	ECGC_BASE_DIR \
	"/exercise_weight_control"
#define ECGC_EXERC_MODERATE_DIR	ECGC_BASE_DIR "/exercise_moderate"

#define ECGC_HR_LIMIT_LOW	"heart_rate_limit_low"
#define ECGC_HR_LIMIT_HIGH	"heart_rate_limit_high"

#define ECGC_HR_REST		ECGC_BASE_DIR "/resting_heart_rate"
#define ECGC_HR_MAX		ECGC_BASE_DIR "/maximum_heart_rate"

#define ECGC_DEFAULT_FOLDER	ECGC_BASE_DIR "/default_folder"

#define ECGC_EXERC_DFL_NAME	ECGC_BASE_DIR "/exercise_default_name"
#define ECGC_EXERC_DFL_RANGE_ID	ECGC_BASE_DIR \
	"/exercise_default_heart_rate_range_id"

#define ECGC_IGNORE_TIME_ZONES	ECGC_BASE_DIR "/ignore_time_zones"

#define USE_METRIC		ECGC_BASE_DIR "/metric_units"

#define DISPLAY_ON		ECGC_BASE_DIR "/display_on"

#endif /* _GCONF_KEYS_H */
