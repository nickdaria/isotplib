// isotplib.cpp

#ifdef __cplusplus

// Include the C headers wrapped in `extern "C"` to prevent C++ name mangling
extern "C" {
    #include "ISOTPSession.h"
    #include "ISOTPConversions.h"
    #include "ISOTPConstants.h"
    #include "isotplib.h"
}

#else
// If this file is included in a C environment, raise a compilation error
#error "isotplib.cpp is designed to work in C++ environments only."
#endif
