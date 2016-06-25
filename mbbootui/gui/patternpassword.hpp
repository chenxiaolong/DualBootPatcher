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

class GUIPatternPassword : public GUIObject, public RenderObject, public ActionObject
{
public:
    GUIPatternPassword(xml_node<>* node);
    virtual ~GUIPatternPassword();

public:
    virtual int Render();
    virtual int Update();
    virtual int NotifyTouch(TOUCH_STATE state, int x, int y);
    virtual int NotifyVarChange(const std::string& varName,
                                const std::string& value);
    virtual int SetRenderPos(int x, int y, int w = 0, int h = 0);

protected:
    void CalculateDotPositions();
    void ResetActiveDots();
    void ConnectDot(int dot_idx);
    void ConnectIntermediateDots(int dot_idx);
    void Resize(size_t size);
    int InDot(int x, int y);
    bool DotUsed(int dot_idx);
    std::string GeneratePassphrase();
    void PatternDrawn();

    struct Dot
    {
        int x;
        int y;
        bool active;
    };

    std::string mSizeVar;
    size_t mGridSize;

    Dot* mDots;
    int* mConnectedDots;
    size_t mConnectedDotsLen;
    int mCurLineX;
    int mCurLineY;
    bool mTrackingTouch;
    bool mNeedRender;

    COLOR mDotColor;
    COLOR mActiveDotColor;
    COLOR mLineColor;
    ImageResource *mDotImage;
    ImageResource *mActiveDotImage;
    gr_surface mDotCircle;
    gr_surface mActiveDotCircle;
    int mDotRadius;
    int mLineWidth;

    std::string mPassVar;
    GUIAction *mAction;
    int mUpdate;
};
