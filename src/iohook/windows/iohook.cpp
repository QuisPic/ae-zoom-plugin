#include "iohook.h"
#include <windows.h>

#include "dispatch-event.h"

static HHOOK keyboard_event_hook = NULL;
static HHOOK mouse_event_hook = NULL;

LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
  bool consumed = false;
  WORD flags = HIWORD(lParam);

  /** accept only key down events and skip modifier keys */
  if (nCode == HC_ACTION && !(flags & KF_UP) && wParam != VK_SHIFT &&
      wParam != VK_CONTROL && wParam != VK_LWIN && wParam != VK_RWIN &&
      wParam != VK_MENU) {
    consumed = dispatch_key_press(wParam, flags);
  }

  if (!consumed || nCode < 0) {
    return CallNextHookEx(NULL, nCode, wParam, lParam);
  } else {
    return -1;
  }
}

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
  bool consumed = false;

  if (nCode == HC_ACTION) {
    switch (wParam) {
    case WM_LBUTTONDOWN:
      consumed =
          dispatch_button_press((MOUSEHOOKSTRUCT *)lParam, MOUSE_BUTTON1);
      break;
    case WM_RBUTTONDOWN:
      consumed =
          dispatch_button_press((MOUSEHOOKSTRUCT *)lParam, MOUSE_BUTTON2);
      break;
    case WM_MBUTTONDOWN:
      consumed =
          dispatch_button_press((MOUSEHOOKSTRUCT *)lParam, MOUSE_BUTTON3);
      break;
    case WM_XBUTTONDOWN: {
      auto mshook = (MOUSEHOOKSTRUCTEX *)lParam;
      if (HIWORD(mshook->mouseData) == XBUTTON1) {
        consumed = dispatch_button_press(mshook, MOUSE_BUTTON4);
      } else if (HIWORD(mshook->mouseData) == XBUTTON2) {
        consumed = dispatch_button_press(mshook, MOUSE_BUTTON5);
      } else {
        uint16_t button = HIWORD(mshook->mouseData);
        consumed = dispatch_button_press(mshook, button);
      }
      break;
    }
    case WM_MOUSEWHEEL:
      consumed = dispatch_mouse_wheel((MOUSEHOOKSTRUCTEX *)lParam,
                                      WHEEL_VERTICAL_DIRECTION);
      break;
    case WM_MOUSEHWHEEL:
      consumed = dispatch_mouse_wheel((MOUSEHOOKSTRUCTEX *)lParam,
                                      WHEEL_HORIZONTAL_DIRECTION);
      break;
    default:
      break;
    }
  }

  if (!consumed || nCode < 0) {
    return CallNextHookEx(NULL, nCode, wParam, lParam);
  } else {
    return -1;
  }
}

int iohook_run(HWND hWnd) {
  DWORD hwndThreadId = GetWindowThreadProcessId(hWnd, NULL);

  // Create the native hooks.
  keyboard_event_hook =
      SetWindowsHookEx(WH_KEYBOARD, KeyboardHookProc, NULL, hwndThreadId);
  mouse_event_hook =
      SetWindowsHookEx(WH_MOUSE, MouseHookProc, NULL, hwndThreadId);

  if (keyboard_event_hook != NULL && mouse_event_hook != NULL) {
    dispatch_hook_enabled();
    return UIOHOOK_SUCCESS;
  } else {
    return UIOHOOK_ERROR_SET_WINDOWS_HOOK_EX;
  }
}

int iohook_stop() {
  if (keyboard_event_hook == NULL && mouse_event_hook == NULL) {
    return UIOHOOK_FAILURE;
  }

  if (keyboard_event_hook != NULL) {
    UnhookWindowsHookEx(keyboard_event_hook);
    keyboard_event_hook = NULL;
  }

  if (mouse_event_hook != NULL) {
    UnhookWindowsHookEx(mouse_event_hook);
    mouse_event_hook = NULL;
  }

  dispatch_hook_disabled();
  return UIOHOOK_SUCCESS;
}
