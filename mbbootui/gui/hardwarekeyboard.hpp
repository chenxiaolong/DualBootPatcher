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

#include <unordered_set>

class HardwareKeyboard
{
public:
    HardwareKeyboard();
    virtual ~HardwareKeyboard();

public:
    // called by the input handler for key events
    int KeyDown(int key_code);
    int KeyUp(int key_code);

    // called by the input handler when holding a key down
    int KeyRepeat();

    // called by multi-key actions to suppress key-release notifications
    void ConsumeKeyRelease(int key);

    bool IsKeyDown(int key_code);

private:
    int mLastKey;
    int mLastKeyChar;
    std::unordered_set<int> mPressedKeys;
};
