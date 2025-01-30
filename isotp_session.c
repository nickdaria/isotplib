
#include <string.h>
#include "isotp_session.h"
#include "isotp_conversions.h"


//  Helper to decrement fc allowed frames
void decrement_fc_allowed_frames(isotp_session_t* session) {
    if(session->fc_allowed_frames_remaining > 0 && session->fc_allowed_frames_remaining != UINT16_MAX) {
        session->fc_allowed_frames_remaining--;
    }
}

/*

    RX handlers

    These functions are called when the current session mode has received a frame and conditions allow processing

*/
void handle_single_frame(isotp_session_t* session, const uint8_t* frame_data, const size_t frame_length) {
    //  Safety
    if(session == NULL || frame_data == NULL || frame_length == 0) {
        return;
    }
    
    //  Reset session state
    isotp_session_idle(session);

    //  Update session state
    session->state = ISOTP_SESSION_RECEIVING;

    //  Safety: ensure header exists (it should)
    if(frame_length < ISOTP_SPEC_FRAME_SINGLE_DATASTART_IDX) {
        if(session->callback_error_invalid_frame != NULL) { session->callback_error_invalid_frame(session, (isotp_spec_frame_type_t)0xFF, frame_data, frame_length); }
        return;
    }

    //  Extract expected length
    session->full_transmission_length = frame_data[ISOTP_SPEC_FRAME_SINGLE_LEN_IDX] & ISOTP_SPEC_FRAME_SINGLE_LEN_MASK;

    //  Pointer to data
    const uint8_t* packet_start = frame_data + ISOTP_SPEC_FRAME_SINGLE_DATASTART_IDX;

    //  Calculate packet length
    size_t packet_len = frame_length - ISOTP_SPEC_FRAME_SINGLE_DATASTART_IDX;

    //  CAN-FD
    if(session->full_transmission_length == 0) {
        //  FD only works when protocol settings allow
        if(session->protocol_config.frame_format != ISOTP_FORMAT_FD) {
            if(session->callback_error_invalid_frame != NULL) { session->callback_error_invalid_frame(session, ISOTP_SPEC_FRAME_SINGLE, frame_data, frame_length); }
            return;
        }

        //  Length safety
        if(frame_length < ISOTP_SPEC_FRAME_SINGLE_FD_DATASTART_IDX) {
            if(session->callback_error_invalid_frame != NULL) { session->callback_error_invalid_frame(session, ISOTP_SPEC_FRAME_SINGLE, frame_data, frame_length); }
            return;
        }

        //  Extract expected length using CAN-FD spec
        session->full_transmission_length = frame_data[ISOTP_SPEC_FRAME_SINGLE_FD_LEN_IDX] & ISOTP_SPEC_FRAME_SINGLE_FD_LEN_MASK;
    
        //  Update start pointer
        packet_start = frame_data + ISOTP_SPEC_FRAME_SINGLE_FD_DATASTART_IDX;

        //  Recalculate packet length
        packet_len = frame_length - ISOTP_SPEC_FRAME_SINGLE_FD_DATASTART_IDX;
    }

    //  Allow user to assign memory if desired
    if(session->callback_mem_assign != NULL) { session->callback_mem_assign(session, session->full_transmission_length); }

    //  Safety for length byte being at least length of msg_length
    if(packet_len < session->full_transmission_length) {
        if(session->callback_error_invalid_frame != NULL) { session->callback_error_invalid_frame(session, (isotp_spec_frame_type_t)0xFF, frame_data, frame_length); }
        return;
    }

    //  Safety for buffer being large enough
    if(session->full_transmission_length > session->rx_len) {
        if(session->callback_error_transmission_too_large != NULL) { session->callback_error_transmission_too_large(session, packet_start, packet_len, session->full_transmission_length); }
        return;
    }

    //  Load data into buffer
    memcpy(session->rx_buffer, packet_start, session->full_transmission_length);

    //  Update session
    session->buffer_offset += session->full_transmission_length;
    session->state = ISOTP_SESSION_RECEIVED;

    //  Peek
    if(session->callback_peek_first_frame != NULL) { session->callback_peek_first_frame(session, packet_start, session->full_transmission_length); }
    
    //  Callback
    if(session->callback_transmission_rx != NULL) { session->callback_transmission_rx(session); }
    //if(session->state == ISOTP_SESSION_RECEIVED) { isotp_session_idle(session); }
}

