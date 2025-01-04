#pragma once

/*
    ISO-TP Constants
    FlexISOTP
    ndaria (me@nickdaria.com) @nickdaria

    This file contains constants used in the ISO-TP protocol
*/

// ISO-TP frame types
typedef enum {
	ISOTP_SPEC_FRAME_SINGLE = 0x00,
	ISOTP_SPEC_FRAME_FIRST = 0x01,
	ISOTP_SPEC_FRAME_CONSECUTIVE = 0x02,
	ISOTP_SPEC_FRAME_FLOW_CONTROL = 0x03
} isotp_spec_frame_type_t;

//  All frames
#define ISOTP_SPEC_FRAME_TYPE_IDX 0
#define ISOTP_SPEC_FRAME_TYPE_MASK 0xF0
#define ISOTP_SPEC_FRAME_TYPE_SHIFT_R 4

//  Single frame
#define ISOTP_SPEC_FRAME_SINGLE_LEN_IDX 0
#define ISOTP_SPEC_FRAME_SINGLE_LEN_MASK 0x0F

#define ISOTP_SPEC_FRAME_SINGLE_DATASTART_IDX 1

//  First frame
#define ISOTP_SPEC_FRAME_FIRST_LEN_MSB_IDX 0
#define ISOTP_SPEC_FRAME_FIRST_LEN_MSB_MASK 0x0F

#define ISOTP_SPEC_FRAME_FIRST_LEN_LSB_IDX 1
#define ISOTP_SPEC_FRAME_FIRST_LEN_LSB_MASK 0xFF

#define ISOTP_SPEC_FRAME_FIRST_DATASTART_IDX 2

//  Consecutive frame
#define ISOTP_SPEC_FRAME_CONSECUTIVE_INDEX_IDX 0
#define ISOTP_SPEC_FRAME_CONSECUTIVE_INDEX_MASK 0x0F

#define ISOTP_SPEC_FRAME_CONSECUTIVE_DATASTART_IDX 2

//  Flow control frame
#define ISOTP_SPEC_FRAME_FLOWCONTROL_FC_FLAGS_IDX 0
#define ISOTP_SPEC_FRAME_FLOWCONTROL_FC_FLAGS_MASK 0x0F

#define ISOTP_SPEC_FRAME_FLOWCONTROL_BLOCKSIZE_IDX 1
#define ISOTP_SPEC_FRAME_FLOWCONTROL_BLOCKSIZE_MASK 0xFF

//  Flow control flags
typedef enum {
	ISOTP_SPEC_FC_FLAG_CONTINUE_TO_SEND = 0,
    ISOTP_SPEC_FC_FLAG_WAIT = 1,
    ISOTP_SPEC_FC_FLAG_OVERFLOW_ABORT = 2,
} isotp_flow_control_flags_t;

//  Flow control block size value to ignore FC
#define ISOTP_SPEC_FRAME_FLOWCONTROL_BLOCKSIZE_SEND_WITHOUT_FC 0

//  Flow control requested delay calculations
#define ISOTP_SPEC_FC_SEPERATION_TIME_MS_MIN 0
#define ISOTP_SPEC_FC_SEPERATION_TIME_MS_MAX 0x7F
#define ISOTP_SPEC_FC_SEPERATION_TIME_MS_SCALAR 1000

#define ISOTP_SPEC_FC_SEPERATION_TIME_uS_MIN 0xF1
#define ISOTP_SPEC_FC_SEPERATION_TIME_uS_MAX 0xF9
#define ISOTP_SPEC_FC_SEPERATION_TIME_uS_SCALAR 100

/**
 * @brief Calculate the seperation time in microseconds for a given flow control seperation time byte
 */
inline uint32_t isotp_fc_seperation_time_us(uint8_t req_sep_byte) {
    if (req_sep_byte > ISOTP_SPEC_FC_SEPERATION_TIME_MS_MIN && req_sep_byte <= ISOTP_SPEC_FC_SEPERATION_TIME_MS_MAX) {
        // Millisecond delay
        return req_sep_byte * ISOTP_SPEC_FC_SEPERATION_TIME_MS_SCALAR;
    } else if (req_sep_byte >= ISOTP_SPEC_FC_SEPERATION_TIME_uS_MIN && req_sep_byte <= ISOTP_SPEC_FC_SEPERATION_TIME_uS_MAX) {
        // Microsecond delay
        return (req_sep_byte - ISOTP_SPEC_FC_SEPERATION_TIME_uS_MIN + 1) * ISOTP_SPEC_FC_SEPERATION_TIME_uS_SCALAR;
    } else if (req_sep_byte == ISOTP_SPEC_FC_SEPERATION_TIME_MS_MIN) {
        // No delay
        return 0;
    } else {
        // Invalid
        return 0;
    }
}