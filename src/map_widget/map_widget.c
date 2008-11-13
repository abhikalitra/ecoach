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

#include <gtk/gtk.h>
#include <libosso.h>
#include <stdlib.h>
#include <math.h>
#include "map_widget.h"

#ifdef g_debug
#undef g_debug
#endif
#define g_debug(...)

/*screen tap*/
void  map_widget_screen_click(GtkWidget *dmap, guint x, guint y);

static void map_widget_do_callbacks_first(GtkWidget *dmap);
static void map_widget_do_callbacks_second(GtkWidget *dmap);
static void map_widget_do_callbacks_third(GtkWidget *dmap);

const gchar * MAP_WIDGET_FW_DBUS_SERVICE = "org.cityguide.maptileloader";
const gchar * MAP_WIDGET_FW_DBUS_OBJECT_PATH = "/org/cityguide/maptileloader";
const gchar * MAP_WIDGET_FW_DBUS_INTERFACE = "org.cityguide.maptileloader";
const gchar * MAP_WIDGET_FW_FUNCTION_MAPTILE_REQUEST = "MaptileLoaderGetMaptile";
const gchar * MAP_WIDGET_FW_FUNCTION_GET_SUPPORTED_MAPS = "MaptileLoaderGetSupportedMaps";
const gchar * MAP_WIDGET_FW_FUNCTION_SET_REPOFILE = "MaptileLoaderSetRepofile";



#define MAP_WIDGET_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TYPE_MAP_WIDGET, MapWidgetPrivate))

static void map_widget_class_init     	(MapWidgetClass *klass);
static void map_widget_init           	(MapWidget      *dmap);
static void map_widget_realize        	(GtkWidget         *widget);
static gboolean map_widget_expose	(GtkWidget *widget, GdkEventExpose *event);
static void map_widget_size_allocate  	(GtkWidget         *widget,
                                         GtkAllocation     *allocation);
static void map_widget_send_configure 	(MapWidget     *dmap);



static gboolean map_widget_button_press(GtkWidget *dmap, GdkEventButton *event);
static gboolean map_widget_button_release(GtkWidget *dmap, GdkEventButton *event);
static gboolean map_widget_motion_notify(GtkWidget *dmap, GdkEventMotion *event);

static void map_widget_center_unit(GtkWidget *widget, guint new_center_unitx, guint new_center_unity);
static void map_widget_render_tile(GtkWidget *dmap, guint tilex, guint tiley, guint destx, guint desty, gboolean fast_fail);
static gchar *map_widget_get_maptile(GtkWidget *dmap, guint tilex, guint tiley, guint zoom);

static GdkPixbuf *map_widget_pixbuf_trim(GdkPixbuf* pixbuf);
static void map_widget_pixbuf_scale_inplace(GdkPixbuf* pixbuf, guint ratio_p2, guint src_x, guint src_y);

static void map_widget_force_redraw(GtkWidget *dmap);
static guint map_widget_float_to_integer(gfloat value_float);
static guint map_widget_convert_zoom(guint zoom);
static void map_widget_draw_route_track(GtkWidget *dmap, GPtrArray *track_route);


GPtrArray *maptile_key_cache = NULL;
GHashTable *maptile_tile_cache = NULL;

static gboolean
dmap_cb_configure(GtkWidget *widget, GdkEventConfigure *event)
{
	map_widget_configure_event(widget, event);
	return TRUE;
}
/**
 * Class and instance initialization
 */
GType
map_widget_get_type (void)
{
    static GType map_widget_type = 0;

    if (!map_widget_type)
    {
        static const GTypeInfo map_widget_info =
        {
            sizeof (MapWidgetClass),
            NULL,           /* base_init */
            NULL,           /* base_finalize */
            (GClassInitFunc) map_widget_class_init,
            NULL,           /* class_finalize */
            NULL,           /* class_data */
            sizeof (MapWidget),
            0,              /* n_preallocs */
            (GInstanceInitFunc) map_widget_init,
        };

        map_widget_type =
            g_type_register_static (GTK_TYPE_WIDGET, "MapWidget",
                                    &map_widget_info, 0);
    }

    return map_widget_type;
}


static void
map_widget_class_init (MapWidgetClass *class)
{
    GtkWidgetClass *widget_class;
    widget_class = GTK_WIDGET_CLASS (class);

    widget_class->realize = map_widget_realize;
    widget_class->expose_event = map_widget_expose;
    widget_class->size_allocate = map_widget_size_allocate;

    /* Button press and mouse dragging events */
    widget_class->button_press_event = map_widget_button_press;
    widget_class->button_release_event = map_widget_button_release;
    widget_class->motion_notify_event = map_widget_motion_notify;

    g_type_class_add_private (widget_class, sizeof (MapWidgetPrivate));

}


static void
map_widget_init (MapWidget *dmap)
{

    MapWidgetPrivate *priv = MAP_WIDGET_GET_PRIVATE (dmap);

    g_debug("map_widget_init\n");
    priv->pixmap = NULL;
    priv->osso_context = NULL;
    priv->maptype = g_strdup("Open Street");

    priv->dragging = FALSE;

    /** VARIABLES FOR MAINTAINING STATE OF THE CURRENT VIEW. */

    /** Configuration */
    priv->center_ratio = 7;
    /** The "zoom" level defines the resolution of a pixel, from 0 to MAX_ZOOM.
        * Each pixel in the current view is exactly (1 << _zoom) "units" wide. */
    priv->zoom = 1; /* zoom level, from 0 to MAX_ZOOM. */
    priv->zoom_float = 1.0;


    priv->center.unitx = -1; priv->center.unity = -1; /* current center location, X. Y */

    /** The "base tile" is the upper-left tile in the pixmap. */
    priv->base_tilex = -5;
    priv->base_tiley = -5;

    /** The "offset" defines the upper-left corner of the visible portion of the
        * 1024x768 pixmap. */
    priv->offsetx = -1;
    priv->offsety = -1;

    /** CACHED SCREEN INFORMATION THAT IS DEPENDENT ON THE CURRENT VIEW. */
    priv->screen_grids_halfwidth = 0;
    priv->screen_grids_halfheight = 0;
    priv->screen_width_pixels = 0;
    priv->screen_height_pixels = 0;
    priv->focus.unitx = -1; 
    priv->focus.unity = -1;
    priv->focus_unitwidth = 0;
    priv->focus_unitheight = 0;
    priv->min_center.unitx = -1; priv->min_center.unity = -1;
    priv->max_center.unitx = -1; priv->max_center.unity = -1;
    priv->world_size_tiles = -1;
    priv->mylocation.latitude = 0.0f; 
    priv->mylocation.longitude = 0.0f;


    priv->current_orientation = 0.0f;

    priv->gc_mark = NULL;
    priv->color_mark.pixel = 0; priv->color_mark.red = 0; priv->color_mark.green = 0; priv->color_mark.blue = 0;
    priv->draw_line_width = 5;




    priv->static_pois = NULL;
    priv->show_pois = FALSE;

    priv->buddies = NULL;
    priv->show_buddies = FALSE;

    priv->show_current_place = TRUE;
    priv->current_place_icon = NULL;

    priv->auto_center = FALSE;

    /* Level one will be drawn first */
    priv->cb_functions_level_one = NULL;
    priv->cb_functions_level_two = NULL;
    priv->cb_functions_level_three = NULL;

    /* DENSITY */
    priv->draw_tile_overlay = NULL;

    priv->screen_clicked_cb = NULL;

    /*route*/
    priv->route = NULL;
    priv->show_route = FALSE;

    /*track*/
    priv->track = NULL;
    priv->show_track = FALSE;
    priv->track_not_empty = FALSE;
    priv->track = g_ptr_array_new();
    priv->track_route_tail_length = 0;

    

    DEBUG_PRIVATE_MEMBERS();

    gtk_widget_add_events (GTK_WIDGET (dmap),
                           GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
                           GDK_POINTER_MOTION_MASK);
}

