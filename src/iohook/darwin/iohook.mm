#import "iohook.h"
#import "dispatch-event.h"
#import "input-helper.h"
#import <AppKit/AppKit.h>

static id _Nullable monitorHandler = NULL;

// Implement the event handler block
NSEvent * (^eventHandler)(NSEvent *) = ^NSEvent *(NSEvent *event) {
  bool consumed = false;

  switch ([event type]) {
  case NSEventTypeKeyDown:
    consumed = dispatch_key_press(event);
    break;
  case NSEventTypeKeyUp:
    consumed = dispatch_key_release(event);
    break;
  case NSEventTypeLeftMouseDown:
    consumed = dispatch_button_press(event, MOUSE_BUTTON1);
    break;
  case NSEventTypeRightMouseDown:
    consumed = dispatch_button_press(event, MOUSE_BUTTON2);
    break;
  case NSEventTypeOtherMouseDown:
    if (event.buttonNumber < UINT16_MAX) {
      uint16_t button = event.buttonNumber + 1;
      consumed = dispatch_button_press(event, button);
    }
    break;
  case NSEventTypeScrollWheel:
    consumed = dispatch_mouse_wheel(event);
    break;
  default:
    break;
  }

  // Return a modified event here or NULL to stop dispatching
  return consumed ? NULL : event;
};

int iohook_run() {
  // Define the event mask to listen for
  NSEventMask eventMask = NSEventMaskKeyDown | NSEventMaskKeyUp | NSEventMaskLeftMouseDown |
                          NSEventMaskRightMouseDown |
                          NSEventMaskOtherMouseDown | NSEventMaskScrollWheel;

  // Add the local monitor for events
  monitorHandler = [NSEvent addLocalMonitorForEventsMatchingMask:eventMask
                                                         handler:eventHandler];

  if (monitorHandler) {
    dispatch_hook_enabled();
    return UIOHOOK_SUCCESS;
  } else {
    return UIOHOOK_ERROR_CREATE_EVENT_MONITOR;
  }
}

int iohook_stop() {
  if (monitorHandler) {
    [NSEvent removeMonitor:monitorHandler];
    monitorHandler = NULL;
  }

  dispatch_hook_disabled();
  return UIOHOOK_SUCCESS;
}
