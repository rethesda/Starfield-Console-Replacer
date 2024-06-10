#include <Windows.h>

#include <algorithm>
#include <vector>
#include <string>

#include "main.h"
#include "simpledraw.h"
#include "callback.h"


#define SETTINGS_REGISTRY_PATH "BetterConsoleConfig.txt"


struct Setting {
        uint64_t key_hash;
        uint32_t key_offset;
        uint32_t value_offset;
};
constexpr static bool operator < (const Setting& A, const Setting& B) { return A.key_hash < B.key_hash; }
constexpr static bool operator < (const Setting& A, const uint64_t B) { return A.key_hash < B; }
constexpr static bool operator < (const uint64_t A, const Setting& B) { return A < B.key_hash; }

struct ConfigFile {
        unsigned char* file_buffer;
        uint64_t buffer_size;
        std::vector<Setting> lines;
        HANDLE out_file;
        const char* mod_name;
        char file_path[MAX_PATH];
};

static ConfigFile* BetterConsoleConfig = nullptr;
static const auto SimpleDraw = GetSimpleDrawAPI();


static uint64_t hash_fnv1a(const char* str) {
        uint64_t ret = 0xcbf29ce484222325;

        unsigned char c;
        while (c = *str++) {
                ret ^= c;
                ret *= 0x00000100000001B3;
        }

        return ret;
}