void handle_first_frame(isotp_session_t* session, const uint8_t* frame_data, const size_t frame_length) {
    //  Safety
    if(session == NULL || frame_data == NULL || frame_length == 0) {
        return;
    }

    //  Reset session state
    isotp_session_idle(session);

    //  Update session state
    session->state = ISOTP_SESSION_RECEIVING;

    //  Safety: ensure header exists
    if(frame_length < ISOTP_SPEC_FRAME_FIRST_DATASTART_IDX) {
        if(session->callback_error_invalid_frame != NULL) { session->callback_error_invalid_frame(session, (isotp_spec_frame_type_t)0xFF, frame_data, frame_length); }
        return;
    }

    //  Extract expected length
    size_t elen_msb = frame_data[ISOTP_SPEC_FRAME_FIRST_LEN_MSB_IDX] & ISOTP_SPEC_FRAME_FIRST_LEN_MSB_MASK;
    size_t elen_lsb = frame_data[ISOTP_SPEC_FRAME_FIRST_LEN_LSB_IDX] & ISOTP_SPEC_FRAME_FIRST_LEN_LSB_MASK;
    session->full_transmission_length = (elen_msb << 8) | elen_lsb;

    //  Pointer to data
    const uint8_t* packet_start = frame_data + ISOTP_SPEC_FRAME_FIRST_DATASTART_IDX;
    
    //  Calculate packet length
    size_t packet_len = frame_length - ISOTP_SPEC_FRAME_FIRST_DATASTART_IDX;

    //  CAN-FD
    if(session->full_transmission_length == 0) {
        //  FD only works when protocol settings allow
        if(session->protocol_config.frame_format != ISOTP_FORMAT_FD) {
            if(session->callback_error_invalid_frame != NULL) { session->callback_error_invalid_frame(session, ISOTP_SPEC_FRAME_SINGLE, frame_data, frame_length); }
            return;
        }

        //  Length safety
        if (frame_length < ISOTP_SPEC_FRAME_FIRST_FD_DATASTART_IDX) {
            if (session->callback_error_invalid_frame != NULL) {
                session->callback_error_invalid_frame(session, (isotp_spec_frame_type_t)0xFF, frame_data, frame_length);
            }
            return;
        }

        //  Extract expected length using CAN-FD spec
        session->full_transmission_length = 0;
        for (size_t i = ISOTP_SPEC_FRAME_FIRST_FD_MSB_IDX; i <= ISOTP_SPEC_FRAME_FIRST_FD_LSB_IDX; ++i) {
            session->full_transmission_length |= frame_data[i] << (8 * (ISOTP_SPEC_FRAME_FIRST_FD_LSB_IDX - i));
        }

        //  Update start pointer
        packet_start = frame_data + ISOTP_SPEC_FRAME_FIRST_FD_DATASTART_IDX;

        //  Recalculate packet length
        packet_len = frame_length - ISOTP_SPEC_FRAME_FIRST_FD_DATASTART_IDX;
    }

    //  Safety for buffer being large enough
    if(session->full_transmission_length > session->rx_len) {
        if(session->callback_error_transmission_too_large != NULL) { session->callback_error_transmission_too_large(session, packet_start, packet_len, session->full_transmission_length); }
        return;
    }

    //  Load data into buffer
    memcpy(session->rx_buffer, packet_start, packet_len);

    //  Update session
    session->buffer_offset += packet_len;
    decrement_fc_allowed_frames(session);   //  Will queue FC delay if configured

    //  Callback
    if(session->callback_peek_first_frame != NULL) { session->callback_peek_first_frame(session, packet_start, packet_len); }
}

