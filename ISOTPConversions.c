#include "ISOTPConversions.h"
#include "ISOTPConstants.h"

uint32_t isotp_fc_separation_time_us(uint8_t req_sep_byte) {
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

uint8_t isotp_fc_separation_time_byte(uint32_t uS) {
    if(uS == 0) {
        return ISOTP_SPEC_FRAME_FLOWCONTROL_BLOCKSIZE_SEND_WITHOUT_FC;
    }
    else if(uS > ISOTP_SPEC_FC_SEPERATION_TIME_MS_MIN && uS <= ISOTP_SPEC_FC_SEPERATION_TIME_MS_MAX) {
        return uS / ISOTP_SPEC_FC_SEPERATION_TIME_MS_SCALAR;
    }
    else if(uS >= ISOTP_SPEC_FC_SEPERATION_TIME_uS_MIN && uS <= ISOTP_SPEC_FC_SEPERATION_TIME_uS_MAX) {
        return (uS / ISOTP_SPEC_FC_SEPERATION_TIME_uS_SCALAR) + ISOTP_SPEC_FC_SEPERATION_TIME_uS_MIN - 1;
    }
    else {
        //  Invalid input
        return ISOTP_SPEC_FRAME_FLOWCONTROL_BLOCKSIZE_SEND_WITHOUT_FC;
    }
}