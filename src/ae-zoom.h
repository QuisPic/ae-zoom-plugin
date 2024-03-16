#pragma once

#include "AEConfig.h"
#include <string>

#ifdef AE_OS_WIN
#include <windows.h>
#endif

#include "AE_GeneralPlug.h"
#include "ExternalObject/SoCClient.h"
#include "entry.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

#define AEGP_MAX_STREAM_DIM 4
constexpr char SETTINGS_SECTION_NAME[] = "Quis/Ae_Smooth_Zoom";

// This entry point is exported through the PiPL (.r file)
extern "C" DllExport AEGP_PluginInitFuncPrototype EntryPointFunc;

class LogMessage {
public:
  unsigned int level = 0;
  std::string format_str;
  std::string instructions_str;

  LogMessage(unsigned int level, const std::string &_format_str,
             const std::string &_instructions_str)
      : level(level), format_str(_format_str),
        instructions_str(_instructions_str) {}
};

enum class WIND_RECT { FULL, CLIENT };

enum class ZOOM_STATUS { INITIALIZATION_ERROR, INITIALIZED, FINISHED };
