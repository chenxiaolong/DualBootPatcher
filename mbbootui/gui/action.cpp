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

#include "gui/action.hpp"

#include <algorithm>

#include <cstring>

#include <linux/input.h>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mbdevice/device.h"
#include "mblog/logging.h"
#include "mbutil/directory.h"
#include "mbutil/file.h"
#include "mbutil/path.h"
#include "mbutil/properties.h"
#include "mbutil/string.h"

#include "config/config.hpp"

#include "daemon_connection.h"
#include "data.hpp"
#include "twrp-functions.hpp"
#include "variables.h"

#include "gui/blanktimer.hpp"
#include "gui/gui.h"
#include "gui/hardwarekeyboard.hpp"

GUIAction::mapFunc GUIAction::mf;
std::unordered_set<std::string> GUIAction::setActionsRunningInCallerThread;

MonotonicCond GUIAction::s_autoboot_cond;
pthread_mutex_t GUIAction::s_autoboot_mutex = PTHREAD_MUTEX_INITIALIZER;
volatile GUIAction::AutobootSignal GUIAction::s_autoboot_signal =
    GUIAction::AutobootSignal::NONE;

static void *ActionThread_work_wrapper(void *data);

class ActionThread
{
public:
    ActionThread();
    ~ActionThread();

    void threadActions(GUIAction *act);
    void run(void *data);
private:
    friend void *ActionThread_work_wrapper(void*);
    struct ThreadData
    {
        ActionThread *this_;
        GUIAction *act;
        ThreadData(ActionThread *this_, GUIAction *act) : this_(this_), act(act) {}
    };

    pthread_t m_thread;
    bool m_thread_running;
    pthread_mutex_t m_act_lock;
};

static ActionThread action_thread; // for all kinds of longer running actions
static ActionThread cancel_thread; // for longer running "cancel" actions

static void *ActionThread_work_wrapper(void *data)
{
    static_cast<ActionThread::ThreadData*>(data)->this_->run(data);
    return nullptr;
}

ActionThread::ActionThread()
{
    m_thread_running = false;
    pthread_mutex_init(&m_act_lock, nullptr);
}

ActionThread::~ActionThread()
{
    pthread_mutex_lock(&m_act_lock);
    if (m_thread_running) {
        pthread_mutex_unlock(&m_act_lock);
        pthread_join(m_thread, nullptr);
    } else {
        pthread_mutex_unlock(&m_act_lock);
    }
    pthread_mutex_destroy(&m_act_lock);
}

void ActionThread::threadActions(GUIAction *act)
{
    pthread_mutex_lock(&m_act_lock);
    if (m_thread_running) {
        pthread_mutex_unlock(&m_act_lock);
        LOGE("Another threaded action is already running -- not running %zu actions starting with '%s'",
             act->mActions.size(), act->mActions[0].mFunction.c_str());
    } else {
        m_thread_running = true;
        pthread_mutex_unlock(&m_act_lock);
        ThreadData *d = new ThreadData(this, act);
        pthread_create(&m_thread, nullptr, &ActionThread_work_wrapper, d);
    }
}

void ActionThread::run(void *data)
{
    ThreadData *d = (ThreadData*) data;
    GUIAction* act = d->act;

    for (auto it = act->mActions.begin(); it != act->mActions.end(); ++it) {
        act->doAction(*it);
    }

    pthread_mutex_lock(&m_act_lock);
    m_thread_running = false;
    pthread_mutex_unlock(&m_act_lock);
    delete d;
}

