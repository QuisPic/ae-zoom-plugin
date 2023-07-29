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
//static bool									S_js_bridge_created = false;
static std::optional<KeyCodes>				S_key_codes_pass;
static std::vector<KeyBindAction>			S_key_bindings;

#ifdef AE_OS_WIN
static HWND									S_main_win_h = nullptr;
#elif defined AE_OS_MAC
static int									S_main_win_id = -1;
#endif

static std::vector<LogMessage>				log_messages;
static std::vector<KeyBindAction*>			S_zoom_actions;

static std::thread							S_uiohook_thread;
static std::mutex							S_create_key_bind_mutex;
static std::mutex							S_zoom_action_mutex;
static std::mutex							S_log_mutex;

static std::string zoom_increment_js = {
	#include "JS/zoom-increment.js"
};

static std::string zoom_set_to_js = {
	#include "JS/zoom-set-to.js"
};

static std::string init_js_bridge_js = {
	#include "JS/init-js-bridge.js"
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

static void logger(unsigned int level, const std::string& format_str) {
    switch (level) {
        case LOG_LEVEL_INFO:
            break;

        case LOG_LEVEL_WARN:
        case LOG_LEVEL_ERROR:
			std::lock_guard lock(S_log_mutex);
			log_messages.emplace_back(level, format_str);
            break;
    }
}

bool IsMainWindowEnabled()
{
#ifdef AE_OS_WIN
	return IsWindowEnabled(S_main_win_h);
#elif defined AE_OS_MAC
	return true;
#endif
}

std::tuple<A_Err, std::string> RunExtendscript(const std::string& script_str)
{
	A_Err err = A_Err_NONE;

	/* After Effects can't execute script and displays an error if the main window is disabled */
	if (!IsMainWindowEnabled())
	{
		return { err, "" };
	}

	AEGP_SuiteHandler suites(sP);
	AEGP_MemHandle outResultH = nullptr;
	AEGP_MemHandle outErrStringH = nullptr;

	auto MemHandleToString = [&suites, &err](AEGP_MemHandle mH) -> std::string
	{
		void* memP = nullptr;

		ERR(suites.MemorySuite1()->AEGP_LockMemHandle(mH, &memP));
		std::string result_str = std::bit_cast<char*>(memP);
		ERR(suites.MemorySuite1()->AEGP_UnlockMemHandle(mH));

		return result_str;
	};

	ERR(suites.UtilitySuite6()->AEGP_ExecuteScript(
		S_zoom_id,
		script_str.c_str(),
		false,
		&outResultH,
		&outErrStringH
	));

	std::string result_str = MemHandleToString(outResultH);
	std::string error_str = MemHandleToString(outErrStringH);

	if (!error_str.empty())
	{
		logger(LOG_LEVEL_ERROR, error_str);
	}

	return { err, result_str };
}

//static A_Err InitJSBridge()
//{
//	A_Err err = A_Err_NONE;
//	AEGP_SuiteHandler	suites(sP);
//
//	std::string script_str = 
//		init_js_bridge_js + 
//		"(" + 
//		std::to_string(S_start_key_capture_cmd) + 
//		"," +
//		std::to_string(S_stop_key_capture_cmd) + 
//		"," +
//		std::to_string(S_update_key_binds_cmd) + 
//		")";
//
//	std::tie(err, std::ignore) = RunExtendscript(script_str);
//
//	if (!err)
//	{
//		S_js_bridge_created = true;
//	}
//
//	return err;
//}

static A_Err ReadKeyBindings()
{
	A_Err err = A_Err_NONE;
	AEGP_SuiteHandler	suites(sP);
	AEGP_PersistentBlobH blobH;
	A_char key_bindings_buf[2000];
	A_Boolean key_exists = false;

	if (S_key_bindings.size() > 0)
	{
		S_key_bindings.clear();
	}

	ERR(suites.PersistentDataSuite4()->AEGP_GetApplicationBlob(AEGP_PersistentType_MACHINE_SPECIFIC, &blobH));

	if (blobH)
	{
		ERR(suites.PersistentDataSuite4()->AEGP_DoesKeyExist(
			blobH,
			SETTINGS_SECTION_NAME,
			"keyBindings",
			&key_exists));

		if (key_exists)
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
	}

	return err;
}

static bool IsSameProcessId(
#ifdef AE_OS_WIN
	HWND hwnd0,
	HWND hwnd1
#elif defined AE_OS_MAC
	CFDictionaryRef winDict
#endif
)
{
#ifdef AE_OS_WIN
	DWORD process_id_0;
	GetWindowThreadProcessId(hwnd0, &process_id_0);
	DWORD process_id_1;
	GetWindowThreadProcessId(hwnd1, &process_id_1);

	return process_id_0 == process_id_1;
#elif defined AE_OS_MAC
	int pid = getpid(); // Get the PID of the current process

	CFNumberRef windowPID = (CFNumberRef)CFDictionaryGetValue(winDict, kCGWindowOwnerPID);
	int windowPIDValue;
	CFNumberGetValue(windowPID, kCFNumberIntType, &windowPIDValue);

	return windowPIDValue == pid;
#endif
}

#ifdef AE_OS_MAC
void GetMainMacWindowId()
{
    // Get the list of all windows
    CFArrayRef windowList = CGWindowListCopyWindowInfo(kCGWindowListOptionOnScreenOnly, kCGNullWindowID);

    // Iterate through the windows to find the one that belongs to the current process
    for (int i = 0; i < CFArrayGetCount(windowList); i++) {
        CFDictionaryRef winDict = (CFDictionaryRef)CFArrayGetValueAtIndex(windowList, i);

        if (IsSameProcessId(winDict)) {
            CFNumberRef winID = (CFNumberRef)CFDictionaryGetValue(winDict, kCGWindowNumber);
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
	return IsSameProcessId(S_main_win_h, GetForegroundWindow());
#elif defined AE_OS_MAC
    if (S_main_win_id == -1)
    {
        GetMainMacWindowId();
    }

	CFArrayRef windowArray = CGWindowListCopyWindowInfo(
		kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements, 
		kCGNullWindowID
	);

	for (CFIndex i = 0, n = CFArrayGetCount(windowArray); i < n; i++) {
		int layer, id;
		CFDictionaryRef windict = (CFDictionaryRef)CFArrayGetValueAtIndex(windowArray, i);
		CFNumberRef layernum = (CFNumberRef)CFDictionaryGetValue(windict, kCGWindowLayer);

		if (layernum) {
			CFNumberGetValue(layernum, kCFNumberIntType, &layer);

			if (layer == 0) {
				// CFNumberRef winnum = (CFNumberRef)CFDictionaryGetValue(windict, kCGWindowNumber);
				// CFNumberGetValue(winnum, kCFNumberIntType, &id);

				return IsSameProcessId(windict);
				// return id == S_main_win_id;
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

	// Get the cursor position
	CGEventRef event = CGEventCreate(NULL);
	CGPoint cursorPosition = CGEventGetLocation(event);
	CFRelease(event);
    
    CFArrayRef winList = CGWindowListCopyWindowInfo(
		kCGWindowListExcludeDesktopElements |
		kCGWindowListOptionIncludingWindow  |
		kCGWindowListOptionOnScreenAboveWindow,
		S_main_win_id);

	for (CFIndex i = 0, n = CFArrayGetCount(winList); i < n; i++) {
        CFDictionaryRef winDict = (CFDictionaryRef)CFArrayGetValueAtIndex(winList, i);
        CFDictionaryRef bounds = (CFDictionaryRef)CFDictionaryGetValue(winDict, kCGWindowBounds);
        CGRect windowFrame;
        CGRectMakeWithDictionaryRepresentation(bounds, &windowFrame);
        
        // Check if the cursor is over the window's frame
		if (CGRectContainsPoint(windowFrame, cursorPosition))
		{
			isOverWindow = IsSameProcessId(winDict);
			break;
		}
	}
    
    CFRelease(winList);
#elif defined AE_OS_WIN
    POINT cursor_pos;
    GetCursorPos(&cursor_pos); // Get the cursor position in screen coordinates

	isOverWindow = IsSameProcessId(S_main_win_h, WindowFromPoint(cursor_pos));
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
		if (S_is_creating_key_bind && isMainWindowActive())
		{
			if (event->type == EVENT_KEY_PRESSED)
			{
				if (event->data.keyboard.keycode == VC_ESCAPE)
				{
					// stop event propagation for Escape because it must close the Key Capture window
					// and it may accidentally stop extendscript execution
					event->reserved = 0x1;
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

			S_call_idle_routines();
		}
		else if (((event->type == EVENT_KEY_PRESSED && isMainWindowActive()) ||
				((event->type == EVENT_MOUSE_PRESSED || event->type == EVENT_MOUSE_WHEEL) && 
					IsCursorOnMainWindow())))
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

	/* Displaying errors and warnings from uiohook */
	if (log_messages.size() > 0)
	{
		AEGP_SuiteHandler	suites(sP);
		std::lock_guard lock(S_log_mutex);

		for (const auto& msg : log_messages)
		{
			ERR(suites.UtilitySuite3()->AEGP_ReportInfo(S_zoom_id, msg.format));
		}

		log_messages.clear();
	}

	///* Init JS bridge */
	//if (!S_js_bridge_created)
	//{
	//	AEGP_SuiteHandler	suites(sP);
	//	A_Boolean scripting_is_available = false;

	//	ERR(suites.UtilitySuite6()->AEGP_IsScriptingAvailable(&scripting_is_available));

	//	if (scripting_is_available)
	//	{
	//		ERR(InitJSBridge());
	//	}
	//}

	/* Passing key presses to the script panel */
	if (S_is_creating_key_bind && S_key_codes_pass) {
		AEGP_SuiteHandler	suites(sP);
		KeyCodes keyCodes = S_key_codes_pass.value();

		const std::lock_guard lock(S_create_key_bind_mutex);
		S_key_codes_pass.reset();

		std::string pass_fn_script =
			pass_key_bind_js +
			"(" +
			std::to_string(keyCodes.type) +
			"," +
			std::to_string(keyCodes.mask) +
			"," +
			std::to_string(keyCodes.keycode) +
			")";

		std::tie(err, std::ignore) = RunExtendscript(pass_fn_script);
	}

	/* Changing zoom on key presses */
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

			std::tie(err, std::ignore) = RunExtendscript(script_str);
		}

		S_zoom_actions.clear();
	}

	return err;
}

//static A_Err UpdateMenuHook(
//	AEGP_GlobalRefcon		plugin_refconPV,	/* >> */
//	AEGP_UpdateMenuRefcon	refconPV,			/* >> */
//	AEGP_WindowType			active_window)		/* >> */
//{
//	A_Err 				err = A_Err_NONE;
//	AEGP_SuiteHandler	suites(sP);
//
//	ERR(suites.CommandSuite1()->AEGP_EnableCommand(S_zoom_cmd));
//	ERR(suites.CommandSuite1()->AEGP_EnableCommand(S_init_js_bridge_cmd));
//	ERR(suites.CommandSuite1()->AEGP_EnableCommand(S_start_key_capture_cmd));
//	ERR(suites.CommandSuite1()->AEGP_EnableCommand(S_stop_key_capture_cmd));
//	ERR(suites.CommandSuite1()->AEGP_EnableCommand(S_update_key_binds_cmd));
//
//	return err;
//}
//
//static A_Err CommandHook(
//	AEGP_GlobalRefcon	plugin_refconPV,		/* >> */
//	AEGP_CommandRefcon	refconPV,				/* >> */
//	AEGP_Command		command,				/* >> */
//	AEGP_HookPriority	hook_priority,			/* >> */
//	A_Boolean			already_handledB,		/* >> */
//	A_Boolean*			handledPB)				/* << */
//{
//	A_Err err = A_Err_NONE;
//	AEGP_SuiteHandler	suites(sP);
//
//	if (already_handledB)
//	{
//		return err;
//	}
//
//	try {
//		if (S_zoom_cmd == command) {
////			HackBipBop();
//			*handledPB = TRUE;
//		}
//		else if (S_init_js_bridge_cmd == command)
//		{
//			ERR(InitJSBridge());
//			*handledPB = TRUE;
//		}
//		else if (S_start_key_capture_cmd == command)
//		{
//			S_is_creating_key_bind = true;
//			*handledPB = true;
//		}
//		else if (S_stop_key_capture_cmd == command)
//		{
//			S_is_creating_key_bind = false;
//			S_key_codes_pass.reset();
//			*handledPB = true;
//		}
//		else if (S_update_key_binds_cmd == command)
//		{
//			ERR(ReadKeyBindings());
//			*handledPB = true;
//		}
//	}
//	catch (A_Err& thrown_err) 
//	{
//		err = thrown_err;
//	}
//
//	return err;
//}
//

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

	if (S_uiohook_thread.joinable())
	{
		S_uiohook_thread.join();
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
    
	//ERR(suites.CommandSuite1()->AEGP_GetUniqueCommand(&S_zoom_cmd));
	//ERR(suites.CommandSuite1()->AEGP_GetUniqueCommand(&S_init_js_bridge_cmd));
	//ERR(suites.CommandSuite1()->AEGP_GetUniqueCommand(&S_start_key_capture_cmd));
	//ERR(suites.CommandSuite1()->AEGP_GetUniqueCommand(&S_update_key_binds_cmd));

	//ERR(suites.CommandSuite1()->AEGP_InsertMenuCommand(S_zoom_cmd, "Hack Bip Bop", AEGP_Menu_ANIMATION, AEGP_MENU_INSERT_AT_BOTTOM));
	//ERR(suites.CommandSuite1()->AEGP_InsertMenuCommand(S_init_js_bridge_cmd, "__zoom_init_js_bridge__", AEGP_Menu_NONE, AEGP_MENU_INSERT_SORTED));
	//ERR(suites.CommandSuite1()->AEGP_InsertMenuCommand(S_start_key_capture_cmd, "__zoom_start_key_capture__", AEGP_Menu_NONE, AEGP_MENU_INSERT_SORTED));
	//ERR(suites.CommandSuite1()->AEGP_InsertMenuCommand(S_stop_key_capture_cmd, "__zoom_stop_key_capture__", AEGP_Menu_NONE, AEGP_MENU_INSERT_SORTED));
	//ERR(suites.CommandSuite1()->AEGP_InsertMenuCommand(S_update_key_binds_cmd, "__zoom_update_key_binds__", AEGP_Menu_NONE, AEGP_MENU_INSERT_SORTED));

	//ERR(suites.RegisterSuite5()->AEGP_RegisterCommandHook(S_zoom_id, AEGP_HP_BeforeAE, AEGP_Command_ALL, CommandHook, 0));
	//ERR(suites.RegisterSuite5()->AEGP_RegisterUpdateMenuHook(S_zoom_id, UpdateMenuHook, 0));
	ERR(suites.RegisterSuite5()->AEGP_RegisterIdleHook(S_zoom_id, IdleHook, 0));
	ERR(suites.RegisterSuite5()->AEGP_RegisterDeathHook(S_zoom_id, DeathHook, 0));

	ERR(suites.MemorySuite1()->AEGP_SetMemReportingOn(true));

#ifdef AE_OS_WIN
	ERR(suites.UtilitySuite6()->AEGP_GetMainHWND(&S_main_win_h));
#endif

	ERR(ReadKeyBindings());

    // Set the event callback for uiohook events.
    hook_set_dispatch_proc(&dispatch_proc, NULL);

	// Start hook thread
	S_uiohook_thread = std::thread(HookProc);

	return err;
}

/**
* \brief Free any string memory which has been returned as function result.
*
* \param *p Pointer to the string
*/
extern "C" DllExport void ESFreeMem(void* p)
{
	delete(char*)(p);
}

/**
* \brief Returns the version number of the library
*
* ExtendScript publishes this number as the version property of the object
* created by new ExternalObject.
*/
extern "C" DllExport long ESGetVersion()
{
	return 0x1;
}

/**
* \brief Initialize the library and return function signatures.
*
* These signatures have no effect on the arguments that can be passed to the functions.
* They are used by JavaScript to cast the arguments, and to populate the
* reflection interface.
*/
extern "C" DllExport char* ESInitialize(const TaggedData * *argv, long argc)
{
	return nullptr;
}

/**
* \brief Terminate the library.
*
* Does any necessary clean up that is needed.
*/
extern "C" DllExport void ESTerminate()
{}

/* This function is called from the script panel */
extern "C" DllExport long isAvailableFn(TaggedData* argv, long argc, TaggedData* retval)
{
	// return true
	retval->type = kTypeBool;
	retval->data.intval = 1;

	return kESErrOK;
}

/* This function is called from the script panel */
extern "C" DllExport long updateKeyBindings(TaggedData* argv, long argc, TaggedData* retval)
{
	ReadKeyBindings();
	return kESErrOK;
}

/* This function is called from the script panel */
extern "C" DllExport long startKeyCapture(TaggedData* argv, long argc, TaggedData* retval)
{
	S_is_creating_key_bind = true;
	return kESErrOK;
}

/* This function is called from the script panel */
extern "C" DllExport long endKeyCapture(TaggedData* argv, long argc, TaggedData* retval)
{
	S_is_creating_key_bind = false;
	S_key_codes_pass.reset();

	return kESErrOK;
}
