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

// pages.cpp - Source to manage GUI base objects

#include "gui/objects.hpp"

#include <algorithm>

#include <cstring>

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mblog/logging.h"
#include "mbutil/delete.h"
#include "mbutil/directory.h"
#include "mbutil/path.h"

#include "data.hpp"
#include "twrp-functions.hpp"
#include "variables.h"

#include "config/config.hpp"

#include "gui/blanktimer.hpp"
#include "gui/gui.h"

#include "gui/action.hpp"
#include "gui/animation.hpp"
#include "gui/button.hpp"
#include "gui/checkbox.hpp"
#include "gui/console.hpp"
#include "gui/fileselector.hpp"
#include "gui/fill.hpp"
#include "gui/hardwarekeyboard.hpp"
#include "gui/image.hpp"
#include "gui/input.hpp"
#include "gui/keyboard.hpp"
#include "gui/listbox.hpp"
#include "gui/mousecursor.hpp"
#include "gui/patternpassword.hpp"
#include "gui/progressbar.hpp"
#include "gui/scrolllist.hpp"
#include "gui/slider.hpp"
#include "gui/slidervalue.hpp"
#include "gui/terminal.hpp"
#include "gui/text.hpp"
#include "gui/textbox.hpp"

#define LOG_TAG "mbbootui/gui/pages"

#define TW_THEME_VERSION 1
#define TW_THEME_VER_ERR -2

extern int gGuiRunning;

// From console.cpp
extern size_t last_message_count;
extern std::vector<std::string> gConsole;
extern std::vector<std::string> gConsoleColor;

std::unordered_map<std::string, PageSet*> PageManager::mPageSets;
PageSet* PageManager::mCurrentSet;
MouseCursor *PageManager::mMouseCursor = nullptr;
HardwareKeyboard *PageManager::mHardwareKeyboard = nullptr;
bool PageManager::mReloadTheme = false;
std::string PageManager::mStartPage = "main";
std::vector<language_struct> Language_List;

int tw_x_offset = 0;
int tw_y_offset = 0;

// Helper routine to convert a string to a color declaration
int ConvertStrToColor(std::string str, COLOR* color)
{
    // Set the default, solid black
    memset(color, 0, sizeof(COLOR));
    color->alpha = 255;

    // Translate variables
    DataManager::GetValue(str, str);

    // Look for some defaults
    if (str == "black") {
        return 0;
    } else if (str == "white") {
        color->red = color->green = color->blue = 255;
        return 0;
    } else if (str == "red") {
        color->red = 255;
        return 0;
    } else if (str == "green") {
        color->green = 255;
        return 0;
    } else if (str == "blue") {
        color->blue = 255;
        return 0;
    }

    // At this point, we require an RGB(A) color
    if (str[0] != '#') {
        return -1;
    }

    str.erase(0, 1);

    int result;
    if (str.size() >= 8) {
        // We have alpha channel
        std::string alpha = str.substr(6, 2);
        result = strtol(alpha.c_str(), nullptr, 16);
        color->alpha = result & 0x000000FF;
        str.resize(6);
        result = strtol(str.c_str(), nullptr, 16);
        color->red = (result >> 16) & 0x000000FF;
        color->green = (result >> 8) & 0x000000FF;
        color->blue = result & 0x000000FF;
    } else {
        result = strtol(str.c_str(), nullptr, 16);
        color->red = (result >> 16) & 0x000000FF;
        color->green = (result >> 8) & 0x000000FF;
        color->blue = result & 0x000000FF;
    }
    return 0;
}

// Helper APIs
xml_node<>* FindNode(xml_node<>* parent, const char* nodename, int depth /* = 0 */)
{
    if (!parent) {
        return nullptr;
    }

    xml_node<>* child = parent->first_node(nodename);
    if (child) {
        return child;
    }

    if (depth == 10) {
        LOGE("Too many style loops detected.");
        return nullptr;
    }

    xml_node<>* style = parent->first_node("style");
    if (style) {
        while (style) {
            if (!style->first_attribute("name")) {
                LOGE("No name given for style.");
                continue;
            } else {
                std::string name = style->first_attribute("name")->value();
                xml_node<>* node = PageManager::FindStyle(name);

                if (node) {
                    // We found the style that was named
                    xml_node<>* stylenode = FindNode(node, nodename, depth + 1);
                    if (stylenode) {
                        return stylenode;
                    }
                }
            }
            style = style->next_sibling("style");
        }
    } else {
        // Search for stylename in the parent node <object type="foo" style="foo2">
        xml_attribute<>* attr = parent->first_attribute("style");
        // If no style is found anywhere else and the node wasn't found in the object itself
        // as a special case we will search for a style that uses the same style name as the
        // object type, so <object type="button"> would search for a style named "button"
        if (!attr) {
            attr = parent->first_attribute("type");
        }
        // if there's no attribute type, the object type must be the element name
        std::string stylename = attr ? attr->value() : parent->name();
        xml_node<>* node = PageManager::FindStyle(stylename);
        if (node) {
            xml_node<>* stylenode = FindNode(node, nodename, depth + 1);
            if (stylenode) {
                return stylenode;
            }
        }
    }
    return nullptr;
}

std::string LoadAttrString(xml_node<>* element, const char* attrname, const char* defaultvalue)
{
    if (!element) {
        return defaultvalue;
    }

    xml_attribute<>* attr = element->first_attribute(attrname);
    return attr ? attr->value() : defaultvalue;
}

int LoadAttrInt(xml_node<>* element, const char* attrname, int defaultvalue)
{
    std::string value = LoadAttrString(element, attrname);
    // resolve variables
    DataManager::GetValue(value, value);
    return value.empty() ? defaultvalue : atoi(value.c_str());
}

int LoadAttrIntScaleX(xml_node<>* element, const char* attrname, int defaultvalue)
{
    return scale_theme_x(LoadAttrInt(element, attrname, defaultvalue));
}

int LoadAttrIntScaleY(xml_node<>* element, const char* attrname, int defaultvalue)
{
    return scale_theme_y(LoadAttrInt(element, attrname, defaultvalue));
}

