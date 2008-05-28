/*
 * This file is part of MapWidget
 *
 * Copyright (C) 2007 Pekka Rönkkö (pronkko@gmail.com).
 *
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

#ifndef __MAP_WIDGET_H
#define __MAP_WIDGET_H

#include <gtk/gtk.h>
#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib.h>
#include <dbus/dbus.h>
#include <libosso.h>


#include "map_widget_defs.h"




#define TYPE_MAP_WIDGET			(map_widget_get_type ())
#define MAP_WIDGET(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_MAP_WIDGET, MapWidget))
#define MAP_WIDGET_CLASS(obj)		(G_TYPE_CHECK_CLASS_CAST ((obj), TYPE_MAP_WIDGET, MapWidgetClass))
#define IS_MAP_WIDGET(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_MAP_WIDGET))
#define IS_MAP_WIDGET_CLASS(obj)	(G_TYPE_CHECK_CLASS_TYPE ((obj), TYPE_MAP_WIDGET))
#define MAP_WIDGET_GET_CLASS		(G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_MAP_WIDGET, MapWidgetClass))


typedef struct _MapWidget MapWidget;
struct _MapWidget
{
	GtkWidget widget;
	/* < private > */
};


typedef struct _MapWidgetClass	MapWidgetClass;
struct _MapWidgetClass
{
	GtkWidgetClass parent_class;
};


typedef struct _MapPoint MapPoint;
struct _MapPoint 
{
  gfloat latitude;
  gfloat longitude;
};



typedef struct _Point Point;
struct _Point 
{
    guint unitx;
    guint unity;
};


/* type of own callback function */
typedef void (mw_callback)(GtkWidget *dmap, gpointer data);



typedef struct _MW_cb MW_cb;
struct _MW_cb 
{
    mw_callback* cb_func_name;
    gboolean do_cb_func;
    gpointer data;
    
};

typedef struct _PoiInfo PoiInfo;

struct _PoiInfo {
    gchar *lat;
    gchar *lng;
    gchar *name;
    gboolean show_name;
    gchar *icon;
    gchar *text;
    guint min_zoomlevel;
    guint max_zoomlevel;
};


typedef struct _BuddyInfo BuddyInfo;

struct _BuddyInfo {
    gchar *lat;
    gchar *lng;
    gchar *name;
    gboolean show_name;
    gchar *icon;
    gchar *text;
    guint min_zoomlevel;
    guint max_zoomlevel;
};

/* Private members */
typedef struct _MapWidgetPrivate MapWidgetPrivate;

struct _MapWidgetPrivate
{
    GdkPixmap *pixmap;  
    osso_context_t *osso_context;
    gchar *maptype;

    gboolean dragging;

    /** VARIABLES FOR MAINTAINING STATE OF THE CURRENT VIEW. */

    /** Configuration */
    guint center_ratio;

    /** The "zoom" level defines the resolution of a pixel, from 0 to MAX_ZOOM.
      * Each pixel in the current view is exactly (1 << _zoom) "units" wide. */
    guint zoom; /* zoom level, from 0 to MAX_ZOOM. */

    gfloat zoom_float;
    
    Point center; /* current center location, X. */

    /** The "base tile" is the upper-left tile in the pixmap. */
    guint base_tilex;
    guint base_tiley;

    /** The "offset" defines the upper-left corner of the visible portion of the
        * 1024x768 pixmap. */
    guint offsetx;
    guint offsety;

    /** CACHED SCREEN INFORMATION THAT IS DEPENDENT ON THE CURRENT VIEW. */
    guint screen_grids_halfwidth;
    guint screen_grids_halfheight;
    guint screen_width_pixels;
    guint screen_height_pixels;
    Point focus;
    guint focus_unitwidth;
    guint focus_unitheight;
    Point min_center;
    Point max_center;
    guint world_size_tiles;
    MapPoint mylocation;

    gfloat current_orientation;


    GdkGC *gc_mark;
    GdkColor color_mark;
    guint draw_line_width;

     
    GPtrArray *static_pois;
    gboolean show_pois;

    GPtrArray *buddies;
    gboolean show_buddies;

    gboolean show_current_place;
    gchar* current_place_icon;

    gboolean auto_center;

    /* DENSITY */
    void (* draw_tile_overlay) (GdkDrawable *drawble, gint tilex, gint tiley, gint zoom, gint offsetx, gint offsety);
    /*screen tap*/
    /* Poi info click */
    void (* screen_clicked_cb) (GtkWidget *dmap, gint x, gint y, gfloat lat, gfloat lon);


