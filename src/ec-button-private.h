/* Generated by GOB (v2.0.16)   (do not edit directly) */

#ifndef __EC_BUTTON_PRIVATE_H__
#define __EC_BUTTON_PRIVATE_H__

#include "ec-button.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _EcButtonPrivate {
#line 20 "ec-button.gob"
	char * label_text;
#line 23 "ec-button.gob"
	char * title_text;
#line 26 "ec-button.gob"
	gint btn_down_offset;
#line 34 "ec-button.gob"
	gboolean center_vertically;
#line 50 "ec-button.gob"
	gboolean center_text_vertically;
#line 66 "ec-button.gob"
	PangoLayout * layout_label;
#line 74 "ec-button.gob"
	PangoLayout * layout_title;
#line 82 "ec-button.gob"
	GdkPixbuf * bg_pixbuf[EC_BUTTON_STATE_COUNT];
#line 94 "ec-button.gob"
	GdkPixbuf * icon_pixbuf;
#line 97 "ec-button.gob"
	EcButtonState state;
#line 98 "ec-button.gob"
	gboolean cursor_inside;
#line 36 "ec-button-private.h"
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
