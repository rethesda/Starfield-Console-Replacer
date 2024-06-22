#include <Windows.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

 extern "C" EXCEPTION_DISPOSITION __declspec(dllexport) __CxxFrameHandler4(void* A, void* B, void* C, void* D) {
         auto hmodule = LoadLibraryExW(L"vcruntime140_1.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
         decltype(&__CxxFrameHandler4) p = nullptr;
         if (!hmodule) goto DIE;
         p = (decltype(&__CxxFrameHandler4))GetProcAddress(hmodule, __func__);
         if (!p) goto DIE;
         return p(A, B, C, D);
 DIE:
         MessageBoxA(NULL, __func__, "Betterconsole Crashed!", 0);
         std::abort();
}
