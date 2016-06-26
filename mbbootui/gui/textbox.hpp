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

class GUITextBox : public GUIScrollList
{
public:
    GUITextBox(xml_node<>* node);

public:
    // Update - Update any UI component animations (called <= 30 FPS)
    //  Return 0 if nothing to update, 1 on success and contiue, >1 if full render required, and <0 on error
    virtual int Update();

    // NotifyVarChange - Notify of a variable change
    virtual int NotifyVarChange(const std::string& varName, const std::string& value);

    // ScrollList interface
    virtual size_t GetItemCount();
    virtual void RenderItem(size_t itemindex, int yPos, bool selected);
    virtual void NotifySelect(size_t item_selected);

protected:
    size_t mLastCount;
    bool mIsStatic;
    std::vector<std::string> mLastValue; // Parsed text - parsed for variables but not word wrapped
    std::vector<std::string> mText;      // Original text - not parsed for variables and not word wrapped
    std::vector<std::string> rText;      // Rendered text - what we actually see
};
