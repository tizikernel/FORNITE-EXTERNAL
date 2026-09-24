#include "../security_macros.h"

#ifndef FATAL_GLOBAL_JUNK_INCLUDED
#define FATAL_GLOBAL_JUNK_INCLUDED

// Centrally managed in junk_engine.h

#define JUNK do { volatile int x = 0; } while(0)
#define SPOOF_FUNC

#endif
