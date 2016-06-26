// resource.cpp - Source to manage GUI resources

#include "gui/objects.hpp"

#include <iomanip>
#include <iostream>

#include <fcntl.h>
#include <unistd.h>

#include "mblog/logging.h"

#include "twrp-functions.hpp"

#include "gui/gui.h"

#define TMP_RESOURCE_NAME   "/tmp/extract.bin"

Resource::Resource(xml_node<>* node, ZipArchive* pZip __unused)
{
    if (node && node->first_attribute("name")) {
        mName = node->first_attribute("name")->value();
    }
}

int Resource::ExtractResource(ZipArchive* pZip,
                              const std::string& folderName,
                              const std::string& fileName,
                              const std::string& fileExtn,
                              const std::string& destFile)
{
    if (!pZip) {
        return -1;
    }

    std::string src = folderName + "/" + fileName + fileExtn;

    const ZipEntry* binary = mzFindZipEntry(pZip, src.c_str());
    if (binary == nullptr) {
        return -1;
    }

    unlink(destFile.c_str());
    int fd = creat(destFile.c_str(), 0666);
    if (fd < 0) {
        return -1;
    }

    int ret = 0;
    if (!mzExtractZipEntryToFile(pZip, binary, fd)) {
        ret = -1;
    }

    close(fd);
    return ret;
}

void Resource::LoadImage(ZipArchive* pZip, const std::string& file,
                         gr_surface* surface)
{
    int rc = 0;
    if (ExtractResource(pZip, "images", file, ".png", TMP_RESOURCE_NAME) == 0) {
        rc = res_create_surface(TMP_RESOURCE_NAME, surface);
        unlink(TMP_RESOURCE_NAME);
    } else if (ExtractResource(pZip, "images", file, "", TMP_RESOURCE_NAME) == 0) {
        // JPG includes the .jpg extension in the filename so extension should be blank
        rc = res_create_surface(TMP_RESOURCE_NAME, surface);
        unlink(TMP_RESOURCE_NAME);
    } else if (!pZip) {
        // File name in xml may have included .png so try without adding .png
        rc = res_create_surface(file.c_str(), surface);
    }
    if (rc != 0) {
        LOGI("Failed to load image from %s%s, error %d", file.c_str(), pZip ? " (zip)" : "", rc);
    }
}

void Resource::CheckAndScaleImage(gr_surface source, gr_surface* destination, int retain_aspect)
{
    if (!source) {
        *destination = nullptr;
        return;
    }
    if (get_scale_w() != 0 && get_scale_h() != 0) {
        float scale_w = get_scale_w(), scale_h = get_scale_h();
        if (retain_aspect) {
            if (scale_w < scale_h) {
                scale_h = scale_w;
            } else {
                scale_w = scale_h;
            }
        }
        if (res_scale_surface(source, destination, scale_w, scale_h)) {
            LOGI("Error scaling image, using regular size.");
            *destination = source;
        }
    } else {
        *destination = source;
    }
}

FontResource::FontResource(xml_node<>* node, ZipArchive* pZip)
    : Resource(node, pZip)
{
    origFontSize = 0;
    origFont = nullptr;
    LoadFont(node, pZip);
}

void FontResource::LoadFont(xml_node<>* node, ZipArchive* pZip)
{
    std::string file;
    xml_attribute<>* attr;

    mFont = nullptr;
    if (!node) {
        return;
    }

    attr = node->first_attribute("filename");
    if (!attr) {
        return;
    }

    file = attr->value();

    if (file.size() >= 4 && file.compare(file.size() - 4, 4, ".ttf") == 0) {
        int font_size = 0;

        if (origFontSize != 0) {
            attr = node->first_attribute("scale");
            if (attr == nullptr) {
                return;
            }
            font_size = origFontSize * atoi(attr->value()) / 100;
        } else {
            attr = node->first_attribute("size");
            if (attr == nullptr) {
                return;
            }
            font_size = scale_theme_min(atoi(attr->value()));
            origFontSize = font_size;
        }

        int dpi = 300;

        attr = node->first_attribute("dpi");
        if (attr) {
            dpi = atoi(attr->value());
        }

        // we can't use TMP_RESOURCE_NAME here because the ttf subsystem is caching the name and scaling needs to reload the font
        std::string tmpname = "/tmp/" + file;
        if (ExtractResource(pZip, "fonts", file, "", tmpname) == 0) {
            mFont = gr_ttf_loadFont(tmpname.c_str(), font_size, dpi);
        } else {
            file = TWFunc::get_resource_path("fonts/" + file);
            mFont = gr_ttf_loadFont(file.c_str(), font_size, dpi);
        }
    } else {
        LOGE("Non-TTF fonts are no longer supported.");
    }
}

