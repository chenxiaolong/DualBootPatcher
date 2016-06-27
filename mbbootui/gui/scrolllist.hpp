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

class GUIScrollList : public GUIObject, public RenderObject, public ActionObject
{
public:
    GUIScrollList(xml_node<>* node);
    virtual ~GUIScrollList();

public:
    // Render - Render the full object to the GL surface
    //  Return 0 on success, <0 on error
    virtual int Render();

    // Update - Update any UI component animations (called <= 30 FPS)
    //  Return 0 if nothing to update, 1 on success and contiue, >1 if full render required, and <0 on error
    virtual int Update();

    // NotifyTouch - Notify of a touch event
    //  Return 0 on success, >0 to ignore remainder of touch, and <0 on error
    virtual int NotifyTouch(TOUCH_STATE state, int x, int y);

    // NotifyVarChange - Notify of a variable change
    virtual int NotifyVarChange(const std::string& varName, const std::string& value);

    // SetPos - Update the position of the render object
    //  Return 0 on success, <0 on error
    virtual int SetRenderPos(int x, int y, int w = 0, int h = 0);

    // SetPageFocus - Notify when a page gains or loses focus
    virtual void SetPageFocus(int inFocus);

protected:
    // derived classes need to implement these
    // get number of items
    virtual size_t GetItemCount()
    {
        return 0;
    }

    // render a single item in rect (mRenderX, yPos, mRenderW, actualItemHeight)
    virtual void RenderItem(size_t itemindex, int yPos, bool selected);

    // an item was selected
    virtual void NotifySelect(size_t item_selected __unused) {}

    // render a standard-layout list item with optional icon and text
    void RenderStdItem(int yPos, bool selected, ImageResource* icon, const char* text, int iconAndTextH = 0);

    enum { NO_ITEM = (size_t) -1 };

    // returns item index at coordinates or NO_ITEM if there is no item there
    size_t HitTestItem(int x, int y);

    // Called by the derived class to set the max icon size so we can calculate the proper actualItemHeight and center smaller icons if the icon size varies
    void SetMaxIconSize(int w, int h);

    // This will make sure that the item indicated by list_index is visible on the screen
    void SetVisibleListLocation(size_t list_index);

    // Handle scrolling changes for drags and kinetic scrolling
    void HandleScrolling();

    // Returns many full rows the list is capable of displaying
    int GetDisplayItemCount();

    // Returns the size in pixels of a partial item or row size
    int GetDisplayRemainder();

protected:
    // Background
    COLOR mBackgroundColor;
    ImageResource* mBackground; // background image, if any, automatically centered

    // Header
    COLOR mHeaderBackgroundColor;
    COLOR mHeaderFontColor;
    std::string mHeaderText; // Original header text without parsing any variables
    std::string mLastHeaderValue; // Header text after parsing variables
    bool mHeaderIsStatic; // indicates if the header is static (no need to check for changes in NotifyVarChange)
    int mHeaderH; // actual header height including font, icon, padding, and separator heights
    ImageResource* mHeaderIcon;
    int mHeaderIconHeight, mHeaderIconWidth; // width and height of the header icon if present
    int mHeaderSeparatorH; // Height of the separator between header and list items
    COLOR mHeaderSeparatorColor; // color of the header separator

    // Per-item layout
    FontResource* mFont;
    COLOR mFontColor;
    bool hasHighlightColor; // indicates if a highlight color was set
    COLOR mHighlightColor; // background row highlight color
    COLOR mFontHighlightColor;
    int mFontHeight;
    int actualItemHeight; // Actual height of each item in pixels including max icon size, font size, and padding
    int maxIconWidth, maxIconHeight; // max icon width and height for the list, set by derived class in SetMaxIconSize
    int mItemSpacing; // stores the spacing or padding on the y axis, part of the actualItemHeight
    int mSeparatorH; // Height of the separator between items
    COLOR mSeparatorColor; // color of the separator that is between items

    // Scrollbar
    int mFastScrollW; // width of the fastscroll area
    int mFastScrollLineW; // width of the line for fastscroll rendering
    int mFastScrollRectW; // width of the rectangle for fastscroll
    int mFastScrollRectH; // minimum height of the rectangle for fastscroll
    COLOR mFastScrollLineColor;
    COLOR mFastScrollRectColor;

    // Scrolling and dynamic state
    int mFastScrollRectCurrentY; // current top of fastscroll rect relative to list top
    int mFastScrollRectCurrentH; // current height of fastscroll rect
    int mFastScrollRectTouchY; // offset from top of fastscroll rect where the user initially touched
    bool hasScroll; // indicates that we have enough items in the list to scroll
    int firstDisplayedItem; // this item goes at the top of the display list - may only be partially visible
    int scrollingSpeed; // on a touch release, this is set based on the difference in the y-axis between the last 2 touches and indicates how fast the kinetic scrolling will go
    int y_offset; // this is how many pixels offset in the y axis for per pixel scrolling, is always <= 0 and should never be < -actualItemHeight
    bool allowSelection; // true if touched item can be selected, false for pure read-only lists and the console
    size_t selectedItem; // selected item index after the initial touch, set to -1 if we are scrolling
    int touchDebounce; // debounce for touches, minimum of 6 pixels but may be larger calculated based actualItemHeight / 3
    int lastY, last2Y; // last 2 touch locations, used for tracking kinetic scroll speed
    int fastScroll; // indicates that the inital touch was inside the fastscroll region - makes for easier fast scrolling as the touches don't have to stay within the fast scroll region and you drag your finger
    int mUpdate; // indicates that a change took place and we need to re-render
    bool AddLines(std::vector<std::string>* origText, std::vector<std::string>* origColor, size_t* lastCount, std::vector<std::string>* rText, std::vector<std::string>* rColor);
};
