#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "../FlexISOTP.h"

//  FlexISOTP session
flexisotp_session_t session;
uint8_t tx_buffer[128];
uint8_t rx_buffer[256];

uint8_t can_tx_buf[8];

//  Prototypes for command functions
void cmd_enter();
void cmd_buf();
void cmd_hexdata(const uint8_t* buf, const size_t len);
void cmd_sendTest();

//  Gets line from user (not including newline). Can be empty (enter pressed).
size_t user_rx_cmd(uint8_t* buffer, const size_t buffer_size) {
    size_t length = 0;
    while (length < buffer_size) {
        int c = getchar();
        if (c == '\n' || c == EOF) {
            break;
        }
        buffer[length++] = c;
    }
    buffer[length] = '\0'; // Null-terminate for safety
    return length;
}

//  Processes user commands
void usr_process_cmd(const uint8_t* buffer, const size_t length) {
    // Empty input
    if (length == 0) {
        cmd_enter();
        return;
    }

    // 'b' command
    if (length == 1 && buffer[0] == 'b') {
        cmd_buf();
        return;
    }

    // 'c' command
    if (length == 1 && buffer[0] == 'c') {
        cmd_sendTest(); // Pass 0 as a placeholder
        return;
    }

    // Attempt to parse as hex
    uint8_t hex_buffer[128];
    size_t hex_len = 0;

    for (size_t i = 0; i < length; i += 2) {
        if (i + 1 >= length || !isxdigit(buffer[i]) || !isxdigit(buffer[i + 1])) {
            printf("[ERROR] Invalid hex input: '%c%c'\n", buffer[i], i + 1 < length ? buffer[i + 1] : ' ');
            return;
        }
        sscanf((const char*)&buffer[i], "%2hhX", &hex_buffer[hex_len++]);
    }

    // Call cmd_hexdata with the parsed hex buffer
    cmd_hexdata(hex_buffer, hex_len);
}

//  Command '': Enter pressed
void cmd_enter() {
    uint32_t requested_separation_time = 0;
    size_t tx_size = flexisotp_session_can_tx(&session, can_tx_buf, sizeof(can_tx_buf), &requested_separation_time);


    if(tx_size > 0) {
        //printf("[TX] ");
        // for (size_t i = 0; i < tx_size; i++) {
        //     printf("%02X ", can_tx_buf[i]);
        // }
        printf(" (Separation: %u)", requested_separation_time);
    } else {
        printf("No frame to send");
    }

    printf("\n");
}

//  Command 'b': Example function
void cmd_buf() {
    printf("[bufdump]\n");
}

//  Command for hex data
void cmd_hexdata(const uint8_t* buf, const size_t len) {
    // printf("[RX] ", len);
    // for (size_t i = 0; i < len; i++) {
    //     printf("%02X ", buf[i]);
    // }
    // printf("\n");

    //  Pass into FlexISOTP
    flexisotp_session_can_rx(&session, buf, len);
}

void sendTestData(const size_t len) {
    uint8_t test_buf[len];

    for(size_t i = 1; i < len; i++) {
        test_buf[i] = i;
    }

    flexisotp_session_send(&session, test_buf, len);
}

//  Command for sending test data
void cmd_sendTest() {
    printf("Enter decimal size for test: ");
    fflush(stdout);

    char input[16];
    size_t size = 0;

    if (fgets(input, sizeof(input), stdin)) {
        size = strtoul(input, NULL, 10);

        // Validate input size
        if (size > 0 && size <= sizeof(tx_buffer)) {
            sendTestData(size);
        } else {
            printf("[ERROR] Size must be between 1 and %lu.\n", sizeof(tx_buffer));
        }
    } else {
        printf("[ERROR] Failed to read input.\n");
    }
}

//  Main loop
void playground() {
    // Get user input
    uint8_t buffer[256];
    size_t length = user_rx_cmd(buffer, sizeof(buffer));

    // Process user command
    usr_process_cmd(buffer, length);
}

/*
    Callbacks
*/
void cb_can_rx(flexisotp_session_t *session, const uint8_t *msg_data, const size_t msg_length) {
    printf("[RX] ");
    for (size_t i = 0; i < msg_length; i++) {
        printf("%02X ", msg_data[i]);
    }
    printf("\n");
}

void cb_can_tx(flexisotp_session_t *session, const uint8_t *msg_data, const size_t msg_length) {
    printf("[TX] ");
    for (size_t i = 0; i < msg_length; i++) {
        printf("%02X ", msg_data[i]);
    }
    printf("\n");
}

