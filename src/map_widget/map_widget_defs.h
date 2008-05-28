/*
 * This file is part of MapWidget
 *
 * Copyright (C) 2007 Pekka Rönkkö (pronkko@gmail.com).
 * 
 * Part of code is ripped from John Costigan's Maemo Mapper.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __MAP_WIDGET_DEFS_H__
#define __MAP_WIDGET_DEFS_H__

#define DEBUG_PRIVATE_MEMBERS() \
	g_debug(" \
		center_ratio: %d\n \
		zoom: %d\n \
		center.unitx: %d center.unity: %d\n \
		base_tilex: %d base_tiley: %d\n \
		offsetx: %d offsety: %d\n \
		screen_grids_halfwidth: %d screen_grids_halfheigth: %d\n \
		screen_width_pixels: %d screen_height_pixels: %d\n \
		focus.unitx: %d focus.unity: %d\n \
		focus_unitwidth: %d focus_unitheight: %d\n \
		min_center.unitx: %d min_center.unity: %d\n \
		max_center.unitx: %d max_center.unity: %d\n \
		world_size_tiles: %d\n \
		mylocation.latitude: %f\n \
		mylocation.longitude: %f\n \
		auto_center %d\n", \
		priv->center_ratio, \
		priv->zoom, \
		priv->center.unitx, \
		priv->center.unity, \
		priv->base_tilex, \
		priv->base_tiley, \
		priv->offsetx, \
		priv->offsety, \
		priv->screen_grids_halfwidth, \
		priv->screen_grids_halfheight, \
		priv->screen_width_pixels, \
		priv->screen_height_pixels, \
		priv->focus.unitx, priv->focus.unity, \
		priv->focus_unitwidth, \
		priv->focus_unitheight, \
		priv->min_center.unitx, priv->min_center.unity, \
		priv->max_center.unitx, priv->max_center.unity, \
		priv->world_size_tiles, \
		priv->mylocation.latitude, priv->mylocation.longitude,priv->auto_center)

/** Maemo Mapper definitions of units, tiles and coordinate transformations */

#define BOUND(x, a, b) { \
    if((x) < (a)) \
        (x) = (a); \
    else if((x) > (b)) \
        (x) = (b); \
}

#define MACRO_QUEUE_DRAW_AREA() \
    gtk_widget_queue_draw_area( \
            dmap, \
            0, 0, \
            priv->screen_width_pixels, \
            priv->screen_height_pixels)


/** MAX_ZOOM defines the largest map zoom level we will download.
 * (MAX_ZOOM - 1) is the largest map zoom level that the user can zoom to.
 */
#define MAX_ZOOM 16

#define PI   (3.14159265358979323846f)

#define WORLD_SIZE_UNITS (2 << (MAX_ZOOM + TILE_SIZE_P2))

#define TILE_SIZE_PIXELS (256)
#define TILE_SIZE_P2 (8)
#define BUF_WIDTH_TILES (4)
#define BUF_HEIGHT_TILES (3)
#define BUF_WIDTH_PIXELS (1024)
#define BUF_HEIGHT_PIXELS (768)

#define tile2grid(tile) ((tile) << 3)
#define grid2tile(grid) ((grid) >> 3)
#define tile2pixel(tile) ((tile) << 8)
#define pixel2tile(pixel) ((pixel) >> 8)
#define tile2unit(tile) ((tile) << (8 + priv->zoom))
#define unit2tile(unit) ((unit) >> (8 + priv->zoom))
#define tile2zunit(tile, zoom) ((tile) << (8 + zoom))
#define unit2ztile(unit, zoom) ((unit) >> (8 + zoom))

#define grid2pixel(grid) ((grid) << 5)
#define pixel2grid(pixel) ((pixel) >> 5)
#define grid2unit(grid) ((grid) << (5 + priv->zoom))
#define unit2grid(unit) ((unit) >> (5 + priv->zoom))

#define pixel2unit(pixel) ((pixel) << priv->zoom)
#define unit2pixel(pixel) ((pixel) >> priv->zoom)
#define pixel2zunit(pixel, zoom) ((pixel) << (zoom))

#define unit2bufx(unit) (unit2pixel(unit) - tile2pixel(priv->base_tilex))
#define bufx2unit(x) (pixel2unit(x) + tile2unit(priv->base_tilex))
#define unit2bufy(unit) (unit2pixel(unit) - tile2pixel(priv->base_tiley))
#define bufy2unit(y) (pixel2unit(y) + tile2unit(priv->base_tiley))

#define unit2x(unit) (unit2pixel(unit) - tile2pixel(priv->base_tilex) - priv->offsetx)
#define x2unit(x) (pixel2unit(x + priv->offsetx) + tile2unit(priv->base_tilex))
#define unit2y(unit) (unit2pixel(unit) - tile2pixel(priv->base_tiley) - priv->offsety)
#define y2unit(y) (pixel2unit(y + priv->offsety) + tile2unit(priv->base_tiley))

