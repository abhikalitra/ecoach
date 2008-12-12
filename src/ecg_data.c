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

/*****************************************************************************
 * Includes                                                                  *
 *****************************************************************************/

/* This module */
#include "ecg_data.h"

/* System */
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <time.h>
#include <string.h>			/* for strerror() */
#include <unistd.h>

/* Other modules */
#include "gconf_keys.h"
#include "ec_error.h"

#include "debug.h"

/****************************************************************************
 * Definitions                                                              *
 ****************************************************************************/

#define ECG_CHUNK_HEADER_LEN			6
#define ECG_PACKET_HEADER_LEN			5
#define ECG_PACKET_ID_ECG			'\xAA'
#define ECG_PACKET_ID_ACC_2			'\x55'
#define ECG_PACKET_ID_ACC_3			'\x56'
#define ECG_DATA_POLLING_STOP_CHECK_INTERVAL	15
#define ECG_DATA_READ_BUFFER_SIZE		1024
#define FRWD_PACKET_SIZE			93
/****************************************************************************
 * Private function prototypes                                              *
 ****************************************************************************/

static void ecg_data_invoke_callbacks(EcgData *self, gint heart_rate);
static void ecg_data_process(EcgData *self);
static gboolean ecg_data_process_data_chunk(EcgData *self);
static gint ecg_data_process_data_block(EcgData *self);
static gint ecg_data_process_ecg_data_block(EcgData *self);
static gint ecg_data_process_acc_data_block(EcgData *self, gint axis_count);
static gboolean ecg_data_synchronize(EcgData *self, gboolean force);
static void ecg_data_pop(EcgData *self, guint len, guint8 *checksum);

static gboolean ecg_data_connect_bluetooth(EcgData *self, GError **error);
static void ecg_data_disconnect_bluetooth(EcgData *self);
static void ecg_data_wait_for_disconnect(EcgData *self);
static gboolean ecg_data_setup_serial_pipe(EcgData *self, GError **error);
static gboolean frwd_parse_heartrate(EcgData *self,gchar* frwd);
/**
 * @brief Remove unnecessary voltage data from the array.
 *
 * @param self Pointer to #EcgData
 * @param len Amount of bytes to remove
 *
 * @todo Do this automatically, as no single data getter can remove
 * data anymore (others need it, too)
 */
static void ecg_data_dispose_voltage_data(EcgData *self, guint len);


/**
 * @brief Read data from rfcomm socket and push it into a pipe.
 *
 * @param self Pointer to #EcgData
 *
 * @returns FALSE on critical error, TRUE otherwise.
 */
static gboolean ecg_data_read_and_push_bluetooth_data(EcgData *self);

static gpointer ecg_data_bluetooth_poller(gpointer user_data);

static void ecg_data_receive_data(
		GIOChannel *channel,
		guchar *buffer,
		gsize count);

static void ecg_data_send_data(
		GIOChannel *channel,
		guchar *buf,
		gssize count,
		gboolean flush);

/**
 * @brief Callback for setting the bluetooth address of the ECG device
 *
 * @param entry GConfEntry that contains the data
 * @param user_data Pointer to #EcgData
 */
static void ecg_data_set_bluetooth_address(
		const GConfEntry *entry,
		gpointer user_data,
		gpointer user_data_2);

/**
 * @brief Push new raw data to the buffer.
 *
 * The pushed data will be removed automatically when it is no longer
 * needed.
 *
 * @param self Pointer to #EcgData
 * @param data The new data
 * @param len The length of the data
 */
static void ecg_data_push(EcgData *self, const guint8 *data, guint len);

/*****************************************************************************
 * Function declarations                                                     *
 *****************************************************************************/

/*===========================================================================*
 * Public function declarations                                              *
 *===========================================================================*/

EcgData *ecg_data_new(GConfHelperData *gconf_helper)
{
	g_return_val_if_fail(gconf_helper != NULL, NULL);

	DEBUG_BEGIN();

	EcgData *self = g_new0(EcgData, 1);
	if(!self)
	{
		g_critical("Not enough memory");
		DEBUG_END();
		return NULL;
	}

	self->gconf_helper = gconf_helper;

	self->buffer = g_byte_array_new();
	self->voltage_array = g_byte_array_new();
	self->connection_status_mutex = g_mutex_new();
	self->connection_status = ECG_DATA_DISCONNECTED;

	self->current_sequence_number = -1;
	self->bluetooth_serial_fd = -1;

	gconf_helper_add_key_string(
		gconf_helper,
		ECGC_BLUETOOTH_ADDRESS,
		"",
		ecg_data_set_bluetooth_address,
		self,
		NULL);

	DEBUG_END();
	return self;
}

void ecg_data_destroy(EcgData *self)
{
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	ecg_data_remove_callback_ecg(self, NULL, NULL);
	ecg_data_wait_for_disconnect(self);

	g_mutex_free(self->connection_status_mutex);
	g_byte_array_free(self->buffer, TRUE);
	g_byte_array_free(self->voltage_array, TRUE);

	g_free(self);
	DEBUG_END();
}