GUIAction::GUIAction(xml_node<>* node)
    : GUIObject(node)
{
    xml_node<>* child;
    xml_node<>* actions;
    xml_attribute<>* attr;

    if (!node) {
        return;
    }

    if (mf.empty()) {
#define ADD_ACTION(n) mf[#n] = &GUIAction::n
#define ADD_ACTION_EX(name, func) mf[name] = &GUIAction::func
        // These actions will be run in the caller's thread
        ADD_ACTION(reboot);
        ADD_ACTION(home);
        ADD_ACTION(key);
        ADD_ACTION(page);
        ADD_ACTION(reload);
        ADD_ACTION(set);
        ADD_ACTION(clear);
        ADD_ACTION(restoredefaultsettings);
        ADD_ACTION(compute);
        ADD_ACTION_EX("addsubtract", compute);
        ADD_ACTION(setguitimezone);
        ADD_ACTION(overlay);
        ADD_ACTION(sleep);
        ADD_ACTION(screenshot);
        ADD_ACTION(setbrightness);
        ADD_ACTION(setlanguage);
        ADD_ACTION(autoboot_cancel);
        ADD_ACTION(autoboot_skip);

        // remember actions that run in the caller thread
        for (auto it = mf.cbegin(); it != mf.cend(); ++it) {
            setActionsRunningInCallerThread.insert(it->first);
        }

        // These actions will run in a separate thread
        ADD_ACTION(autoboot);
        ADD_ACTION(switch_rom);
    }

    // First, get the action
    actions = FindNode(node, "actions");
    if (actions) {
        child = FindNode(actions, "action");
    } else {
        child = FindNode(node, "action");
    }

    if (!child) {
        return;
    }

    while (child) {
        Action action;

        attr = child->first_attribute("function");
        if (!attr) {
            return;
        }

        action.mFunction = attr->value();
        action.mArg = child->value();
        mActions.push_back(action);

        child = child->next_sibling("action");
    }

    // Now, let's get either the key or region
    child = FindNode(node, "touch");
    if (child) {
        attr = child->first_attribute("key");
        if (attr) {
            std::vector<std::string> keys = mb::util::split(attr->value(), "+");
            for (size_t i = 0; i < keys.size(); ++i) {
                const int key = getKeyByName(keys[i]);
                mKeys[key] = false;
            }
        } else {
            attr = child->first_attribute("x");
            if (!attr) {
                return;
            }
            mActionX = atol(attr->value());
            attr = child->first_attribute("y");
            if (!attr) {
                return;
            }
            mActionY = atol(attr->value());
            attr = child->first_attribute("w");
            if (!attr) {
                return;
            }
            mActionW = atol(attr->value());
            attr = child->first_attribute("h");
            if (!attr) {
                return;
            }
            mActionH = atol(attr->value());
        }
    }
}

int GUIAction::NotifyTouch(TOUCH_STATE state, int x __unused, int y __unused)
{
    if (state == TOUCH_RELEASE) {
        doActions();
    }

    return 0;
}

int GUIAction::NotifyKey(int key, bool down)
{
    auto itr = mKeys.find(key);
    if (itr == mKeys.end()) {
        return 1;
    }

    bool prevState = itr->second;
    itr->second = down;

    // If there is only one key for this action, wait for key up so it
    // doesn't trigger with multi-key actions.
    // Else, check if all buttons are pressed, then consume their release events
    // so they don't trigger one-button actions and reset mKeys pressed status
    if (mKeys.size() == 1) {
        if (!down && prevState) {
            doActions();
            return 0;
        }
    } else if (down) {
        for (itr = mKeys.begin(); itr != mKeys.end(); ++itr) {
            if (!itr->second) {
                return 1;
            }
        }

        // Passed, all req buttons are pressed, reset them and consume release events
        HardwareKeyboard *kb = PageManager::GetHardwareKeyboard();
        for (itr = mKeys.begin(); itr != mKeys.end(); ++itr) {
            kb->ConsumeKeyRelease(itr->first);
            itr->second = false;
        }

        doActions();
        return 0;
    }

    return 1;
}

int GUIAction::NotifyVarChange(const std::string& varName,
                               const std::string& value)
{
    GUIObject::NotifyVarChange(varName, value);

    if (varName.empty() && !isConditionValid() && mKeys.empty() && !mActionW) {
        doActions();
    } else if ((varName.empty() || IsConditionVariable(varName)) && isConditionValid() && isConditionTrue()) {
        doActions();
    }

    return 0;
}

GUIAction::ThreadType GUIAction::getThreadType(const Action& action)
{
    std::string func = gui_parse_text(action.mFunction);
    bool needsThread = setActionsRunningInCallerThread.find(func) == setActionsRunningInCallerThread.end();
    if (needsThread) {
        return THREAD_ACTION;
    }
    return THREAD_NONE;
}

