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
#include "util.h"

/* System */
#include <stdlib.h>
#include <string.h>

/* GLib */
#include <glib/gmessages.h>
#include <glib/gstrfuncs.h>
#include <glib/gmem.h>

/* Other modules */
#include "debug.h"

/*****************************************************************************
 * Static variables                                                          *
 *****************************************************************************/

static Settings *_util_settings = NULL;

/*****************************************************************************
 * Private function prototypes                                               *
 *****************************************************************************/

static const gchar *util_get_timezone_string();

/**
 * @brief Convert a time from a broken-down representation to time_t format,
 * with both input and output being in UTC
 *
 * @param tm Time to convert
 *
 * @return Time in time_t format
 */
time_t my_timegm(struct tm *tm);

/*****************************************************************************
 * Function declarations                                                     *
 *****************************************************************************/

/*===========================================================================*
 * Public functions                                                          *
 *===========================================================================*/

void util_initialize(Settings *settings)
{
	_util_settings = settings;
	tzset();
}

void util_replace_chars_with_char(
		gchar *text,
		gchar to_replace,
		gchar replace_with)
{
	DEBUG_BEGIN();

	g_return_if_fail(text != NULL);

	gchar *ptr = text;

	do {
		if(*ptr == to_replace)
		{
			*ptr = replace_with;
		}
		ptr++;
	} while(*ptr != '\0');

	DEBUG_END();
}

void util_subtract_time(struct timeval *time1, struct timeval *time2,
		struct timeval *result)
{	
	g_return_if_fail(time1 != NULL);
	g_return_if_fail(time2 != NULL);
	g_return_if_fail(result != NULL);

	DEBUG_BEGIN();

	if(
			(time2->tv_sec > time1->tv_sec)
				||
			(
			 	(time2->tv_sec == time1->tv_sec)
					&&
			 	(time2->tv_usec > time1->tv_usec)
			)
	  )
	{
		g_warning("struct timeval cannot represent negative values");
		DEBUG_END();
		return;
	}

	result->tv_sec = time1->tv_sec - time2->tv_sec;
	if(time2->tv_usec > time1->tv_usec)
	{
		result->tv_sec--;
		result->tv_usec = 1000000 - time2->tv_usec + time1->tv_usec;
	} else {
		result->tv_usec = time1->tv_usec - time2->tv_usec;
	}

	DEBUG_END();
}

void util_add_time(struct timeval *time1, struct timeval *time2,
		struct timeval *result)
{
	g_return_if_fail(time1 != NULL);
	g_return_if_fail(time2 != NULL);
	g_return_if_fail(result != NULL);

	DEBUG_BEGIN();

	result->tv_sec = time1->tv_sec + time2->tv_sec;
	if(time1->tv_usec + time2->tv_usec > 1000000)
	{
		result->tv_sec++;
		result->tv_usec = time1->tv_usec + time2->tv_usec - 1000000;
	} else {
		result->tv_usec = time1->tv_usec + time2->tv_usec;
	}

	DEBUG_END();
}

gchar *util_xml_date_time_string_from_timeval(struct timeval *time)
{
	gchar *csecs_s = NULL;
	gchar *retval = NULL;
	glong csecs = 0;
	time_t time_src;
	struct tm time_dest;

	g_return_val_if_fail(time != NULL, NULL);
	DEBUG_BEGIN();

	time_src = time->tv_sec;
	localtime_r(&time_src, &time_dest);

	/* XML dateTime second fraction must not end with a zero, even though
	 * seems a bit weird since it prevents including accuracy of the time
	 * by including necessary amount of significant digits. */
	csecs = time->tv_usec / 1000L;
	if(csecs == 0)
	{
		csecs_s = g_strdup("");
	} else if(csecs % 10 == 0) {
		csecs_s = g_strdup_printf(".%ld", csecs / 10);
	} else {
		csecs_s = g_strdup_printf(".%02ld", csecs);
	}

	retval = g_strdup_printf("%04d-%02d-%02dT%02d:%02d:%02d%s%s",
			time_dest.tm_year + 1900,
			time_dest.tm_mon + 1,
			time_dest.tm_mday,
			time_dest.tm_hour,
			time_dest.tm_min,
			time_dest.tm_sec,
			csecs_s,
			util_get_timezone_string());

	g_free(csecs_s);

	DEBUG_END();
	return retval;
}

gchar *util_date_string_from_timeval(struct timeval *time)
{
	struct timeval timeval_src;
	gchar *retval = NULL;
	time_t time_src;
	struct tm time_dest;

	DEBUG_BEGIN();

	if(time == NULL)
	{
		gettimeofday(&timeval_src, NULL);
		time_src = timeval_src.tv_sec;
	} else {
		time_src = time->tv_sec;
	}
	localtime_r(&time_src, &time_dest);

	retval = g_strdup_printf("%04d-%02d-%02d",
			time_dest.tm_year + 1900,
			time_dest.tm_mon + 1,
			time_dest.tm_mday);

	DEBUG_END();
	return retval;
}

