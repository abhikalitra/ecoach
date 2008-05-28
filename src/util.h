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
#ifndef _UTIL_H
#define _UTIL_H

/* Configuration */
#include "config.h"

/* System */
#include <sys/time.h>
#include <time.h>

/* Glib */
#include <glib/gtypes.h>

/* Other modules */
#include "settings.h"

/**
 * @brief Initialize the utility functions
 *
 * @param settings Pointer to #Settings
 */
void util_initialize(Settings *settings);

/**
 * @brief Replace characters in a string with a character
 *
 * @param text Text to replace the characters in
 * @param to_replace Characters to be replaced
 * @param replace_with Characters to be replaced with
 */
void util_replace_chars_with_char(
		gchar *text,
		gchar to_replace,
		gchar replace_with);

/**
 * @brief Subtracts time2 from time1 and puts the result into %result
 *
 * @param time1 Time to be subtracted from
 * @param time2 Time to subtract
 * @param result Storage place for the result
 */
void util_subtract_time(struct timeval *time1, struct timeval *time2,
		struct timeval *result);

/**
 * @brief Adds up time1 and time2 and puts the result into %result
 *
 * @param time1 Time to be added
 * @param time2 Time to be added
 * @param result Storage place for the result. This may point to same
 * memory location as time1 or time2.
 */
void util_add_time(struct timeval *time1, struct timeval *time2,
		struct timeval *result);

/**
 * @brief Create an XML dateTime string representation from a struct timeval.
 * 
 * @param time Time to represent as a string
 * 
 * @return The time in XML string representation
 *
 * @note The return value must be freed when no longer used
 */
gchar *util_xml_date_time_string_from_timeval(struct timeval *time);

/**
 * @brief Create a date string representation from a struct timeval, ignoring
 * the time
 *
 * If the time parameter is NULL, the current system time is used
 *
 * @param time Time to represent as a string, or NULL to use current time
 *
 * @return The time in format yyyy-mm-dd (must be freed with g_free)
 */
gchar *util_date_string_from_timeval(struct timeval *time);

/**
 * @brief Create a struct timeval representation from an xml dateTime string
 *
 * @param string The string to be parsed
 * @param time The timeval representation of the time
 *
 * @return TRUE if the parsing succeeded, FALSE otherwise
 */
gboolean util_timeval_from_xml_date_time_string(
		const gchar *string,
		struct timeval *time);

/**
 * @brief Compare two times
 *
 * @param time_1 Time 1 to compare
 * @param time_2 Time 2 to compare
 *
 * @return -1 if time_1 is smaller than time_2, 1 if time_1 is greater
 * than time_2 and 0 if the times are equal
 */
gint util_compare_timevals(struct timeval *time_1, struct timeval *time_2);

#endif /* _UTIL_H */
