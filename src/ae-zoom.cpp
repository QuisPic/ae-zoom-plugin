#include "ae-zoom.h"

#ifdef AE_OS_MAC
#include <objc/objc-runtime.h>
#endif

/* Macros for reading a file to a string at compile time */
#define STRINGIFY(...) #__VA_ARGS__
#define STR(...) STRINGIFY(__VA_ARGS__)

static AEGP_Command							S_zoom_cmd = 0L;
static AEGP_Command							S_init_js_bridge_cmd = 0L;
static AEGP_Command							S_start_key_capture_cmd = 0L;
static AEGP_Command							S_stop_key_capture_cmd = 0L;
static AEGP_Command							S_update_key_binds_cmd = 0L;

static AEGP_PluginID						S_zoom_id = 0L;
static SPBasicSuite*						sP = NULL;

static A_Err (*S_call_idle_routines)(void) = nullptr;

static float								S_zoom_delta = 0.0f;
static bool									S_is_creating_key_bind = false;
static std::optional<KeyCodes>				S_key_codes_pass;
static std::vector<KeyBindAction>			S_key_bindings;

#ifdef AE_OS_WIN
static HWND									S_main_win_h = nullptr;
#elif defined AE_OS_MAC
static int									S_main_win_id = -1;
#endif

static std::vector<LogMessage>				log_messages;
static std::vector<KeyBindAction*>			S_zoom_actions;

static std::thread							S_mouse_events_thread;
static std::mutex							S_create_key_bind_mutex;
static std::mutex							S_zoom_action_mutex;
static std::mutex							S_log_mutex;

static std::string zoom_increment_js = {
	#include "JS/zoom-increment.js"
};

static std::string zoom_set_to_js = {
	#include "JS/zoom-set-to.js"
};

static std::string save_cmd_ids_js = {
	#include "JS/save-cmd-ids.js"
};

static std::string pass_key_bind_js = {
	#include "JS/pass-key-bind.js"
};

static void logger_proc(unsigned int level, void *user_data, const char *format, va_list args) {
    switch (level) {
        case LOG_LEVEL_INFO:
            break;

        case LOG_LEVEL_WARN:
        case LOG_LEVEL_ERROR:
			std::lock_guard lock(S_log_mutex);
			log_messages.push_back({ level, format });
            break;
    }
}

static void logger(unsigned int level, const char *format, ...) {
    va_list args;

    va_start(args, format);
    logger_proc(level, NULL, format, args);
    va_end(args);
}

static A_Err ReadKeyBindings()
{
	A_Err err = A_Err_NONE;
	AEGP_SuiteHandler	suites(sP);
	AEGP_PersistentBlobH blobH;
	A_char key_bindings_buf[2000];

	if (S_key_bindings.size() > 0)
	{
		S_key_bindings.clear();
	}

	ERR(suites.PersistentDataSuite4()->AEGP_GetApplicationBlob(AEGP_PersistentType_MACHINE_SPECIFIC, &blobH));

	if (blobH)
	{
		ERR(suites.PersistentDataSuite4()->AEGP_GetString(
			blobH,
			SETTINGS_SECTION_NAME,
			"keyBindings",
			nullptr,
			2000,
			key_bindings_buf,
			nullptr
		));

		json key_bindings_json = json::parse(std::string(key_bindings_buf));

		for (const auto& kbind_j : key_bindings_json) {
			if (!kbind_j["enabled"])
			{
				continue;
			}

			S_key_bindings.emplace_back(kbind_j.get<KeyBindAction>());
		}
	}

	return err;
}