COLOR LoadAttrColor(xml_node<>* element, const char* attrname, bool* found_color, COLOR defaultvalue)
{
    std::string value = LoadAttrString(element, attrname);
    *found_color = !value.empty();
    // resolve variables
    DataManager::GetValue(value, value);
    COLOR ret = defaultvalue;
    if (ConvertStrToColor(value, &ret) == 0) {
        return ret;
    } else {
        return defaultvalue;
    }
}

COLOR LoadAttrColor(xml_node<>* element, const char* attrname, COLOR defaultvalue)
{
    bool found_color = false;
    return LoadAttrColor(element, attrname, &found_color, defaultvalue);
}

FontResource* LoadAttrFont(xml_node<>* element, const char* attrname)
{
    std::string name = LoadAttrString(element, attrname, "");
    if (name.empty()) {
        return nullptr;
    } else {
        return PageManager::GetResources()->FindFont(name);
    }
}

ImageResource* LoadAttrImage(xml_node<>* element, const char* attrname)
{
    std::string name = LoadAttrString(element, attrname, "");
    if (name.empty()) {
        return nullptr;
    } else {
        return PageManager::GetResources()->FindImage(name);
    }
}

AnimationResource* LoadAttrAnimation(xml_node<>* element, const char* attrname)
{
    std::string name = LoadAttrString(element, attrname, "");
    if (name.empty()) {
        return nullptr;
    } else {
        return PageManager::GetResources()->FindAnimation(name);
    }
}

bool LoadPlacement(xml_node<>* node, int* x, int* y, int* w /* = NULL */, int* h /* = NULL */, Placement* placement /* = NULL */)
{
    if (!node) {
        return false;
    }

    if (node->first_attribute("x")) {
        *x = LoadAttrIntScaleX(node, "x") + tw_x_offset;
    }

    if (node->first_attribute("y")) {
        *y = LoadAttrIntScaleY(node, "y") + tw_y_offset;
    }

    if (w && node->first_attribute("w")) {
        *w = LoadAttrIntScaleX(node, "w");
    }

    if (h && node->first_attribute("h")) {
        *h = LoadAttrIntScaleY(node, "h");
    }

    if (placement && node->first_attribute("placement")) {
        *placement = (Placement) LoadAttrInt(node, "placement");
    }

    return true;
}

int ActionObject::SetActionPos(int x, int y, int w, int h)
{
    if (x < 0 || y < 0) {
        return -1;
    }

    mActionX = x;
    mActionY = y;
    if (w || h) {
        mActionW = w;
        mActionH = h;
    }
    return 0;
}

Page::Page(xml_node<>* page, std::vector<xml_node<>*> *templates)
{
    mTouchStart = nullptr;

    // We can memset the whole structure, because the alpha channel is ignored
    memset(&mBackground, 0, sizeof(COLOR));

    // With NULL, we make a console-only display
    if (!page) {
        mName = "console";

        GUIConsole* element = new GUIConsole(nullptr);
        mRenders.push_back(element);
        mActions.push_back(element);
        return;
    }

    if (page->first_attribute("name")) {
        mName = page->first_attribute("name")->value();
    } else {
        LOGE("No page name attribute found!");
        return;
    }

    LOGI("Loading page %s", mName.c_str());

    // This is a recursive routine for template handling
    ProcessNode(page, templates, 0);
}

Page::~Page()
{
    for (auto itr = mObjects.begin(); itr != mObjects.end(); ++itr) {
        delete *itr;
    }
}

bool Page::ProcessNode(xml_node<>* page, std::vector<xml_node<>*> *templates, int depth)
{
    if (depth == 10) {
        LOGE("Page processing depth has exceeded 10. Failing out. This is likely a recursive template.");
        return false;
    }

    for (xml_node<>* child = page->first_node(); child; child = child->next_sibling()) {
        std::string type = child->name();

        if (type == "background") {
            mBackground = LoadAttrColor(child, "color", COLOR(0,0,0,0));
            continue;
        }

        if (type == "object") {
            // legacy format : <object type="...">
            xml_attribute<>* attr = child->first_attribute("type");
            type = attr ? attr->value() : "*unspecified*";
        }

        if (type == "text") {
            GUIText* element = new GUIText(child);
            mObjects.push_back(element);
            mRenders.push_back(element);
            mActions.push_back(element);
        } else if (type == "image") {
            GUIImage* element = new GUIImage(child);
            mObjects.push_back(element);
            mRenders.push_back(element);
        } else if (type == "fill") {
            GUIFill* element = new GUIFill(child);
            mObjects.push_back(element);
            mRenders.push_back(element);
        } else if (type == "action") {
            GUIAction* element = new GUIAction(child);
            mObjects.push_back(element);
            mActions.push_back(element);
        } else if (type == "console") {
            GUIConsole* element = new GUIConsole(child);
            mObjects.push_back(element);
            mRenders.push_back(element);
            mActions.push_back(element);
        } else if (type == "terminal") {
            GUITerminal* element = new GUITerminal(child);
            mObjects.push_back(element);
            mRenders.push_back(element);
            mActions.push_back(element);
            mInputs.push_back(element);
        } else if (type == "button") {
            GUIButton* element = new GUIButton(child);
            mObjects.push_back(element);
            mRenders.push_back(element);
            mActions.push_back(element);
        } else if (type == "checkbox") {
            GUICheckbox* element = new GUICheckbox(child);
            mObjects.push_back(element);
            mRenders.push_back(element);
            mActions.push_back(element);
        } else if (type == "fileselector") {
            GUIFileSelector* element = new GUIFileSelector(child);
            mObjects.push_back(element);
            mRenders.push_back(element);
            mActions.push_back(element);
        } else if (type == "animation") {
            GUIAnimation* element = new GUIAnimation(child);
            mObjects.push_back(element);
            mRenders.push_back(element);
        } else if (type == "progressbar") {
            GUIProgressBar* element = new GUIProgressBar(child);
            mObjects.push_back(element);
            mRenders.push_back(element);
            mActions.push_back(element);
        } else if (type == "slider") {
            GUISlider* element = new GUISlider(child);
            mObjects.push_back(element);
            mRenders.push_back(element);
            mActions.push_back(element);
        } else if (type == "slidervalue") {
            GUISliderValue *element = new GUISliderValue(child);
            mObjects.push_back(element);
            mRenders.push_back(element);
            mActions.push_back(element);
        } else if (type == "listbox") {
            GUIListBox* element = new GUIListBox(child);
            mObjects.push_back(element);
            mRenders.push_back(element);
            mActions.push_back(element);
        } else if (type == "keyboard") {
            GUIKeyboard* element = new GUIKeyboard(child);
            mObjects.push_back(element);
            mRenders.push_back(element);
            mActions.push_back(element);
        } else if (type == "input") {
            GUIInput* element = new GUIInput(child);
            mObjects.push_back(element);
            mRenders.push_back(element);
            mActions.push_back(element);
            mInputs.push_back(element);
        } else if (type == "patternpassword") {
            GUIPatternPassword* element = new GUIPatternPassword(child);
            mObjects.push_back(element);
            mRenders.push_back(element);
            mActions.push_back(element);
        } else if (type == "textbox") {
            GUITextBox* element = new GUITextBox(child);
            mObjects.push_back(element);
            mRenders.push_back(element);
            mActions.push_back(element);
        } else if (type == "template") {
            if (!templates || !child->first_attribute("name")) {
                LOGE("Invalid template request.");
            } else {
                std::string name = child->first_attribute("name")->value();
                xml_node<>* node;
                bool node_found = false;

                // We need to find the correct template
                for (auto itr = templates->begin(); itr != templates->end(); itr++) {
                    node = (*itr)->first_node("template");

                    while (node) {
                        if (!node->first_attribute("name")) {
                            continue;
                        }

                        if (name == node->first_attribute("name")->value()) {
                            if (!ProcessNode(node, templates, depth + 1)) {
                                return false;
                            } else {
                                node_found = true;
                                break;
                            }
                        }
                        if (node_found) {
                            break;
                        }
                        node = node->next_sibling("template");
                    }
                    // [check] why is there no if (node_found) here too?
                }
            }
        } else {
            LOGE("Unknown object type: %s.", type.c_str());
        }
    }
    return true;
}