extern ConfigFile* ConfigLoadFile(const char* filename) {
        DEBUG("Reading config file: '%s'", filename);

        // Allocate a settingsfile object
        ConfigFile* const ret = (decltype(ret))calloc(1, sizeof(*ret));
        ASSERT(ret != NULL && "Malloc actually failed");

        // Initialize c++ objects in the struct
        ::new (&ret->lines) decltype(ret->lines);

        // Open settings file
        const auto hfile = CreateFileA(GetPathInDllDir(ret->file_path, filename), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hfile == INVALID_HANDLE_VALUE) return ret; //file does not exist probably

        // Get size of file, make sure its < 4GB
        LARGE_INTEGER li_file_size;
        const auto file_size_ret = GetFileSizeEx(hfile, &li_file_size);
        ASSERT(file_size_ret == TRUE && "Could not get size of file!");
        ASSERT(li_file_size.QuadPart < UINT32_MAX && "File too large >4GB");
        const auto file_size = (uint32_t)li_file_size.QuadPart;

        // Round allocation up to page size
        ret->buffer_size = (((file_size + 2ULL) + 4095) & ~uint64_t{ 4095 });
        ASSERT(file_size < ret->buffer_size && "Math failed me");

        // Allocate memory for file
        ret->file_buffer = (unsigned char*)malloc(ret->buffer_size);
        ASSERT(ret->file_buffer != NULL && "The impossible happened");

        // Read entire file to ram
        const auto read_file_ret = ReadFile(hfile, ret->file_buffer, file_size, NULL, NULL);
        ASSERT(read_file_ret == TRUE && "Could not read file!");

        // Null out the area at the end of the allocation
        memset(ret->file_buffer + file_size, 0, ret->buffer_size - file_size);

        // Ensure the file ends with a newline character for the parser
        ret->file_buffer[file_size] = '\n';
        CloseHandle(hfile);

        // a settings file consists of "key ! value" pairs separated by newlines, using '!' makes the parser more efficient
        // keys are composed of 2 parts: "mod_name : key_name"
        // this allows different plugins to have the same key_name without collisions
        // avoid non-printing characters in keys, while the parser doesn't care, the file is intended to be plain utf8
        // newline characters in keys is restricted (triggers assert)
        // whitespace characters at the beginning and end of keys is restricted (triggers assert)
        // whitespace is trimmed from the left of mod_name and the right of key_name (the join with ':' is assumed to be correct)
        // text in values is quoted and escaped, so newline restrictions and whitespace trimming does not effect values
        // duplicate keys in the settings file have no guarantees on which one is retrieved during lookup
        // settings are only saved if they are read, so old settings self-clean from the registry
        // therefore, your plugin should load all settings it uses instead of trying to lazy load settings
        // settings files have a size limit of 4 gigabytes, I truely hope this limit is never reached
        // the settings file is utf8 encoded, should be plain text, and is human readable / editable
        // memory consumption for the settings file is the size of the file + (16 bytes * number_of_newlines) + a constant overhead
        // the settings file buffer is never freed for the life of the application, its always valid to try to read a setting

        enum token_type : unsigned char {
                TT_NONE,
                TT_NULL,
                TT_NEWLINE,
                TT_EXCLAIM,
        };

        unsigned char lookup['!' + 8 & ~7]; //lol
        memset(lookup, TT_NONE, sizeof(lookup));
        lookup[0] = TT_NULL;
        lookup['\n'] = TT_NEWLINE;
        lookup['!'] = TT_EXCLAIM;

        unsigned char* f = ret->file_buffer;
        unsigned char* expos = nullptr;
        unsigned char* lastlf = f - 1;

        for (;; ++f) {
                const unsigned char c = *f;

                //the most common case (>80% of standard utf8 text)
                if (c > '!') continue;

                const auto t = lookup[c];

                // next most common case
                if (!t) continue;

                // there could be more '!' characters than newlines
                if (t == TT_EXCLAIM) {
                        expos = f;
                        continue;
                }

                if (t == TT_NEWLINE) {
                        // the parser found a newline character
                        // we know where the last newline was `lastlf`
                        // we also know if an exclaimation was found `expos`
                        // we can determine where the keys and values are using only this info
                        unsigned char* keypos = lastlf + 1;
                        lastlf = f;

                        if (!expos) continue; // no '!'? then there can't be a key!value pair 

                        unsigned char* valpos = expos;
                        expos = nullptr;

                        //step 1: advance keypos until the first non-whitespace character (left-trim)
                        while (::isspace(*keypos)) keypos;

                        if (keypos == valpos) continue; //key was empty

                        //step 2: key is not empty, so trim space on the right
                        unsigned char* null_maker = valpos;
                        *null_maker = 0; // null out the '!' character
                        while (::isspace(*null_maker)) *null_maker-- = 0;

                        //keypos is null terminated, not empty, and trimmed

                        //Step 4: right trim the value
                        null_maker = f;
                        while (::isspace(*null_maker)) *null_maker-- = 0;

                        if (null_maker == valpos) continue; //value was empty

                        //step 5: left trim the value
                        ++valpos; //advance past the (now null) '!' character
                        while (::isspace(*valpos)) ++valpos;

                        //valpos is null terminated, not empty, and trimmed

                        // finally, insert the key and value into the array
                        Setting s{};
                        const auto buffer_start = ret->file_buffer;
                        s.key_hash = hash_fnv1a((const char*)keypos);
                        s.key_offset = (uint32_t)(keypos - buffer_start);
                        s.value_offset = (uint32_t)(valpos - buffer_start);
                        DEBUG("KEY{%s}, VALUE{%s}, HASH{%p}", keypos, valpos, (void*)s.key_hash);
                        ret->lines.push_back(s);
                        continue;
                }
                // the only other option is the terminating condition: TT_NULL
                // so unconditionally break out of the parsing loop
                break;
        }
        // the while file is sorted and adding keys at runtime is not supported
        // use std::equal_range to perform lookups instead of hash table
        std::sort(ret->lines.begin(), ret->lines.end());
        return ret;
}

