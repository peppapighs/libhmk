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

#include <stdint.h>

//--------------------------------------------------------------------+
// Keycodes
//--------------------------------------------------------------------+

enum {
    // Internal keycodes
    KC_NO = 0x0000,
    KC_TRANSPARENT = 0x0001,

    // Basic keycodes
    KC_A = 0x0004,
    KC_B = 0x0005,
    KC_C = 0x0006,
    KC_D = 0x0007,
    KC_E = 0x0008,
    KC_F = 0x0009,
    KC_G = 0x000A,
    KC_H = 0x000B,
    KC_I = 0x000C,
    KC_J = 0x000D,
    KC_K = 0x000E,
    KC_L = 0x000F,
    KC_M = 0x0010,
    KC_N = 0x0011,
    KC_O = 0x0012,
    KC_P = 0x0013,
    KC_Q = 0x0014,
    KC_R = 0x0015,
    KC_S = 0x0016,
    KC_T = 0x0017,
    KC_U = 0x0018,
    KC_V = 0x0019,
    KC_W = 0x001A,
    KC_X = 0x001B,
    KC_Y = 0x001C,
    KC_Z = 0x001D,
    KC_1 = 0x001E,
    KC_2 = 0x001F,
    KC_3 = 0x0020,
    KC_4 = 0x0021,
    KC_5 = 0x0022,
    KC_6 = 0x0023,
    KC_7 = 0x0024,
    KC_8 = 0x0025,
    KC_9 = 0x0026,
    KC_0 = 0x0027,
    KC_ENTER = 0x0028,
    KC_ESCAPE = 0x0029,
    KC_BACKSPACE = 0x002A,
    KC_TAB = 0x002B,
    KC_SPACE = 0x002C,
    KC_MINUS = 0x002D,
    KC_EQUAL = 0x002E,
    KC_LEFT_BRACKET = 0x002F,
    KC_RIGHT_BRACKET = 0x0030,
    KC_BACKSLASH = 0x0031,
    KC_NONUS_HASH = 0x0032,
    KC_SEMICOLON = 0x0033,
    KC_QUOTE = 0x0034,
    KC_GRAVE = 0x0035,
    KC_COMMA = 0x0036,
    KC_DOT = 0x0037,
    KC_SLASH = 0x0038,
    KC_CAPS_LOCK = 0x0039,
    KC_F1 = 0x003A,
    KC_F2 = 0x003B,
    KC_F3 = 0x003C,
    KC_F4 = 0x003D,
    KC_F5 = 0x003E,
    KC_F6 = 0x003F,
    KC_F7 = 0x0040,
    KC_F8 = 0x0041,
    KC_F9 = 0x0042,
    KC_F10 = 0x0043,
    KC_F11 = 0x0044,
    KC_F12 = 0x0045,
    KC_PRINT_SCREEN = 0x0046,
    KC_SCROLL_LOCK = 0x0047,
    KC_PAUSE = 0x0048,
    KC_INSERT = 0x0049,
    KC_HOME = 0x004A,
    KC_PAGE_UP = 0x004B,
    KC_DELETE = 0x004C,
    KC_END = 0x004D,
    KC_PAGE_DOWN = 0x004E,
    KC_RIGHT = 0x004F,
    KC_LEFT = 0x0050,
    KC_DOWN = 0x0051,
    KC_UP = 0x0052,
    KC_NUM_LOCK = 0x0053,
    KC_KP_SLASH = 0x0054,
    KC_KP_ASTERISK = 0x0055,
    KC_KP_MINUS = 0x0056,
    KC_KP_PLUS = 0x0057,
    KC_KP_ENTER = 0x0058,
    KC_KP_1 = 0x0059,
    KC_KP_2 = 0x005A,
    KC_KP_3 = 0x005B,
    KC_KP_4 = 0x005C,
    KC_KP_5 = 0x005D,
    KC_KP_6 = 0x005E,
    KC_KP_7 = 0x005F,
    KC_KP_8 = 0x0060,
    KC_KP_9 = 0x0061,
    KC_KP_0 = 0x0062,
    KC_KP_DOT = 0x0063,
    KC_NONUS_BACKSLASH = 0x0064,
    KC_APPLICATION = 0x0065,
    KC_KB_POWER = 0x0066,
    KC_KP_EQUAL = 0x0067,
    KC_F13 = 0x0068,
    KC_F14 = 0x0069,
    KC_F15 = 0x006A,
    KC_F16 = 0x006B,
    KC_F17 = 0x006C,
    KC_F18 = 0x006D,
    KC_F19 = 0x006E,
    KC_F20 = 0x006F,
    KC_F21 = 0x0070,
    KC_F22 = 0x0071,
    KC_F23 = 0x0072,
    KC_F24 = 0x0073,
    KC_EXECUTE = 0x0074,
    KC_HELP = 0x0075,
    KC_MENU = 0x0076,
    KC_SELECT = 0x0077,
    KC_STOP = 0x0078,
    KC_AGAIN = 0x0079,
    KC_UNDO = 0x007A,
    KC_CUT = 0x007B,
    KC_COPY = 0x007C,
    KC_PASTE = 0x007D,
    KC_FIND = 0x007E,
    KC_KB_MUTE = 0x007F,
    KC_KB_VOLUME_UP = 0x0080,
    KC_KB_VOLUME_DOWN = 0x0081,
    KC_LOCKING_CAPS_LOCK = 0x0082,
    KC_LOCKING_NUM_LOCK = 0x0083,
    KC_LOCKING_SCROLL_LOCK = 0x0084,
    KC_KP_COMMA = 0x0085,
    KC_KP_EQUAL_AS400 = 0x0086,
    KC_INTERNATIONAL_1 = 0x0087,
    KC_INTERNATIONAL_2 = 0x0088,
    KC_INTERNATIONAL_3 = 0x0089,
    KC_INTERNATIONAL_4 = 0x008A,
    KC_INTERNATIONAL_5 = 0x008B,
    KC_INTERNATIONAL_6 = 0x008C,
    KC_INTERNATIONAL_7 = 0x008D,
    KC_INTERNATIONAL_8 = 0x008E,
    KC_INTERNATIONAL_9 = 0x008F,
    KC_LANGUAGE_1 = 0x0090,
    KC_LANGUAGE_2 = 0x0091,
    KC_LANGUAGE_3 = 0x0092,
    KC_LANGUAGE_4 = 0x0093,
    KC_LANGUAGE_5 = 0x0094,
    KC_LANGUAGE_6 = 0x0095,
    KC_LANGUAGE_7 = 0x0096,
    KC_LANGUAGE_8 = 0x0097,
    KC_LANGUAGE_9 = 0x0098,
    KC_ALTERNATE_ERASE = 0x0099,
    KC_SYSTEM_REQUEST = 0x009A,
    KC_CANCEL = 0x009B,
    KC_CLEAR = 0x009C,
    KC_PRIOR = 0x009D,
    KC_RETURN = 0x009E,
    KC_SEPARATOR = 0x009F,
    KC_OUT = 0x00A0,
    KC_OPER = 0x00A1,
    KC_CLEAR_AGAIN = 0x00A2,
    KC_CRSEL = 0x00A3,
    KC_EXSEL = 0x00A4,