#ifdef AE_OS_MAC
void GetMainMacWindowId()
{
    // Get the list of all windows
    CFArrayRef windowList = CGWindowListCopyWindowInfo(kCGWindowListOptionOnScreenOnly, kCGNullWindowID);

    // Get the PID of the current process
    int pid = getpid();

    // Iterate through the windows to find the one that belongs to the current process
    for (int i = 0; i < CFArrayGetCount(windowList); i++) {
        CFDictionaryRef window = (CFDictionaryRef)CFArrayGetValueAtIndex(windowList, i);
        CFNumberRef windowPID = (CFNumberRef)CFDictionaryGetValue(window, kCGWindowOwnerPID);

        int windowPIDValue;
        CFNumberGetValue(windowPID, kCFNumberIntType, &windowPIDValue);

        if (windowPIDValue == pid) {
            CFNumberRef winID = (CFNumberRef)CFDictionaryGetValue(window, kCGWindowNumber);
            CFNumberGetValue(winID, kCFNumberIntType, &S_main_win_id);
            break;
        }
    }

    CFRelease(windowList);
}
#endif

bool isMainWindowActive()
{
#ifdef AE_OS_WIN
	return S_main_win_h == GetForegroundWindow();
#elif defined AE_OS_MAC
    if (S_main_win_id == -1)
    {
        GetMainMacWindowId();
    }

	CFArrayRef windowArray = CGWindowListCopyWindowInfo(
		kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements, 
		kCGNullWindowID
	);

	CFIndex i, n;
	for (i = 0, n = CFArrayGetCount(windowArray); i < n; i++) {
		int layer, id;
		CFDictionaryRef windict = (CFDictionaryRef)CFArrayGetValueAtIndex(windowArray, i);
		CFNumberRef layernum = (CFNumberRef)CFDictionaryGetValue(windict, kCGWindowLayer);

		if (layernum) {
			CFNumberGetValue(layernum, kCFNumberIntType, &layer);

			if (layer == 0) {
				CFNumberRef winnum = (CFNumberRef)CFDictionaryGetValue(windict, kCGWindowNumber);
				CFNumberGetValue(winnum, kCFNumberIntType, &id);

				return id == S_main_win_id;
			}
		}
	}

	return false;
#endif
}

bool IsCursorOnMainWindow() {
    bool isOverWindow = false;
#ifdef AE_OS_MAC
    if (S_main_win_id == -1)
    {
        GetMainMacWindowId();
    }
    
    // Get the window's frame
    CFArrayRef CFWinArray = CGWindowListCopyWindowInfo(kCGWindowListOptionIncludingWindow, S_main_win_id);
    
    if (CFArrayGetCount(CFWinArray) > 0)
    {
        CFDictionaryRef main_win = (CFDictionaryRef)CFArrayGetValueAtIndex(CFWinArray, 0);
        CFDictionaryRef bounds = (CFDictionaryRef)CFDictionaryGetValue(main_win, kCGWindowBounds);
        CGRect windowFrame;
        CGRectMakeWithDictionaryRepresentation(bounds, &windowFrame);
        
        // Get the cursor position
        CGEventRef event = CGEventCreate(NULL);
        CGPoint cursorPosition = CGEventGetLocation(event);
        CFRelease(event);

        // Check if the cursor is over the window's frame
        isOverWindow = CGRectContainsPoint(windowFrame, cursorPosition);
    }
    
    CFRelease(CFWinArray);
#elif defined AE_OS_WIN
	static auto isPointInsideWindow = [](HWND hwnd, POINT cursor_p, WIND_RECT rect_type) -> bool
	{
		RECT win_rect = { 0, 0, 0, 0 };

		if (rect_type == WIND_RECT::FULL)
		{
			GetWindowRect(hwnd, &win_rect); // Get the area of the window
		}
		else if (rect_type == WIND_RECT::CLIENT)
		{
			ScreenToClient(hwnd, &cursor_p); // Convert the screen coordinates to client coordinates relative to the window
			GetClientRect(hwnd, &win_rect); // Get the client area of the window
		}

		return PtInRect(&win_rect, cursor_p); // Check if the cursor is inside the area
	};

    POINT cursor_pos;
    GetCursorPos(&cursor_pos); // Get the cursor position in screen coordinates

	/* Check if cursor is inside AE main window */
	isOverWindow = isPointInsideWindow(S_main_win_h, cursor_pos, WIND_RECT::CLIENT);

	/* If cursor is inside we need to check if it's over any window in front the main window*/
	if (isOverWindow)
	{
		DWORD main_process_id;
		GetWindowThreadProcessId(S_main_win_h, &main_process_id);
		HWND front_hwnd = GetWindow(S_main_win_h, GW_HWNDPREV);

		while (front_hwnd)
		{
			DWORD front_process_id;
			GetWindowThreadProcessId(front_hwnd, &front_process_id);

			if (IsWindowVisible(front_hwnd) && 
				main_process_id != front_process_id &&
				isPointInsideWindow(front_hwnd, cursor_pos, WIND_RECT::FULL))
			{
				isOverWindow = false;
				break;
			}

			front_hwnd = GetWindow(front_hwnd, GW_HWNDPREV);
		}
	}
#endif
    
    return isOverWindow;
}