int Page::Render()
{
    // Render background
    gr_color(mBackground.red, mBackground.green, mBackground.blue, mBackground.alpha);
    gr_fill(0, 0, gr_fb_width(), gr_fb_height());

    // Render remaining objects
    for (auto iter = mRenders.begin(); iter != mRenders.end(); iter++) {
        if ((*iter)->Render()) {
            LOGE("A render request has failed.");
        }
    }
    return 0;
}

int Page::Update()
{
    int retCode = 0;

    for (auto iter = mRenders.begin(); iter != mRenders.end(); iter++) {
        int ret = (*iter)->Update();
        if (ret < 0) {
            LOGE("An update request has failed.");
        } else if (ret > retCode) {
            retCode = ret;
        }
    }

    return retCode;
}

int Page::NotifyTouch(TOUCH_STATE state, int x, int y)
{
    // By default, return 1 to ignore further touches if nobody is listening
    int ret = 1;

    // Don't try to handle a lack of handlers
    if (mActions.size() == 0) {
        return ret;
    }

    // We record mTouchStart so we can pass all the touch stream to the same handler
    if (state == TOUCH_START) {
        // We work backwards, from top-most element to bottom-most element
        for (auto iter = mActions.rbegin(); iter != mActions.rend(); iter++) {
            if ((*iter)->IsInRegion(x, y)) {
                mTouchStart = (*iter);
                ret = mTouchStart->NotifyTouch(state, x, y);
                if (ret >= 0) {
                    break;
                }
                mTouchStart = nullptr;
            }
        }
    } else if (state == TOUCH_RELEASE && mTouchStart != nullptr) {
        ret = mTouchStart->NotifyTouch(state, x, y);
        mTouchStart = nullptr;
    } else if ((state == TOUCH_DRAG
            || state == TOUCH_HOLD
            || state == TOUCH_REPEAT)
            && mTouchStart != nullptr) {
        ret = mTouchStart->NotifyTouch(state, x, y);
    }
    return ret;
}

int Page::NotifyKey(int key, bool down)
{
    int ret = 1;
    // We work backwards, from top-most element to bottom-most element
    for (auto iter = mActions.rbegin(); iter != mActions.rend(); iter++) {
        ret = (*iter)->NotifyKey(key, down);
        if (ret == 0) {
            return 0;
        }
        if (ret < 0) {
            LOGE("An action handler has returned an error");
            ret = 1;
        }
    }
    return ret;
}

int Page::NotifyCharInput(int ch)
{
    // We work backwards, from top-most element to bottom-most element
    for (auto iter = mInputs.rbegin(); iter != mInputs.rend(); iter++) {
        int ret = (*iter)->NotifyCharInput(ch);
        if (ret == 0) {
            return 0;
        } else if (ret < 0) {
            LOGE("A char input handler has returned an error");
        }
    }
    return 1;
}

int Page::SetKeyBoardFocus(int inFocus)
{
    // We work backwards, from top-most element to bottom-most element
    for (auto iter = mInputs.rbegin(); iter != mInputs.rend(); iter++) {
        int ret = (*iter)->SetInputFocus(inFocus);
        if (ret == 0) {
            return 0;
        } else if (ret < 0) {
            LOGE("An input focus handler has returned an error");
        }
    }
    return 1;
}

void Page::SetPageFocus(int inFocus)
{
    // Render remaining objects
    for (auto iter = mRenders.begin(); iter != mRenders.end(); iter++) {
        (*iter)->SetPageFocus(inFocus);
    }

    return;
}

int Page::NotifyVarChange(const std::string& varName, const std::string& value)
{
    for (auto iter = mObjects.begin(); iter != mObjects.end(); ++iter) {
        if ((*iter)->NotifyVarChange(varName, value)) {
            LOGE("An action handler errored on NotifyVarChange.");
        }
    }
    return 0;
}


// transient data for loading themes
struct LoadingContext
{
    ZipArchive* zip; // zip to load theme from, or NULL for the stock theme
    std::unordered_set<std::string> filenames; // to detect cyclic includes
    std::string basepath; // if zip is NULL, base path to load includes from with trailing slash, otherwise empty
    std::vector<xml_document<>*> xmldocs; // all loaded xml docs
    std::vector<char*> xmlbuffers; // text buffers with xml content
    std::vector<xml_node<>*> styles; // refer to <styles> nodes inside xmldocs
    std::vector<xml_node<>*> templates; // refer to <templates> nodes inside xmldocs

