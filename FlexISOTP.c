#include "FlexISOTP.h"
#include <string.h>

/*

    RX handlers

    These functions are called when the current session mode has received a frame and conditions allow processing

*/
void handle_single_frame(flexisotp_session_t* session, const uint8_t* msg_data, const size_t msg_length) {
    //  Safety
    if(session == NULL || msg_data == NULL || msg_length == 0) {
        return;
    }

    //  Safety for length byte existing
    if(msg_length < ISOTP_SPEC_FRAME_SINGLE_DATASTART_IDX) {
        if(session->error_invalid_frame != NULL) { session->error_invalid_frame(session, 0xFF, msg_data, msg_length); }
        return;
    }

    //  Extract expected length
    size_t packet_length = msg_data[ISOTP_SPEC_FRAME_SINGLE_LEN_IDX] & ISOTP_SPEC_FRAME_SINGLE_LEN_MASK;

    //  Safety for buffer being large enough
    if(packet_length > session->rx_len) {
        if(session->error_transmission_too_large != NULL) { session->error_transmission_too_large(session, msg_data, msg_length, packet_length); }
        return;
    }

    //  Safety for length byte being at least length of msg_length
    if(msg_length - ISOTP_SPEC_FRAME_SINGLE_DATASTART_IDX < packet_length) {
        if(session->error_invalid_frame != NULL) { session->error_invalid_frame(session, 0xFF, msg_data, msg_length); }
        return;
    }

    //  Reset session state
    flexisotp_session_idle(session);
    session->state = ISOTP_SESSION_RECEIVING;

    //  Load data into buffer
    const uint8_t* packet_start = msg_data + ISOTP_SPEC_FRAME_SINGLE_DATASTART_IDX;
    memcpy(session->rx_buffer, packet_start, packet_length);

    //  Update session
    session->rx_buffer_offset += packet_length;
    session->state = ISOTP_SESSION_RECEIVED;

    //  Callback
    if(session->callback_peek_first_frame != NULL) { session->callback_peek_first_frame(session, packet_start, packet_length); }
    if(session->callback_transmission_rx != NULL) { session->callback_transmission_rx(session);}

    //  Reset session state if not acted upon
    if(session->state == ISOTP_SESSION_RECEIVED) {
        flexisotp_session_idle(session);
    }
}

void handle_first_frame(flexisotp_session_t* session, const uint8_t* data, const size_t length) {
    //  Safety
    if(session == NULL || data == NULL || length == 0) {
        return;
    }

    //  Callback
    //if(session->callback_peek_first_frame != NULL) { session->callback_peek_first_frame(session, data, length); }
}

void handle_consecutive_frame(flexisotp_session_t* session, const uint8_t* data, const size_t length) {
    //  Safety
    if(session == NULL || data == NULL || length == 0) {
        return;
    }

    //  Callback
    //if(session->callback_peek_consecutive_frame != NULL) { session->callback_peek_consecutive_frame(session, data, length, 0); }
}

void handle_flow_control_frame(flexisotp_session_t* session, const uint8_t* data, const size_t length) {
    //  Safety
    if(session == NULL || data == NULL || length == 0) {
        return;
    }

    //  Callback
    //if(session->callback_peek_consecutive_frame != NULL) { session->callback_peek_consecutive_frame(session, data, length, 0); }
}

/*

    RX modes

    These functions are called when a frame is received based on session mode, allowing for different behavior based on the current state

*/

//  Session is idle, accept new frames
void rx_idle(const isotp_spec_frame_type_t frame_type, flexisotp_session_t* session, const uint8_t* data, const size_t length) {
    switch(frame_type) {
        case ISOTP_SPEC_FRAME_SINGLE:
            //  [IDEAL] Single frame received
            handle_single_frame(session, data, length);
            break;
        case ISOTP_SPEC_FRAME_FIRST:
            //  [IDEAL] First frame received
            handle_first_frame(session, data, length);
            break;
        case ISOTP_SPEC_FRAME_CONSECUTIVE:
        case ISOTP_SPEC_FRAME_FLOW_CONTROL:
            //  Unexpected frame
            if(session->error_unexpected_frame_type != NULL) { session->error_unexpected_frame_type(session, data, length); }
            break;
        default:
            //  Invalid frame type
            if(session->error_invalid_frame != NULL) { session->error_invalid_frame(session, frame_type, data, length); }
            break;
    }
}

void rx_transmitting(const isotp_spec_frame_type_t frame_type, flexisotp_session_t* session, const uint8_t* data, const size_t length) {
    switch(frame_type) {
        case ISOTP_SPEC_FRAME_SINGLE:
            //  Single frame received, abandon current transmission to satisfy new request
            flexisotp_session_idle(session);
            handle_single_frame(session, data, length);
            break;
        case ISOTP_SPEC_FRAME_FIRST:
            //  First frame received, abandon current transmission to satisfy new request
            flexisotp_session_idle(session);
            handle_first_frame(session, data, length);
            break;
        case ISOTP_SPEC_FRAME_CONSECUTIVE:
            //  Unexpected frame
            if(session->error_unexpected_frame_type != NULL) { session->error_unexpected_frame_type(session, data, length); }
            break;
        case ISOTP_SPEC_FRAME_FLOW_CONTROL:
            //  [IDEAL] Flow control frame received
            handle_flow_control_frame(session, data, length);
            break;
        default:
            //  Invalid frame type
            if(session->error_invalid_frame != NULL) { session->error_invalid_frame(session, frame_type, data, length); }
            break;
    }
}