    // System keycodes
    KC_SYSTEM_POWER = 0x00A5,
    KC_SYSTEM_SLEEP = 0x00A6,
    KC_SYSTEM_WAKE = 0x00A7,

    // Consumer keycodes
    KC_AUDIO_MUTE = 0x00A8,
    KC_AUDIO_VOL_UP = 0x00A9,
    KC_AUDIO_VOL_DOWN = 0x00AA,
    KC_MEDIA_NEXT_TRACK = 0x00AB,
    KC_MEDIA_PREV_TRACK = 0x00AC,
    KC_MEDIA_STOP = 0x00AD,
    KC_MEDIA_PLAY_PAUSE = 0x00AE,
    KC_MEDIA_SELECT = 0x00AF,
    KC_MEDIA_EJECT = 0x00B0,
    KC_MAIL = 0x00B1,
    KC_CALCULATOR = 0x00B2,
    KC_MY_COMPUTER = 0x00B3,
    KC_WWW_SEARCH = 0x00B4,
    KC_WWW_HOME = 0x00B5,
    KC_WWW_BACK = 0x00B6,
    KC_WWW_FORWARD = 0x00B7,
    KC_WWW_STOP = 0x00B8,
    KC_WWW_REFRESH = 0x00B9,
    KC_WWW_FAVORITES = 0x00BA,
    KC_MEDIA_FAST_FORWARD = 0x00BB,
    KC_MEDIA_REWIND = 0x00BC,
    KC_BRIGHTNESS_UP = 0x00BD,
    KC_BRIGHTNESS_DOWN = 0x00BE,
    KC_CONTROL_PANEL = 0x00BF,
    KC_ASSISTANT = 0x00C0,
    KC_MISSION_CONTROL = 0x00C1,
    KC_LAUNCHPAD = 0x00C2,

