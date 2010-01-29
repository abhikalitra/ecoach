/*
 *  eCoach
 *
 *  Copyright (C) 2008  Jukka Alasalmi, Sampo Savola
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

#ifndef _ECG_DATA_H
#define _ECG_DATA_H

/* Configuration */
#include "config.h"

/* GLib */
#include <glib.h>
#include <glib/gtypes.h>

/* Other modules */
#include "gconf_helper.h"

#define EC_MAX_NUM_EVENTS   20

typedef struct _EcgData EcgData;

/**
 * @brief Type definition for ecg data callback
 *
 * @todo update documentation if required
 *
 * @param self Pointer to #EcgData
 * @param data Ecg data buffer
 * @param offset Start point of the newly arrived data
 * @param len Length of the newly arrived data
 * @param user_data User data that was set for the callback
 */
typedef void (*EcgDataFunc)
	(EcgData *self,
	gint heart_rate, gpointer *user_data);

typedef enum _EcgDataConnectionStatus {
	ECG_DATA_DISCONNECTED,
	ECG_DATA_CONNECTING,
	ECG_DATA_CONNECTED,
	ECG_DATA_REQUEST_DISCONNECT,
	ECG_DATA_DISCONNECTING
} EcgDataConnectionStatus;

typedef enum _HRMName{
	FRWD,
	ZEPHYR,
	POLAR
} HRMName;
/**
 * @brief Struct to hold data for a callback
 */
typedef struct _EcgDataCallbackData {
		EcgDataFunc callback;
		gpointer user_data;
} EcgDataCallbackData;

struct _EcgData {
	/**
	 * @brief Sample rate (in Hz)
	 *
	 * If you need to retrieve this, use the #ecg_data_get_sample_rate()
	 * function
	 */
	gint sample_rate;

	GConfHelperData *gconf_helper;

	/**
	 * @brief List of callbacks
	 */
	GSList *callbacks;
	
	/**
	 * @brief ECG data from the heart rate monitor is in signed char
	 * format.
	 *
	 * If this is needed, connecting to a callback is the right way
	 * to do it.
	 */
	GByteArray *voltage_array;

	/**
	 * @brief Time stamps of the events.
	 *
	 * This is a public, read-only field
	 *
	 * @todo What kind of time format?
	 * 
	 * @todo This should be probably retrieved via a function.
	 */
	guint events[EC_MAX_NUM_EVENTS];
	
	/**
	 * @brief The time of the latest arrived sample.
	 *
	 * This is a public, read-only field.
	 *
	 * @todo What kind of format?
	 *
	 * @todo Retrieve only via a function.
	 */
	guint current_time;

	/**
	 * @brief The battery level in percents
	 */
	guint battery_level;

	/**
	 * @brief FIFO-like buffer for the ECG data
	 *
	 * Consider this field private.
	 */
	GByteArray *buffer;

	/**
	 * @brief The sequence number of current data packet.
	 *
	 * This an unsigned, 12-bit number, so it fits into a signed
	 * (32-bit) integer. The negative value is used for marking
	 * 'no data processed yet'.
	 *
	 * Consider this field private.
	 */
	gint current_sequence_number;

	/**
	 * @brief Last processed location in the buffer.
	 *
	 * This must point to a synchronization mark after it.
	 *
	 * Consider this field private.
	 *
	 * @todo Remove this from the code
	 */
	guint last_processed_location;

	gboolean in_sync;

	/** @brief Bluetooth address of the ECG device */
	gchar *bluetooth_address;

	/** @brief Whether or not connected to the device */
	gboolean connected;

	gint bluetooth_serial_fd;

	/** @brief The file descriptors for the Bluetooth serial data pipes */
	gint bluetooth_serial_pipe[2];

	/** @brief The writable end of the pipe */
	GIOChannel *bluetooth_serial_channel_write;

	/** @brief The readable end of the pipe */
	GIOChannel *bluetooth_serial_channel_read;

	/** @brief The watch id for the readable end of the pipe */
	guint bluetooth_serial_channel_read_watch_id;

	/** @brief Thread for reading data from the rfcomm device */
	GThread *bluetooth_poll_thread;

	/** @brief Connection status */
	EcgDataConnectionStatus connection_status;
	GMutex *connection_status_mutex;
	
	gint hr;
	
	gchar *bluetooth_name;
	HRMName *hrm_name;
	gint hr1,hr2,hr3,count;
};

/**
 * @brief Create a new EcgData object.
 *
 * @param gconf_helper Pointer to #GConfHelperData
 * @return Newly allocated EcgData object, or NULL in case of an error
 */
EcgData *ecg_data_new(GConfHelperData *gconf_helper);

/**
 * @brief Add a callback that is invoked when new ECG data arrives.
 *
 * @note Whenever there are callbacks registered, the connection to ECG
 * device is established and data polling started. Therefore, adding a
 * callback might sometimes last a considerable amount of time, and it might
 * even fail if the connection to the ECG monitor fails.
 *
 * @param self Pointer to #EcgData
 * @param callback Function to be called
 * @param user_data User data pointer passed to the callback
 * @param error Return location for possible error
 *
 * @return TRUE on success, FALSE on failure
 *
 * @note If you set the user_data to be NULL, when you remove that callback
 * with #ecg_data_remove_callback_ecg, <em>all callbacks registered to that
 * callback function will be removed</em>. That is because a NULL
 * user_data acts as a wildcard for the #ecg_data_remove_callback_ecg.
 */
gboolean ecg_data_add_callback_ecg(
		EcgData *self,
		EcgDataFunc callback,
		gpointer user_data,
		GError **error);

/**
 * @brief Remove a callback.
 *
 * If a parameter is NULL, it is considered to be a wildcard.
 * Calling this function with callback and user_data as NULL will remove all
 * callbacks.
 *
 * @note When the last callback is removed, connection to ECG device is
 * closed and data polling stopped.
 *
 * @param self Pointer to #EcgData (must not be NULL)
 * @param callback Callback function
 * @param user_data User data that was passed to the callback
 */
void ecg_data_remove_callback_ecg(
		EcgData *self,
		EcgDataFunc callback,
		gpointer user_data);

/**
 * @brief Retrieve sample rate.
 *
 * @note You should only retrieve this when enough data has been retrieved
 * from ECG monitor. A good place to get this is from a callback invoked
 * by #EcgData, since that is only called when enough data has arrived.
 *
 * @param self Pointer to #EcgData
 */
gint ecg_data_get_sample_rate(EcgData *self);

/**
 * @brief Retrieve units per mV
 *
 * Units per mV means that how many units one mV is.
 *
 * @param self Pointer to #EcgData
 *
 * @note This is a hardcoded value
 */
gint ecg_data_get_units_per_mv(EcgData *self);

/**
 * @brief Retrieve zero level
 *
 * Zero level where is the zero voltage level.
 *
 * @param self Pointer to #EcgData
 *
 * @note This is a hardcoded value
 */
gint ecg_data_get_zero_level(EcgData *self);

#endif /* _ECG_DATA_H */