gboolean ecg_data_add_callback_ecg(
		EcgData *self,
		EcgDataFunc callback,
		gpointer user_data,
		GError **error)
{
	gboolean first = FALSE;
	EcgDataCallbackData *cb_data = NULL;
	EcgDataConnectionStatus status = ECG_DATA_DISCONNECTED;
	gboolean bluetooth_connection_ok = TRUE;

	g_return_val_if_fail(error == NULL || *error == NULL, FALSE);
	g_return_val_if_fail(self != NULL, FALSE);
	g_return_val_if_fail(callback != NULL, FALSE);
	DEBUG_BEGIN();

	if(self->callbacks == NULL)
	{
		DEBUG_LONG("First callback added. Connecting to ECG monitor");
		first = TRUE;
	}

	if(first)
	{
		g_mutex_lock(self->connection_status_mutex);
		status = self->connection_status;
		g_mutex_unlock(self->connection_status_mutex);

		switch(status)
		{
			case ECG_DATA_DISCONNECTED:
				bluetooth_connection_ok =
					ecg_data_connect_bluetooth(self,
							error);
				break;
			case ECG_DATA_CONNECTED:
			case ECG_DATA_CONNECTING:
				g_critical("Already connecting or connected, "
						"even though this is the first "
						"callback.\n"
						"This shouldn't happen");
				/* Don't do anything */
				break;
			case ECG_DATA_REQUEST_DISCONNECT:
			case ECG_DATA_DISCONNECTING:
				/* Wait for the connection to be closed
				 * and then connect again */
				ecg_data_wait_for_disconnect(self);
				bluetooth_connection_ok =
					ecg_data_connect_bluetooth(self,
							error);
				break;
		}
	}

	if(bluetooth_connection_ok)
	{
		cb_data = g_new0(EcgDataCallbackData, 1);

		cb_data->callback = callback;
		cb_data->user_data = user_data;

		self->callbacks = g_slist_append(self->callbacks, cb_data);
	}

	return bluetooth_connection_ok;

	DEBUG_END();
}

void ecg_data_remove_callback_ecg(
		EcgData *self,
		EcgDataFunc callback,
		gpointer user_data)
{
	GSList *temp = NULL;
	GSList *indices = NULL;
	GSList *to_remove = NULL;

	gboolean match = TRUE;
	EcgDataCallbackData *cb_data = NULL;
	gint index = 0;

	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	if(self->callbacks == NULL)
	{
		/* There are no callbacks. Nothing to be done */
		DEBUG_END();
		return;
	}

	for(temp = self->callbacks; temp; temp = g_slist_next(temp))
	{
		match = TRUE;
		cb_data = (EcgDataCallbackData *)temp->data;
		if(callback)
		{
			if(callback != cb_data->callback)
			{
				match = FALSE;
			}
		}
		if(user_data)
		{
			if(user_data != cb_data->user_data)
			{
				match = FALSE;
			}
		}
		if(match)
		{
			/* We can't just remove the item, because the
			 * list may be relocated, which would break the
			 * looping. Instead, append the index to another
			 * list */
			indices = g_slist_append(indices,
					GINT_TO_POINTER(index));
		}
		index++;
	}

	/* Reverse the list of indices, as we must go from end to begin
	 * to avoid shifting in the list */
	indices = g_slist_reverse(indices);

	for(temp = indices; temp; temp = g_slist_next(temp))
	{
		index = GPOINTER_TO_INT(temp->data);
		DEBUG_LONG("Removing callback %d", index);
		to_remove = g_slist_nth(self->callbacks, index);
		cb_data = (EcgDataCallbackData *)to_remove->data;
		g_free(cb_data);
		self->callbacks = g_slist_delete_link(
				self->callbacks,
				to_remove);
	}

	if(self->callbacks == NULL)
	{
		DEBUG_LONG("Last callback removed. Stopping ECG");

		/* Last callback was removed. Stop ECG monitoring */
		ecg_data_disconnect_bluetooth(self);
	}
	DEBUG_END();
}

gint ecg_data_get_sample_rate(EcgData *self)
{
	g_return_val_if_fail(self != NULL, 0);
	return self->sample_rate;
}

gint ecg_data_get_units_per_mv(EcgData *self)
{
	g_return_val_if_fail(self != NULL, 0);
	return 50;
}

gint ecg_data_get_zero_level(EcgData *self)
{
	g_return_val_if_fail(self != NULL, 0);
	return 128;
}

/*===========================================================================*
 * Private function declarations                                             *
 *===========================================================================*/

static void ecg_data_invoke_callbacks(EcgData *self, gint heart_rate)
{
	GSList *temp = NULL;
	EcgDataCallbackData *cb_data = NULL;

	DEBUG_BEGIN();

	for(temp = self->callbacks; temp; temp = g_slist_next(temp))
	{
		cb_data = (EcgDataCallbackData *)temp->data;
		if(cb_data->callback)
		{
		cb_data->callback(self,heart_rate, cb_data->user_data);
		}
	}

	DEBUG_END();
}

static void ecg_data_push(EcgData *self, const guint8 *data, guint len)
{
	g_return_if_fail(self != NULL);
	g_return_if_fail(data != NULL);

	DEBUG_BEGIN();

	DEBUG("Pushing %d bytes of data to buffer", len);
	self->buffer = g_byte_array_append(self->buffer, data, len);

	ecg_data_process(self);

	DEBUG_END();
}

/**
 * @brief Process newly arrived ecg data.
 *
 * This will update voltage, events and current_time in the
 * EcgData struct.
 *
 * @param self Pointer to #EcgData
 */
