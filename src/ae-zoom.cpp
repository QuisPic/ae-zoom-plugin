#include <mutex>
#include <thread>
#include <string>
#include <vector>

#include "ae-zoom.h"
#include "ae.h"
#include "uiohook.h"

#ifdef AE_OS_MAC
#include <objc/objc-runtime.h>
#endif

// Macros for reading a file to a string at compile time
#define STRINGIFY(...) #__VA_ARGS__
#define STR(...) STRINGIFY(__VA_ARGS__)

static AEGP_Command					S_zoom_cmd = 0L;
static AEGP_PluginID				S_zoom_id = 0L;
static SPBasicSuite*				sP = NULL;
static float						S_zoom_delta = 0.0f;
static bool							S_ctrl = false;
static std::thread					S_mouse_events_thread;
#ifdef AE_OS_WIN
static HWND							S_main_win_h = nullptr;
#elif defined AE_OS_MAC
static int                          S_main_win_id = -1;
#endif
static std::vector<LogMessage>		log_messages;

// mutexes
static std::mutex					S_mouse_wheel_mutex;

static std::string zoom_script = {
	#include "zoom-script.js"
};

static void logger_proc(unsigned int level, void *user_data, const char *format, va_list args) {
    switch (level) {
        case LOG_LEVEL_INFO:
            break;

        case LOG_LEVEL_WARN:
        case LOG_LEVEL_ERROR:
            //vfprintf(stderr, format, args);
			std::lock_guard lock(S_mouse_wheel_mutex);
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

#ifdef AE_OS_MAC
void GetMainWindowId()
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

bool IsCursorInsideMainWindow() {
    bool isInside = false;
#ifdef AE_OS_MAC
    if (S_main_win_id == -1)
    {
        GetMainWindowId();
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

        // Check if the cursor is inside the window's frame
        isInside = CGRectContainsPoint(windowFrame, cursorPosition);
    }
    
    CFRelease(CFWinArray);
#elif defined AE_OS_WIN
    POINT cursorPos;
    GetCursorPos(&cursorPos); // Get the cursor position in screen coordinates

    ScreenToClient(S_main_win_h, &cursorPos); // Convert the screen coordinates to client coordinates relative to the window

    RECT clientRect;
    GetClientRect(S_main_win_h, &clientRect); // Get the client area of the window

    // Check if the cursor is inside the client area
    isInside = PtInRect(&clientRect, cursorPos))
#endif
    
    return isInside;
}

#ifdef AE_OS_WIN
template <typename T>
T GetFromAfterFXDll(std::string fn_name)
{
	HINSTANCE hDLL = LoadLibrary("AfterFXLib.dll");

	if (!hDLL) {
		logger(LOG_LEVEL_ERROR, "Failed to load AfterFXLib.dll");
		return nullptr;
	}

	T result = reinterpret_cast<T>(GetProcAddress(hDLL, fn_name.c_str()));

	FreeLibrary(hDLL);
	return result;
}

void HackBipBop()
{
	AEGP_SuiteHandler	suites(sP);
	//suites.UtilitySuite3()->AEGP_ReportInfo(S_zoom_id, std::to_string(helpCtx).c_str());

	auto gEgg = GetFromAfterFXDll<CEggApp*>("?gEgg@@3PEAVCEggApp@@EA");
	//auto GetSelectedToolFn = GetFromAfterFXDll<GetSelectedTool>("?GetSelectedTool@CEggApp@@QEBA?AW4ToolID@@XZ");
	auto GetActualPrimaryPreviewItemFn = 
		GetFromAfterFXDll<GetActualPrimaryPreviewItem>(
			"?GetActualPrimaryPreviewItem@CEggApp@@QEAAXPEAPEAVCPanoProjItem@@PEAPEAVCDirProjItem@@PEAPEAVBEE_Item@@_N33@Z"
		);
	auto GetCurrentItemFn = 
		GetFromAfterFXDll<GetCurrentItem>(
			"?GetCurrentItem@CEggApp@@QEAAXPEAPEAVBEE_Item@@PEAPEAVCPanoProjItem@@@Z"
		);
	auto GetCurrentItemHFn = 
		GetFromAfterFXDll<GetCurrentItemH>(
			"?GetCurrentItemH@CEggApp@@QEAAPEAVBEE_Item@@XZ"
		);

	CPanoProjItem* view_pano = nullptr;
	CDirProjItem* ccdir = nullptr;
	//BEE_Item* bee_item = nullptr;
	//GetCurrentItemFn(gEgg, &bee_item, &view_pano);
	BEE_Item* bee_item = GetCurrentItemHFn(gEgg);
	//GetActualPrimaryPreviewItemFn(gEgg, &view_pano, &ccdir, &bee_item, false, false, false);
	//auto selectedTool = GetSelectedTool(gEgg);

	if (bee_item)
	{
		suites.UtilitySuite3()->AEGP_ReportInfo(S_zoom_id, "yesssssssssssssss");
	}
}
#endif

void dispatch_proc(uiohook_event * const event, void *user_data) {
    switch (event->type) {
	case EVENT_KEY_PRESSED:
		if (
			#ifdef AE_OS_WIN
				event->data.keyboard.keycode == VC_CONTROL_L || 
				event->data.keyboard.keycode == VC_CONTROL_R
			#else
				event->data.keyboard.keycode == VC_META_L || 
				event->data.keyboard.keycode == VC_META_R
			#endif
		) 
		{
			S_ctrl = true;
		}
		break;
	case EVENT_KEY_RELEASED:
		if (
			#ifdef AE_OS_WIN
				event->data.keyboard.keycode == VC_CONTROL_L || 
				event->data.keyboard.keycode == VC_CONTROL_R
			#else
				event->data.keyboard.keycode == VC_META_L || 
				event->data.keyboard.keycode == VC_META_R
			#endif
		) 
		{
			S_ctrl = false;
		}
		break;
	case EVENT_MOUSE_WHEEL:
		if (
//			S_main_win_h == GetForegroundWindow() &&
            IsCursorInsideMainWindow()
		) {
			if (S_ctrl && event->data.wheel.delta)
			{
				const std::lock_guard lock(S_mouse_wheel_mutex);
				S_zoom_delta += 
					event->data.wheel.rotation / static_cast<float>(event->data.wheel.delta);

				event->reserved = 0x1;
			}
		}

		break;

	default:
		break;
    }
}

int MouseHookProc() {
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
    
#ifdef AE_OS_MAC
    if (!S_main_win_id)
    {
        GetMainWindowId();
    }
#endif

	if (log_messages.size() > 0)
	{
		AEGP_SuiteHandler	suites(sP);

		for (auto it = log_messages.begin(); it != log_messages.end();)
		{
			suites.UtilitySuite3()->AEGP_ReportInfo(S_zoom_id, log_messages[0].format);

			std::lock_guard lock(S_mouse_wheel_mutex);
			it = log_messages.erase(log_messages.begin());
		}
	}

	if (S_zoom_delta) 
	{
		AEGP_SuiteHandler	suites(sP);

		suites.UtilitySuite6()->AEGP_ExecuteScript(
			S_zoom_id,
			(zoom_script + "(" + std::to_string(S_zoom_delta) + ")").c_str(),
			false,
			nullptr,
			nullptr
		);

		const std::lock_guard lock(S_mouse_wheel_mutex);
		S_zoom_delta = 0;

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
	A_Err err = A_Err_NONE;
	A_Err err2 = A_Err_NONE;
	AEGP_SuiteHandler suites(sP);
    
	ERR(suites.CommandSuite1()->AEGP_GetUniqueCommand(&S_zoom_cmd));
	ERR(suites.CommandSuite1()->AEGP_InsertMenuCommand(S_zoom_cmd, "Hack Bip Bop", AEGP_Menu_ANIMATION, AEGP_MENU_INSERT_AT_BOTTOM));

	ERR(suites.RegisterSuite5()->AEGP_RegisterCommandHook(S_zoom_id, AEGP_HP_BeforeAE, AEGP_Command_ALL, CommandHook, 0));
	ERR(suites.RegisterSuite5()->AEGP_RegisterUpdateMenuHook(S_zoom_id, UpdateMenuHook, 0));
	ERR(suites.RegisterSuite5()->AEGP_RegisterIdleHook(S_zoom_id, IdleHook, 0));
	ERR(suites.RegisterSuite5()->AEGP_RegisterDeathHook(S_zoom_id, DeathHook, 0));

#ifdef AE_OS_WIN
	suites.UtilitySuite6()->AEGP_GetMainHWND(&S_main_win_h);
#endif

	if (err)
	{
		ERR2(suites.UtilitySuite3()->AEGP_ReportInfo(S_zoom_id, "Could not register command hook."));
	}

    // Set the event callback for uiohook events.
    hook_set_dispatch_proc(&dispatch_proc, NULL);

	S_mouse_events_thread = std::thread(MouseHookProc);

	return err;
}