/* DENSITY */
void map_widget_set_tile_overlay(GtkWidget *dmap, void *overlay_cb)
{
    MapWidgetPrivate *priv;

    priv = MAP_WIDGET_GET_PRIVATE(dmap);
    g_return_if_fail(IS_MAP_WIDGET(dmap));

    priv->draw_tile_overlay = overlay_cb;

}

/*screen tap*/
/* Poi or buddies info clicked */
void map_widget_set_screen_clicked_cb(GtkWidget *dmap, void *screen_clicked_cb)
{
    MapWidgetPrivate *priv;

    priv = MAP_WIDGET_GET_PRIVATE(dmap);
    g_return_if_fail(IS_MAP_WIDGET(dmap));

    priv->screen_clicked_cb = screen_clicked_cb;
}


GtkWidget *map_widget_create(void)
{
    return g_object_new (TYPE_MAP_WIDGET, NULL);
}


void map_widget_new_with_defaults(GtkWidget *dmap,osso_context_t *_osso)
{


    	MapWidgetPrivate *priv;
	gfloat center_lat, center_lon;

    	priv = MAP_WIDGET_GET_PRIVATE(dmap);
    	g_return_if_fail(IS_MAP_WIDGET(dmap));


        gtk_widget_realize(dmap);

	map_widget_init_dbus(dmap, _osso);

	g_signal_connect(G_OBJECT(dmap), "configure_event",
			G_CALLBACK(dmap_cb_configure), NULL);

	priv->maptype = DEFAULT_MAP_TYPE;
	priv->center_ratio = DEFAULT_CENTER_RATIO;

    	priv->current_orientation = DEFAULT_ORIENTATION;


    	center_lat = DEFAULT_CENTER_LATITUDE;
    	center_lon = DEFAULT_CENTER_LONGITUDE;

	priv->zoom = map_widget_convert_zoom( map_widget_float_to_integer(DEFAULT_ZOOM) );

    	latlon2unit(center_lat, center_lon, priv->center.unitx, priv->center.unity);


}

void map_widget_new_from_center_zoom (GtkWidget *dmap, const MapPoint* center,
					    gfloat zoom,
					    gfloat orientation,osso_context_t *_osso)
{
    	MapWidgetPrivate *priv;
	gfloat center_lat, center_lon;


    	priv = MAP_WIDGET_GET_PRIVATE(dmap);
    	g_return_if_fail(IS_MAP_WIDGET(dmap));


        gtk_widget_realize(dmap);

	map_widget_init_dbus(dmap, _osso);



	g_signal_connect(G_OBJECT(dmap), "configure_event",
			G_CALLBACK(dmap_cb_configure), NULL);
	priv->maptype = DEFAULT_MAP_TYPE;
    	priv->current_orientation = DEFAULT_ORIENTATION;

    
    	priv->mylocation.latitude = center->latitude;
    	priv->mylocation.longitude = center->longitude;


	center_lat = priv->mylocation.latitude;
    	center_lon = priv->mylocation.longitude;

	priv->zoom_float = zoom;
        priv->zoom = map_widget_convert_zoom( map_widget_float_to_integer(zoom) );

    	latlon2unit(center_lat, center_lon, priv->center.unitx, priv->center.unity);

}
void map_widget_new_from_center_zoom_type (GtkWidget *dmap, const MapPoint* center,
						 gfloat zoom,
						 gfloat orientation,
						 gchar* type,osso_context_t *_osso)
{
    	MapWidgetPrivate *priv;
	gfloat center_lat, center_lon;

	priv = MAP_WIDGET_GET_PRIVATE(dmap);
    	g_return_if_fail(IS_MAP_WIDGET(dmap));




        gtk_widget_realize(dmap);
	
	map_widget_init_dbus(dmap, _osso);

    	

	g_signal_connect(G_OBJECT(dmap), "configure_event",
			G_CALLBACK(dmap_cb_configure), NULL);
	priv->maptype = DEFAULT_MAP_TYPE;
    	priv->current_orientation = DEFAULT_ORIENTATION;

    
    	priv->mylocation.latitude = center->latitude;
    	priv->mylocation.longitude = center->longitude;


	center_lat = priv->mylocation.latitude;
    	center_lon = priv->mylocation.longitude;

	priv->zoom_float = zoom;
        priv->zoom = map_widget_convert_zoom(map_widget_float_to_integer(zoom) );

	priv->maptype = type;

    	latlon2unit(center_lat, center_lon, priv->center.unitx, priv->center.unity);


}

/**
 * Widget creation, size change and expose event handling
 */

static void
map_widget_realize(GtkWidget *widget)
{
    g_debug("Realize event!\n");

    MapWidget *dmap;
    GdkWindowAttr attributes;
    gint attributes_mask;

    g_return_if_fail (IS_MAP_WIDGET (widget));

    dmap = MAP_WIDGET (widget);
    GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.x = widget->allocation.x;
    attributes.y = widget->allocation.y;
    attributes.width = widget->allocation.width;
    attributes.height = widget->allocation.height;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.visual = gtk_widget_get_visual (widget);
    attributes.colormap = gtk_widget_get_colormap (widget);
    attributes.event_mask = gtk_widget_get_events (widget) | GDK_EXPOSURE_MASK;

    attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

    widget->window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
    gdk_window_set_user_data (widget->window, dmap);

    widget->style = gtk_style_attach (widget->style, widget->window);
    gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);


    map_widget_send_configure (MAP_WIDGET (widget));

}

static void
map_widget_size_allocate (GtkWidget     *widget,
                             GtkAllocation *allocation)
{
    g_debug("Size allocate!\n");

    g_return_if_fail (IS_MAP_WIDGET (widget));
    g_return_if_fail (allocation != NULL);

    widget->allocation = *allocation;

    if (GTK_WIDGET_REALIZED (widget))
    {
        gdk_window_move_resize (widget->window,
                                allocation->x, allocation->y,
                                allocation->width, allocation->height);

        map_widget_send_configure (MAP_WIDGET (widget));
    }

}

static gboolean map_widget_expose(GtkWidget *dmap, GdkEventExpose *event)
{
    g_debug("Expose event!\n");

    MapWidgetPrivate *priv;
    g_return_val_if_fail(IS_MAP_WIDGET(dmap), FALSE);

    priv = MAP_WIDGET_GET_PRIVATE(dmap);

    gdk_draw_drawable(
            dmap->window,
            priv->gc_mark,
            priv->pixmap,
            event->area.x + priv->offsetx, event->area.y + priv->offsety,
            event->area.x, event->area.y,
            event->area.width, event->area.height);


   map_widget_do_callbacks_first(dmap);



   if (priv->show_pois)
	     map_widget_draw_static_info(dmap);
/*route*/
   if (priv->show_route)
	     map_widget_draw_route_track(dmap, priv->route);

/*track*/
   if (priv->show_track && priv->track_not_empty == TRUE)
	     map_widget_draw_route_track(dmap, priv->track);


   if (priv->show_buddies)
	     map_widget_draw_buddies(dmap);

   map_widget_do_callbacks_second(dmap);

   if (priv->show_current_place)
     map_widget_draw_current_place(dmap);

   map_widget_do_callbacks_third(dmap); 

   return FALSE;
}


static void
map_widget_send_configure (MapWidget *dmap)
{
    g_debug("Send configure!\n");

    GtkWidget *widget;
    GdkEvent *event = gdk_event_new (GDK_CONFIGURE);

    widget = GTK_WIDGET (dmap);

    event->configure.window = g_object_ref (widget->window);
    event->configure.send_event = TRUE;
    event->configure.x = widget->allocation.x;
    event->configure.y = widget->allocation.y;
    event->configure.width = widget->allocation.width;
    event->configure.height = widget->allocation.height;

    gtk_widget_event (widget, event);
    gdk_event_free (event);
}

/** 
 * Mouse and key event handling 
 *
 */
static gboolean
map_widget_button_press(GtkWidget *dmap, GdkEventButton *event)
{
    MapWidgetPrivate *priv;
    g_return_val_if_fail(IS_MAP_WIDGET(dmap), FALSE);

    priv = MAP_WIDGET_GET_PRIVATE(dmap);
    priv->dragging = TRUE;

    g_debug("Button pressed, x=%lf, y=%lf!\n", event->x, event->y);

    /*screen tap*/ // FIX moved to press
    if (priv->screen_clicked_cb)
        map_widget_screen_click(dmap,event->x, event->y);
    return FALSE;
}