static void ecg_data_process(EcgData *self)
{
	g_return_if_fail(self != NULL);

	DEBUG_BEGIN();
	
	/*gint i;
	gint offset = 0;
	gint syke = 0;
	
	gchar *pointer = g_strstr_len((const gchar *)self->buffer->data, self->buffer->len, "FRWD");

	DEBUG("Pointer: %p, data: %p", pointer, 
	      self->buffer->data);
	
	if(pointer != NULL){
		
		offset = pointer - (gchar *)self->buffer->data;	
		ecg_data_pop(self,offset,NULL);
		frwd_parse_heartrate(self,pointer);
		ecg_data_invoke_callbacks(self,self->hr);
		}
	ecg_data_pop(self,93,NULL);
	*/
	gint offset = 0;

	while(self->buffer->len > 0)
	{
		gchar *pointer = g_strstr_len((const gchar *)self->buffer->data,
					       self->buffer->len,
	    "FRWD");
		if(!pointer)
		{
			/* No FRWD data. Clear buffer and wait for more data. */
			ecg_data_pop(self, self->buffer->len, NULL);
			break;
		}

		/* Remove non-FRWD data from the beginning of the buffer */
		offset = pointer - (gchar *)self->buffer->data;
		if(offset > 0)
		ecg_data_pop(self, offset, NULL);

		if(frwd_parse_heartrate(self, pointer))
		{
			/* Remove parsed data */
			ecg_data_pop(self, FRWD_PACKET_SIZE,NULL);
		}
		else {
			/*Wait for more data */
			break;
		}
		/* Continue until the buffer is empty */
	}

	DEBUG_END();
}

/**
 * @brief Process one data chunk from buffer.
 *
 * Data chunk consists of several data blocks. These data blocks have one
 * common header, and also each one has an individual header.
 *
 * @param self Pointer to #EcgData
 *
 * @return TRUE if the data block was complete, FALSE if not.
 */
static gboolean ecg_data_process_data_chunk(EcgData *self)
{
	static gint data_block_count = -1;
	static gint current_data_block = 0;
	gint data_block_offset = 0;
	guint16 sequence_number = 0;
	guint8 seq_number_temp = 0;
	gint i = 0;
	gint retval = 0;
	gboolean was_ok = TRUE;
	static guint8 checksum = 0;

	g_return_val_if_fail(self != NULL, FALSE);
	DEBUG_BEGIN();
	
	

	
	if(data_block_count == -1)
	{
		/* We are processing a yet unprocessed data chunk. */

		/* Check that we have the full packet header. If not, we'll get
		 * back here later. */
		if(self->buffer->len < ECG_PACKET_HEADER_LEN)
		{
			DEBUG_LONG("Packet header incomplete. Waiting for more "
					"data");
			DEBUG_END();
			return FALSE;
		}

		/* Just a check to be sure */
		if(!(
					(self->buffer->data[0] == 0x00) &&
					(self->buffer->data[1] == 0xFE)
		    ))
		{
			g_critical("Sync mark is not where it is supposed "
					"to be.");
			DEBUG_END();
			return ecg_data_synchronize(self, FALSE);
		}

		self->battery_level = ((guint8)self->buffer->data[2]) / 2;
		/* Get the most significant byte and shift it */
		sequence_number = ((guint16)(self->buffer->data[3])) & 0x0F;
		sequence_number = sequence_number << 8;

		/* Get the four first butes from the end part, and shift it */
		sequence_number += (guint16)(self->buffer->data[4]);
		sequence_number += (guint16)seq_number_temp;

		if(self->buffer->data[3] && (1 << 4))
		{
			/**
			 * @todo: How is the exact position of the event
			 * retrieved?
			 *
			 * Answer: there is no way (confirmed from alive
			 * technologies; however, the time window for the
			 * event is only about 250 ms)
			 */
			DEBUG_LONG("There was an event.");
		}

		DEBUG_LONG("Packet sequence number: %d", (gint)sequence_number);

		if(self->current_sequence_number != -1)
		{
			if(sequence_number != self->current_sequence_number + 1)
			{
				g_warning("Unexpected data sequence number:\n"
						"%d (expected %d)",
						sequence_number,
						self->current_sequence_number
						+ 1);
			}
		}

		self->current_sequence_number = (gint)sequence_number;

		/* How many data blocks are there? */
		data_block_count = (gint)((guint8)(self->buffer->data[5]));
		DEBUG_LONG("%d data blocks", data_block_count);

		/* We have now read the whole header and stored the extracted
		 * data, so it can be removed from the buffer now */
		// self->last_processed_location = ECG_CHUNK_HEADER_LEN;

		checksum = 0;
		ecg_data_pop(self, ECG_CHUNK_HEADER_LEN, &checksum);

		/* @todo Is this variable needed anymore? */
		data_block_offset = 0;
	} else {
		/* Continue from the first unprocessed data block in the
		 * current data chunk */
		DEBUG_LONG("Continuing processing of data chunk %d",
				self->current_sequence_number);
		data_block_offset = 0;
	}

	for(i = current_data_block; i < data_block_count; i++)
	/* for(i = 0; i < data_block_count; i++) */
	{
		DEBUG_LONG("Processing data block %d", i);
		retval = ecg_data_process_data_block(self);
		switch(retval)
		{
			case -1:
				/* Not enough data */
				current_data_block = i;
				return FALSE;
			case -2:
				/* Invalid data. Synchronize and start over,
				 * if synchronization was OK */
				was_ok = FALSE;
				break;
			default:
				if(retval < 1)
				{
					g_critical("Invalid response from "
						"ecg_data_process_data_block:"
						" %d", retval);
					/* Just to be sure, synchronize and
					 * start over, if synchronization was
					 * OK */
					was_ok = FALSE;
					break;
				}

				/* Everything OK. Remove the data that was
				 * processed. */
				/* self->last_processed_location =
					data_block_offset; */
				ecg_data_pop(self, retval, &checksum);
				data_block_offset = 0;
				break;

		}
		if(!was_ok)
		{
			break;
		}
	}

	if(!was_ok)
	{
		goto resync_required;
	}

	/* Verify the checksum and remove it */
	if(checksum != (guint8)self->buffer->data[0])
	{
		g_warning("Checksum does not match (%d ; %d)",
				checksum, (guint8)self->buffer->data[0]);
		goto resync_required;
	} else {
		DEBUG_LONG("Checksum OK");
	}
	/* Remove the checksum from the buffer */

	ecg_data_pop(self, 1, NULL);
	/* All data blocks in the data chunk were processed OK. Reset
	 * the required variables. */
	current_data_block = 0;
	data_block_count = -1;

	/** @todo update this only after processing whole data unit */
	self->current_sequence_number = sequence_number;

	/* Return TRUE so that the next block will be processed (if
	 * it happens to be in the buffer) */
	DEBUG_END();
	return TRUE;

resync_required:
	/* If a resynchronization is required, we must reset the variables */
	current_data_block = 0;
	data_block_count = -1;
	/* If there was data corruption, also the current sequence number
	 * might be wrong */
	self->current_sequence_number = -1;
	DEBUG_END();
	return ecg_data_synchronize(self, TRUE);
}