int GUIAction::doActions()
{
    if (mActions.size() < 1) {
        return -1;
    }

    // Determine in which thread to run the actions.
    // Do it for all actions at once before starting, so that we can cancel the whole batch if the thread is already busy.
    ThreadType threadType = THREAD_NONE;
    for (auto it = mActions.begin(); it != mActions.end(); ++it) {
        ThreadType tt = getThreadType(*it);
        if (tt == THREAD_NONE) {
            continue;
        }
        if (threadType == THREAD_NONE) {
            threadType = tt;
        } else if (threadType != tt) {
            LOGE("Can't mix normal and cancel actions in the same list. "
                 "Running the whole batch in the cancel thread.");
            threadType = THREAD_CANCEL;
            break;
        }
    }

    // Now run the actions in the desired thread.
    switch (threadType) {
    case THREAD_ACTION:
        action_thread.threadActions(this);
        break;

    case THREAD_CANCEL:
        cancel_thread.threadActions(this);
        break;

    default: {
        // no iterators here because theme reloading might kill our object
        const size_t cnt = mActions.size();
        for (size_t i = 0; i < cnt; ++i) {
            doAction(mActions[i]);
        }
    }
    }

    return 0;
}

int GUIAction::doAction(const Action& action)
{
    std::string function = gui_parse_text(action.mFunction);
    std::string arg = gui_parse_text(action.mArg);

    // find function and execute it
    auto funcitr = mf.find(function);
    if (funcitr != mf.end()) {
        return (this->*funcitr->second)(arg);
    }

    LOGE("Unknown action '%s'", function.c_str());
    return -1;
}

void GUIAction::operation_start(const std::string& operation_name)
{
    LOGI("operation_start: '%s'", operation_name.c_str());
    time(&Start);
    DataManager::SetValue(VAR_TW_ACTION_BUSY, 1);
    DataManager::SetValue(VAR_TW_UI_PROGRESS, 0);
    DataManager::SetValue(VAR_TW_OPERATION, operation_name);
    DataManager::SetValue(VAR_TW_OPERATION_STATE, 0);
    DataManager::SetValue(VAR_TW_OPERATION_STATUS, 0);
}

void GUIAction::operation_end(const int operation_status)
{
    time_t Stop;
    DataManager::SetValue(VAR_TW_UI_PROGRESS, 100);
    if (operation_status != 0) {
        DataManager::SetValue(VAR_TW_OPERATION_STATUS, 1);
    } else {
        DataManager::SetValue(VAR_TW_OPERATION_STATUS, 0);
    }
    DataManager::SetValue(VAR_TW_OPERATION_STATE, 1);
    DataManager::SetValue(VAR_TW_ACTION_BUSY, 0);
    blankTimer.resetTimerAndUnblank();
    time(&Stop);
    if ((int) difftime(Stop, Start) > 10) {
        DataManager::Vibrate(VAR_TW_ACTION_VIBRATE);
    }
    LOGI("operation_end - status=%d", operation_status);
}

int GUIAction::reboot(const std::string& arg)
{
    sync();
    DataManager::SetValue(VAR_TW_GUI_DONE, 1);
    DataManager::SetValue(VAR_TW_EXIT_ACTION, arg);

    return 0;
}

int GUIAction::home(const std::string& arg __unused)
{
    gui_changePage("main");
    return 0;
}

int GUIAction::key(const std::string& arg)
{
    const int key = getKeyByName(arg);
    PageManager::NotifyKey(key, true);
    PageManager::NotifyKey(key, false);
    return 0;
}

int GUIAction::page(const std::string& arg)
{
    std::string page_name = gui_parse_text(arg);
    return gui_changePage(page_name);
}

int GUIAction::reload(const std::string& arg __unused)
{
    PageManager::RequestReload();
    // The actual reload is handled in pages.cpp in PageManager::RunReload()
    // The reload will occur on the next Update or Render call and will
    // be performed in the rendoer thread instead of the action thread
    // to prevent crashing which could occur when we start deleting
    // GUI resources in the action thread while we attempt to render
    // with those same resources in another thread.
    return 0;
}

