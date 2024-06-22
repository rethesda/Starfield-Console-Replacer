#pragma once

#include <stdlib.h>
#include <stdint.h>

#define MODMENU_DEBUG

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRA_LEAN

#define BETTERAPI_ENABLE_SFSE_MINIMAL
#include "../betterapi.h"

#include "debug.h"
#include "callback.h"
#include "hook_api.h"
#include "simpledraw.h"
#include "log_buffer.h"
#include "settings.h"

#define IM_ASSERT(_EXPR) ASSERT(_EXPR)
#include "../imgui/imgui.h"


#define INTERNAL_PLUGIN_NAME "(internal)"
#define BETTERCONSOLE_VERSION "1.4.1"
constexpr uint32_t GAME_VERSION = MAKE_VERSION(1, 12, 32);



//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
struct ModMenuSettings {
        uint32_t FontScaleOverride = 0;
        bool PauseGameWhenOpened = false;
};


extern const ModMenuSettings* GetSettings();
extern ModMenuSettings* GetSettingsMutable();
extern const BetterAPI* GetBetterAPI();
