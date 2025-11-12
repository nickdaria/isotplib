#include "isotp_conversions.h"
#include "isotp_specification.h"

uint32_t isotp_spec_fc_separation_time_us(uint8_t req_sep_byte) {
    if (req_sep_byte >= ISOTP_SPEC_FC_SEPERATION_TIME_MS_MIN && req_sep_byte <= ISOTP_SPEC_FC_SEPERATION_TIME_MS_MAX) {
        // Millisecond delay (0x01-0x7F = 1-127ms)
        return req_sep_byte * ISOTP_SPEC_FC_SEPERATION_TIME_MS_SCALAR;
    } else if (req_sep_byte >= ISOTP_SPEC_FC_SEPERATION_TIME_uS_MIN && req_sep_byte <= ISOTP_SPEC_FC_SEPERATION_TIME_uS_MAX) {
        // Microsecond delay (0xF1-0xF9 = 100-900us)
        return (req_sep_byte - ISOTP_SPEC_FC_SEPERATION_TIME_uS_MIN + 1) * ISOTP_SPEC_FC_SEPERATION_TIME_uS_SCALAR;
    } else if (req_sep_byte == ISOTP_SPEC_FC_SEPERATION_TIME_MS_NONE) {
        // No delay (0x00)
        return 0;
    } else {
        // Invalid (0x80-0xF0, 0xFA-0xFF)
        return 0;
    }
}

uint8_t isotp_spec_fc_separation_time_byte(uint32_t uS) {
    if(uS == 0) {
        // No delay
        return ISOTP_SPEC_FC_SEPERATION_TIME_MS_NONE;
    }
    else if(uS >= 100 && uS <= 900) {
        // Microsecond delay (100-900us = 0xF1-0xF9)
        return (uS / ISOTP_SPEC_FC_SEPERATION_TIME_uS_SCALAR) + ISOTP_SPEC_FC_SEPERATION_TIME_uS_MIN - 1;
    }
    else if(uS >= 1000 && uS <= 127000) {
        // Millisecond delay (1-127ms = 0x01-0x7F)
        return uS / ISOTP_SPEC_FC_SEPERATION_TIME_MS_SCALAR;
    }
    else {
        // Invalid input - return no delay
        return ISOTP_SPEC_FC_SEPERATION_TIME_MS_NONE;
    }
}