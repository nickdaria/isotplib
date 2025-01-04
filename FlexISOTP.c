#include "FlexISOTP.h"


/*

    CAN Reception

*/

//  Session is idle, accept new frames
void rx_idle(const isotp_spec_frame_type_t frame_type, flexisotp_session_t* session, const uint8_t* data, const size_t length) {
    switch(frame_type) {
        case ISOTP_SPEC_FRAME_SINGLE:
            break;
        case ISOTP_SPEC_FRAME_FIRST:
            //  First frame received
            session->state = ISOTP_SESSION_RECEIVING;

            if(session->callback_peek_first_frame != NULL) { session->callback_peek_first_frame(session, data, length, 0); }
            break;
        case ISOTP_SPEC_FRAME_CONSECUTIVE:
        case ISOTP_SPEC_FRAME_FLOW_CONTROL:
            //  Unexpected frame
            if(session->error_unexpected_frame_type != NULL) { session->error_unexpected_frame_type(session, data, length); }
            break;
        default:
            //  Invalid frame type
            if(session->error_invalid_frame != NULL) { session->error_invalid_frame(session, data, length); }
            break;
    }
}

void rx_transmitting(const isotp_spec_frame_type_t frame_type, flexisotp_session_t* session, const uint8_t* data, const size_t length) {

}

void rx_recieving(const isotp_spec_frame_type_t frame_type, flexisotp_session_t* session, const uint8_t* data, const size_t length) {

}

void rx_received(const isotp_spec_frame_type_t frame_type, flexisotp_session_t* session, const uint8_t* data, const size_t length) {

}

void flexisotp_session_can_rx(flexisotp_session_t* session, const uint8_t* data, const size_t length) {
    //  Safety
    if(session == NULL || data == NULL || length == 0) {
        return;
    }

    //  ISO-TP Frames must be at least 2 bytes long
    if(length < 2) {
        if(session->error_invalid_frame != NULL) { session->error_invalid_frame(session, data, length); }
        return;
    }

    //  Determine frame type
    isotp_spec_frame_type_t frame_type = (data[ISOTP_SPEC_FRAME_TYPE_BYTE] & ISOTP_SPEC_FRAME_TYPE_MASK) >> ISOTP_SPEC_FRAME_TYPE_SHIFT;
    
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

    Initialization

*/
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
    session->callback_peek_first_frame = NULL;
    session->callback_peek_consecutive_frame = NULL;
    session->error_invalid_frame = NULL;
    session->error_unexpected_frame_type = NULL;
    session->error_consecutive_out_of_order = NULL;

    //  Reset session state
    session->state = ISOTP_SESSION_IDLE;
    session->fc_allowed_frames_remaining = 0;
    session->tx_buffer_offset = 0;
    session->rx_buffer_offset = 0;
    session->rx_expected_len = 0;
    session->rx_last_consecutive = 0;
}