#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/twai.h"
#include "isotp_session.h"

//  TWAI
#define TX_GPIO_NUM 21
#define RX_GPIO_NUM 22
#define TWAI_BAUD_RATE 500000

//  CAN IDs
#define CAN_ID_INCOMING_REQUEST 0x7E8       //  Messages from OBD-II ECM
#define CAN_ID_OUTGOING_RESPONSE 0x7E0      //  Messages to OBD-II ECM

//  Session
static isotp_session_t can_session;
static uint8_t tx_buf[512];
static uint8_t rx_buf[512];

// Callbacks
void transmission_rx_callback(void* context) {
    //  Example: Respond with the same message but reversed
    reversed_buf[context->full_transmission_length];

    for(size_t i = 0; i < context->full_transmission_length; i++) {
        reversed_buf[i] = context->rx_buffer[context->full_transmission_length - i - 1];
    }

    isotp_session_send(context, reversed_buf, sizeof(reversed_buf));
}

void error_callback(void* context, const uint8_t* msg_data, const size_t msg_length) {
    isotp_session_idle((isotp_session_t*)context);
}

// CAN Task
void CAN_task(void *arg) {
    // Configure TWAI driver
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TX_GPIO_NUM, RX_GPIO_NUM, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    // Install and start the TWAI driver
    if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
        vTaskDelete(NULL);
    }

    if (twai_start() != ESP_OK) {
        vTaskDelete(NULL);
    }

    twai_message_t rx_msg;

    // Infinite loop to read messages from the CAN bus
    while (1) {
        //  Recieve
        if (twai_receive(&rx_msg, 0) == ESP_OK) {
            if (rx_msg.identifier == CAN_ID_INCOMING_REQUEST) {
                isotp_session_can_rx(&can_session, rx_msg.data, rx_msg.data_length_code);
            }
        }

        //  Send
        static next_tx_time_uS = 0;
        if(next_tx_time_uS >= esp_timer_get_time()) {
            twai_message_t tx_msg;

            uint32_t requested_separation_uS;
            size_t uds_tx_len = isotp_session_can_tx(can_session, tx_msg.data, tx_msg.data_length_code, &requested_separation_uS);

            next_tx_time_uS = esp_timer_get_time() + requested_separation_uS;
        }
    }

    // Clean up on task exit
    twai_stop();
    twai_driver_uninstall();
    vTaskDelete(NULL);
}

void app_main() {
    // Initialize CAN session
    isotp_session_init(&can_session, ISOTP_FORMAT_NORMAL, tx_buf, sizeof(tx_buf), rx_buf, sizeof(rx_buf));
    can_session.callback_transmission_rx = transmission_rx_callback;
    can_session.callback_error_invalid_frame = error_callback;
    can_session.callback_error_partner_aborted_transfer = error_callback;
    can_session.callback_error_transmission_too_large = error_callback;
    can_session.callback_error_consecutive_out_of_order = error_callback;
    can_session.callback_error_unexpected_frame_type = error_callback;

    // Create the CAN task
    xTaskCreate(CAN_task, "CAN_task", 4096, NULL, 5, NULL);
}