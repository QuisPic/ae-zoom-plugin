#include "dispatch-event.h"
#include "input-helper.h"
#include "iohook.h"
#include "logger.h"

// Virtual event pointer.
static iohook_event io_event;

// Event dispatch callback.
static dispatcher_t dispatch = NULL;
static void *dispatch_data = NULL;

void iohook_set_dispatch_proc(dispatcher_t dispatch_proc, void *user_data) {
  dispatch = dispatch_proc;
  dispatch_data = user_data;
}

// Send out an event if a dispatcher was set.
static void dispatch_event(iohook_event *const event) {
  if (dispatch != NULL) {
    dispatch(event, dispatch_data);
  } else {
    logger(LOG_LEVEL_WARN, "%s [%u]: No dispatch callback set!\n", __FUNCTION__,
           __LINE__);
  }
}

bool dispatch_hook_enabled() {
  bool consumed = false;

  // Populate the hook start event.
  io_event.time = 0; // we don't need timestamp for this event
  io_event.reserved = 0x00;

  io_event.type = EVENT_HOOK_ENABLED;
  io_event.mask = 0x00;

  // Fire the hook start event.
  dispatch_event(&io_event);
  consumed = io_event.reserved & 0x01;

  return consumed;
}

bool dispatch_hook_disabled() {
  bool consumed = false;

  // Populate the hook stop event.
  io_event.time = 0; // we don't need timestamp for this event
  io_event.reserved = 0x00;

  io_event.type = EVENT_HOOK_DISABLED;
  io_event.mask = 0x00;

  // Fire the hook stop event.
  dispatch_event(&io_event);

  consumed = io_event.reserved & 0x01;

  return consumed;
}

bool dispatch_key_press(WPARAM wParam, WORD flags) {
  bool consumed = false;

  // Populate key pressed event.
  io_event.time = 0; // don't need time at the moment
  io_event.reserved = 0x00;
  io_event.type = EVENT_KEY_PRESSED;
  io_event.mask = get_modifiers();

  io_event.data.keyboard.keycode = keycode_to_scancode((DWORD)wParam, flags);
  io_event.data.keyboard.rawcode = (uint16_t)wParam;
  io_event.data.keyboard.keychar = CHAR_UNDEFINED;
  io_event.data.keyboard.repeat = (flags & KF_REPEAT) == KF_REPEAT;

  // Fire key pressed event.
  dispatch_event(&io_event);
  consumed = io_event.reserved & 0x01;

  return consumed;
}

bool dispatch_button_press(MOUSEHOOKSTRUCT *mshook, uint16_t button) {
  bool consumed = false;

  // Populate mouse pressed event.
  io_event.time = 0; // we don't need time
  io_event.reserved = 0x00;

  io_event.type = EVENT_MOUSE_PRESSED;
  io_event.mask = get_modifiers();

  io_event.data.mouse.button = button;
  io_event.data.mouse.clicks = 0; // we don't need clicks count
  io_event.data.mouse.x = 0;      // we don't need location
  io_event.data.mouse.y = 0;      // we don't need location

  // Fire mouse pressed event.
  dispatch_event(&io_event);
  consumed = io_event.reserved & 0x01;

  return consumed;
}

bool dispatch_mouse_wheel(MOUSEHOOKSTRUCTEX *mshook, uint8_t direction) {
  bool consumed = false;

  // Populate mouse wheel event.
  io_event.time = 0; // we don't need time
  io_event.reserved = 0x00;

  io_event.type = EVENT_MOUSE_WHEEL;
  io_event.mask = get_modifiers();

  io_event.data.wheel.x = 0; // we don't need location
  io_event.data.wheel.y = 0; // we don't need location
  io_event.data.wheel.rotation =
      (int16_t)GET_WHEEL_DELTA_WPARAM(mshook->mouseData);
  io_event.data.wheel.delta = WHEEL_DELTA;
  io_event.data.wheel.precise = false;

  UINT uiAction = SPI_GETWHEELSCROLLLINES;
  if (direction == WHEEL_HORIZONTAL_DIRECTION) {
    uiAction = SPI_GETWHEELSCROLLCHARS;
  }

  UINT wheel_amount = 3;
  if (SystemParametersInfo(uiAction, 0, &wheel_amount, 0)) {
    if (wheel_amount == WHEEL_PAGESCROLL) {
      /* If this number is WHEEL_PAGESCROLL, a wheel roll should be interpreted
       * as clicking once in the page down or page up regions of the scroll bar.
       */

      io_event.data.wheel.type = WHEEL_BLOCK_SCROLL;
      io_event.data.wheel.rotation *= 1;
    } else {
      /* If this number is 0, no scrolling should occur.
       * If the number of lines to scroll is greater than the number of lines
       * viewable, the scroll operation should also be interpreted as a page
       * down or page up operation. */

      io_event.data.wheel.type = WHEEL_UNIT_SCROLL;
      io_event.data.wheel.rotation *= wheel_amount;
    }

    // Set the direction based on what event was received.
    io_event.data.wheel.direction = direction;

    // Fire mouse wheel event.
    dispatch_event(&io_event);
    consumed = io_event.reserved & 0x01;
  } else {
    logger(LOG_LEVEL_WARN, "%s [%u]: SystemParametersInfo() failed.\n",
           __FUNCTION__, __LINE__);
  }

  return consumed;
}