void handle_consecutive_frame(isotp_session_t* session, const uint8_t* frame_data, const size_t frame_length) {
    // Safety
    if (session == NULL || frame_data == NULL || frame_length == 0) {
        return;
    }

    // Safety: session state
    if (session->state != ISOTP_SESSION_RECEIVING) {
        if (session->callback_error_unexpected_frame_type != NULL) { 
            session->callback_error_unexpected_frame_type(session, frame_data, frame_length); 
        }
        return;
    }

    // Safety: ensure header exists
    if (frame_length < ISOTP_SPEC_FRAME_CONSECUTIVE_DATASTART_IDX) {
        if (session->callback_error_invalid_frame != NULL) { 
            session->callback_error_invalid_frame(session, (isotp_spec_frame_type_t)0xFF, frame_data, frame_length); 
        }
        return;
    }

    // Get index
    uint8_t index = frame_data[ISOTP_SPEC_FRAME_CONSECUTIVE_INDEX_IDX] & ISOTP_SPEC_FRAME_CONSECUTIVE_INDEX_MASK;

    // Verify the received index matches the expected index
    if (index != session->fc_idx_track_consecutive) {
        if (session->callback_error_consecutive_out_of_order != NULL) {
            session->callback_error_consecutive_out_of_order(session, frame_data, frame_length, session->fc_idx_track_consecutive, index);
        }
        return;
    }

    // Increment expected index and handle rollover
    session->fc_idx_track_consecutive++;
    if (session->fc_idx_track_consecutive >= session->protocol_config.consecutive_index_end) {
        session->fc_idx_track_consecutive = session->protocol_config.consecutive_index_start;
    }

    //  Get packet parameters
    size_t packet_len = frame_length - ISOTP_SPEC_FRAME_CONSECUTIVE_DATASTART_IDX;
    const uint8_t* packet_start = frame_data + ISOTP_SPEC_FRAME_CONSECUTIVE_DATASTART_IDX;
    size_t bytes_remaining = session->full_transmission_length - session->buffer_offset;

    //  Add to buffer
    if (packet_len > bytes_remaining) {
        packet_len = bytes_remaining;
    }

    //  Copy data
    memcpy((uint8_t*)session->rx_buffer + session->buffer_offset, packet_start, packet_len);

    //  Update session
    session->buffer_offset += packet_len;

    //  Peek callback
    if (session->callback_peek_consecutive_frame != NULL) {
        session->callback_peek_consecutive_frame(session, packet_start, packet_len, session->buffer_offset - packet_len);
    }

    // Check if transmission is complete
    if (session->buffer_offset >= session->full_transmission_length) {
        //  Update state
        session->state = ISOTP_SESSION_RECEIVED;

        //  Callback
        if(session->callback_transmission_rx != NULL) { session->callback_transmission_rx(session); }
        //if(session->state == ISOTP_SESSION_RECEIVED) { isotp_session_idle(session); }
    }
}

void handle_flow_control_frame(isotp_session_t* session, const uint8_t* frame_data, const size_t frame_length) {
    //  Safety
    if(session == NULL || frame_data == NULL || frame_length == 0) {
        return;
    }

    //  Safety: session state
    if(session->state != ISOTP_SESSION_TRANSMITTING_AWAITING_FC && session->state != ISOTP_SESSION_TRANSMITTING) {
        if(session->callback_error_unexpected_frame_type != NULL) { session->callback_error_unexpected_frame_type(session, frame_data, frame_length); }
        return;
    }

    //  LIN does not use FC
    if(session->protocol_config.frame_format == ISOTP_FORMAT_LIN) {
        //  Unexpected frame
        if(session->callback_error_unexpected_frame_type != NULL) { session->callback_error_unexpected_frame_type(session, frame_data, frame_length); }
        return;
    }

    //  Read FC flags
    isotp_flow_control_flags_t fc_flags = (isotp_flow_control_flags_t)(frame_data[ISOTP_SPEC_FRAME_FLOWCONTROL_FC_FLAGS_IDX] & ISOTP_SPEC_FRAME_FLOWCONTROL_FC_FLAGS_MASK);

    //  Read pblock size
    uint8_t block_size = ISOTP_SPEC_FRAME_FLOWCONTROL_BLOCKSIZE_SEND_WITHOUT_FC;    //  Default to no FC
    if(frame_length >= ISOTP_SPEC_FRAME_FLOWCONTROL_BLOCKSIZE_IDX + 1) {
        block_size = frame_data[ISOTP_SPEC_FRAME_FLOWCONTROL_BLOCKSIZE_IDX] & ISOTP_SPEC_FRAME_FLOWCONTROL_BLOCKSIZE_MASK;
    }

    //  Read separation time
    uint32_t separation_time = 0;
    if(frame_length >= ISOTP_SPEC_FRAME_FLOWCONTROL_SEPARATION_TIME_IDX + 1) {
        separation_time = isotp_spec_fc_separation_time_us(frame_data[ISOTP_SPEC_FRAME_FLOWCONTROL_SEPARATION_TIME_IDX] & ISOTP_SPEC_FRAME_FLOWCONTROL_SEPARATION_TIME_MASK);
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
            if(session->callback_error_partner_aborted_transfer != NULL) { session->callback_error_partner_aborted_transfer(session, frame_data, frame_length); }
            break;
        default:
            //  Invalid FC flags
            if(session->callback_error_invalid_frame != NULL) { session->callback_error_invalid_frame(session, (isotp_spec_frame_type_t)0xFF, frame_data, frame_length); }
            return;
    }

    //  Load seperation time
    session->fc_requested_separation_uS = separation_time;

    //  Load block size
    session->fc_allowed_frames_remaining = block_size;
    if(session->fc_allowed_frames_remaining == 0) {
        session->fc_allowed_frames_remaining = UINT16_MAX;
    }
}

