#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "isotp_specification.h"

// ISO-TP session states
typedef enum {
	ISOTP_SESSION_IDLE = 0,
	ISOTP_SESSION_TRANSMITTING = 1,
	ISOTP_SESSION_TRANSMITTING_AWAITING_FC = 2,
	ISOTP_SESSION_RECEIVING = 3,
	ISOTP_SESSION_RECEIVED = 4,
} isotp_session_state_t;

typedef enum {
	ISOTP_FORMAT_NORMAL,	//	CAN 2.0 	ISO 15765-2
	ISOTP_FORMAT_FD,		//	CAN FD 		ISO 15765-2
	ISOTP_FORMAT_LIN,		//	LIN 		ISO 17987-2
	ISOTP_FORMAT_cnt,
} isotp_format_t;

typedef struct {
	//	Frame Format
	isotp_format_t frame_format;					//	ISO-TP frame frame_format

	//	Padding
	bool padding_enabled;					//  Flag indicating if padding should be used
	uint8_t padding_byte;					//  Byte to use for padding

	uint8_t consecutive_index_first;		//  Expected start index
	uint8_t consecutive_index_start;		//  Index consecutive frames start at
	uint8_t consecutive_index_end;			//  Index consecutive frames roll over at

	uint32_t fc_default_separation_time;	//  Valid uS seperation time for flow control frames (0 = no seperation, 100-900 uS or 1000-127000 uS)
	size_t fc_default_request_size;			//  Number of frames to request in a flow control if not overridden (0 = all)
} isotp_session_protocol_config_t;

// ISOTP session
typedef struct {
	/**
	 * @brief (required) Callback run when a full transmission is recieved. The data can be accessed from inside the session.
	 * 
	 */
	void (*callback_transmission_rx)(void* context);

	/**
	 * @brief (required) Catch-all callback for when a frame is recieved outside of expected frames in the current state.
	 * 
	 */
	void (*callback_error_invalid_frame) (void* context, const isotp_spec_frame_type_t rx_frame_type, const uint8_t* msg_data, const size_t msg_length);

	/**
	 * @brief (required) Callback run when the partner sends an ISO-TP transmission abort. Typically just cancel recieve with `isotp_session_idle` 
	 * 
	 */
	void (*callback_error_partner_aborted_transfer) (void* context, const uint8_t* msg_data, const size_t msg_length);

	/**
	 * @brief (required) Callback run when the stated size is too large for the RX buffer size. Typically just cancel recieve with `isotp_session_idle` 
	 * 
	 */
	void (*callback_error_transmission_too_large) (void* context, const uint8_t* data, const size_t length, const size_t requested_size);

	/**
	 * @brief (required) Callback run when a consecutive frame index was out of order (as defined by session.protocol_config). Typically just cancel recieve with `isotp_session_idle` 
	 * 
	 */
	void (*callback_error_consecutive_out_of_order) (void* context, const uint8_t* data, const size_t length, const uint8_t expected_index, const uint8_t recieved_index);

	/**
	 * @brief (required) Catch-all callback for when a frame is recieved outside of expected frames in the current state. No action required.
	 * 
	 */
	void (*callback_error_unexpected_frame_type) (void* context, const uint8_t* msg_data, const size_t msg_length);

	/**
	 * @brief (optional) Callback run when the first frame of a new transmission is recieved. Used by UDS to `isotp_session_send` a denial if not authorized/allowed before recieving the entire message.
	 * 
	 */
	void (*callback_peek_first_frame) (void* context, const uint8_t* data, const size_t length);

	/**
	 * @brief (optional) Callback run when each valid consecutive frame is recieved
	 * 
	 */
	void (*callback_peek_consecutive_frame) (void* context, const uint8_t* data, const size_t length, const size_t start_idx);

	/**
	 * @brief (optional) Callback for when a CAN frame is recieved
	 * 
	 */
	void (*callback_can_rx)(void* context, const uint8_t* msg_data, const size_t msg_length);

	/**
	 * @brief (optional) Callback for when a CAN frame is transmitted
	 * 
	 */
	void (*callback_can_tx)(void* context, const uint8_t* msg_data, const size_t msg_length);

	/**
	 * @brief (optional) If desired, the user can allocate memory with `isotp_session_use_rx_buffer` at the start of each new message inside this callback
	 * 
	 */
	void (*callback_mem_assign) (void* context, const size_t indicated_length);

	//	ISO-TP Protocol Configuration
	isotp_session_protocol_config_t protocol_config;

	//  Buffers
	void* tx_buffer;
	size_t tx_len;
	void* rx_buffer;
	size_t rx_len;

	//  Current Session State
	isotp_session_state_t state;			//  (Live) Current session state

	//  Bidirectional parameters
	uint16_t fc_allowed_frames_remaining;		//  (Live) Counter of frames that can be sent/recieved before flow control reqd (0 = FC needed, UINT16_MAX = FC not required)
	uint8_t fc_idx_track_consecutive;			//	(Live) Index of last consecutive frame index sent/recieved
	
	uint8_t fc_requested_block_size;			//  (Config) Block size currently requested (0 = All frames)
	uint32_t fc_requested_separation_uS;		//  (Config) Separation time currently requested (0 = No separation time)

	size_t full_transmission_length;			//  (Live) Reported length of the transmission being sent/recieved
	size_t buffer_offset;                		//  (Live) How many bytes have been sent/recieved from the current buffer
} isotp_session_t;

/**
 * @brief Resets entire session to default state, callbacks to NULL, and loads provided buffer data
 * 
 * @param session 
 * @param tx_buffer 
 * @param tx_len 
 * @param rx_buffer 
 * @param rx_len 
 */
void isotp_session_init(isotp_session_t* session, const isotp_format_t frame_format, void* tx_buffer, size_t tx_len, void* rx_buffer, size_t rx_len);

/**
 * @brief Allows for live reconfiguration of the RX buffer. Intended to be used from len_rx callback to allow for dynamic buffer allocation if desired. Only works in idle & memory_config callback states.
 * 
 * @param session Session to update
 * @param rx_buffer New RX buffer to use
 * @param rx_len New RX buffer length - sizeof(rx_buffer)
 * @return true Buffer successfully reconfigured
 * @return false State not valid (not idle or memory_config callback)
 */
bool isotp_session_use_rx_buffer(isotp_session_t* session, void* rx_buffer, size_t rx_len);

/**
 * @brief Resets session state to idle
 */
void isotp_session_idle(isotp_session_t* session);

/**
 * @brief Copies data to session tx buffer and starts transmission
 * 
 * @param session 
 * @param data 
 * @param data_length 
 * @return size_t 
 */
size_t isotp_session_send(isotp_session_t* session, const uint8_t* data, const size_t data_length);

/**
 * @brief Processes a recieved CAN frame and associated callbacks
 * 
 * @param session 
 * @param frame_data 
 * @param frame_length 
 */
void isotp_session_can_rx(isotp_session_t* session, const uint8_t* frame_data, const size_t frame_length);

/**
 * @brief Fetches the next ISO-TP frame to transmit
 * 
 * @param session Session to work with
 * @param frame_data Outputted frame data
 * @param frame_length Length of frame buffer
 * @param frame_size Size of frame allowed
 */
size_t isotp_session_can_tx(isotp_session_t* session, uint8_t* frame_data, const size_t frame_size, uint32_t* requested_separation_uS);

#ifdef __cplusplus
}
#endif