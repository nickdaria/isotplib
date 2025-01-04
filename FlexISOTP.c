#include "FlexISOTP.h"

#include <string.h>
#include "ISOTPConversions.h"

//  Helper to reset the session if no send was requested in the callback
void callbackCleanup(flexisotp_session_t* session) {
    if(session->state != ISOTP_SESSION_TRANSMITTING || session->state != ISOTP_SESSION_TRANSMITTING_AWAITING_FC) {
        flexisotp_session_idle(session);
    }
}

//  Helper to decrement fc allowed frames
void decrement_fc_allowed_frames(flexisotp_session_t* session) {
    if(session->fc_allowed_frames_remaining > 0 && session->fc_allowed_frames_remaining != UINT16_MAX) {
        session->fc_allowed_frames_remaining--;
    }
}

/*

    RX handlers

    These functions are called when the current session mode has received a frame and conditions allow processing

*/
void handle_single_frame(flexisotp_session_t* session, const uint8_t* frame_data, const size_t frame_length) {
    //  Safety
    if(session == NULL || frame_data == NULL || frame_length == 0) {
        return;
    }
    
    //  Reset session state
    flexisotp_session_idle(session);

    //  Update session state
    session->state = ISOTP_SESSION_RECEIVING;

    //  Safety: ensure header exists (it should)
    if(frame_length < ISOTP_SPEC_FRAME_SINGLE_DATASTART_IDX) {
        if(session->error_invalid_frame != NULL) { session->error_invalid_frame(session, 0xFF, frame_data, frame_length); }
        callbackCleanup(session);
        return;
    }

    //  Extract expected length
    session->full_transmission_length = frame_data[ISOTP_SPEC_FRAME_SINGLE_LEN_IDX] & ISOTP_SPEC_FRAME_SINGLE_LEN_MASK;

    //  Calculate packet length
    size_t packet_len = frame_length - ISOTP_SPEC_FRAME_SINGLE_DATASTART_IDX;

    // //  Safety for length byte being at least length of msg_length
    // if(packet_len < session->full_transmission_length) {
    //     if(session->error_invalid_frame != NULL) { session->error_invalid_frame(session, 0xFF, frame_data, frame_length); }
    //     callbackCleanup(session);
    //     return;
    // }

    //  Pointer to data
    const uint8_t* packet_start = frame_data + ISOTP_SPEC_FRAME_SINGLE_DATASTART_IDX;

    //  Safety for buffer being large enough
    if(session->full_transmission_length > session->rx_len) {
        if(session->error_transmission_too_large != NULL) { session->error_transmission_too_large(session, packet_start, packet_len, session->full_transmission_length); }
        callbackCleanup(session);
        return;
    }

    //  Load data into buffer
    memcpy(session->rx_buffer, packet_start, session->full_transmission_length);

    //  Update session
    session->buffer_offset += session->full_transmission_length;
    session->state = ISOTP_SESSION_RECEIVED;

    //  Callback
    if(session->callback_peek_first_frame != NULL) { session->callback_peek_first_frame(session, packet_start, session->full_transmission_length); }
    if(session->callback_transmission_rx != NULL) { session->callback_transmission_rx(session);}

    //  Reset session state if not acted upon
    callbackCleanup(session);
}