int GUIAction::set(const std::string& arg)
{
    if (arg.find('=') != std::string::npos) {
        std::string varName = arg.substr(0, arg.find('='));
        std::string value = arg.substr(arg.find('=') + 1, std::string::npos);

        DataManager::GetValue(value, value);
        DataManager::SetValue(varName, value);
    } else {
        DataManager::SetValue(arg, "1");
    }
    return 0;
}

int GUIAction::clear(const std::string& arg)
{
    DataManager::SetValue(arg, "0");
    return 0;
}

int GUIAction::restoredefaultsettings(const std::string& arg __unused)
{
    operation_start("Restore Defaults");
    DataManager::ResetDefaults();
    operation_end(0);
    return 0;
}

int GUIAction::compute(const std::string& arg)
{
    if (arg.find("+") != std::string::npos) {
        std::string varName = arg.substr(0, arg.find('+'));
        std::string string_to_add = arg.substr(arg.find('+') + 1, std::string::npos);
        int amount_to_add = atoi(string_to_add.c_str());
        int value;

        DataManager::GetValue(varName, value);
        DataManager::SetValue(varName, value + amount_to_add);
        return 0;
    }
    if (arg.find("-") != std::string::npos) {
        std::string varName = arg.substr(0, arg.find('-'));
        std::string string_to_subtract = arg.substr(arg.find('-') + 1, std::string::npos);
        int amount_to_subtract = atoi(string_to_subtract.c_str());
        int value;

        DataManager::GetValue(varName, value);
        value -= amount_to_subtract;
        if (value <= 0) {
            value = 0;
        }
        DataManager::SetValue(varName, value);
        return 0;
    }
    if (arg.find("*") != std::string::npos) {
        std::string varName = arg.substr(0, arg.find('*'));
        std::string multiply_by_str = gui_parse_text(arg.substr(arg.find('*') + 1, std::string::npos));
        int multiply_by = atoi(multiply_by_str.c_str());
        int value;

        DataManager::GetValue(varName, value);
        DataManager::SetValue(varName, value*multiply_by);
        return 0;
    }
    if (arg.find("/") != std::string::npos) {
        std::string varName = arg.substr(0, arg.find('/'));
        std::string divide_by_str = gui_parse_text(arg.substr(arg.find('/') + 1, std::string::npos));
        int divide_by = atoi(divide_by_str.c_str());
        int value;

        if (divide_by != 0) {
            DataManager::GetValue(varName, value);
            DataManager::SetValue(varName, value/divide_by);
        }
        return 0;
    }
    LOGE("Unable to perform compute '%s'", arg.c_str());
    return -1;
}

int GUIAction::setguitimezone(const std::string& arg __unused)
{
    std::string SelectedZone;
    DataManager::GetValue(VAR_TW_TIME_ZONE_GUISEL, SelectedZone); // read the selected time zone into SelectedZone
    std::string Zone = SelectedZone.substr(0, SelectedZone.find(';')); // parse to get time zone
    std::string DSTZone = SelectedZone.substr(SelectedZone.find(';') + 1, std::string::npos); // parse to get DST component

    int dst;
    DataManager::GetValue(VAR_TW_TIME_ZONE_GUIDST, dst); // check wether user chose to use DST

    std::string offset;
    DataManager::GetValue(VAR_TW_TIME_ZONE_GUIOFFSET, offset); // pull in offset

    std::string NewTimeZone = Zone;
    if (offset != "0") {
        NewTimeZone += ":" + offset;
    }

    if (dst != 0) {
        NewTimeZone += DSTZone;
    }

    DataManager::SetValue(VAR_TW_TIME_ZONE, NewTimeZone);
    DataManager::update_tz_environment_variables();
    return 0;
}

int GUIAction::overlay(const std::string& arg)
{
    return gui_changeOverlay(arg);
}

int GUIAction::sleep(const std::string& arg)
{
    operation_start("Sleep");
    usleep(atoi(arg.c_str()));
    operation_end(0);
    return 0;
}

