#include "ae-zoom.h"
#include "AEGP_SuiteHandler.h"
#include "AE_Macros.h"
#include "ExternalObject/SoSharedLibDefs.h"
#include "ae-egg.h"
#include "options.h"
#include "uiohook.h"
#include "zoom-actions.h"
#include "util-functions.h"
#include "JS/JS.h"
#include <atomic>
#include <cstdint>
#include <exception>
#include <future>
#include <mutex>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#ifdef AE_OS_MAC
#include <objc/objc-runtime.h>
#endif

static AEGP_PluginID S_zoom_id = 0L;
static SPBasicSuite *sP = NULL;

static A_Err (*S_call_idle_routines)(void) = nullptr;

static std::atomic<bool> S_is_creating_key_bind = false;
static std::atomic<bool> S_logging_enabled = true;
static std::shared_future<ZOOM_STATUS> S_zoom_status_future;
static std::optional<ZOOM_STATUS> S_zoom_status;
static std::shared_future<int> S_iohook_future;
static std::optional<KeyCodes> S_key_codes_pass;

static AEGP_Command hack_cmd;

#ifdef AE_OS_WIN
static HWND S_main_win_h = nullptr;
#endif

static std::vector<LogMessage> log_messages;
// static std::vector<KeyBindAction> S_zoom_actions;
static ZoomActions S_zoom_actions;

static std::mutex S_create_key_bind_mutex;
// static std::mutex S_zoom_action_mutex;
static std::mutex S_log_mutex;

/**
 * \brief Utility function to handle strings and memory clean up
 *
 * \param s - The referenced string
 */
static char *getNewBuffer(std::string &s) {
  // Dynamically allocate memory buffer to hold the string
  // to pass back to JavaScript
  char *buff = new char[1 + s.length()];

  memset(buff, 0, s.length() + 1);
  strcpy(buff, s.c_str());

  return buff;
}

void logger(unsigned int level, std::tuple<std::string, std::string> strings) {
  if (!S_logging_enabled) {
    return;
  };

  switch (level) {
  case LOG_LEVEL_INFO:
    break;

  case LOG_LEVEL_WARN:
  case LOG_LEVEL_ERROR:
    std::lock_guard lock(S_log_mutex);
    log_messages.emplace_back(level, std::get<0>(strings),
                              std::get<1>(strings));
    break;
  }
}

void logger(unsigned int level, const std::string &format_str,
            const std::string &instructions_str) {
  if (!S_logging_enabled) {
    return;
  };

  switch (level) {
  case LOG_LEVEL_INFO:
    break;

  case LOG_LEVEL_WARN:
  case LOG_LEVEL_ERROR:
    std::lock_guard lock(S_log_mutex);
    log_messages.emplace_back(level, format_str, instructions_str);
    break;
  }
}

bool IsMainWindowEnabled() {
#ifdef AE_OS_WIN
  return IsWindowEnabled(S_main_win_h);
#elif defined AE_OS_MAC
  return true;
#endif
}

std::tuple<A_Err, std::string> RunExtendscript(const std::string &script_str) {
  A_Err err = A_Err_NONE;

  /* After Effects can't execute script and displays an error if the main window
   * is disabled */
  if (!IsMainWindowEnabled()) {
    return {err, ""};
  }

  AEGP_SuiteHandler suites(sP);
  AEGP_MemHandle outResultH = nullptr;
  AEGP_MemHandle outErrStringH = nullptr;

  auto MemHandleToString = [&suites, &err](AEGP_MemHandle mH) -> std::string {
    void *memP = nullptr;

    ERR(suites.MemorySuite1()->AEGP_LockMemHandle(mH, &memP));
    std::string result_str = std::bit_cast<char *>(memP);
    ERR(suites.MemorySuite1()->AEGP_UnlockMemHandle(mH));

    return result_str;
  };

  ERR(suites.UtilitySuite6()->AEGP_ExecuteScript(
      S_zoom_id, script_str.c_str(), false, &outResultH, &outErrStringH));

  std::string result_str = MemHandleToString(outResultH);
  std::string error_str = MemHandleToString(outErrStringH);

  if (!error_str.empty()) {
    logger(LOG_LEVEL_ERROR, error_str, "");
  }

  ERR(suites.MemorySuite1()->AEGP_FreeMemHandle(outResultH));
  ERR(suites.MemorySuite1()->AEGP_FreeMemHandle(outErrStringH));

  return {err, result_str};
}

