#pragma once

#include <mutex>
#include <thread>
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <vector>
#include <tuple>

#include "AEConfig.h"
#ifdef AE_OS_WIN
#include <windows.h>
#endif

#include "entry.h"
#include "AE_GeneralPlug.h"
#include "AE_Macros.h"
#include "AEGP_SuiteHandler.h"
#include "Strings.h"

#include "ExternalObject/SoCClient.h"
#include "ExternalObject/SoSharedLibDefs.h"

#include <uiohook.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

#define AEGP_MAX_STREAM_DIM 4
constexpr uint16_t MOUSE_WHEEL_UP = 1;
constexpr uint16_t MOUSE_WHEEL_DOWN = 2;
constexpr char SETTINGS_SECTION_NAME[] = "Quis/Ae_Smooth_Zoom";

// This entry point is exported through the PiPL (.r file)
extern "C" DllExport AEGP_PluginInitFuncPrototype EntryPointFunc;

class LogMessage {
private:
	std::string format_str;
public:
	unsigned int level = 0;
	const char* format = nullptr;

	LogMessage(unsigned int level, const char* format)
		: level(level), format(format)
	{}

	LogMessage(unsigned int level, const std::string& _format_str)
		: format_str(_format_str), level(level) 
	{
		format = format_str.c_str();
	}
};

enum class WIND_RECT
{
	FULL,
	CLIENT
};

enum class KB_ACTION
{
	INCREASE,
	DECREASE,
	SET_TO
};

class KeyCodes 
{
public:
	event_type type;
	uint16_t mask;
    uint16_t keycode;

	KeyCodes() = default;

	KeyCodes(const event_type& type, const uint16_t& mask, const uint16_t& keycode)
		: type(type), mask(mask), keycode(keycode)
	{}

	KeyCodes(uiohook_event* e)
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

	auto operator<=>(const KeyCodes&) const = default;
};

class KeyBindAction
{
public:
	KeyCodes keyCodes;
	KB_ACTION action;
	double amount;

	KeyBindAction() = default;
	KeyBindAction(const KeyCodes& keyCodes, const KB_ACTION& action, double amount)
		: keyCodes(keyCodes), action(action), amount(amount)
	{}
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(KeyCodes, type, mask, keycode);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(KeyBindAction, keyCodes, action, amount);