int GUIAction::screenshot(const std::string& arg __unused)
{
    time_t tm;
    char path[256];
    size_t path_len;
    int ret = 0;

    strlcpy(path, tw_screenshots_path.c_str(), sizeof(path));
    strlcat(path, "/", sizeof(path));

    if (!mb::util::mkdir_recursive(path, 0755)) {
        ret = -1;
    }

    tm = time(nullptr);
    path_len = strlen(path);

    // Screenshot_2014-01-01-18-21-38.png
    if (ret == 0 && strftime(path + path_len, sizeof(path) - path_len,
                             "Screenshot_%Y-%m-%d-%H-%M-%S.png",
                             localtime(&tm)) == 0) {
        ret = -1;
    }

    if (ret == 0) {
        ret = gr_save_screenshot(path);
    }
    if (ret == 0) {
        gui_msg(Msg("screenshot_saved=Screenshot was saved to {1}")(path));

        // blink to notify that the screenshow was taken
        gr_color(255, 255, 255, 255);
        gr_fill(0, 0, gr_fb_width(), gr_fb_height());
        gr_flip();
        gui_forceRender();
    } else {
        gui_err("screenshot_err=Failed to take a screenshot!");
    }
    return 0;
}

int GUIAction::setbrightness(const std::string& arg)
{
    return TWFunc::Set_Brightness(arg);
}

int GUIAction::autoboot(const std::string& arg __unused)
{
    struct timespec wait_end;
    struct timespec now;
    AutobootSignal signal = AutobootSignal::NONE;

    pthread_mutex_lock(&s_autoboot_mutex);
    s_autoboot_signal = AutobootSignal::NONE;
    pthread_mutex_unlock(&s_autoboot_mutex);

    operation_start("autoboot");

    gui_msg(Msg("autoboot_welcome")(static_cast<std::string>(Msg("app_name"))));
    gui_msg(Msg("autoboot_version_bootui")(DataManager::GetStrValue(VAR_TW_VERSION)));
    gui_msg(Msg("autoboot_version_mbtool")(DataManager::GetStrValue(VAR_TW_MBTOOL_VERSION)));

    gui_msg(Msg("autoboot_autobooting_to")(DataManager::GetStrValue(VAR_TW_ROM_ID)));

    int timeout = DataManager::GetIntValue(VAR_TW_AUTOBOOT_TIMEOUT);

    for (int i = timeout; i > 0; --i) {
        gui_msg(Msg("autoboot_booting_in")(i));

        clock_gettime(CLOCK_MONOTONIC, &now);

        wait_end.tv_sec = now.tv_sec + 1;
        wait_end.tv_nsec = now.tv_nsec;

        pthread_mutex_lock(&s_autoboot_mutex);
        pthread_cond_t *cond = s_autoboot_cond;
        if (cond) {
            pthread_cond_timedwait(cond, &s_autoboot_mutex, &wait_end);
        } else {
            ::sleep(1);
        }
        if (s_autoboot_signal != AutobootSignal::NONE) {
            signal = s_autoboot_signal;
        }
        pthread_mutex_unlock(&s_autoboot_mutex);

        if (signal != AutobootSignal::NONE) {
            break;
        }
    }

    if (signal == AutobootSignal::CANCEL) {
        gui_msg("autoboot_canceled");

        operation_end(0);
    } else {
        gui_msg("autoboot_booting_now");

        operation_end(0);

        // Simply exit to continue boot
        DataManager::SetValue(VAR_TW_GUI_DONE, 1);
        DataManager::SetValue(VAR_TW_EXIT_ACTION, "");
    }

    return 0;
}

int GUIAction::autoboot_cancel(const std::string& arg __unused)
{
    pthread_mutex_lock(&s_autoboot_mutex);
    pthread_cond_t *cond = s_autoboot_cond;
    if (cond) {
        pthread_cond_signal(s_autoboot_cond);
    }
    s_autoboot_signal = AutobootSignal::CANCEL;
    pthread_mutex_unlock(&s_autoboot_mutex);
    return 0;
}