/*

    RX modes

    These functions are called when a frame is received based on session mode, allowing for different behavior based on the current state

*/

//  Session is idle, accept new frames
void rx_idle(const isotp_spec_frame_type_t frame_type, isotp_session_t* session, const uint8_t* frame_data, const size_t frame_length) {
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
            if(session->callback_error_unexpected_frame_type != NULL) { session->callback_error_unexpected_frame_type(session, frame_data, frame_length); }
            break;
        default:
            //  Invalid frame type
            if(session->callback_error_invalid_frame != NULL) { session->callback_error_invalid_frame(session, frame_type, frame_data, frame_length); }
            break;
    }
}

void rx_transmitting(const isotp_spec_frame_type_t frame_type, isotp_session_t* session, const uint8_t* frame_data, const size_t frame_length) {
    switch(frame_type) {
        case ISOTP_SPEC_FRAME_SINGLE:
            //  Single frame received, abandon current transmission to satisfy new request
            isotp_session_idle(session);
            handle_single_frame(session, frame_data, frame_length);
            break;
        case ISOTP_SPEC_FRAME_FIRST:
            //  First frame received, abandon current transmission to satisfy new request
            isotp_session_idle(session);
            handle_first_frame(session, frame_data, frame_length);
            break;
        case ISOTP_SPEC_FRAME_CONSECUTIVE:
            //  Unexpected frame
            if(session->callback_error_unexpected_frame_type != NULL) { session->callback_error_unexpected_frame_type(session, frame_data, frame_length); }
            break;
        case ISOTP_SPEC_FRAME_FLOW_CONTROL:
            //  [IDEAL] Flow control frame received
            handle_flow_control_frame(session, frame_data, frame_length);
            break;
        default:
            //  Invalid frame type
            if(session->callback_error_invalid_frame != NULL) { session->callback_error_invalid_frame(session, frame_type, frame_data, frame_length); }
            break;
    }
}

void rx_recieving(const isotp_spec_frame_type_t frame_type, isotp_session_t* session, const uint8_t* frame_data, const size_t frame_length) {
    switch(frame_type) {
        case ISOTP_SPEC_FRAME_SINGLE:
            //  Single frame received, abandon current transmission to satisfy new request
            handle_single_frame(session, frame_data, frame_length);
            break;
        case ISOTP_SPEC_FRAME_FIRST:
            //  First frame received, abandon current transmission to satisfy new request
            handle_first_frame(session, frame_data, frame_length);
            break;
        case ISOTP_SPEC_FRAME_CONSECUTIVE:
            //  [IDEAL] Consecutive frame received
            handle_consecutive_frame(session, frame_data, frame_length);
            break;
        case ISOTP_SPEC_FRAME_FLOW_CONTROL:
            //  Unexpected frame
            if(session->callback_error_unexpected_frame_type != NULL) { session->callback_error_unexpected_frame_type(session, frame_data, frame_length); }
            break;
        default:
            //  Invalid frame type
            if(session->callback_error_invalid_frame != NULL) { session->callback_error_invalid_frame(session, frame_type, frame_data, frame_length); }
            break;
    }
}

void rx_received(const isotp_spec_frame_type_t frame_type, isotp_session_t* session, const uint8_t* frame_data, const size_t frame_length) {
    //  Program has not yet handled RX buffer
    //  We need to have a callback and return a busy error
}