void FontResource::DeleteFont()
{
    if (mFont) {
        gr_ttf_freeFont(mFont);
    }
    mFont = nullptr;
    if (origFont) {
        gr_ttf_freeFont(origFont);
    }
    origFont = nullptr;
}

void FontResource::Override(xml_node<>* node, ZipArchive* pZip)
{
    if (!origFont) {
        origFont = mFont;
    } else if (mFont) {
        gr_ttf_freeFont(mFont);
        mFont = nullptr;
    }
    LoadFont(node, pZip);
}

FontResource::~FontResource()
{
    DeleteFont();
}

ImageResource::ImageResource(xml_node<>* node, ZipArchive* pZip)
    : Resource(node, pZip)
{
    std::string file;
    gr_surface temp_surface = nullptr;

    mSurface = nullptr;
    if (!node) {
        LOGE("ImageResource node is NULL");
        return;
    }

    if (node->first_attribute("filename")) {
        file = node->first_attribute("filename")->value();
    } else {
        LOGE("No filename specified for image resource.");
        return;
    }

    bool retain_aspect = (node->first_attribute("retainaspect") != nullptr);
    // the value does not matter, if retainaspect is present, we assume that we want to retain it
    LoadImage(pZip, file, &temp_surface);
    CheckAndScaleImage(temp_surface, &mSurface, retain_aspect);
}

ImageResource::~ImageResource()
{
    if (mSurface) {
        res_free_surface(mSurface);
    }
}

AnimationResource::AnimationResource(xml_node<>* node, ZipArchive* pZip)
    : Resource(node, pZip)
{
    std::string file;
    int fileNum = 1;

    if (!node) {
        return;
    }

    if (node->first_attribute("filename")) {
        file = node->first_attribute("filename")->value();
    } else {
        LOGE("No filename specified for image resource.");
        return;
    }

    bool retain_aspect = (node->first_attribute("retainaspect") != nullptr);
    // the value does not matter, if retainaspect is present, we assume that we want to retain it
    for (;;) {
        std::ostringstream fileName;
        fileName << file << std::setfill ('0') << std::setw (3) << fileNum;

        gr_surface surface, temp_surface = nullptr;
        LoadImage(pZip, fileName.str(), &temp_surface);
        CheckAndScaleImage(temp_surface, &surface, retain_aspect);
        if (surface) {
            mSurfaces.push_back(surface);
            fileNum++;
        } else {
            break; // Done loading animation images
        }
    }
}

AnimationResource::~AnimationResource()
{
    for (auto it = mSurfaces.begin(); it != mSurfaces.end(); ++it) {
        res_free_surface(*it);
    }

    mSurfaces.clear();
}

FontResource* ResourceManager::FindFont(const std::string& name) const
{
    for (auto it = mFonts.begin(); it != mFonts.end(); ++it) {
        if (name == (*it)->GetName()) {
            return *it;
        }
    }
    return nullptr;
}

ImageResource* ResourceManager::FindImage(const std::string& name) const
{
    for (auto it = mImages.begin(); it != mImages.end(); ++it) {
        if (name == (*it)->GetName()) {
            return *it;
        }
    }
    return nullptr;
}

AnimationResource* ResourceManager::FindAnimation(const std::string& name) const
{
    for (auto it = mAnimations.begin(); it != mAnimations.end(); ++it) {
        if (name == (*it)->GetName()) {
            return *it;
        }
    }
    return nullptr;
}

std::string ResourceManager::FindString(const std::string& name) const
{
#if 0
    if (this != nullptr) {
#endif
        auto it = mStrings.find(name);
        if (it != mStrings.end()) {
            return it->second.value;
        }
        LOGE("String resource '%s' not found. No default value.", name.c_str());
        PageManager::AddStringResource("NO DEFAULT", name, "[" + name + ("]"));
#if 0
    } else {
        LOGI("String resources not loaded when looking for '%s'. No default value.", name.c_str());
    }
#endif
    return "[" + name + ("]");
}

