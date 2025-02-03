#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "isotp_session.h"

#define ISOTPLIB_VERSION_MAJOR         1
#define ISOTPLIB_VERSION_MINOR         0
#define ISOTPLIB_VERSION_REVISION      0
#define ISOTPLIB_VERSION_CHECK(maj, min) ((maj==ISOTP_MAJOR_VERSION) && (min<=ISOTP_MINOR_VERSION))

#ifdef __cplusplus
}
#endif