template <typename T>
static A_Err ReadAndStoreSetting(T *storage, const std::string &key,
                                 const std::string &exeption_str) {
  A_Err err = A_Err_NONE;
  AEGP_SuiteHandler suites(sP);
  AEGP_PersistentBlobH blobH = nullptr;
  constexpr int buf_size = 1000;
  A_char buf[buf_size];
  A_Boolean key_exists = false;

  ERR(suites.PersistentDataSuite4()->AEGP_GetApplicationBlob(
      AEGP_PersistentType_MACHINE_SPECIFIC, &blobH));

  if (blobH) {
    ERR(suites.PersistentDataSuite4()->AEGP_DoesKeyExist(
        blobH, SETTINGS_SECTION_NAME, key.c_str(), &key_exists));

    if (key_exists) {
      ERR(suites.PersistentDataSuite4()->AEGP_GetString(
          blobH, SETTINGS_SECTION_NAME, key.c_str(), nullptr, buf_size, buf,
          nullptr));

      try {
        json j = json::parse(std::string(buf));
        *storage = j.get<T>();
      } catch (const json::exception &e) {
        logger(LOG_LEVEL_ERROR, exeption_str, std::string(e.what()));
      }
    }
  }

  return err;
}

static A_Err ReadExperimentalOptions() {
  A_Err err = A_Err_NONE;

  ERR(ReadAndStoreSetting(&gExperimentalOptions, "experimental",
                          "\n\nCan't read Zoom \"Experimental\" options. "
                          "Please save the \"Experimental\" "
                          "options again using Zoom script.\n"));

  return err;
}

static A_Err ReadHighDpiOptions() {
  A_Err err = A_Err_NONE;

  ERR(ReadAndStoreSetting(
      &gHighDpiOptions, "highDPI",
      "\n\nCan't read Zoom \"High DPI Display Support\" "
      "options. Please save the \"High DPI Display Support\" "
      "options again using Zoom script.\n"));

  return err;
}

static A_Err ReadKeyBindings() {
  A_Err err = A_Err_NONE;

  ERR(ReadAndStoreSetting(&gKeyBindings, "keyBindings",
                          "\n\nCan't read Zoom Key Bindings. Please save "
                          "Key Bindings again using Zoom script.\n"));

  return err;
}

static bool IsSameProcessId(
#ifdef AE_OS_WIN
    HWND hwnd
#elif defined AE_OS_MAC
    CFDictionaryRef winDict
#endif
) {
#ifdef AE_OS_WIN
  static DWORD S_pid = GetCurrentProcessId();
  DWORD windowPIDValue;

  GetWindowThreadProcessId(hwnd, &windowPIDValue);
#elif defined AE_OS_MAC
  static int S_pid = getpid();
  int windowPIDValue;

  CFNumberRef windowPID =
      (CFNumberRef)CFDictionaryGetValue(winDict, kCGWindowOwnerPID);
  CFNumberGetValue(windowPID, kCFNumberIntType, &windowPIDValue);
#endif
  return windowPIDValue == S_pid;
}

