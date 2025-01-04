#pragma once

#include <stdint.h>

/**
 * @brief Convert FC seperation time byte to uS
 * 
 * @param req_sep_byte 
 * @return uint32_t 
 */
uint32_t isotp_fc_seperation_time_us(uint8_t req_sep_byte);

/**
 * @brief Convert uS to FC seperation time byte
 * 
 * @param uS 
 * @return uint8_t 
 */
uint8_t isotp_fc_seperation_time_byte(uint32_t uS);