    LoadingContext()
    {
        zip = nullptr;
    }

    ~LoadingContext()
    {
        // free all xml buffers
        for (auto it = xmlbuffers.begin(); it != xmlbuffers.end(); ++it) {
            free(*it);
        }
    }
};

// for FindStyle
LoadingContext* PageManager::currentLoadingContext = nullptr;


PageSet::PageSet()
{
    mResources = new ResourceManager;
    mCurrentPage = nullptr;

    set_scale_values(1, 1); // Reset any previous scaling values
}

PageSet::~PageSet()
{
    mOverlays.clear();
    for (auto itr = mPages.begin(); itr != mPages.end(); ++itr) {
        delete *itr;
    }

    delete mResources;
}

int PageSet::Load(LoadingContext& ctx, const std::string& filename)
{
    bool isMain = ctx.xmlbuffers.empty(); // if we have no files yet, remember that this is the main XML file

    if (!ctx.filenames.insert(filename).second) {
        // ignore already loaded files to prevent crash with cyclic includes
        return 0;
    }

    // load XML into buffer
    char* xmlbuffer = PageManager::LoadFileToBuffer(filename, ctx.zip);
    if (!xmlbuffer) {
        return -1; // error already displayed by LoadFileToBuffer
    }
    ctx.xmlbuffers.push_back(xmlbuffer);

    // parse XML
    xml_document<>* doc = new xml_document<>();
    doc->parse<0>(xmlbuffer);
    ctx.xmldocs.push_back(doc);

    xml_node<>* root = doc->first_node("recovery");
    if (!root) {
        root = doc->first_node("install");
    }
    if (!root) {
        LOGE("Unknown root element in %s", filename.c_str());
        return -1;
    }

    if (isMain) {
        int rc = LoadDetails(ctx, root);
        if (rc != 0) {
            return rc;
        }
    }

    LOGI("Loading resources...");
    xml_node<>* child = root->first_node("resources");
    if (child) {
        mResources->LoadResources(child, ctx.zip, "theme");
    }

    LOGI("Loading variables...");
    child = root->first_node("variables");
    if (child) {
        LoadVariables(child);
    }

    LOGI("Loading mouse cursor...");
    child = root->first_node("mousecursor");
    if (child) {
        PageManager::LoadCursorData(child);
    }

    LOGI("Loading pages...");
    child = root->first_node("templates");
    if (child) {
        ctx.templates.push_back(child);
    }

    child = root->first_node("styles");
    if (child) {
        ctx.styles.push_back(child);
    }

    // Load pages
    child = root->first_node("pages");
    if (child) {
        if (LoadPages(ctx, child)) {
            LOGE("PageSet::Load returning -1");
            return -1;
        }
    }

    // process includes recursively
    child = root->first_node("include");
    if (child) {
        xml_node<>* include = child->first_node("xmlfile");
        while (include != nullptr) {
            xml_attribute<>* attr = include->first_attribute("name");
            if (!attr) {
                LOGE("Skipping include/xmlfile with no name");
                continue;
            }

            std::string filename = ctx.basepath + attr->value();
            LOGI("Including file: %s...", filename.c_str());
            int rc = Load(ctx, filename);
            if (rc != 0) {
                return rc;
            }

            include = include->next_sibling("xmlfile");
        }
    }

    return 0;
}

void PageSet::MakeEmergencyConsoleIfNeeded()
{
    if (mPages.empty()) {
        mCurrentPage = new Page(nullptr, nullptr); // fallback console page
        // TODO: since removal of non-TTF fonts, the emergency console doesn't work without a font, which might be missing too
        mPages.push_back(mCurrentPage);
    }
}

int PageSet::LoadLanguage(char* languageFile, ZipArchive* package)
{
    xml_document<> lang;
    xml_node<>* parent;
    xml_node<>* child;
    std::string resource_source;
    int ret = 0;

    if (languageFile) {
        printf("parsing languageFile\n");
        lang.parse<0>(languageFile);
        printf("parsing languageFile done\n");
    } else {
        return -1;
    }

    parent = lang.first_node("language");
    if (!parent) {
        LOGE("Unable to locate language node in language file.");
        lang.clear();
        return -1;
    }

    child = parent->first_node("display");
    if (child) {
        DataManager::SetValue(VAR_TW_LANGUAGE_DISPLAY, child->value());
        resource_source = child->value();
    } else {
        LOGE("language file does not have a display value set");
        DataManager::SetValue(VAR_TW_LANGUAGE_DISPLAY, "Not Set");
        resource_source = languageFile;
    }

    child = parent->first_node("resources");
    if (child) {
        mResources->LoadResources(child, package, resource_source);
    } else {
        ret = -1;
    }
    lang.clear();
    return ret;
}

