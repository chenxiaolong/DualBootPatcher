/*
 * Copyright 2012 bigbiff/Dees_Troy TeamWin
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

#include "gui/blanktimer.hpp"

#include "minuitwrp/minui.h"

#include "data.hpp"
#include "twrp-functions.hpp"
#include "variables.h"

#include "config/config.hpp"
#include "gui/pages.hpp"

using namespace std::chrono;

blanktimer::blanktimer()
{
    pthread_mutex_init(&mutex, nullptr);
    setTime(0); // no timeout
    state = kOn;
    // WARNING: Deviation from TWRP. This is commented out to prevent
    // DataManager functions from being called before main()
    //orig_brightness = getBrightness();
}

bool blanktimer::isScreenOff()
{
    return state >= kOff;
}

void blanktimer::setTime(int newtime)
{
    pthread_mutex_lock(&mutex);
    sleepTimer = newtime;
    pthread_mutex_unlock(&mutex);
}

void blanktimer::setTimer()
{
    btimer = steady_clock::now();
}

void blanktimer::checkForTimeout()
{
    if (!(tw_device.tw_flags() & mb::device::TwFlag::NoScreenTimeout)) {
        pthread_mutex_lock(&mutex);
        auto curTime = steady_clock::now();
        auto diff = duration_cast<seconds>(curTime - btimer);
        if (sleepTimer > 2 && diff.count() > (sleepTimer - 2) && state == kOn) {
            orig_brightness = getBrightness();
            state = kDim;
            TWFunc::Set_Brightness("5");
        }
        if (sleepTimer && diff.count() > sleepTimer && state < kOff) {
            state = kOff;
            TWFunc::Set_Brightness("0");
            PageManager::ChangeOverlay("lock");
        }
        if (!(tw_device.tw_flags() & mb::device::TwFlag::NoScreenBlank)) {
            if (state == kOff) {
                gr_fb_blank(true);
                state = kBlanked;
            }
        }
        pthread_mutex_unlock(&mutex);
    }
}

std::string blanktimer::getBrightness()
{
    std::string result;

    if (DataManager::GetIntValue(VAR_TW_HAS_BRIGHTNESS_FILE)) {
        DataManager::GetValue(VAR_TW_BRIGHTNESS, result);
        if (result.empty()) {
            result = "255";
        }
    }
    return result;
}

void blanktimer::resetTimerAndUnblank()
{
    if (!(tw_device.tw_flags() & mb::device::TwFlag::NoScreenTimeout)) {
        pthread_mutex_lock(&mutex);
        setTimer();
        switch (state) {
        case kBlanked:
            if (!(tw_device.tw_flags() & mb::device::TwFlag::NoScreenBlank)) {
                gr_fb_blank(false);
            }
            // No break here, we want to keep going
        case kOff:
            gui_forceRender();
            // No break here, we want to keep going
        case kDim:
            if (!orig_brightness.empty()) {
                TWFunc::Set_Brightness(orig_brightness);
            }
            state = kOn;
        case kOn:
            break;
        }
        pthread_mutex_unlock(&mutex);
    }
}
