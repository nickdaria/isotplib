#pragma once

#include "ISOTPSession.h"

#define ISOTP_VERSION_MAJOR         0
#define ISOTP_VERSION_MINOR         1
#define ISOTP_VERSION_REVISION      1
#define ISOTP_VERSION_CHECK(maj, min) ((maj==ISOTP_MAJOR_VERSION) && (min<=ISOTP_MINOR_VERSION))