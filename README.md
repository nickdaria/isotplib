# ISOTPlib
Highly configurable and flexible C library for implementing ISO 15765-2 (Transport Layer) for CAN, CAN-FD, and LIN networks.

> [!IMPORTANT] 
This library solely focuses on implementation of the ISO-TP protocol. ISO-TP is typically used to implement Unified Diagnostic Services (UDS). This library is designed to work perfectly with [udslib](https://github.com/nickdaria/udslib).

# 🚀 Key Features
- CAN, CAN-FD, and LIN implementations of ISO-TP
- Easy-to-use callbacks and error handling
- Pure C, platform agnostic
- Static memory allocation
- Tight scope - no bloat

# ⚡️ Advanced Features
- Concurrent sessions
- Per-session protocol configuration - padding enable, padding byte, consecutive index ordering, etc
- Supports user implementation of dynamic RX memory allocation

# ✏️ Usage
See `examples/` for functioning code (command line & microcontroller)
1. Create a session
    ```C
    static isotp_session_t can_session;
    ```
2. Add required callbacks
    ```C
    void transmission_rx_callback(void* context) {
        auto* session_context = (isotp_session_t*)context;

        // Do as you please with the data
        for (size_t i = 0; i < session_context->full_transmission_length; i++) {
            byte this_idx = ((uint8_t*)session_context->rx_buffer)[i];
        }

        uint8_t deny_buf = {0x7F, 0x45};

        //  Must send data or isotp_session_idle(context) if ignoring
        isotp_session_send(session_context, deny_buf, sizeof(deny_buf));
    }

    void error_invalid_frame_callback(void* context, const isotp_spec_frame_type_t rx_frame_type, const uint8_t* msg_data, const size_t msg_length) {
        isotp_session_idle((isotp_session_t*)context);
    }

    void error_partner_aborted_transfer_callback(void* context, const uint8_t* msg_data, const size_t msg_length) {
        isotp_session_idle((isotp_session_t*)context);
    }

    void error_transmission_too_large_callback(void* context, const uint8_t* data, const size_t length, const size_t requested_size) {
        isotp_session_idle((isotp_session_t*)context);
    }

    void error_consecutive_out_of_order_callback(void* context, const uint8_t* data, const size_t length, const uint8_t expected_index, const uint8_t received_index) {
        isotp_session_idle((isotp_session_t*)context);
    }

    void error_unexpected_frame_type_callback(void* context, const uint8_t* msg_data, const size_t msg_length) {
        isotp_session_idle((isotp_session_t*)context);
    }
    ```
3. Initialize session and bind required callbacks
    ```C
    isotp_session_init(&can_session, ISOTP_FORMAT_NORMAL, tx_buf, sizeof(tx_buf), rx_buf, sizeof(rx_buf));

    can_session.callback_transmission_rx = transmission_rx_callback;
    can_session.callback_error_invalid_frame = error_invalid_frame_callback;
    can_session.callback_error_partner_aborted_transfer = error_partner_aborted_transfer_callback;
    can_session.callback_error_transmission_too_large = error_transmission_too_large_callback;
    can_session.callback_error_consecutive_out_of_order = error_consecutive_out_of_order_callback;
    can_session.callback_error_unexpected_frame_type = error_unexpected_frame_type_callback;
    ```
    - **(optional)** Override protocol config
        ```C
        can_session.protocol_config.padding_enabled = true;
        can_session.protocol_config.padding_byte = 0xAA;
        ```
4. Pass in recieved CAN frames (must be filtered)
    ```C
    //  This message struct will be HAL/CAN library specific
    switch(msg.id) {
        case ECU_DIAG_RESPONSE:
            isotp_session_can_rx(&can_session, msg.data, msg.length)
            break;
    }
    ```
5. Send queued frames (block for requested uS to support separation time)
    ```C
    //  micros() on Arduino, esp_timer_get_time() on IDF, etc
    //  you can load new messages into a HAL struct or raw byte array
    static uint64_t next_tx_uS;
    if(hal_micros() >= next_tx_uS) {
        can_msg_t tx_msg;
        uint32_t requested_separation_uS = 0;

        //  Load message
        size_t uds_tx_len = isotp_session_can_tx(&can_session, tx_msg.data, sizeof(tx_msg.data), &requested_separation_uS);
        tx_msg.length = uds_tx_len;

        //  Update timestamp
        next_tx_time_uS = hal_micros() + requested_separation_uS;

        //  Send
        if(uds_tx_len > 0) {
            hal_can_send(tx_msg);
        }
    }
    ```
6. Done! Now you are responding to messages and sending responses. You can also call `isotp_session_send()` from your program to initiate requests