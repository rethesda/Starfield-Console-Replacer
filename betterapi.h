#ifndef BETTERAPI_API_H
#define BETTERAPI_API_H


// Integrate betterconsole in your mod in 7 easy steps:
// 
// Step 1) #include "betterapi.h" anywhere in your project
// 
// 
// Step 2) In only a single translation unit (.cpp file) define BETTERAPI_IMPLEMENTATION
//         before including betterapi.h to prevent linker errors - like this:
// 
//         #define BETTERAPI_IMPLEMENTATION // only do this in one file
//         #include "betterapi.h"           // now everything will work
// 
// 
// Step 3) Implement the following function (and save the betterapi pointer for use later):
// 
//         DLLEXPORT void BetterConsoleReceiver(const struct better_api_t* betterapi);
// 
// 
// Step 4) Before using the betterconsole api, call this function with the
//         `betterapi` pointer received from your BetterConsoleReceiver:
// 
//         BETTERAPI_INIT(betterapi); //this should be done first in BetterConsoleReceiver
// 
// 
// Step 5) Register the name of your plugin with betterconsole using the callback api.
//         This will also check version compatibility between the installed version
//         of betterconsole and your mod:
// 
//         my_mod_handle = betterapi->Callback->RegisterMod("My Mod Name", BETTERAPI_VERSION);
// 
// 
// Step 6) Register additional callbacks for any functionality you want:
// 
//         betterapi->Callback->RegisterDrawCallback(my_mod_handle, &my_draw_callback);
//
// 
// Step 7) Enjoy! betterapi integration is a soft dependency so if a user doesn't have
//         or want betterconsole installed, they can still use your mod just without the 
//         betterconsole features and api. If your mod can function without betterconsole
//         you dont need to do any additional work, the BetterConsoleReceiver just wont
//         be called.
//
//
// Bonus Step: If you are writing an asi mod or trying to port a cheat engine table,
//             you may want to Control+F for SFSE_MINIMAL. You can utilize the sfse
//             minimal api to make your mod into a combo asi and sfse plugin without
//             having to include any other headers or libraries not even any sfse code.
//             I have written and included an example of how to use this near the
//             bottom of the file.


// Betterconsole is written in C++ but the public API is plain C99
// and only includes headers that are from the C standard library 
// (only for types and defines, no C library functions are called).
// This makes it easier to integrate betterconsole into any project
// because the whole API is one header file with no other dependencies.
// On a philisophical level, if you cannot describe an interface in C
// using only builtin types and defines, then the interface is not
// simple enough. Since almost every programming language has some type
// of C compatible foreign function interface, this allows you to write
// mods in almost any programming language albeit with some limitations.
#ifdef __cplusplus
#define DLLEXPORT extern "C" __declspec(dllexport)
#else
#define DLLEXPORT __declspec(dllexport)
#endif // __cplusplus
#include <stdint.h> 
#include <stdbool.h>


// Version number for betterapi, when a new version of betterconsole
// changes the public API this number is incremented. Usually only when
// bigger feature changes are added I will not break the API in a bugfix
// releas of betterconsole and feature releases are not expected to happen
// more often than starfield updates.
#define BETTERAPI_VERSION 1


// Foreward declaration of the main betterapi structure.
// you will receive this as the only argument in your BetterConsoleReceiver
// From how this has been used internally, you may want to assign the individual
// fields of this structure to const global variables to avoid writing 
// "api->structure->function" all over the place.
struct better_api_t;

// Foreward declaration of init function, call this first in your BetterConsoleReceiver
// function to initialize the betterapi system before using any of the api.
void BETTERAPI_INIT(const struct better_api_t* betterapi);


// Note: this API is not thread safe,
// all of the below functions are operating within idxgiswapchain::present
// as the game is trying submit a frame to the gpu
// do not use any of these apis from a separate thread
// and please keep it quick, don't slow down the game
// I will add an api for a separate task thread to do sig scanning or other slow tasks


// Opaque handle for the log buffer system
typedef uint32_t LogBufferHandle;


// When you register a mod you receive a handle to your mod
// this handle can be used to register additional functionality
// like hotkeys or draw callbacks
typedef uint32_t RegistrationHandle;


// The settings callback tells you what type of action to perform from these options:
// ConfigAction_Read - betterconsole finished reading the settings file
//                     and now you can try loading any settings you saved
// ConfigAction_Write - betterconsole is about to save all settings to
//                      the settings file
// ConfigAction_Edit - the user is in the settings menu and wants to adjust a setting
//                     show a UI to the user to edit the setting
typedef enum ConfigAction {
        ConfigAction_Read,
        ConfigAction_Write,
        ConfigAction_Edit,
} ConfigAction;