bool isMainWindowActive() {
#ifdef AE_OS_WIN
  return IsSameProcessId(GetForegroundWindow());
#elif defined AE_OS_MAC
  CFArrayRef windowArray = CGWindowListCopyWindowInfo(
      kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements,
      kCGNullWindowID);

  for (CFIndex i = 0, n = CFArrayGetCount(windowArray); i < n; i++) {
    CFDictionaryRef windict =
        (CFDictionaryRef)CFArrayGetValueAtIndex(windowArray, i);
    CFNumberRef layernum =
        (CFNumberRef)CFDictionaryGetValue(windict, kCGWindowLayer);

    if (layernum) {
      int layer;
      CFNumberGetValue(layernum, kCFNumberIntType, &layer);

      if (layer == 0) {
        return IsSameProcessId(windict);
      }
    }
  }

  CFRelease(windowArray);
  return false;
#endif
}

bool IsCursorOnMainWindow() {
  bool isOverWindow = false;

#ifdef AE_OS_MAC
  // Get the cursor position
  CGEventRef event = CGEventCreate(NULL);
  CGPoint cursorPosition = CGEventGetLocation(event);
  CFRelease(event);

  CFArrayRef winList = CGWindowListCopyWindowInfo(
      kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements,
      kCGNullWindowID);

  for (CFIndex i = 0, n = CFArrayGetCount(winList); i < n; i++) {
    CFDictionaryRef winDict =
        (CFDictionaryRef)CFArrayGetValueAtIndex(winList, i);
    CFDictionaryRef bounds =
        (CFDictionaryRef)CFDictionaryGetValue(winDict, kCGWindowBounds);
    CGRect windowFrame;
    CGRectMakeWithDictionaryRepresentation(bounds, &windowFrame);

    // Check if the cursor is over the window's frame
    if (CGRectContainsPoint(windowFrame, cursorPosition)) {
      isOverWindow = IsSameProcessId(winDict);
      break;
    }
  }

  CFRelease(winList);
#elif defined AE_OS_WIN
  POINT cursor_pos;
  GetCursorPos(&cursor_pos); // Get the cursor position in screen coordinates

  isOverWindow = IsSameProcessId(WindowFromPoint(cursor_pos));
#endif

  return isOverWindow;
}

void dispatch_proc(uiohook_event *const event, void *user_data) {
  static uint16_t last_key_code = VC_UNDEFINED;

  if (event->type == EVENT_KEY_RELEASED &&
      event->data.keyboard.keycode == last_key_code) {
    last_key_code = VC_UNDEFINED;
  }

  if (event->type == EVENT_HOOK_ENABLED &&
      S_zoom_status_future.wait_for(std::chrono::seconds(0)) !=
          std::future_status::ready) {
    auto zoom_status_promise =
        std::bit_cast<std::promise<ZOOM_STATUS> *>(user_data);
    zoom_status_promise->set_value(ZOOM_STATUS::INITIALIZED);
  } else if (S_is_creating_key_bind) {
#ifdef AE_OS_WIN
    // stop event propagation for Escape because it must close the Key
    // Capture window and it may accidentally stop extendscript execution
    // if passed to AE
    if ((event->type == EVENT_KEY_PRESSED || event->type == EVENT_KEY_TYPED ||
         event->type == EVENT_KEY_RELEASED) &&
        event->data.keyboard.keycode == VC_ESCAPE) {
      event->reserved = 0x1;
    }
#endif

    if (event->type == EVENT_KEY_PRESSED ||
        event->type == EVENT_MOUSE_PRESSED ||
        event->type == EVENT_MOUSE_WHEEL) {
      if (event->type == EVENT_KEY_PRESSED) {
        if (last_key_code == event->data.keyboard.keycode) {
          return;
        } else {
          last_key_code = event->data.keyboard.keycode;
        }
      }

      const std::lock_guard lock(S_create_key_bind_mutex);
      S_key_codes_pass.emplace(event);

      S_call_idle_routines();
    }
  } else if (((event->type == EVENT_KEY_PRESSED && isMainWindowActive()) ||
              ((event->type == EVENT_MOUSE_PRESSED ||
                event->type == EVENT_MOUSE_WHEEL) &&
               IsCursorOnMainWindow()))) {
    KeyCodes current_key_codes(event);
    bool key_bind_found = false;

    for (const KeyBindAction &kbind : gKeyBindings) {
      if (!kbind.enabled) {
        continue;
      }

      if (current_key_codes == kbind.keyCodes) {
        //   std::lock_guard lock(S_zoom_action_mutex);
        //
        //   /** Add new action only if it's different from last action in queue
        //   */ if (!S_zoom_actions.empty() &&
        //       S_zoom_actions.back().action == kbind.action) {
        //     if (kbind.action == KB_ACTION::CHANGE) {
        //       S_zoom_actions.back().amount += kbind.amount;
        //     } else if (kbind.action == KB_ACTION::SET_TO) {
        //       S_zoom_actions.back().amount = kbind.amount;
        //     }
        //   } else {
        //     S_zoom_actions.push_back(kbind);
        //   }

        S_zoom_actions.post(kbind);

        key_bind_found = true;
      }
    }

    // stop event propagation on all AE's default key bindings that change
    // viewport zoom
    if (key_bind_found &&
        (event->type == EVENT_MOUSE_WHEEL ||
         current_key_codes == KeyCodes{EVENT_KEY_PRESSED, 0, VC_COMMA} ||
         current_key_codes == KeyCodes{EVENT_KEY_PRESSED, 0, VC_PERIOD} ||
#ifdef AE_OS_WIN
         current_key_codes ==
             KeyCodes{EVENT_KEY_PRESSED, MASK_CTRL, VC_MINUS} ||
         current_key_codes ==
             KeyCodes{EVENT_KEY_PRESSED, MASK_CTRL, VC_EQUALS}))
#elif defined AE_OS_MAC
         current_key_codes ==
             KeyCodes{EVENT_KEY_PRESSED, MASK_META, VC_MINUS} ||
         current_key_codes ==
             KeyCodes{EVENT_KEY_PRESSED, MASK_META, VC_EQUALS}))