gboolean util_timeval_from_xml_date_time_string(
		const gchar *string,
		struct timeval *time)
{
	const gchar *remainder;
	gchar *retval;
	struct tm time_dest;
	struct tm tz;

	guint csecs = 0;
	gchar csecs_c[2];
	csecs_c[1] = '\0';

	g_return_val_if_fail(string != NULL, FALSE);
	g_return_val_if_fail(time != NULL, FALSE);
	DEBUG_BEGIN();

	/* Initialize the values in the time_dest and tz */
	memset(&time_dest, 0, sizeof(struct tm));
	memset(&tz, 0, sizeof(struct tm));

	remainder = strptime(string, "%Y-%m-%dT%T", &time_dest);
	if(remainder == NULL)
	{
		g_warning("Incorrect time format: %s", string);
		return FALSE;
	}

	if(*remainder == '.')
	{
		remainder++;
		if(*remainder)
		{
			csecs_c[0] = *remainder;
			csecs = g_ascii_strtoull(csecs_c, NULL, 10) * 10L;
			remainder++;
			if(*remainder)
			{
				csecs_c[0] = *remainder;
				csecs += g_ascii_strtoull(csecs_c, NULL, 10);
				*remainder++;
			}
		}
	}

	time->tv_sec = my_timegm(&time_dest);
	time->tv_usec = csecs * 10000;

	if(settings_get_ignore_time_zones(_util_settings))
	{
		/* Ignore time zone data, if there is any */
		DEBUG_END();
		return TRUE;
	}

	/* See if there is time zone information (mktime always assumes that
	 * input is in local time) */
	if(*remainder == '+' || *remainder == '-')
	{
		if(*(remainder + 1) != '\0')
		{
			/* The time is in format <localtime>(+/-)<timezone> */
			retval = strptime(remainder + 1, "%H:%M", &tz);

			if(retval != NULL)
			{
				/* First, convert to UTC and then to local
				 * time */
				if(*remainder == '+')
				{
					time->tv_sec -= tz.tm_hour * 3600 +
						tz.tm_min * 60;
				} else {
					time->tv_sec += tz.tm_hour * 3600 +
						tz.tm_min * 60;
				}
				time->tv_sec -= timezone;
			}
		}
	} else if(*remainder == 'Z') {
		/* The time is represented as UTC */
		time->tv_sec -= timezone;
	}
	/* Otherwise assume that the data is in local time zone */

	DEBUG_END();
	return TRUE;
}

gint util_compare_timevals(struct timeval *time_1, struct timeval *time_2)
{
	g_return_val_if_fail(time_1 != NULL, 0);
	g_return_val_if_fail(time_2 != NULL, 0);

	DEBUG_BEGIN();

	if(time_1->tv_sec > time_2->tv_sec)
	{
		DEBUG_END();
		return 1;
	}

	if(time_1->tv_sec < time_2->tv_sec)
	{
		DEBUG_END();
		return -1;
	}

	if(time_1->tv_usec > time_2->tv_usec)
	{
		DEBUG_END();
		return 1;
	}

	if(time_2->tv_usec < time_2->tv_usec)
	{
		DEBUG_END();
		return 1;
	}

	DEBUG_END();
	return 0;
}

/*===========================================================================*
 * Private functions                                                         *
 *===========================================================================*/

static const gchar *util_get_timezone_string()
{
	glong hours;
	glong minutes;

	static gchar *tzstring = NULL;

	if(!tzstring)
	{
		tzset();
		/* The timezone variable is for a reason or another seconds *west* from UTC,
		 * so for example GMT+02:00 would have the timezone value -2 * 60 * 60,
		 * i.e., -7200 and so on */
		hours = labs(timezone / 3600L);
		minutes = labs((timezone % 60L) / 60L);
		if(timezone > 0)
		{
			tzstring = g_strdup_printf("-%02ld:%02ld", hours, minutes);
		} else {
			tzstring = g_strdup_printf("+%02ld:%02ld", hours, minutes);
		}
	}

	return tzstring;
}

time_t my_timegm(struct tm *tm)
{
	time_t retval;
	g_return_val_if_fail(tm != NULL, 0);
	const gchar *tz;

	tz = g_getenv("TZ");
	g_setenv("TZ", "", 1);
	tzset();
	retval = mktime(tm);
	if(tz)
	{
		g_setenv("TZ", tz, 1);
	} else {
		g_unsetenv("TZ");
	}
	tzset();
	return retval;
}