// while many compilers let you cast a function pointer to a void*
// the C standard only lets you cast a function pointer to another
// function pointer. Whatever, but this does help document the API better
typedef void (*FUNC_PTR)(void);


// for registering your mod with betterconsole, this callback will be called every
// frame when your mod is active in the UI and the ui is showing
// `imgui_context` can be ignored if using the SimpleDraw API, but if you link with
//  imgui directly this can be used to create more complex user interfaces but you
//  must call ImGui::SetCurrentContext() first in your draw callback
typedef void (*DRAW_CALLBACK)(void* imgui_context);


// if your application would like to take advantage of the built-in settings 
// registry you will need to provide a callback function that is called when
// the settings file is loaded, saved, or when the user is in the settings menu
// and selects your mod. The Settings API provides automatic read/write/edit 
// functionality if the built-in types work for you.
// `read_write_or_edit` is one of: ConfigAction_Read, ConfigAction_Write, or
//                      ConfigAction_Edit
typedef void (*CONFIG_CALLBACK)(ConfigAction read_write_or_edit);


// This is a function called when the user presses a hotkey combination that
// matches the hotkey set for your plugin. Setting a hotkey is done by the user
// in the hotkeys menu. Hotkeys are saved automatically in the settings registry
// so a set hotkey is preserved across sessions. Only one hotkey callback per mod
// can be registered, but you can request hotkeys multiple times with different
// user data.
// 
// `userdata` is any arbitrary data that you want to pass to the callback usually
//            to help identify the hotkey if multiple hotkeys are requested
typedef void (*HOTKEY_CALLBACK)(uintptr_t userdata);


// For the simpledraw selectionlist, this is a function called to provide text for
// a single entry in a selectable list of items. This function will be called every
// frame for every *visible* item in the list. The return value should be a null
// terminated string that will be shown in the UI.
// 
// `userdata` is any arbitrary data that you want to pass to the callback
// 
// `index` is the index of the item in the list to "stringify"
// 
// `out_buffer` is provided as a temporary buffer that can be used
//              to store the result of snprintf() or similar since
//              freeing a heap allocation made within the callback
//              is a very difficult task.
typedef const char* (*CALLBACK_SELECTIONLIST)(const void* userdata, uint32_t index, char* out_buffer, uint32_t out_buffer_size);


// For the simpledraw table renderer, used to build the UI shown in tables.
// This callback will be called for each *visible* cell in the table (unseen cells
// will not be rendered and thus will not call this function). Tables are drawn
// first left to right for each column, then from top to bottom for each row.
// 
// `table_userdata` is any arbitrary data that you want to pass to the callback and
//                  should be used for the data you wish to draw in the table
// 
// `current_row` is the index of the current row in the table that is being drawn
// 
// `current_column` is the index of the current column in the table that is being drawn
// TODO: next version update this to use unsigned integers and possibly use uintptr_t 
//       instead of void* for table_userdata
typedef void (*CALLBACK_TABLE)(void* table_userdata, int current_row, int current_column);


// This is the main api you will use to add betterconsole integration to your mod
struct callback_api_t {
        // Register your mod with betterconsole.
        // Almost all mods will need this to interact with betterconsole. The
        // returned handle can be used to register additional functionality such
        // as hotkeys or draw callbacks. This function also checks the version
        // of the BetterConsole API your mod uses for compatibility.
        // 
        // `mod_name` should uniquely identify your mod and conform to the following:
        //                 - must be less than 32 characters
        //                 - must have a length of >3 characters
        //                 - must not begin or end with whitespace
        // 
        // `betterapi_version` is the version of the BetterConsole API your mod uses
        //                     you should pass BETTERAPI_VERSION unmodified to this function
        RegistrationHandle(*RegisterMod)(const char* mod_name, uint32_t betterapi_version);
        

        // Register a function to show your mod's user interface.
        // Usually, you would utilize the SimpleDraw API to create your UI,
        // but more advanced users may link with imgui directly and utilize
        // the imgui context provided as the first parameter to the draw callback.
        // If using imgui directly, you must call ImGui::SetCurrentContext() in
        // the draw callback before using any imgui functions AND you must
        // match the version of imgui with the version used by betterconsole.
        // 
        // `handle` is your registration handle from a previous call to RegisterMod
        // 
        // `draw_callback` will be called every frame when the following conditions are met:
        //                 - the BetterConsole UI is open
        //                 - your mod is the one in focus
        void (*RegisterDrawCallback)(RegistrationHandle handle, DRAW_CALLBACK draw_callback);