int GUIAction::autoboot_skip(const std::string& arg __unused)
{
    pthread_mutex_lock(&s_autoboot_mutex);
    pthread_cond_t *cond = s_autoboot_cond;
    if (cond) {
        pthread_cond_signal(s_autoboot_cond);
    }
    s_autoboot_signal = AutobootSignal::SKIP;
    pthread_mutex_unlock(&s_autoboot_mutex);
    return 0;
}

int GUIAction::switch_rom(const std::string& arg)
{
    std::string exit_action;
    int ret = 0;

    operation_start("Switching ROM");

    std::string current_rom;
    mbtool_interface->get_booted_rom_id(&current_rom);

    if (current_rom == arg) {
        // No need to switch if the user picked the current ROM
        gui_msg(Msg("switch_rom_not_needed")(arg));

        // Exit to continue boot
        exit_action.clear();
    } else {
        // Call mbtool to switch ROMs
        gui_msg(Msg("switch_rom_switching_to")(arg));

        auto const &boot_devs = tw_device.boot_block_devs();
        auto it = std::find_if(boot_devs.begin(), boot_devs.end(),
                               [&](const std::string &path) {
            return mb::util::path_exists(path.c_str(), true);
        });

        if (it == boot_devs.end()) {
            gui_msg(Msg(msg::kError, "switch_rom_unknown_boot_partition"));
            ret = 1;
        }

        SwitchRomResult result;
        if (ret == 0 && !mbtool_interface->switch_rom(
                arg, *it, tw_device.block_dev_base_dirs(), false, &result)) {
            gui_msg(Msg(msg::kError, "mbtool_connection_error"));
            ret = 1;
        }

        if (ret == 0) {
            switch (result) {
            case SwitchRomResult::SUCCEEDED:
                gui_msg(Msg("switch_rom_result_succeeded")(arg));
                break;
            case SwitchRomResult::FAILED:
                gui_msg(Msg(msg::kError, "switch_rom_result_failed")(arg));
                ret = 1;
                break;
            case SwitchRomResult::CHECKSUM_INVALID:
                gui_msg(Msg(msg::kError, "switch_rom_result_checksum_invalid")(arg));
                ret = 1;
                break;
            case SwitchRomResult::CHECKSUM_NOT_FOUND:
                gui_msg(Msg(msg::kError, "switch_rom_result_checksum_not_found")(arg));
                ret = 1;
                break;
            default:
                gui_msg(Msg(msg::kError, "switch_rom_result_unknown")(arg));
                ret = 1;
                break;
            }
        }

        if (ret == 0) {
            // Skip boot menu for next boot
            static const char *skip_path = "/raw/cache/multiboot/bootui/skip";
            mb::util::mkdir_parent(skip_path, 0755);
            mb::util::file_write_data(skip_path, arg.data(), arg.size());
        }

        // Reboot when exiting
        exit_action = "reboot";
    }

    operation_end(ret);

    if (ret == 0) {
        DataManager::SetValue(VAR_TW_GUI_DONE, 1);
        DataManager::SetValue(VAR_TW_EXIT_ACTION, exit_action);
    }

    return 0;
}

int GUIAction::getKeyByName(const std::string& key)
{
    if (key == "home") {
        return KEY_HOMEPAGE;  // note: KEY_HOME is cursor movement (like KEY_END)
    } else if (key == "menu") {
        return KEY_MENU;
    } else if (key == "back") {
        return KEY_BACK;
    } else if (key == "search") {
        return KEY_SEARCH;
    } else if (key == "voldown") {
        return KEY_VOLUMEDOWN;
    } else if (key == "volup") {
        return KEY_VOLUMEUP;
    } else if (key == "power") {
        return KEY_POWER;
    }

    return atol(key.c_str());
}

int GUIAction::setlanguage(const std::string& arg __unused)
{
    int op_status = 0;

    operation_start("Set Language");
    PageManager::LoadLanguage(DataManager::GetStrValue(VAR_TW_LANGUAGE));
    PageManager::RequestReload();
    op_status = 0; // success

    operation_end(op_status);
    return 0;
}