    /*route*/
    GPtrArray *route;
    gboolean show_route;

    /*route*/
    GPtrArray *track;
    gboolean show_track;
    gboolean track_not_empty;


    guint track_route_tail_length;


    GPtrArray *cb_functions_level_one;
    GPtrArray *cb_functions_level_two;
    GPtrArray *cb_functions_level_three;


};

/**
 * Defines a rectangular map area by specifying the North East and the
 * South West corner.
 *
 * TODO: Should there be a restriction, so that
 *          north_east.latitude  > south_west.latitude
 *          north_east.longitude > south_west.longitude
 *
 * TODO: Should this be a class of its own, with its own header file?
 * TODO: "bounding box" expression usually used without orientation.
 */
typedef struct {
  MapPoint north_east;
  MapPoint south_west;
  gfloat   orientation;
} MapArea;


 
/* DENSITY */
void map_widget_set_tile_overlay(GtkWidget *dmap, void *overlay_cb);


/**
 * Creates a base to new map widget.
 */

GtkWidget *map_widget_create (void);
/**
 * Makes a new map widget that has the default center point, the
 * default zoom level, the default map type, and the default
 * orientation.
 */

void map_widget_new_with_defaults(GtkWidget *dmap,osso_context_t *_osso);
/**
 * Creates a new map widget that is initialized to a given center
 * point, zoom level, orientation, and the default map type.
 *
 * @param center      The resulting map widget uses a deep copy of this
 *                    point. Read only. Non-null.
 * @param zoom        The resulting map widget uses this zoom level.
 *		      1.0 = whole earth, 17.0 = nearest.
 * @param orientation The orientation of the map.
 */
void map_widget_new_from_center_zoom (GtkWidget *dmap, const MapPoint* center,
					    gfloat zoom,
					    gfloat orientation,osso_context_t *_osso);


/**
 * Creates a new map widget that is initialized to a given center
 * point, zoom level, orientation and map type.
 *
 * @param center The resulting map widget uses a deep copy of this
 *               point. Read only. Non-null.
 * @param zoom   The resulting map widget uses this zoom level.
 *		      1.0 = whole earth, 17.0 = nearest.
 * @param type   The resulting map widget uses this map type.
 * @param orientation The orientation of the map.
 */
void map_widget_new_from_center_zoom_type (GtkWidget *dmap, const MapPoint* center,
						 gfloat zoom,
						 gfloat orientation,
						 gchar* type,osso_context_t *_osso);


gboolean map_widget_configure_event(GtkWidget *widget, GdkEventConfigure *event);

/**
 * Change maptype. 
 *
 */

void map_widget_change_maptype(GtkWidget *dmap,gchar* new_maptype);
/**
 * Set new zoom and draw map with new zoom level. 
 *
 * @param zoom  The map widget uses this zoom level.
 *		      1.0 = whole earth, 17.0 = nearest.
 */
void map_widget_set_zoom(GtkWidget *dmap, gfloat zoom);



/**
 * Returns the distance, in meters, from point to point. 
 *
 * @param one A non-null point. Read only.
 * @param two A non-null point. Read only.
 */
gfloat map_widget_calculate_distance(const MapPoint *src, const MapPoint *dest);
/**
 * Returns the default map type. Whatever that is. Read only.
 */
const gchar* map_widget_get_default_map_type(void);
/**
 * Returns the default zoom level. Whatever that is. Read only.
 */
const gfloat map_widget_get_default_zoom_level (void);

/**
 * Returns the default map center point. Whatever that is. Read only.
 */
const MapPoint* map_widget_get_default_center (void);

/**
 * Returns the default orientation. MAP_ORIENTATION_NORTH if
 * orientation is not supported.
 */
const gfloat map_widget_get_default_orientation (void);


/**
 * Returns the current zoom level. Whatever that is. Read only.
 */
guint map_widget_get_current_zoom_level (GtkWidget *dmap);


void map_widget_init_dbus(GtkWidget *dmap, osso_context_t *osso_conn);


gboolean map_widget_load_mapdata_defaults(GtkWidget *dmap);

/**
 * Center an onscreen pixel
 *
 * @param dmap Pointer to MapWidget
 * @param center_x X coordinate of the pixel to center
 * @param center_y Y coordinate of the pixel to center
 */