    // Modifiers
    KC_LEFT_CTRL = 0x00E0,
    KC_LEFT_SHIFT = 0x00E1,
    KC_LEFT_ALT = 0x00E2,
    KC_LEFT_GUI = 0x00E3,
    KC_RIGHT_CTRL = 0x00E4,
    KC_RIGHT_SHIFT = 0x00E5,
    KC_RIGHT_ALT = 0x00E6,
    KC_RIGHT_GUI = 0x00E7,

    // Advanced key keycodes
    SP_ADVANCED_KEY = 0x0100,
    SP_ADVANCED_KEY_MAX = 0x01FF,

    // Layer activation keycodes
    SP_LAYER_TO = 0x0200,
    SP_LAYER_TO_MAX = 0x020F,

    // Layer-momentary keycodes
    SP_LAYER_MO = 0x0210,
    SP_LAYER_MO_MAX = 0x021F,

    // Layer-default keycodes
    SP_LAYER_DEF = 0x0220,
    SP_LAYER_DEF_MAX = 0x022F,

    // Layer-toggle keycodes
    SP_LAYER_TOGGLE = 0x0230,
    SP_LAYER_TOGGLE_MAX = 0x023F,

    // Profile activation keycodes
    SP_PROFILE_TO = 0x0240,
    SP_PROFILE_TO_MAX = 0x024F,

    // Magic keycodes
    SP_MAGIC_BOOTLOADER = 0x1000,
    SP_MAGIC_REBOOT = 0x1001,
    SP_MAGIC_FACTORY_RESET = 0x1002,
    SP_MAGIC_RECALIBRATE = 0x1003,

    // Aliases
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
    KC_PEQL = KC_KP_EQUAL,
    KC_EXEC = KC_EXECUTE,
    KC_SLCT = KC_SELECT,
    KC_AGIN = KC_AGAIN,
    KC_PSTE = KC_PASTE,
    KC_LCAP = KC_LOCKING_CAPS_LOCK,
    KC_LNUM = KC_LOCKING_NUM_LOCK,
    KC_LSCR = KC_LOCKING_SCROLL_LOCK,
    KC_PCMM = KC_KP_COMMA,
    KC_INT1 = KC_INTERNATIONAL_1,
    KC_INT2 = KC_INTERNATIONAL_2,
    KC_INT3 = KC_INTERNATIONAL_3,
    KC_INT4 = KC_INTERNATIONAL_4,
    KC_INT5 = KC_INTERNATIONAL_5,
    KC_INT6 = KC_INTERNATIONAL_6,
    KC_INT7 = KC_INTERNATIONAL_7,
    KC_INT8 = KC_INTERNATIONAL_8,
    KC_INT9 = KC_INTERNATIONAL_9,
    KC_LNG1 = KC_LANGUAGE_1,
    KC_LNG2 = KC_LANGUAGE_2,
    KC_LNG3 = KC_LANGUAGE_3,
    KC_LNG4 = KC_LANGUAGE_4,
    KC_LNG5 = KC_LANGUAGE_5,
    KC_LNG6 = KC_LANGUAGE_6,
    KC_LNG7 = KC_LANGUAGE_7,
    KC_LNG8 = KC_LANGUAGE_8,
    KC_LNG9 = KC_LANGUAGE_9,
    KC_ERAS = KC_ALTERNATE_ERASE,
    KC_SYRQ = KC_SYSTEM_REQUEST,
    KC_CNCL = KC_CANCEL,
    KC_CLR = KC_CLEAR,
    KC_PRIR = KC_PRIOR,
    KC_RETN = KC_RETURN,
    KC_SEPR = KC_SEPARATOR,
    KC_CLAG = KC_CLEAR_AGAIN,
    KC_CRSL = KC_CRSEL,
    KC_EXSL = KC_EXSEL,
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
    KC_EJCT = KC_MEDIA_EJECT,
    KC_CALC = KC_CALCULATOR,
    KC_MYCM = KC_MY_COMPUTER,
    KC_WSCH = KC_WWW_SEARCH,
    KC_WHOM = KC_WWW_HOME,
    KC_WBAK = KC_WWW_BACK,
    KC_WFWD = KC_WWW_FORWARD,
    KC_WSTP = KC_WWW_STOP,
    KC_WREF = KC_WWW_REFRESH,
    KC_WFAV = KC_WWW_FAVORITES,
    KC_MFFD = KC_MEDIA_FAST_FORWARD,
    KC_MRWD = KC_MEDIA_REWIND,
    KC_BRIU = KC_BRIGHTNESS_UP,
    KC_BRID = KC_BRIGHTNESS_DOWN,
    KC_CPNL = KC_CONTROL_PANEL,
    KC_ASST = KC_ASSISTANT,
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
};

//--------------------------------------------------------------------+
// Keycode Ranges
//--------------------------------------------------------------------+

