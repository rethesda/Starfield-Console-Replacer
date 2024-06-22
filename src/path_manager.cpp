#include "main.h"

#include <Windows.h>

// Are wr initialized?
static bool pm_initialized = false;

// The path of BetterConsole.dll
static char* dll_path = nullptr;
static size_t dll_path_size = 0;


// The path of the current folder (should be the same as the exe folder)
static char* cur_dir = nullptr;
static size_t cur_dir_size = 0;


// --CHAT GPT--
static char* WideStringToUTF8(const wchar_t* wideStr) {
        // Check if input wide string is NULL
        if (wideStr == NULL) {
                return NULL;
        }

        // Calculate the length of the UTF-8 string
        int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, NULL, 0, NULL, NULL);
        if (utf8Len == 0) {
                return NULL; // Conversion failed
        }

        // Allocate memory for the UTF-8 string
        char* utf8Str = (char*)malloc(utf8Len);
        if (utf8Str == NULL) {
                return NULL; // Memory allocation failed
        }

        // Perform the conversion
        int result = WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, utf8Str, utf8Len, NULL, NULL);
        if (result == 0) {
                free(utf8Str); // Free allocated memory on failure
                return NULL; // Conversion failed
        }

        return utf8Str; // Success
}



extern void PathInitDllMain(void* hinstance_self) {
        // the maximum path length on windowx 10 1607 or later is 32767 characters
        // the MAX_PATH define is 260 for legacy reasons
        wchar_t *wide_path = (wchar_t*)calloc(32768, sizeof(*wide_path));
        ASSERT(wide_path != nullptr);

        GetModuleFileNameW((HINSTANCE)hinstance_self, wide_path, 32768);
        dll_path = WideStringToUTF8(wide_path);
        ASSERT(dll_path != nullptr);
        dll_path_size = strlen(dll_path);
        
        // remove the dll name
        char* dp = &dll_path[dll_path_size - 1];
        while(dll_path_size &&  (*dp != '\\') && (*dp != '/')) {
                --dll_path_size;
                *dp-- = '\0';
        }

        GetCurrentDirectoryW(32768, wide_path);
        // add a trailing backslash
        wcscat_s(wide_path, 32768, L"\\");

        cur_dir = WideStringToUTF8(wide_path);
        ASSERT(cur_dir != nullptr);
        cur_dir_size = strlen(cur_dir);

        free(wide_path);
        pm_initialized = true;
}


extern const char* PathInCurrentDir(char* path_buffer, size_t path_buffer_size, const char* filename) {
        ASSERT(filename != nullptr);
        ASSERT(pm_initialized == true);

        const auto filename_size = strlen(filename);
        const auto path_size = cur_dir_size + filename_size + 1;
        ASSERT(path_buffer_size >= path_size);
        
        memcpy(path_buffer, cur_dir, cur_dir_size);
        memcpy(&path_buffer[cur_dir_size], filename, filename_size + 1);
        DEBUG("Path: '%s'", path_buffer);

        return path_buffer;
}


extern const char* PathInDllDir(char* path_buffer, size_t path_buffer_size, const char* filename) {
        ASSERT(filename != nullptr);
        ASSERT(pm_initialized == true);

        const auto filename_size = strlen(filename);
        const auto path_size = dll_path_size + filename_size + 1;
        ASSERT(path_buffer_size >= path_size);
        
        memcpy(path_buffer, dll_path, dll_path_size);
        memcpy(&path_buffer[dll_path_size], filename, filename_size + 1);
        DEBUG("Path: '%s'", path_buffer);

        return path_buffer;
}


extern const char* PathWithoutCurrentDir(const char* path) {
        ASSERT(path != nullptr);
        ASSERT(pm_initialized == true);

        if (strncmp(path, cur_dir, cur_dir_size) == 0) {
                return path + cur_dir_size;
        }
        return nullptr;
}