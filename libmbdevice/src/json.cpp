/*
 * Copyright (C) 2016-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of DualBootPatcher
 *
 * DualBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DualBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DualBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mbdevice/json.h"

#include <array>

#include <cassert>
#include <cstring>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/schema.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "mbcommon/string.h"
#include "mbdevice/schema.h"


using namespace rapidjson;

namespace mb::device
{

using DeviceFlagMapping = std::pair<const char *, DeviceFlag>;
using TwPixelFormatMapping = std::pair<const char *, TwPixelFormat>;

static constexpr std::array<DeviceFlagMapping, 2> g_device_flag_mappings{{
    { "HAS_COMBINED_BOOT_AND_RECOVERY", DeviceFlag::HasCombinedBootAndRecovery },
    { "FSTAB_SKIP_SDCARD0",             DeviceFlag::FstabSkipSdcard0 },
}};

static constexpr std::array<TwPixelFormatMapping, 5> g_tw_pxfmt_mappings{{
    { "DEFAULT",   TwPixelFormat::Default },
    { "ABGR_8888", TwPixelFormat::Abgr8888 },
    { "RGBX_8888", TwPixelFormat::Rgbx8888 },
    { "BGRA_8888", TwPixelFormat::Bgra8888 },
    { "RGBA_8888", TwPixelFormat::Rgba8888 },
}};

static void json_error_set_parse_error(JsonError &error, size_t offset,
                                       std::string message)
{
    error.type = JsonErrorType::ParseError;
    error.offset = offset;
    error.message = std::move(message);
}

static void json_error_set_schema_validation_failure(JsonError &error,
                                                     std::string schema_uri,
                                                     std::string schema_keyword,
                                                     std::string document_uri)
{
    error.type = JsonErrorType::SchemaValidationFailure;
    error.schema_uri = std::move(schema_uri);
    error.schema_keyword = std::move(schema_keyword);
    error.document_uri = std::move(document_uri);
}

static inline std::string get_string(const Value &node)
{
    return {node.GetString(), node.GetStringLength()};
}

static inline std::vector<std::string> get_string_array(const Value &node)
{
    std::vector<std::string> array;

    for (auto const &item : node.GetArray()) {
        array.push_back(get_string(item));
    }

    return array;
}

static void process_device_flags(Device &device, const Value &node)
{
    DeviceFlags flags = 0;

    for (auto const &item : node.GetArray()) {
        auto const &str = get_string(item);
        DeviceFlags old_flags = flags;

        for (auto const &mapping : g_device_flag_mappings) {
            if (str == mapping.first) {
                flags |= mapping.second;
                break;
            }
        }

        (void) old_flags;
        assert(flags != old_flags);
    }

    device.set_flags(flags);
}

static void process_boot_ui_pixel_format(Device &device, const Value &node)
{
    auto const &str = get_string(node);

    for (auto const &item : g_tw_pxfmt_mappings) {
        if (str == item.first) {
            device.set_tw_pixel_format(item.second);
            return;
        }
    }

    assert(false);
}

static void process_boot_ui(Device &device, const Value &node)
{
    for (auto const &item : node.GetObject()) {
        auto const &key = get_string(item.name);

        if (key == "supported") {
            device.set_tw_supported(item.value.GetBool());
        } else if (key == "pixel_format") {
            process_boot_ui_pixel_format(device, item.value);
        } else {
            assert(false);
        }
    }
}

static void process_block_devs(Device &device, const Value &node)
{
    for (auto const &item : node.GetObject()) {
        auto const &key = get_string(item.name);

        if (key == "base_dirs") {
            device.set_block_dev_base_dirs(get_string_array(item.value));
        } else if (key == "system") {
            device.set_system_block_devs(get_string_array(item.value));
        } else if (key == "cache") {
            device.set_cache_block_devs(get_string_array(item.value));
        } else if (key == "data") {
            device.set_data_block_devs(get_string_array(item.value));
        } else if (key == "boot") {
            device.set_boot_block_devs(get_string_array(item.value));
        } else if (key == "recovery") {
            device.set_recovery_block_devs(get_string_array(item.value));
        } else if (key == "extra") {
            device.set_extra_block_devs(get_string_array(item.value));
        } else {
            assert(false);
        }
    }
}

static void process_device(Device &device, const Value &node)
{
    for (auto const &item : node.GetObject()) {
        auto const &key = get_string(item.name);

        if (key == "name") {
            device.set_name(get_string(item.value));
        } else if (key == "id") {
            device.set_id(get_string(item.value));
        } else if (key == "codenames") {
            device.set_codenames(get_string_array(item.value));
        } else if (key == "architecture") {
            device.set_architecture(get_string(item.value));
        } else if (key == "flags") {
            process_device_flags(device, item.value);
        } else if (key == "block_devs") {
            process_block_devs(device, item.value);
        } else if (key == "boot_ui") {
            process_boot_ui(device, item.value);
        } else {
            assert(false);
        }
    }
}

bool device_from_json(const std::string &json, Device &device, JsonError &error)
{
    DeviceSchemaProvider<> sp;
    const SchemaDocument *sd = sp.GetSchema("device.json");
    if (!sd) {
        assert(false);
        return false;
    }

    Document d;
    StringStream is(json.c_str());
    SchemaValidatingReader<kParseDefaultFlags, StringStream, UTF8<>> reader(is, *sd);
    d.Populate(reader);

    const ParseResult &result = reader.GetParseResult();
    if (!result) {
        if (!reader.IsValid()) {
            StringBuffer sb;
            reader.GetInvalidSchemaPointer().StringifyUriFragment(sb);
            std::string schema_uri{sb.GetString(), sb.GetLength()};
            sb.Clear();
            reader.GetInvalidDocumentPointer().StringifyUriFragment(sb);
            std::string document_uri{sb.GetString(), sb.GetLength()};

            json_error_set_schema_validation_failure(
                    error, std::move(schema_uri),
                    reader.GetInvalidSchemaKeyword(), std::move(document_uri));
        } else {
            json_error_set_parse_error(error, result.Offset(),
                                       GetParseError_En(result.Code()));
        }
        return false;
    }

    device = Device();
    process_device(device, d);
    return true;
}

bool device_list_from_json(const std::string &json,
                           std::vector<Device> &devices,
                           JsonError &error)
{
    DeviceSchemaProvider<> sp;
    const SchemaDocument *sd = sp.GetSchema("device_list.json");
    if (!sd) {
        assert(false);
        return false;
    }

    Document d;
    StringStream is(json.c_str());
    SchemaValidatingReader<kParseDefaultFlags, StringStream, UTF8<>> reader(is, *sd);
    d.Populate(reader);

    const ParseResult &result = reader.GetParseResult();
    if (!result) {
        if (!reader.IsValid()) {
            StringBuffer sb;
            reader.GetInvalidSchemaPointer().StringifyUriFragment(sb);
            std::string schema_uri{sb.GetString(), sb.GetLength()};
            sb.Clear();
            reader.GetInvalidDocumentPointer().StringifyUriFragment(sb);
            std::string document_uri{sb.GetString(), sb.GetLength()};

            json_error_set_schema_validation_failure(
                    error, std::move(schema_uri),
                    reader.GetInvalidSchemaKeyword(), std::move(document_uri));
        } else {
            json_error_set_parse_error(error, result.Offset(),
                                       GetParseError_En(result.Code()));
        }
        return false;
    }

    std::vector<Device> array;

    for (auto const &item : d.GetArray()) {
        Device device;
        process_device(device, item);
        array.push_back(std::move(device));
    }

    devices.swap(array);
    return true;
}

bool device_to_json(const Device &device, std::string &json)
{
    Document d;
    d.SetObject();

    auto &alloc = d.GetAllocator();

    auto const &id = device.id();
    if (!id.empty()) {
        d.AddMember("id", id, alloc);
    }

    auto const &codenames = device.codenames();
    if (!codenames.empty()) {
        Value array(kArrayType);
        for (auto const &c : codenames) {
            array.PushBack(StringRef(c), alloc);
        }
        d.AddMember("codenames", array, alloc);
    }

    auto const &name = device.name();
    if (!name.empty()) {
        d.AddMember("name", name, alloc);
    }

    auto const &architecture = device.architecture();
    if (!architecture.empty()) {
        d.AddMember("architecture", architecture, alloc);
    }

    auto const flags = device.flags();
    if (flags) {
        Value array(kArrayType);
        for (auto const &item : g_device_flag_mappings) {
            if (flags & item.second) {
                array.PushBack(StringRef(item.first), alloc);
            }
        }
        d.AddMember("flags", array, alloc);
    }

    /* Block devs */
    Value block_devs(kObjectType);

    auto const &base_dirs = device.block_dev_base_dirs();
    if (!base_dirs.empty()) {
        Value array(kArrayType);
        for (auto const &p : base_dirs) {
            array.PushBack(StringRef(p), alloc);
        }
        block_devs.AddMember("base_dirs", array, alloc);
    }

    auto const &system_devs = device.system_block_devs();
    if (!system_devs.empty()) {
        Value array(kArrayType);
        for (auto const &p : system_devs) {
            array.PushBack(StringRef(p), alloc);
        }
        block_devs.AddMember("system", array, alloc);
    }

    auto const &cache_devs = device.cache_block_devs();
    if (!cache_devs.empty()) {
        Value array(kArrayType);
        for (auto const &p : cache_devs) {
            array.PushBack(StringRef(p), alloc);
        }
        block_devs.AddMember("cache", array, alloc);
    }

    auto const &data_devs = device.data_block_devs();
    if (!data_devs.empty()) {
        Value array(kArrayType);
        for (auto const &p : data_devs) {
            array.PushBack(StringRef(p), alloc);
        }
        block_devs.AddMember("data", array, alloc);
    }

    auto const &boot_devs = device.boot_block_devs();
    if (!boot_devs.empty()) {
        Value array(kArrayType);
        for (auto const &p : boot_devs) {
            array.PushBack(StringRef(p), alloc);
        }
        block_devs.AddMember("boot", array, alloc);
    }

    auto const &recovery_devs = device.recovery_block_devs();
    if (!recovery_devs.empty()) {
        Value array(kArrayType);
        for (auto const &p : recovery_devs) {
            array.PushBack(StringRef(p), alloc);
        }
        block_devs.AddMember("recovery", array, alloc);
    }

    auto const &extra_devs = device.extra_block_devs();
    if (!extra_devs.empty()) {
        Value array(kArrayType);
        for (auto const &p : extra_devs) {
            array.PushBack(StringRef(p), alloc);
        }
        block_devs.AddMember("extra", array, alloc);
    }

    if (!block_devs.ObjectEmpty()) {
        d.AddMember("block_devs", block_devs, alloc);
    }

    /* Boot UI */
    Value boot_ui(kObjectType);

    if (device.tw_supported()) {
        boot_ui.AddMember("supported", true, alloc);
    }

    auto const pixel_format = device.tw_pixel_format();
    if (pixel_format != TwPixelFormat::Default) {
        for (auto const &item : g_tw_pxfmt_mappings) {
            if (pixel_format == item.second) {
                boot_ui.AddMember("pixel_format", StringRef(item.first), alloc);
                break;
            }
        }
    }

    if (!boot_ui.ObjectEmpty()) {
        d.AddMember("boot_ui", boot_ui, alloc);
    }

    StringBuffer sb;
    Writer<StringBuffer> writer(sb);
    DeviceSchemaProvider<> sp;
    const SchemaDocument *sd = sp.GetSchema("device.json");
    if (!sd) {
        assert(false);
        return false;
    }
    GenericSchemaValidator<SchemaDocument, decltype(writer)> sv(*sd, writer);

    if (!d.Accept(sv)) {
        return false;
    }

    json = {sb.GetString(), sb.GetSize()};
    return true;
}

}