/**
 * @brief Process one data block from the ecg data.
 *
 * @param self Pointer to #EcgData
 * 
 * @return Length of the processed data block on success, -1 if there was
 * not enough data and -2 if the data was invalid (-2 means that the stream
 * should be synchronized again, as there is probably data corruption).
 */
static gint ecg_data_process_data_block(EcgData *self)
{
	gint retval = 1;

	g_return_val_if_fail(self != NULL, -2);
	DEBUG_BEGIN();

	if(self->buffer->len < ECG_PACKET_HEADER_LEN)
	{
		DEBUG("Not enough data yet.");
		return -1;
	}

	/* Determine the packet type: ECG, 2 Axis accelerometer or
	 * 3 axis accelerometer */
	switch(self->buffer->data[0])
	{
		case ECG_PACKET_ID_ECG:
			retval = ecg_data_process_ecg_data_block(self);
			break;
		case ECG_PACKET_ID_ACC_2:
			retval = ecg_data_process_acc_data_block(self, 2);
			break;
		case ECG_PACKET_ID_ACC_3:
			retval = ecg_data_process_acc_data_block(self, 3);
			break;
		default:
			g_warning("Unknown data packet ID: 0x%X",
					self->buffer->data[0]);
			DEBUG_END();
			return -2;
	}

	DEBUG_END();
	return retval;
}

/**
 * @brief Extract ecg data from a data block
 *
 * @param self Pointer to #EcgData
 *
 * @return Length of the processed data block on success, -1 if there was
 * not enough data and -2 if the data was invalid (-2 means that the stream
 * should be synchronized again, as there is probably data corruption).
 */
static gint ecg_data_process_ecg_data_block(EcgData *self)
{
	guint data_block_length = 0;
	guint ecg_data_offset = 0;

	g_return_val_if_fail(self != NULL, -2);
	DEBUG_BEGIN();

	data_block_length = self->buffer->data[1];
	data_block_length = data_block_length << 8;
	data_block_length += self->buffer->data[2];
	DEBUG("Data block length: %d", data_block_length);

	if(self->buffer->len < data_block_length)
	{
		DEBUG("Not enough data yet.");
		return -1;
	}

	switch(self->buffer->data[3])
	{
		case '\x01':
			DEBUG("150 samples per second");
			self->sample_rate = 150;
			break;
		case '\x02':
			DEBUG("300 samples per second");
			self->sample_rate = 300;
			break;
		default:
			g_warning("Unknown ECG data format ID: 0x%X",
					self->buffer->data[3]);
			return -2;
	}

	ecg_data_offset = self->voltage_array->len;

	self->voltage_array = g_byte_array_append(
			self->voltage_array,
			self->buffer->data + ECG_PACKET_HEADER_LEN,
			data_block_length - ECG_PACKET_HEADER_LEN);

	/*
	ecg_data_invoke_callbacks(self, ecg_data_offset,
			data_block_length - ECG_PACKET_HEADER_LEN);
*/
	/** @todo: Is the voltage buffer even needed anymore? */
	ecg_data_dispose_voltage_data(self, data_block_length -
			ECG_PACKET_HEADER_LEN);

	DEBUG_END();
	return data_block_length;
}

