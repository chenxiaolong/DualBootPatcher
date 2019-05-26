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

#include "gui/gui.h"

#include <atomic>
#include <chrono>

#include <linux/input.h>
#include <unistd.h>

#include "mblog/logging.h"
#include "mbutil/path.h"

#include "data.hpp"
#include "twrp-functions.hpp"
#include "variables.h"

#include "config/config.hpp"

#include "minuitwrp/minui.h"

#include "gui/blanktimer.hpp"
#include "gui/hardwarekeyboard.hpp"
#include "gui/mousecursor.hpp"
#include "gui/objects.hpp"
#include "gui/pages.hpp"

#define LOG_TAG "mbbootui/gui/gui"

// Enable to print render time of each frame to the log file
//#define PRINT_RENDER_TIME 1

#ifdef _EVENT_LOGGING
#define LOGEVENT(...) LOGE(__VA_ARGS__)
#else
#define LOGEVENT(...) do {} while (0)
#endif

using namespace std::chrono;

// Global values
static int gGuiInitialized = 0;
static std::atomic_int gForceRender;
blanktimer blankTimer;
static float scale_theme_w = 1;
static float scale_theme_h = 1;

// Needed by pages.cpp too
int gGuiRunning = 0;

int g_pty_fd = -1;  // set by terminal on init
void terminal_pty_read();

static int gRecorder = -1;

void gr_write_frame_to_file(int fd);

void flip()
{
    if (gRecorder != -1) {
        timespec time;
        clock_gettime(CLOCK_MONOTONIC, &time);
        write(gRecorder, &time, sizeof(timespec));
        gr_write_frame_to_file(gRecorder);
    }
    gr_flip();
}

void rapidxml::parse_error_handler(const char *what, void *where)
{
    fprintf(stderr, "Parser error: %s\n", what);
    fprintf(stderr, "  Start of string: %s\n",(char *) where);
    LOGE("Error parsing XML file.");
    //abort();
}

class InputHandler
{
public:
    void init()
    {
        // these might be read from DataManager in the future
        touch_hold_ms = 500;
        touch_repeat_ms = 100;
        key_hold_ms = 500;
        key_repeat_ms = 100;
        touch_status = TS_NONE;
        key_status = KS_NONE;
        state = AS_NO_ACTION;
        x = y = 0;

        if (!(tw_device.tw_flags() & mb::device::TwFlag::NoScreenTimeout)) {
            std::string seconds;
            DataManager::GetValue(VAR_TW_SCREEN_TIMEOUT_SECS, seconds);
            blankTimer.setTime(atoi(seconds.c_str()));
            blankTimer.resetTimerAndUnblank();
        } else {
            LOGI("Skipping screen timeout: TW_NO_SCREEN_TIMEOUT is set");
        }
    }

    // process input events. returns true if any event was received.
    bool processInput(int timeout_ms);

    void handleDrag();

private:
    // timeouts for touch/key hold and repeat
    int touch_hold_ms;
    int touch_repeat_ms;
    int key_hold_ms;
    int key_repeat_ms;

    enum touch_status_enum
    {
        TS_NONE = 0,
        TS_TOUCH_AND_HOLD = 1,
        TS_TOUCH_REPEAT = 2,
    };

    enum key_status_enum
    {
        KS_NONE = 0,
        KS_KEY_PRESSED = 1,
        KS_KEY_REPEAT = 2,
    };

    enum action_state_enum
    {
        AS_IN_ACTION_AREA = 0, // we've touched a spot with an action
        AS_NO_ACTION = 1,    // we've touched in an empty area (no action) and ignore remaining events until touch release
    };
    touch_status_enum touch_status;
    key_status_enum key_status;
    action_state_enum state;
    int x, y; // x and y coordinates of last touch
    struct timespec touchStart; // used to track time for long press / key repeat

    void processHoldAndRepeat();
    void process_EV_REL(input_event& ev);
    void process_EV_ABS(input_event& ev);
    void process_EV_KEY(input_event& ev);

    void doTouchStart();
};

InputHandler input_handler;


