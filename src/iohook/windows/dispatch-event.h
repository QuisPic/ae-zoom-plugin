#include <cstdint>
#include <windows.h>

extern bool dispatch_hook_enabled();
extern bool dispatch_hook_disabled();
extern bool dispatch_key_press(WPARAM wParam, WORD flags);
extern bool dispatch_button_press(MOUSEHOOKSTRUCT *mshook, uint16_t button);
extern bool dispatch_mouse_wheel(MOUSEHOOKSTRUCTEX *mshook, uint8_t direction);