/**
 * @brief Extract accelerometer data from a data block
 *
 * @param self Pointer to #EcgData
 * @param axis_count Axis count of the accelerometer
 *
 * @return Length of the processed data block on success, -1 if there was
 * not enough data and -2 if the data was invalid (-2 means that the stream
 * should be synchronized again, as there is probably data corruption).
 */
static gint ecg_data_process_acc_data_block(EcgData *self, gint axis_count)
{
	guint data_block_length = 0;
	guint ecg_data_offset = 0;

	g_return_val_if_fail(self != NULL, -2);
	g_return_val_if_fail(axis_count == 2 || axis_count == 3, -2);

	DEBUG_BEGIN();

	data_block_length = self->buffer->data[1];
	data_block_length = data_block_length << 8;
	data_block_length += self->buffer->data[2];
	DEBUG("Data block length: %d (0x%X)", data_block_length,
			data_block_length);

	if(self->buffer->len < data_block_length)
	{
		DEBUG("Not enough data yet.");
		return -1;
	}

	if(self->buffer->data[3] != '\x00')
	{
		g_warning("Invalid accelerometer data format: 0x%X",
				self->buffer->data[3]);
		return -2;
	}

	ecg_data_offset = self->voltage_array->len;

#if 0
	/** @todo Extract the acc data */
	gint i;
	for(i = 0; i < data_block_length - ECG_PACKET_HEADER_LEN; i++)
	{
		printf("0x%X\t", self->buffer->data[ECG_PACKET_HEADER_LEN + i]);
		if((i % 8) == 7)
		{
			printf("\n");
		}
	}
	printf("\n");

	self->voltage_array = g_byte_array_append(
			self->voltage_array,
			self->buffer->data + ECG_PACKET_HEADER_LEN,
			data_block_length - ECG_PACKET_HEADER_LEN);

	if(self->ecg_callback)
	{
		self->ecg_callback(
				self,
				self->voltage_array,
				ecg_data_offset,
				data_block_length - ECG_PACKET_HEADER_LEN,
				self->ecg_callback_data);
	}

#endif
	DEBUG_END();
	return data_block_length;
}

/**
 * @brief Search for the synchronization bytes in the data
 * 
 * @param self Pointer to #EcgData
 * @param force If set to TRUE, a resynchronization is forced (this means,
 * that the sync mark at [0-1] is ignored.
 */
static gboolean ecg_data_synchronize(EcgData *self, gboolean force)
{
	/**
	 * @todo Can we rely only on the 0x00 0xFE sync mark?
	 * It is quite unlikely for this to happen randomly, as
	 * it means that voltage should rise from minimum to
	 * <strong>almost</strong> maximum in two consecutive samples.
	 * Or that the headers would contain this.
	 *
	 * On the other hand, resynchronization is done, if something doesn't
	 * match later (i.e., invalid chunk/packet headers etc.)
	 */

	gint i = 0;

	g_return_val_if_fail(self != NULL, FALSE);
	DEBUG_BEGIN();

	if(force)
	{
		/* Check that there is room even for the initial sync mark */
		if(self->buffer->len < 2)
		{
			DEBUG_END();
			return FALSE;
		}
		/* Remove the original sync mark */
		//self->last_processed_location = 1;
		ecg_data_pop(self, 2, NULL);
	}

	/* Loop only to buffer->len - 1, because the sync mark is
	 * two bytes */
	for(i = 0; i < self->buffer->len - 1; i++)
	{
		if(self->buffer->data[i] ==  0x00)
		{
			if(self->buffer->data[i+1] == 0xFE)
			{
				/* Throw away anything before sync mark,
				 * because it might be anything. We don't
				 * have a header for it. */
				DEBUG("Found sync mark at %d (%X)", i, i);
				if(i < self->buffer->len - 5)
				{
					DEBUG("Battery level seems now to be %d (%X)",
							(guint8)self->buffer->data[i+2],
							(guint8)self->buffer->data[i+2]);
				}

				if(i > 0)
				{
					DEBUG("Removing %d unnecessary bytes",
							i);
					// self->last_processed_location = i;
					ecg_data_pop(self, i, NULL);
				}
				// self->last_processed_location = 0;
				self->in_sync = TRUE;
				DEBUG_END();
				return TRUE;
			}
		}
	}

	/* The sync mark was not found. Set the in_sync to FALSE,
	 * because this might be sometimes called even if synchronization
	 * was at some point reached */
	self->in_sync = FALSE;
	DEBUG_END();
	return FALSE;
}

/**
 * @brief Remove unneeded data from the beginning of the buffer.
 * 
 * This function is only used internally.
 *
 * @param self Pointer to EcgData
 * @param len Amount of bytes to remove
 * @param checksum Pointer to checksum, or NULL if the checksum is not wanted
 */
static void ecg_data_pop(EcgData *self, guint len, guint8 *checksum)
{
	guint8 *data_ptr;

	g_return_if_fail(self != NULL);
	g_return_if_fail(len <= self->buffer->len);
	g_return_if_fail(len > 0);

	DEBUG_BEGIN();

	/*
	if(self->last_processed_location + 1 < len)
	{
		g_critical("Attempt to removed unprocessed Ecg data (%d/%d)",
				self->last_processed_location + 1, len);
		DEBUG_END();
		return;
	}
	*/

	if(checksum)
	{
		/* Add to the checksum of the removed data */
		data_ptr = self->buffer->data;
		for(data_ptr = self->buffer->data;
				data_ptr < self->buffer->data + len;
				data_ptr++)
		{
			*checksum += *((guint8 *)data_ptr);
		}
	}

	// self->last_processed_location = self->last_processed_location + 1 - len;
	DEBUG("Removing %d bytes", len);
	self->buffer = g_byte_array_remove_range(
			self->buffer,
			0, len);

	DEBUG_END();
}

