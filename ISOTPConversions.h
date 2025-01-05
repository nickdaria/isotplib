#pragma once

#include <stdint.h>

/**
 * @brief Convert FC separation time byte to uS
 * 
 * @param req_sep_byte Separation time byte
 * @return uint32_t 0 for none, 100-900 uS (100uS steps), or 1000-127000 uS (1ms steps)
 */
uint32_t isotp_fc_separation_time_us(uint8_t req_sep_byte);

/**
 * @brief Convert uS to FC separation time byte
 * 
 * @param uS 0 for none, 100-900 uS (100uS steps), or 1000-127000 uS (1ms steps)
 * @return uint8_t Separation time byte for valid entry, no separation for invalid entry
 */
uint8_t isotp_fc_separation_time_byte(uint32_t uS);