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

#ifndef _SETTINGS_H
#define _SETTINGS_H

/*****************************************************************************
 * Includes                                                                  *
 *****************************************************************************/

/* Configuration */
#include "config.h"

/* Other modules */
#include "gconf_helper.h"

/*****************************************************************************
 * Type definitions                                                          *
 *****************************************************************************/

typedef struct _Settings Settings;

/*****************************************************************************
 * Function prototypes                                                       *
 *****************************************************************************/

Settings *settings_initialize(GConfHelperData *gconf_helper);

gboolean settings_get_ignore_time_zones(Settings *self);
void settings_set_ignore_time_zones(Settings *self, gboolean ignore_time_zones);

#endif /* _SETTINGS_H */