#define IS_BASIC_KEYCODE(keycode) (KC_A <= (keycode) && (keycode) <= KC_EXSEL)
#define IS_SYSTEM_KEYCODE(keycode)                                             \
    (KC_SYSTEM_POWER <= (keycode) && (keycode) <= KC_SYSTEM_WAKE)
#define IS_CONSUMER_KEYCODE(keycode)                                           \
    (KC_AUDIO_MUTE <= (keycode) && (keycode) <= KC_LAUNCHPAD)
#define IS_MODIFIER_KEYCODE(keycode)                                           \
    (KC_LEFT_CTRL <= (keycode) && (keycode) <= KC_RIGHT_GUI)
#define IS_HID_KEYCODE(keycode)                                                \
    (KC_NO <= (keycode) && (keycode) <= KC_RIGHT_GUI)

#define IS_ADVANCED_KEY_KEYCODE(keycode)                                       \
    (SP_ADVANCED_KEY <= (keycode) && (keycode) <= SP_ADVANCED_KEY_MAX)

#define IS_LAYER_TO_KEYCODE(keycode)                                           \
    (SP_LAYER_TO <= (keycode) && (keycode) <= SP_LAYER_TO_MAX)
#define IS_LAYER_MO_KEYCODE(keycode)                                           \
    (SP_LAYER_MO <= (keycode) && (keycode) <= SP_LAYER_MO_MAX)
#define IS_LAYER_DEF_KEYCODE(keycode)                                          \
    (SP_LAYER_DEF <= (keycode) && (keycode) <= SP_LAYER_DEF_MAX)
#define IS_LAYER_TOGGLE_KEYCODE(keycode)                                       \
    (SP_LAYER_TOGGLE <= (keycode) && (keycode) <= SP_LAYER_TOGGLE_MAX)

#define IS_PROFILE_TO_KEYCODE(keycode)                                         \
    (SP_PROFILE_TO <= (keycode) && (keycode) <= SP_PROFILE_TO_MAX)

#define IS_MAGIC_KEYCODE(keycode)                                              \
    (SP_MAGIC_BOOTLOADER <= (keycode) && (keycode) <= SP_MAGIC_RECALIBRATE)

//--------------------------------------------------------------------+
// Advanced Key Macros
//--------------------------------------------------------------------+

#define AK(index) ((index) | SP_ADVANCED_KEY)

#define SP_ADVANCED_KEY_GET_INDEX(keycode) ((keycode) & 0x00FF)

//--------------------------------------------------------------------+
// Layer Activation Macros
//--------------------------------------------------------------------+

#define TO(layer) ((layer) | SP_LAYER_TO)

#define SP_LAYER_TO_GET_LAYER(keycode) ((keycode) & 0x000F)

//--------------------------------------------------------------------+
// Layer-Momentary Macros
//--------------------------------------------------------------------+

#define MO(layer) ((layer) | SP_LAYER_MO)

#define SP_LAYER_MO_GET_LAYER(keycode) ((keycode) & 0x000F)

//--------------------------------------------------------------------+
// Layer-Default Macros
//--------------------------------------------------------------------+

#define DF(layer) ((layer) | SP_LAYER_DEF)

#define SP_LAYER_DEF_GET_LAYER(keycode) ((keycode) & 0x000F)

//--------------------------------------------------------------------+
// Layer-Toggle Macros
//--------------------------------------------------------------------+

#define TG(layer) ((layer) | SP_LAYER_TOGGLE)

#define SP_LAYER_TOGGLE_GET_LAYER(keycode) ((keycode) & 0x000F)

//--------------------------------------------------------------------+
// Profile Activation Macros
//--------------------------------------------------------------------+

#define PTO(profile) ((profile) | SP_PROFILE_TO)

#define SP_PROFILE_TO_GET_PROFILE(keycode) ((keycode) & 0x000F)

//--------------------------------------------------------------------+
// Keycode APIs
// WARNING: The APIs below do not check if the keycode is valid. Please make
// sure that the keycode is within the valid range before calling these APIs.
//--------------------------------------------------------------------+

/**
 * @brief Convert a keycode to a HID keyboard modifier
 *
 * @param keycode Keycode
 *
 * @return HID keyboard modifier
 */
uint8_t keycode_to_modifier(uint16_t keycode);

/**
 * @brief Convert a keycode to a HID system page key
 *
 * @param keycode Keycode
 *
 * @return HID system page key
 */
uint16_t keycode_to_system(uint16_t keycode);

/**
 * @brief Convert a keycode to a HID consumer page key
 *
 * @param keycode Keycode
 *
 * @return HID consumer page key
 */
uint16_t keycode_to_consumer(uint16_t keycode);