void cb_error_invalid_frame(flexisotp_session_t *context, const isotp_spec_frame_type_t rx_frame_type, const uint8_t *msg_data, const size_t msg_length) {
    printf("[ERR] Invalid frame, type: %u, length: %u ", rx_frame_type, msg_length);
    for (size_t i = 0; i < msg_length; i++) {
        printf("%02X ", msg_data[i]);
    }
    printf("\n");

    flexisotp_session_idle(context);
}

void cb_error_unexpected_frame_type(flexisotp_session_t *context, const uint8_t *msg_data, const size_t msg_length) {
    printf("[ERR] Unexpected frame type, length: %u ", msg_length);
    for (size_t i = 0; i < msg_length; i++) {
        printf("%02X ", msg_data[i]);
    }
    printf("\n");

    flexisotp_session_idle(context);
}

void cb_error_partner_aborted_transfer(flexisotp_session_t *context, const uint8_t *msg_data, const size_t msg_length) {
    printf("[ERR] Partner aborted transfer, length: %u ", msg_length);
    for (size_t i = 0; i < msg_length; i++) {
        printf("%02X ", msg_data[i]);
    }
    printf("\n");
    
    flexisotp_session_idle(context);
}

void cb_error_transmission_too_large(flexisotp_session_t *context, const uint8_t *data, const size_t length, const size_t requested_size) {
    printf("[ERR] Transmission too large, length: %u, requested: %u ", length, requested_size);
    for (size_t i = 0; i < length; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
    
    flexisotp_session_idle(context);
}

void cb_error_consecutive_out_of_order(flexisotp_session_t *context, const uint8_t *data, const size_t length, const uint8_t expected_index, const uint8_t received_index) {
    printf("[ERR] Consecutive out of order, length: %u, expected: %u, received: %u ", length, expected_index, received_index);
    for (size_t i = 0; i < length; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
    
    flexisotp_session_idle(context);
}

void cb_transmission_rx(flexisotp_session_t *context) {
    printf("[CB Recieved] ");
    uint8_t* buffer = (uint8_t*)context->rx_buffer; // Cast rx_buffer to uint8_t*
    for(size_t i = 0; i < context->full_transmission_length; i++) {
        printf("%02X ", buffer[i]); // Index the casted buffer
    }
    printf("\n");

    //  Reset session
    flexisotp_session_idle(context);
}

void cb_peek_first_frame(flexisotp_session_t *context, const uint8_t *data, const size_t length) {
    printf("[CB] Peek first frame, length: %u ", length);
    for (size_t i = 0; i < length; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

void cb_peek_consecutive_frame(flexisotp_session_t *context, const uint8_t *data, const size_t length, const size_t start_idx) {
    printf("[CB] Peek consecutive frame, length: %u, start: %u ", length, start_idx);
    for (size_t i = 0; i < length; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

int main() {
    flexisotp_session_init(&session, &tx_buffer, sizeof(tx_buffer), &rx_buffer, sizeof(rx_buffer));

    //  Register callbacks
    session.callback_can_rx = (void (*)(void *, const uint8_t *, const size_t))cb_can_rx;
    session.callback_can_tx = (void (*)(void *, const uint8_t *, const size_t))cb_can_tx;
    session.error_invalid_frame = (void (*)(void *, const isotp_spec_frame_type_t, const uint8_t *, const size_t))cb_error_invalid_frame;
    session.error_unexpected_frame_type = (void (*)(void *, const uint8_t *, const size_t))cb_error_unexpected_frame_type;
    session.error_partner_aborted_transfer = (void (*)(void *, const uint8_t *, const size_t))cb_error_partner_aborted_transfer;
    session.error_transmission_too_large = (void (*)(void *, const uint8_t *, const size_t, const size_t))cb_error_transmission_too_large;
    session.error_consecutive_out_of_order = (void (*)(void *, const uint8_t *, const size_t, const uint8_t, const uint8_t))cb_error_consecutive_out_of_order;
    session.callback_transmission_rx = (void (*)(void *))cb_transmission_rx;
    session.callback_peek_first_frame = (void (*)(void *, const uint8_t *, const size_t))cb_peek_first_frame;
    session.callback_peek_consecutive_frame = (void (*)(void *, const uint8_t *, const size_t, const size_t))cb_peek_consecutive_frame;


    while (1) {
        printf("> "); // Prompt user
        fflush(stdout); // Ensure prompt is visible
        playground();
    }
    return 0;
}
