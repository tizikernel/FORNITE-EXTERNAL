#include "../protection/security_macros.h"

#ifndef FATAL_GLOBAL_JUNK_INCLUDED
#define FATAL_GLOBAL_JUNK_INCLUDED

// All security logic and structures (FUNC, funcList, anti_attach, integrity checks)
// are now centrally managed in protection/junk_engine.h and included via security_macros.h.

#define JUNK CALL_RANDOM_JUNK
#define JUNK_LARGE CALL_RANDOM_JUNK
#define JUNK_SMALL CALL_RANDOM_JUNK

#define WATERMARK_UNIQUE_NAME2(prefix, line) prefix##line
#define WATERMARK_UNIQUE_NAME1(prefix, line) WATERMARK_UNIQUE_NAME2(prefix, line)
#define WATERMARK_UNIQUE_NAME(prefix) WATERMARK_UNIQUE_NAME1(prefix, __LINE__)

#define WATERMARK(watermark) \
    __declspec(allocate(Name_Of_Sections_watermark)) \
    static const char WATERMARK_UNIQUE_NAME(__watermark)[] = watermark; \
    static const char* const volatile WATERMARK_UNIQUE_NAME(__ptr1) = WATERMARK_UNIQUE_NAME(__watermark); \
    (void)WATERMARK_UNIQUE_NAME(__ptr1)

#define ULTRA_MEGA_JUNK(x) AbTdhF(x)
#define Junkyyyyyyy(x) JkDpomZd(x)

#define AbTdhF(x) do { \
    volatile int _crazy_var1 = 0xDEADC0DE; \
    chaos_seed = (chaos_seed * 0x8088405 + 1) & 0xFFFFFFFF; \
} while(0)

#define JkDpomZd(x) do { \
    volatile int _crazy_var = 0x1337; \
} while(0)

#define CALL_RANDOM_JUNK AbTdhF(0)
#define COOL_JUNK JkDpomZd(0)

// -------------------------------------------------------------------------
// Call Dispatcher Macros
// -------------------------------------------------------------------------

#define CALL(expr) ([]() { return expr; }())
#define CALL_VOID(expr) ([&]() { expr; }())

static inline void anti_instrumentation() {
    while (true) {
        Sleep(1000);
    }
}

#define START_ANTI_INSTRUMENTATION std::thread([]() { anti_instrumentation(); }).detach()
#define SPOOF_FUNC 

#endif // FATAL_GLOBAL_JUNK_INCLUDED
