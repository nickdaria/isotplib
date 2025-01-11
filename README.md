# ISOTPlib
Highly configurable and flexible C library for implementing ISO 15765-2 (Transport Layer) for CAN, CAN-FD, and LIN networks.

> [!IMPORTANT] 
This library solely focuses on implementation of the ISO-TP protocol. As ISO-TP is often implemented alongside UDS, this library was designed to integrate seamlessly with [udslib](https://github.com/nickdaria/udslib).

# 🚀 Key Features
- CAN, CAN-FD, and LIN implementations of ISO-TP
- Easy-to-use callbacks and error handling
- Pure C, platform agnostic w/ C++/Arduino compatibility
- Static memory allocation
- Tight scope - no bloat

# ⚡️ Advanced Features
- Concurrent sessions
- Per-session protocol configuration - padding enable, padding byte, consecutive index ordering, etc
- Supports user implementation of dynamic RX memory allocation

# ✏️ Usage
- See `examples/` for functioning code (command line & microcontroller)
- See [Implementation](https://github.com/nickdaria/isotplib/wiki/Implementation) for generic implementation directions
