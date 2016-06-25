// hardwarekeyboard.cpp - HardwareKeyboard object

#include "gui/hardwarekeyboard.hpp"

#include <linux/input.h>

#include "gui/keyboard.hpp"

HardwareKeyboard::HardwareKeyboard() : mLastKeyChar(0)
{
}

HardwareKeyboard::~HardwareKeyboard()
{
}

// Map keys to other keys.
static int TranslateKeyCode(int key_code)
{
    switch (key_code) {
    case KEY_SLEEP: // Lock key on Asus Transformer hardware keyboard
        return KEY_POWER;
    }
    return key_code;
}

static int KeyCodeToChar(int key_code, bool shiftkey, bool ctrlkey)
{
    int keyboard = -1;

    switch (key_code) {
    case KEY_A:
        keyboard = shiftkey ? 'A' : 'a';
        break;
    case KEY_B:
        keyboard = shiftkey ? 'B' : 'b';
        break;
    case KEY_C:
        keyboard = shiftkey ? 'C' : 'c';
        break;
    case KEY_D:
        keyboard = shiftkey ? 'D' : 'd';
        break;
    case KEY_E:
        keyboard = shiftkey ? 'E' : 'e';
        break;
    case KEY_F:
        keyboard = shiftkey ? 'F' : 'f';
        break;
    case KEY_G:
        keyboard = shiftkey ? 'G' : 'g';
        break;
    case KEY_H:
        keyboard = shiftkey ? 'H' : 'h';
        break;
    case KEY_I:
        keyboard = shiftkey ? 'I' : 'i';
        break;
    case KEY_J:
        keyboard = shiftkey ? 'J' : 'j';
        break;
    case KEY_K:
        keyboard = shiftkey ? 'K' : 'k';
        break;
    case KEY_L:
        keyboard = shiftkey ? 'L' : 'l';
        break;
    case KEY_M:
        keyboard = shiftkey ? 'M' : 'm';
        break;
    case KEY_N:
        keyboard = shiftkey ? 'N' : 'n';
        break;
    case KEY_O:
        keyboard = shiftkey ? 'O' : 'o';
        break;
    case KEY_P:
        keyboard = shiftkey ? 'P' : 'p';
        break;
    case KEY_Q:
        keyboard = shiftkey ? 'Q' : 'q';
        break;
    case KEY_R:
        keyboard = shiftkey ? 'R' : 'r';
        break;
    case KEY_S:
        keyboard = shiftkey ? 'S' : 's';
        break;
    case KEY_T:
        keyboard = shiftkey ? 'T' : 't';
        break;
    case KEY_U:
        keyboard = shiftkey ? 'U' : 'u';
        break;
    case KEY_V:
        keyboard = shiftkey ? 'V' : 'v';
        break;
    case KEY_W:
        keyboard = shiftkey ? 'W' : 'w';
        break;
    case KEY_X:
        keyboard = shiftkey ? 'X' : 'x';
        break;
    case KEY_Y:
        keyboard = shiftkey ? 'Y' : 'y';
        break;
    case KEY_Z:
        keyboard = shiftkey ? 'Z' : 'z';
        break;
    case KEY_0:
        keyboard = shiftkey ? ')' : '0';
        break;
    case KEY_1:
        keyboard = shiftkey ? '!' : '1';
        break;
    case KEY_2:
        keyboard = shiftkey ? '@' : '2';
        break;
    case KEY_3:
        keyboard = shiftkey ? '#' : '3';
        break;
    case KEY_4:
        keyboard = shiftkey ? '$' : '4';
        break;
    case KEY_5:
        keyboard = shiftkey ? '%' : '5';
        break;
    case KEY_6:
        keyboard = shiftkey ? '^' : '6';
        break;
    case KEY_7:
        keyboard = shiftkey ? '&' : '7';
        break;
    case KEY_8:
        keyboard = shiftkey ? '*' : '8';
        break;
    case KEY_9:
        keyboard = shiftkey ? '(' : '9';
        break;
    case KEY_SPACE:
        keyboard = ' ';
        break;
    case KEY_BACKSPACE:
        keyboard = KEYBOARD_BACKSPACE;
        break;
    case KEY_TAB:
        keyboard = KEYBOARD_TAB;
        break;
    case KEY_ENTER:
        keyboard = KEYBOARD_ACTION;
        break;
    case KEY_SLASH:
        keyboard = shiftkey ? '?' : '/';
        break;
    case KEY_DOT:
        keyboard = shiftkey ? '>' : '.';
        break;
    case KEY_COMMA:
        keyboard = shiftkey ? '<' : ',';
        break;
    case KEY_MINUS:
        keyboard = shiftkey ? '_' : '-';
        break;
    case KEY_GRAVE:
        keyboard = shiftkey ? '~' : '`';
        break;
    case KEY_EQUAL:
        keyboard = shiftkey ? '+' : '=';
        break;
    case KEY_LEFTBRACE:
        keyboard = shiftkey ? '{' : '[';
        break;
    case KEY_RIGHTBRACE:
        keyboard = shiftkey ? '}' : ']';
        break;
    case KEY_BACKSLASH:
        keyboard = shiftkey ? '|' : '\\';
        break;
    case KEY_SEMICOLON:
        keyboard = shiftkey ? ':' : ';';
        break;
    case KEY_APOSTROPHE:
        keyboard = shiftkey ? '"' : '\'';
        break;

#ifdef _EVENT_LOGGING
    default:
        LOGE("Unmapped keycode: %i", key_code);
        break;
#endif
    }
    if (ctrlkey) {
        if (keyboard >= 96) {
            keyboard -= 96;
        } else {
            keyboard = -1;
        }
    }
    return keyboard;
}