static gboolean
map_widget_button_release(GtkWidget *dmap, GdkEventButton *event)
{
    MapWidgetPrivate *priv;

    g_return_val_if_fail(IS_MAP_WIDGET(dmap), FALSE);

    g_debug("Button released, event->x=%lf, event->y=%lf\n", event->x, event->y);
    


    priv = MAP_WIDGET_GET_PRIVATE(dmap);
    priv->dragging = FALSE;

    map_widget_center_unit(
            dmap,
            x2unit((gint)(event->x + 0.5)),
            y2unit((gint)(event->y + 0.5)));

//    g_debug("12Button released, event->x=%lf, event->y=%lf\n", event->x, event->y);
//    DEBUG_PRIVATE_MEMBERS();
    return FALSE;
}

static gboolean
map_widget_motion_notify(GtkWidget *dmap, GdkEventMotion *event)
{
    MapWidgetPrivate *priv;
    g_return_val_if_fail(IS_MAP_WIDGET(dmap), FALSE);

    priv = MAP_WIDGET_GET_PRIVATE(dmap);
    if ( priv->dragging )
    {
        g_debug("Dragging!\n");
    }

    return FALSE;
}

void map_widget_center_onscreen_coords(
		GtkWidget *dmap,
		gint center_x,
		gint center_y)
{
	g_return_if_fail(dmap != NULL);
	MapWidgetPrivate *priv = MAP_WIDGET_GET_PRIVATE(dmap);
	map_widget_center_unit(
			dmap,
			x2unit(center_x),
			y2unit(center_y));

}

/**
 * Coordinate and unit transformation helper functions
 */
static void
map_widget_center_unit(GtkWidget *dmap, guint new_center_unitx, guint new_center_unity)
{
    g_debug("map_widget_center_unit\n");
    gint new_base_tilex, new_base_tiley;
    guint new_x, new_y;
    guint j, k, base_new_x, base_old_x, old_x, old_y, iox, ioy;

    MapWidgetPrivate *priv;
    g_return_if_fail(IS_MAP_WIDGET(dmap));

    priv = MAP_WIDGET_GET_PRIVATE(dmap);


    g_debug("Old center unitx: %d, unity: %d\n", priv->center.unitx, priv->center.unity);

    /* Assure that _center.unitx/y are bounded. */
    BOUND(new_center_unitx, priv->min_center.unitx, priv->max_center.unitx);
    BOUND(new_center_unity, priv->min_center.unity, priv->max_center.unity);

    priv->center.unitx = new_center_unitx;
    priv->center.unity = new_center_unity;
    g_debug("New center unitx: %d, unity: %d\n", priv->center.unitx, priv->center.unity);


    new_base_tilex = grid2tile((gint)pixel2grid(
            (gint)unit2pixel((gint)priv->center.unitx))
            - (gint)priv->screen_grids_halfwidth);
    new_base_tiley = grid2tile(pixel2grid(
            unit2pixel(priv->center.unity))
            - priv->screen_grids_halfheight);

    /* Same zoom level, so it's likely that we can reuse some of the old
    * buffer's pixels. */

    if(new_base_tilex != priv->base_tilex || new_base_tiley != priv->base_tiley)
    {
        /* If copying from old parts to new parts, we need to make sure we
        * don't overwrite the old parts when copying, so set up new_x,
        * new_y, old_x, old_y, iox, and ioy with that in mind. */
        if(new_base_tiley < priv->base_tiley)
        {
            /* New is lower than old - start at bottom and go up. */
            new_y = BUF_HEIGHT_TILES - 1;
            ioy = -1;
        }
        else
        {
            /* New is higher than old - start at top and go down. */
            new_y = 0;
            ioy = 1;
        }
        if(new_base_tilex < priv->base_tilex)
        {
            /* New is righter than old - start at right and go left. */
            base_new_x = BUF_WIDTH_TILES - 1;
            iox = -1;
        }
        else
        {
            /* New is lefter than old - start at left and go right. */
            base_new_x = 0;
            iox = 1;
        }

        /* Iterate over the y tile values. */
        old_y = new_y + new_base_tiley - priv->base_tiley;
        base_old_x = base_new_x + new_base_tilex - priv->base_tilex;
        priv->base_tilex = new_base_tilex;
        priv->base_tiley = new_base_tiley;
        for(j = 0; j < BUF_HEIGHT_TILES; ++j, new_y += ioy, old_y += ioy)
        {
            new_x = base_new_x;
            old_x = base_old_x;
            /* Iterate over the x tile values. */
            for(k = 0; k < BUF_WIDTH_TILES; ++k, new_x += iox, old_x += iox)
            {
                /* Can we get this grid block from the old buffer?. */
                if(old_x >= 0 && old_x < BUF_WIDTH_TILES
                        && old_y >= 0 && old_y < BUF_HEIGHT_TILES)
                {
                    /* Copy from old buffer to new buffer. */
                    gdk_draw_drawable(
                            priv->pixmap,
                            priv->gc_mark,
                            priv->pixmap,
                            old_x * TILE_SIZE_PIXELS,
                            old_y * TILE_SIZE_PIXELS,
                            new_x * TILE_SIZE_PIXELS,
                            new_y * TILE_SIZE_PIXELS,
                            TILE_SIZE_PIXELS, TILE_SIZE_PIXELS);
                }
                else
                {
                    map_widget_render_tile(dmap,
                            new_base_tilex + new_x,
                            new_base_tiley + new_y,
                            new_x * TILE_SIZE_PIXELS,
                            new_y * TILE_SIZE_PIXELS,
                            FALSE);
                    /* DENSITY */
                    if ( priv->draw_tile_overlay )
                    {
                        priv->draw_tile_overlay(priv->pixmap,
                                                new_base_tilex + new_x,
                                                new_base_tiley + new_y,
                                                priv->zoom,
                                                new_x * TILE_SIZE_PIXELS,
                                                new_y * TILE_SIZE_PIXELS);
                    }
                    else
                        g_debug("No tile overlay function");
                }
            }
        }
            /* map_render_paths();
                map_render_poi();*/
    MACRO_RECALC_OFFSET();
    MACRO_RECALC_FOCUS_BASE();
    }

    MACRO_RECALC_OFFSET();
    MACRO_RECALC_FOCUS_BASE();



    MACRO_QUEUE_DRAW_AREA();


}
/***/
static void
map_widget_render_tile(GtkWidget *dmap, guint tilex, guint tiley, guint destx, guint desty, gboolean fast_fail)
{
    GdkPixbuf *pixbuf = NULL;
    GError *error = NULL;
    guint zoff = 0;
    gchar *maptile = NULL;

    MapWidgetPrivate *priv;
    g_return_if_fail(IS_MAP_WIDGET(dmap));

    priv = MAP_WIDGET_GET_PRIVATE(dmap);

    if ( tilex < priv->world_size_tiles && tiley < priv->world_size_tiles )
    {

       // Get maptile file location from Maps framework
        maptile = map_widget_get_maptile(dmap, tilex, tiley, priv->zoom);

        if ( maptile )
        {
            g_debug("We have maptile %s, let's create a pixbuf...\n", maptile);
            pixbuf = gdk_pixbuf_new_from_file(maptile, &error);
        }
        /* We should try to download maptile from higher zoom level */
        else
        {
            for(zoff = 1; !pixbuf && (priv->zoom + zoff) <= MAX_ZOOM
                    && zoff <= TILE_SIZE_P2; zoff += 1)
            {
                /* Attempt to blit a wider map. */
                maptile = map_widget_get_maptile(dmap, (tilex >> zoff), (tiley >> zoff), (priv->zoom + zoff));
                if ( maptile )
                {
                    pixbuf = gdk_pixbuf_new_from_file(maptile, &error);
                    /* Out of memory, corrupted image, unsupperted image format etc. weird error */
                    if(error)
                    {
                        pixbuf = NULL;
                        error = NULL;
                    }
                }
                else
                    /* Try, try, try again */
                    continue;
            }	
        }

        if ( maptile )
            g_free(maptile);

        if ( pixbuf )
        {
            g_debug("We have pixbuf, let's see if we need trimming and scaling...\n");
            /* Trimming if necessary */
            if ( gdk_pixbuf_get_width(pixbuf) != TILE_SIZE_PIXELS || 
                 gdk_pixbuf_get_height(pixbuf) != TILE_SIZE_PIXELS )
                    pixbuf = map_widget_pixbuf_trim(pixbuf);
            /* We got  the maptile from higher zoom level */
            if ( zoff )
                map_widget_pixbuf_scale_inplace(pixbuf, zoff,
                                        (tilex - ((tilex>>zoff) << zoff)) << (TILE_SIZE_P2-zoff),
                                        (tiley - ((tiley>>zoff) << zoff)) << (TILE_SIZE_P2-zoff));
        }
    }
    if ( pixbuf )
    {
        g_debug("Everything ready for drawing, so let's draw the tile...\n");
        gdk_draw_pixbuf(
            priv->pixmap,
            priv->gc_mark,
            pixbuf,
            0, 0,
            destx, desty,
            TILE_SIZE_PIXELS, TILE_SIZE_PIXELS,
            GDK_RGB_DITHER_NONE, 0, 0);
        g_object_unref(pixbuf);
    }
    else
    {
        g_debug("Pixbuf creation failure, let's draw black rectangle instead...\n");
        gdk_draw_rectangle(priv->pixmap, dmap->style->black_gc, TRUE,
                            destx, desty,
                            TILE_SIZE_PIXELS, TILE_SIZE_PIXELS);
    }

}

