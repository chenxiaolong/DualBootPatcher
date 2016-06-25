// resources.hpp - Base classes for resource management of GUI

#ifndef _RESOURCE_HEADER
#define _RESOURCE_HEADER

#include "minuitwrp/minui.h"

#include "gui/rapidxml.hpp"

struct ZipArchive;

// Base Objects
class Resource
{
public:
    Resource(xml_node<>* node, ZipArchive* pZip);
    virtual ~Resource() {}

public:
    std::string GetName()
    {
        return mName;
    }

private:
    std::string mName;

protected:
    static int ExtractResource(ZipArchive* pZip,
                               const std::string& folderName,
                               const std::string& fileName,
                               const std::string& fileExtn,
                               const std::string& destFile);
    static void LoadImage(ZipArchive* pZip,
                          const std::string& file, gr_surface* surface);
    static void CheckAndScaleImage(gr_surface source, gr_surface* destination,
                                   int retain_aspect);
};

class FontResource : public Resource
{
public:
    FontResource(xml_node<>* node, ZipArchive* pZip);
    virtual ~FontResource();

public:
    void* GetResource()
    {
#if 0
        return this ? mFont : nullptr;
#else
        return mFont;
#endif
    }

    int GetHeight()
    {
#if 0
        return gr_ttf_getMaxFontHeight(this ? mFont : nullptr);
#else
        return gr_ttf_getMaxFontHeight(mFont);
#endif
    }

    void Override(xml_node<>* node, ZipArchive* pZip);

protected:
    void* mFont;

private:
    void LoadFont(xml_node<>* node, ZipArchive* pZip);
    void DeleteFont();

private:
    int origFontSize;
    void* origFont;
};

class ImageResource : public Resource
{
public:
    ImageResource(xml_node<>* node, ZipArchive* pZip);
    virtual ~ImageResource();

public:
    gr_surface GetResource()
    {
#if 0
        return this ? mSurface : nullptr;
#else
        return mSurface;
#endif
    }

    int GetWidth()
    {
#if 0
        return gr_get_width(this ? mSurface : nullptr);
#else
        return gr_get_width(mSurface);
#endif
    }

    int GetHeight()
    {
#if 0
        return gr_get_height(this ? mSurface : nullptr);
#else
        return gr_get_height(mSurface);
#endif
    }

protected:
    gr_surface mSurface;
};

class AnimationResource : public Resource
{
public:
    AnimationResource(xml_node<>* node, ZipArchive* pZip);
    virtual ~AnimationResource();

public:
    gr_surface GetResource()
    {
#if 0
        return (!this || mSurfaces.empty()) ? nullptr : mSurfaces.at(0);
#else
        return mSurfaces.empty() ? nullptr : mSurfaces.at(0);
#endif
    }

    gr_surface GetResource(int entry)
    {
#if 0
        return (!this || mSurfaces.empty()) ? nullptr : mSurfaces.at(entry);
#else
        return mSurfaces.empty() ? nullptr : mSurfaces.at(entry);
#endif
    }

    int GetWidth()
    {
#if 0
        return gr_get_width(this ? GetResource() : nullptr);
#else
        return gr_get_width(GetResource());
#endif
    }

    int GetHeight()
    {
#if 0
        return gr_get_height(this ? GetResource() : nullptr);
#else
        return gr_get_height(GetResource());
#endif
    }

    int GetResourceCount()
    {
        return mSurfaces.size();
    }

protected:
    std::vector<gr_surface> mSurfaces;
};

class ResourceManager
{
public:
    ResourceManager();
    virtual ~ResourceManager();
    void AddStringResource(std::string resource_source, std::string resource_name, std::string value);
    void LoadResources(xml_node<>* resList, ZipArchive* pZip, std::string resource_source);

public:
    FontResource* FindFont(const std::string& name) const;
    ImageResource* FindImage(const std::string& name) const;
    AnimationResource* FindAnimation(const std::string& name) const;
    std::string FindString(const std::string& name) const;
    std::string FindString(const std::string& name,
                           const std::string& default_string) const;
    void DumpStrings() const;

private:
    struct string_resource_struct
    {
        std::string value;
        std::string source;
    };
    std::vector<FontResource*> mFonts;
    std::vector<ImageResource*> mImages;
    std::vector<AnimationResource*> mAnimations;
    std::unordered_map<std::string, string_resource_struct> mStrings;
};

#endif  // _RESOURCE_HEADER