#endif
    {
      event->reserved = 0x1; // stop event propagation
    }
  }
}

std::tuple<std::string, std::string> GetHookErrorStrings(int status) {
  std::tuple<std::string, std::string> result;

  if (status == UIOHOOK_SUCCESS) {
    return result;
  }

  switch (status) {
  // System level errors.
  case UIOHOOK_ERROR_OUT_OF_MEMORY:
    result = {"Failed to allocate memory.", ""};
    break;

  // X11 specific errors.
  case UIOHOOK_ERROR_X_OPEN_DISPLAY:
    result = {"Failed to open X11 display.", ""};
    break;

  case UIOHOOK_ERROR_X_RECORD_NOT_FOUND:
    result = {"Unable to locate XRecord extension.", ""};
    break;

  case UIOHOOK_ERROR_X_RECORD_ALLOC_RANGE:
    result = {"Unable to allocate XRecord range.", ""};
    break;

  case UIOHOOK_ERROR_X_RECORD_CREATE_CONTEXT:
    result = {"Unable to allocate XRecord context.", ""};
    break;

  case UIOHOOK_ERROR_X_RECORD_ENABLE_CONTEXT:
    result = {"Failed to enable XRecord context.", ""};
    break;

  case UIOHOOK_ERROR_X_RECORD_GET_CONTEXT:
    // NOTE This is the only platform specific error that occurs on hook_stop().
    result = {"Failed to get XRecord context.", ""};
    break;

  // Windows specific errors.
  case UIOHOOK_ERROR_SET_WINDOWS_HOOK_EX:
    result = {"Failed to register low level windows hook.", ""};
    break;

  // Darwin specific errors.
  case UIOHOOK_ERROR_AXAPI_DISABLED:
    result = {"Failed to enable access for assistive devices.",
              "Zoom plug-in requires accessibility permissions to detect key "
              "bindings. Enable the permissions in System Settings -> Privacy "
              "& Security -> Accessibility -> Adobe After Effects."};
    break;

  case UIOHOOK_ERROR_CREATE_EVENT_PORT:
    result = {"Failed to create apple event port.", ""};
    break;

  case UIOHOOK_ERROR_CREATE_RUN_LOOP_SOURCE:
    result = {"Failed to create apple run loop source.", ""};
    break;

  case UIOHOOK_ERROR_GET_RUNLOOP:
    result = {"Failed to acquire apple run loop.", ""};
    break;

  case UIOHOOK_ERROR_CREATE_OBSERVER:
    result = {"Failed to create apple run loop observer.", ""};
    break;

  // Default error.
  case UIOHOOK_FAILURE:
  default:
    result = {"An unknown hook error occurred.", ""};
    break;
  }

  return result;
}