int PageSet::LoadDetails(LoadingContext& ctx, xml_node<>* root)
{
    xml_node<>* child = root->first_node("details");
    if (child) {
        int theme_ver = 0;
        xml_node<>* themeversion = child->first_node("themeversion");
        if (themeversion && themeversion->value()) {
            theme_ver = atoi(themeversion->value());
        } else {
            LOGI("No themeversion in theme.");
        }
        if (theme_ver != TW_THEME_VERSION) {
            LOGI("theme version from xml: %i, expected %i", theme_ver, TW_THEME_VERSION);
            if (ctx.zip) {
                gui_err("theme_ver_err=Custom theme version does not match TWRP version. Using stock theme.");
                return TW_THEME_VER_ERR;
            } else {
                gui_print_color("warning", "Stock theme version does not match TWRP version.\n");
            }
        }
        xml_node<>* resolution = child->first_node("resolution");
        if (resolution) {
            LOGI("Checking resolution...");
            xml_attribute<>* width_attr = resolution->first_attribute("width");
            xml_attribute<>* height_attr = resolution->first_attribute("height");
            xml_attribute<>* noscale_attr = resolution->first_attribute("noscaling");
            if (width_attr && height_attr && !noscale_attr) {
                int width = atoi(width_attr->value());
                int height = atoi(height_attr->value());
                int offx = 0, offy = 0;
                if (tw_device.tw_flags() & mb::device::TwFlag::RoundScreen) {
                    xml_node<>* roundscreen = child->first_node("roundscreen");
                    if (roundscreen) {
                        LOGI("TW_ROUND_SCREEN := true, using round screen XML settings.");
                        xml_attribute<>* offx_attr = roundscreen->first_attribute("offset_x");
                        xml_attribute<>* offy_attr = roundscreen->first_attribute("offset_y");
                        if (offx_attr) {
                            offx = atoi(offx_attr->value());
                        }
                        if (offy_attr) {
                            offy = atoi(offy_attr->value());
                        }
                    }
                }
                if (width != 0 && height != 0) {
                    float scale_w = ((float) gr_fb_width() - ((float) offx * 2.0)) / (float) width;
                    float scale_h = ((float) gr_fb_height() - ((float) offy * 2.0)) / (float) height;
                    if (tw_device.tw_flags() & mb::device::TwFlag::RoundScreen) {
                        float scale_off_w = (float) gr_fb_width() / (float) width;
                        float scale_off_h = (float) gr_fb_height() / (float) height;
                        tw_x_offset = offx * scale_off_w;
                        tw_y_offset = offy * scale_off_h;
                    }
                    if (scale_w != 1 || scale_h != 1) {
                        LOGI("Scaling theme width %fx and height %fx, offsets x: %i y: %i", scale_w, scale_h, tw_x_offset, tw_y_offset);
                        set_scale_values(scale_w, scale_h);
                    }
                }
            } else {
                LOGI("XML does not contain width and height, no scaling will be applied");
            }
        } else {
            LOGI("XML contains no resolution tag, no scaling will be applied.");
        }
    } else {
        LOGI("XML contains no details tag, no scaling will be applied.");
    }

    return 0;
}

int PageSet::SetPage(const std::string& page)
{
    Page* tmp = FindPage(page);
    if (tmp) {
        if (mCurrentPage) {
            mCurrentPage->SetPageFocus(0);
        }
        mCurrentPage = tmp;
        mCurrentPage->SetPageFocus(1);
        mCurrentPage->NotifyVarChange("", "");
        return 0;
    } else {
        LOGE("Unable to locate page (%s)", page.c_str());
    }
    return -1;
}

int PageSet::SetOverlay(Page* page)
{
    if (page) {
        if (mOverlays.size() >= 10) {
            LOGE("Too many overlays requested, max is 10.");
            return -1;
        }

        for (auto iter = mOverlays.begin(); iter != mOverlays.end(); iter++) {
            if ((*iter)->GetName() == page->GetName()) {
                mOverlays.erase(iter);
                // SetOverlay() is (and should stay) the only function which
                // adds to mOverlays. Then, each page can appear at most once.
                break;
            }
        }

        page->SetPageFocus(1);
        page->NotifyVarChange("", "");

        if (!mOverlays.empty()) {
            mOverlays.back()->SetPageFocus(0);
        }

        mOverlays.push_back(page);
    } else {
        if (!mOverlays.empty()) {
            mOverlays.back()->SetPageFocus(0);
            mOverlays.pop_back();
            if (!mOverlays.empty()) {
                mOverlays.back()->SetPageFocus(1);
            } else if (mCurrentPage) {
                mCurrentPage->SetPageFocus(1); // Just in case somehow the regular page lost focus, we'll set it again
            }
        }
    }
    return 0;
}

const ResourceManager* PageSet::GetResources()
{
    return mResources;
}

Page* PageSet::FindPage(const std::string& name)
{
    for (auto iter = mPages.begin(); iter != mPages.end(); iter++) {
        if (name == (*iter)->GetName()) {
            return (*iter);
        }
    }
    return nullptr;
}

int PageSet::LoadVariables(xml_node<>* vars)
{
    xml_node<>* child;
    xml_attribute<> *name, *value, *persist;
    int p;

    child = vars->first_node("variable");
    while (child) {
        name = child->first_attribute("name");
        value = child->first_attribute("value");
        persist = child->first_attribute("persist");
        if (name && value) {
            if (strcmp(name->value(), "tw_x_offset") == 0) {
                tw_x_offset = atoi(value->value());
                child = child->next_sibling("variable");
                continue;
            }
            if (strcmp(name->value(), "tw_y_offset") == 0) {
                tw_y_offset = atoi(value->value());
                child = child->next_sibling("variable");
                continue;
            }
            p = persist ? atoi(persist->value()) : 0;
            std::string temp = value->value();
            std::string valstr = gui_parse_text(temp);

            if (valstr.find("+") != std::string::npos) {
                std::string val1str = valstr;
                val1str = val1str.substr(0, val1str.find('+'));
                std::string val2str = valstr;
                val2str = val2str.substr(val2str.find('+') + 1, std::string::npos);
                int val1 = atoi(val1str.c_str());
                int val2 = atoi(val2str.c_str());
                int val = val1 + val2;

                DataManager::SetValue(name->value(), val, p);
            } else if (valstr.find("-") != std::string::npos) {
                std::string val1str = valstr;
                val1str = val1str.substr(0, val1str.find('-'));
                std::string val2str = valstr;
                val2str = val2str.substr(val2str.find('-') + 1, std::string::npos);
                int val1 = atoi(val1str.c_str());
                int val2 = atoi(val2str.c_str());
                int val = val1 - val2;

                DataManager::SetValue(name->value(), val, p);
            } else {
                DataManager::SetValue(name->value(), valstr, p);
            }
        }

        child = child->next_sibling("variable");
    }
    return 0;
}

int PageSet::LoadPages(LoadingContext& ctx, xml_node<>* pages)
{
    xml_node<>* child;

    if (!pages) {
        return -1;
    }

    child = pages->first_node("page");
    while (child != nullptr) {
        Page* page = new Page(child, &ctx.templates);
        if (page->GetName().empty()) {
            LOGE("Unable to process load page");
            delete page;
        } else {
            mPages.push_back(page);
        }
        child = child->next_sibling("page");
    }
    if (mPages.size() > 0) {
        return 0;
    }
    return -1;
}

int PageSet::IsCurrentPage(Page* page)
{
    return ((mCurrentPage && mCurrentPage == page) ? 1 : 0);
}