        // Register a configuration callback
        // 
        // `handle` is your registration handle from a previous call to RegisterMod
        // 
        // `config_callback` will be called when the settings file is loaded (read event)
        //                   saved (write event) or when the user is in the setting menu
        //                   and selects your mod in the UI (edit event)
        void (*RegisterConfigCallback)(RegistrationHandle handle, CONFIG_CALLBACK config_callback);


        // Register a hotkey callback
        // 
        // `handle` is your registration handle from a previous call to RegisterMod
        //
        //  `hotkey_callback` will be called when the user presses the hotkey combo
        //                    set for your plugin. to have an effect, you must also
        //                    call RequestHotkey. This is a two-step process because
        //                    your plugin can have only one hotkey callback but you
        //                    request any number of hotkeys with different userdata.
        void (*RegisterHotkeyCallback)(RegistrationHandle handle, HOTKEY_CALLBACK hotkey_callback);


        // Register a hotkey handler, the user configures the hotkey in the hotkeys menu
        // 
        // `handle` is the handle you created when registering your mod
        // 
        // `hotkey_name` is the name of the hotkey in the ui, use a descriptive name
        //               less than the 32 character internal buffer. this string is 
        //               copied to an internal buffer so feel free to use a generated
        //               name via snprintf() or similar on stack allocated memory.
        // 
        // `userdata` is optional extra data that is sent to the hotkey callback
        //            if you want to request multiple hotkeys this parameter is
        //            how you would differentiate them.
        void (*RequestHotkey)(RegistrationHandle handle, const char* hotkey_name, uintptr_t userdata);
};


/// This API is used for saving and loading data to a configuration file.
struct config_api_t {
        // this handles read / write / edit on 32-bit unsigned integers
        void (*ConfigU32)(ConfigAction action, const char* key_name, uint32_t* value);

        // get the unparsed string value of a key if it exists or null
        //const char* (*ConfigRead)(const char* key_name);
};



// This api deals with hooking functions and vtables
struct hook_api_t {
        // Hook old_func so it is redirected to new_func
        // returns a function pointer to call old_func after the hook
        FUNC_PTR (*HookFunction)(FUNC_PTR old_func, FUNC_PTR new_func);

        // Hook a vtable function for all instances of a class
        // returns the old function pointer
        FUNC_PTR(*HookVirtualTable)(void* class_instance, unsigned method_index, FUNC_PTR new_func);

        // Relocate an offset from imagebase
        // sfse offers a better version of this, but if you are using the minimal api (see below)
        // then you might want to use this
        void* (*Relocate)(unsigned imagebase_offset);

        // Write memory
        // same as Relocate, use the sfse version if you can
        bool(*WriteMemory)(void* dest, const void* src, unsigned size);

        // get the address of a function through starfield's import address table
        FUNC_PTR(*GetProcAddressFromIAT)(const char* dll_name, const char* func_name);

        // hook a function in the Import Address Table of starfield
        FUNC_PTR(*HookFunctionIAT)(const char* dll_name, const char* func_name, const FUNC_PTR new_function);

        // !EXPERIMENTAL API! AOB scan the exe memory and return the first match 
        void* (*AOBScanEXE)(const char* signature);
};

// This API allows you to create a basic mod menu without linking to imgui.
// if you play your cards right, you can write an entire mod without any other
// headers or libraries... not even the standard library!
struct simple_draw_t {
        // draw a separator line
        void (*Separator)(void);

        // draw some text, supports format specifiers
        void (*Text)(const char *fmt, ...);

        // draw a button, returns true if the button was clicked
        bool (*Button)(const char* name);

        // draw a checkbox, needs to store persistent state so pass a static boolean pointer
        // returns the value of *state for convenience in an 'if' statement
        bool (*Checkbox)(const char* name, bool* state);

        // draw a text input widget for a single line of text
        // buffer is where the text is stored
        // true_on_enter determines if the widget returns true on every character typed or only when enter is pressed
        bool (*InputText)(const char* name, char* buffer, uint32_t buffer_size, bool true_on_enter);


        // split the remainaing space into a left and right region
        // the left region occupies a left_size (eg: .5) fraction of the screen
        // the remaining space is reserved for the HBoxRight() side
        // min_size is a safety minimum number of charactes wide that hboxleft can be
        void (*HBoxLeft)(float left_size, float min_size_em);

        // the counterpart of HBoxLeft 
        // no argument needed, the size is the remaining space not taken up by hbox left
        void (*HBoxRight)();

        // needed to end hbox calculation
        void (*HBoxEnd)();

