#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "ISOTPConstants.h"

// ISO-TP session states
typedef enum {
	ISOTP_SESSION_IDLE = 0,
	ISOTP_SESSION_TRANSMITTING = 1,
	ISOTP_SESSION_TRANSMITTING_AWAITING_FC = 2,
	ISOTP_SESSION_RECEIVING = 3,
	ISOTP_SESSION_RECEIVED = 4,
} flexisotp_session_state_t;

typedef struct {
	//	Padding
	bool padding_enabled;					//  Flag indicating if padding should be used
	uint8_t padding_byte;					//  Byte to use for padding

	uint8_t consecutive_index_start;		//  Index consecutive frames start at
	uint8_t consecutive_index_end;			//  Index consecutive frames roll over at

	size_t fc_default_request_size;			//  Number of frames to request in a flow control if not overridden (0 = all)
} flexisotp_protocol_config_t;

// FlexISOTP session
typedef struct {
	//	Raw CAN Callbacks
	void (*callback_can_rx)(void* context, const uint8_t* msg_data, const size_t msg_length);
	void (*callback_can_tx)(void* context, const uint8_t* msg_data, const size_t msg_length);

	//	Frame Error Callbacks
	void (*error_invalid_frame) (void* context, const isotp_spec_frame_type_t rx_frame_type, const uint8_t* msg_data, const size_t msg_length);
	void (*error_unexpected_frame_type) (void* context, const uint8_t* msg_data, const size_t msg_length);
	void (*error_partner_aborted_transfer) (void* context, const uint8_t* msg_data, const size_t msg_length);

	//	ISO-TP Error Callbacks
	void (*error_transmission_too_large) (void* context, const uint8_t* data, const size_t length, const size_t requested_size);
	void (*error_consecutive_out_of_order) (void* context, const uint8_t* data, const size_t length, const uint8_t expected_index, const uint8_t recieved_index);

	//	ISO-TP Data Callbacks
	void (*callback_transmission_rx)(void* context);
	void (*callback_peek_first_frame) (void* context, const uint8_t* data, const size_t length);
	void (*callback_peek_consecutive_frame) (void* context, const uint8_t* data, const size_t length, const size_t start_idx);

	//	ISO-TP Protocol Configuration
	flexisotp_protocol_config_t protocol_config;

	//  Buffers
	void* tx_buffer;
	size_t tx_len;
	void* rx_buffer;
	size_t rx_len;

	//  Current Session State
	flexisotp_session_state_t state;			//  Current session state

	//  Bidirectional parameters
	uint16_t fc_allowed_frames_remaining;		//  Number of frames that can be sent/recieved before flow control (0 = FC needed, UINT16_MAX = no FC needed)
	uint32_t fc_requested_seperation;			//   (0 = No seperation time)

	//  Transmit parameters
	size_t tx_buffer_offset;                	//  Length of data sent from the current buffer

	//  Receive parameters
	size_t rx_buffer_offset;                	//  Length of data recieved from the current buffer
	size_t rx_expected_len;						//  Reported length of the transmission being received
	uint8_t rx_last_consecutive;				//	Last consecutive frame index received
} flexisotp_session_t;

/**
 * @brief Resets entire session to default state, callbacks to NULL, and loads provided buffer data
 * 
 * @param session 
 * @param tx_buffer 
 * @param tx_len 
 * @param rx_buffer 
 * @param rx_len 
 */
void flexisotp_session_init(flexisotp_session_t* session, void* tx_buffer, size_t tx_len, void* rx_buffer, size_t rx_len);

/**
 * @brief Resets session state to idle
 */
void flexisotp_session_idle(flexisotp_session_t* session);

/**
 * @brief Processes a recieved CAN frame and associated callbacks
 * 
 * @param session 
 * @param data 
 * @param length 
 */
void flexisotp_session_can_rx(flexisotp_session_t* session, const uint8_t* data, const size_t length);

/**
 * @brief Checks if a CAN frame is queued to be sent, and loads the contents to the provided buffer if so
 * 
 * @param session 
 * @param data 
 * @param length 
 * @return true 
 * @return false 
 */
bool flexisotp_session_can_tx(flexisotp_session_t* session, uint8_t* data, size_t* length);