std::string PageSet::GetCurrentPage() const
{
    return mCurrentPage ? mCurrentPage->GetName() : "";
}

int PageSet::Render()
{
    int ret;

    ret = (mCurrentPage ? mCurrentPage->Render() : -1);
    if (ret < 0) {
        return ret;
    }

    for (auto iter = mOverlays.begin(); iter != mOverlays.end(); iter++) {
        ret = ((*iter) ? (*iter)->Render() : -1);
        if (ret < 0) {
            return ret;
        }
    }
    return ret;
}

int PageSet::Update()
{
    int ret;

    ret = (mCurrentPage ? mCurrentPage->Update() : -1);
    if (ret < 0 || ret > 1) {
        return ret;
    }

    for (auto iter = mOverlays.begin(); iter != mOverlays.end(); iter++) {
        ret = ((*iter) ? (*iter)->Update() : -1);
        if (ret < 0) {
            return ret;
        }
    }
    return ret;
}

int PageSet::NotifyTouch(TOUCH_STATE state, int x, int y)
{
    if (!mOverlays.empty()) {
        return mOverlays.back()->NotifyTouch(state, x, y);
    }

    return (mCurrentPage ? mCurrentPage->NotifyTouch(state, x, y) : -1);
}

int PageSet::NotifyKey(int key, bool down)
{
    if (!mOverlays.empty()) {
        return mOverlays.back()->NotifyKey(key, down);
    }

    return (mCurrentPage ? mCurrentPage->NotifyKey(key, down) : -1);
}

int PageSet::NotifyCharInput(int ch)
{
    if (!mOverlays.empty()) {
        return mOverlays.back()->NotifyCharInput(ch);
    }

    return (mCurrentPage ? mCurrentPage->NotifyCharInput(ch) : -1);
}

int PageSet::SetKeyBoardFocus(int inFocus)
{
    if (!mOverlays.empty()) {
        return mOverlays.back()->SetKeyBoardFocus(inFocus);
    }

    return (mCurrentPage ? mCurrentPage->SetKeyBoardFocus(inFocus) : -1);
}

int PageSet::NotifyVarChange(const std::string& varName,
                             const std::string& value)
{
    for (auto iter = mOverlays.begin(); iter != mOverlays.end(); iter++) {
        (*iter)->NotifyVarChange(varName, value);
    }

    return (mCurrentPage ? mCurrentPage->NotifyVarChange(varName, value) : -1);
}

void PageSet::AddStringResource(std::string resource_source,
                                std::string resource_name,
                                std::string value)
{
    mResources->AddStringResource(std::move(resource_source),
                                  std::move(resource_name),
                                  std::move(value));
}

char* PageManager::LoadFileToBuffer(const std::string& filename,
                                    ZipArchive* package)
{
    size_t len;
    char* buffer = nullptr;

    if (!package) {
        // We can try to load the XML directly...
        LOGI("PageManager::LoadFileToBuffer loading filename: '%s' directly", filename.c_str());
        struct stat st;
        if (stat(filename.c_str(), &st) != 0) {
            // This isn't always an error, sometimes we request files that don't exist.
            return nullptr;
        }

        len = (size_t) st.st_size;

        buffer = (char*) malloc(len + 1);
        if (!buffer) {
            LOGE("PageManager::LoadFileToBuffer failed to malloc");
            return nullptr;
        }

        int fd = open(filename.c_str(), O_RDONLY);
        if (fd == -1) {
            LOGE("PageManager::LoadFileToBuffer failed to open '%s' - (%s)", filename.c_str(), strerror(errno));
            free(buffer);
            return nullptr;
        }

        if (read(fd, buffer, len) < 0) {
            LOGE("PageManager::LoadFileToBuffer failed to read '%s' - (%s)", filename.c_str(), strerror(errno));
            free(buffer);
            close(fd);
            return nullptr;
        }
        close(fd);
    } else {
        LOGI("PageManager::LoadFileToBuffer loading filename: '%s' from zip", filename.c_str());
        const ZipEntry* zipentry = mzFindZipEntry(package, filename.c_str());
        if (zipentry == nullptr) {
            LOGE("Unable to locate '%s' in zip file", filename.c_str());
            return nullptr;
        }

        // Allocate the buffer for the file
        len = mzGetZipEntryUncompLen(zipentry);
        buffer = (char*) malloc(len + 1);
        if (!buffer) {
            return nullptr;
        }

        if (!mzExtractZipEntryToBuffer(package, zipentry, (unsigned char*) buffer)) {
            LOGE("Unable to extract '%s'", filename.c_str());
            free(buffer);
            return nullptr;
        }
    }
    // NULL-terminate the string
    buffer[len] = 0x00;
    return buffer;
}

void PageManager::LoadLanguageListDir(const std::string& dir)
{
    if (!mb::util::path_exists(dir, false)) {
        LOGE("LoadLanguageListDir '%s' path not found", dir.c_str());
        return;
    }

    DIR *d = opendir(dir.c_str());
    struct dirent *p;

    if (d == nullptr) {
        LOGE("LoadLanguageListDir error opening dir: '%s', %s", dir.c_str(), strerror(errno));
        return;
    }

    while ((p = readdir(d))) {
        if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, "..") || strlen(p->d_name) < 5) {
            continue;
        }

        std::string file = p->d_name;
        if (file.substr(strlen(p->d_name) - 4) != ".xml") {
            continue;
        }
        std::string path = dir + p->d_name;
        std::string file_no_extn = file.substr(0, strlen(p->d_name) - 4);
        struct language_struct language_entry;
        language_entry.filename = file_no_extn;
        char* xmlFile = PageManager::LoadFileToBuffer(dir + p->d_name, nullptr);
        if (xmlFile == nullptr) {
            LOGE("LoadLanguageListDir unable to load '%s'", language_entry.filename.c_str());
            continue;
        }
        xml_document<> *doc = new xml_document<>();
        doc->parse<0>(xmlFile);

        xml_node<>* parent = doc->first_node("language");
        if (!parent) {
            LOGE("Invalid language XML file '%s'", language_entry.filename.c_str());
        } else {
            xml_node<>* child = parent->first_node("display");
            if (child) {
                language_entry.displayvalue = child->value();
            } else {
                LOGE("No display value for '%s'", language_entry.filename.c_str());
                language_entry.displayvalue = language_entry.filename;
            }
            Language_List.push_back(language_entry);
        }
        doc->clear();
        delete doc;
        free(xmlFile);
    }
    closedir(d);
}