void rx_recieving(const isotp_spec_frame_type_t frame_type, flexisotp_session_t* session, const uint8_t* data, const size_t length) {
    switch(frame_type) {
        case ISOTP_SPEC_FRAME_SINGLE:
            //  Single frame received, abandon current transmission to satisfy new request
            flexisotp_session_idle(session);
            handle_single_frame(session, data, length);
            break;
        case ISOTP_SPEC_FRAME_FIRST:
            //  First frame received, abandon current transmission to satisfy new request
            flexisotp_session_idle(session);
            handle_first_frame(session, data, length);
            break;
        case ISOTP_SPEC_FRAME_CONSECUTIVE:
            //  [IDEAL] Consecutive frame received
            handle_consecutive_frame(session, data, length);
            break;
        case ISOTP_SPEC_FRAME_FLOW_CONTROL:
            //  Unexpected frame
            if(session->error_unexpected_frame_type != NULL) { session->error_unexpected_frame_type(session, data, length); }
            break;
        default:
            //  Invalid frame type
            if(session->error_invalid_frame != NULL) { session->error_invalid_frame(session, frame_type, data, length); }
            break;
    }
}

void rx_received(const isotp_spec_frame_type_t frame_type, flexisotp_session_t* session, const uint8_t* data, const size_t length) {
    //  Program has not yet handled RX buffer
    //  We need to have a callback and return a busy error
}

void flexisotp_session_can_rx(flexisotp_session_t* session, const uint8_t* data, const size_t length) {
    //  Safety
    if(session == NULL || data == NULL || length == 0) {
        return;
    }

    //  ISO-TP Frames must be at least 2 bytes long
    if(length < 2) {
        if(session->error_invalid_frame != NULL) { session->error_invalid_frame(session, 0xFF, data, length); }
        return;
    }

    //  Determine frame type
    isotp_spec_frame_type_t frame_type = (data[ISOTP_SPEC_FRAME_TYPE_IDX] & ISOTP_SPEC_FRAME_TYPE_MASK) >> ISOTP_SPEC_FRAME_TYPE_SHIFT_R;
    
    //  Process frame based on session state
    switch(session->state) {
        case ISOTP_SESSION_IDLE:
            rx_idle(frame_type, session, data, length);
            break;
        case ISOTP_SESSION_TRANSMITTING:
            rx_transmitting(frame_type, session, data, length);
            break;
        case ISOTP_SESSION_TRANSMITTING_AWAITING_FC:
            rx_transmitting(frame_type, session, data, length);
            break;
        case ISOTP_SESSION_RECEIVING:
            rx_recieving(frame_type, session, data, length);
            break;
        case ISOTP_SESSION_RECEIVED:
            rx_received(frame_type, session, data, length);
            break;
    }
}

/*

    CAN Transmission

*/
bool flexisotp_session_can_tx(flexisotp_session_t* session, uint8_t* data, size_t* length) {
    //  Safety
    if(session == NULL || data == NULL || length == 0) {
        return false;
    }

    return false;
}

/*

    Helpers

*/
void flexisotp_session_idle(flexisotp_session_t* session) {
    //  Safety
    if(session == NULL) {
        return;
    }

    //  Reset session state
    session->state = ISOTP_SESSION_IDLE;
    session->fc_allowed_frames_remaining = 0;
    session->tx_buffer_offset = 0;
    session->rx_buffer_offset = 0;
    session->rx_expected_len = 0;
    session->rx_last_consecutive = 0;
}

void flexisotp_session_init(flexisotp_session_t* session, void* tx_buffer, size_t tx_len, void* rx_buffer, size_t rx_len) {
    //  Safety
    if(session == NULL) {
        return;
    }

    //  Default protocol configuration
    session->protocol_config.padding_enabled = true;
    session->protocol_config.padding_byte = 0xFF;
    session->protocol_config.consecutive_index_start = 0;
    session->protocol_config.consecutive_index_end = 15;
    session->protocol_config.fc_default_request_size = 1;

    //  Load buffers
    session->tx_buffer = tx_buffer;
    session->tx_len = tx_len;
    session->rx_buffer = rx_buffer;
    session->rx_len = rx_len;

    //  Clear callbacks
    session->callback_can_rx = NULL;
    session->callback_can_tx = NULL;
    session->callback_transmission_rx = NULL;
    session->callback_peek_first_frame = NULL;
    session->callback_peek_consecutive_frame = NULL;
    session->error_invalid_frame = NULL;
    session->error_transmission_too_large = NULL;
    session->error_unexpected_frame_type = NULL;
    session->error_consecutive_out_of_order = NULL;

    //  Reset session state
    flexisotp_session_idle(session);
}