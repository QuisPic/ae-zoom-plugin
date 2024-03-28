#include "osx.h"

#ifdef AE_OS_MAC
#import <AppKit/AppKit.h>
#import <AppKit/NSWorkspace.h>
#import <Foundation/Foundation.h>

int osx::getFrontmostAppPID() {
  NSRunningApplication *focusedApp;
  focusedApp = [[NSWorkspace sharedWorkspace] frontmostApplication];

  return [focusedApp processIdentifier];
}

std::optional<int> osx::windowUnderCursorPID() {
  pid_t pidUnderCursor = -1;
  NSPoint cursorLocation = [NSEvent mouseLocation];
  NSInteger windowNumber = [NSWindow windowNumberAtPoint:cursorLocation
                             belowWindowWithWindowNumber:0];

  NSArray *windowInfo = (__bridge_transfer NSArray *)CGWindowListCopyWindowInfo(
      kCGWindowListOptionIncludingWindow, (CGWindowID)windowNumber);

  if (windowInfo.count > 0) {
    pidUnderCursor =
        [windowInfo[0][(__bridge NSString *)kCGWindowOwnerPID] intValue];
  }

  return pidUnderCursor != -1 ? std::optional(pidUnderCursor) : std::nullopt;
}

void osx::addEventMonitor() {
  // Define the event mask to listen for key down events
  NSEventMask eventMask = NSEventMaskKeyDown | NSEventMaskLeftMouseDown |
                          NSEventMaskRightMouseDown | NSEventMaskOtherMouseDown;

  // Implement the event handler block
  NSEvent * (^eventHandler)(NSEvent *) = ^NSEvent *(NSEvent *theEvent) {
    if ([theEvent type] == NSEventTypeKeyDown) {
      NSLog(@"Key is pressed with keycode %d", theEvent.keyCode);
    } else if ([theEvent type] == NSEventTypeLeftMouseDown ||
               [theEvent type] == NSEventTypeRightMouseDown ||
               [theEvent type] == NSEventTypeOtherMouseDown) {
      NSLog(@"Mouse button pressed with button number %d",
            theEvent.buttonNumber);
    }
    // You can return a modified event here or nil to stop dispatching
    return theEvent;
  };

  // Add the local monitor for events
  [NSEvent addLocalMonitorForEventsMatchingMask:eventMask handler:eventHandler];
}
#endif
