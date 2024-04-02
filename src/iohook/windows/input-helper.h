#pragma once

#include <cstdint>
#include <windows.h>

#define CAPLOK 0x01
#define WCH_NONE 0xF000
#define WCH_DEAD 0xF001

#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif

extern unsigned short keycode_to_scancode(DWORD vk_code, WORD flags);
extern uint16_t get_modifiers();