static void ecg_data_dispose_voltage_data(EcgData *self, guint len)
{
	DEBUG_BEGIN();
	self->voltage_array = g_byte_array_remove_range(
			self->voltage_array,
			0, len);
	DEBUG_END();
}

/*---------------------------------------------------------------------------*
 * Bluetooth connection related functions                                    *
 *---------------------------------------------------------------------------*/

static void ecg_data_set_bluetooth_address(
		const GConfEntry *entry,
		gpointer user_data,
		gpointer user_data_2)
{
	GConfValue *value = NULL;
	const gchar *bluetooth_address = NULL;
	EcgData *self = (EcgData *)user_data;

	g_return_if_fail(self != NULL);
	g_return_if_fail(entry != NULL);

	DEBUG_BEGIN();

	value = gconf_entry_get_value(entry);
	bluetooth_address = gconf_value_get_string(value);

	if(self->bluetooth_address)
	{
		g_free(self->bluetooth_address);
	}

	self->bluetooth_address = g_strdup(bluetooth_address);

	DEBUG_END();
}

static gboolean ecg_data_bluetooth_data_arrived(
		GIOChannel *source,
		GIOCondition condition,
		gpointer user_data)
{
	gchar str_ret[ECG_DATA_READ_BUFFER_SIZE];

	EcgData *self = (EcgData *)user_data;

	g_return_val_if_fail(user_data != NULL, FALSE);
	DEBUG_BEGIN();

	guchar data_size_c[2];
	guint data_size;

	/* Read 2 characters to data_size_c */
	ecg_data_receive_data(
			self->bluetooth_serial_channel_read,
			data_size_c,
			2);

	data_size = (gint)data_size_c[1] + ((gint) data_size_c[0] << 8);

	/* Read <data_size> characters to str_ret */
	ecg_data_receive_data(
			self->bluetooth_serial_channel_read,
			str_ret,
			data_size);

	ecg_data_push(self, (gint8 *)str_ret, (guint)data_size);
	
	DEBUG_END();
	return TRUE;
}

