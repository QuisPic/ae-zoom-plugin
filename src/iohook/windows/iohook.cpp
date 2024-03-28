#include "iohook.h"
#include <iostream>

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

HHOOK win::addEventHookForHwnd(HWND hWnd) {
  HHOOK hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD, KeyboardHookProc, NULL,
                                         GetWindowThreadProcessId(hWnd, NULL));

  return hKeyboardHook;
}

// LRESULT CALLBACK HookProc(int nCode, WPARAM wParam, LPARAM lParam) {
//   if (nCode == HCBT_CREATEWND) {
//     // Get the HWND of the created window
//     HWND hWnd = (HWND)lParam;
//
//     // Get the process ID of the thread that created the window
//     DWORD dwProcessId;
//     GetWindowThreadProcessId(hWnd, &dwProcessId);
//
//     // Get the current process ID
//     DWORD currentProcessId = GetCurrentProcessId();
//
//     // Check if the window belongs to the current process
//     if (dwProcessId == currentProcessId) {
//       // Install a keyboard hook for the specific window
//       HHOOK hKeyboardHook =
//           SetWindowsHookEx(WH_KEYBOARD, KeyboardHookProc, NULL,
//                            GetWindowThreadProcessId(hWnd, NULL));
//       if (hKeyboardHook == NULL) {
//         // Handle hook installation failure
//       }
//     }
//   }
//
//   // Pass the message to the next hook procedure
//   return CallNextHookEx(NULL, nCode, wParam, lParam);
// }
//
// int main() {
//   // Install a creation hook to monitor window creation
//   HHOOK hCBThook = SetWindowsHookEx(WH_CBT, HookProc, NULL, 0);
//   if (hCBThook == NULL) {
//     // Handle hook installation failure
//   }
//
//   // Message loop to keep the application running
//   MSG msg;
//   while (GetMessage(&msg, NULL, 0, 0) > 0) {
//     TranslateMessage(&msg);
//     DispatchMessage(&msg);
//   }
//
//   // Unhook before exiting
//   UnhookWindowsHookEx(hCBThook);
//
//   return 0;
// }