/**
 * Do an in-place scaling of a pixbuf's pixels at the given ratio from the
 * given source location.  It would have been nice if gdk_pixbuf provided
 * this method, but I guess it's not general-purpose enough.
 */
static void
map_widget_pixbuf_scale_inplace(GdkPixbuf* pixbuf, guint ratio_p2,
        guint src_x, guint src_y)
{
    guint dest_x = 0, dest_y = 0, dest_dim = TILE_SIZE_PIXELS;
    guint rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    guint n_channels = gdk_pixbuf_get_n_channels(pixbuf);
    guchar* pixels = gdk_pixbuf_get_pixels(pixbuf);

    /* Sweep through the entire dest area, copying as necessary, but
     * DO NOT OVERWRITE THE SOURCE AREA.  We'll copy it afterward. */
    do {
        guint src_dim = dest_dim >> ratio_p2;
        guint src_endx = src_x - dest_x + src_dim;
        gint x, y;
        for(y = dest_dim - 1; y >= 0; y--)
        {
            guint src_offset_y, dest_offset_y;
            src_offset_y = (src_y + (y >> ratio_p2)) * rowstride;
            dest_offset_y = (dest_y + y) * rowstride;
            x = dest_dim - 1;
            if((unsigned)(dest_y + y - src_y) < src_dim
                    && (unsigned)(dest_x + x - src_x) < src_dim)
                x -= src_dim;
            for(; x >= 0; x--)
            {
                guint src_offset, dest_offset, i;
                src_offset = src_offset_y + (src_x+(x>>ratio_p2)) * n_channels;
                dest_offset = dest_offset_y + (dest_x + x) * n_channels;
                pixels[dest_offset] = pixels[src_offset];
                for(i = n_channels - 1; i; --i) /* copy other channels */
                    pixels[dest_offset + i] = pixels[src_offset + i];
                if((unsigned)(dest_y + y - src_y) < src_dim && x == src_endx)
                    x -= src_dim;
            }
        }
        /* Reuse src_dim and src_endx to store new src_x and src_y. */
        src_dim = src_x + ((src_x - dest_x) >> ratio_p2);
        src_endx = src_y + ((src_y - dest_y) >> ratio_p2);
        dest_x = src_x;
        dest_y = src_y;
        src_x = src_dim;
        src_y = src_endx;
    }
    while((dest_dim >>= ratio_p2) > 1);

}

static GdkPixbuf*
map_widget_pixbuf_trim(GdkPixbuf* pixbuf)
{
    GdkPixbuf* mpixbuf = gdk_pixbuf_new(
            GDK_COLORSPACE_RGB, gdk_pixbuf_get_has_alpha(pixbuf),
            8, TILE_SIZE_PIXELS, TILE_SIZE_PIXELS);

    gdk_pixbuf_copy_area(pixbuf,
            (gdk_pixbuf_get_width(pixbuf) - TILE_SIZE_PIXELS) / 2,
            (gdk_pixbuf_get_height(pixbuf) - TILE_SIZE_PIXELS) / 2,
            TILE_SIZE_PIXELS, TILE_SIZE_PIXELS,
            mpixbuf,
            0, 0);

    g_object_unref(pixbuf);
    return mpixbuf;
}

static gchar *
map_widget_get_maptile(GtkWidget *dmap, guint tilex, guint tiley, guint zoom)
{

    int ret = 0;
    gchar *maptile = NULL;
    osso_rpc_t retval;

    MapWidgetPrivate *priv;
    g_return_val_if_fail(IS_MAP_WIDGET(dmap), NULL);

    priv = MAP_WIDGET_GET_PRIVATE(dmap);

    if ( !maptile_tile_cache )
        maptile_tile_cache = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    if ( !maptile_key_cache )
        maptile_key_cache = g_ptr_array_sized_new(100);

    gchar *key = g_strdup_printf("%s:%d:%d:%d", priv->maptype, zoom, tilex, tiley);
//    g_debug("Key: %s", key);

    gchar *cache_tile = g_hash_table_lookup(maptile_tile_cache, key);
    if ( cache_tile )
    {

//        g_debug("Found tile %s from cache", cache_tile);
        maptile = g_strdup(cache_tile);
    }
    else
    {

//        g_debug("Received maptile location %s from %s service\n", maptile, MAP_WIDGET_FW_DBUS_SERVICE);

        osso_rpc_set_timeout(priv->osso_context, 9000);

        ret = osso_rpc_run( priv->osso_context,
                            MAP_WIDGET_FW_DBUS_SERVICE,
                            MAP_WIDGET_FW_DBUS_OBJECT_PATH,
                            MAP_WIDGET_FW_DBUS_INTERFACE,
                            MAP_WIDGET_FW_FUNCTION_MAPTILE_REQUEST,
                            &retval,
                            DBUS_TYPE_STRING, priv->maptype,
                            DBUS_TYPE_UINT32, tilex,
                            DBUS_TYPE_UINT32, tiley,
                            DBUS_TYPE_UINT32, zoom,
                            DBUS_TYPE_INVALID );
    
        if ( ret != OSSO_OK )
            g_warning("Could not connect to %s service!\n", MAP_WIDGET_FW_DBUS_SERVICE);
        else
        {
            maptile = g_strdup(retval.value.s);
//            g_debug("Received maptile location %s from %s service\n", maptile, MAP_WIDGET_FW_DBUS_SERVICE);
            if ( maptile )
            {
                g_ptr_array_add(maptile_key_cache, key);

                /* Keep maximum of 100 tile locations in the memory */
                if ( maptile_key_cache->len > 99 )
                {
//                    g_debug("Cache len is %d, removing key and maptile", maptile_key_cache->len);
                    gchar *remove_key = g_ptr_array_index(maptile_key_cache, 0);
                    if ( remove_key )
                    {
//                        g_debug("Removing key: %s", remove_key);
                        g_hash_table_remove(maptile_tile_cache, remove_key);
                        g_ptr_array_remove(maptile_key_cache, remove_key);
                        g_free(remove_key);
                    }
                }
//                g_debug("Inserting tile: %s to cache with key: %s", maptile, key);
                g_hash_table_insert(maptile_tile_cache, g_strdup(key), g_strdup(maptile));
            }
        }
        osso_rpc_free_val(&retval);
    }

    return maptile;
}

