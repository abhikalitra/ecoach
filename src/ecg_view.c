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

/* System */
#include <fcntl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

/* This module */
#include "ecg_view.h"

/* Custom modules */
#include "ec_error.h"
#include "gconf_keys.h"

#include "debug.h"

#define ECG_VIEW_WIDTH	650
#define ECG_VIEW_HEIGHT 350

static gboolean ecg_view_expose(
		GtkWidget *widget,
		GdkEventExpose *event,
		gpointer user_data);

static void ecg_view_btn_close_clicked(GtkWidget *widget, gpointer user_data);

EcgViewData *ecg_view_new(GConfHelperData *gconf_helper, EcgData *ecg_data)
{
	EcgViewData *self = NULL;

	g_return_val_if_fail(gconf_helper != NULL, NULL);
	g_return_val_if_fail(ecg_data != NULL, NULL);

	DEBUG_BEGIN();

	self = g_new0(EcgViewData, 1);
	if(!self)
	{
		g_critical("Not enough memory");
		return NULL;
	}

	self->gconf_helper = gconf_helper;
	self->ecg_data = ecg_data;

	self->main_widget = gtk_vbox_new(FALSE, 0);

	self->drawing_area = gtk_drawing_area_new();
	gtk_widget_set_size_request(self->drawing_area, 800, 410);

	gtk_box_pack_start(GTK_BOX(self->main_widget),
			self->drawing_area,
			TRUE, TRUE, 0);

	self->btn_close = gtk_button_new_with_mnemonic("_Back");

	gtk_widget_set_size_request(self->btn_close, 70, 120);

	gtk_box_pack_end(GTK_BOX(self->main_widget),
			self->btn_close,
			TRUE, TRUE, 0);

	g_signal_connect(G_OBJECT(self->btn_close), "clicked",
			G_CALLBACK(ecg_view_btn_close_clicked), self);

	self->pixmap = gdk_pixmap_new(
			NULL, 
			ECG_VIEW_WIDTH,
			ECG_VIEW_HEIGHT,	/* Size */
			16			/* Depth */
			);

	g_signal_connect(G_OBJECT(self->drawing_area),
			"expose_event",
			G_CALLBACK(ecg_view_expose),
			self);

	DEBUG_END();
	return self;
}

static void ecg_view_ecg_data_arrived(
		EcgData *ecg_data,
		guint8 *data,
		guint len,
		gpointer user_data)
{
	gint i = 0;
	GdkGC *gc = NULL;
	gint x1 = 0;
	gint x2 = 0;
	guint8 y1 = 0;
	guint8 y2 = 0;
	GdkRectangle rect;
	GdkColor fg;
	GdkColor bg;
	gdouble y1d = 0;
	gdouble y2d = 0;

	EcgViewData *self = (EcgViewData *)user_data;

	g_return_if_fail(user_data != NULL);
	DEBUG_BEGIN();

	/** @todo: draw from the previous end point */

	gc = gdk_gc_new(GDK_DRAWABLE(self->pixmap));

	/* Clear the drawable area */
	gdk_color_parse("#FFF", &fg);
	gdk_color_parse("#FFF", &bg);

	gdk_gc_set_rgb_fg_color(gc, &fg);
	gdk_gc_set_rgb_bg_color(gc, &bg);


	if(self->paint_position + len > ECG_VIEW_WIDTH)
	{
		gdk_draw_rectangle(GDK_DRAWABLE(self->pixmap),
				gc,
				TRUE, /* Filled */
				self->paint_position, 0, /* x, y */
				ECG_VIEW_WIDTH - self->paint_position,
				ECG_VIEW_HEIGHT);
		gdk_draw_rectangle(GDK_DRAWABLE(self->pixmap),
				gc,
				TRUE, /* Filled */
				0, 0, /* x, y */
				(gint)len - ECG_VIEW_WIDTH +
					self->paint_position,
				ECG_VIEW_HEIGHT);
	} else {
		gdk_draw_rectangle(GDK_DRAWABLE(self->pixmap),
				gc,
				TRUE, /* Filled */
				self->paint_position, 0,
				len, ECG_VIEW_HEIGHT);
	}

	gdk_color_parse("#000", &fg);
	gdk_gc_set_rgb_fg_color(gc, &fg);

	/* Draw the data to the pixbuf */

	x1 = self->paint_position - 1;

	for(i = 1; i < len - 1; i++)
	{
		x2 = x1 + 1;
		x1++;
		
		y1 = data[i];
		y2 = data[i + 1];
		y1d = (gdouble)y1;
		y2d = (gdouble)y2;

		/* Scale the y1 and y2 */
		y1d = (gdouble)ECG_VIEW_HEIGHT - ((gdouble)(y1d)) *
			((gdouble)ECG_VIEW_HEIGHT / 255.0);
		y2d = (gdouble)ECG_VIEW_HEIGHT - ((gdouble)(y2d)) *
			((gdouble)ECG_VIEW_HEIGHT / 255.0);

		if(x2 >= ECG_VIEW_WIDTH)
		{
			x1 = x2 = 0;
			y2d = y1d;
		}

		gdk_draw_line(
				GDK_DRAWABLE(self->pixmap),
				gc,
				x1, y1d,
				x2, y2d);
	}

	/* Draw a cursor */
	gdk_color_parse("#0F0", &fg);
	gdk_gc_set_rgb_fg_color(gc, &fg);
	gdk_draw_line(
			GDK_DRAWABLE(self->pixmap),
			gc,
			x1, 0,
			x1, ECG_VIEW_HEIGHT);

	gdk_gc_unref(gc);

	rect.x = 0;
	rect.y = 0;
	rect.width = self->drawing_area->allocation.width;
	rect.height = self->drawing_area->allocation.height;
	gdk_window_invalidate_rect(self->drawing_area->window,
			&rect,
			FALSE);

	self->paint_position = x2;

	DEBUG_END();
}

void ecg_view_start_drawing(EcgViewData *self)
{
	GError *error = NULL;
	gboolean retval;

	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	retval = ecg_data_add_callback_ecg(self->ecg_data,
			ecg_view_ecg_data_arrived,
			self, &error);

	if(!retval)
	{
		ec_error_show_message_error(error->message);
	}

	self->is_drawing = retval;

	DEBUG_END();
}

void ecg_view_stop_drawing(EcgViewData *self)
{
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	if(self->is_drawing)
	{
		self->is_drawing = FALSE;
		ecg_data_remove_callback_ecg(self->ecg_data,
				ecg_view_ecg_data_arrived,
				self);
	}

	DEBUG_END();
}

static void ecg_view_btn_close_clicked(GtkWidget *widget, gpointer user_data)
{
	EcgViewData *self = (EcgViewData *)user_data;
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	ecg_view_stop_drawing(self);

	DEBUG_END();
}

static gboolean ecg_view_expose(
		GtkWidget *widget,
		GdkEventExpose *event,
		gpointer user_data)
{
	EcgViewData *self = (EcgViewData *)user_data;
	g_return_val_if_fail(self != NULL, FALSE);

	if(!self->is_drawing)
	{
		return TRUE;
	}

	gdk_draw_drawable(widget->window,
			widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
			GDK_DRAWABLE(self->pixmap),
			0, 0,
			0, 0,
			ECG_VIEW_WIDTH, ECG_VIEW_HEIGHT);

	return TRUE;
}