void PageManager::LoadLanguageList(ZipArchive* package)
{
    Language_List.clear();
    if (mb::util::path_exists(TWFunc::get_resource_path("customlanguages"), false)) {
        (void) mb::util::delete_recursive(TWFunc::get_resource_path("customlanguages"));
    }
    if (package) {
        (void) mb::util::mkdir_recursive(TWFunc::get_resource_path("customlanguages"), 0700);
        struct utimbuf timestamp = { 1217592000, 1217592000 };  // 8/1/2008 default
#ifdef HAVE_SELINUX
        mzExtractRecursive(package, "languages", TWFunc::get_resource_path("customlanguages/").c_str(), &timestamp, nullptr, nullptr, nullptr);
#else
        mzExtractRecursive(package, "languages", TWFunc::get_resource_path("customlanguages/").c_str(), &timestamp, nullptr, nullptr);
#endif
        LoadLanguageListDir(TWFunc::get_resource_path("customlanguages/"));
    } else {
        LoadLanguageListDir(TWFunc::get_resource_path("languages/"));
    }

    std::sort(Language_List.begin(), Language_List.end());
}

void PageManager::LoadLanguage(const std::string& filename)
{
    std::string actual_filename;
    if (mb::util::path_exists(TWFunc::get_resource_path("customlanguages/" + filename + ".xml"), false)) {
        actual_filename = TWFunc::get_resource_path("customlanguages/" + filename + ".xml");
    } else {
        actual_filename = TWFunc::get_resource_path("languages/" + filename + ".xml");
    }
    char* xmlFile = PageManager::LoadFileToBuffer(actual_filename, nullptr);
    if (xmlFile == nullptr) {
        LOGE("Unable to load '%s'", actual_filename.c_str());
    } else {
        mCurrentSet->LoadLanguage(xmlFile, nullptr);
        free(xmlFile);
    }
}

int PageManager::LoadPackage(const std::string& name,
                             const std::string& package,
                             const std::string& startpage)
{
    std::string mainxmlfilename = package;
    ZipArchive zip;
    char* languageFile = nullptr;
    char* baseLanguageFile = nullptr;
    PageSet* pageSet = nullptr;
    int ret;
    MemMapping map;

    mReloadTheme = false;
    mStartPage = startpage;

    // init the loading context
    LoadingContext ctx;

    // Open the XML file
    LOGI("Loading package: %s (%s)", name.c_str(), package.c_str());
    if (package.size() > 4 && package.substr(package.size() - 4) != ".zip") {
        LOGI("Load XML directly");
        tw_x_offset = tw_device.tw_default_x_offset();
        tw_y_offset = tw_device.tw_default_y_offset();
        if (name != "splash") {
            LoadLanguageList(nullptr);
            languageFile = LoadFileToBuffer(TWFunc::get_resource_path("languages/en.xml"), nullptr);
        }
        ctx.basepath = tw_resource_path;
        ctx.basepath += '/';
    } else {
        LOGI("Loading zip theme");
        tw_x_offset = 0;
        tw_y_offset = 0;
        if (!mb::util::path_exists(package, false)) {
            return -1;
        }
        if (sysMapFile(package.c_str(), &map) != 0) {
            LOGE("Failed to map '%s'", package.c_str());
            goto error;
        }
        if (mzOpenZipArchive(map.addr, map.length, &zip)) {
            LOGE("Unable to open zip archive '%s'", package.c_str());
            sysReleaseMap(&map);
            goto error;
        }
        ctx.zip = &zip;
        mainxmlfilename = "ui.xml";
        LoadLanguageList(ctx.zip);
        languageFile = LoadFileToBuffer("languages/en.xml", ctx.zip);
        baseLanguageFile = LoadFileToBuffer(TWFunc::get_resource_path("languages/en.xml"), nullptr);
    }

    // Before loading, mCurrentSet must be the loading package so we can find resources
    pageSet = mCurrentSet;
    mCurrentSet = new PageSet();

    if (baseLanguageFile) {
        mCurrentSet->LoadLanguage(baseLanguageFile, nullptr);
        free(baseLanguageFile);
    }

    if (languageFile) {
        mCurrentSet->LoadLanguage(languageFile, ctx.zip);
        free(languageFile);
    }

    // Load and parse the XML and all includes
    currentLoadingContext = &ctx; // required to find styles
    ret = mCurrentSet->Load(ctx, mainxmlfilename);
    currentLoadingContext = nullptr;

    if (ret == 0) {
        mCurrentSet->SetPage(startpage);
        mPageSets.insert(std::pair<std::string, PageSet*>(name, mCurrentSet));
    } else {
        if (ret != TW_THEME_VER_ERR) {
            LOGE("Package %s failed to load.", name.c_str());
        }
    }

    // reset to previous pageset
    mCurrentSet = pageSet;

    if (ctx.zip) {
        mzCloseZipArchive(ctx.zip);
        sysReleaseMap(&map);
    }
    return ret;

error:
    // Sometimes we get here without a real error
    if (ctx.zip) {
        mzCloseZipArchive(ctx.zip);
        sysReleaseMap(&map);
    }
    return -1;
}

PageSet* PageManager::FindPackage(const std::string& name)
{
    auto iter = mPageSets.find(name);
    if (iter != mPageSets.end()) {
        return (*iter).second;
    }

    LOGE("Unable to locate package %s", name.c_str());
    return nullptr;
}

PageSet* PageManager::SelectPackage(const std::string& name)
{
    LOGI("Switching packages (%s)", name.c_str());
    PageSet* tmp;

    tmp = FindPackage(name);
    if (tmp) {
        mCurrentSet = tmp;
        mCurrentSet->MakeEmergencyConsoleIfNeeded();
        mCurrentSet->NotifyVarChange("", "");
    } else {
        LOGE("Unable to find package.");
    }

    return mCurrentSet;
}