bool InputHandler::processInput(int timeout_ms)
{
    input_event ev;
    int ret = ev_get(&ev, timeout_ms);

    if (ret < 0) {
        // This path means that we did not get any new touch data, but
        // we do not get new touch data if you press and hold on either
        // the screen or on a keyboard key or mouse button
        if (touch_status || key_status) {
            processHoldAndRepeat();
        }
        return (ret != -2);  // -2 means no more events in the queue
    }

    switch (ev.type) {
    case EV_ABS:
        process_EV_ABS(ev);
        break;

    case EV_REL:
        process_EV_REL(ev);
        break;

    case EV_KEY:
        process_EV_KEY(ev);
        break;
    }

    blankTimer.resetTimerAndUnblank();
    return true;  // we got an event, so there might be more in the queue
}

void InputHandler::processHoldAndRepeat()
{
    HardwareKeyboard *kb = PageManager::GetHardwareKeyboard();

    // touch and key repeat section
    struct timespec curTime;
    clock_gettime(CLOCK_MONOTONIC, &curTime);
    long seconds = curTime.tv_sec - touchStart.tv_sec;
    long nseconds = curTime.tv_nsec - touchStart.tv_nsec;
    long mtime = ((seconds) * 1000 + nseconds / 1000000.0) + 0.5;

    if (touch_status == TS_TOUCH_AND_HOLD && mtime > touch_hold_ms) {
        touch_status = TS_TOUCH_REPEAT;
        clock_gettime(CLOCK_MONOTONIC, &touchStart);
        LOGEVENT("TOUCH_HOLD: %d,%d", x, y);
        PageManager::NotifyTouch(TOUCH_HOLD, x, y);
    } else if (touch_status == TS_TOUCH_REPEAT && mtime > touch_repeat_ms) {
        LOGEVENT("TOUCH_REPEAT: %d,%d", x, y);
        clock_gettime(CLOCK_MONOTONIC, &touchStart);
        PageManager::NotifyTouch(TOUCH_REPEAT, x, y);
    } else if (key_status == KS_KEY_PRESSED && mtime > key_hold_ms) {
        LOGEVENT("KEY_HOLD: %d,%d", x, y);
        clock_gettime(CLOCK_MONOTONIC, &touchStart);
        key_status = KS_KEY_REPEAT;
        kb->KeyRepeat();
    } else if (key_status == KS_KEY_REPEAT && mtime > key_repeat_ms) {
        LOGEVENT("KEY_REPEAT: %d,%d", x, y);
        clock_gettime(CLOCK_MONOTONIC, &touchStart);
        kb->KeyRepeat();
    }
}

void InputHandler::doTouchStart()
{
    LOGEVENT("TOUCH_START: %d,%d", x, y);
    if (PageManager::NotifyTouch(TOUCH_START, x, y) > 0) {
        state = AS_NO_ACTION;
    } else {
        state = AS_IN_ACTION_AREA;
    }
    touch_status = TS_TOUCH_AND_HOLD;
    clock_gettime(CLOCK_MONOTONIC, &touchStart);
}

void InputHandler::process_EV_ABS(input_event& ev)
{
    x = ev.value >> 16;
    y = ev.value & 0xFFFF;

    if (ev.code == 0) {
#ifndef TW_USE_KEY_CODE_TOUCH_SYNC
        if (state == AS_IN_ACTION_AREA) {
            LOGEVENT("TOUCH_RELEASE: %d,%d", x, y);
            PageManager::NotifyTouch(TOUCH_RELEASE, x, y);
        }
        touch_status = TS_NONE;
#endif
    } else {
        if (!touch_status) {
#ifndef TW_USE_KEY_CODE_TOUCH_SYNC
            doTouchStart();
#endif
        } else {
            if (state == AS_IN_ACTION_AREA) {
                LOGEVENT("TOUCH_DRAG: %d,%d", x, y);
            }
        }
    }
}