void handle_first_frame(flexisotp_session_t* session, const uint8_t* frame_data, const size_t frame_length) {
    //  Safety
    if(session == NULL || frame_data == NULL || frame_length == 0) {
        return;
    }

    //  Reset session state
    flexisotp_session_idle(session);

    //  Update session state
    session->state = ISOTP_SESSION_RECEIVING;

    //  Safety: ensure header exists
    if(frame_length < ISOTP_SPEC_FRAME_FIRST_DATASTART_IDX) {
        if(session->error_invalid_frame != NULL) { session->error_invalid_frame(session, 0xFF, frame_data, frame_length); }
        callbackCleanup(session);
        return;
    }

    //  Extract expected length
    size_t elen_msb = frame_data[ISOTP_SPEC_FRAME_FIRST_LEN_MSB_IDX] & ISOTP_SPEC_FRAME_FIRST_LEN_MSB_MASK;
    size_t elen_lsb = frame_data[ISOTP_SPEC_FRAME_FIRST_LEN_LSB_IDX] & ISOTP_SPEC_FRAME_FIRST_LEN_LSB_MASK;
    session->full_transmission_length = (elen_msb << 8) | elen_lsb;

    //  Calculate packet length
    size_t packet_len = frame_length - ISOTP_SPEC_FRAME_FIRST_DATASTART_IDX;

    //  Pointer to data
    const uint8_t* packet_start = frame_data + ISOTP_SPEC_FRAME_FIRST_DATASTART_IDX;

    //  Safety for buffer being large enough
    if(session->full_transmission_length > session->rx_len) {
        if(session->error_transmission_too_large != NULL) { session->error_transmission_too_large(session, packet_start, packet_len, session->full_transmission_length); }
        callbackCleanup(session);
        return;
    }

    //  Load data into buffer
    memcpy(session->rx_buffer, packet_start, packet_len);

    //  Update session
    session->buffer_offset += packet_len;
    decrement_fc_allowed_frames(session);   //  Will queue FC delay if configured

    //  Callback
    if(session->callback_peek_first_frame != NULL) { session->callback_peek_first_frame(session, packet_start, packet_len); }
    callbackCleanup(session);
}

void handle_consecutive_frame(flexisotp_session_t* session, const uint8_t* frame_data, const size_t frame_length) {
    //  Safety
    if(session == NULL || frame_data == NULL || frame_length == 0) {
        return;
    }

    //  Safety: session state
    if(session->state != ISOTP_SESSION_RECEIVING) {
        if(session->error_unexpected_frame_type != NULL) { session->error_unexpected_frame_type(session, frame_data, frame_length); }
        callbackCleanup(session);
        return;
    }

    //  Safety: ensure header exists
    if(frame_length < ISOTP_SPEC_FRAME_CONSECUTIVE_DATASTART_IDX) {
        if(session->error_invalid_frame != NULL) { session->error_invalid_frame(session, 0xFF, frame_data, frame_length); }
        callbackCleanup(session);
        return;
    }

    //  Get index
    uint8_t index = frame_data[ISOTP_SPEC_FRAME_CONSECUTIVE_INDEX_IDX] & ISOTP_SPEC_FRAME_CONSECUTIVE_INDEX_MASK;
    
    //  Calculate expected index
    uint8_t expected_index = session->fc_idx_last_consecutive + 1;
    if(expected_index > session->protocol_config.consecutive_index_end) {
        expected_index = session->protocol_config.consecutive_index_start;
    }

    //  Get packet parameters
    size_t packet_len = frame_length - ISOTP_SPEC_FRAME_CONSECUTIVE_DATASTART_IDX;
    const uint8_t* packet_start = frame_data + ISOTP_SPEC_FRAME_CONSECUTIVE_DATASTART_IDX;
    size_t bytes_remaining = session->full_transmission_length - session->buffer_offset;

    //  Safety: verify index
    if(index != expected_index) {
        if(session->error_consecutive_out_of_order != NULL) { session->error_consecutive_out_of_order(session, packet_start, packet_len, expected_index, index); }
        callbackCleanup(session);
        return;
    }

    //  Add to buffer
    if(packet_len > bytes_remaining) {
        packet_len = bytes_remaining;
    }
    memcpy(session->rx_buffer + session->buffer_offset, packet_start, packet_len);

    //  Update session
    session->buffer_offset += packet_len;

    //  Peek callback
    if(session->callback_peek_consecutive_frame != NULL) { session->callback_peek_consecutive_frame(session, packet_start, packet_len, session->buffer_offset - packet_len); }
    callbackCleanup(session);

    //  Check if transmission is complete
    if(session->buffer_offset >= session->full_transmission_length) {
        session->state = ISOTP_SESSION_RECEIVED;
        if(session->callback_transmission_rx != NULL) { session->callback_transmission_rx(session); }
        callbackCleanup(session);
    }

    //  Save last index
    session->fc_idx_last_consecutive = index;
}