/*
#define leadx2unit() (_pos.unitx + (priv->lead_ratio) * pixel2unit(priv->vel_offsetx))
#define leady2unit() (_pos.unity + (0.6f*priv->lead_ratio)*pixel2unit(priv->vel_offsety))
*/

#define MACRO_RECALC_CENTER(center_unitx, center_unity) { \
             center_unitx = priv->center.unitx; \
             center_unity = priv->center.unity; \
}


#define MERCATOR_SPAN (-6.28318377773622f)
#define MERCATOR_TOP (3.14159188886811f)
#define latlon2unit(lat, lon, unitx, unity) { \
    gfloat tmp; \
    unitx = (lon + 180.f) * (WORLD_SIZE_UNITS / 360.f) + 0.5f; \
    tmp = sinf(lat * (PI / 180.f)); \
    unity = 0.5f + (WORLD_SIZE_UNITS / MERCATOR_SPAN) \
        * (logf((1.f + tmp) / (1.f - tmp)) * 0.5f - MERCATOR_TOP); \
}

#define unit2latlon(unitx, unity, lat, lon) { \
    (lon) = ((unitx) * (360.f / WORLD_SIZE_UNITS)) - 180.f; \
    (lat) = (360.f * (atanf(expf(((unity) \
                                  * (MERCATOR_SPAN / WORLD_SIZE_UNITS)) \
                     + MERCATOR_TOP)))) * (1.f / PI) - 90.f; \
}

#define MACRO_RECALC_OFFSET() { \
    priv->offsetx = grid2pixel( \
            unit2grid(priv->center.unitx) \
            - priv->screen_grids_halfwidth \
            - tile2grid(priv->base_tilex)); \
    priv->offsety = grid2pixel( \
            unit2grid(priv->center.unity) \
            - priv->screen_grids_halfheight \
            - tile2grid(priv->base_tiley)); \
}

#define MACRO_RECALC_FOCUS_BASE() { \
    priv->focus.unitx = x2unit(priv->screen_width_pixels * priv->center_ratio / 20); \
    priv->focus.unity = y2unit(priv->screen_height_pixels * priv->center_ratio / 20); \
}

#define MACRO_RECALC_FOCUS_SIZE() { \
    priv->focus_unitwidth = pixel2unit( \
            (10 - priv->center_ratio) * priv->screen_width_pixels / 10); \
    priv->focus_unitheight = pixel2unit( \
            (10 - priv->center_ratio) * priv->screen_height_pixels / 10); \
}

#define MACRO_RECALC_CENTER_BOUNDS() { \
  priv->min_center.unitx = pixel2unit(grid2pixel(priv->screen_grids_halfwidth)); \
  priv->min_center.unity = pixel2unit(grid2pixel(priv->screen_grids_halfheight)); \
  priv->max_center.unitx = WORLD_SIZE_UNITS-grid2unit(priv->screen_grids_halfwidth) - 1;\
  priv->max_center.unity = WORLD_SIZE_UNITS-grid2unit(priv->screen_grids_halfheight)- 1;\
}

#define EARTH_RADIUS (3440.06479f)


#define MAP_ORIENTATION_NORTH   0.0
#define MAP_ORIENTATION_EAST   90.0
#define MAP_ORIENTATION_SOUTH 180.0
#define MAP_ORIENTATION_WEST  270.0

#define DEFAULT_CENTER_RATIO 7
#define DEFAULT_ZOOM 8.0
#define DEFAULT_CENTER_LATITUDE 65.013422
#define DEFAULT_CENTER_LONGITUDE 25.40923 
#define DEFAULT_MAP_TYPE "Open Street"
#define DEFAULT_ORIENTATION MAP_ORIENTATION_NORTH

#define COLOR_BLACK 0		/* BLACK */
#define COLOR_BLUE 1
#define COLOR_RED 2
#define COLOR_GREEN 3


/* defines for poi icons */

#define WIKI_W "/usr/share/cityguide/pictures/poi/icon_wikipedia_26_a.png"

#define PEOPLE_GREEN "/usr/share/cityguide/pictures/poi/icon_people_green_26.png"
#define PEOPLE_BLACK "/usr/share/cityguide/pictures/poi/icon_people_black_26.png"
#define PEOPLE_RED "/usr/share/cityguide/pictures/poi/icon_people_red_26.png"
#define PEOPLE_BLUE "/usr/share/cityguide/pictures/poi/icon_people_blue_26.png"
#define PEOPLE_YELLOW "/usr/share/cityguide/pictures/poi/icon_people_yellow_26.png"

#define PERSON_GREEN "/usr/share/cityguide/pictures/poi/icon_person_green_26.png"
#define PERSON_BLACK "/usr/share/cityguide/pictures/poi/icon_person_black_26.png"
#define PERSON_RED "/usr/share/cityguide/pictures/poi/icon_person_red_26.png"
#define PERSON_BLUE "/usr/share/cityguide/pictures/poi/icon_person_blue_26.png"
#define PERSON_YELLOW "/usr/share/cityguide/pictures/poi/icon_person_yellow_26.png"









/** Maemo Mapper definitions end */

#endif
