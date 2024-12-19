#pragma once

#include <stdint.h>
#include <stdbool.h>

// ISO-TP frame types
typedef enum {
	FRAME_SINGLE = 0x00,
	FRAME_FIRST = 0x01,
	FRAME_CONSECUTIVE = 0x02,
	FRAME_FLOW_CONTROL = 0x03
} flexisotp_frame_type_t;

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
	//	Callbacks
	void (*callback_can_rx)(void* context, const uint8_t* data, const size_t length);
	void (*callback_can_tx)(void* context, const uint8_t* data, const size_t length);
	void (*callback_peek_first_frame) (void* context, const uint8_t* data, const size_t length, const size_t protocol_offset);
	void (*callback_peek_consecutive_frame) (void* context, const uint8_t* data, const size_t length, const size_t protocol_offset);
	void (*error_unexpected_frame_type) (void* context, const uint8_t* data, const size_t length);
	void (*error_consecutive_out_of_order) (void* context, const uint8_t* data, const size_t length);

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
	uint16_t fc_allowed_frames_remaining;		//  Number of frames that can be sent/recieved before flow control (0 = unlimited)

	//  Transmit parameters
	size_t tx_buffer_offset;                	//  Length of data sent from the current buffer

	//  Receive parameters
	size_t rx_buffer_offset;                	//  Length of data recieved from the current buffer
	size_t rx_expected_len;						//  Reported length of the transmission being received
	uint8_t rx_last_consecutive;				//	Last consecutive frame index received
} flexisotp_session_t;