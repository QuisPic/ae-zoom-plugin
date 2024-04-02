#pragma once

#import <AppKit/AppKit.h>
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <MacTypes.h>
#include <cstdint>

#ifdef USE_IOKIT
#include <IOKit/hidsystem/ev_keymap.h>
#endif
#include <stdbool.h>

#ifndef USE_IOKIT
// Some of the system key codes that maybe missing from IOKit.  They appear to
// have shown up over the years.

#ifndef NX_NOSPECIALKEY
#define NX_NOSPECIALKEY 0xFFFF
#endif

#ifndef NX_KEYTYPE_SOUND_UP
#define NX_KEYTYPE_SOUND_UP 0x00
#endif

#ifndef NX_KEYTYPE_SOUND_DOWN
#define NX_KEYTYPE_SOUND_DOWN 0x01
#endif

#ifndef NX_KEYTYPE_BRIGHTNESS_UP
#define NX_KEYTYPE_BRIGHTNESS_UP 0x02
#endif

#ifndef NX_KEYTYPE_BRIGHTNESS_DOWN
#define NX_KEYTYPE_BRIGHTNESS_DOWN 0x03
#endif

#ifndef NX_KEYTYPE_CAPS_LOCK
#define NX_KEYTYPE_CAPS_LOCK 0x04
#endif

#ifndef NX_KEYTYPE_HELP
#define NX_KEYTYPE_HELP 0x05
#endif

#ifndef NX_POWER_KEY
#define NX_POWER_KEY 0x06
#endif

#ifndef NX_KEYTYPE_MUTE
#define NX_KEYTYPE_MUTE 0x07
#endif

#ifndef NX_UP_ARROW_KEY
#define NX_UP_ARROW_KEY 0x0B
#endif

#ifndef NX_DOWN_ARROW_KEY
#define NX_DOWN_ARROW_KEY 0x09
#endif

#ifndef NX_KEYTYPE_NUM_LOCK
#define NX_KEYTYPE_NUM_LOCK 0x0A
#endif

#ifndef NX_KEYTYPE_CONTRAST_UP
#define NX_KEYTYPE_CONTRAST_UP 0x0B
#endif

#ifndef NX_KEYTYPE_CONTRAST_DOWN
#define NX_KEYTYPE_CONTRAST_DOWN 0x0C
#endif

#ifndef NX_KEYTYPE_LAUNCH_PANEL
#define NX_KEYTYPE_LAUNCH_PANEL 0x0D
#endif

#ifndef NX_KEYTYPE_EJECT
#define NX_KEYTYPE_EJECT 0x0E
#endif

#ifndef NX_KEYTYPE_VIDMIRROR
#define NX_KEYTYPE_VIDMIRROR 0x0F
#endif

#ifndef NX_KEYTYPE_PLAY
#define NX_KEYTYPE_PLAY 0x10
#endif

#ifndef NX_KEYTYPE_NEXT
#define NX_KEYTYPE_NEXT 0x11
#endif

#ifndef NX_KEYTYPE_PREVIOUS
#define NX_KEYTYPE_PREVIOUS 0x12
#endif

#ifndef NX_KEYTYPE_FAST
#define NX_KEYTYPE_FAST 0x13
#endif

#ifndef NX_KEYTYPE_REWIND
#define NX_KEYTYPE_REWIND 0x14
#endif

#ifndef NX_KEYTYPE_ILLUMINATION_UP
#define NX_KEYTYPE_ILLUMINATION_UP 0x15
#endif

#ifndef NX_KEYTYPE_ILLUMINATION_DOWN
#define NX_KEYTYPE_ILLUMINATION_DOWN 0x16
#endif

#ifndef NX_KEYTYPE_ILLUMINATION_TOGGLE
#define NX_KEYTYPE_ILLUMINATION_TOGGLE 0x17
#endif

#ifndef NX_NUMSPECIALKEYS
#define NX_NUMSPECIALKEYS 0x18 /* Maximum number of special keys */
#endif

#endif

// These virtual key codes do not appear to be defined by Apple.
#define kVK_NX_Power 0xE0 | NX_POWER_KEY     /* 0xE6 */
#define kVK_NX_Eject 0xE0 | NX_KEYTYPE_EJECT /* 0xEE */

#define kVK_MEDIA_Play 0xE0 | NX_KEYTYPE_PLAY         /* 0xF0 */
#define kVK_MEDIA_Next 0xE0 | NX_KEYTYPE_NEXT         /* 0xF1 */
#define kVK_MEDIA_Previous 0xE0 | NX_KEYTYPE_PREVIOUS /* 0xF2 */

#define kVK_RightCommand 0x36
#define kVK_ContextMenu 0x6E // AKA kMenuPowerGlyph
#define kVK_Undefined 0xFF

// These button codes do not appear to be defined by Apple.
#define kVK_LBUTTON kCGMouseButtonLeft
#define kVK_RBUTTON kCGMouseButtonRight
#define kVK_MBUTTON kCGMouseButtonCenter
#define kVK_XBUTTON1 3
#define kVK_XBUTTON2 4

// These button masks do not appear to be defined by Apple.
#define kCGEventFlagMaskButtonLeft 1 << 0
#define kCGEventFlagMaskButtonRight 1 << 1
#define kCGEventFlagMaskButtonCenter 1 << 2
#define kCGEventFlagMaskXButton1 1 << 3
#define kCGEventFlagMaskXButton2 1 << 4

/* Converts an OSX keycode to the appropriate UIOHook scancode constant. */
extern uint16_t keycode_to_scancode(uint16_t keycode);

/* Get the current native modifier mask state. */
extern uint16_t get_modifiers(NSEventModifierFlags flags);