void InputHandler::process_EV_KEY(input_event& ev)
{
    HardwareKeyboard *kb = PageManager::GetHardwareKeyboard();

    // Handle key-press here
    LOGEVENT("TOUCH_KEY: %d", ev.code);
    // Left mouse button is treated as a touch
    if (ev.code == BTN_LEFT) {
        MouseCursor *cursor = PageManager::GetMouseCursor();
        if (ev.value == 1) {
            cursor->GetPos(x, y);
            doTouchStart();
        } else if (touch_status) {
            // Left mouse button was previously pressed and now is
            // being released so send a TOUCH_RELEASE
            if (state == AS_IN_ACTION_AREA) {
                cursor->GetPos(x, y);

                LOGEVENT("Mouse TOUCH_RELEASE: %d,%d", x, y);
                PageManager::NotifyTouch(TOUCH_RELEASE, x, y);
            }
            touch_status = TS_NONE;
        }
    }
    // side mouse button, often used for "back" function
    else if (ev.code == BTN_SIDE) {
        if (ev.value == 1) {
            kb->KeyDown(KEY_BACK);
        } else {
            kb->KeyUp(KEY_BACK);
        }
    } else if (ev.value != 0) {
        // This is a key press
#ifdef TW_USE_KEY_CODE_TOUCH_SYNC
        if (ev.code == TW_USE_KEY_CODE_TOUCH_SYNC) {
            LOGEVENT("key code %i key press == touch start %i %i", TW_USE_KEY_CODE_TOUCH_SYNC, x, y);
            doTouchStart();
            return;
        }
#endif
        if (kb->KeyDown(ev.code)) {
            // Key repeat is enabled for this key
            key_status = KS_KEY_PRESSED;
            touch_status = TS_NONE;
            clock_gettime(CLOCK_MONOTONIC, &touchStart);
        } else {
            key_status = KS_NONE;
            touch_status = TS_NONE;
        }
    } else {
        // This is a key release
        kb->KeyUp(ev.code);
        key_status = KS_NONE;
        touch_status = TS_NONE;
#ifdef TW_USE_KEY_CODE_TOUCH_SYNC
        if (ev.code == TW_USE_KEY_CODE_TOUCH_SYNC) {
            LOGEVENT("key code %i key release == touch release %i %i", TW_USE_KEY_CODE_TOUCH_SYNC, x, y);
            PageManager::NotifyTouch(TOUCH_RELEASE, x, y);
        }
#endif
    }
}

void InputHandler::process_EV_REL(input_event& ev)
{
    // Mouse movement
    MouseCursor *cursor = PageManager::GetMouseCursor();
    LOGEVENT("EV_REL %d %d", ev.code, ev.value);
    if (ev.code == REL_X) {
        cursor->Move(ev.value, 0);
    } else if (ev.code == REL_Y) {
        cursor->Move(0, ev.value);
    }

    if (touch_status) {
        cursor->GetPos(x, y);
        LOGEVENT("Mouse TOUCH_DRAG: %d, %d", x, y);
        key_status = KS_NONE;
    }
}

void InputHandler::handleDrag()
{
    // This allows us to only send one NotifyTouch event per render
    // cycle to reduce overhead and perceived input latency.
    static int prevx = 0, prevy = 0; // these track where the last drag notice was so that we don't send duplicate drag notices
    if (touch_status && (x != prevx || y != prevy)) {
        prevx = x;
        prevy = y;
        if (PageManager::NotifyTouch(TOUCH_DRAG, x, y) > 0) {
            state = AS_NO_ACTION;
        } else {
            state = AS_IN_ACTION_AREA;
        }
    }
}

// Get and dispatch input events until it's time to draw the next frame
// This special function will return immediately the first time, but then
// always returns 1/30th of a second (or immediately if called later) from
// the last time it was called
static void loopTimer(int input_timeout_ms)
{
    static steady_clock::time_point lastCall;
    static int initialized = 0;

    if (!initialized) {
        lastCall = steady_clock::now();
        initialized = 1;
        return;
    }

    do {
        bool got_event = input_handler.processInput(input_timeout_ms); // get inputs but don't send drag notices
        auto curTime = steady_clock::now();
        auto diff = duration_cast<nanoseconds>(curTime - lastCall);

        // This is really 2 or 30 times per second
        // As long as we get events, increase the timeout so we can catch up with input
        long timeout = got_event ? 500000000 : 33333333;

        if (diff.count() > timeout) {
            //auto input_time = duration_cast<milliseconds>(curTime - lastCall);
            //LOGI("loopTimer(): %" PRId64 " ms, count: %u",
            //     input_time.count(), count);

            lastCall = curTime;
            input_handler.handleDrag(); // send only drag notices if needed
            return;
        }

        // We need to sleep some period time microseconds
        //unsigned int sleepTime = 33333 -(diff.tv_nsec / 1000);
        //usleep(sleepTime); // removed so we can scan for input
        input_timeout_ms = 0;
    } while (1);
}

