/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of MultiBootPatcher
 *
 * MultiBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultiBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "apk.h"

#include <androidfw/ResourceTypes.h>
#include <utils/String8.h>

#include "util/finally.h"
#include "util/fts.h"
#include "util/logging.h"
#include "util/string.h"

#include "external/minizip/unzip.h"

namespace mb
{

static bool read_to_memory(unzFile uf, std::vector<unsigned char> *out)
{
    unz_file_info64 fi;
    memset(&fi, 0, sizeof(fi));

    int ret = unzGetCurrentFileInfo64(
        uf,         // file
        &fi,        // pfile_info
        nullptr,    // filename
        0,          // filename_size
        nullptr,    // extrafield
        0,          // extrafield_size
        nullptr,    // comment
        0           // comment_size
    );

    if (ret != UNZ_OK) {
        return false;
    }

    std::vector<unsigned char> data;
    data.reserve(fi.uncompressed_size);

    ret = unzOpenCurrentFile(uf);
    if (ret != UNZ_OK) {
        return false;
    }

    char buf[50 * 1024]; // 50 KiB
    int bytes_read;

    while ((bytes_read = unzReadCurrentFile(uf, buf, sizeof(buf))) > 0) {
        data.insert(data.end(), buf, buf + bytes_read);
    }

    unzCloseCurrentFile(uf);

    if (bytes_read != 0) {
        return false;
    }

    data.swap(*out);
    return true;
}

bool ApkFile::open(const std::string &path)
{
    unzFile uf = unzOpen(path.c_str());
    if (!uf) {
        LOGE("{}: Failed to open archive", path.c_str());
        return false;
    }

    auto close_uf = util::finally([&]{
        unzClose(uf);
    });

    if (unzLocateFile(uf, "AndroidManifest.xml", nullptr) != UNZ_OK) {
        LOGE("{}: package does not contain AndroidManifest.xml", path.c_str());
        return false;
    }

    std::vector<unsigned char> data;
    if (!read_to_memory(uf, &data)) {
        LOGE("{}: Failed to extract AndroidManifest.xml", path.c_str());
        return false;
    }

    // Load AndroidManifest.xml and check package name
    return parse_manifest(data.data(), data.size());
}

struct namespace_entry {
    android::String8 prefix;
    android::String8 uri;
};

static android::String8 build_ns(const std::vector<namespace_entry> &namespaces,
                                 const char16_t *ns)
{
    android::String8 str;
    if (ns) {
        str = android::String8(ns);
        for (const namespace_entry &ne : namespaces) {
            if (ne.uri == str) {
                str = ne.prefix;
                break;
            }
        }
        str.append(":");
    }
    return str;
}

bool ApkFile::parse_manifest(const void *data, const std::size_t size)
{
    android::ResXMLTree tree;

    auto free_xml_when_finished = util::finally([&]{
        tree.uninit();
    });

    if (tree.setTo(data, size) != android::NO_ERROR) {
        LOGE("Binary XML resource is corrupt");
        return false;
    }

    tree.restart();
    int depth = 0;
    std::vector<android::String8> stack;

    std::vector<namespace_entry> namespaces;

    android::ResXMLTree::event_code_t code;
    while ((code = tree.next()) != android::ResXMLTree::END_DOCUMENT
            && code != android::ResXMLTree::BAD_DOCUMENT) {
        if (code == android::ResXMLTree::START_TAG) {
            ++depth;

            // Get element name
            std::size_t len;
            const char16_t *ns16 = tree.getElementNamespace(&len);
            android::String8 name = build_ns(namespaces, ns16);
            name.append(android::String8(tree.getElementName(&len)));

            if (depth == 1 && name == "manifest") {
                // Check "package" attribute
                int numattrs = tree.getAttributeCount();
                for (int i = 0; i < numattrs; ++i) {
                    // Get attribute name
                    ns16 = tree.getAttributeNamespace(i, &len);
                    name = build_ns(namespaces, ns16);
                    name.append(android::String8(
                            tree.getAttributeName(i, &len)));

                    if (name == "package") {
                        android::Res_value value;
                        tree.getAttributeValue(i, &value);

                        if (value.dataType != android::Res_value::TYPE_STRING) {
                            LOGE("package attribute in <manifest> is not a string");
                            return false;
                        }

                        android::String8 string_value(
                                tree.getAttributeStringValue(i, &len));
                        package = string_value.string();
                    } else if (name == "android:versionCode") {
                        android::Res_value value;
                        tree.getAttributeValue(i, &value);

                        if (value.dataType != android::Res_value::TYPE_INT_DEC) {
                            LOGE("android:versionCode attribute in <manifest> is not a decimal number");
                            return false;
                        }

                        version_code = value.data;
                    } else if (name == "android:versionName") {
                        android::Res_value value;
                        tree.getAttributeValue(i, &value);

                        if (value.dataType != android::Res_value::TYPE_STRING) {
                            LOGE("android:versionName attribute in <manifest> is not a string");
                            return false;
                        }

                        android::String8 string_value(
                                tree.getAttributeStringValue(i, &len));
                        version_name = string_value.string();
                    }
                }

                return true;
            }
        } else if (code == android::ResXMLTree::END_TAG) {
            --depth;
        } else if (code == android::ResXMLTree::START_NAMESPACE) {
            namespace_entry ns;
            std::size_t len;
            const char16_t *prefix16 = tree.getNamespacePrefix(&len);
            if (prefix16) {
                ns.prefix = android::String8(prefix16);
            } else {
                ns.prefix = "<DEF>";
            }
            ns.uri = android::String8(tree.getNamespaceUri(&len));

            namespaces.push_back(ns);
        } else if (code == android::ResXMLTree::END_NAMESPACE) {
            const namespace_entry &ns = namespaces.front();
            std::size_t len;
            const char16_t *prefix16 = tree.getNamespacePrefix(&len);
            android::String8 pr;
            if (prefix16) {
                pr = android::String8(prefix16);
            } else {
                pr = "<DEF>";
            }
            if (ns.prefix != pr) {
                LOGE("Bad end namespace prefix (found={}, expected={})",
                     pr.string(), ns.prefix.string());
            }

            android::String8 uri(tree.getNamespaceUri(&len));
            if (ns.uri != uri) {
                LOGE("Bad end namespace URI (found={}, expected={})",
                     uri.string(), ns.uri.string());
            }

            namespaces.pop_back();
        }
    }

    LOGE("<manifest> element not found");
    return false;
}

/*!
 * \brief Find apk in directory using the AndroidManifest.xml metadata
 */
class ApkFinder : public util::FTSWrapper {
public:
    ApkFinder(std::string path, std::string package)
        : FTSWrapper(path, FTS_GroupSpecialFiles),
        _package(package)
    {
    }

    virtual int on_changed_path() override
    {
        // Don't search beyond the 2nd level
        if (_curr->fts_level > 2) {
            return Action::FTS_Skip;
        }

        return Action::FTS_OK;
    }

    virtual int on_reached_file() override
    {
        // Skip non-APK files
        if (!util::ends_with(_curr->fts_name, ".apk")) {
            return Action::FTS_Skip;
        }

        ApkFile af;
        if (!af.open(_curr->fts_accpath)) {
            LOGE("{}: Failed to open or parse apk", _curr->fts_path);
            return Action::FTS_Skip;
        }

        if (af.package == _package) {
            _apk = _curr->fts_path;
            return Action::FTS_Stop;
        }

        return Action::FTS_OK;
    }

    std::string find()
    {
        if (run() && !_apk.empty()) {
            return _apk;
        }
        return std::string();
    }

private:
    std::string _package;
    std::string _apk;
};

std::string find_apk(const std::string &directory, const std::string &pkgname)
{
    ApkFinder af(directory, pkgname);
    return af.find();
}

}