void dispatch_proc(uiohook_event * const event, void *user_data) {
	static uint16_t last_key_code = VC_UNDEFINED;

	if (
		event->type == EVENT_KEY_PRESSED ||
		event->type == EVENT_MOUSE_WHEEL ||
		event->type == EVENT_MOUSE_PRESSED
	)
	{
		if (S_is_creating_key_bind)
		{
			if (event->type == EVENT_KEY_PRESSED)
			{
				if (event->data.keyboard.keycode == VC_ESCAPE)
				{
					// do not process Escape because Escape must close Key Capture window
					return;
				}
				else if (last_key_code == event->data.keyboard.keycode)
				{
					return;
				}
				else
				{
					last_key_code = event->data.keyboard.keycode;
				}
			}

			const std::lock_guard lock(S_create_key_bind_mutex);
			S_key_codes_pass.emplace(event);

			// event->reserved = 0x1; // stop event propagation

			S_call_idle_routines();
		}
		else if ((event->type == EVENT_KEY_PRESSED && isMainWindowActive()) ||
				((event->type == EVENT_MOUSE_PRESSED || 
				event->type == EVENT_MOUSE_WHEEL) && IsCursorOnMainWindow()))
		{
			KeyCodes current_key_codes(event);
			bool key_bind_found = false;

			for (KeyBindAction& kbind : S_key_bindings)
			{
				if (current_key_codes == kbind.keyCodes)
				{
					std::lock_guard lock(S_zoom_action_mutex);
					S_zoom_actions.push_back(&kbind);

					key_bind_found = true;
				}
			}

			if (key_bind_found)
			{
				bool isType = event->type == EVENT_MOUSE_PRESSED;
				bool isMask =	event->mask & (MASK_CTRL) ||
								event->mask & (MASK_META) ||
								event->mask & (MASK_SHIFT) ||
								event->mask & (MASK_ALT);
				bool isButton = event->data.mouse.button == 1 || event->data.mouse.button == 2;

				/* Do not stop event propagation on pure mouse clicks */
				if (!(isType && !isMask && isButton))
				{
					event->reserved = 0x1; // stop event propagation
				}
			}
		}
	}
	else if (
		S_is_creating_key_bind &&
		event->type == EVENT_KEY_RELEASED && 
		event->data.keyboard.keycode == last_key_code
	)
	{
		last_key_code = VC_UNDEFINED;
	}
}

