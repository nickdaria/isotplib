# FlexISOTP
FlexISOTP is a flexible C library designed to make implementing the ISO-TP CAN communication layer a breeze. It can be used for raw data transfer, but it is designed to be used as an abstraction layer between a CAN interface and the [UDS FlexDiag library](https://github.com/nickdaria/FlexDiag).

# 🚀 Key Features
- **Simple operation:** Simply register callbacks at setup, pass in recieved CAN frames, and check for frames to send and you're good to send and recieve transmissions!
- **No dynamic memory allocation required:** The library is designed to be used with statically allocated buffers, but dynamic buffer sizes can be used if desired.
- **Non-intrusive scope:** FlexISOTP does not handle CAN IDs, CAN interfaces, or data processing. It simply facilitates single/multi-frame transfers and lets you handle communication.
- **Concurrent Sessions:** The struct-based design allows for an unlimited number of sessions.
- **Error Detection:** Missed continuous frame and timeout callbacks
- **Customizable Padding:** Each instance can have outgoing frames padded with a custom byte or sent at actual length depending on recievers requirements.

# 🎯 Goals
- **Reliability:** Statically allocated memory and clear code design patterns
- **FlexDiag:** Easily tie into the powerful UDS-based framework of FlexDiag
- **Platform-agnostic:** Codebase compatible with embedded/Arduino/Desktop and independent of CAN interfaces or libraries

## 📖 Use Cases
- Automotive Diagnostics