void
map_widget_set_zoom(GtkWidget *dmap, gfloat zoom_f)
{
    MapWidgetPrivate *priv;
    g_return_if_fail(IS_MAP_WIDGET(dmap));

    priv = MAP_WIDGET_GET_PRIVATE(dmap);
    priv->zoom_float = zoom_f;

    guint zoom;

     zoom = map_widget_convert_zoom( map_widget_float_to_integer(zoom_f) );

    /* Note that, since new_zoom is a guint and MIN_ZOOM is 0, this if
    * condition also checks for new_zoom >= MIN_ZOOM. */
    if(zoom > (MAX_ZOOM - 1))
            return;
    priv->zoom = zoom;

    priv->world_size_tiles = unit2tile(WORLD_SIZE_UNITS);

    /* If we're leading, update the center to reflect new zoom level. */
    MACRO_RECALC_CENTER(priv->center.unitx, priv->center.unity);

    /* Update center bounds to reflect new zoom level. */
    priv->min_center.unitx = pixel2unit(grid2pixel(priv->screen_grids_halfwidth));
    priv->min_center.unity = pixel2unit(grid2pixel(priv->screen_grids_halfheight));
    priv->max_center.unitx = WORLD_SIZE_UNITS-grid2unit(priv->screen_grids_halfwidth) -1;
    priv->max_center.unity = WORLD_SIZE_UNITS-grid2unit(priv->screen_grids_halfheight)-1;

    BOUND(priv->center.unitx, priv->min_center.unitx, priv->max_center.unitx);
    BOUND(priv->center.unity, priv->min_center.unity, priv->max_center.unity);

    priv->base_tilex = grid2tile((gint)pixel2grid(
                    (gint)unit2pixel((gint)priv->center.unitx))
            - (gint)priv->screen_grids_halfwidth);
    priv->base_tiley = grid2tile(pixel2grid(
                    unit2pixel(priv->center.unity))
            - priv->screen_grids_halfheight);

    /* New zoom level, so we can't reuse the old buffer's pixels. */

    /* Update state variables. */
    MACRO_RECALC_OFFSET();
    MACRO_RECALC_FOCUS_BASE();
    MACRO_RECALC_FOCUS_SIZE();


    map_widget_force_redraw(dmap);
}

/**
 *  Interface functions
 */

/**
 * Receive configure event from window
 */
gboolean
map_widget_configure_event(GtkWidget *dmap, GdkEventConfigure *event)
{
    g_debug("Configure event!\n");

    MapWidgetPrivate *priv;
    g_return_val_if_fail(IS_MAP_WIDGET(dmap), FALSE);

    priv = MAP_WIDGET_GET_PRIVATE(dmap);
    DEBUG_PRIVATE_MEMBERS();

    priv->screen_width_pixels = dmap->allocation.width;
    priv->screen_height_pixels = dmap->allocation.height;
    priv->screen_width_pixels = event->width;
    priv->screen_height_pixels = event->height;

    priv->screen_grids_halfwidth = pixel2grid(priv->screen_width_pixels) / 2;
    priv->screen_grids_halfheight = pixel2grid(priv->screen_height_pixels) / 2;

    MACRO_RECALC_FOCUS_BASE();
    MACRO_RECALC_FOCUS_SIZE();

    priv->min_center.unitx = pixel2unit(grid2pixel(priv->screen_grids_halfwidth));
    priv->min_center.unity = pixel2unit(grid2pixel(priv->screen_grids_halfheight));
    priv->max_center.unitx = WORLD_SIZE_UNITS-grid2unit(priv->screen_grids_halfwidth) -1;
    priv->max_center.unity = WORLD_SIZE_UNITS-grid2unit(priv->screen_grids_halfheight)-1;

    DEBUG_PRIVATE_MEMBERS();
    map_widget_center_unit(dmap, priv->center.unitx, priv->center.unity);
    DEBUG_PRIVATE_MEMBERS();
    return TRUE;
}

/**
 * Initialize DBUS connection for maptile fetching
 */
void
map_widget_init_dbus(GtkWidget *dmap, osso_context_t *osso_conn)
{
    MapWidgetPrivate *priv;

    g_return_if_fail(IS_MAP_WIDGET(dmap));

    priv = MAP_WIDGET_GET_PRIVATE(dmap);

    if ( priv->pixmap == NULL )
    {
        priv->pixmap = gdk_pixmap_new(dmap->window,
                                      BUF_WIDTH_PIXELS, BUF_HEIGHT_PIXELS,
                                      -1);
        if ( priv->pixmap )
            g_debug("Pixmap creation successful\n");
    }

    priv->osso_context = osso_conn;


    priv->gc_mark = get_color(dmap,COLOR_BLACK);

}



const gchar* map_widget_get_default_map_type (void) 
{
    gchar* test;
    test = g_strdup(DEFAULT_MAP_TYPE);

    return test;
}

const MapPoint* map_widget_get_default_center (void)
{
    MapPoint *default_center = NULL;

    default_center->latitude = DEFAULT_CENTER_LATITUDE;
    default_center->longitude = DEFAULT_CENTER_LONGITUDE;

    return default_center;
}
const gfloat map_widget_get_default_zoom_level (void)
{

    return DEFAULT_ZOOM;
}

const gfloat map_widget_get_default_orientation (void)
{

    return DEFAULT_ORIENTATION;

}

guint map_widget_get_current_zoom_level (GtkWidget *dmap)
{

	MapWidgetPrivate *priv;
	priv = MAP_WIDGET_GET_PRIVATE(dmap);

	return map_widget_convert_zoom(priv->zoom);

}

gfloat map_widget_calculate_distance(const MapPoint *src, const MapPoint *dest)
{

  g_debug("priv %f ja %f,--- normal %f ja %f\n",src->latitude,
						src->longitude,
						dest->latitude,
						dest->longitude);


    gfloat dlat, dlon, slat, slon, a;
    gdouble distance;

    gfloat cp_src_lat = 0.0;
    gfloat cp_src_lon = 0.0;
    gfloat cp_dest_lat = 0.0;
    gfloat cp_dest_lon = 0.0;


    cp_src_lat = src->latitude;
    cp_src_lon = src->longitude;
    cp_dest_lat = dest->latitude;
    cp_dest_lon = dest->longitude;
    
    /* convert to radians. */
    cp_src_lat *= (PI / 180.f);
    cp_src_lon *= (PI / 180.f);
    cp_dest_lat *= (PI / 180.f);
    cp_dest_lon *= (PI / 180.f);

    dlat = cp_dest_lat - cp_src_lat;
    dlon = cp_dest_lon - cp_src_lon;

    slat = sinf(dlat / 2.f);
    slon = sinf(dlon / 2.f);
    a = (slat * slat) + (cosf(cp_src_lat) * cosf(cp_dest_lat) * slon * slon);

    
    distance = ((2.f * atan2f(sqrtf(a), sqrtf(1.f - a))) * EARTH_RADIUS);

g_debug("distance: %f\n", distance*1000);

/* Return: Distance in meters */
    return distance*1000;

}

/**
 * Force a redraw of the entire _map_pixmap, including fetching the
 * background maps from disk and redrawing the tracks on top of them.
 */
static void
map_widget_force_redraw(GtkWidget *dmap)
{

MapWidgetPrivate *priv;
priv = MAP_WIDGET_GET_PRIVATE(dmap);

    guint new_x, new_y;


    for(new_y = 0; new_y < BUF_HEIGHT_TILES; ++new_y)
        for(new_x = 0; new_x < BUF_WIDTH_TILES; ++new_x)
        {
           map_widget_render_tile(dmap,
                    priv->base_tilex+ new_x,
                    priv->base_tiley + new_y,
                    new_x * TILE_SIZE_PIXELS,
                    new_y * TILE_SIZE_PIXELS,
                    FALSE);
            /* DENSITY */
            if ( priv->draw_tile_overlay )
            {
                priv->draw_tile_overlay(priv->pixmap,
                                        priv->base_tilex + new_x,
                                        priv->base_tiley + new_y,
                                        priv->zoom,
                                        new_x * TILE_SIZE_PIXELS,
                                        new_y * TILE_SIZE_PIXELS);
            }
            else
                g_debug("No tile overlay function");
        }

    MACRO_QUEUE_DRAW_AREA();
}


