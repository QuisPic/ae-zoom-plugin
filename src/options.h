#pragma once
#include "iohook.h"
#include <nlohmann/json.hpp>
#include <vector>

constexpr uint16_t MOUSE_WHEEL_UP = 1;
constexpr uint16_t MOUSE_WHEEL_DOWN = 2;

enum class ZOOM_AROUND { PANEL_CENTER, CURSOR_POSTION };
enum class KB_ACTION { CHANGE, DECREMENT, SET_TO };

struct ViewPositionExperimentalOption {
  bool enabled;
  ZOOM_AROUND zoomAround;

  ViewPositionExperimentalOption()
      : enabled(false), zoomAround(ZOOM_AROUND::PANEL_CENTER){};

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(ViewPositionExperimentalOption, enabled,
                                 zoomAround);
};

struct ExperimentalOptions {
  bool detectCursorInsideView;
  ViewPositionExperimentalOption fixViewportPosition;

  ExperimentalOptions()
      : detectCursorInsideView(false), fixViewportPosition(){};

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(ExperimentalOptions, detectCursorInsideView,
                                 fixViewportPosition);
};

struct HighDpiOptions {
  bool enabled;
  float scale;

  HighDpiOptions() : enabled(false), scale(2) {}

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(HighDpiOptions, enabled, scale);
};

class KeyCodes {
public:
  event_type type = EVENT_KEY_PRESSED;
  uint16_t mask = 0x0;
  uint16_t keycode = VC_UNDEFINED;

  KeyCodes() = default;

  KeyCodes(const event_type &type, const uint16_t &mask,
           const uint16_t &keycode)
      : type(type), mask(mask), keycode(keycode) {}

  KeyCodes(iohook_event *e) {
    type = e->type;
    // mask = 0x0;
    mask = e->mask;

    // make the mask accept both left and right buttons
    // if (e->mask & (MASK_CTRL)) {
    //   mask |= MASK_CTRL;
    // }
    //
    // if (e->mask & (MASK_META)) {
    //   mask |= MASK_META;
    // }
    //
    // if (e->mask & (MASK_SHIFT)) {
    //   mask |= MASK_SHIFT;
    // }
    //
    // if (e->mask & (MASK_ALT)) {
    //   mask |= MASK_ALT;
    // }

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
    return type == other.type && mask == other.mask && keycode == other.keycode;
  }

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(KeyCodes, type, mask, keycode);
};

class KeyBindAction {
public:
  bool enabled = true;
  KeyCodes keyCodes;
  KB_ACTION action = KB_ACTION::CHANGE;
  double amount = 0;

  double getAmount(const HighDpiOptions &highDpiOpt) const {
    return highDpiOpt.enabled ? amount / highDpiOpt.scale : amount;
  }

  KeyBindAction() = default;
  KeyBindAction(const bool enabled, const KeyCodes &keyCodes,
                const KB_ACTION &action, double amount)
      : enabled(enabled), keyCodes(keyCodes), action(action), amount(amount) {}

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(KeyBindAction, enabled, keyCodes, action,
                                 amount);
};

extern ExperimentalOptions gExperimentalOptions;
extern HighDpiOptions gHighDpiOptions;
extern std::vector<KeyBindAction> gKeyBindings;
