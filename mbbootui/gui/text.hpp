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

// Derived Objects
// GUIText - Used for static text
class GUIText : public GUIObject, public RenderObject, public ActionObject
{
public:
    // w and h may be ignored, in which case, no bounding box is applied
    GUIText(xml_node<>* node);

public:
    // Render - Render the full object to the GL surface
    //  Return 0 on success, <0 on error
    virtual int Render();

    // Update - Update any UI component animations (called <= 30 FPS)
    //  Return 0 if nothing to update, 1 on success and contiue, >1 if full render required, and <0 on error
    virtual int Update();

    // Retrieve the size of the current string (dynamic strings may change per call)
    virtual int GetCurrentBounds(int& w, int& h);

    // Notify of a variable change
    virtual int NotifyVarChange(const std::string& varName, const std::string& value);

    // Set maximum width in pixels
    virtual int SetMaxWidth(unsigned width);

    void SetText(std::string newtext);

public:
    bool isHighlighted;
    bool scaleWidth;
    unsigned maxWidth;

protected:
    std::string mText;
    std::string mLastValue;
    COLOR mColor;
    COLOR mHighlightColor;
    FontResource* mFont;
    int mIsStatic;
    int mVarChanged;
    int mFontHeight;
};
