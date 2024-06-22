#include "main.h"
#include "path_manager.h"

#include <Windows.h>
#include <Psapi.h>

extern void BroadcastBetterAPIMessage(const struct better_api_t *API) {
        DWORD needed = 0;
        EnumProcessModules(GetCurrentProcess(), NULL, 0, &needed);

        DWORD size = (needed | 127) + 1;
        HMODULE* handles = (HMODULE*)malloc(size);
        ASSERT(handles != NULL);

        EnumProcessModules(GetCurrentProcess(), handles, size, &needed);

        auto count = needed / sizeof(*handles);

        for (unsigned i = 0; i < count; ++i) {
                if (!handles[i]) {
                        continue;
                }
                
                char path[MAX_PATH];
                GetModuleFileNameA(handles[i], path, MAX_PATH);

                //skip all modules that cant be mods (all dll mods would be in the game folder or a subfolder)
                const char* short_path = PathWithoutCurrentDir(path);

                if (!short_path) {
                        continue;
                }

                DEBUG("Checking module: '%s'", short_path); 

                const auto BetterAPICallback = (void(*)(const struct better_api_t*)) GetProcAddress(handles[i], "BetterConsoleReceiver");
                
                if (BetterAPICallback) {
                        DEBUG("BetterConsoleReceiver found in mod: %s", short_path);
                        BetterAPICallback(API);
                }
        }

        free(handles);
}