void handle_flow_control_frame(flexisotp_session_t* session, const uint8_t* frame_data, const size_t frame_length) {
    //  Safety
    if(session == NULL || frame_data == NULL || frame_length == 0) {
        return;
    }

    //  Safety: session state
    if(session->state != ISOTP_SESSION_TRANSMITTING_AWAITING_FC && session->state != ISOTP_SESSION_TRANSMITTING) {
        if(session->error_unexpected_frame_type != NULL) { session->error_unexpected_frame_type(session, frame_data, frame_length); }
        callbackCleanup(session);
        return;
    }

    //  Read FC flags
    isotp_flow_control_flags_t fc_flags = (isotp_flow_control_flags_t)frame_data[ISOTP_SPEC_FRAME_FLOWCONTROL_FC_FLAGS_IDX] & ISOTP_SPEC_FRAME_FLOWCONTROL_FC_FLAGS_MASK;

    //  Read pblock size
    uint8_t block_size = ISOTP_SPEC_FRAME_FLOWCONTROL_BLOCKSIZE_SEND_WITHOUT_FC;    //  Default to no FC
    if(frame_length < ISOTP_SPEC_FRAME_FLOWCONTROL_BLOCKSIZE_IDX + 1) {
        block_size = frame_data[ISOTP_SPEC_FRAME_FLOWCONTROL_BLOCKSIZE_IDX] & ISOTP_SPEC_FRAME_FLOWCONTROL_BLOCKSIZE_MASK;
    }

    //  Read separation time
    uint32_t separation_time = 0;
    if(frame_length > ISOTP_SPEC_FRAME_FLOWCONTROL_SEPARATION_TIME_IDX + 1) {
        separation_time = isotp_fc_seperation_time_us(frame_data[ISOTP_SPEC_FRAME_FLOWCONTROL_SEPARATION_TIME_IDX] & ISOTP_SPEC_FRAME_FLOWCONTROL_SEPARATION_TIME_MASK);
    }

    //  Process FC frame
    switch(fc_flags) {
        case ISOTP_SPEC_FC_FLAG_CONTINUE_TO_SEND:
            //  Continue transmission
            session->state = ISOTP_SESSION_TRANSMITTING;
            break;
        case ISOTP_SPEC_FC_FLAG_WAIT:
            //  Wait
            session->state = ISOTP_SESSION_TRANSMITTING_AWAITING_FC;
            break;
        case ISOTP_SPEC_FC_FLAG_OVERFLOW_ABORT:
            //  Abort transmission
            if(session->error_partner_aborted_transfer != NULL) { session->error_partner_aborted_transfer(session, frame_data, frame_length); }
            callbackCleanup(session);
            break;
        default:
            //  Invalid FC flags
            if(session->error_invalid_frame != NULL) { session->error_invalid_frame(session, 0xFF, frame_data, frame_length); }
            callbackCleanup(session);
            return;
    }

    //  Load requested parameters
    session->fc_requested_separation = separation_time;
    session->fc_allowed_frames_remaining = block_size;
}

/*

    RX modes

    These functions are called when a frame is received based on session mode, allowing for different behavior based on the current state

*/

//  Session is idle, accept new frames
void rx_idle(const isotp_spec_frame_type_t frame_type, flexisotp_session_t* session, const uint8_t* frame_data, const size_t frame_length) {
    switch(frame_type) {
        case ISOTP_SPEC_FRAME_SINGLE:
            //  [IDEAL] Single frame received
            handle_single_frame(session, frame_data, frame_length);
            break;
        case ISOTP_SPEC_FRAME_FIRST:
            //  [IDEAL] First frame received
            handle_first_frame(session, frame_data, frame_length);
            break;
        case ISOTP_SPEC_FRAME_CONSECUTIVE:
        case ISOTP_SPEC_FRAME_FLOW_CONTROL:
            //  Unexpected frame
            if(session->error_unexpected_frame_type != NULL) { session->error_unexpected_frame_type(session, frame_data, frame_length); }
            break;
        default:
            //  Invalid frame type
            if(session->error_invalid_frame != NULL) { session->error_invalid_frame(session, frame_type, frame_data, frame_length); }
            break;
    }
}