static gboolean ecg_data_connect_bluetooth(EcgData *self, GError **error)
{
	struct sockaddr_rc addr = { 0 };
	gint status = 0;
	gint i = 0;
	gchar *err_msg;

	g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

	DEBUG_BEGIN();

	if(self->bluetooth_address == NULL)
	{
		g_set_error(error, EC_ERROR, EC_ERROR_HRM_NOT_CONFIGURED,
				"Heart rate monitor is not configured");
		return FALSE;
	}

	if(strcmp(self->bluetooth_address, "") == 0)
	{
		g_set_error(error, EC_ERROR, EC_ERROR_HRM_NOT_CONFIGURED,
				"Heart rate monitor is not configured");
		return FALSE;
	}

	g_mutex_lock(self->connection_status_mutex);
	self->connection_status = ECG_DATA_CONNECTING;
	g_mutex_unlock(self->connection_status_mutex);

	/* Create a socket */
	self->bluetooth_serial_fd = socket(
			AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
	if(self->bluetooth_serial_fd < 0)
	{
		/* @todo Error message */
		err_msg = strerror(errno);
		g_set_error(error, EC_ERROR, EC_ERROR_BLUETOOTH,
				err_msg);
		goto connection_failure;
	}

	addr.rc_family = AF_BLUETOOTH;

	/* Convert the Bluetooth address from string */
	str2ba(self->bluetooth_address, &addr.rc_bdaddr);

	for(i = 1; i <= 30; i++)
	{
	addr.rc_channel = (uint8_t)i;
	status = connect(self->bluetooth_serial_fd,(struct sockaddr *)&addr,
						    sizeof(addr));
		if(status == 0)
		{
			DEBUG("Available bluetooth port: %d", i);
			break;
		}
		if(errno == ECONNREFUSED){
		break;
		}
	}	
	if(status < 0)
	{
		int err_no = errno;
		g_message("Error: %s", strerror(errno));
		/* Connecting failed for all ports. */
		g_set_error(error, EC_ERROR, EC_ERROR_BLUETOOTH,
				"No Bluetooth ports available");
		goto connection_failure;
	}

	if(!ecg_data_setup_serial_pipe(self, error))
	{
		DEBUG_END();
		return FALSE;
	}

	DEBUG_END();
	return TRUE;

connection_failure:
	if(self->bluetooth_serial_fd != -1)
	{
		shutdown(self->bluetooth_serial_fd, SHUT_RDWR);
		self->bluetooth_serial_fd = -1;
	}

	g_mutex_lock(self->connection_status_mutex);
	self->connection_status = ECG_DATA_DISCONNECTED;
	g_mutex_unlock(self->connection_status_mutex);

	DEBUG_END();
	return FALSE;
}

static void ecg_data_disconnect_bluetooth(EcgData *self)
{
	g_return_if_fail(self != NULL);
	DEBUG_BEGIN();

	/* Set the connection status to REQEUST_DISCONNECT so that
	 * the data poller knows to stop polling
	 */
	g_mutex_lock(self->connection_status_mutex);
	self->connection_status = ECG_DATA_REQUEST_DISCONNECT;
	g_mutex_unlock(self->connection_status_mutex);

	/* Firstly, we remove the watch for the readable end of the serial
	 * channel so that we can immediately stop sending data to the
	 * listeners (that should happen automatically anyhow, though, because
	 * the callbacks have already been removed)
	 */
	g_source_remove(self->bluetooth_serial_channel_read_watch_id);
	self->bluetooth_serial_channel_read_watch_id = 0;

	DEBUG_END();
}

static void ecg_data_wait_for_disconnect(EcgData *self)
{
	struct timespec tv;
	struct timespec rem;
	EcgDataConnectionStatus status;

	DEBUG_BEGIN();

	g_mutex_lock(self->connection_status_mutex);
	status = self->connection_status;
	g_mutex_unlock(self->connection_status_mutex);

	if(status != ECG_DATA_REQUEST_DISCONNECT &&
	   status != ECG_DATA_DISCONNECTING &&
           status != ECG_DATA_DISCONNECTED)
	{
		ecg_data_disconnect_bluetooth(self);
	}

	do {
		tv.tv_sec = 1;
		tv.tv_nsec = 0;
		nanosleep(&tv, &rem);
		g_mutex_lock(self->connection_status_mutex);
		status = self->connection_status;
		g_mutex_unlock(self->connection_status_mutex);
	} while(status == ECG_DATA_REQUEST_DISCONNECT ||
		status == ECG_DATA_DISCONNECTING);
	DEBUG_END();
}

static gboolean ecg_data_setup_serial_pipe(EcgData *self, GError **error)
{
	gchar *err_msg = NULL;
	g_return_val_if_fail(self != NULL, FALSE);
	g_return_val_if_fail(error == NULL || *error == NULL, FALSE);
	DEBUG_BEGIN();

	if(pipe(self->bluetooth_serial_pipe) == -1)
	{
		err_msg = strerror(errno);
		g_set_error(error, EC_ERROR, EC_ERROR_PIPE,
				err_msg);
		/** @todo Disconnect bluetooth */
		return FALSE;
	}

	/* Create the writable channel for the pipe */
	self->bluetooth_serial_channel_write = g_io_channel_unix_new(
			self->bluetooth_serial_pipe[1]);

	/* Binary data, set NULL encoding */
	g_io_channel_set_encoding(self->bluetooth_serial_channel_write,
			NULL, NULL);

	/* Create the readable channel for the pipe */
	self->bluetooth_serial_channel_read = g_io_channel_unix_new(
			self->bluetooth_serial_pipe[0]);

	/* Binary data, set NULL encoding */
	g_io_channel_set_encoding(self->bluetooth_serial_channel_read,
			NULL, NULL);

	g_io_channel_set_buffered(self->bluetooth_serial_channel_read,
			FALSE);

	self->bluetooth_serial_channel_read_watch_id =
		g_io_add_watch(self->bluetooth_serial_channel_read,
				G_IO_IN,
				ecg_data_bluetooth_data_arrived,
				self);

	g_mutex_lock(self->connection_status_mutex);
	self->connection_status = ECG_DATA_CONNECTED;
	g_mutex_unlock(self->connection_status_mutex);

	/* Finally, create the thread for polling data from the actual
	 * rfcomm socket */
	self->bluetooth_poll_thread = g_thread_create(
			ecg_data_bluetooth_poller,
			self,
			FALSE,
			NULL);

	/** @todo error handling for glib functions */

	DEBUG_END();
	return TRUE;
}

static gpointer ecg_data_bluetooth_poller(gpointer user_data)
{
	EcgData *self = (EcgData *)user_data;
	struct timeval tv;
	fd_set readfs;
	GError *error = NULL;

	gboolean stop_thread = FALSE;

	g_return_val_if_fail(self != NULL, NULL);
	DEBUG_BEGIN();

	FD_ZERO(&readfs);
	FD_SET(self->bluetooth_serial_fd, &readfs);

	do {
		/* Check every N seconds if the polling should be stopped */
		tv.tv_sec = ECG_DATA_POLLING_STOP_CHECK_INTERVAL;
		tv.tv_usec = 0;

		switch(select(self->bluetooth_serial_fd + 1,
					&readfs,
					NULL, /* Ignore writefds */
					NULL, /* Ignore exceptfds */
					&tv))
		{
			case -1:
				g_critical("Select() call failed");
				stop_thread = TRUE;
				break;
			case 0:
				DEBUG("No data arrived yet");
				/* There was no data. This is normal */
				break;
			default:
				/* There is data available. Read it, and
				 * then push to the read pipe. */
				ecg_data_read_and_push_bluetooth_data(self);
		}
		if(!stop_thread)
		{
			g_mutex_lock(self->connection_status_mutex);
			stop_thread = (self->connection_status ==
				ECG_DATA_REQUEST_DISCONNECT);
			g_mutex_unlock(self->connection_status_mutex);
		}
	} while(!stop_thread);

	DEBUG_LONG("Disconnecting ECG Bluetooth");

	g_mutex_lock(self->connection_status_mutex);
	self->connection_status = ECG_DATA_DISCONNECTING;
	g_mutex_unlock(self->connection_status_mutex);

	/* Close all the IO channels */
	DEBUG("Shutting down bluetooth_serial_channel_write");
	g_io_channel_shutdown(self->bluetooth_serial_channel_write,
			FALSE, &error);
	if(error)
	{
		g_warning("Error shutting down GIOChannel: %s",
				error->message);
		g_error_free(error);
		error = NULL;
	}
	g_io_channel_unref(self->bluetooth_serial_channel_write);
	self->bluetooth_serial_channel_write = NULL;

	DEBUG("Shutting down bluetooth_serial_channel_read");
	g_io_channel_shutdown(self->bluetooth_serial_channel_read,
			FALSE, &error);
	if(error)
	{
		g_warning("Error shutting down GIOChannel: %s",
				error->message);
		g_error_free(error);
		error = NULL;
	}
	g_io_channel_unref(self->bluetooth_serial_channel_read);
	self->bluetooth_serial_channel_read = NULL;

	shutdown(self->bluetooth_serial_fd, SHUT_RDWR);
	self->bluetooth_serial_fd = -1;

	close(self->bluetooth_serial_pipe[0]);
	close(self->bluetooth_serial_pipe[1]);
	self->bluetooth_serial_pipe[0] = -1;
	self->bluetooth_serial_pipe[1] = -1;

	/* Empty the byte arrays */
	DEBUG("Clearing arrays");
	if(self->buffer->len > 0)
	{
		g_byte_array_remove_range(self->buffer, 0,
				self->buffer->len);
	}
	if(self->voltage_array->len > 0)
	{
		g_byte_array_remove_range(self->voltage_array, 0,
				self->voltage_array->len);
	}
	DEBUG("Cleared arrays");

	g_mutex_lock(self->connection_status_mutex);
	self->connection_status = ECG_DATA_DISCONNECTED;
	g_mutex_unlock(self->connection_status_mutex);

	DEBUG_END();
	return NULL;
}

static gboolean ecg_data_read_and_push_bluetooth_data(EcgData *self)
{
	size_t bufsize = ECG_DATA_READ_BUFFER_SIZE;
	ssize_t read_size = 0;

	guchar data_len[2];
	guchar buf[ECG_DATA_READ_BUFFER_SIZE];

	g_return_val_if_fail(self != NULL, FALSE);
	DEBUG_BEGIN();

	read_size = read(self->bluetooth_serial_fd, buf, bufsize);
	DEBUG("Received %d bytes", read_size);

	if(read_size < 1)
	{
		DEBUG_END();
		return TRUE;
	}

	data_len[1] = read_size & 0xFF;
	data_len[0] = read_size >> 8;

	ecg_data_send_data(self->bluetooth_serial_channel_write,
			data_len, 2, FALSE);
	ecg_data_send_data(self->bluetooth_serial_channel_write,
			buf, read_size, TRUE);

	DEBUG_END();
	return TRUE;
}

static void ecg_data_receive_data(
		GIOChannel *channel,
		guchar *buffer,
		gsize count)
{
	gint read_bytes;
	gint pos = 0;

	struct timespec tv;
	struct timespec rem;

	DEBUG_BEGIN();

	do {
		tv.tv_sec = 1;
		tv.tv_nsec = 0;

		g_io_channel_read_chars(channel,
				buffer + pos,
				count - pos,
				&read_bytes,
				NULL);
		pos = pos + read_bytes;

		if(pos < count)
		{
			nanosleep(&tv, &rem);
		}
	} while(pos < count);

	/** @todo error checking */
	DEBUG_END();
}

static void ecg_data_send_data(
		GIOChannel *channel,
		guchar *buf,
		gssize count,
		gboolean flush)
{
	struct timespec tv;
	struct timespec rem;
	gint position = 0;
	gsize wrote_size = 0;

	DEBUG_BEGIN();

	do {
		tv.tv_sec = 1;
		tv.tv_nsec = 0;

		g_io_channel_write_chars(
				channel,
				buf + position,
				count - position,
				&wrote_size,
				NULL);

		position += wrote_size;
		if(position != wrote_size)
		{
			nanosleep(&tv, &rem);
		}
	} while(position != count);

	if(flush)
	{
		g_io_channel_flush(channel, NULL);
	}

	/** @todo error checking */
	DEBUG_END();
}

static gboolean frwd_parse_heartrate(EcgData *self,gchar* frwd){
	
	int i;
	gint value = 0;
	DEBUG_BEGIN();

	if(self->buffer->len < FRWD_PACKET_SIZE)
	{
		DEBUG_END();
		return FALSE;
	}
	gchar *decrypt = g_strndup(frwd + 12, 3);
	for(i = 0; i < 3; i++)
	{
		decrypt[i] /= 2;
	}
	value = strtol(decrypt, NULL, 10);
	
	if(value < 235 && value > 20)
	self->hr = value;
	
	gchar *decryp = g_strndup(frwd + 51, 6);
	for(i = 0; i < 6; i++)
	{
		decryp[i] /= 2;
	}
	value = strtol(decryp, NULL, 10);
	if(self->buffer->len < FRWD_PACKET_SIZE)
	{
		return FALSE;
	}
	ecg_data_invoke_callbacks(self,self->hr);
	g_free(decrypt);
	DEBUG_END();
	return TRUE;

}