void map_widget_set_current_location(GtkWidget *dmap, MapPoint* new_center) 
{

	MapWidgetPrivate *priv;
	priv = MAP_WIDGET_GET_PRIVATE(dmap);


	priv->mylocation.latitude = new_center->latitude;
	priv->mylocation.longitude  = new_center->longitude;

 	latlon2unit(priv->mylocation.latitude, priv->mylocation.longitude, priv->center.unitx, priv->center.unity);
	
        if (priv->auto_center)
		map_widget_center_unit(dmap, priv->center.unitx, priv->center.unity);

}



void map_widget_draw_current_place(GtkWidget *dmap)
{


	MapWidgetPrivate *priv;
	priv = MAP_WIDGET_GET_PRIVATE(dmap);

    if (priv->show_current_place)
    {
	guint mark_x1;
	guint mark_y1;

	gint pplx = 0;
	gint pply = 0;
	GdkPixbuf *pixbuf = NULL;
	GError *error = NULL;


    	
        latlon2unit(priv->mylocation.latitude,priv->mylocation.longitude, pplx, pply);
        mark_x1 = unit2x(pplx);
        mark_y1 = unit2y(pply);

	if (priv->current_place_icon != NULL)
	{
		pixbuf = gdk_pixbuf_new_from_file(priv->current_place_icon, &error);
		
		if (pixbuf != NULL)
		{
			gdk_draw_pixbuf(dmap->window,
	                        priv->gc_mark,
	                        pixbuf,
	                        0, 0,
	                        mark_x1,
	                        mark_y1,
	                        -1,-1,
	                        GDK_RGB_DITHER_NONE, 0, 0);
			g_object_unref(pixbuf);
	
		}
		else
		{
	   	 gdk_draw_arc(	dmap->window,
			get_color(dmap,COLOR_RED),
			FALSE, 
			mark_x1,
			mark_y1,
        		3, 
			3,
        		0, 
			360 * 64);
		}
	}
	else
	{
	   	 gdk_draw_arc(	dmap->window,
			get_color(dmap,COLOR_RED),
			FALSE, 
			mark_x1-10,
			mark_y1-10,
        		20,
			20,
        		0, 
			360 * 64);
	}

    }
}

static void draw_pango_text(GtkWidget *dmap,guint x,guint y,gchar *name)
{


	MapWidgetPrivate *priv;
	priv = MAP_WIDGET_GET_PRIVATE(dmap);

        
        GdkGC *gc;
	PangoContext* context;
	PangoLayout *layout;
        PangoFontDescription *desc;

	context = gtk_widget_get_pango_context(GTK_WIDGET(dmap));
	layout = pango_layout_new(context);
        gc = get_color(dmap,COLOR_RED);
     
        pango_layout_set_text(layout,name,-1);

        desc = pango_font_description_from_string ("Sans Bold 12");
        pango_layout_set_font_description (layout, desc);
        gdk_draw_layout(dmap->window,gc,x+4,y-20,layout);


}

void map_widget_draw_static_info(GtkWidget *dmap)
{

	MapWidgetPrivate *priv;
	priv = MAP_WIDGET_GET_PRIVATE(dmap);

	gint i;
	gint pplx = 0;
	gint pply = 0;

	guint mark_x1;
	guint mark_y1;
	
        PoiInfo *returned_poi_info = NULL;    	

	MapPoint *poi;
    	poi = g_new0(MapPoint, 1);

	GdkGC *gc_poi;
	GdkPixbuf *pixbuf = NULL;
	GError *error = NULL;


        gc_poi = get_color(dmap,COLOR_BLUE);
      for (i = 0; i <= priv->static_pois->len - 1; i++) {
        returned_poi_info = g_ptr_array_index(priv->static_pois, i);
	
	poi->latitude =  atof(returned_poi_info->lat);
	poi->longitude = atof(returned_poi_info->lng);

        latlon2unit(poi->latitude,poi->longitude, pplx, pply);
        mark_x1 = unit2x(pplx);
        mark_y1 = unit2y(pply);

	/* draw poi icon, if icon is NULL or pixpuf creation fail then it draw just dot. 
	*/
		if (returned_poi_info->icon != NULL)
		{
		pixbuf = gdk_pixbuf_new_from_file(returned_poi_info->icon, &error);

		    if ( pixbuf != NULL)
		    {
		    gdk_draw_pixbuf(dmap->window,
	                        gc_poi,
	                        pixbuf,
	                        0, 0,
	                        mark_x1,
	                        mark_y1,
	                        -1,-1,
	                        GDK_RGB_DITHER_NONE, 0, 0);
	
		    g_object_unref(pixbuf);
		    }
		    else 
		    {
		    gdk_draw_arc(dmap->window,
			     gc_poi,
			     TRUE, 
			     mark_x1, 
			     mark_y1,
        		     8, 
		             8,
        		     0, 
			     360 * 64);
		    }
		}

		else
		{
		gdk_draw_arc(dmap->window,
			     gc_poi,
			     TRUE, 
			     mark_x1, 
			     mark_y1,
        		     8, 
		             8,
        		     0, 
			     360 * 64);
	
		}
		if (returned_poi_info->show_name)
		{
		    draw_pango_text(dmap,mark_x1,mark_y1,returned_poi_info->name);
		}

	}

}






void map_widget_draw_buddies(GtkWidget *dmap)
{

	MapWidgetPrivate *priv;
	priv = MAP_WIDGET_GET_PRIVATE(dmap);

	gint i;
	gint pplx = 0;
	gint pply = 0;

	guint mark_x1;
	guint mark_y1;
	
        BuddyInfo *returned_buddy_info = NULL;    	

	MapPoint *buddy;
    	buddy = g_new0(MapPoint, 1);

 	GdkGC *gc_buddy;

	GdkPixbuf *pixbuf = NULL;
	GError *error = NULL;

        gc_buddy = get_color(dmap,COLOR_BLUE);




    

    for (i = 0; i <= priv->buddies->len - 1; i++) 
    {        
	returned_buddy_info = g_ptr_array_index(priv->buddies, i);
	
	buddy->latitude =  atof(returned_buddy_info->lat);
	buddy->longitude = atof(returned_buddy_info->lng);
        g_debug("drawing buddy no:%d: %f, %f\n",i, buddy->latitude, buddy->longitude );
        latlon2unit(buddy->latitude,buddy->longitude, pplx, pply);
        mark_x1 = unit2x(pplx);
        mark_y1 = unit2y(pply);

	
		if (returned_buddy_info->icon != NULL)
		{
		pixbuf = gdk_pixbuf_new_from_file(returned_buddy_info->icon, &error);
		
		    if (pixbuf != NULL)
		    {
		    gdk_draw_pixbuf(dmap->window,
	                        gc_buddy,
	                        pixbuf,
	                        0, 0,
	                        mark_x1,
	                        mark_y1,
	                        -1,-1,
	                        GDK_RGB_DITHER_NONE, 0, 0);
		    g_object_unref(pixbuf);
	  	    }
		    else
		    {
		    gdk_draw_arc(dmap->window,
			gc_buddy,
			TRUE, 
			mark_x1, 
			mark_y1,
        		8, 
			8,
        		0, 
			360 * 64);

                    }

		}

		else
		{
	   	 gdk_draw_arc(dmap->window,
			gc_buddy,
			TRUE, 
			mark_x1, 
			mark_y1,
        		8, 
			8,
        		0, 
			360 * 64);

		}
		if (returned_buddy_info->show_name)
		{
		    draw_pango_text(dmap,mark_x1,mark_y1,returned_buddy_info->name);
		}	
		
	}

}



