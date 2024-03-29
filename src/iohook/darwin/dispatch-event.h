#import <AppKit/AppKit.h>

extern bool dispatch_hook_enabled();
extern bool dispatch_hook_disabled();
extern bool dispatch_key_press(NSEvent *event);
extern bool dispatch_button_press(NSEvent *event, NSInteger button);
extern bool dispatch_mouse_wheel(NSEvent *event);
