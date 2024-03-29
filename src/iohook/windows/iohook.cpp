#include "iohook.h"
#include <iostream>
#include <string>

LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
  if (nCode == HC_ACTION) {
    KBDLLHOOKSTRUCT *keyInfo = (KBDLLHOOKSTRUCT *)lParam;

    std::cout << "Key pressed with key code: " << wParam << std::endl;

    // Optionally, prevent further processing by returning -1
    // return -1;
  }

  // Pass the message to the next hook procedure in the keyboard hook chain
  return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
  if (nCode == HC_ACTION) {
    std::string msg;

    switch (wParam) {
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
      std::cout << "Mouse click: " << wParam << "\n";
      break;
    case WM_MOUSEWHEEL:
      std::cout << "Mouse wheel: " << wParam << "\n";
      break;
    }

    // Optionally, prevent further processing by returning -1
    // return -1;
  }

  // Pass the message to the next hook procedure in the keyboard hook chain
  return CallNextHookEx(NULL, nCode, wParam, lParam);
}

HHOOK win::addEventHookForHwnd(HWND hWnd) {
  DWORD hwndThreadId = GetWindowThreadProcessId(hWnd, NULL);
  HHOOK hKeyboardHook =
      SetWindowsHookEx(WH_KEYBOARD, KeyboardHookProc, NULL, hwndThreadId);
  HHOOK hMouseHook =
      SetWindowsHookEx(WH_MOUSE, MouseHookProc, NULL, hwndThreadId);

  return hKeyboardHook;
}