int HookProc(std::promise<ZOOM_STATUS> zoom_status_promise) {
  // Set the event callback for uiohook events.
  hook_set_dispatch_proc(&dispatch_proc, &zoom_status_promise);

  // Start the hook and block.
  // NOTE If EVENT_HOOK_ENABLED was delivered, the status will always succeed.
  int status = hook_run();

  if (status != UIOHOOK_SUCCESS) {
    if (S_zoom_status_future.wait_for(std::chrono::seconds(0)) !=
        std::future_status::ready) {
      zoom_status_promise.set_value(ZOOM_STATUS::INITIALIZATION_ERROR);
    }

    logger(LOG_LEVEL_ERROR, GetHookErrorStrings(status));
  }

  return status;
}

static void StartIOHook() {
  S_zoom_status.reset();
  std::promise<ZOOM_STATUS> zoom_status_promise;
  S_zoom_status_future = zoom_status_promise.get_future();
  S_iohook_future =
      std::async(std::launch::async, HookProc, std::move(zoom_status_promise));
}

static A_Err IdleHook(AEGP_GlobalRefcon plugin_refconP, AEGP_IdleRefcon refconP,
                      A_long *max_sleepPL) {
  A_Err err = A_Err_NONE;

  /* Displaying errors and warnings */
  if (log_messages.size() > 0) {
    AEGP_SuiteHandler suites(sP);
    std::lock_guard lock(S_log_mutex);

    if (S_logging_enabled) {
      for (const auto &msg : log_messages) {
        std::string msg_str = msg.format_str;

        if (!msg.instructions_str.empty()) {
          msg_str += "\n" + msg.instructions_str;
        }

        ERR(suites.UtilitySuite3()->AEGP_ReportInfo(S_zoom_id,
                                                    msg_str.c_str()));
      }
    }

    log_messages.clear();
  }

  /* Passing key presses to the script panel */
  if (S_is_creating_key_bind && S_key_codes_pass) {
    AEGP_SuiteHandler suites(sP);
    KeyCodes keyCodes = S_key_codes_pass.value();

    const std::lock_guard lock(S_create_key_bind_mutex);
    S_key_codes_pass.reset();

    std::string pass_fn_script = pass_key_bind_js + "(" +
                                 std::to_string(keyCodes.type) + "," +
                                 std::to_string(keyCodes.mask) + "," +
                                 std::to_string(keyCodes.keycode) + ")";

    std::tie(err, std::ignore) = RunExtendscript(pass_fn_script);
  }

  /* Changing zoom on key presses */
  if (S_zoom_actions.size() > 0) {
    S_zoom_actions.runActions();
    // const std::lock_guard lock(S_zoom_action_mutex);

    /** Lock actions queue while we iterate over it */
    // const std::lock_guard lock(S_zoom_actions.getMutex());
    //
    // for (const auto &act : S_zoom_actions.actions()) {
    //   double zoomValue = act.getAmount(gHighDpiOptions);
    //
    //   if (gExperimentalOptions.fixViewportPosition.enabled) {
    //     std::optional<ViewPano> view_pano;
    //     if (gExperimentalOptions.detectCursorInsideView &&
    //         (act.keyCodes.type == EVENT_MOUSE_CLICKED ||
    //          act.keyCodes.type == EVENT_MOUSE_WHEEL)) {
    //       view_pano = gAeEgg.getViewPanoUnderCursor();
    //     } else {
    //       view_pano = gAeEgg.getActiveViewPano();
    //     }
    //
    //     if (view_pano) {
    //       ZOOM_AROUND zoom_around = ZOOM_AROUND::PANEL_CENTER;
    //
    //       switch (act.action) {
    //       case KB_ACTION::CHANGE: {
    //         if (gExperimentalOptions.fixViewportPosition.zoomAround ==
    //                 ZOOM_AROUND::CURSOR_POSTION &&
    //             act.keyCodes.type == EVENT_MOUSE_WHEEL) {
    //           zoom_around = ZOOM_AROUND::CURSOR_POSTION;
    //         }
    //
    //         auto current_zoom = view_pano->getZoom();
    //         view_pano->setZoomFixed(current_zoom + zoomValue / 100.0,
    //                                 zoom_around);
    //         break;
    //       }
    //       case KB_ACTION::SET_TO: {
    //         view_pano->setZoomFixed(zoomValue / 100.0, zoom_around);
    //         break;
    //       }
    //       default:
    //         break;
    //       }
    //     }
    //   } else {
    //     std::string script_str;
    //
    //     switch (act.action) {
    //     case KB_ACTION::CHANGE:
    //       script_str =
    //           zoom_increment_js + "(" + std::to_string(zoomValue) + ")";
    //       break;
    //     case KB_ACTION::SET_TO:
    //       script_str = zoom_set_to_js + "(" + std::to_string(zoomValue) + ")";
    //       break;
    //     default:
    //       break;
    //     }
    //
    //     std::tie(err, std::ignore) = RunExtendscript(script_str);
    //   }
    // }
    //
    // S_zoom_actions.clear();
  }

  return err;
}