        //this is the same as hbox, but for vertically separated spaces
        void (*VboxTop)(float top_size, float min_size_em);
        void (*VBoxBottom)();
        void (*VBoxEnd)();


        // display an int number editor as a draggable slider that is also editable
        // min and max define the range of the *value
        // step defines the granilarity of the movement
        // the return value is true if the value was changed this frame
        bool (*DragInt)(const char* name, int* value, int min, int max);

        // follows the same rules as the sliderint above
        bool (*DragFloat)(const char* name, float* value, float min, float max);


        // use the remaining vertical space to render the logbuffer referenced by handle
        // if scroll_to_bottom is true, force the logbuffer region to scroll from its current position
        // to the bottom (useful when you add lines)
        void (*ShowLogBuffer)(LogBufferHandle handle, bool scroll_to_bottom);

        // use the remaining vertical space to render the logbuffer referenced by handle
        // if scroll_to_bottom is true, force the logbuffer region to scroll from its current position
        // takes a pointer to an array of line numbers and only displays those lines in the widget
        void (*ShowFilteredLogBuffer)(LogBufferHandle handle, const uint32_t* lines, uint32_t line_count, bool scroll_to_bottom);

        // Show a selectable list of text items, returns true when the selection changes.
        // `selection` is the index of the selected item and should be static
        // `userdata` is any arbitrary data that you want to pass to the callback
        // `count` is the number of items in the list
        // proper clipping and scrolling is performed to make even very long lists efficient
        bool(*SelectionList)(uint32_t *selection, const void* userdata, uint32_t count, CALLBACK_SELECTIONLIST to_string);

        // draw a table one cell at a time through the `draw_cell` callback
        // the number of columns is set to `header_count` because those numbers shoud be the same
        // proper vertical and horizontal clipping and scrolling is done to accelerate very large tables
        void (*Table)(const char * const * const header_labels, uint32_t header_count, void* table_userdata, uint32_t row_count, CALLBACK_TABLE draw_cell);

        // draw tabs that can be switched between
        void (*TabBar)(const char* const* const headers, uint32_t header_count, int* state);

        // draw buttons in a row, returns the index of the selected button
        int (*ButtonBar)(const char* const* const labels, uint32_t label_count);

        // show a tooltip marker that looks like "(?)"
        // when the user hovers over the marker `text` will be shown
        void (*Tip)(const char* text);

        //place multiple simpledraw elements on the same line
        void (*SameLine)();
};


// the logbuffer API is deprecated and will be changed in a future release
struct log_buffer_api_t {
	// Create a handle, needed to identify the log buffer
        // name is the name of the logbuffer as it shows up in the UI
        // path is an optional path to a logfile to append with logs
        LogBufferHandle (*Create)(const char* name, const char* path);

        // Get the number of lines stored in the buffer
        uint32_t (*GetLineCount)(LogBufferHandle);

        // Retrieve a specific line for storage
        const char* (*GetLine)(LogBufferHandle, uint32_t line);

        // Log a line of text to the buffer
        void (*Append)(LogBufferHandle, const char* log_line);

        // Restore the log buffer from a file, the file is then used for appending
        LogBufferHandle (*Restore)(const char* name, const char* filename);
};


// this api is just a small selection of the most commonly needed standard library functions
// using this api means that your plugin might not need to link with the standard library at all
struct std_api_t {
        // allocate `bytes` of ram, return NULL on error
        void* (*malloc)(size_t bytes);

        // free a previously allocated block of ram
        void (*free)(void* malloc_buffer);

        // printf style formatting into `buffer` of `buffer_size` bytes, returns <0 on error or bytes used
        int (*snprintf)(char* buffer, size_t buffer_size, const char* fmt, ...);

        // copy `bytes` of memory from `src` to `dest`, return dest
        void* (*memcpy)(void* dest, const void* src, size_t bytes);

        // set `bytes` of memory in `dest` to `value`, return dest, value must be <256
        void* (*memset)(void* dest, int value, size_t bytes);
};

struct console_api_t {
        // run `command` on the in-game console
        // max length of `command` is limited to 512 bytes by the game
        // NOTE: the game wont run console commands until the main menu
        //       has finished loading otherwise it freezes waiting
        //       for the console to be ready (if your compiling shaders
        //       the game could seem frozen for several minutes)
        void (*RunCommand)(char* command);
};