void isotp_session_can_rx(isotp_session_t* session, const uint8_t* data, const size_t length) {
    //  Safety
    if(session == NULL || data == NULL || length == 0) {
        return;
    }

    //  ISO-TP Frames must be at least 2 bytes long
    if(length < 2) {
        if(session->callback_error_invalid_frame != NULL) { session->callback_error_invalid_frame(session, (isotp_spec_frame_type_t)0xFF, data, length); }
        return;
    }

    //  Determine frame type
    isotp_spec_frame_type_t frame_type = (isotp_spec_frame_type_t)((data[ISOTP_SPEC_FRAME_TYPE_IDX] & ISOTP_SPEC_FRAME_TYPE_MASK) >> ISOTP_SPEC_FRAME_TYPE_SHIFT);
    
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
size_t tx_transmitting(isotp_session_t* session, uint8_t* frame_data, const size_t frame_size, uint32_t* requested_separation_uS) {
    //  Verify we are transmitting
    if(session->state != ISOTP_SESSION_TRANSMITTING) {
        return 0;
    }

    //  Return
    if(requested_separation_uS != NULL) { *requested_separation_uS = 0; }
    size_t ret_frame_size = 0;

    //  First frame?
    if(session->buffer_offset == 0) {
        //  Can we fit this in a single frame?
        if(session->full_transmission_length <= frame_size - ISOTP_SPEC_FRAME_SINGLE_DATASTART_IDX) {
            //  Single frame
            frame_data[ISOTP_SPEC_FRAME_TYPE_IDX] &= ~ISOTP_SPEC_FRAME_TYPE_MASK; // Clear the type bits
            frame_data[ISOTP_SPEC_FRAME_TYPE_IDX] |= (ISOTP_SPEC_FRAME_SINGLE << ISOTP_SPEC_FRAME_TYPE_SHIFT) & ISOTP_SPEC_FRAME_TYPE_MASK;

            //  Clear length bits
            frame_data[ISOTP_SPEC_FRAME_SINGLE_LEN_IDX] &= ~ISOTP_SPEC_FRAME_SINGLE_LEN_MASK; // Clear the length bits

            //  Insert length
            switch(session->protocol_config.frame_format) {
                case ISOTP_FORMAT_FD:
                    //  Set FD length
                    frame_data[ISOTP_SPEC_FRAME_SINGLE_FD_LEN_IDX] &= (uint8_t)~ISOTP_SPEC_FRAME_SINGLE_FD_LEN_MASK; // Clear the length bits
                    frame_data[ISOTP_SPEC_FRAME_SINGLE_FD_LEN_IDX] |= session->full_transmission_length & ISOTP_SPEC_FRAME_SINGLE_FD_LEN_MASK;
                    break;
                case ISOTP_FORMAT_NORMAL:
                case ISOTP_FORMAT_LIN:
                    //  Set the length
                    frame_data[ISOTP_SPEC_FRAME_SINGLE_LEN_IDX] |= session->full_transmission_length & ISOTP_SPEC_FRAME_SINGLE_LEN_MASK;
                    break;
            }

            //  Setup parameters
            const uint8_t* packet_start = (uint8_t*)session->tx_buffer;
            const size_t packet_len = session->full_transmission_length;

            //  Copy data
            memcpy(frame_data + ISOTP_SPEC_FRAME_SINGLE_DATASTART_IDX, packet_start, packet_len);

            //  Send frame data
            ret_frame_size = packet_len + ISOTP_SPEC_FRAME_SINGLE_DATASTART_IDX;

            //  Conclude transmission
            isotp_session_idle(session);
        }
        else {
            //  First frame
            frame_data[ISOTP_SPEC_FRAME_TYPE_IDX] &= ~ISOTP_SPEC_FRAME_TYPE_MASK; // Clear the type bits
            frame_data[ISOTP_SPEC_FRAME_TYPE_IDX] |= (ISOTP_SPEC_FRAME_FIRST << ISOTP_SPEC_FRAME_TYPE_SHIFT) & ISOTP_SPEC_FRAME_TYPE_MASK;

            //  Clear length bits
            frame_data[ISOTP_SPEC_FRAME_FIRST_LEN_MSB_IDX] &= (uint8_t)~ISOTP_SPEC_FRAME_FIRST_LEN_MSB_MASK;
            frame_data[ISOTP_SPEC_FRAME_FIRST_LEN_LSB_IDX] &= (uint8_t)~ISOTP_SPEC_FRAME_FIRST_LEN_LSB_MASK;

            //  Insert length
            switch (session->protocol_config.frame_format) {
            case ISOTP_FORMAT_FD: {
                    //  Set FD length
                    size_t length = session->full_transmission_length;
                    for (size_t i = ISOTP_SPEC_FRAME_FIRST_FD_MSB_IDX; i <= ISOTP_SPEC_FRAME_FIRST_FD_LSB_IDX; ++i) {
                        frame_data[i] = (uint8_t)(length & 0xFF);
                        length >>= 8;
                    }
                    break;
                }
                case ISOTP_FORMAT_NORMAL:
                case ISOTP_FORMAT_LIN:
                    //  Set the length
                    frame_data[ISOTP_SPEC_FRAME_FIRST_LEN_MSB_IDX] |= (session->full_transmission_length >> 8) & ISOTP_SPEC_FRAME_FIRST_LEN_MSB_MASK;
                    frame_data[ISOTP_SPEC_FRAME_FIRST_LEN_LSB_IDX] |= session->full_transmission_length & ISOTP_SPEC_FRAME_FIRST_LEN_LSB_MASK;
                    break;
            }

            //  Setup parameters
            const uint8_t* packet_start = (uint8_t*)session->tx_buffer;
            const size_t packet_len = frame_size - ISOTP_SPEC_FRAME_FIRST_DATASTART_IDX;

            //  Copy data
            memcpy(frame_data + ISOTP_SPEC_FRAME_FIRST_DATASTART_IDX, packet_start, packet_len);

            //  Advance buffer
            session->buffer_offset += packet_len;

            //  Send frame data
            ret_frame_size = packet_len + ISOTP_SPEC_FRAME_FIRST_DATASTART_IDX;
        }
    }
    else {
        //  Consecutive frame
        frame_data[ISOTP_SPEC_FRAME_TYPE_IDX] &= ~ISOTP_SPEC_FRAME_TYPE_MASK; // Clear the type bits
        frame_data[ISOTP_SPEC_FRAME_TYPE_IDX] |= (ISOTP_SPEC_FRAME_CONSECUTIVE << ISOTP_SPEC_FRAME_TYPE_SHIFT) & ISOTP_SPEC_FRAME_TYPE_MASK;

        //  Set the index
        frame_data[ISOTP_SPEC_FRAME_CONSECUTIVE_INDEX_IDX] &= ~ISOTP_SPEC_FRAME_CONSECUTIVE_INDEX_MASK; // Clear the index bits
        frame_data[ISOTP_SPEC_FRAME_CONSECUTIVE_INDEX_IDX] |= session->fc_idx_track_consecutive & ISOTP_SPEC_FRAME_CONSECUTIVE_INDEX_MASK;

        //  Setup parameters
        const uint8_t* packet_start = (uint8_t*)session->tx_buffer + session->buffer_offset;
        const size_t bytes_remaining = session->full_transmission_length - session->buffer_offset;

        //  Calculate packet length
        size_t packet_len = frame_size - ISOTP_SPEC_FRAME_CONSECUTIVE_DATASTART_IDX;
        if(packet_len > bytes_remaining) {
            packet_len = bytes_remaining;
        }

        //  Copy data
        memcpy(frame_data + ISOTP_SPEC_FRAME_CONSECUTIVE_DATASTART_IDX, packet_start, packet_len);

        //  Advance buffer
        session->buffer_offset += packet_len;

        //  Increment expected index
        session->fc_idx_track_consecutive++;
        if(session->fc_idx_track_consecutive > session->protocol_config.consecutive_index_end) {
            session->fc_idx_track_consecutive = session->protocol_config.consecutive_index_start;
        }

        //  Desired separation time
        if(requested_separation_uS != NULL) { *requested_separation_uS = session->fc_requested_separation_uS; }

        //  Send frame data
        ret_frame_size = packet_len + ISOTP_SPEC_FRAME_CONSECUTIVE_DATASTART_IDX;
    }
    
    //  Decrement allowed frames counter
    decrement_fc_allowed_frames(session);

    //  Check if we need to enter flow control wait mode (LIN does not have FC)
    if(session->fc_allowed_frames_remaining == 0 && session->protocol_config.frame_format != ISOTP_FORMAT_LIN) {
        session->state = ISOTP_SESSION_TRANSMITTING_AWAITING_FC;
    }

    //  Check if done
    if(session->buffer_offset >= session->full_transmission_length) {
        //  Done
        isotp_session_idle(session);
    }

    //  Return
    return ret_frame_size;
}

size_t tx_recieving(isotp_session_t* session, uint8_t* frame_data, const size_t frame_size, uint32_t* requested_separation_uS) {
    //  Verify we are recieving data
    if(session->state != ISOTP_SESSION_RECEIVING) {
        return 0;
    }

    //  Return value
    if(requested_separation_uS != NULL) { *requested_separation_uS = 0; }
    size_t return_val = 0;

    //  No FC in LIN busses
    if(session->protocol_config.frame_format == ISOTP_FORMAT_LIN) {
        return 0;
    }

    //  When recieving, we just need to check if a flow control frame is needed
    if(session->fc_allowed_frames_remaining == 0) {
        //  Send flow control frame
        //  TODO: Add ability to wait and abort?
        isotp_flow_control_flags_t fc_flag = ISOTP_SPEC_FC_FLAG_CONTINUE_TO_SEND;

        //  Reset block size and request configured number of frames
        session->fc_allowed_frames_remaining = session->fc_requested_block_size;
        if(session->fc_allowed_frames_remaining == 0) {
            session->fc_allowed_frames_remaining = UINT16_MAX;
        }
        
        //  Seperation time
        uint8_t seperation_time = isotp_spec_fc_separation_time_byte(session->fc_requested_separation_uS);

        //  Assemble CAN frame
        frame_data[ISOTP_SPEC_FRAME_TYPE_IDX] &= ~ISOTP_SPEC_FRAME_TYPE_MASK; // Clear the type bits
        frame_data[ISOTP_SPEC_FRAME_TYPE_IDX] |= (ISOTP_SPEC_FRAME_FLOW_CONTROL << ISOTP_SPEC_FRAME_TYPE_SHIFT) & ISOTP_SPEC_FRAME_TYPE_MASK;

        // Set the flow control flag bits (lower nibble of the same byte)
        frame_data[ISOTP_SPEC_FRAME_FLOWCONTROL_FC_FLAGS_IDX] &= ~ISOTP_SPEC_FRAME_FLOWCONTROL_FC_FLAGS_MASK; // Clear the flag bits
        frame_data[ISOTP_SPEC_FRAME_FLOWCONTROL_FC_FLAGS_IDX] |= fc_flag & ISOTP_SPEC_FRAME_FLOWCONTROL_FC_FLAGS_MASK;

        // Set the block size
        uint8_t block_size_send = session->fc_allowed_frames_remaining == UINT16_MAX ? ISOTP_SPEC_FRAME_FLOWCONTROL_BLOCKSIZE_SEND_WITHOUT_FC : session->fc_allowed_frames_remaining;
        frame_data[ISOTP_SPEC_FRAME_FLOWCONTROL_BLOCKSIZE_IDX] = block_size_send & ISOTP_SPEC_FRAME_FLOWCONTROL_BLOCKSIZE_MASK;

        //  Set the seperation time
        frame_data[ISOTP_SPEC_FRAME_FLOWCONTROL_SEPARATION_TIME_IDX] = seperation_time & ISOTP_SPEC_FRAME_FLOWCONTROL_SEPARATION_TIME_MASK;

        //  Set frame length
        return_val = ISOTP_SPEC_FRAME_FLOWCONTROL_HEADER_END;

        //  Reset flow control counter
        session->fc_allowed_frames_remaining = session->protocol_config.fc_default_request_size;
        if(session->fc_allowed_frames_remaining == 0) {
            session->fc_allowed_frames_remaining = UINT16_MAX;
        }
    }

    //  Return
    return return_val;
}

size_t isotp_session_can_tx(isotp_session_t* session, uint8_t* frame_data, const size_t frame_size, uint32_t* requested_separation_uS) {
    //  Safety
    if(session == NULL || frame_data == NULL) {
        return 0;
    }

    //  Determine action based on session state
    uint32_t ret_frame_length = 0;
    switch(session->state) {
        case ISOTP_SESSION_IDLE:
        case ISOTP_SESSION_RECEIVED:
        case ISOTP_SESSION_TRANSMITTING_AWAITING_FC:
            //  No need to transmit
            ret_frame_length = 0;
            break;
        case ISOTP_SESSION_TRANSMITTING:
            //  Transmitting
            ret_frame_length = tx_transmitting(session, frame_data, frame_size, requested_separation_uS);
            break;
        case ISOTP_SESSION_RECEIVING:
            //  Receiving
            ret_frame_length = tx_recieving(session, frame_data, frame_size, requested_separation_uS);
            break;
    }

    //  Padding (if enabled)
    if(session->protocol_config.padding_enabled && ret_frame_length > 0) {
        for(size_t i = ret_frame_length; i < frame_size; i++) {
            frame_data[i] = session->protocol_config.padding_byte;
        }

        //  Update frame length
        ret_frame_length = frame_size;
    }

    //  CAN TX callback
    if(session->callback_can_tx != NULL && ret_frame_length > 0) { session->callback_can_tx(session, frame_data, ret_frame_length); }

    //  No action taken
    return ret_frame_length;
}

/*

    Helpers

*/
void isotp_session_idle(isotp_session_t* session) {
    //  Safety
    if(session == NULL) {
        return;
    }

    //  Reset session state
    session->state = ISOTP_SESSION_IDLE;
    session->fc_allowed_frames_remaining = 0;
    session->fc_requested_separation_uS = session->protocol_config.fc_default_separation_time;
    session->fc_requested_block_size = session->protocol_config.fc_default_request_size;
    session->buffer_offset = 0;
    session->full_transmission_length = 0;
    session->fc_idx_track_consecutive = session->protocol_config.consecutive_index_first;
}

size_t isotp_session_send(isotp_session_t* session, const uint8_t* data, const size_t data_length) {
    //  Safety
    if(session == NULL || data == NULL || data_length == 0) {
        return 0;
    }

    //  Reset session state
    isotp_session_idle(session);

    //  Copy data into buffer
    size_t copy_len = data_length;
    if(copy_len > session->tx_len) {
        copy_len = session->tx_len;
    }
    memcpy(session->tx_buffer, data, copy_len);

    //  Set transmit length
    session->full_transmission_length = copy_len;

    //  Update session state
    session->state = ISOTP_SESSION_TRANSMITTING;

    //  Return
    return copy_len;
}

void isotp_session_init(isotp_session_t* session, const isotp_format_t frame_format, void* tx_buffer, size_t tx_len, void* rx_buffer, size_t rx_len) {
    //  Safety
    if(session == NULL) {
        return;
    }

    //  Default protocol configuration
    session->protocol_config.padding_enabled = true;
    session->protocol_config.padding_byte = 0xFF;
    session->protocol_config.consecutive_index_first = ISOTP_SPEC_FRAME_CONSECUTIVE_INDEXING_START;
    session->protocol_config.consecutive_index_start = ISOTP_SPEC_FRAME_CONSECUTIVE_INDEXING_MIN;
    session->protocol_config.consecutive_index_end = ISOTP_SPEC_FRAME_CONSECUTIVE_INDEXING_MAX;
    session->protocol_config.fc_default_request_size = 0;   //  request all frames by default
    session->protocol_config.fc_default_separation_time = 0;    //  no delay by default
    session->protocol_config.frame_format = frame_format;

    //  Load buffers
    session->tx_buffer = tx_buffer;
    session->tx_len = tx_len;
    session->rx_buffer = rx_buffer;
    session->rx_len = rx_len;

    //  Clear callbacks
    session->callback_can_rx = NULL;
    session->callback_can_tx = NULL;
    session->callback_transmission_rx = NULL;
    session->callback_mem_assign = NULL;
    session->callback_peek_first_frame = NULL;
    session->callback_peek_consecutive_frame = NULL;
    session->callback_error_invalid_frame = NULL;
    session->callback_error_transmission_too_large = NULL;
    session->callback_error_partner_aborted_transfer = NULL;
    session->callback_error_unexpected_frame_type = NULL;
    session->callback_error_consecutive_out_of_order = NULL;

    //  Reset session state
    isotp_session_idle(session);
}

bool isotp_session_use_rx_buffer(isotp_session_t* session, void* rx_buffer, size_t rx_len) {
    //  Safety
    if(session == NULL || rx_buffer == NULL || rx_len == 0) {
        return false;
    }

    //  Check state
    if(session->state != ISOTP_SESSION_IDLE && session->state != ISOTP_SESSION_RECEIVING) {
        return false;
    }

    //  Recieving state callback only
    if(session->state == ISOTP_SESSION_RECEIVING && session->buffer_offset != 0) {
        return false;
    }

    //  Update buffer
    session->rx_buffer = rx_buffer;
    session->rx_len = rx_len;

    //  Success
    return true;
}