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

#include "gui/scrolllist.hpp"

class TerminalEngine;
class GUITerminal : public GUIScrollList, public InputObject
{
public:
    GUITerminal(xml_node<>* node);

public:
    // Update - Update any UI component animations (called <= 30 FPS)
    //  Return 0 if nothing to update, 1 on success and contiue, >1 if full render required, and <0 on error
    virtual int Update();

    // NotifyTouch - Notify of a touch event
    //  Return 0 on success, >0 to ignore remainder of touch, and <0 on error (Return error to allow other handlers)
    virtual int NotifyTouch(TOUCH_STATE state, int x, int y);

    // NotifyKey - Notify of a key press
    //  Return 0 on success (and consume key), >0 to pass key to next handler, and <0 on error
    virtual int NotifyKey(int key, bool down);

    // character input
    virtual int NotifyCharInput(int ch);

    // SetPageFocus - Notify when a page gains or loses focus
    virtual void SetPageFocus(int inFocus);

    // ScrollList interface
    virtual size_t GetItemCount();
    virtual void RenderItem(size_t itemindex, int yPos, bool selected);
    virtual void NotifySelect(size_t item_selected);

protected:
    void InitAndResize();

    TerminalEngine* engine; // non-visual parts of the terminal (text buffer etc.), not owned
    int updateCounter; // to track if anything changed in the back-end
    bool lastCondition; // to track if the condition became true and we might need to resize the terminal engine
};
