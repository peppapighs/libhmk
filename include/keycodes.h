/*
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "common.h"

typedef enum {
  KC_NO = 0x00,
  KC_TRANSPARENT = 0x01,

  // Keyboard keycodes
  KC_A = 0x02,
  KC_B = 0x03,
  KC_C = 0x04,
  KC_D = 0x05,
  KC_E = 0x06,
  KC_F = 0x07,
  KC_G = 0x08,
  KC_H = 0x09,
  KC_I = 0x0A,
  KC_J = 0x0B,
  KC_K = 0x0C,
  KC_L = 0x0D,
  KC_M = 0x0E,
  KC_N = 0x0F,
  KC_O = 0x10,
  KC_P = 0x11,
  KC_Q = 0x12,
  KC_R = 0x13,
  KC_S = 0x14,
  KC_T = 0x15,
  KC_U = 0x16,
  KC_V = 0x17,
  KC_W = 0x18,
  KC_X = 0x19,
  KC_Y = 0x1A,
  KC_Z = 0x1B,
  KC_1 = 0x1C,
  KC_2 = 0x1D,
  KC_3 = 0x1E,
  KC_4 = 0x1F,
  KC_5 = 0x20,
  KC_6 = 0x21,
  KC_7 = 0x22,
  KC_8 = 0x23,
  KC_9 = 0x24,
  KC_0 = 0x25,
  KC_ENTER = 0x26,
  KC_ESCAPE = 0x27,
  KC_BACKSPACE = 0x28,
  KC_TAB = 0x29,
  KC_SPACE = 0x2A,
  KC_MINUS = 0x2B,
  KC_EQUAL = 0x2C,
  KC_LEFT_BRACKET = 0x2D,
  KC_RIGHT_BRACKET = 0x2E,
  KC_BACKSLASH = 0x2F,
  KC_NONUS_HASH = 0x30,
  KC_SEMICOLON = 0x31,
  KC_QUOTE = 0x32,
  KC_GRAVE = 0x33,
  KC_COMMA = 0x34,
  KC_DOT = 0x35,
  KC_SLASH = 0x36,
  KC_CAPS_LOCK = 0x37,
  KC_F1 = 0x38,
  KC_F2 = 0x39,
  KC_F3 = 0x3A,
  KC_F4 = 0x3B,
  KC_F5 = 0x3C,
  KC_F6 = 0x3D,
  KC_F7 = 0x3E,
  KC_F8 = 0x3F,
  KC_F9 = 0x40,
  KC_F10 = 0x41,
  KC_F11 = 0x42,
  KC_F12 = 0x43,
  KC_PRINT_SCREEN = 0x44,
  KC_SCROLL_LOCK = 0x45,
  KC_PAUSE = 0x46,
  KC_INSERT = 0x47,
  KC_HOME = 0x48,
  KC_PAGE_UP = 0x49,
  KC_DELETE = 0x4A,
  KC_END = 0x4B,
  KC_PAGE_DOWN = 0x4C,
  KC_RIGHT = 0x4D,
  KC_LEFT = 0x4E,
  KC_DOWN = 0x4F,
  KC_UP = 0x50,
  KC_NUM_LOCK = 0x51,
  KC_KP_SLASH = 0x52,
  KC_KP_ASTERISK = 0x53,
  KC_KP_MINUS = 0x54,
  KC_KP_PLUS = 0x55,
  KC_KP_ENTER = 0x56,
  KC_KP_1 = 0x57,
  KC_KP_2 = 0x58,
  KC_KP_3 = 0x59,
  KC_KP_4 = 0x5A,
  KC_KP_5 = 0x5B,
  KC_KP_6 = 0x5C,
  KC_KP_7 = 0x5D,
  KC_KP_8 = 0x5E,
  KC_KP_9 = 0x5F,
  KC_KP_0 = 0x60,
  KC_KP_DOT = 0x61,
  KC_NONUS_BACKSLASH = 0x62,
  KC_APPLICATION = 0x63,
  KC_F13 = 0x64,
  KC_F14 = 0x65,
  KC_F15 = 0x66,
  KC_F16 = 0x67,
  KC_F17 = 0x68,
  KC_F18 = 0x69,
  KC_F19 = 0x6A,
  KC_F20 = 0x6B,
  KC_F21 = 0x6C,
  KC_F22 = 0x6D,
  KC_F23 = 0x6E,
  KC_F24 = 0x6F,
  KC_INTERNATIONAL_1 = 0x70,
  KC_INTERNATIONAL_2 = 0x71,
  KC_INTERNATIONAL_3 = 0x72,
  KC_INTERNATIONAL_4 = 0x73,
  KC_INTERNATIONAL_5 = 0x74,
  KC_INTERNATIONAL_6 = 0x75,
  KC_LANGUAGE_1 = 0x76,
  KC_LANGUAGE_2 = 0x77,
  KC_LANGUAGE_3 = 0x78,
  KC_LANGUAGE_4 = 0x79,
  KC_LANGUAGE_5 = 0x7A,
  KC_LANGUAGE_6 = 0x7B,

  // Modifier keycodes
  KC_LEFT_CTRL = 0x7C,
  KC_LEFT_SHIFT = 0x7D,
  KC_LEFT_ALT = 0x7E,
  KC_LEFT_GUI = 0x7F,
  KC_RIGHT_CTRL = 0x80,
  KC_RIGHT_SHIFT = 0x81,
  KC_RIGHT_ALT = 0x82,
  KC_RIGHT_GUI = 0x83,

  // System keycodes
  KC_SYSTEM_POWER = 0x84,
  KC_SYSTEM_SLEEP = 0x85,
  KC_SYSTEM_WAKE = 0x86,

  // Consumer keycodes
  KC_AUDIO_MUTE = 0x87,
  KC_AUDIO_VOL_UP = 0x88,
  KC_AUDIO_VOL_DOWN = 0x89,
  KC_MEDIA_NEXT_TRACK = 0x8A,
  KC_MEDIA_PREV_TRACK = 0x8B,
  KC_MEDIA_STOP = 0x8C,
  KC_MEDIA_PLAY_PAUSE = 0x8D,
  KC_MEDIA_SELECT = 0x8E,
  KC_MAIL = 0x8F,
  KC_CALCULATOR = 0x90,
  KC_MY_COMPUTER = 0x91,
  KC_WWW_SEARCH = 0x92,
  KC_WWW_HOME = 0x93,
  KC_WWW_BACK = 0x94,
  KC_WWW_FORWARD = 0x95,
  KC_WWW_REFRESH = 0x96,
  KC_WWW_FAVORITES = 0x97,
  KC_BRIGHTNESS_UP = 0x98,
  KC_BRIGHTNESS_DOWN = 0x99,
  KC_MISSION_CONTROL = 0x9A,
  KC_LAUNCHPAD = 0x9B,

  // Mouse keycodes
  SP_MOUSE_BUTTON_1 = 0x9C,
  SP_MOUSE_BUTTON_2 = 0x9D,
  SP_MOUSE_BUTTON_3 = 0x9E,
  SP_MOUSE_BUTTON_4 = 0x9F,
  SP_MOUSE_BUTTON_5 = 0xA0,

  // Layer keycodes
  SP_MO_MIN = 0xC0,
  SP_MO_MAX = 0xC7,

  // Profile keycodes
  SP_PF_MIN = 0xC8,
  SP_PF_MAX = 0xCF,

  // Special keycodes
  SP_KEY_LOCK = 0xD0,
  SP_LAYER_LOCK = 0xD1,
  SP_PROFILE_SWAP = 0xD2,
  SP_PROFILE_NEXT = 0xD3,
  SP_BOOT = 0xD4,

  XXXXXXX = KC_NO,
  _______ = KC_TRANSPARENT,
  KC_TRNS = KC_TRANSPARENT,
  KC_ENT = KC_ENTER,
  KC_ESC = KC_ESCAPE,
  KC_BSPC = KC_BACKSPACE,
  KC_SPC = KC_SPACE,
  KC_MINS = KC_MINUS,
  KC_EQL = KC_EQUAL,
  KC_LBRC = KC_LEFT_BRACKET,
  KC_RBRC = KC_RIGHT_BRACKET,
  KC_BSLS = KC_BACKSLASH,
  KC_NUHS = KC_NONUS_HASH,
  KC_SCLN = KC_SEMICOLON,
  KC_QUOT = KC_QUOTE,
  KC_GRV = KC_GRAVE,
  KC_COMM = KC_COMMA,
  KC_SLSH = KC_SLASH,
  KC_CAPS = KC_CAPS_LOCK,
  KC_PSCR = KC_PRINT_SCREEN,
  KC_SCRL = KC_SCROLL_LOCK,
  KC_BRMD = KC_SCROLL_LOCK,
  KC_PAUS = KC_PAUSE,
  KC_BRK = KC_PAUSE,
  KC_BRMU = KC_PAUSE,
  KC_INS = KC_INSERT,
  KC_PGUP = KC_PAGE_UP,
  KC_DEL = KC_DELETE,
  KC_PGDN = KC_PAGE_DOWN,
  KC_RGHT = KC_RIGHT,
  KC_NUM = KC_NUM_LOCK,
  KC_PSLS = KC_KP_SLASH,
  KC_PAST = KC_KP_ASTERISK,
  KC_PMNS = KC_KP_MINUS,
  KC_PPLS = KC_KP_PLUS,
  KC_PENT = KC_KP_ENTER,
  KC_P1 = KC_KP_1,
  KC_P2 = KC_KP_2,
  KC_P3 = KC_KP_3,
  KC_P4 = KC_KP_4,
  KC_P5 = KC_KP_5,
  KC_P6 = KC_KP_6,
  KC_P7 = KC_KP_7,
  KC_P8 = KC_KP_8,
  KC_P9 = KC_KP_9,
  KC_P0 = KC_KP_0,
  KC_PDOT = KC_KP_DOT,
  KC_NUBS = KC_NONUS_BACKSLASH,
  KC_APP = KC_APPLICATION,
  KC_INT1 = KC_INTERNATIONAL_1,
  KC_INT2 = KC_INTERNATIONAL_2,
  KC_INT3 = KC_INTERNATIONAL_3,
  KC_INT4 = KC_INTERNATIONAL_4,
  KC_INT5 = KC_INTERNATIONAL_5,
  KC_INT6 = KC_INTERNATIONAL_6,
  KC_LNG1 = KC_LANGUAGE_1,
  KC_LNG2 = KC_LANGUAGE_2,
  KC_LNG3 = KC_LANGUAGE_3,
  KC_LNG4 = KC_LANGUAGE_4,
  KC_LNG5 = KC_LANGUAGE_5,
  KC_LNG6 = KC_LANGUAGE_6,
  KC_PWR = KC_SYSTEM_POWER,
  KC_SLEP = KC_SYSTEM_SLEEP,
  KC_WAKE = KC_SYSTEM_WAKE,
  KC_MUTE = KC_AUDIO_MUTE,
  KC_VOLU = KC_AUDIO_VOL_UP,
  KC_VOLD = KC_AUDIO_VOL_DOWN,
  KC_MNXT = KC_MEDIA_NEXT_TRACK,
  KC_MPRV = KC_MEDIA_PREV_TRACK,
  KC_MSTP = KC_MEDIA_STOP,
  KC_MPLY = KC_MEDIA_PLAY_PAUSE,
  KC_MSEL = KC_MEDIA_SELECT,
  KC_CALC = KC_CALCULATOR,
  KC_MYCM = KC_MY_COMPUTER,
  KC_WSCH = KC_WWW_SEARCH,
  KC_WHOM = KC_WWW_HOME,
  KC_WBAK = KC_WWW_BACK,
  KC_WFWD = KC_WWW_FORWARD,
  KC_WREF = KC_WWW_REFRESH,
  KC_WFAV = KC_WWW_FAVORITES,
  KC_BRIU = KC_BRIGHTNESS_UP,
  KC_BRID = KC_BRIGHTNESS_DOWN,
  KC_MCTL = KC_MISSION_CONTROL,
  KC_LPAD = KC_LAUNCHPAD,
  KC_LCTL = KC_LEFT_CTRL,
  KC_LSFT = KC_LEFT_SHIFT,
  KC_LALT = KC_LEFT_ALT,
  KC_LOPT = KC_LEFT_ALT,
  KC_LGUI = KC_LEFT_GUI,
  KC_LCMD = KC_LEFT_GUI,
  KC_LWIN = KC_LEFT_GUI,
  KC_RCTL = KC_RIGHT_CTRL,
  KC_RSFT = KC_RIGHT_SHIFT,
  KC_RALT = KC_RIGHT_ALT,
  KC_ROPT = KC_RIGHT_ALT,
  KC_ALGR = KC_RIGHT_ALT,
  KC_RGUI = KC_RIGHT_GUI,
  KC_RCMD = KC_RIGHT_GUI,
  KC_RWIN = KC_RIGHT_GUI,
  MS_BTN1 = SP_MOUSE_BUTTON_1,
  MS_BTN2 = SP_MOUSE_BUTTON_2,
  MS_BTN3 = SP_MOUSE_BUTTON_3,
  MS_BTN4 = SP_MOUSE_BUTTON_4,
  MS_BTN5 = SP_MOUSE_BUTTON_5,
  KY_LOCK = SP_KEY_LOCK,
  LY_LOCK = SP_LAYER_LOCK,
  PF_SWAP = SP_PROFILE_SWAP,
  PF_NEXT = SP_PROFILE_NEXT,
} keycode_t;

// Map keycodes to their corresponding HID codes
extern const uint16_t keycode_to_hid[];

//--------------------------------------------------------------------+
// Keycode Macros
//--------------------------------------------------------------------+

#define MO(layer) (SP_MO_MIN | (layer))
#define MO_GET_LAYER(kc) ((kc) & 0x07)

#define PF(profile) (SP_PF_MIN | (profile))
#define PF_GET_PROFILE(kc) ((kc) & 0x07)

#define IS_KEYBOARD_KEYCODE(kc) (KC_A <= (kc) && (kc) <= KC_LANGUAGE_6)
#define IS_MODIFIER_KEYCODE(kc) (KC_LEFT_CTRL <= (kc) && (kc) <= KC_RIGHT_GUI)
#define IS_SYSTEM_KEYCODE(kc)                                                  \
  (KC_SYSTEM_POWER <= (kc) && (kc) <= KC_SYSTEM_WAKE)
#define IS_CONSUMER_KEYCODE(kc) (KC_AUDIO_MUTE <= (kc) && (kc) <= KC_LAUNCHPAD)
#define IS_MOUSE_KEYCODE(kc)                                                   \
  (SP_MOUSE_BUTTON_1 <= (kc) && (kc) <= SP_MOUSE_BUTTON_5)

//--------------------------------------------------------------------+
// Keycode Ranges
//--------------------------------------------------------------------+

#define KEYBOARD_KEYCODE_RANGE KC_A... KC_LANGUAGE_6
#define MODIFIER_KEYCODE_RANGE KC_LEFT_CTRL... KC_RIGHT_GUI
#define SYSTEM_KEYCODE_RANGE KC_SYSTEM_POWER... KC_SYSTEM_WAKE
#define CONSUMER_KEYCODE_RANGE KC_AUDIO_MUTE... KC_LAUNCHPAD
#define MOUSE_KEYCODE_RANGE SP_MOUSE_BUTTON_1... SP_MOUSE_BUTTON_5
#define HID_KEYCODE_RANGE KC_A... SP_MOUSE_BUTTON_5
#define MOMENTARY_LAYER_RANGE SP_MO_MIN... SP_MO_MAX
#define PROFILE_RANGE SP_PF_MIN... SP_PF_MAX
