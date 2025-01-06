# ISOTPlib
ISOTPlib is a flexible C library designed to make implementing the ISO-TP CAN communication layer a breeze. It can be used for raw data transfer, but it is designed to be used as an abstraction layer between a CAN interface and the [UDS FlexDiag library](https://github.com/nickdaria/FlexDiag). Unlike other libraries, this one is not designed to deal with loops, arbitration IDs, timers, or CAN drivers - instead only taking care of what ISO-TP is actually responsible for.

# 🚀 Key Features
- **Simple operation:** Simply register callbacks at setup, pass in recieved CAN frames, and check for frames to send and you're good to send and recieve transmissions!
- **No dynamic memory allocation required:** The library is designed to be used with statically allocated buffers, but dynamic buffer sizes can be used if desired.
- **Non-intrusive scope:** ISOTPlib does not handle CAN IDs, CAN interfaces, or data processing. It also has no loop. It simply handles incoming frames and generates outgoing frames by request to facilitate single/multi-frame transfers.
- **Concurrent Sessions:** The struct-based design allows for an unlimited number of sessions.
- **Error Detection:** Missed continuous frame and timeout callbacks
- **Customizable Padding:** Each instance can have outgoing frames padded with a custom byte or sent at actual length depending on recievers requirements.

# 🎯 Goals
- **Reliability:** Statically allocated memory and clear code design patterns
- **FlexDiag:** Easily tie into the powerful UDS-based framework of FlexDiag
- **Platform-agnostic:** Codebase compatible with embedded/Arduino/Desktop and independent of CAN interfaces or libraries

## 📖 Use Cases
- Automotive Diagnostics

## ✏️ How to implement
1. Create a `isotp_session_t` and initialize it with `isotp_session_init(...)`
2. Register desired callbacks
3. When a CAN message is recieved, call `isotp_session_can_rx(isotp_session_t* session, const uint8_t* frame_data, const size_t frame_length)`
4. To send a CAN message, call `size_t isotp_session_can_tx(isotp_session_t* session, uint8_t* frame_data, const size_t frame_size, uint32_t* requested_separation_uS)`
    - To support separation time, don't check again for the provided `requested_separation_uS` has elapsed
5. Done!
    - Don't forget to call `isotp_session_idle(isotp_session_t* session)` in your recieved data callback to release the session, even if you didn't respond.
    - If you'd like, you can change protocol options (padding byte, padding enabled, etc) in the session's `isotp_protocol_config_t`