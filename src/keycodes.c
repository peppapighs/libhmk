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

#include "keycodes.h"

#include "tusb.h"

const uint16_t keycode_to_hid[256] = {
    [KC_NO] = 0,
    [KC_TRANSPARENT] = 0,

    // Keyboard keycodes
    [KC_A] = HID_KEY_A,
    [KC_B] = HID_KEY_B,
    [KC_C] = HID_KEY_C,
    [KC_D] = HID_KEY_D,
    [KC_E] = HID_KEY_E,
    [KC_F] = HID_KEY_F,
    [KC_G] = HID_KEY_G,
    [KC_H] = HID_KEY_H,
    [KC_I] = HID_KEY_I,
    [KC_J] = HID_KEY_J,
    [KC_K] = HID_KEY_K,
    [KC_L] = HID_KEY_L,
    [KC_M] = HID_KEY_M,
    [KC_N] = HID_KEY_N,
    [KC_O] = HID_KEY_O,
    [KC_P] = HID_KEY_P,
    [KC_Q] = HID_KEY_Q,
    [KC_R] = HID_KEY_R,
    [KC_S] = HID_KEY_S,
    [KC_T] = HID_KEY_T,
    [KC_U] = HID_KEY_U,
    [KC_V] = HID_KEY_V,
    [KC_W] = HID_KEY_W,
    [KC_X] = HID_KEY_X,
    [KC_Y] = HID_KEY_Y,
    [KC_Z] = HID_KEY_Z,
    [KC_1] = HID_KEY_1,
    [KC_2] = HID_KEY_2,
    [KC_3] = HID_KEY_3,
    [KC_4] = HID_KEY_4,
    [KC_5] = HID_KEY_5,
    [KC_6] = HID_KEY_6,
    [KC_7] = HID_KEY_7,
    [KC_8] = HID_KEY_8,
    [KC_9] = HID_KEY_9,
    [KC_0] = HID_KEY_0,
    [KC_ENTER] = HID_KEY_ENTER,
    [KC_ESCAPE] = HID_KEY_ESCAPE,
    [KC_BACKSPACE] = HID_KEY_BACKSPACE,
    [KC_TAB] = HID_KEY_TAB,
    [KC_SPACE] = HID_KEY_SPACE,
    [KC_MINUS] = HID_KEY_MINUS,
    [KC_EQUAL] = HID_KEY_EQUAL,
    [KC_LEFT_BRACKET] = HID_KEY_BRACKET_LEFT,
    [KC_RIGHT_BRACKET] = HID_KEY_BRACKET_RIGHT,
    [KC_BACKSLASH] = HID_KEY_BACKSLASH,
    [KC_NONUS_HASH] = HID_KEY_EUROPE_1,
    [KC_SEMICOLON] = HID_KEY_SEMICOLON,
    [KC_QUOTE] = HID_KEY_APOSTROPHE,
    [KC_GRAVE] = HID_KEY_GRAVE,
    [KC_COMMA] = HID_KEY_COMMA,
    [KC_DOT] = HID_KEY_PERIOD,
    [KC_SLASH] = HID_KEY_SLASH,
    [KC_CAPS_LOCK] = HID_KEY_CAPS_LOCK,
    [KC_F1] = HID_KEY_F1,
    [KC_F2] = HID_KEY_F2,
    [KC_F3] = HID_KEY_F3,
    [KC_F4] = HID_KEY_F4,
    [KC_F5] = HID_KEY_F5,
    [KC_F6] = HID_KEY_F6,
    [KC_F7] = HID_KEY_F7,
    [KC_F8] = HID_KEY_F8,
    [KC_F9] = HID_KEY_F9,
    [KC_F10] = HID_KEY_F10,
    [KC_F11] = HID_KEY_F11,
    [KC_F12] = HID_KEY_F12,
    [KC_PRINT_SCREEN] = HID_KEY_PRINT_SCREEN,
    [KC_SCROLL_LOCK] = HID_KEY_SCROLL_LOCK,
    [KC_PAUSE] = HID_KEY_PAUSE,
    [KC_INSERT] = HID_KEY_INSERT,
    [KC_HOME] = HID_KEY_HOME,
    [KC_PAGE_UP] = HID_KEY_PAGE_UP,
    [KC_DELETE] = HID_KEY_DELETE,
    [KC_END] = HID_KEY_END,
    [KC_PAGE_DOWN] = HID_KEY_PAGE_DOWN,
    [KC_RIGHT] = HID_KEY_ARROW_RIGHT,
    [KC_LEFT] = HID_KEY_ARROW_LEFT,
    [KC_DOWN] = HID_KEY_ARROW_DOWN,
    [KC_UP] = HID_KEY_ARROW_UP,
    [KC_NUM_LOCK] = HID_KEY_NUM_LOCK,
    [KC_KP_SLASH] = HID_KEY_KEYPAD_DIVIDE,
    [KC_KP_ASTERISK] = HID_KEY_KEYPAD_MULTIPLY,
    [KC_KP_MINUS] = HID_KEY_KEYPAD_SUBTRACT,
    [KC_KP_PLUS] = HID_KEY_KEYPAD_ADD,
    [KC_KP_ENTER] = HID_KEY_KEYPAD_ENTER,
    [KC_KP_1] = HID_KEY_KEYPAD_1,
    [KC_KP_2] = HID_KEY_KEYPAD_2,
    [KC_KP_3] = HID_KEY_KEYPAD_3,
    [KC_KP_4] = HID_KEY_KEYPAD_4,
    [KC_KP_5] = HID_KEY_KEYPAD_5,
    [KC_KP_6] = HID_KEY_KEYPAD_6,
    [KC_KP_7] = HID_KEY_KEYPAD_7,
    [KC_KP_8] = HID_KEY_KEYPAD_8,
    [KC_KP_9] = HID_KEY_KEYPAD_9,
    [KC_KP_0] = HID_KEY_KEYPAD_0,
    [KC_KP_DOT] = HID_KEY_KEYPAD_DECIMAL,
    [KC_NONUS_BACKSLASH] = HID_KEY_EUROPE_2,
    [KC_APPLICATION] = HID_KEY_APPLICATION,
    [KC_F13] = HID_KEY_F13,
    [KC_F14] = HID_KEY_F14,
    [KC_F15] = HID_KEY_F15,
    [KC_F16] = HID_KEY_F16,
    [KC_F17] = HID_KEY_F17,
    [KC_F18] = HID_KEY_F18,
    [KC_F19] = HID_KEY_F19,
    [KC_F20] = HID_KEY_F20,
    [KC_F21] = HID_KEY_F21,
    [KC_F22] = HID_KEY_F22,
    [KC_F23] = HID_KEY_F23,
    [KC_F24] = HID_KEY_F24,
    [KC_INTERNATIONAL_1] = HID_KEY_KANJI1,
    [KC_INTERNATIONAL_2] = HID_KEY_KANJI2,
    [KC_INTERNATIONAL_3] = HID_KEY_KANJI3,
    [KC_INTERNATIONAL_4] = HID_KEY_KANJI4,
    [KC_INTERNATIONAL_5] = HID_KEY_KANJI5,
    [KC_INTERNATIONAL_6] = HID_KEY_KANJI6,
    [KC_LANGUAGE_1] = HID_KEY_LANG1,
    [KC_LANGUAGE_2] = HID_KEY_LANG2,
    [KC_LANGUAGE_3] = HID_KEY_LANG3,
    [KC_LANGUAGE_4] = HID_KEY_LANG4,
    [KC_LANGUAGE_5] = HID_KEY_LANG5,
    [KC_LANGUAGE_6] = HID_KEY_LANG6,

    // Modifier keycodes
    [KC_LEFT_CTRL] = KEYBOARD_MODIFIER_LEFTCTRL,
    [KC_LEFT_SHIFT] = KEYBOARD_MODIFIER_LEFTSHIFT,
    [KC_LEFT_ALT] = KEYBOARD_MODIFIER_LEFTALT,
    [KC_LEFT_GUI] = KEYBOARD_MODIFIER_LEFTGUI,
    [KC_RIGHT_CTRL] = KEYBOARD_MODIFIER_RIGHTCTRL,
    [KC_RIGHT_SHIFT] = KEYBOARD_MODIFIER_RIGHTSHIFT,
    [KC_RIGHT_ALT] = KEYBOARD_MODIFIER_RIGHTALT,
    [KC_RIGHT_GUI] = KEYBOARD_MODIFIER_RIGHTGUI,

    // System keycodes
    [KC_SYSTEM_POWER] = 0x0081, // (4.5 - System Power Down)
    [KC_SYSTEM_SLEEP] = 0x0082, // (4.5 - System Sleep)
    [KC_SYSTEM_WAKE] = 0x0083,  // (4.5 - System Wake Up)

    // Consumer keycodes
    [KC_AUDIO_MUTE] = 0x00E2,       // (15.9 - Mute)
    [KC_AUDIO_VOL_UP] = 0x00E9,     // (15.9 - Volume Increment)
    [KC_AUDIO_VOL_DOWN] = 0x00EA,   // (15.9 - Volume Decrement)
    [KC_MEDIA_NEXT_TRACK] = 0x00B5, // (15.7 - Scan Next Track)
    [KC_MEDIA_PREV_TRACK] = 0x00B6, // (15.7 - Scan Previous Track)
    [KC_MEDIA_STOP] = 0x00B7,       // (15.7 - Stop)
    [KC_MEDIA_PLAY_PAUSE] = 0x00CD, // (15.7 - Play/Pause)
    [KC_MEDIA_SELECT] = 0x0183,  // (15.15 - AL Consumer Control Configuration)
    [KC_MAIL] = 0x018A,          // (15.15 - AL Email Reader)
    [KC_CALCULATOR] = 0x0192,    // (15.15 - AL Calculator)
    [KC_MY_COMPUTER] = 0x0194,   // (15.15 - AL Local Machine Browser)
    [KC_WWW_SEARCH] = 0x0221,    // (15.16 - AC Search)
    [KC_WWW_HOME] = 0x0223,      // (15.16 - AC Home)
    [KC_WWW_BACK] = 0x0224,      // (15.16 - AC Back)
    [KC_WWW_FORWARD] = 0x0225,   // (15.16 - AC Forward)
    [KC_WWW_REFRESH] = 0x0227,   // (15.16 - AC Refresh)
    [KC_WWW_FAVORITES] = 0x022A, // (15.16 - AC Bookmarks)
    [KC_BRIGHTNESS_UP] = 0x006F, // (15.5 - Display Brightness Increment)
    [KC_BRIGHTNESS_DOWN] = 0x0070, // (15.5 - Display Brightness Decrement)
    [KC_MISSION_CONTROL] = 0x029F, // (15.16 - AC Desktop Show All Windows)
    [KC_LAUNCHPAD] = 0x02A0,       // (15.16 - AC Soft Key Left)

    // Mouse keycodes
    [SP_MOUSE_BUTTON_1] = MOUSE_BUTTON_LEFT,
    [SP_MOUSE_BUTTON_2] = MOUSE_BUTTON_RIGHT,
    [SP_MOUSE_BUTTON_3] = MOUSE_BUTTON_MIDDLE,
    [SP_MOUSE_BUTTON_4] = MOUSE_BUTTON_BACKWARD,
    [SP_MOUSE_BUTTON_5] = MOUSE_BUTTON_FORWARD,
};
