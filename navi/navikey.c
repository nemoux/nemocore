#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <navikey.h>

uint32_t nemonavi_translate_vkey(uint32_t code)
{
	static const NemoNaviVKeyCode vkeycodemap[] = {
		VKEY_UNKNOWN,       // 0x00:
		VKEY_UNKNOWN,       // 0x01:
		VKEY_UNKNOWN,       // 0x02:
		VKEY_UNKNOWN,       // 0x03:
		VKEY_UNKNOWN,       // 0x04:
		VKEY_UNKNOWN,       // 0x05:
		VKEY_UNKNOWN,       // 0x06:
		VKEY_UNKNOWN,       // XKB   evdev (XKB - 8)      X KeySym
		VKEY_UNKNOWN,       // ===   ===============      ======
		VKEY_ESCAPE,        // 0x09: KEY_ESC              Escape
		VKEY_1,             // 0x0A: KEY_1                1
		VKEY_2,             // 0x0B: KEY_2                2
		VKEY_3,             // 0x0C: KEY_3                3
		VKEY_4,             // 0x0D: KEY_4                4
		VKEY_5,             // 0x0E: KEY_5                5
		VKEY_6,             // 0x0F: KEY_6                6
		VKEY_7,             // 0x10: KEY_7                7
		VKEY_8,             // 0x11: KEY_8                8
		VKEY_9,             // 0x12: KEY_9                9
		VKEY_0,             // 0x13: KEY_0                0
		VKEY_OEM_MINUS,     // 0x14: KEY_MINUS            minus
		VKEY_OEM_PLUS,      // 0x15: KEY_EQUAL            equal
		VKEY_BACK,          // 0x16: KEY_BACKSPACE        BackSpace
		VKEY_TAB,           // 0x17: KEY_TAB              Tab
		VKEY_Q,             // 0x18: KEY_Q                q
		VKEY_W,             // 0x19: KEY_W                w
		VKEY_E,             // 0x1A: KEY_E                e
		VKEY_R,             // 0x1B: KEY_R                r
		VKEY_T,             // 0x1C: KEY_T                t
		VKEY_Y,             // 0x1D: KEY_Y                y
		VKEY_U,             // 0x1E: KEY_U                u
		VKEY_I,             // 0x1F: KEY_I                i
		VKEY_O,             // 0x20: KEY_O                o
		VKEY_P,             // 0x21: KEY_P                p
		VKEY_OEM_4,         // 0x22: KEY_LEFTBRACE        bracketleft
		VKEY_OEM_6,         // 0x23: KEY_RIGHTBRACE       bracketright
		VKEY_RETURN,        // 0x24: KEY_ENTER            Return
		VKEY_LCONTROL,      // 0x25: KEY_LEFTCTRL         Control_L
		VKEY_A,             // 0x26: KEY_A                a
		VKEY_S,             // 0x27: KEY_S                s
		VKEY_D,             // 0x28: KEY_D                d
		VKEY_F,             // 0x29: KEY_F                f
		VKEY_G,             // 0x2A: KEY_G                g
		VKEY_H,             // 0x2B: KEY_H                h
		VKEY_J,             // 0x2C: KEY_J                j
		VKEY_K,             // 0x2D: KEY_K                k
		VKEY_L,             // 0x2E: KEY_L                l
		VKEY_OEM_1,         // 0x2F: KEY_SEMICOLON        semicolon
		VKEY_OEM_7,         // 0x30: KEY_APOSTROPHE       apostrophe
		VKEY_OEM_3,         // 0x31: KEY_GRAVE            grave
		VKEY_LSHIFT,        // 0x32: KEY_LEFTSHIFT        Shift_L
		VKEY_OEM_5,         // 0x33: KEY_BACKSLASH        backslash
		VKEY_Z,             // 0x34: KEY_Z                z
		VKEY_X,             // 0x35: KEY_X                x
		VKEY_C,             // 0x36: KEY_C                c
		VKEY_V,             // 0x37: KEY_V                v
		VKEY_B,             // 0x38: KEY_B                b
		VKEY_N,             // 0x39: KEY_N                n
		VKEY_M,             // 0x3A: KEY_M                m
		VKEY_OEM_COMMA,     // 0x3B: KEY_COMMA            comma
		VKEY_OEM_PERIOD,    // 0x3C: KEY_DOT              period
		VKEY_OEM_2,         // 0x3D: KEY_SLASH            slash
		VKEY_RSHIFT,        // 0x3E: KEY_RIGHTSHIFT       Shift_R
		VKEY_MULTIPLY,      // 0x3F: KEY_KPASTERISK       KP_Multiply
		VKEY_LMENU,         // 0x40: KEY_LEFTALT          Alt_L
		VKEY_SPACE,         // 0x41: KEY_SPACE            space
		VKEY_CAPITAL,       // 0x42: KEY_CAPSLOCK         Caps_Lock
		VKEY_F1,            // 0x43: KEY_F1               F1
		VKEY_F2,            // 0x44: KEY_F2               F2
		VKEY_F3,            // 0x45: KEY_F3               F3
		VKEY_F4,            // 0x46: KEY_F4               F4
		VKEY_F5,            // 0x47: KEY_F5               F5
		VKEY_F6,            // 0x48: KEY_F6               F6
		VKEY_F7,            // 0x49: KEY_F7               F7
		VKEY_F8,            // 0x4A: KEY_F8               F8
		VKEY_F9,            // 0x4B: KEY_F9               F9
		VKEY_F10,           // 0x4C: KEY_F10              F10
		VKEY_NUMLOCK,       // 0x4D: KEY_NUMLOCK          Num_Lock
		VKEY_SCROLL,        // 0x4E: KEY_SCROLLLOCK       Scroll_Lock
		VKEY_NUMPAD7,       // 0x4F: KEY_KP7              KP_7
		VKEY_NUMPAD8,       // 0x50: KEY_KP8              KP_8
		VKEY_NUMPAD9,       // 0x51: KEY_KP9              KP_9
		VKEY_SUBTRACT,      // 0x52: KEY_KPMINUS          KP_Subtract
		VKEY_NUMPAD4,       // 0x53: KEY_KP4              KP_4
		VKEY_NUMPAD5,       // 0x54: KEY_KP5              KP_5
		VKEY_NUMPAD6,       // 0x55: KEY_KP6              KP_6
		VKEY_ADD,           // 0x56: KEY_KPPLUS           KP_Add
		VKEY_NUMPAD1,       // 0x57: KEY_KP1              KP_1
		VKEY_NUMPAD2,       // 0x58: KEY_KP2              KP_2
		VKEY_NUMPAD3,       // 0x59: KEY_KP3              KP_3
		VKEY_NUMPAD0,       // 0x5A: KEY_KP0              KP_0
		VKEY_DECIMAL,       // 0x5B: KEY_KPDOT            KP_Decimal
		VKEY_UNKNOWN,       // 0x5C:
		VKEY_DBE_DBCSCHAR,  // 0x5D: KEY_ZENKAKUHANKAKU   Zenkaku_Hankaku
		VKEY_OEM_5,         // 0x5E: KEY_102ND            backslash
		VKEY_F11,           // 0x5F: KEY_F11              F11
		VKEY_F12,           // 0x60: KEY_F12              F12
		VKEY_OEM_102,       // 0x61: KEY_RO               Romaji
		VKEY_UNSUPPORTED,   // 0x62: KEY_KATAKANA         Katakana
		VKEY_UNSUPPORTED,   // 0x63: KEY_HIRAGANA         Hiragana
		VKEY_CONVERT,       // 0x64: KEY_HENKAN           Henkan
		VKEY_UNSUPPORTED,   // 0x65: KEY_KATAKANAHIRAGANA Hiragana_Katakana
		VKEY_NONCONVERT,    // 0x66: KEY_MUHENKAN         Muhenkan
		VKEY_SEPARATOR,     // 0x67: KEY_KPJPCOMMA        KP_Separator
		VKEY_RETURN,        // 0x68: KEY_KPENTER          KP_Enter
		VKEY_RCONTROL,      // 0x69: KEY_RIGHTCTRL        Control_R
		VKEY_DIVIDE,        // 0x6A: KEY_KPSLASH          KP_Divide
		VKEY_PRINT,         // 0x6B: KEY_SYSRQ            Print
		VKEY_RMENU,         // 0x6C: KEY_RIGHTALT         Alt_R
		VKEY_RETURN,        // 0x6D: KEY_LINEFEED         Linefeed
		VKEY_HOME,          // 0x6E: KEY_HOME             Home
		VKEY_UP,            // 0x6F: KEY_UP               Up
		VKEY_PRIOR,         // 0x70: KEY_PAGEUP           Page_Up
		VKEY_LEFT,          // 0x71: KEY_LEFT             Left
		VKEY_RIGHT,         // 0x72: KEY_RIGHT            Right
		VKEY_END,           // 0x73: KEY_END              End
		VKEY_DOWN,          // 0x74: KEY_DOWN             Down
		VKEY_NEXT,          // 0x75: KEY_PAGEDOWN         Page_Down
		VKEY_INSERT,        // 0x76: KEY_INSERT           Insert
		VKEY_DELETE,        // 0x77: KEY_DELETE           Delete
		VKEY_UNSUPPORTED,   // 0x78: KEY_MACRO
		VKEY_VOLUME_MUTE,   // 0x79: KEY_MUTE             XF86AudioMute
		VKEY_VOLUME_DOWN,   // 0x7A: KEY_VOLUMEDOWN       XF86AudioLowerVolume
		VKEY_VOLUME_UP,     // 0x7B: KEY_VOLUMEUP         XF86AudioRaiseVolume
		VKEY_POWER,         // 0x7C: KEY_POWER            XF86PowerOff
		VKEY_OEM_PLUS,      // 0x7D: KEY_KPEQUAL          KP_Equal
		VKEY_UNSUPPORTED,   // 0x7E: KEY_KPPLUSMINUS      plusminus
		VKEY_PAUSE,         // 0x7F: KEY_PAUSE            Pause
		VKEY_MEDIA_LAUNCH_APP1,  // 0x80: KEY_SCALE            XF86LaunchA
		VKEY_DECIMAL,            // 0x81: KEY_KPCOMMA          KP_Decimal
		VKEY_HANGUL,             // 0x82: KEY_HANGUEL          Hangul
		VKEY_HANJA,              // 0x83: KEY_HANJA            Hangul_Hanja
		VKEY_OEM_5,              // 0x84: KEY_YEN              yen
		VKEY_LWIN,               // 0x85: KEY_LEFTMETA         Super_L
		VKEY_RWIN,               // 0x86: KEY_RIGHTMETA        Super_R
		VKEY_COMPOSE,            // 0x87: KEY_COMPOSE          Menu
	};

	return vkeycodemap[code + 8];
}