void rx_transmitting(const isotp_spec_frame_type_t frame_type, flexisotp_session_t* session, const uint8_t* frame_data, const size_t frame_length) {
    switch(frame_type) {
        case ISOTP_SPEC_FRAME_SINGLE:
            //  Single frame received, abandon current transmission to satisfy new request
            flexisotp_session_idle(session);
            handle_single_frame(session, frame_data, frame_length);
            break;
        case ISOTP_SPEC_FRAME_FIRST:
            //  First frame received, abandon current transmission to satisfy new request
            flexisotp_session_idle(session);
            handle_first_frame(session, frame_data, frame_length);
            break;
        case ISOTP_SPEC_FRAME_CONSECUTIVE:
            //  Unexpected frame
            if(session->error_unexpected_frame_type != NULL) { session->error_unexpected_frame_type(session, frame_data, frame_length); }
            break;
        case ISOTP_SPEC_FRAME_FLOW_CONTROL:
            //  [IDEAL] Flow control frame received
            handle_flow_control_frame(session, frame_data, frame_length);
            break;
        default:
            //  Invalid frame type
            if(session->error_invalid_frame != NULL) { session->error_invalid_frame(session, frame_type, frame_data, frame_length); }
            break;
    }
}

void rx_recieving(const isotp_spec_frame_type_t frame_type, flexisotp_session_t* session, const uint8_t* frame_data, const size_t frame_length) {
    switch(frame_type) {
        case ISOTP_SPEC_FRAME_SINGLE:
            //  Single frame received, abandon current transmission to satisfy new request
            flexisotp_session_idle(session);
            handle_single_frame(session, frame_data, frame_length);
            break;
        case ISOTP_SPEC_FRAME_FIRST:
            //  First frame received, abandon current transmission to satisfy new request
            flexisotp_session_idle(session);
            handle_first_frame(session, frame_data, frame_length);
            break;
        case ISOTP_SPEC_FRAME_CONSECUTIVE:
            //  [IDEAL] Consecutive frame received
            handle_consecutive_frame(session, frame_data, frame_length);
            break;
        case ISOTP_SPEC_FRAME_FLOW_CONTROL:
            //  Unexpected frame
            if(session->error_unexpected_frame_type != NULL) { session->error_unexpected_frame_type(session, frame_data, frame_length); }
            break;
        default:
            //  Invalid frame type
            if(session->error_invalid_frame != NULL) { session->error_invalid_frame(session, frame_type, frame_data, frame_length); }
            break;
    }
}