int HookProc() {
    // Start the hook and block.
    // NOTE If EVENT_HOOK_ENABLED was delivered, the status will always succeed.
    int status = hook_run();

    switch (status) {
        case UIOHOOK_SUCCESS:
            // Everything is ok.
            break;

        // System level errors.
        case UIOHOOK_ERROR_OUT_OF_MEMORY:
            logger(LOG_LEVEL_ERROR, "Failed to allocate memory. (%#X)", status);
            break;


        // X11 specific errors.
        case UIOHOOK_ERROR_X_OPEN_DISPLAY:
            logger(LOG_LEVEL_ERROR, "Failed to open X11 display. (%#X)", status);
            break;

        case UIOHOOK_ERROR_X_RECORD_NOT_FOUND:
            logger(LOG_LEVEL_ERROR, "Unable to locate XRecord extension. (%#X)", status);
            break;

        case UIOHOOK_ERROR_X_RECORD_ALLOC_RANGE:
            logger(LOG_LEVEL_ERROR, "Unable to allocate XRecord range. (%#X)", status);
            break;

        case UIOHOOK_ERROR_X_RECORD_CREATE_CONTEXT:
            logger(LOG_LEVEL_ERROR, "Unable to allocate XRecord context. (%#X)", status);
            break;

        case UIOHOOK_ERROR_X_RECORD_ENABLE_CONTEXT:
            logger(LOG_LEVEL_ERROR, "Failed to enable XRecord context. (%#X)", status);
            break;

            
        // Windows specific errors.
        case UIOHOOK_ERROR_SET_WINDOWS_HOOK_EX:
            logger(LOG_LEVEL_ERROR, "Failed to register low level windows hook. (%#X)", status);
            break;


        // Darwin specific errors.
        case UIOHOOK_ERROR_AXAPI_DISABLED:
            logger(LOG_LEVEL_ERROR, "Failed to enable access for assistive devices. (%#X)", status);
            break;

        case UIOHOOK_ERROR_CREATE_EVENT_PORT:
            logger(LOG_LEVEL_ERROR, "Failed to create apple event port. (%#X)", status);
            break;

        case UIOHOOK_ERROR_CREATE_RUN_LOOP_SOURCE:
            logger(LOG_LEVEL_ERROR, "Failed to create apple run loop source. (%#X)", status);
            break;

        case UIOHOOK_ERROR_GET_RUNLOOP:
            logger(LOG_LEVEL_ERROR, "Failed to acquire apple run loop. (%#X)", status);
            break;

        case UIOHOOK_ERROR_CREATE_OBSERVER:
            logger(LOG_LEVEL_ERROR, "Failed to create apple run loop observer. (%#X)", status);
            break;

        // Default error.
        case UIOHOOK_FAILURE:
        default:
            logger(LOG_LEVEL_ERROR, "An unknown hook error occurred. (%#X)", status);
            break;
    }

    return status;
}

static	A_Err	IdleHook(
	AEGP_GlobalRefcon	plugin_refconP,
	AEGP_IdleRefcon		refconP,
	A_long* max_sleepPL)
{
	A_Err err = A_Err_NONE;

	if (log_messages.size() > 0)
	{
		AEGP_SuiteHandler	suites(sP);

		for (auto it = log_messages.begin(); it != log_messages.end();)
		{
			ERR(suites.UtilitySuite3()->AEGP_ReportInfo(S_zoom_id, it->format));

			std::lock_guard lock(S_log_mutex);
			it = log_messages.erase(log_messages.begin());
		}
	}

	if (S_is_creating_key_bind && S_key_codes_pass) {
		AEGP_SuiteHandler	suites(sP);
		KeyCodes& keyCodes = S_key_codes_pass.value();

		std::string pass_fn_str =
			pass_key_bind_js +
			"(" +
			std::to_string(keyCodes.type) +
			"," +
			std::to_string(keyCodes.mask) +
			"," +
			std::to_string(keyCodes.keycode) +
			")";

		ERR(suites.UtilitySuite6()->AEGP_ExecuteScript(
			S_zoom_id,
			pass_fn_str.c_str(),
			false,
			nullptr,
			nullptr
		));

		const std::lock_guard lock(S_create_key_bind_mutex);
		S_key_codes_pass.reset();
	}

	if (S_zoom_actions.size() > 0) 
	{
		AEGP_SuiteHandler	suites(sP);
		std::string script_str;

		const std::lock_guard lock(S_zoom_action_mutex);

		for (const auto act : S_zoom_actions)
		{
			switch (act->action)
			{
			case KB_ACTION::INCREASE:
				script_str = zoom_increment_js + "(" + std::to_string(act->amount) + ")";
				break;
			case KB_ACTION::DECREASE:
				script_str = zoom_increment_js + "(" + std::to_string(-act->amount) + ")";
				break;
			case KB_ACTION::SET_TO:
				script_str = zoom_set_to_js + "(" + std::to_string(act->amount) + ")";
				break;
			default:
				break;
			}

			ERR(suites.UtilitySuite6()->AEGP_ExecuteScript(
				S_zoom_id,
				script_str.c_str(),
				false,
				nullptr,
				nullptr
			));
		}

		S_zoom_actions.clear();
	}

	return err;
}