int PageManager::ReloadPackage(const std::string& name,
                               const std::string& package)
{
    mReloadTheme = false;

    auto iter = mPageSets.find(name);
    if (iter == mPageSets.end()) {
        return -1;
    }

    if (mMouseCursor) {
        mMouseCursor->ResetData(gr_fb_width(), gr_fb_height());
    }

    PageSet* set = (*iter).second;
    mPageSets.erase(iter);

    if (LoadPackage(name, package, mStartPage) != 0) {
        LOGI("Failed to load package '%s'.", package.c_str());
        mPageSets.insert(std::pair<std::string, PageSet*>(name, set));
        return -1;
    }
    if (mCurrentSet == set) {
        SelectPackage(name);
    }
    delete set;
    GUIConsole::Translate_Now();
    return 0;
}

void PageManager::ReleasePackage(const std::string& name)
{
    auto iter = mPageSets.find(name);
    if (iter == mPageSets.end()) {
        return;
    }

    PageSet* set = (*iter).second;
    mPageSets.erase(iter);
    delete set;
    if (set == mCurrentSet) {
        mCurrentSet = nullptr;
    }
    return;
}

int PageManager::RunReload()
{
    int ret_val = 0;
    std::string theme_path;

    if (!mReloadTheme) {
        return 0;
    }

    mReloadTheme = false;

    if (ret_val != 0 || ReloadPackage("TWRP", tw_theme_zip_path) != 0) {
        // Loading the custom theme failed - try loading the stock theme
        LOGI("Attempting to reload stock theme...");
        if (ReloadPackage("TWRP", TWFunc::get_resource_path("ui.xml"))) {
            LOGE("Failed to load base packages.");
            ret_val = 1;
        }
    }
    if (ret_val == 0) {
        std::string language = DataManager::GetStrValue(VAR_TW_LANGUAGE);
        if (language != "en.xml") {
            LOGI("Loading language '%s'", language.c_str());
            LoadLanguage(language);
        }
    }

    // This makes the console re-translate
    last_message_count = 0;
    gConsole.clear();
    gConsoleColor.clear();

    return ret_val;
}

void PageManager::RequestReload()
{
    mReloadTheme = true;
}

void PageManager::SetStartPage(const std::string& page_name)
{
    mStartPage = page_name;
}

int PageManager::ChangePage(const std::string& name)
{
    DataManager::SetValue(VAR_TW_OPERATION_STATE, 0);
    int ret = (mCurrentSet ? mCurrentSet->SetPage(name) : -1);
    return ret;
}

std::string PageManager::GetCurrentPage()
{
    return mCurrentSet ? mCurrentSet->GetCurrentPage() : "";
}

int PageManager::ChangeOverlay(const std::string& name)
{
    if (name.empty()) {
        return mCurrentSet->SetOverlay(nullptr);
    } else {
        Page* page = mCurrentSet ? mCurrentSet->FindPage(name) : nullptr;
        return mCurrentSet->SetOverlay(page);
    }
}

const ResourceManager* PageManager::GetResources()
{
    return (mCurrentSet ? mCurrentSet->GetResources() : nullptr);
}

int PageManager::IsCurrentPage(Page* page)
{
    return (mCurrentSet ? mCurrentSet->IsCurrentPage(page) : 0);
}

int PageManager::Render()
{
    if (blankTimer.isScreenOff()) {
        return 0;
    }

    int res = (mCurrentSet ? mCurrentSet->Render() : -1);
    if (mMouseCursor) {
        mMouseCursor->Render();
    }
    return res;
}

HardwareKeyboard *PageManager::GetHardwareKeyboard()
{
    if (!mHardwareKeyboard) {
        mHardwareKeyboard = new HardwareKeyboard();
    }
    return mHardwareKeyboard;
}

xml_node<>* PageManager::FindStyle(std::string name)
{
    if (!currentLoadingContext) {
        LOGE("FindStyle works only while loading a theme.");
        return nullptr;
    }

    for (auto itr = currentLoadingContext->styles.begin(); itr != currentLoadingContext->styles.end(); itr++) {
        xml_node<>* node = (*itr)->first_node("style");

        while (node) {
            if (!node->first_attribute("name")) {
                continue;
            }

            if (name == node->first_attribute("name")->value()) {
                return node;
            }
            node = node->next_sibling("style");
        }
    }
    return nullptr;
}

MouseCursor *PageManager::GetMouseCursor()
{
    if (!mMouseCursor) {
        mMouseCursor = new MouseCursor(gr_fb_width(), gr_fb_height());
    }
    return mMouseCursor;
}

void PageManager::LoadCursorData(xml_node<>* node)
{
    if (!mMouseCursor) {
        mMouseCursor = new MouseCursor(gr_fb_width(), gr_fb_height());
    }

    mMouseCursor->LoadData(node);
}

int PageManager::Update()
{
    if (blankTimer.isScreenOff()) {
        return 0;
    }

    if (RunReload()) {
        return -2;
    }

    int res = (mCurrentSet ? mCurrentSet->Update() : -1);

    if (mMouseCursor) {
        int c_res = mMouseCursor->Update();
        if (c_res > res) {
            res = c_res;
        }
    }
    return res;
}

int PageManager::NotifyTouch(TOUCH_STATE state, int x, int y)
{
    return (mCurrentSet ? mCurrentSet->NotifyTouch(state, x, y) : -1);
}

int PageManager::NotifyKey(int key, bool down)
{
    return (mCurrentSet ? mCurrentSet->NotifyKey(key, down) : -1);
}

int PageManager::NotifyCharInput(int ch)
{
    return (mCurrentSet ? mCurrentSet->NotifyCharInput(ch) : -1);
}

int PageManager::SetKeyBoardFocus(int inFocus)
{
    return (mCurrentSet ? mCurrentSet->SetKeyBoardFocus(inFocus) : -1);
}

int PageManager::NotifyVarChange(std::string varName, std::string value)
{
    return (mCurrentSet ? mCurrentSet->NotifyVarChange(varName, value) : -1);
}

void PageManager::AddStringResource(std::string resource_source, std::string resource_name, std::string value)
{
    if (mCurrentSet) {
        mCurrentSet->AddStringResource(resource_source, resource_name, value);
    }
}

extern "C" void gui_notifyVarChange(const char *name, const char* value)
{
    if (!gGuiRunning) {
        return;
    }

    PageManager::NotifyVarChange(name, value);
}