static A_Err CommandHook(AEGP_GlobalRefcon plugin_refconPV, /* >> */
                         AEGP_CommandRefcon refconPV,       /* >> */
                         AEGP_Command command,              /* >> */
                         AEGP_HookPriority hook_priority,   /* >> */
                         A_Boolean already_handledB,        /* >> */
                         A_Boolean *handledPB)              /* << */
{
  A_Err err = A_Err_NONE;

  if (already_handledB) {
    return err;
  }

  if (command == hack_cmd) {
    AEGP_SuiteHandler suites(sP);

    // suites.UtilitySuite3()->AEGP_ReportInfo(S_zoom_id, msg_str.c_str());
    *handledPB = true;
  }

  return err;
}

static A_Err UpdateMenuHook(AEGP_GlobalRefcon plugin_refconPV, /* >> */
                            AEGP_UpdateMenuRefcon refconPV,    /* >> */
                            AEGP_WindowType active_window)     /* >> */
{
  A_Err err = A_Err_NONE;
  AEGP_SuiteHandler suites(sP);

  ERR(suites.CommandSuite1()->AEGP_EnableCommand(hack_cmd));

  return err;
}

static A_Err DeathHook(AEGP_GlobalRefcon plugin_refconP,
                       AEGP_DeathRefcon refconP) {
  // if the iohook thread is not finished
  if (S_iohook_future.valid() && S_iohook_future.wait_for(std::chrono::seconds(
                                     0)) == std::future_status::timeout) {
    int status = hook_stop();

    if (status != UIOHOOK_SUCCESS) {
      logger(LOG_LEVEL_ERROR, GetHookErrorStrings(status));
    }

    if (S_iohook_future.valid() &&
        S_iohook_future.wait_for(std::chrono::seconds(5)) !=
            std::future_status::ready) {
      logger(LOG_LEVEL_ERROR, "Zoom Plug-in did not close properly.", "");
    }
  }

  return A_Err_NONE;
}

