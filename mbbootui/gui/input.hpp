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

#include "gui/action.hpp"
#include "gui/text.hpp"

// GUIInput - Used for keyboard input
class GUIInput : public GUIObject, public RenderObject, public ActionObject, public InputObject
{
public:
    GUIInput(xml_node<>* node);
    virtual ~GUIInput();

public:
    // Render - Render the full object to the GL surface
    //  Return 0 on success, <0 on error
    virtual int Render();

    // Update - Update any UI component animations (called <= 30 FPS)
    //  Return 0 if nothing to update, 1 on success and contiue, >1 if full render required, and <0 on error
    virtual int Update();

    // Notify of a variable change
    virtual int NotifyVarChange(const std::string& varName, const std::string& value);

    // NotifyTouch - Notify of a touch event
    //  Return 0 on success, >0 to ignore remainder of touch, and <0 on error
    virtual int NotifyTouch(TOUCH_STATE state, int x, int y);

    virtual int NotifyKey(int key, bool down);
    virtual int NotifyCharInput(int ch);

protected:
    virtual int GetSelection(int x, int y);

    // Handles displaying the text properly when chars are added, deleted, or for scrolling
    void HandleTextLocation(int x);
    void UpdateDisplayText();
    void HandleCursorByTouch(int x);
    void HandleCursorByText();

protected:
    GUIText* mInputText;
    GUIAction* mAction;
    ImageResource* mBackground;
    ImageResource* mCursor;
    FontResource* mFont;
    std::string mVariable;
    std::string mMask;
    std::string mValue;
    std::string displayValue;
    COLOR mBackgroundColor;
    COLOR mCursorColor;
    int scrollingX;
    int cursorX;     // actual x axis location of the cursor
    int lastX;
    int mCursorLocation;
    int mBackgroundX, mBackgroundY, mBackgroundW, mBackgroundH;
    int mFontY;
    int textWidth;
    unsigned mFontHeight;
    unsigned CursorWidth;
    bool mRendered;
    bool HasMask;
    bool DrawCursor;
    bool isLocalChange;
    bool HasAllowed;
    bool HasDisabled;
    std::string AllowedList;
    std::string DisabledList;
    unsigned MinLen;
    unsigned MaxLen;
};
