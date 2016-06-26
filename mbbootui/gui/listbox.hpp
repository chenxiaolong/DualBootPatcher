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

#include "gui/action.hpp"

class GUIListBox : public GUIScrollList
{
public:
    GUIListBox(xml_node<>* node);
    virtual ~GUIListBox();

public:
    // Update - Update any UI component animations (called <= 30 FPS)
    //  Return 0 if nothing to update, 1 on success and contiue, >1 if full render required, and <0 on error
    virtual int Update();

    // NotifyVarChange - Notify of a variable change
    virtual int NotifyVarChange(const std::string& varName, const std::string& value);

    // SetPageFocus - Notify when a page gains or loses focus
    virtual void SetPageFocus(int inFocus);

    virtual size_t GetItemCount();
    virtual void RenderItem(size_t itemindex, int yPos, bool selected);
    virtual void NotifySelect(size_t item_selected);

protected:
    struct ListItem
    {
        std::string displayName;
        std::string variableName;
        std::string variableValue;
        unsigned int selected;
        GUIAction* action;
        std::vector<Condition> mConditions;
    };

protected:
    std::vector<ListItem> mListItems;
    std::vector<size_t> mVisibleItems; // contains indexes in mListItems of visible items only
    std::string mVariable;
    std::string currentValue;
    ImageResource* mIconSelected;
    ImageResource* mIconUnselected;
    bool isCheckList;
    bool isTextParsed;
};
