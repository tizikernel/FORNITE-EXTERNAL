#include "../security_macros.h"

#ifndef FATAL_GLOBAL_JUNK_INCLUDED
#define FATAL_GLOBAL_JUNK_INCLUDED

// Security logic is centralized in junk_engine.h (included via security_macros.h)

#define JUNK CALL_RANDOM_JUNK
#define CALL_RANDOM_JUNK do { volatile int x = 0x1337; } while(0)
#define SPOOF_FUNC

#endif