// This is all the above structs wrapped up in one place
// why pointers to apis instead of the api itself? so that
// I may extend any api without changing the size of BetterAPI
// this helps with forwards compatibility and i wont need to
// update the BETTERAPI_VERSION as often.
typedef struct better_api_t {
        const struct hook_api_t* Hook;
        const struct log_buffer_api_t* LogBuffer;
        const struct simple_draw_t* SimpleDraw;
        const struct callback_api_t* Callback;
        const struct config_api_t* Config;
        const struct std_api_t* Stdlib;
        const struct console_api_t* Console;
} BetterAPI;
#endif // !BETTERAPI_API_H




#ifdef BETTERAPI_ENABLE_SFSE_MINIMAL
#ifndef BETTERAPI_SFSE_MINIMAL
#define BETTERAPI_SFSE_MINIMAL

// For people that want to port Cheat Engine or ASI mods to sfse without including
// sfse code into the project, define BETTERAPI_ENABLE_SFSE_MINIMAL before 
// including betterapi.h, this should get you 90% of the way to a
// plug and play sfse plugin with no brain required:
// 
// #define BETTERAPI_ENABLE_SFSE_MINIMAL
// #include "betterapi.h"

// if you want your dll to be loadable with sfse you need to export two symbols:
// `SFSEPlugin_Version` and `SFSEPlugin_Load` paste this in one translation unit
// and modify to suit your needs:

/*
// Step 1) Export this struct so sfse knows your DLL is compatible
DLLEXPORT SFSEPluginVersionData SFSEPlugin_Version = {
        1,                            // SFSE api version, 1 is current

        1,                            // Plugin api version, 1 is current

        "BetterConsole",              // Mod/Plugin Name (limit: 256 characters)

        "Linuxversion",               // Mod Author(s)   (limit: 256 characters)

        1,                            // <0 uses hardcoded offsets (game version specific)>
                                      // <1 uses signatures (not game version specific)>

        1,                            // <0 relies on specific game structs>
                                      // <1 mod does not care if game structs change>

        {MAKE_VERSION(1, 11, 36), 0}, // compatible with game version 1.11.36

        0,                            // 0 = does not rely on any specific sfse version

        0, 0                          // reserved fields, must be 0
};

// Step 2) Export this function so sfse knows to load your dll.
//         Doing anything inside the function is optional.
DLLEXPORT void SFSEPlugin_Load(const SFSEInterface * sfse) {}
*/


#define MAKE_VERSION(major, minor, build) ((((major)&0xFF)<<24)|(((minor)&0xFF)<<16)|(((build)&0xFFF)<<4))

typedef uint32_t PluginHandle;

typedef enum SFSEEnumeration_t {
        InterfaceID_Invalid = 0,
        InterfaceID_Messaging,
        InterfaceID_Trampoline,

        MessageType_SFSE_PostLoad = 0,
        MessageType_SFSE_PostPostLoad,
} InterfaceID;

typedef struct SFSEPluginVersionData_t {
        uint32_t        dataVersion;
        uint32_t        pluginVersion;
        char            name[256];
        char            author[256];
        uint32_t        addressIndependence;
        uint32_t        structureIndependence;
        uint32_t        compatibleVersions[16];
        uint32_t        seVersionRequired;
        uint32_t        reservedNonBreaking;
        uint32_t        reservedBreaking;
} SFSEPluginVersionData;

typedef struct SFSEPluginInfo_t {
        uint32_t	infoVersion;
        const char*     name;
        uint32_t	version;
} SFSEPluginInfo;

typedef struct SFSEInterface_t {
        uint32_t	sfseVersion;
        uint32_t	runtimeVersion;
        uint32_t	interfaceVersion;
        void*           (*QueryInterface)(uint32_t id);
        PluginHandle    (*GetPluginHandle)(void);
        SFSEPluginInfo* (*GetPluginInfo)(const char* name);
} SFSEInterface;

typedef struct SFSEMessage_t {
        const char*     sender;
        uint32_t        type;
        uint32_t        dataLen;
        void*           data;
} SFSEMessage;

typedef void (*SFSEMessageEventCallback)(SFSEMessage* msg);

typedef struct SFSEMessagingInterface_t {
        uint32_t        interfaceVersion;
        bool            (*RegisterListener)(PluginHandle listener, const char* sender, SFSEMessageEventCallback handler);
        bool	        (*Dispatch)(PluginHandle sender, uint32_t messageType, void* data, uint32_t dataLen, const char* receiver);
}SFSEMessagingInterface;

#endif // !BETTERAPI_SFSE_MINIMAL
#endif

#ifdef BETTERAPI_IMPLEMENTATION
void BETTERAPI_INIT(const struct better_api_t* betterapi) {
        // Currently nothing is done to initialize the beterapi system
        // this may change in the future
}
#endif // BETTERAPI_IMPLEMENTATION
