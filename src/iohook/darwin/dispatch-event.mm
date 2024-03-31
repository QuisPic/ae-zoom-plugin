#import "dispatch-event.h"
#import "input-helper.h"
#import "iohook.h"
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

bool dispatch_key_press(NSEvent *event) {
  bool consumed = false;

  // Populate key pressed event.
  io_event.time = event.timestamp;
  io_event.reserved = 0x00;
  io_event.type = EVENT_KEY_PRESSED;
  io_event.mask = get_modifiers(event.modifierFlags);

  io_event.data.keyboard.keycode = keycode_to_scancode(event.keyCode);
  io_event.data.keyboard.rawcode = event.keyCode;
  io_event.data.keyboard.keychar = CHAR_UNDEFINED;
  io_event.data.keyboard.repeat = event.ARepeat;

  // Fire key pressed event.
  dispatch_event(&io_event);
  consumed = io_event.reserved & 0x01;

  return consumed;
}

bool dispatch_button_press(NSEvent *event, NSInteger button) {
  bool consumed = false;

  // Populate mouse pressed event.
  io_event.time = event.timestamp;
  io_event.reserved = 0x00;

  io_event.type = EVENT_MOUSE_PRESSED;
  io_event.mask = get_modifiers(event.modifierFlags);

  // io_event.data.mouse.button = event.buttonNumber + 1;
  io_event.data.mouse.button = button;
  io_event.data.mouse.clicks = event.clickCount;
  io_event.data.mouse.x = 0; // we don't need location
  io_event.data.mouse.y = 0; // we don't need location

  // Fire mouse pressed event.
  dispatch_event(&io_event);
  consumed = io_event.reserved & 0x01;

  return consumed;
}

bool dispatch_mouse_wheel(NSEvent *event) {
  bool consumed = false;

  // Populate mouse wheel event.
  io_event.time = event.timestamp;
  io_event.reserved = 0x00;

  io_event.type = EVENT_MOUSE_WHEEL;
  io_event.mask = get_modifiers(event.modifierFlags);

  io_event.data.wheel.x = 0; // we don't need location
  io_event.data.wheel.y = 0; // we don't need location
  io_event.data.wheel.delta = 0;
  io_event.data.wheel.rotation = 0;

  if (event.hasPreciseScrollingDeltas) {
    // continuous device (trackpad)
    io_event.data.wheel.precise = true;
    io_event.data.wheel.type = WHEEL_BLOCK_SCROLL;

    if (event.deltaY != 0) {
      io_event.data.wheel.direction = WHEEL_VERTICAL_DIRECTION;
      io_event.data.wheel.delta = event.deltaY;
      io_event.data.wheel.rotation = event.deltaY;
    } else if (event.deltaX != 0) {
      io_event.data.wheel.direction = WHEEL_HORIZONTAL_DIRECTION;
      io_event.data.wheel.delta = event.deltaX;
      io_event.data.wheel.rotation = event.deltaX;
    }
  } else {
    // non-continuous device (wheel mice)
    io_event.data.wheel.type = WHEEL_UNIT_SCROLL;
    io_event.data.wheel.precise = false;

    if (event.scrollingDeltaY != 0) {
      io_event.data.wheel.direction = WHEEL_VERTICAL_DIRECTION;
      io_event.data.wheel.delta = event.scrollingDeltaY;
      io_event.data.wheel.rotation = event.scrollingDeltaY;
    } else if (event.scrollingDeltaX != 0) {
      io_event.data.wheel.direction = WHEEL_HORIZONTAL_DIRECTION;
      io_event.data.wheel.delta = event.scrollingDeltaX;
      io_event.data.wheel.rotation = event.scrollingDeltaX;
    }
  }

  // Fire mouse wheel event.
  if (io_event.data.wheel.rotation != 0) {
    dispatch_event(&io_event);
  }
  consumed = io_event.reserved & 0x01;

  return consumed;
}
