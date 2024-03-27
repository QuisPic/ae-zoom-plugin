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
#endif
