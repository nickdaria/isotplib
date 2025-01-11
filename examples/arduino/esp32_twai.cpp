#include <Arduino.h>
#include "driver/twai.h"
#include "isotplib.h"

// TWAI Configuration
#define TX_GPIO_NUM D10
#define RX_GPIO_NUM D9
#define TWAI_BAUD_RATE 500000

// CAN IDs
#define CAN_ID_INCOMING_REQUEST 0x7E8 // Messages from OBD-II ECM
#define CAN_ID_OUTGOING_RESPONSE 0x7E0 // Messages to OBD-II ECM

// Session
static isotp_session_t can_session;
static uint8_t tx_buf[512];
static uint8_t rx_buf[512];

// Callbacks
void transmission_rx_callback(void* context) {
    uint8_t reversed_buf[512];
    auto* session_context = (isotp_session_t*)context;

    // Example: Respond with the same message but reversed
    for (size_t i = 0; i < session_context->full_transmission_length; i++) {
        reversed_buf[i] = ((uint8_t*)session_context->rx_buffer)[session_context->full_transmission_length - i - 1];
    }

    isotp_session_send(session_context, reversed_buf, session_context->full_transmission_length);
}

// Invalid frame received
void error_invalid_frame_callback(void* context, const isotp_spec_frame_type_t rx_frame_type, const uint8_t* msg_data, const size_t msg_length) {
    Serial.println("Error: Invalid frame received.");
    isotp_session_idle((isotp_session_t*)context); // Reset session to idle
}

// Partner aborted the transfer
void error_partner_aborted_transfer_callback(void* context, const uint8_t* msg_data, const size_t msg_length) {
    Serial.println("Error: Partner aborted transfer.");
    isotp_session_idle((isotp_session_t*)context); // Reset session to idle
}

// Transmission too large for RX buffer
void error_transmission_too_large_callback(void* context, const uint8_t* data, const size_t length, const size_t requested_size) {
    Serial.printf("Error: Transmission too large. Requested size: %zu, Buffer size: %zu\n", requested_size, length);
    isotp_session_idle((isotp_session_t*)context); // Reset session to idle
}

// Consecutive frame index out of order
void error_consecutive_out_of_order_callback(void* context, const uint8_t* data, const size_t length, const uint8_t expected_index, const uint8_t received_index) {
    Serial.printf("Error: Consecutive frame out of order. Expected index: %u, Received index: %u\n", expected_index, received_index);
    isotp_session_idle((isotp_session_t*)context); // Reset session to idle
}

// Unexpected frame type
void error_unexpected_frame_type_callback(void* context, const uint8_t* msg_data, const size_t msg_length) {
    Serial.println("Error: Unexpected frame type.");
    isotp_session_idle((isotp_session_t*)context); // Reset session to idle
}

bool CAN_rx() {
  // Receive
  twai_message_t rx_msg;
  if (twai_receive(&rx_msg, 0) == ESP_OK) {
      Serial.print("-> 0x");
      Serial.print(rx_msg.identifier, HEX);
      Serial.print(" -");

      for (size_t i = 0; i < rx_msg.data_length_code; i++) {
          Serial.print(" 0x");
          Serial.print(rx_msg.data[i], HEX);
      }
      Serial.println();

      if (rx_msg.identifier == CAN_ID_INCOMING_REQUEST) {
          isotp_session_can_rx(&can_session, rx_msg.data, rx_msg.data_length_code);
      }

      return true;
  }

  return false;
}

void CAN_tx() {
  // Send
  static uint32_t next_tx_time_uS = 0;
  if (esp_timer_get_time() >= next_tx_time_uS) {
      twai_message_t tx_msg = {};
      uint32_t requested_separation_uS = 0;

      size_t uds_tx_len = isotp_session_can_tx(&can_session, tx_msg.data, sizeof(tx_msg.data), &requested_separation_uS);
      if (uds_tx_len > 0) {
          tx_msg.identifier = CAN_ID_OUTGOING_RESPONSE;
          tx_msg.flags = TWAI_MSG_FLAG_NONE;
          tx_msg.data_length_code = uds_tx_len;

          Serial.print("<- 0x");
          Serial.print(tx_msg.identifier, HEX);
          Serial.print(" -");
          for (size_t i = 0; i < tx_msg.data_length_code; i++) {
              Serial.print(" 0x");
              Serial.print(tx_msg.data[i], HEX);
          }
          Serial.print(" uS: ");
          Serial.print(requested_separation_uS);
          Serial.print(" ");

          esp_err_t tx_err = twai_transmit(&tx_msg, 0);
          if (tx_err == ESP_OK) {
            Serial.println("OK");
          }
          else {
            Serial.print("Err: 0x");
            Serial.println(tx_err, HEX);
          }

          next_tx_time_uS = esp_timer_get_time() + requested_separation_uS;
      }
  }
}

// CAN Task
void CAN_task(void* arg) {
    // Configure TWAI driver
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)TX_GPIO_NUM, (gpio_num_t)RX_GPIO_NUM, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    // Install and start the TWAI driver
    if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
        Serial.println("Failed to install TWAI driver");
        vTaskDelete(NULL);
        return;
    }

    if (twai_start() != ESP_OK) {
        Serial.println("Failed to start TWAI driver");
        vTaskDelete(NULL);
        return;
    }

    // Infinite loop to read messages from the CAN bus
    while (1) {
        while(1) {
          if(!CAN_rx()) {
            break;
          }
        }

        //  Recieve
        CAN_tx();
    }

    // Clean up on task exit
    twai_stop();
    twai_driver_uninstall();
    vTaskDelete(NULL);
}

void setup() {
    Serial.begin(115200);

    // Initialize CAN session
    isotp_session_init(&can_session, ISOTP_FORMAT_NORMAL, tx_buf, sizeof(tx_buf), rx_buf, sizeof(rx_buf));
    can_session.callback_transmission_rx = transmission_rx_callback;
    can_session.callback_error_invalid_frame = error_invalid_frame_callback;
    can_session.callback_error_partner_aborted_transfer = error_partner_aborted_transfer_callback;
    can_session.callback_error_transmission_too_large = error_transmission_too_large_callback;
    can_session.callback_error_consecutive_out_of_order = error_consecutive_out_of_order_callback;
    can_session.callback_error_unexpected_frame_type = error_unexpected_frame_type_callback;

    // Create the CAN task
    xTaskCreatePinnedToCore(CAN_task, "CAN_task", 4096, NULL, 5, NULL, 1);
}

void loop() {
    // No code needed in the main loop for this example
    delay(10);
}