#ifndef _LOCATION_DISTANCE_UTILS_FIX_H
#define _LOCATION_DISTANCE_UTILS_FIX_H

/*
 * Following file has a bug that prevents it from being used:
 * #include <location/location-distance-utils.h>
 *
 * Please see https://bugs.maemo.org/show_bug.cgi?id=2949
 *
 * The correct prototype is defined here
 */
double location_distance_between(
		double latitude_s,
		double longitude_s,
		double latitude_f,
		double longitude_f);

#endif /* _LOCATION_DISTANCE_UTILS_FIX_H */