int nemonavi_is_character_vkey(uint32_t code)
{
	static const int vkeycodemap[] = {
		0,       // 0x00:
		0,       // 0x01:
		0,       // 0x02:
		0,       // 0x03:
		0,       // 0x04:
		0,       // 0x05:
		0,       // 0x06:
		0,       // XKB   evdev (XKB - 8)      X KeySym
		0,       // ===   ===============      ======
		0,       // 0x09: KEY_ESC              Escape
		1,       // 0x0A: KEY_1                1
		1,       // 0x0B: KEY_2                2
		1,       // 0x0C: KEY_3                3
		1,       // 0x0D: KEY_4                4
		1,       // 0x0E: KEY_5                5
		1,       // 0x0F: KEY_6                6
		1,       // 0x10: KEY_7                7
		1,       // 0x11: KEY_8                8
		1,       // 0x12: KEY_9                9
		1,       // 0x13: KEY_0                0
		1,       // 0x14: KEY_MINUS            minus
		1,       // 0x15: KEY_EQUAL            equal
		0,       // 0x16: KEY_BACKSPACE        BackSpace
		1,       // 0x17: KEY_TAB              Tab
		1,       // 0x18: KEY_Q                q
		1,       // 0x19: KEY_W                w
		1,       // 0x1A: KEY_E                e
		1,       // 0x1B: KEY_R                r
		1,       // 0x1C: KEY_T                t
		1,       // 0x1D: KEY_Y                y
		1,       // 0x1E: KEY_U                u
		1,       // 0x1F: KEY_I                i
		1,       // 0x20: KEY_O                o
		1,       // 0x21: KEY_P                p
		1,       // 0x22: KEY_LEFTBRACE        bracketleft
		1,       // 0x23: KEY_RIGHTBRACE       bracketright
		1,       // 0x24: KEY_ENTER            Return
		0,       // 0x25: KEY_LEFTCTRL         Control_L
		1,       // 0x26: KEY_A                a
		1,       // 0x27: KEY_S                s
		1,       // 0x28: KEY_D                d
		1,       // 0x29: KEY_F                f
		1,       // 0x2A: KEY_G                g
		1,       // 0x2B: KEY_H                h
		1,       // 0x2C: KEY_J                j
		1,       // 0x2D: KEY_K                k
		1,       // 0x2E: KEY_L                l
		1,       // 0x2F: KEY_SEMICOLON        semicolon
		1,       // 0x30: KEY_APOSTROPHE       apostrophe
		1,       // 0x31: KEY_GRAVE            grave
		0,       // 0x32: KEY_LEFTSHIFT        Shift_L
		1,       // 0x33: KEY_BACKSLASH        backslash
		1,       // 0x34: KEY_Z                z
		1,       // 0x35: KEY_X                x
		1,       // 0x36: KEY_C                c
		1,       // 0x37: KEY_V                v
		1,       // 0x38: KEY_B                b
		1,       // 0x39: KEY_N                n
		1,       // 0x3A: KEY_M                m
		1,       // 0x3B: KEY_COMMA            comma
		1,       // 0x3C: KEY_DOT              period
		1,       // 0x3D: KEY_SLASH            slash
		0,       // 0x3E: KEY_RIGHTSHIFT       Shift_R
		1,       // 0x3F: KEY_KPASTERISK       KP_Multiply
		0,       // 0x40: KEY_LEFTALT          Alt_L
		1,       // 0x41: KEY_SPACE            space
		0,       // 0x42: KEY_CAPSLOCK         Caps_Lock
		0,       // 0x43: KEY_F1               F1
		0,       // 0x44: KEY_F2               F2
		0,       // 0x45: KEY_F3               F3
		0,       // 0x46: KEY_F4               F4
		0,       // 0x47: KEY_F5               F5
		0,       // 0x48: KEY_F6               F6
		0,       // 0x49: KEY_F7               F7
		0,       // 0x4A: KEY_F8               F8
		0,       // 0x4B: KEY_F9               F9
		0,       // 0x4C: KEY_F10              F10
		0,       // 0x4D: KEY_NUMLOCK          Num_Lock
		0,       // 0x4E: KEY_SCROLLLOCK       Scroll_Lock
		0,       // 0x4F: KEY_KP7              KP_7
		0,       // 0x50: KEY_KP8              KP_8
		0,       // 0x51: KEY_KP9              KP_9
		1,       // 0x52: KEY_KPMINUS          KP_Subtract
		0,       // 0x53: KEY_KP4              KP_4
		0,       // 0x54: KEY_KP5              KP_5
		0,       // 0x55: KEY_KP6              KP_6
		1,       // 0x56: KEY_KPPLUS           KP_Add
		0,       // 0x57: KEY_KP1              KP_1
		0,       // 0x58: KEY_KP2              KP_2
		0,       // 0x59: KEY_KP3              KP_3
		0,       // 0x5A: KEY_KP0              KP_0
		1,       // 0x5B: KEY_KPDOT            KP_Decimal
		0,       // 0x5C:
		0,       // 0x5D: KEY_ZENKAKUHANKAKU   Zenkaku_Hankaku
		0,       // 0x5E: KEY_102ND            backslash
		0,       // 0x5F: KEY_F11              F11
		0,       // 0x60: KEY_F12              F12
		0,       // 0x61: KEY_RO               Romaji
		0,       // 0x62: KEY_KATAKANA         Katakana
		0,       // 0x63: KEY_HIRAGANA         Hiragana
		0,       // 0x64: KEY_HENKAN           Henkan
		0,       // 0x65: KEY_KATAKANAHIRAGANA Hiragana_Katakana
		0,       // 0x66: KEY_MUHENKAN         Muhenkan
		0,       // 0x67: KEY_KPJPCOMMA        KP_Separator
		0,       // 0x68: KEY_KPENTER          KP_Enter
		0,       // 0x69: KEY_RIGHTCTRL        Control_R
		1,       // 0x6A: KEY_KPSLASH          KP_Divide
		0,       // 0x6B: KEY_SYSRQ            Print
		0,       // 0x6C: KEY_RIGHTALT         Alt_R
		0,       // 0x6D: KEY_LINEFEED         Linefeed
		0,       // 0x6E: KEY_HOME             Home
		0,       // 0x6F: KEY_UP               Up
		0,       // 0x70: KEY_PAGEUP           Page_Up
		0,       // 0x71: KEY_LEFT             Left
		0,       // 0x72: KEY_RIGHT            Right
		0,       // 0x73: KEY_END              End
		0,       // 0x74: KEY_DOWN             Down
		0,       // 0x75: KEY_PAGEDOWN         Page_Down
		0,       // 0x76: KEY_INSERT           Insert
		0,       // 0x77: KEY_DELETE           Delete
		0,       // 0x78: KEY_MACRO
		0,       // 0x79: KEY_MUTE             XF86AudioMute
		0,       // 0x7A: KEY_VOLUMEDOWN       XF86AudioLowerVolume
		0,       // 0x7B: KEY_VOLUMEUP         XF86AudioRaiseVolume
		0,       // 0x7C: KEY_POWER            XF86PowerOff
		0,       // 0x7D: KEY_KPEQUAL          KP_Equal
		1,       // 0x7E: KEY_KPPLUSMINUS      plusminus
		0,       // 0x7F: KEY_PAUSE            Pause
		0,       // 0x80: KEY_SCALE            XF86LaunchA
		1,       // 0x81: KEY_KPCOMMA          KP_Decimal
		0,       // 0x82: KEY_HANGUEL          Hangul
		0,       // 0x83: KEY_HANJA            Hangul_Hanja
		0,       // 0x84: KEY_YEN              yen
		0,       // 0x85: KEY_LEFTMETA         Super_L
		0,       // 0x86: KEY_RIGHTMETA        Super_R
		0,       // 0x87: KEY_COMPOSE          Menu
	};

	return vkeycodemap[code + 8];
}
