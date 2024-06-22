#include "main.h"


#ifndef MODMENU_DEBUG
#ifdef _DEBUG
#define MODMENU_DEBUG
#endif // _DEBUG
#endif // !MODMENU_DEBUG

#ifdef MODMENU_DEBUG
extern void DebugImpl(const char* const filename, const char* const func, int line, const char* const fmt, ...) noexcept;
[[noreturn]] extern void AssertImpl(const char* const filename, const char* const func, int line, const char* const text) noexcept;
extern void TraceImpl(const char* const filename, const char* const func, int line, const char* const fmt, ...) noexcept;
#define DEBUG(...) do { DebugImpl(__FILE__, __func__, __LINE__, " " __VA_ARGS__); } while(0)
#define ASSERT(CONDITION) do { if (!(CONDITION)) { AssertImpl(__FILE__, __func__, __LINE__, " " #CONDITION); } } while(0)
#define TRACE(...) do { TraceImpl(__FILE__, __func__, __LINE__, " " __VA_ARGS__); } while(0)
#define IMGUI_DEBUG_PARANOID
#else
#define DEBUG(...) do { } while(0)
#define ASSERT(...) do { } while(0)
#defince TRACE(...) do {} while (0)
#endif // MODMENU_DEBUG


// increment the frame counter
extern void DebugTickFrame();

// return true every nth frame as determined by `modulus`
extern bool DebugEveryNFrames(unsigned modulus);