// perform the preparation necessary to write the config file to disk
extern void ConfigOpen(ConfigFile* file) {
        if ((file->out_file == NULL) || (file->out_file == INVALID_HANDLE_VALUE)) {
                file->out_file = CreateFileA(file->file_path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                ASSERT(file->out_file != INVALID_HANDLE_VALUE);
        }
        else {
                ASSERT("config file is already open");
        }

        file->mod_name = nullptr;
}

// set the mod_name to namespace the subsequent settings to
extern void ConfigSetMod(const char* mod_name) {
        ASSERT(BetterConsoleConfig != NULL);
        ASSERT(mod_name != NULL && "mod_name cannot be NULL");
        ASSERT(!::isspace((unsigned char)*mod_name) && "mod_name cannot start with whitespace");
        BetterConsoleConfig->mod_name = mod_name;
}

// perform the necessary actions to finish writing the config file
extern void ConfigClose(ConfigFile* file) {
        CloseHandle(file->out_file);
        file->out_file = INVALID_HANDLE_VALUE;
}


const char* ConfigLookupKey(const char* key_name) {
        ASSERT(BetterConsoleConfig->mod_name);
        char key_buffer[128];
        snprintf(key_buffer, sizeof(key_buffer), "%s:%s", BetterConsoleConfig->mod_name, key_name);
        const auto hash = hash_fnv1a(key_buffer);

        auto range = std::equal_range(BetterConsoleConfig->lines.begin(), BetterConsoleConfig->lines.end(), hash);

        for (auto& i = range.first; i != range.second; i++) {
                if (strncmp((const char*)BetterConsoleConfig->file_buffer + i->key_offset, key_buffer, sizeof(key_buffer)) == 0) {
                        return (const char*)BetterConsoleConfig->file_buffer + i->value_offset;
                }
        }

        return nullptr;
}


extern void ConfigWriteString(ConfigFile* file, const char* str) {
        //TODO more checks
        WriteFile(file->out_file, str, (DWORD)strlen(str), NULL, NULL);
}


extern void ConfigU32(ConfigAction action, const char* key_name, uint32_t* value) {
        if (action == ConfigAction_Read) {
                const auto val = ConfigLookupKey(key_name);
                if (val) *value = strtoul(val, nullptr, 0);
        }
        else if (action == ConfigAction_Write) {
                char write_buffer[128];
                snprintf(write_buffer, sizeof(write_buffer), "%s:%s!%u\n", BetterConsoleConfig->mod_name, key_name, *value);
                ConfigWriteString(BetterConsoleConfig, write_buffer);
        }
        else if (action == ConfigAction_Edit) {
                ImGui::DragScalar(key_name, ImGuiDataType_U32, value);
        }
}


static constexpr const struct config_api_t Config = {
        //ConfigLookupKey,
        ConfigU32
};


extern const struct config_api_t* GetConfigAPI() {
        return &Config;
}



extern void LoadSettingsRegistry() {
        ASSERT(BetterConsoleConfig == nullptr && "BetterConsoleConfig is already loaded");
        BetterConsoleConfig = ConfigLoadFile(SETTINGS_REGISTRY_PATH);
        
        uint32_t num_config;
        const auto config = CallbackGetHandles(CALLBACKTYPE_CONFIG, &num_config);
        for (unsigned i = 0; i < num_config; i++) {
                BetterConsoleConfig->mod_name = CallbackGetName(config[i]);
                const auto callback = CallbackGetCallback(CALLBACKTYPE_CONFIG, config[i]);
                callback.config_callback(ConfigAction_Read);
        }
}

extern void SaveSettingsRegistry() {
        ASSERT(BetterConsoleConfig != nullptr && "BetterConsoleConfig was not loaded");
        ConfigOpen(BetterConsoleConfig);

        uint32_t num_config;
        const auto config = CallbackGetHandles(CALLBACKTYPE_CONFIG, &num_config);
        for (unsigned i = 0; i < num_config; i++) {
                BetterConsoleConfig->mod_name = CallbackGetName(config[i]);
                const auto callback = CallbackGetCallback(CALLBACKTYPE_CONFIG, config[i]);
                callback.config_callback(ConfigAction_Write);
        }
        ConfigClose(BetterConsoleConfig);
}


extern void draw_settings_tab() {
        uint32_t num_config;
        const auto config = CallbackGetHandles(CALLBACKTYPE_CONFIG, &num_config);

        const auto to_string = [](const void* handles, uint32_t index, char*, uint32_t) noexcept -> const char* {
                const auto u = (const RegistrationHandle*)handles;
                return CallbackGetName(u[index]);
        };

        if (!num_config) {
                SimpleDraw->Text("No configuration callbacks registered");
                return;
        }

        static uint32_t selection = 0;
        SimpleDraw->HBoxLeft(0.3, 12.f);
        SimpleDraw->SelectionList(&selection, config, num_config, to_string);
        SimpleDraw->HBoxRight();
        if (selection >= 0) {
                const auto callback = CallbackGetCallback(CALLBACKTYPE_CONFIG, config[selection]);
                callback.config_callback(ConfigAction_Edit);
        }
        SimpleDraw->HBoxEnd();
}
