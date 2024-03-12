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
#include <uiohook.h>

using json = nlohmann::json;

#define AEGP_MAX_STREAM_DIM 4
constexpr uint16_t MOUSE_WHEEL_UP = 1;
constexpr uint16_t MOUSE_WHEEL_DOWN = 2;
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

enum class KB_ACTION { CHANGE, SET_TO };

enum class ZOOM_STATUS { INITIALIZATION_ERROR, INITIALIZED, FINISHED };

class KeyCodes {
public:
  event_type type;
  uint16_t mask;
  uint16_t keycode;

  KeyCodes() = default;

  KeyCodes(const event_type &type, const uint16_t &mask,
           const uint16_t &keycode)
      : type(type), mask(mask), keycode(keycode) {}

  KeyCodes(uiohook_event *e) {
    type = e->type;
    mask = 0x0;

    // make the mask accept both left and right buttons
    if (e->mask & (MASK_CTRL)) {
      mask |= MASK_CTRL;
    }

    if (e->mask & (MASK_META)) {
      mask |= MASK_META;
    }

    if (e->mask & (MASK_SHIFT)) {
      mask |= MASK_SHIFT;
    }

    if (e->mask & (MASK_ALT)) {
      mask |= MASK_ALT;
    }

    switch (e->type) {
    case EVENT_KEY_PRESSED:
      keycode = e->data.keyboard.keycode;
      break;
    case EVENT_MOUSE_PRESSED:
      keycode = e->data.mouse.button;
      break;
    case EVENT_MOUSE_WHEEL:
      keycode = e->data.wheel.rotation >= 0 ? MOUSE_WHEEL_UP : MOUSE_WHEEL_DOWN;
      break;
    default:
      keycode = 0;
      break;
    }
  }

  bool operator==(const KeyCodes &other) const {
    return type == other.type && (mask == other.mask || (mask & other.mask)) &&
           keycode == other.keycode;
  }
};

class KeyBindAction {
public:
  bool enabled;
  KeyCodes keyCodes;
  KB_ACTION action;
  double amount;

  KeyBindAction() = default;
  KeyBindAction(const bool enabled, const KeyCodes &keyCodes,
                const KB_ACTION &action, double amount)
      : enabled(enabled), keyCodes(keyCodes), action(action), amount(amount) {}
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(KeyCodes, type, mask, keycode);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(KeyBindAction, enabled, keyCodes, action,
                                   amount);