A_Err EntryPointFunc(struct SPBasicSuite *pica_basicP,  /* >> */
                     A_long major_versionL,             /* >> */
                     A_long minor_versionL,             /* >> */
                     AEGP_PluginID aegp_plugin_id,      /* >> */
                     AEGP_GlobalRefcon *global_refconP) /* << */
{
  A_Err err = A_Err_NONE;

  try {
    sP = pica_basicP;
    S_zoom_id = aegp_plugin_id;
    AEGP_SuiteHandler suites(sP);
    S_call_idle_routines =
        suites.UtilitySuite6()->AEGP_CauseIdleRoutinesToBeCalled;

    ERR(suites.CommandSuite1()->AEGP_GetUniqueCommand(&hack_cmd));
    // ERR(suites.CommandSuite1()->AEGP_InsertMenuCommand(
    //     hack_cmd, "Hack Bip Bop", AEGP_Menu_ANIMATION,
    //     AEGP_MENU_INSERT_AT_BOTTOM));
    // ERR(suites.RegisterSuite5()->AEGP_RegisterCommandHook(
    //     S_zoom_id, AEGP_HP_BeforeAE, AEGP_Command_ALL, CommandHook, 0));
    //
    // ERR(suites.RegisterSuite5()->AEGP_RegisterUpdateMenuHook(
    //     S_zoom_id, UpdateMenuHook, 0));

    ERR(suites.RegisterSuite5()->AEGP_RegisterIdleHook(S_zoom_id, IdleHook, 0));
    ERR(suites.RegisterSuite5()->AEGP_RegisterDeathHook(S_zoom_id, DeathHook,
                                                        0));

    ERR(suites.MemorySuite1()->AEGP_SetMemReportingOn(true));

#ifdef AE_OS_WIN
    ERR(suites.UtilitySuite6()->AEGP_GetMainHWND(&S_main_win_h));
#endif

    ERR(ReadKeyBindings());
    ERR(ReadExperimentalOptions());
    ERR(ReadHighDpiOptions());

    gAeEgg = AeEgg(major_versionL);

    // Start hook thread
    StartIOHook();

  } catch (A_Err &thrown_err) {
    err = thrown_err;
  }

  return err;
}

/**
 * \brief Free any string memory which has been returned as function result.
 *
 * \param *p Pointer to the string
 */
extern "C" DllExport void ESFreeMem(void *p) { delete (char *)(p); }

/**
 * \brief Returns the version number of the library
 *
 * ExtendScript publishes this number as the version property of the object
 * created by new ExternalObject.
 */
extern "C" DllExport long ESGetVersion() { return 0x1; }

/**
 * \brief Initialize the library and return function signatures.
 *
 * These signatures have no effect on the arguments that can be passed to the
 * functions. They are used by JavaScript to cast the arguments, and to populate
 * the reflection interface.
 */
extern "C" DllExport char *ESInitialize(const TaggedData **argv, long argc) {
  // return nullptr;
  return "postZoomAction_df";
}

/**
 * \brief Terminate the library.
 *
 * Does any necessary clean up that is needed.
 */
extern "C" DllExport void ESTerminate() {}

template <typename T>
static std::optional<T> WaitForFuture(const std::shared_future<T> &f,
                                      std::chrono::seconds sec) {
  std::optional<T> result;

  if (f.valid() && f.wait_for(sec) == std::future_status::ready) {
    result = f.get();
  }

  return result;
}

template <typename F> void try_catch(F &&f) {
  try {
    f();
  } catch (std::exception &e) {
    logger(LOG_LEVEL_ERROR, "Error: " + std::string(e.what()), "");
  } catch (...) {
    logger(LOG_LEVEL_ERROR, "Unknown Error", "");
  }
}