bool HardwareKeyboard::IsKeyDown(int key_code)
{
    auto it = mPressedKeys.find(key_code);
    return (it != mPressedKeys.end());
}

int HardwareKeyboard::KeyDown(int key_code)
{
#ifdef _EVENT_LOGGING
    LOGE("HardwareKeyboard::KeyDown %i", key_code);
#endif
    key_code = TranslateKeyCode(key_code);
    mPressedKeys.insert(key_code);

    bool ctrlkey = IsKeyDown(KEY_LEFTCTRL) || IsKeyDown(KEY_RIGHTCTRL);
    bool shiftkey = IsKeyDown(KEY_LEFTSHIFT) || IsKeyDown(KEY_RIGHTSHIFT);

    int ch = KeyCodeToChar(key_code, shiftkey, ctrlkey);

    if (ch != -1) {
        mLastKeyChar = ch;
        if (!PageManager::NotifyCharInput(ch)) {
            return 1;  // Return 1 to enable key repeat
        }
    } else {
        mLastKeyChar = 0;
        mLastKey = key_code;
        if (!PageManager::NotifyKey(key_code, true)) {
            return 1;  // Return 1 to enable key repeat
        }
    }
    return 0;
}

int HardwareKeyboard::KeyUp(int key_code)
{
#ifdef _EVENT_LOGGING
    LOGE("HardwareKeyboard::KeyUp %i", key_code);
#endif
    key_code = TranslateKeyCode(key_code);
    auto itr = mPressedKeys.find(key_code);
    if (itr != mPressedKeys.end()) {
        mPressedKeys.erase(itr);
        PageManager::NotifyKey(key_code, false);
    }
    return 0;
}

int HardwareKeyboard::KeyRepeat()
{
#ifdef _EVENT_LOGGING
    LOGE("HardwareKeyboard::KeyRepeat: %i", mLastKeyChar);
#endif
    if (mLastKeyChar) {
        PageManager::NotifyCharInput(mLastKeyChar);
    } else if (mLastKey) {
        PageManager::NotifyKey(mLastKey, true);
    }
    return 0;
}

void HardwareKeyboard::ConsumeKeyRelease(int key)
{
    // causes following KeyUp event to suppress notifications
    mPressedKeys.erase(key);
}