static int runPages(const char *page_name, const int stop_on_page_done)
{
    DataManager::SetValue(VAR_TW_PAGE_DONE, 0);
    DataManager::SetValue(VAR_TW_GUI_DONE, 0);

    if (page_name) {
        PageManager::SetStartPage(page_name);
        gui_changePage(page_name);
    }

    gGuiRunning = 1;

    DataManager::SetValue(VAR_TW_LOADED, 1);

    struct timeval timeout;
    fd_set fdset;
    int has_data = 0;

    int input_timeout_ms = 0;
    int idle_frames = 0;

    for (;;) {
        loopTimer(input_timeout_ms);
        if (g_pty_fd > 0) {
            // TODO: this is not nice, we should have one central select for input, pty
            FD_ZERO(&fdset);
            FD_SET(g_pty_fd, &fdset);
            timeout.tv_sec = 0;
            timeout.tv_usec = 1;
            has_data = select(g_pty_fd + 1, &fdset, nullptr, nullptr, &timeout);
            if (has_data > 0) {
                terminal_pty_read();
            }
        }

        if (!gForceRender) {
            int ret = PageManager::Update();
            if (ret == 0) {
                ++idle_frames;
            } else if (ret == -2) {
                break; // Theme reload failure
            } else {
                idle_frames = 0;
            }
            // due to possible animation objects, we need to delay activating the input timeout
            input_timeout_ms = idle_frames > 15 ? 1000 : 0;

#ifndef PRINT_RENDER_TIME
            if (ret > 1) {
                PageManager::Render();
            }

            if (ret > 0) {
                flip();
            }
#else
            if (ret > 1) {
                auto start = steady_clock::now();
                PageManager::Render();
                auto end = steady_clock::now();
                auto render_t = duration_cast<milliseconds>(end - start);

                flip();
                auto flip_end = steady_clock::now();
                auto flip_t = duration_cast<milliseconds>(flip_end - end);

                LOGI("Render(): %" PRId64 " ms, flip(): %" PRId64 " ms, total: %" PRId64 " ms",
                     render_t.count(), flip_t.count(),
                     render_t.count() + flip_t.count());
            } else if (ret > 0) {
                flip();
            }
#endif
        } else {
            gForceRender = 0;
            PageManager::Render();
            flip();
            input_timeout_ms = 0;
        }

        blankTimer.checkForTimeout();
        if (stop_on_page_done && DataManager::GetIntValue(VAR_TW_PAGE_DONE) != 0) {
            gui_changePage("main");
            break;
        }
        if (DataManager::GetIntValue(VAR_TW_GUI_DONE) != 0) {
            break;
        }
    }
    gGuiRunning = 0;
    return 0;
}

int gui_forceRender()
{
    gForceRender = 1;
    return 0;
}

int gui_changePage(std::string newPage)
{
    LOGI("Set page: '%s'", newPage.c_str());
    PageManager::ChangePage(newPage);
    gForceRender = 1;
    return 0;
}

int gui_changeOverlay(std::string overlay)
{
    LOGI("Set overlay: '%s'", overlay.c_str());
    PageManager::ChangeOverlay(overlay);
    gForceRender = 1;
    return 0;
}

std::string gui_parse_text(std::string str)
{
    // This function parses text for DataManager values encompassed by %value% in the XML
    // and string resources (%@resource_name%)
    size_t pos = 0, next, end;

    while (1) {
        next = str.find("{@", pos);
        if (next == std::string::npos) {
            break;
        }

        end = str.find('}', next + 1);
        if (end == std::string::npos) {
            break;
        }

        std::string var = str.substr(next + 2, (end - next) - 2);
        str.erase(next, (end - next) + 1);

        size_t default_loc = var.find('=', 0);
        std::string lookup;
        if (default_loc == std::string::npos) {
            str.insert(next, PageManager::GetResources()->FindString(var));
        } else {
            lookup = var.substr(0, default_loc);
            std::string default_string = var.substr(default_loc + 1, var.size() - default_loc - 1);
            str.insert(next, PageManager::GetResources()->FindString(lookup, default_string));
        }
    }
    pos = 0;
    while (1) {
        next = str.find('%', pos);
        if (next == std::string::npos) {
            return str;
        }

        end = str.find('%', next + 1);
        if (end == std::string::npos) {
            return str;
        }

        // We have a block of data
        std::string var = str.substr(next + 1, (end - next) - 1);
        str.erase(next, (end - next) + 1);

        if (next + 1 == end) {
            str.insert(next, 1, '%');
        } else {
            std::string value;
            if (var.size() > 0 && var[0] == '@') {
                // this is a string resource ("%@string_name%")
                value = PageManager::GetResources()->FindString(var.substr(1));
                str.insert(next, value);
            } else if (DataManager::GetValue(var, value) == 0) {
                str.insert(next, value);
            }
        }

        pos = next + 1;
    }
}

