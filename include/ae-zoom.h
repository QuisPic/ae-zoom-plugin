#pragma once

#include <memory>
#include <vector>

#include "AEConfig.h"
#ifdef AE_OS_WIN
#include <windows.h>
#endif

#include "entry.h"
#include "AE_GeneralPlug.h"
#include "AE_Macros.h"
#include "AEGP_SuiteHandler.h"
#include "Strings.h"

#include <uiohook.h>
#include <nlohmann/json.hpp>

#define AEGP_MAX_STREAM_DIM 4
constexpr uint16_t MOUSE_WHEEL_UP = 1;
constexpr uint16_t MOUSE_WHEEL_DOWN = 2;

// This entry point is exported through the PiPL (.r file)
extern "C" DllExport AEGP_PluginInitFuncPrototype EntryPointFunc;

struct LogMessage {
	unsigned int level = 0;
	const char* format = nullptr;
};

struct KeyBind {
	event_type type;
	uint16_t mask;
    uint16_t keycode;

	KeyBind(): 
		type(EVENT_KEY_PRESSED), mask(0x0), keycode(VC_UNDEFINED) {};

	KeyBind(uiohook_event* e)
	{
		type = e->type;
		mask = e->mask;

		switch (e->type)
		{
		case EVENT_KEY_PRESSED:
			keycode = e->data.keyboard.keycode;
			break;
		case EVENT_MOUSE_PRESSED:
			keycode = e->data.mouse.button;
			break;
		case EVENT_MOUSE_WHEEL:
			keycode = e->data.wheel.rotation >= 0
				? MOUSE_WHEEL_UP
				: MOUSE_WHEEL_DOWN;
			break;
		default:
			keycode = 0;
			break;
		}
	}

	auto operator<=>(const KeyBind&) const = default;
};