static A_Err UpdateMenuHook(
	AEGP_GlobalRefcon		plugin_refconPV,	/* >> */
	AEGP_UpdateMenuRefcon	refconPV,			/* >> */
	AEGP_WindowType			active_window)		/* >> */
{
	A_Err 				err = A_Err_NONE;
	AEGP_SuiteHandler	suites(sP);

	ERR(suites.CommandSuite1()->AEGP_EnableCommand(S_zoom_cmd));
	ERR(suites.CommandSuite1()->AEGP_EnableCommand(S_init_js_bridge_cmd));
	ERR(suites.CommandSuite1()->AEGP_EnableCommand(S_start_key_capture_cmd));
	ERR(suites.CommandSuite1()->AEGP_EnableCommand(S_stop_key_capture_cmd));
	ERR(suites.CommandSuite1()->AEGP_EnableCommand(S_update_key_binds_cmd));

	return err;
}

static A_Err CommandHook(
	AEGP_GlobalRefcon	plugin_refconPV,		/* >> */
	AEGP_CommandRefcon	refconPV,				/* >> */
	AEGP_Command		command,				/* >> */
	AEGP_HookPriority	hook_priority,			/* >> */
	A_Boolean			already_handledB,		/* >> */
	A_Boolean*			handledPB)				/* << */
{
	A_Err err = A_Err_NONE;
	AEGP_SuiteHandler	suites(sP);

	if (already_handledB)
	{
		return err;
	}

	try {
		if (S_zoom_cmd == command) {
//			HackBipBop();
			*handledPB = TRUE;
		}
		else if (S_init_js_bridge_cmd == command)
		{
			std::string script_str = 
				save_cmd_ids_js + 
				"(" + 
				std::to_string(S_start_key_capture_cmd) + 
				"," +
				std::to_string(S_stop_key_capture_cmd) + 
				"," +
				std::to_string(S_update_key_binds_cmd) + 
				")";

			suites.UtilitySuite6()->AEGP_ExecuteScript(
				S_zoom_id,
				script_str.c_str(),
				false,
				nullptr,
				nullptr
			);

			*handledPB = TRUE;
		}
		else if (S_start_key_capture_cmd == command)
		{
			S_is_creating_key_bind = true;
			*handledPB = true;
		}
		else if (S_stop_key_capture_cmd == command)
		{
			S_is_creating_key_bind = false;
			*handledPB = true;
		}
		else if (S_update_key_binds_cmd == command)
		{
			ReadKeyBindings();
			*handledPB = true;
		}
	}
	catch (A_Err& thrown_err) 
	{
		err = thrown_err;
	}

	return err;
}


