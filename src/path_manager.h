#pragma once

// In DllMain call PathInitDllMain with the HINSTANCE parameter
extern void PathInitDllMain(void* hinstance_self);

// Call PathInCurrentDir with a filename to get the full path in the current directory
// path_buffer should be MAX_PATH in size or more
extern const char* PathInCurrentDir(char* path_buffer, size_t path_buffer_size, const char* filename);

// Call PathInDllDir with a filename to get the full path in the betterconsole dll directory
// path_buffer should be MAX_PATH in size or more
extern const char* PathInDllDir(char* path_buffer, size_t path_buffer_size, const char* filename);

// Call PathWithoutCurrentDir with a path to get the path without the current directory
// returns NULL if the path is not in the current directory or a subdirectory
extern const char* PathWithoutCurrentDir(const char* path);