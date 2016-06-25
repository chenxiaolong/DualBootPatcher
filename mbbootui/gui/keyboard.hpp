/*
 * Copyright 2013 bigbiff/Dees_Troy TeamWin
 * This file is part of TWRP/TeamWin Recovery Project.
 *
 * TWRP is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * TWRP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with TWRP.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "gui/objects.hpp"

// these are ASCII codes reported via NotifyCharInput
// other special keys (arrows etc.) are reported via NotifyKey
#define KEYBOARD_ACTION 13      // CR
#define KEYBOARD_BACKSPACE 8    // Backspace
#define KEYBOARD_TAB 9          // Tab
#define KEYBOARD_SWIPE_LEFT 21  // Ctrl+U to delete line, same as in readline (used by shell etc.)
#define KEYBOARD_SWIPE_RIGHT 11 // Ctrl+K, same as in readline

class GUIKeyboard : public GUIObject, public RenderObject, public ActionObject
{
public:
    GUIKeyboard(xml_node<>* node);
    virtual ~GUIKeyboard();

public:
    virtual int Render();
    virtual int Update();
    virtual int NotifyTouch(TOUCH_STATE state, int x, int y);
    virtual int SetRenderPos(int x, int y, int w = 0, int h = 0);
    virtual void SetPageFocus(int inFocus);

protected:
    struct Key
    {
        int key; // positive: ASCII/Unicode code; negative: Linux key code (KEY_*)
        int longpresskey;
        int end_x;
        int layout;
    };

    int ParseKey(const char* keyinfo, Key& key, int& Xindex, int keyWidth, bool longpress);
    void LoadKeyLabels(xml_node<>* parent, int layout);
    void DrawKey(Key& key, int keyX, int keyY, int keyW, int keyH);
    int KeyCharToCtrlChar(int key);

    enum
    {
        MAX_KEYBOARD_LAYOUTS = 5,
        MAX_KEYBOARD_ROWS = 9,
        MAX_KEYBOARD_KEYS = 20
    };

    struct Layout
    {
        ImageResource* keyboardImg;
        Key keys[MAX_KEYBOARD_ROWS][MAX_KEYBOARD_KEYS];
        int row_end_y[MAX_KEYBOARD_ROWS];
        bool is_caps;
        int revert_layout;
    };
    Layout layouts[MAX_KEYBOARD_LAYOUTS];

    struct KeyLabel
    {
        int key; // same as in struct Key
        int layout_from; // 1-based; 0 for labels that apply to all layouts
        int layout_to; // same as Key.layout
        std::string text; // key label text
        ImageResource* image; // image (overrides text if defined)
    };
    std::vector<KeyLabel> mKeyLabels;

    // Find key at screen coordinates
    Key* HitTestKey(int x, int y);

    bool mRendered;
    std::string mVariable;
    int currentLayout;
    bool CapsLockOn;
    static bool CtrlActive; // all keyboards share a common Control key state so that the Control key can be on a separate keyboard instance
    int highlightRenderCount;
    Key* currentKey;
    bool hasHighlight, hasCapsHighlight, hasCtrlHighlight;
    COLOR mHighlightColor;
    COLOR mCapsHighlightColor;
    COLOR mCtrlHighlightColor;
    COLOR mFontColor; // for centered key labels
    COLOR mFontColorSmall; // for centered key labels
    FontResource* mFont; // for main key labels
    FontResource* mSmallFont; // for key labels like "?123"
    FontResource* mLongpressFont; // for the small longpress label in the upper right corner
    int longpressOffsetX, longpressOffsetY; // distance of the longpress label from the key corner
    COLOR mLongpressFontColor;
    COLOR mBackgroundColor; // keyboard background color
    COLOR mKeyColorAlphanumeric; // key background color
    COLOR mKeyColorOther; // key background color
    int mKeyMarginX, mKeyMarginY; // space around key boxes - applied to left/right and top/bottom
};
