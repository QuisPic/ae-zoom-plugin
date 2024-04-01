#include "ae-zoom.h"
#include "AEGP_SuiteHandler.h"
#include "AE_Macros.h"
#include "ExternalObject/SoSharedLibDefs.h"
#include "JS/JS.h"
#include "ae-egg.h"
#include "iohook.h"
#include "options.h"
#include "util-functions.h"
#include <cmath>
#include <exception>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#ifdef AE_OS_MAC
#include <objc/objc-runtime.h>
#endif

static AEGP_PluginID S_zoom_id = 0L;
static SPBasicSuite *sP = NULL;
static bool S_is_creating_key_bind = false;
static bool S_logging_enabled = true;
static std::optional<ZOOM_STATUS> S_zoom_status;
static int S_iohook_status = UIOHOOK_FAILURE;
static std::optional<KeyCodes> S_key_codes_pass;
static std::vector<LogMessage> log_messages;

// static AEGP_Command test_cmd;

#ifdef AE_OS_WIN
static HWND S_main_win_h = nullptr;
#endif

/**
 * \brief Utility function to handle strings and memory clean up
 *
 * \param s - The referenced string
 */
static char *getNewBuffer(const std::string &s) {
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

static void dispatchKeyBindAction(const KeyBindAction &act,
                                  iohook_event *const event) {
  double zoomValue = act.getAmount(gHighDpiOptions);

  if (event->type == EVENT_MOUSE_WHEEL && event->data.wheel.precise) {
    zoomValue *= std::abs(event->data.wheel.rotation);
  }

  std::optional<ViewPano> view_pano;
  if (gExperimentalOptions.detectCursorInsideView &&
      (act.keyCodes.type == EVENT_MOUSE_PRESSED ||
       act.keyCodes.type == EVENT_MOUSE_WHEEL)) {
    view_pano = gAeEgg.getViewPanoUnderCursor();

    if (!view_pano) {
      return;
    }
  }

  if (gExperimentalOptions.fixViewportPosition.enabled) {
    if (!view_pano) {
      view_pano = gAeEgg.getActiveViewPano();
    }

    if (view_pano) {
      ZOOM_AROUND zoom_around = ZOOM_AROUND::PANEL_CENTER;

      switch (act.action) {
      case KB_ACTION::DECREMENT: // support deprecated action
        zoomValue = -zoomValue;
      case KB_ACTION::CHANGE: {
        if (gExperimentalOptions.fixViewportPosition.zoomAround ==
                ZOOM_AROUND::CURSOR_POSTION &&
            act.keyCodes.type == EVENT_MOUSE_WHEEL) {
          zoom_around = ZOOM_AROUND::CURSOR_POSTION;
        }

        auto current_zoom = view_pano->getZoom();
        view_pano->setZoomFixed(current_zoom + zoomValue / 100.0, zoom_around);
        break;
      }
      case KB_ACTION::SET_TO: {
        view_pano->setZoomFixed(zoomValue / 100.0, zoom_around);
        break;
      }
      default:
        break;
      }
    }
  } else {
    std::string script_str;

    switch (act.action) {
    case KB_ACTION::DECREMENT: // support deprecated action
      zoomValue = -zoomValue;
    case KB_ACTION::CHANGE:
      script_str = zoom_increment_js + "(" + std::to_string(zoomValue) + ")";
      break;
    case KB_ACTION::SET_TO:
      script_str = zoom_set_to_js + "(" + std::to_string(zoomValue) + ")";
      break;
    default:
      break;
    }

    RunExtendscript(script_str);
  }
}

void dispatch_proc(iohook_event *const event, void *user_data) {
  if (event->type == EVENT_HOOK_ENABLED) {
    S_zoom_status = ZOOM_STATUS::INITIALIZED;
  } else if (event->type == EVENT_HOOK_DISABLED) {
    S_zoom_status = ZOOM_STATUS::FINISHED;
  } else if (S_is_creating_key_bind) {
    // stop event propagation for Escape because it must close the Key
    // Capture window and it may accidentally stop extendscript execution
    // if passed to AE
    if ((event->type == EVENT_KEY_PRESSED ||
         event->type == EVENT_KEY_RELEASED) &&
        event->data.keyboard.keycode == VC_ESCAPE) {
      event->reserved = 0x1;
    }

    /** no need to pass repeated events when creating key bind */
    if (event->type == EVENT_KEY_PRESSED && event->data.keyboard.repeat) {
      return;
    }

    if (event->type == EVENT_KEY_PRESSED ||
        (event->type == EVENT_KEY_RELEASED &&
         event->data.keyboard.keycode == VC_ESCAPE) ||
        event->type == EVENT_MOUSE_PRESSED ||
        event->type == EVENT_MOUSE_WHEEL) {
      KeyCodes keyCodes(event);
      std::string pass_fn_script = pass_key_bind_js + "(" +
                                   std::to_string(keyCodes.type) + "," +
                                   std::to_string(keyCodes.mask) + "," +
                                   std::to_string(keyCodes.keycode) + ")";

      /** pass key codes to the script */
      RunExtendscript(pass_fn_script);
    }
  } else if (((event->type == EVENT_KEY_PRESSED) ||
              ((event->type == EVENT_MOUSE_PRESSED ||
                event->type == EVENT_MOUSE_WHEEL)))) {
    KeyCodes current_key_codes(event);

    for (const KeyBindAction &kbind : gKeyBindings) {
      if (!kbind.enabled) {
        continue;
      }

      if (current_key_codes == kbind.keyCodes) {
        /** dispatch this keybind's action */
        dispatchKeyBindAction(kbind, event);

        // stop event propagation on all AE's default key bindings that change
        // viewport zoom
        if ((event->type == EVENT_MOUSE_WHEEL ||
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

  // Windows specific errors.
  case UIOHOOK_ERROR_SET_WINDOWS_HOOK_EX:
    result = {"Failed to register windows hook.", ""};
    break;
  case UIOHOOK_ERROR_UNHOOK_WINDOWS_HOOK_EX:
    result = {"Failed to unhook windows hook.", ""};
    break;

  // Darwin specific errors.
  case UIOHOOK_ERROR_CREATE_EVENT_MONITOR:
    result = {"Failed to create local events monitor.", ""};
    break;

  // Default error.
  case UIOHOOK_FAILURE:
  default:
    result = {"An unknown hook error occurred.", ""};
    break;
  }

  return result;
}

static int StartIOHook() {
  S_zoom_status.reset();

  // Set the event callback for uiohook events.
  iohook_set_dispatch_proc(&dispatch_proc, nullptr);

/** create input hook */
#ifdef AE_OS_WIN
  S_iohook_status = iohook_run(S_main_win_h);
#elif defined AE_OS_MAC
  S_iohook_status = iohook_run();
#endif

  if (S_iohook_status != UIOHOOK_SUCCESS) {
    S_zoom_status = ZOOM_STATUS::INITIALIZATION_ERROR;
    logger(LOG_LEVEL_ERROR, GetHookErrorStrings(S_iohook_status));
  }

  return S_iohook_status;
}

static int StopIOHook() {
  if (S_zoom_status != ZOOM_STATUS::FINISHED) {
    S_iohook_status = iohook_stop();
  }

  return S_iohook_status;
}

static A_Err IdleHook(AEGP_GlobalRefcon plugin_refconP, AEGP_IdleRefcon refconP,
                      A_long *max_sleepPL) {
  A_Err err = A_Err_NONE;

  static bool isExtendscriptFlagSet = false;
  static AEGP_SuiteHandler suites(sP);

  /** Tell the script that the plug-in is loaded into Ae */
  if (!isExtendscriptFlagSet) {
    RunExtendscript("$.global.__quis_zoom_plugin_is_loaded = true;");
    isExtendscriptFlagSet = true;
  }

  /* Displaying errors and warnings */
  if (log_messages.size() > 0) {
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

  return err;
}

// static A_Err CommandHook(AEGP_GlobalRefcon plugin_refconPV, /* >> */
//                          AEGP_CommandRefcon refconPV,       /* >> */
//                          AEGP_Command command,              /* >> */
//                          AEGP_HookPriority hook_priority,   /* >> */
//                          A_Boolean already_handledB,        /* >> */
//                          A_Boolean *handledPB)              /* << */
// {
//   A_Err err = A_Err_NONE;
//
//   if (already_handledB) {
//     return err;
//   }
//
//   if (command == test_cmd) {
//     AEGP_SuiteHandler suites(sP);
//
//     // suites.UtilitySuite3()->AEGP_ReportInfo(S_zoom_id, msg_str.c_str());
//     *handledPB = true;
//   }
//
//   return err;
// }
//
// static A_Err UpdateMenuHook(AEGP_GlobalRefcon plugin_refconPV, /* >> */
//                             AEGP_UpdateMenuRefcon refconPV,    /* >> */
//                             AEGP_WindowType active_window)     /* >> */
// {
//   A_Err err = A_Err_NONE;
//   AEGP_SuiteHandler suites(sP);
//
//   ERR(suites.CommandSuite1()->AEGP_EnableCommand(test_cmd));
//
//   return err;
// }

static A_Err DeathHook(AEGP_GlobalRefcon plugin_refconP,
                       AEGP_DeathRefcon refconP) {
  int status = StopIOHook();

  if (status != UIOHOOK_SUCCESS) {
    logger(LOG_LEVEL_ERROR, GetHookErrorStrings(status));
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

    // ERR(suites.CommandSuite1()->AEGP_GetUniqueCommand(&test_cmd));
    // ERR(suites.CommandSuite1()->AEGP_InsertMenuCommand(
    //     test_cmd, "Test Bip Bop", AEGP_Menu_ANIMATION,
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

    // ERR(suites.UtilitySuite3()->AEGP_ReportInfo(S_zoom_id, buf));

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
extern "C" DllExport const char *ESInitialize(const TaggedData **argv,
                                              long argc) {
  // return nullptr;
  return "postZoomAction_df";
}

/**
 * \brief Terminate the library.
 *
 * Does any necessary clean up that is needed.
 */
extern "C" DllExport void ESTerminate() {}

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
  if (S_zoom_status) {
    retval->type = kTypeInteger;
    retval->data.intval = static_cast<long>(S_zoom_status.value());
  }
}

extern "C" DllExport bool quis_zoom_plugin = true;

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
    auto [error_str, insructions_str] = GetHookErrorStrings(S_iohook_status);
    std::string str = error_str + "\n" + insructions_str;

    retval->type = kTypeString;
    retval->data.string = getNewBuffer(str);
  });

  return kESErrOK;
}

/* This function is called from the script panel */
extern "C" DllExport long reload(TaggedData *argv, long argc,
                                 TaggedData *retval) {
  try_catch([&retval]() {
    /** Disable logging because we want to pass all errors to the script instead
     * of creating alerts */
    S_logging_enabled = false;
    int status = StopIOHook();

    // if iohook removed successfully
    if (status == UIOHOOK_SUCCESS) {
      StartIOHook();
      FillZoomStatusForScript(retval);
    }

    ReadKeyBindings();
    ReadExperimentalOptions();
    ReadHighDpiOptions();

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

    iohook_event e;
    e.type = EVENT_KEY_PRESSED;
    dispatchKeyBindAction(kbind, &e);

    retval->data.intval = true;
  });

  return kESErrOK;
}

/* This function is called from the script panel */
extern "C" DllExport long getVersion(TaggedData *argv, long argc,
                                     TaggedData *retval) {
  try_catch([&retval]() {
    retval->type = kTypeString;
    retval->data.string = getNewBuffer(VERSION);
  });

  return kESErrOK;
}
