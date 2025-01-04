#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "../FlexISOTP.h"

//  Prototypes for command functions
void cmd_enter();
void cmd_buf();
void cmd_hexdata(const uint8_t* buf, const size_t len);

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
    printf("[o] Enter pressed.\n");
}

//  Command 'b': Example function
void cmd_buf() {
    printf("[bufdump]\n");
}

//  Command for hex data
void cmd_hexdata(const uint8_t* buf, const size_t len) {
    printf("[RX] ", len);
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", buf[i]);
    }
    printf("\n");
}

//  Main loop
void playground() {
    // Get user input
    uint8_t buffer[256];
    size_t length = user_rx_cmd(buffer, sizeof(buffer));

    // Process user command
    usr_process_cmd(buffer, length);
}

int main() {
    while (1) {
        printf("> "); // Prompt user
        fflush(stdout); // Ensure prompt is visible
        playground();
    }
    return 0;
}
