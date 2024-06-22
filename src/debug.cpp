#include "main.h"
#include "path_manager.h"

#include <Windows.h>


static uint64_t frame_counter = 0;

// increment the frame counter
extern void DebugTickFrame() {
        ++frame_counter;
}

// return true every nth frame as determined by `modulus`
extern bool DebugEveryNFrames(unsigned modulus) {
        return (frame_counter % modulus == 0);
}


#ifdef MODMENU_DEBUG
struct FilenameOnly {
        const char* name;
        uint32_t name_len;
};
static FilenameOnly filename_only(const char* path) {
        FilenameOnly ret{ path, 0 };

        for (uint32_t i = 0; path[i]; ++i) {
                if (path[i] == '/' || path[i] == '\\') {
                        ret.name = &path[i + 1];
                }
        }

        for (uint32_t i = 0; ret.name[i]; ++i) {
                ret.name_len = i;
                if (ret.name[i] == '.') break;
        }

        return ret;
}



// this assert does not log to file or allocate memory
//  its the emergency use only assert for when all else fails
[[noreturn]] static void msg_assert(const char* message) {
        MessageBoxA(NULL, message, "BetterConsole SUPER_ASSERT", 0);
        abort();
}
#define SUPER_ASSERT(CONDITION) do{if(!(CONDITION))msg_assert(#CONDITION);}while(0)


#include <mutex>
std::mutex logging_mutex;
static thread_local char format_buffer[4096];
static constexpr auto buffer_size = sizeof(format_buffer);
static void write_log(const char* const str) noexcept {
        static HANDLE debugfile = INVALID_HANDLE_VALUE;
        static thread_local bool is_locked = false;

        // drop logs on recursive lock
        if (is_locked) return;

        logging_mutex.lock();
        is_locked = true;

        if (debugfile == INVALID_HANDLE_VALUE) {
                char path[MAX_PATH];
                PathInDllDir(path, sizeof(path), "BetterConsoleLog.txt");
                debugfile = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                SUPER_ASSERT(debugfile != INVALID_HANDLE_VALUE);
                TRACE("Writing log to %s", path);
        }
        if (debugfile != INVALID_HANDLE_VALUE) {
                WriteFile(debugfile, str, (DWORD)strnlen(str, 4096), NULL, NULL);
                FlushFileBuffers(debugfile);
        }
        else {
                MessageBoxA(NULL, "Could not write to 'BetterConsoleLog.txt'", "ASSERTION FAILURE", 0);
                abort();
        }

        logging_mutex.unlock();
        is_locked = false;
}
extern void DebugImpl(const char* const filename, const char* const func, int line, const char* const fmt, ...) noexcept {
        const auto fn = filename_only(filename);
        auto bytes = snprintf(format_buffer, buffer_size, "%8X %4u %16.*s %32s %4d>", GetCurrentThreadId(), frame_counter, fn.name_len, fn.name, func, line);
        ASSERT(bytes > 0);
        ASSERT(bytes < buffer_size);

        va_list args;
        va_start(args, fmt);
        bytes += vsnprintf(&format_buffer[bytes], buffer_size - bytes, fmt, args);
        va_end(args);
        ASSERT(bytes > 0);
        ASSERT(bytes < buffer_size);

        format_buffer[bytes++] = '\n';
        format_buffer[bytes] = '\0';
        ASSERT(bytes < buffer_size);

        write_log(format_buffer);
}

[[noreturn]] extern void AssertImpl(const char* const filename, const char* const func, int line, const char* const text) noexcept {
        const auto fn = filename_only(filename);
        snprintf(
                format_buffer,
                buffer_size,
                "In file     '%.*s'\n"
                "In function '%s'\n"
                "On line     '%d'\n"
                "Message:    '%s'",
                fn.name_len,
                fn.name,
                func,
                line,
                text
        );
        write_log("!!! ASSERTION FAILURE !!!\n");
        write_log(format_buffer);
        MessageBoxA(NULL, format_buffer, "BetterConsole Crashed!", 0);
        abort();
}

extern void TraceImpl(const char* const filename, const char* const func, int line, const char* const fmt, ...) noexcept {
        static bool init = false;
        static char tracebuff[2048];
        const auto fn = filename_only(filename);
        const auto bytes = snprintf(tracebuff, sizeof(tracebuff), "[%08u]%8.*s:%s:%d> ", frame_counter, fn.name_len, fn.name, func, line);
        ASSERT(bytes > 0 && "trace buffer too small ?");
        va_list args;
        va_start(args, fmt);
        const auto bytes2 = vsnprintf(tracebuff + bytes, sizeof(tracebuff) - bytes, fmt, args);
        ASSERT(bytes2 > 0 && "trace buffer too small ?");
        tracebuff[bytes + bytes2] = '\r';
        tracebuff[bytes + bytes2 + 1] = '\n';
        tracebuff[bytes + bytes2 + 2] = '\0';
        va_end(args);
        if (!init) {
                AllocConsole();
                FILE* file = nullptr;
                freopen_s(&file, "CONIN$", "rb", stdin);
                freopen_s(&file, "CONOUT$", "wb", stdout);
                freopen_s(&file, "CONOUT$", "wb", stderr);
        }
        fputs(tracebuff, stdout);
}
#endif