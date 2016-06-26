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

class MouseCursor : public RenderObject
{
public:
    MouseCursor(int posX, int posY);
    virtual ~MouseCursor();

    virtual int Render();
    virtual int Update();
    virtual int SetRenderPos(int x, int y, int w = 0, int h = 0);

    void Move(int deltaX, int deltaY);
    void GetPos(int& x, int& y);
    void LoadData(xml_node<>* node);
    void ResetData(int resX, int resY);

private:
    int m_resX;
    int m_resY;
    bool m_moved;
    float m_speedMultiplier;
    COLOR m_color;
    ImageResource *m_image;
    bool m_present;
};