void map_widget_center_onscreen_coords(
		GtkWidget *dmap,
		gint center_x,
		gint center_y);
 
/**
 *
 * Drawing functions
 */

/* Draw your current place to map. */
void map_widget_draw_current_place(GtkWidget * dmap);
void map_widget_set_is_current_place_shown(GtkWidget * dmap,
					   gboolean show_current_place);


/* Draw arc to given coordidates. */
void map_widget_draw_arc_to_coords(GtkWidget *dmap, 
		MapPoint* point_to_draw, gint width, gint height, gboolean filled);

/* Draw rectangle to given coordidates. */
void map_widget_draw_rectangle_to_coords(GtkWidget *dmap, 
		MapPoint* point_to_draw, gint width, gint height, gboolean filled);

/* Draw icon to given coordidates. */
void map_widget_draw_icon_to_coords(GtkWidget *dmap,
		MapPoint* point_to_draw, gchar* icon);


/* Draw pois to map if static_poi_array exist and show_poi is true */
void map_widget_draw_static_info(GtkWidget * dmap);


/* Draw buddies to map if buddies_array exist and show_buddies is true */
void map_widget_draw_buddies(GtkWidget * dmap);


/* Set boolean to show current place */
void map_widget_set_is_current_place_shown(GtkWidget * dmap,
					   gboolean show_current_place);

/* Set GPtrArray which contains PoiInfo structs. */

void map_widget_set_static_poi_array(GtkWidget * dmap, GPtrArray * static_pois,
			      gboolean show_pois);

/* Set GPtrArray which contains PoiInfo structs. */
GPtrArray * map_widget_get_static_poi_array(GtkWidget *dmap);


/* Set GPtrArray which contains BuddyInfo structs. 
*/
void map_widget_set_buddies_array(GtkWidget * dmap, GPtrArray * buddies,
			      gboolean show_buddies);

/* Set GPtrArray which contains buddy structs. */
GPtrArray * map_widget_get_buddies_array(GtkWidget *dmap);

/* Get supported maptypes from maptile-loader */
gboolean map_widget_get_maptypes(GtkWidget *dmap, gchar ***maptypes);
 

/* Set new current location */
void map_widget_set_current_location(GtkWidget *dmap, MapPoint* new_center);

/* Set icon for current location */
void map_widget_set_current_location_icon(GtkWidget *dmap, gchar* current_place_icon);

/* Set auto_center on/off */
void map_widget_set_auto_center(GtkWidget *dmap, gboolean auto_center);

/* get auto_center status */
gboolean map_widget_get_auto_center_status(GtkWidget *dmap);


/* draw a bubble to map / not ready yet */
void map_widget_draw_bubble(GtkWidget *dmap,MapPoint* bubble_point);


GdkGC* get_color(GtkWidget *dmap, guint color);

/* Set GPtrArray which contains MW_cb structs.
 
 * @param dmap		Map Widget
 * @param *cb_funcs	GPtrArray that contains callback functions.
 * 			callback function must be type:
 *			void mw_callback(GtkWidget *dmap);
 * 
 *
 * @param level 	Drawing level. 1 will be drawing first. Before anything else
			and levels 2 and 3 after current place, pois and buddies are drawn  */
void map_widget_set_cb_func_array(GtkWidget *dmap, GPtrArray *cb_funcs, guint level);


/* get GPtrArray which contains MW_cb structs.
 
 * @param dmap		Map Widget
 * @param level 	Drawing level.
 
  Returns GPtrArray which contains MW_cb structs, in requested level.*/

GPtrArray* map_widget_get_cb_func_array(GtkWidget *dmap, guint level);
/*screen tap*/
void map_widget_set_screen_clicked_cb(GtkWidget *dmap, void *poi_info_clicked_cb);

/* Set route GPtrArray which contains MapPoint structs. */

void map_widget_set_route_array(GtkWidget * dmap, GPtrArray * route,
			      gboolean show_route);

void map_widget_show_track(GtkWidget *dmap, gboolean show_route);

void map_widget_add_point_to_track(GtkWidget *dmap, MapPoint *track_point);

void map_widget_set_track_tail_length(GtkWidget *dmap, guint track_tail_length);

void map_widget_clear_track(GtkWidget *dmap);

void map_widget_show_location(GtkWidget *dmap, MapPoint* location);


#endif
