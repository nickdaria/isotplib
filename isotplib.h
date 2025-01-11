#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "ISOTPSession.h"

#define ISOTPLIB_VERSION_MAJOR         0
#define ISOTPLIB_VERSION_MINOR         1
#define ISOTPLIB_VERSION_REVISION      1
#define ISOTPLIB_VERSION_CHECK(maj, min) ((maj==ISOTP_MAJOR_VERSION) && (min<=ISOTP_MINOR_VERSION))

#ifdef __cplusplus
}
#endif