static void FillZoomStatusForScript(TaggedData *retval) {
  auto zoom_status =
      WaitForFuture(S_zoom_status_future, std::chrono::seconds(0));

  if (zoom_status) {
    auto iohook_status =
        WaitForFuture(S_iohook_future, std::chrono::seconds(0));

    retval->type = kTypeInteger;

    if (iohook_status.has_value()) {
      retval->data.intval = static_cast<long>(ZOOM_STATUS::FINISHED);
    } else {
      retval->data.intval = static_cast<long>(zoom_status.value());
    }
  }
}

/* This function is called from the script panel */
extern "C" DllExport long status(TaggedData *argv, long argc,
                                 TaggedData *retval) {
  try_catch([&retval]() { FillZoomStatusForScript(retval); });

  return kESErrOK;
}

/* This function is called from the script panel */
extern "C" DllExport long getError(TaggedData *argv, long argc,
                                   TaggedData *retval) {
  try_catch([&retval]() {
    auto iohook_status =
        WaitForFuture(S_iohook_future, std::chrono::seconds(0));

    if (iohook_status) {
      auto [error_str, insructions_str] =
          GetHookErrorStrings(iohook_status.value());
      std::string str = error_str + "\n" + insructions_str;

      retval->type = kTypeString;
      retval->data.string = getNewBuffer(str);
    }
  });

  return kESErrOK;
}

/* This function is called from the script panel */
extern "C" DllExport long reload(TaggedData *argv, long argc,
                                 TaggedData *retval) {
  try_catch([&retval]() {
    // Disable logging because we want to pass all errors to the script
    S_logging_enabled = false;

    auto iohook_status =
        WaitForFuture(S_iohook_future, std::chrono::seconds(0));

    // if the status exists that means that the hook has ended and we can start
    // it again
    if (iohook_status) {
      StartIOHook();

      // wait for zoom status
      auto zoom_status =
          WaitForFuture(S_zoom_status_future, std::chrono::seconds(5));

      FillZoomStatusForScript(retval);
    }

    S_logging_enabled = true;
  });

  return kESErrOK;
}

/* This function is called from the script panel */
extern "C" DllExport long updateKeyBindings(TaggedData *argv, long argc,
                                            TaggedData *retval) {
  try_catch([&retval]() { ReadKeyBindings(); });

  return kESErrOK;
}

/* This function is called from the script panel */
extern "C" DllExport long updateExperimentalOptions(TaggedData *argv, long argc,
                                                    TaggedData *retval) {
  try_catch([&retval]() { ReadExperimentalOptions(); });

  return kESErrOK;
}

/* This function is called from the script panel */
extern "C" DllExport long updateHighDpiOptions(TaggedData *argv, long argc,
                                               TaggedData *retval) {
  try_catch([&retval]() { ReadHighDpiOptions(); });

  return kESErrOK;
}

/* This function is called from the script panel */
extern "C" DllExport long startKeyCapture(TaggedData *argv, long argc,
                                          TaggedData *retval) {
  try_catch([&retval]() {
    S_is_creating_key_bind = true;
    S_key_codes_pass.reset();
  });

  return kESErrOK;
}

/* This function is called from the script panel */
extern "C" DllExport long endKeyCapture(TaggedData *argv, long argc,
                                        TaggedData *retval) {
  try_catch([&retval]() {
    S_is_creating_key_bind = false;
    S_key_codes_pass.reset();
  });

  return kESErrOK;
}

/* This function is called from the script panel */
extern "C" DllExport long postZoomAction(TaggedData *argv, long argc,
                                         TaggedData *retval) {
  retval->type = kTypeBool;
  retval->data.intval = false;

  // Return an error if we do not get what we expect
  if (argc < 2 || argv[0].type != kTypeInteger || argv[1].type != kTypeDouble) {
    return kESErrBadArgumentList;
  }

  try_catch([&retval, &argv]() {
    KeyBindAction kbind;
    kbind.action = KB_ACTION(argv[0].data.intval);
    kbind.amount = argv[1].data.fltval;

    S_zoom_actions.post(kbind);
    S_call_idle_routines();

    retval->data.intval = true;
  });

  return kESErrOK;
}