static A_Err DeathHook(AEGP_GlobalRefcon plugin_refconP, AEGP_DeathRefcon refconP)
{
	int status = hook_stop();
	switch (status) {
		case UIOHOOK_SUCCESS:
			// Everything is ok.
			break;

		// System level errors.
		case UIOHOOK_ERROR_OUT_OF_MEMORY:
			logger(LOG_LEVEL_ERROR, "Failed to allocate memory. (%#X)", status);
			break;

		case UIOHOOK_ERROR_X_RECORD_GET_CONTEXT:
			// NOTE This is the only platform specific error that occurs on hook_stop().
			logger(LOG_LEVEL_ERROR, "Failed to get XRecord context. (%#X)", status);
			break;

		// Default error.
		case UIOHOOK_FAILURE:
		default:
			logger(LOG_LEVEL_ERROR, "An unknown hook error occurred. (%#X)", status);
			break;
	}

	if (S_mouse_events_thread.joinable())
	{
		S_mouse_events_thread.join();
	}

	return A_Err_NONE;
}
 
A_Err EntryPointFunc(
	struct SPBasicSuite		*pica_basicP,		/* >> */
	A_long				 	major_versionL,		/* >> */		
	A_long					minor_versionL,		/* >> */		
	AEGP_PluginID			aegp_plugin_id,		/* >> */
	AEGP_GlobalRefcon		*global_refconP)	/* << */
{
	sP = pica_basicP;
	S_zoom_id = aegp_plugin_id;
	AEGP_SuiteHandler suites(sP);
	S_call_idle_routines = suites.UtilitySuite6()->AEGP_CauseIdleRoutinesToBeCalled;

	A_Err err = A_Err_NONE;
	A_Err err2 = A_Err_NONE;
    
	ERR(suites.CommandSuite1()->AEGP_GetUniqueCommand(&S_zoom_cmd));
	ERR(suites.CommandSuite1()->AEGP_GetUniqueCommand(&S_init_js_bridge_cmd));
	ERR(suites.CommandSuite1()->AEGP_GetUniqueCommand(&S_start_key_capture_cmd));
	ERR(suites.CommandSuite1()->AEGP_GetUniqueCommand(&S_update_key_binds_cmd));

	ERR(suites.CommandSuite1()->AEGP_InsertMenuCommand(S_zoom_cmd, "Hack Bip Bop", AEGP_Menu_ANIMATION, AEGP_MENU_INSERT_AT_BOTTOM));
	ERR(suites.CommandSuite1()->AEGP_InsertMenuCommand(S_init_js_bridge_cmd, "__zoom_save_cmd_ids__", AEGP_Menu_NONE, AEGP_MENU_INSERT_SORTED));
	ERR(suites.CommandSuite1()->AEGP_InsertMenuCommand(S_start_key_capture_cmd, "__zoom_start_key_capture__", AEGP_Menu_NONE, AEGP_MENU_INSERT_SORTED));
	ERR(suites.CommandSuite1()->AEGP_InsertMenuCommand(S_stop_key_capture_cmd, "__zoom_stop_key_capture__", AEGP_Menu_NONE, AEGP_MENU_INSERT_SORTED));
	ERR(suites.CommandSuite1()->AEGP_InsertMenuCommand(S_update_key_binds_cmd, "__zoom_update_key_binds__", AEGP_Menu_NONE, AEGP_MENU_INSERT_SORTED));

	ERR(suites.RegisterSuite5()->AEGP_RegisterCommandHook(S_zoom_id, AEGP_HP_BeforeAE, AEGP_Command_ALL, CommandHook, 0));
	ERR(suites.RegisterSuite5()->AEGP_RegisterUpdateMenuHook(S_zoom_id, UpdateMenuHook, 0));
	ERR(suites.RegisterSuite5()->AEGP_RegisterIdleHook(S_zoom_id, IdleHook, 0));
	ERR(suites.RegisterSuite5()->AEGP_RegisterDeathHook(S_zoom_id, DeathHook, 0));

#ifdef AE_OS_WIN
	suites.UtilitySuite6()->AEGP_GetMainHWND(&S_main_win_h);
#endif

	ReadKeyBindings();

    // Set the event callback for uiohook events.
    hook_set_dispatch_proc(&dispatch_proc, NULL);

	// Start hook thread
	S_mouse_events_thread = std::thread(HookProc);

	return err;
}