void map_widget_draw_arc_to_coords(GtkWidget *dmap, MapPoint* point_to_draw, gint width, gint height, gboolean filled)
{

	MapWidgetPrivate *priv;
	priv = MAP_WIDGET_GET_PRIVATE(dmap);


	gint pplx = 0.0;
	gint pply = 0.0;

	guint mark_x1;
	guint mark_y1;
    	


        latlon2unit(point_to_draw->latitude,point_to_draw->longitude, pplx, pply);
        mark_x1 = unit2x(pplx);
        mark_y1 = unit2y(pply);

   	 gdk_draw_arc(	dmap->window,
			priv->gc_mark,
			filled, 
			mark_x1, 
			mark_y1,
        		width, 
			height,
        		0, 
			360 * 64);

}
void map_widget_draw_rectangle_to_coords(GtkWidget *dmap, 
		MapPoint* point_to_draw, gint width, gint height, gboolean filled)
{

	MapWidgetPrivate *priv;
	priv = MAP_WIDGET_GET_PRIVATE(dmap);


	gint pplx = 0.0;
	gint pply = 0.0;

	guint mark_x1;
	guint mark_y1;
    	


        latlon2unit(point_to_draw->latitude,point_to_draw->longitude, pplx, pply);
        mark_x1 = unit2x(pplx);
        mark_y1 = unit2y(pply);

   	 gdk_draw_rectangle(dmap->window,
			priv->gc_mark,
			filled, 
			mark_x1, 
			mark_y1,
        		width, 
			height);

}

void map_widget_draw_icon_to_coords(GtkWidget *dmap,
		MapPoint* point_to_draw, gchar* icon)
{

	MapWidgetPrivate *priv;
	priv = MAP_WIDGET_GET_PRIVATE(dmap);

	gint pplx = 0.0;
	gint pply = 0.0;

	guint mark_x1;
	guint mark_y1;
	GdkPixbuf *pixbuf = NULL;
	GError *error = NULL;

        pixbuf = gdk_pixbuf_new_from_file(icon, &error);

        latlon2unit(point_to_draw->latitude,point_to_draw->longitude, pplx, pply);
        mark_x1 = unit2x(pplx);
        mark_y1 = unit2y(pply);

        gdk_draw_pixbuf(dmap->window,
                        priv->gc_mark,
                        pixbuf,
                        0, 0,
                        mark_x1,
                        mark_y1,
                        -1,-1,
                        GDK_RGB_DITHER_NONE, 0, 0);
         g_object_unref(pixbuf);

	
}
gboolean
map_widget_get_maptypes(GtkWidget *dmap, gchar ***maptypes)
{

    DBusConnection *conn;
    DBusMessage *message = NULL;
    DBusMessage *reply = NULL;
    DBusError *error = NULL;
    gboolean ret = TRUE;
    guint n_maptypes = 0;



	MapWidgetPrivate *priv;
	priv = MAP_WIDGET_GET_PRIVATE(dmap);

    conn = osso_get_dbus_connection( priv->osso_context );


    message = dbus_message_new_method_call( MAP_WIDGET_FW_DBUS_SERVICE,
                                            MAP_WIDGET_FW_DBUS_OBJECT_PATH,
                                            MAP_WIDGET_FW_DBUS_INTERFACE,
                                            MAP_WIDGET_FW_FUNCTION_GET_SUPPORTED_MAPS);


    dbus_message_append_args(message, G_TYPE_INVALID);

    reply = dbus_connection_send_with_reply_and_block( conn, message,-1, error);
    if ( reply && !error)
    {
        if (!dbus_message_get_args(reply, error, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, maptypes, &n_maptypes, DBUS_TYPE_INVALID))
        {
            ret = FALSE;
        }
    }
    else
    {
        ret = FALSE;
    }

    dbus_message_unref(message);
    return ret;
}

static guint map_widget_float_to_integer(gfloat value_float)
{

return (int)(value_float+0.5);

}



static guint map_widget_convert_zoom(guint zoom)
{
       gint ret = 17 - zoom;
       if ( ret < 0 )
               return DEFAULT_ZOOM;
       return ret;
}



GdkGC* get_color(GtkWidget *dmap, guint color)
{

	MapWidgetPrivate *priv;
	priv = MAP_WIDGET_GET_PRIVATE(dmap);

	GdkGC* gcmark;
	
	/** THE GdkGC OBJECTS USED FOR DRAWING. */

	if (color == COLOR_BLACK)
	{	
		priv->color_mark.pixel = 0; 
		priv->color_mark.red = 0; 
		priv->color_mark.green = 0; 
		priv->color_mark.blue = 0;
	}
	
	else if (color == COLOR_RED)
	{	
		priv->color_mark.pixel = 0; 
		priv->color_mark.red =  0xe000; 
		priv->color_mark.green = 0x0000; 
		priv->color_mark.blue = 0x0000;
	}
	else if (color == COLOR_BLUE)
	{	
		priv->color_mark.pixel = 0; 
		priv->color_mark.red =  0x0000; 
		priv->color_mark.green = 0x0000; 
		priv->color_mark.blue = 0xe000;
	}
	else if (color == COLOR_GREEN)
	{	
		priv->color_mark.pixel = 0; 
		priv->color_mark.red =  0x0000; 
		priv->color_mark.green = 0xe000; 
		priv->color_mark.blue = 0x0000;
	}
	else /*  */
	{	
		priv->color_mark.pixel = 0; 
		priv->color_mark.red = 0; 
		priv->color_mark.green = 0; 
		priv->color_mark.blue = 0;
	}

	gdk_color_alloc(gtk_widget_get_colormap(dmap),
	        &priv->color_mark);
	    gcmark = gdk_gc_new(priv->pixmap);
	    gdk_gc_set_foreground(gcmark, &priv->color_mark);
	    gdk_gc_set_line_attributes(gcmark,
            priv->draw_line_width, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);


return gcmark;

}

void map_widget_set_static_poi_array(GtkWidget *dmap, GPtrArray *static_pois, gboolean show_pois)
{

	MapWidgetPrivate *priv;
	priv = MAP_WIDGET_GET_PRIVATE(dmap);

	priv->static_pois = static_pois;
	priv->show_pois = show_pois;
        map_widget_force_redraw(dmap);
}
GPtrArray * map_widget_get_static_poi_array(GtkWidget *dmap)
{

	MapWidgetPrivate *priv;
	priv = MAP_WIDGET_GET_PRIVATE(dmap);

	return priv->static_pois;
}

void map_widget_set_buddies_array(GtkWidget *dmap, GPtrArray *buddies, gboolean show_buddies)
{

	MapWidgetPrivate *priv;
	priv = MAP_WIDGET_GET_PRIVATE(dmap);

	priv->buddies = buddies;
	priv->show_buddies = show_buddies;
        map_widget_force_redraw(dmap);
}
GPtrArray * map_widget_get_buddies_array(GtkWidget *dmap)
{

	MapWidgetPrivate *priv;
	priv = MAP_WIDGET_GET_PRIVATE(dmap);

	return priv->buddies;
}


void map_widget_set_is_current_place_shown(GtkWidget *dmap, gboolean show_current_place)
{
	
	MapWidgetPrivate *priv;
	priv = MAP_WIDGET_GET_PRIVATE(dmap);

	if (show_current_place == TRUE)
		priv->show_current_place = TRUE;

	if (show_current_place == FALSE)
		priv->show_current_place = FALSE;

	else
	   ;/*do nothing */


}
void map_widget_change_maptype(GtkWidget *dmap,gchar* new_maptype)
{

    MapWidgetPrivate *priv;
    priv = MAP_WIDGET_GET_PRIVATE(dmap);

    priv->maptype = new_maptype;
    map_widget_force_redraw(dmap);


}

gboolean map_widget_get_auto_center_status(GtkWidget *dmap)
{
	MapWidgetPrivate *priv;
	priv = MAP_WIDGET_GET_PRIVATE(dmap);

	return priv->auto_center;
}

void map_widget_set_auto_center(GtkWidget *dmap, gboolean auto_center)
{
    MapWidgetPrivate *priv;
    priv = MAP_WIDGET_GET_PRIVATE(dmap);


    if (auto_center == TRUE)
    {
	priv->auto_center =TRUE;
	map_widget_center_unit(dmap, priv->center.unitx, priv->center.unity);
    }
    else if (auto_center == FALSE)
	priv->auto_center = FALSE;
    else
	priv->auto_center = FALSE;

}

