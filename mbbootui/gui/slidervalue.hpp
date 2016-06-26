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

class GUISliderValue: public GUIObject, public RenderObject, public ActionObject
{
public:
    GUISliderValue(xml_node<>* node);
    virtual ~GUISliderValue();

public:
    // Render - Render the full object to the GL surface
    //  Return 0 on success, <0 on error
    virtual int Render();

    // Update - Update any UI component animations (called <= 30 FPS)
    //  Return 0 if nothing to update, 1 on success and contiue, >1 if full render required, and <0 on error
    virtual int Update();

    // SetPos - Update the position of the render object
    //  Return 0 on success, <0 on error
    virtual int SetRenderPos(int x, int y, int w = 0, int h = 0);

    // NotifyTouch - Notify of a touch event
    //  Return 0 on success, >0 to ignore remainder of touch, and <0 on error
    virtual int NotifyTouch(TOUCH_STATE state, int x, int y);

    // Notify of a variable change
    virtual int NotifyVarChange(const std::string& varName, const std::string& value);

    // SetPageFocus - Notify when a page gains or loses focus
    virtual void SetPageFocus(int inFocus);

protected:
    int measureText(const std::string& str);
    int valueFromPct(float pct);
    float pctFromValue(int value);
    void loadValue(bool force = false);

    std::string mVariable;
    int mMax;
    int mMin;
    int mValue;
    char *mValueStr;
    float mValuePct;
    std::string mMaxStr;
    std::string mMinStr;
    FontResource *mFont;
    GUIText* mLabel;
    int mLabelW;
    COLOR mTextColor;
    COLOR mLineColor;
    COLOR mSliderColor;
    bool mShowRange;
    bool mShowCurr;
    int mLineX;
    int mLineY;
    int mLineH;
    int mLinePadding;
    int mPadding;
    int mSliderY;
    int mSliderW;
    int mSliderH;
    bool mRendered;
    int mFontHeight;
    GUIAction *mAction;
    bool mChangeOnDrag;
    int mLineW;
    bool mDragging;
    ImageResource *mBackgroundImage;
    ImageResource *mHandleImage;
    ImageResource *mHandleHoverImage;
};