void rx_received(const isotp_spec_frame_type_t frame_type, flexisotp_session_t* session, const uint8_t* frame_data, const size_t frame_length) {
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
    isotp_spec_frame_type_t frame_type = (data[ISOTP_SPEC_FRAME_TYPE_IDX] & ISOTP_SPEC_FRAME_TYPE_MASK) >> ISOTP_SPEC_FRAME_TYPE_SHIFT;
    
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
uint32_t tx_transmitting(flexisotp_session_t* session, uint8_t* frame_data, size_t* frame_length, const size_t frame_size) {
    //  Verify we are transmitting
    if(session->state != ISOTP_SESSION_TRANSMITTING) {
        return 0;
    }

    //  Check if we need to enter flow control wait mode
    if(session->fc_allowed_frames_remaining == 0) {
        session->state = ISOTP_SESSION_TRANSMITTING_AWAITING_FC;
        return 0;
    }

    //  Return
    uint32_t return_val = 0;

    //  First frame?
    if(session->buffer_offset == 0) {
        //  Can we fit this in a single frame?
        if(session->full_transmission_length <= frame_size - ISOTP_SPEC_FRAME_SINGLE_DATASTART_IDX) {
            //  Single frame
            frame_data[ISOTP_SPEC_FRAME_TYPE_IDX] &= ~ISOTP_SPEC_FRAME_TYPE_MASK; // Clear the type bits
            frame_data[ISOTP_SPEC_FRAME_TYPE_IDX] |= (ISOTP_SPEC_FRAME_SINGLE << ISOTP_SPEC_FRAME_TYPE_SHIFT) & ISOTP_SPEC_FRAME_TYPE_MASK;

            //  Set the length
            frame_data[ISOTP_SPEC_FRAME_SINGLE_LEN_IDX] &= ~ISOTP_SPEC_FRAME_SINGLE_LEN_MASK; // Clear the length bits
            frame_data[ISOTP_SPEC_FRAME_SINGLE_LEN_IDX] |= session->full_transmission_length & ISOTP_SPEC_FRAME_SINGLE_LEN_MASK;

            //  Setup parameters
            const uint8_t* packet_start = session->tx_buffer;
            const size_t packet_len = session->full_transmission_length;

            //  Copy data
            memcpy(frame_data + ISOTP_SPEC_FRAME_SINGLE_DATASTART_IDX, packet_start, packet_len);

            //  Send frame data
            return_val = 1;
        }
        else {
            //  First frame
            frame_data[ISOTP_SPEC_FRAME_TYPE_IDX] &= ~ISOTP_SPEC_FRAME_TYPE_MASK; // Clear the type bits
            frame_data[ISOTP_SPEC_FRAME_TYPE_IDX] |= (ISOTP_SPEC_FRAME_FIRST << ISOTP_SPEC_FRAME_TYPE_SHIFT) & ISOTP_SPEC_FRAME_TYPE_MASK;

            //  Set the length
            frame_data[ISOTP_SPEC_FRAME_FIRST_LEN_MSB_IDX] &= (uint8_t)~ISOTP_SPEC_FRAME_FIRST_LEN_MSB_MASK; // Clear the length bits
            frame_data[ISOTP_SPEC_FRAME_FIRST_LEN_MSB_IDX] |= (session->full_transmission_length >> 8) & ISOTP_SPEC_FRAME_FIRST_LEN_MSB_MASK;

            frame_data[ISOTP_SPEC_FRAME_FIRST_LEN_LSB_IDX] &= (uint8_t)~ISOTP_SPEC_FRAME_FIRST_LEN_LSB_MASK; // Clear the length bits
            frame_data[ISOTP_SPEC_FRAME_FIRST_LEN_LSB_IDX] |= session->full_transmission_length & ISOTP_SPEC_FRAME_FIRST_LEN_LSB_MASK;

            //  Setup parameters
            const uint8_t* packet_start = session->tx_buffer;
            const size_t packet_len = frame_size - ISOTP_SPEC_FRAME_FIRST_DATASTART_IDX;

            //  Copy data
            memcpy(frame_data + ISOTP_SPEC_FRAME_FIRST_DATASTART_IDX, packet_start, packet_len);

            //  Send frame data
            return_val = 1;
        }
    }
    else {
        //  Consecutive frame
        frame_data[ISOTP_SPEC_FRAME_TYPE_IDX] &= ~ISOTP_SPEC_FRAME_TYPE_MASK; // Clear the type bits
        frame_data[ISOTP_SPEC_FRAME_TYPE_IDX] |= (ISOTP_SPEC_FRAME_CONSECUTIVE << ISOTP_SPEC_FRAME_TYPE_SHIFT) & ISOTP_SPEC_FRAME_TYPE_MASK;

        //  Set the index
        frame_data[ISOTP_SPEC_FRAME_CONSECUTIVE_INDEX_IDX] &= ~ISOTP_SPEC_FRAME_CONSECUTIVE_INDEX_MASK; // Clear the index bits
        frame_data[ISOTP_SPEC_FRAME_CONSECUTIVE_INDEX_IDX] |= session->fc_idx_last_consecutive & ISOTP_SPEC_FRAME_CONSECUTIVE_INDEX_MASK;

        //  Setup parameters
        const uint8_t* packet_start = session->tx_buffer + session->buffer_offset;
        const size_t packet_len = frame_size - ISOTP_SPEC_FRAME_CONSECUTIVE_DATASTART_IDX;

        //  Copy data
        memcpy(frame_data + ISOTP_SPEC_FRAME_CONSECUTIVE_DATASTART_IDX, packet_start, packet_len);

        //  Send frame data
        return_val = session->fc_requested_separation;
    }

    //  Decrement allowed frames counter
    decrement_fc_allowed_frames(session);

    //  Check if done
    if(session->buffer_offset >= session->full_transmission_length) {
        //  Done
        flexisotp_session_idle(session);
    }

    //  Return
    return return_val;
}

uint32_t tx_recieving(flexisotp_session_t* session, uint8_t* frame_data, size_t* frame_length, const size_t frame_size) {
    //  Verify we are recieving data
    if(session->state != ISOTP_SESSION_RECEIVING) {
        return 0;
    }

    //  When recieving, we just need to check if a flow control frame is needed
    if(session->fc_allowed_frames_remaining == 0) {
        //  Send flow control frame
        //  TODO: Add ability to wait and abort?
        isotp_flow_control_flags_t fc_flag = ISOTP_SPEC_FC_FLAG_CONTINUE_TO_SEND;

        //  Reset block size and request configured number of frames
        session->fc_allowed_frames_remaining = session->fc_requested_block_size;
        
        //  Seperation time
        uint8_t seperation_time = isotp_fc_seperation_time_byte(session->fc_requested_separation);

        //  Assemble CAN frame
        frame_data[ISOTP_SPEC_FRAME_TYPE_IDX] &= ~ISOTP_SPEC_FRAME_TYPE_MASK; // Clear the type bits
        frame_data[ISOTP_SPEC_FRAME_TYPE_IDX] |= (ISOTP_SPEC_FRAME_FLOW_CONTROL << ISOTP_SPEC_FRAME_TYPE_SHIFT) & ISOTP_SPEC_FRAME_TYPE_MASK;

        // Set the flow control flag bits (lower nibble of the same byte)
        frame_data[ISOTP_SPEC_FRAME_FLOWCONTROL_FC_FLAGS_IDX] &= ~ISOTP_SPEC_FRAME_FLOWCONTROL_FC_FLAGS_MASK; // Clear the flag bits
        frame_data[ISOTP_SPEC_FRAME_FLOWCONTROL_FC_FLAGS_IDX] |= fc_flag & ISOTP_SPEC_FRAME_FLOWCONTROL_FC_FLAGS_MASK;

        // Set the block size
        frame_data[ISOTP_SPEC_FRAME_FLOWCONTROL_BLOCKSIZE_IDX] = session->fc_allowed_frames_remaining & ISOTP_SPEC_FRAME_FLOWCONTROL_BLOCKSIZE_MASK;

        //  Set the seperation time
        frame_data[ISOTP_SPEC_FRAME_FLOWCONTROL_SEPARATION_TIME_IDX] = seperation_time & ISOTP_SPEC_FRAME_FLOWCONTROL_SEPARATION_TIME_MASK;

        //  Set frame length
        *frame_length = ISOTP_SPEC_FRAME_FLOWCONTROL_HEADER_END;

        //  Return that we are transmitting
        return 1;
    }

    //  Nothing to send
    return 0;
}

uint32_t flexisotp_session_can_tx(flexisotp_session_t* session, uint8_t* frame_data, size_t* frame_length, const size_t frame_size) {
    //  Safety
    if(session == NULL || frame_data == NULL || frame_length == 0) {
        return false;
    }

    //  Determine action based on session state
    uint32_t return_val = 0;
    switch(session->state) {
        case ISOTP_SESSION_IDLE:
        case ISOTP_SESSION_RECEIVED:
        case ISOTP_SESSION_TRANSMITTING_AWAITING_FC:
            //  No need to transmit
            return_val = 0;
        case ISOTP_SESSION_TRANSMITTING:
            //  Transmitting
            return_val = tx_transmitting(session, frame_data, frame_length, frame_size);
        case ISOTP_SESSION_RECEIVING:
            //  Receiving
            return_val = tx_recieving(session, frame_data, frame_length, frame_size);
    }

    //  Padding (if enabled)
    if(session->protocol_config.padding_enabled) {
        for(size_t i = *frame_length; i < frame_size; i++) {
            frame_data[i] = session->protocol_config.padding_byte;
        }

        //  Update frame length
        *frame_length = frame_size;
    }

    //  CAN TX callback
    if(session->callback_can_tx != NULL) { session->callback_can_tx(session, frame_data, *frame_length); }

    //  No action taken
    return return_val;
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
    session->fc_allowed_frames_remaining = 1;
    session->fc_requested_separation = session->protocol_config.fc_default_separation_time;
    session->fc_requested_block_size = session->protocol_config.fc_default_request_size;
    session->buffer_offset = 0;
    session->full_transmission_length = 0;
    session->fc_idx_last_consecutive = 0;
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
    session->protocol_config.fc_default_request_size = 0;   //  request all frames by default
    session->protocol_config.fc_default_separation_time = 0;    //  no delay by default

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
    session->error_partner_aborted_transfer = NULL;
    session->error_unexpected_frame_type = NULL;
    session->error_consecutive_out_of_order = NULL;

    //  Reset session state
    flexisotp_session_idle(session);
}