void map_widget_draw_bubble(GtkWidget *dmap,MapPoint* bubble_point)
{

    MapWidgetPrivate *priv;
    priv = MAP_WIDGET_GET_PRIVATE(dmap);


            
       
}

static
void map_widget_do_callbacks_first(GtkWidget *dmap)
{

	guint i = 0;
	MW_cb *mw_cb;

	MapWidgetPrivate *priv;
	priv = MAP_WIDGET_GET_PRIVATE(dmap);

	if ( priv->cb_functions_level_one != NULL)
	{		
		for (i = 0; i < priv->cb_functions_level_one->len; i++) {
			mw_cb = g_ptr_array_index(priv->cb_functions_level_one, i);
     			if (mw_cb->do_cb_func)
			{
			  mw_cb->cb_func_name(dmap,mw_cb->data);
   			}
  		}

	}


}
static
void map_widget_do_callbacks_second(GtkWidget *dmap)
{

	guint i = 0;
	MW_cb *mw_cb;

	MapWidgetPrivate *priv;
	priv = MAP_WIDGET_GET_PRIVATE(dmap);

	if ( priv->cb_functions_level_two != NULL)
	{		
		for (i = 0; i < priv->cb_functions_level_two->len; i++) {
			mw_cb = g_ptr_array_index(priv->cb_functions_level_two, i);
     			if (mw_cb->do_cb_func)
			{
			  mw_cb->cb_func_name(dmap,mw_cb->data);

   			}
  		}

	}

}


static
void map_widget_do_callbacks_third(GtkWidget *dmap)
{

	guint i = 0;
	MW_cb *mw_cb;

	MapWidgetPrivate *priv;
	priv = MAP_WIDGET_GET_PRIVATE(dmap);

	if ( priv->cb_functions_level_three != NULL)
	{		
		for (i = 0; i < priv->cb_functions_level_three->len; i++) {
			mw_cb = g_ptr_array_index(priv->cb_functions_level_three, i);
     			if (mw_cb->do_cb_func)
			{
			  mw_cb->cb_func_name(dmap,mw_cb->data);

   			}
  		}

	}


}

void map_widget_set_cb_func_array(GtkWidget *dmap, GPtrArray *cb_funcs, guint level)
{

	MapWidgetPrivate *priv;
	priv = MAP_WIDGET_GET_PRIVATE(dmap);
	
        if (level == 1)
	{
		priv->cb_functions_level_one = cb_funcs;

	}

        if (level == 2)
	{	
		priv->cb_functions_level_two = cb_funcs;

	}
        if (level == 3)
	{
		priv->cb_functions_level_three = cb_funcs;

	}

	
        map_widget_force_redraw(dmap);
	
}

GPtrArray* map_widget_get_cb_func_array(GtkWidget *dmap, guint level)
{
	MapWidgetPrivate *priv;
	priv = MAP_WIDGET_GET_PRIVATE(dmap);
	
	if (level == 1)
		return priv->cb_functions_level_one;
	else if (level == 2)
		return priv->cb_functions_level_two;
	else if (level == 3)
		return priv->cb_functions_level_three;
	else
		return NULL;

}

void map_widget_set_current_location_icon(GtkWidget *dmap, gchar* current_place_icon)
{
	MapWidgetPrivate *priv;
	priv = MAP_WIDGET_GET_PRIVATE(dmap);

	priv->current_place_icon = current_place_icon;
}


/*screen tap*/
void  map_widget_screen_click(GtkWidget *dmap, guint x, guint y)
{

    MapWidgetPrivate *priv;
    priv = MAP_WIDGET_GET_PRIVATE(dmap);
    gfloat lat=0.0;
    gfloat lon=0.0;

    unit2latlon(priv->center.unitx, priv->center.unity, lat, lon);


    if (priv->screen_clicked_cb)
    {
	priv->screen_clicked_cb(dmap, x, y, lat, lon);
    }
}
/* route */
void map_widget_set_route_array(GtkWidget *dmap, GPtrArray *route, gboolean show_route)
{

	MapWidgetPrivate *priv;
	priv = MAP_WIDGET_GET_PRIVATE(dmap);

	priv->route = route;
	priv->show_route = show_route;
        map_widget_force_redraw(dmap);
}
static
void map_widget_draw_route_track(GtkWidget *dmap, GPtrArray *track_route)
{

	MapWidgetPrivate *priv;
	priv = MAP_WIDGET_GET_PRIVATE(dmap);

	gint i;
	gint pplx = 0;
	gint pply = 0;

	guint mark_x1;
	guint mark_y1;
	
	gint pplx2 = 0;
	gint pply2 = 0;

	guint mark_x2;
	guint mark_y2;

	guint x=0;
	guint tail_length=0;

	MapPoint *start_route_point;
    	start_route_point = g_new0(MapPoint, 1);

	MapPoint *end_route_point;
    	end_route_point = g_new0(MapPoint, 1);

	GdkGC *gc_poi;
        gc_poi = get_color(dmap,COLOR_BLACK);

        if (priv->track_route_tail_length !=0)
	{
	tail_length = priv->track_route_tail_length;
	}
	else 
	{
	tail_length = track_route->len;
	}




	for (i = track_route->len-1;i>0 && x<tail_length;i--) 
     	{
  
        if (i< track_route->len-1)
	{

            start_route_point = g_ptr_array_index(track_route, i);
            end_route_point = g_ptr_array_index(track_route, i+1);

            latlon2unit(start_route_point->latitude,start_route_point->longitude, pplx, pply);
            mark_x1 = unit2x(pplx);
            mark_y1 = unit2y(pply);

            latlon2unit(end_route_point->latitude,end_route_point->longitude, pplx2, pply2);
            mark_x2 = unit2x(pplx2);
            mark_y2 = unit2y(pply2);

            gdk_draw_line(dmap->window,
			gc_poi,
			mark_x1,
			mark_y1,
			mark_x2,
			mark_y2);

        x++;
	}   
	}
	
}
/* track */

void map_widget_show_track(GtkWidget *dmap, gboolean show_track)
{

	MapWidgetPrivate *priv;
	priv = MAP_WIDGET_GET_PRIVATE(dmap);

     
	priv->show_track = show_track;


	map_widget_center_unit(dmap, priv->center.unitx, priv->center.unity);
//        map_widget_force_redraw(dmap);
}

void map_widget_add_point_to_track(GtkWidget *dmap, MapPoint *track_point)
{
	MapWidgetPrivate *priv;
	priv = MAP_WIDGET_GET_PRIVATE(dmap);

	g_ptr_array_add(priv->track, track_point);

        priv->track_not_empty = TRUE;
        map_widget_center_unit(dmap, priv->center.unitx, priv->center.unity);
//        map_widget_force_redraw(dmap);
}

void map_widget_clear_track(GtkWidget *dmap)
{
	MapPoint *point = NULL;
	MapWidgetPrivate *priv = NULL;
	gint i = 0;

	priv = MAP_WIDGET_GET_PRIVATE(dmap);

	for(i = 0; i < priv->track->len; i++)
	{
		point = (MapPoint *)g_ptr_array_index(priv->track, i);
		g_free(point);
	}
	g_ptr_array_free(priv->track, TRUE);
	priv->track = g_ptr_array_new();

	priv->track_not_empty = FALSE;
}

void map_widget_set_track_tail_length(GtkWidget *dmap, guint track_tail_length)
{

	MapWidgetPrivate *priv;
	priv = MAP_WIDGET_GET_PRIVATE(dmap);
	priv->track_route_tail_length = track_tail_length;

}  


void map_widget_show_location(GtkWidget *dmap, MapPoint* location)
{


	MapWidgetPrivate *priv;
	priv = MAP_WIDGET_GET_PRIVATE(dmap);

 	latlon2unit(location->latitude, location->longitude, priv->center.unitx, priv->center.unity);
	map_widget_center_unit(dmap, priv->center.unitx, priv->center.unity);

    if (priv->auto_center)
	map_widget_set_auto_center(dmap, FALSE);

}