std::string ResourceManager::FindString(const std::string& name,
                                        const std::string& default_string) const
{
#if 0
    if (this != nullptr) {
#endif
        auto it = mStrings.find(name);
        if (it != mStrings.end()) {
            return it->second.value;
        }
        LOGE("String resource '%s' not found. Using default value.", name.c_str());
        PageManager::AddStringResource("DEFAULT", name, default_string);
#if 0
    } else {
        LOGI("String resources not loaded when looking for '%s'. Using default value.", name.c_str());
    }
#endif
    return default_string;
}

void ResourceManager::DumpStrings() const
{
#if 0
    if (this == nullptr) {
        gui_print("No string resources\n");
        return;
    }
#endif
    gui_print("Dumping all strings:\n");
    for (auto it = mStrings.begin(); it != mStrings.end(); it++) {
        gui_print("source: %s: '%s' = '%s'\n", it->second.source.c_str(), it->first.c_str(), it->second.value.c_str());
    }
    gui_print("Done dumping strings\n");
}

ResourceManager::ResourceManager()
{
}

void ResourceManager::AddStringResource(std::string resource_source,
                                        std::string resource_name,
                                        std::string value)
{
    string_resource_struct res;
    res.source = std::move(resource_source);
    res.value = std::move(value);
    mStrings[resource_name] = res;
}

void ResourceManager::LoadResources(xml_node<>* resList, ZipArchive* pZip,
                                    std::string resource_source)
{
    if (!resList) {
        return;
    }

    for (xml_node<>* child = resList->first_node(); child; child = child->next_sibling()) {
        std::string type = child->name();
        if (type == "resource") {
            // legacy format : <resource type="...">
            xml_attribute<>* attr = child->first_attribute("type");
            type = attr ? attr->value() : "*unspecified*";
        }

        bool error = false;
        if (type == "font") {
            FontResource* res = new FontResource(child, pZip);
            if (res->GetResource()) {
                mFonts.push_back(res);
            } else {
                error = true;
                delete res;
            }
        } else if (type == "fontoverride") {
            if (mFonts.size() != 0 && child && child->first_attribute("name")) {
                std::string FontName = child->first_attribute("name")->value();
                size_t font_count = mFonts.size(), i;
                bool found = false;

                for (i = 0; i < font_count; i++) {
                    if (mFonts[i]->GetName() == FontName) {
                        mFonts[i]->Override(child, pZip);
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    LOGE("Unable to locate font '%s' for override.", FontName.c_str());
                }
            } else if (mFonts.size() != 0) {
                LOGE("Unable to locate font name for type fontoverride.");
            }
        } else if (type == "image") {
            ImageResource* res = new ImageResource(child, pZip);
            if (res->GetResource()) {
                mImages.push_back(res);
            } else {
                error = true;
                delete res;
            }
        } else if (type == "animation") {
            AnimationResource* res = new AnimationResource(child, pZip);
            if (res->GetResourceCount()) {
                mAnimations.push_back(res);
            } else {
                error = true;
                delete res;
            }
        } else if (type == "string") {
            if (xml_attribute<>* attr = child->first_attribute("name")) {
                string_resource_struct res;
                res.source = resource_source;
                res.value = child->value();
                mStrings[attr->value()] = res;
            } else {
                error = true;
            }
        } else {
            LOGE("Resource type (%s) not supported.", type.c_str());
            error = true;
        }

        if (error) {
            std::string res_name;
            if (child->first_attribute("name")) {
                res_name = child->first_attribute("name")->value();
            }
            if (res_name.empty() && child->first_attribute("filename")) {
                res_name = child->first_attribute("filename")->value();
            }

            if (!res_name.empty()) {
                LOGE("Resource (%s)-(%s) failed to load", type.c_str(), res_name.c_str());
            } else {
                LOGE("Resource type (%s) failed to load", type.c_str());
            }
        }
    }
}

ResourceManager::~ResourceManager()
{
    for (auto it = mFonts.begin(); it != mFonts.end(); ++it) {
        delete *it;
    }

    for (auto it = mImages.begin(); it != mImages.end(); ++it) {
        delete *it;
    }

    for (auto it = mAnimations.begin(); it != mAnimations.end(); ++it) {
        delete *it;
    }
}
