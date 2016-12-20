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

#include "gui/monotonic_cond.hpp"

// GUIAction - Used for standard actions
class GUIAction : public GUIObject, public ActionObject
{
    friend class ActionThread;

public:
    GUIAction(xml_node<>* node);

public:
    virtual int NotifyTouch(TOUCH_STATE state, int x, int y);
    virtual int NotifyKey(int key, bool down);
    virtual int NotifyVarChange(const std::string& varName,
                                const std::string& value);

    int doActions();

protected:
    class Action
    {
    public:
        std::string mFunction;
        std::string mArg;
    };

    std::vector<Action> mActions;
    std::unordered_map<int, bool> mKeys;

protected:
    enum ThreadType { THREAD_NONE, THREAD_ACTION, THREAD_CANCEL };

    enum class AutobootSignal { NONE, CANCEL, SKIP };

    int getKeyByName(const std::string& key);
    int doAction(const Action& action);
    ThreadType getThreadType(const Action& action);
    void operation_start(const std::string& operation_name);
    void operation_end(const int operation_status);
    time_t Start;

    // map action name to function pointer
    typedef int (GUIAction::*execFunction)(const std::string&);
    typedef std::unordered_map<std::string, execFunction> mapFunc;
    static mapFunc mf;
    static std::unordered_set<std::string> setActionsRunningInCallerThread;

    // GUI actions
    int reboot(const std::string& arg);
    int home(const std::string& arg);
    int key(const std::string& arg);
    int page(const std::string& arg);
    int reload(const std::string& arg);
    int set(const std::string& arg);
    int clear(const std::string& arg);
    int restoredefaultsettings(const std::string& arg);
    int compute(const std::string& arg);
    int setguitimezone(const std::string& arg);
    int overlay(const std::string& arg);
    int sleep(const std::string& arg);
    int screenshot(const std::string& arg);
    int setbrightness(const std::string& arg);

    // (originally) threaded actions
    int decrypt(const std::string& arg);
    int autoboot(const std::string& arg);
    int autoboot_cancel(const std::string& arg);
    int autoboot_skip(const std::string& arg);

    int switch_rom(const std::string& arg);
    int resize(const std::string& arg);
    int setlanguage(const std::string& arg);

private:
    static MonotonicCond s_autoboot_cond;
    static pthread_mutex_t s_autoboot_mutex;
    static volatile AutobootSignal s_autoboot_signal;
};