std::string gui_lookup(const std::string& resource_name,
                       const std::string& default_value)
{
    return PageManager::GetResources()->FindString(resource_name, default_value);
}

extern "C" int gui_init()
{
    if (gr_init() < 0) {
        return -1;
    }

    TWFunc::Set_Brightness(DataManager::GetStrValue(VAR_TW_BRIGHTNESS));

    // load and show splash screen
    if (PageManager::LoadPackage("splash", TWFunc::get_resource_path("splash.xml"), "splash")) {
        LOGE("Failed to load splash screen XML.");
    } else {
        PageManager::SelectPackage("splash");
        PageManager::Render();
        flip();
        PageManager::ReleasePackage("splash");
    }

    ev_init();
    return 0;
}

extern "C" int gui_loadResources()
{
#ifndef TW_OEM_BUILD
    int check = 0;
    DataManager::GetValue(VAR_TW_IS_ENCRYPTED, check);

    if (check) {
        if (PageManager::LoadPackage("TWRP", TWFunc::get_resource_path("ui.xml"), "decrypt")) {
            gui_err("base_pkg_err=Failed to load base packages.");
            goto error;
        } else {
            check = 1;
        }
    }

    if (check == 0) {
        if (PageManager::LoadPackage("TWRP", tw_theme_zip_path, "main")) {
#endif // ifndef TW_OEM_BUILD
            if (PageManager::LoadPackage("TWRP", TWFunc::get_resource_path("ui.xml"), "main")) {
                gui_err("base_pkg_err=Failed to load base packages.");
                goto error;
            }
#ifndef TW_OEM_BUILD
        }
    }
#endif // ifndef TW_OEM_BUILD
    // Set the default package
    PageManager::SelectPackage("TWRP");

    gGuiInitialized = 1;
    return 0;

error:
    LOGE("An internal error has occurred: unable to load theme.");
    gGuiInitialized = 0;
    return -1;
}

extern "C" int gui_loadCustomResources()
{
#ifndef TW_OEM_BUILD
    // Check for a custom theme
    if (mb::util::path_exists(tw_theme_zip_path, false)) {
        // There is a custom theme, try to load it
        if (PageManager::ReloadPackage("TWRP", tw_theme_zip_path)) {
            // Custom theme failed to load, try to load stock theme
            if (PageManager::ReloadPackage("TWRP", TWFunc::get_resource_path("ui.xml"))) {
                gui_err("base_pkg_err=Failed to load base packages.");
                goto error;
            }
        }
    }
    // Set the default package
    PageManager::SelectPackage("TWRP");
#endif
    return 0;

error:
    LOGE("An internal error has occurred: unable to load theme.");
    gGuiInitialized = 0;
    return -1;
}

extern "C" int gui_start()
{
    return gui_startPage("main", 1, 0);
}

extern "C" int gui_startPage(const char *page_name,
                             const int allow_commands __unused,
                             int stop_on_page_done)
{
    if (!gGuiInitialized) {
        return -1;
    }

    // Set the default package
    PageManager::SelectPackage("TWRP");

    input_handler.init();
    return runPages(page_name, stop_on_page_done);
}


extern "C" void set_scale_values(float w, float h)
{
    scale_theme_w = w;
    scale_theme_h = h;
}

extern "C" int scale_theme_x(int initial_x)
{
    if (scale_theme_w != 1) {
        int scaled = (float) initial_x * scale_theme_w;
        if (scaled == 0 && initial_x > 0) {
            return 1;
        }
        return scaled;
    }
    return initial_x;
}

extern "C" int scale_theme_y(int initial_y)
{
    if (scale_theme_h != 1) {
        int scaled = (float) initial_y * scale_theme_h;
        if (scaled == 0 && initial_y > 0) {
            return 1;
        }
        return scaled;
    }
    return initial_y;
}

extern "C" int scale_theme_min(int initial_value)
{
    if (scale_theme_w != 1 || scale_theme_h != 1) {
        if (scale_theme_w < scale_theme_h) {
            return scale_theme_x(initial_value);
        } else {
            return scale_theme_y(initial_value);
        }
    }
    return initial_value;
}

extern "C" float get_scale_w()
{
    return scale_theme_w;
}

extern "C" float get_scale_h()
{
    return scale_theme_h;
}
