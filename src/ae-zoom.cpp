#include <mutex>
#include <thread>
#include <string>

#include "ae-zoom.h"
#include "uiohook.h"

// Macros for reading a file to a string at compile time
#define STRINGIFY(...) #__VA_ARGS__
#define STR(...) STRINGIFY(__VA_ARGS__)

static AEGP_Command			S_zoom_cmd = 0L;
static AEGP_PluginID		S_zoom_id = 0L;
static A_long				S_idle_count = 0L;
static SPBasicSuite*		sP = NULL;
static long					S_zoom_delta = 0;
static std::mutex			S_mouse_wheel_mutex;
static std::thread			S_mouse_events_thread;
static HWND					S_main_hwnd;

static std::string zoom_script = {
	#include "zoom-script.js"
};

bool isCursorInsideMainWnd()
{
	POINT cursorPos;
	GetCursorPos(&cursorPos); // Get the cursor position in screen coordinates

	ScreenToClient(S_main_hwnd, &cursorPos); // Convert the screen coordinates to client coordinates relative to the window

	RECT clientRect;
	GetClientRect(S_main_hwnd, &clientRect); // Get the client area of the window

	// Check if the cursor is inside the client area
	if (PtInRect(&clientRect, cursorPos))
	{
		return true;
	}
	else
	{
		return false;
	}
}

LRESULT CALLBACK LLMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		const bool is_ctrl_down = GetAsyncKeyState(VK_CONTROL) & 0x8000;

		if (
			is_ctrl_down &&
			wParam == WM_MOUSEWHEEL && 
			S_main_hwnd == GetForegroundWindow() &&
			isCursorInsideMainWnd()
		) {
			const MSLLHOOKSTRUCT* pMouseStruct = std::bit_cast<MSLLHOOKSTRUCT*>(lParam);
			const short wheelDelta = GET_WHEEL_DELTA_WPARAM(pMouseStruct->mouseData);

			if (wheelDelta)
			{
				const std::lock_guard lock(S_mouse_wheel_mutex);
				S_zoom_delta += wheelDelta / WHEEL_DELTA;
			}

			// returning 1 stops this message from dispatching it to the main AE thread
			return 1;
		}
	}


	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void CatchMouse()
{
	HINSTANCE hInstance = GetModuleHandle(NULL);
	const HHOOK hMouseWheelHook = SetWindowsHookEx(WH_MOUSE_LL, LLMouseProc, hInstance, NULL);
	MSG message;

	while (GetMessage(&message, NULL, NULL, NULL)) {
		TranslateMessage(&message);
		DispatchMessage(&message);
	}

	UnhookWindowsHookEx(hMouseWheelHook);
}

static	A_Err	IdleHook(
	AEGP_GlobalRefcon	plugin_refconP,
	AEGP_IdleRefcon		refconP,
	A_long* max_sleepPL)
{
	A_Err err = A_Err_NONE;

	if (S_zoom_delta) {
		AEGP_SuiteHandler	suites(sP);

		suites.UtilitySuite6()->AEGP_ExecuteScript(
			S_zoom_id,
			(zoom_script + "(" + std::to_string(S_zoom_delta) + ")").c_str(),
			false,
			nullptr,
			nullptr
		);
		//suites.UtilitySuite3()->AEGP_ReportInfo(S_zoom_id, std::to_string(S_zoom_delta).c_str());

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
	A_Err 				err = A_Err_NONE,
						err2 = A_Err_NONE;

	AEGP_ItemH			active_itemH = NULL;

	AEGP_ItemType		item_type = AEGP_ItemType_NONE;

	AEGP_SuiteHandler	suites(sP);

	err = suites.ItemSuite6()->AEGP_GetActiveItem(&active_itemH);

	if (!err && active_itemH) {
		err = suites.ItemSuite6()->AEGP_GetItemType(active_itemH, &item_type);

		if (!err && (AEGP_ItemType_COMP == item_type ||
			AEGP_ItemType_FOOTAGE == item_type)) {
			ERR(suites.CommandSuite1()->AEGP_EnableCommand(S_zoom_cmd));
		}
	}
	else {
		ERR2(suites.CommandSuite1()->AEGP_DisableCommand(S_zoom_cmd));
	}
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
	DWORD mouse_thread_id = GetThreadId(S_mouse_events_thread.native_handle());
	PostThreadMessage(mouse_thread_id, WM_QUIT, 0, 0);

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

	ERR(suites.RegisterSuite5()->AEGP_RegisterCommandHook(S_zoom_id, AEGP_HP_BeforeAE, AEGP_Command_ALL, CommandHook, 0));
	ERR(suites.RegisterSuite5()->AEGP_RegisterUpdateMenuHook(S_zoom_id, UpdateMenuHook, 0));
	ERR(suites.RegisterSuite5()->AEGP_RegisterIdleHook(S_zoom_id, IdleHook, 0));
	ERR(suites.RegisterSuite5()->AEGP_RegisterDeathHook(S_zoom_id, DeathHook, 0));

	suites.UtilitySuite6()->AEGP_GetMainHWND(&S_main_hwnd);

	if (err)
	{
		ERR2(suites.UtilitySuite3()->AEGP_ReportInfo(S_zoom_id, "Could not register command hook."));
	}

	S_mouse_events_thread = std::thread(CatchMouse);

	return err;
}
