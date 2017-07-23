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

#include <cerrno>
#include <cinttypes>
#include <cstring>

#include <getopt.h>

#include <jansson.h>
#include <yaml-cpp/yaml.h>

#include "mbdevice/json.h"


using ScopedJsonT = std::unique_ptr<json_t, decltype(json_decref) *>;
using ScopedCharArray = std::unique_ptr<char, decltype(free) *>;

using namespace mb::device;

static void * xmalloc(size_t size)
{
    void *result = malloc(size);
    if (!result) {
        abort();
    }
    return result;
}

static json_t * yaml_node_to_json_node(const YAML::Node &yaml_node)
{
    switch (yaml_node.Type()) {
    case YAML::NodeType::Null:
        return json_object();

    case YAML::NodeType::Scalar:
        try {
            return json_integer(yaml_node.as<json_int_t>());
        } catch (const YAML::BadConversion &e) {}
        try {
            return json_real(yaml_node.as<double>());
        } catch (const YAML::BadConversion &e) {}
        try {
            return json_boolean(yaml_node.as<bool>());
        } catch (const YAML::BadConversion &e) {}
        try {
            return json_string(yaml_node.as<std::string>().c_str());
        } catch (const YAML::BadConversion &e) {}
        throw std::runtime_error("Cannot convert scalar value to known type");

    case YAML::NodeType::Sequence: {
        json_t *array = json_array();

        for (auto const &item : yaml_node) {
            json_array_append_new(array, yaml_node_to_json_node(item));
        }

        return array;
    }

    case YAML::NodeType::Map: {
        json_t *object = json_object();

        for (auto const &item : yaml_node) {
            json_object_set_new(object, item.first.as<std::string>().c_str(),
                                yaml_node_to_json_node(item.second));
        }

        return object;
    }

    case YAML::NodeType::Undefined:
    default:
        throw std::runtime_error("Unknown YAML node type");
    }
}

static void print_json_error(const std::string &path, const JsonError &error)
{
    fprintf(stderr, "%s: Error: ", path.c_str());

    switch (error.type) {
    case JsonErrorType::ParseError:
        fprintf(stderr, "Failed to parse generated JSON\n");
        break;
    case JsonErrorType::MismatchedType:
        fprintf(stderr, "Expected %s, but found %s at %s\n",
                error.expected_type.c_str(), error.actual_type.c_str(),
                error.context.c_str());
        break;
    case JsonErrorType::UnknownKey:
        fprintf(stderr, "Unknown key at %s\n", error.context.c_str());
        break;
    case JsonErrorType::UnknownValue:
        fprintf(stderr, "Unknown value at %s\n", error.context.c_str());
        break;
    default:
        fprintf(stderr, "Unknown error\n");
        break;
    }
}

static void print_validation_error(const std::string &path,
                                   const std::string &id,
                                   ValidateFlags flags)
{
    fprintf(stderr, "%s: [%s] Error during validation (0x%" PRIx64 "):\n",
            path.c_str(), id.empty() ? "unknown" : id.c_str(),
            static_cast<uint64_t>(flags));

    struct {
        ValidateFlag flag;
        const char *msg;
    } mappings[] = {
        { ValidateFlag::MissingId,                     "Missing device ID" },
        { ValidateFlag::MissingCodenames,              "Missing device codenames" },
        { ValidateFlag::MissingName,                   "Missing device name" },
        { ValidateFlag::MissingArchitecture,           "Missing device architecture" },
        { ValidateFlag::MissingSystemBlockDevs,        "Missing system block device paths" },
        { ValidateFlag::MissingCacheBlockDevs,         "Missing cache block device paths" },
        { ValidateFlag::MissingDataBlockDevs,          "Missing data block device paths" },
        { ValidateFlag::MissingBootBlockDevs,          "Missing boot block device paths" },
        { ValidateFlag::MissingRecoveryBlockDevs,      "Missing recovery block device paths" },
        { ValidateFlag::MissingBootUiTheme,            "Missing Boot UI theme" },
        { ValidateFlag::MissingBootUiGraphicsBackends, "Missing Boot UI graphics backends" },
        { ValidateFlag::InvalidArchitecture,           "Invalid device architecture" },
        { ValidateFlag::InvalidFlags,                  "Invalid device flags" },
        { ValidateFlag::InvalidBootUiFlags,            "Invalid Boot UI flags" },
        { static_cast<ValidateFlag>(0),                nullptr },
    };

    for (auto it = mappings; it->msg; ++it) {
        if (flags & it->flag) {
            fprintf(stderr, "- %s\n", it->msg);
            flags &= ~ValidateFlags(it->flag);
        }
    }

    if (flags) {
        fprintf(stderr, "- Unknown remaining flags (0x%" PRIx64 ")",
                static_cast<uint64_t>(flags));
    }
}

static bool validate(const std::string &path, const std::string &json,
                     bool is_array)
{
    JsonError error;

    if (is_array) {
        std::vector<Device> devices;

        if (!device_list_from_json(json, devices, error)) {
            print_json_error(path, error);
            return false;
        }

        bool failed = false;

        for (auto const &device : devices) {
            auto flags = device.validate();
            if (flags) {
                print_validation_error(path, device.id(), flags);
                failed = true;
            }
        }

        if (failed) {
            return false;
        }
    } else {
        Device device;

        if (!device_from_json(json, device, error)) {
            print_json_error(path, error);
            return false;
        }

        auto flags = device.validate();
        if (flags) {
            print_validation_error(path, device.id(), flags);
            return false;
        }
    }

    return true;
}

static void usage(FILE *stream)
{
    fprintf(stream,
            "Usage: devicesgen [OPTION]...\n"
            "\n"
            "Options:\n"
            "  -o, --output <file>\n"
            "                   Output file (outputs to stdout if omitted)\n"
            "  -h, --help       Display this help message\n"
            "  --styled         Output in human-readable format\n");
}

int main(int argc, char *argv[])
{
    int opt;

    enum Options {
        OPT_STYLED             = 1000,
    };

    static const char short_options[] = "o:h";

    static struct option long_options[] = {
        {"styled", no_argument, 0, OPT_STYLED},
        {"output", required_argument, 0, 'o'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    const char *output_file = nullptr;
    bool styled = false;

    while ((opt = getopt_long(argc, argv, short_options,
                              long_options, &long_index)) != -1) {
        switch (opt) {
        case OPT_STYLED:
            styled = true;
            break;

        case 'o':
            output_file = optarg;
            break;

        case 'h':
            usage(stdout);
            return EXIT_SUCCESS;

        default:
            usage(stderr);
            return EXIT_FAILURE;
        }
    }

    json_set_alloc_funcs(&xmalloc, &free);

    ScopedJsonT json_root(json_array(), &json_decref);

    for (int i = optind; i < argc; ++i) {
        try {
            YAML::Node root = YAML::LoadFile(argv[i]);
            ScopedJsonT node(yaml_node_to_json_node(root), json_decref);
            ScopedCharArray output(json_dumps(node.get(), JSON_COMPACT), &free);

            bool valid = validate(argv[i], output.get(), json_is_array(node));
            if (!valid) {
                return EXIT_FAILURE;
            }

            if (json_is_array(node)) {
                size_t index;
                json_t *elem;

                json_array_foreach(node.get(), index, elem) {
                    json_array_append(json_root.get(), elem);
                }
            } else {
                json_array_append_new(json_root.get(), node.release());
            }
        } catch (const std::exception &e) {
            fprintf(stderr, "%s: Failed to convert file: %s\n",
                    argv[i], e.what());
            return EXIT_FAILURE;
        }
    }

    FILE *fp = stdout;

    if (output_file) {
        fp = fopen(output_file, "w");
        if (!fp) {
            fprintf(stderr, "%s: Failed to open file: %s\n",
                    output_file, strerror(errno));
            return EXIT_FAILURE;
        }
    }

    ScopedCharArray output(nullptr, &free);
    if (styled) {
        output.reset(json_dumps(json_root.get(),
                                JSON_INDENT(4) | JSON_SORT_KEYS));
    } else {
        output.reset(json_dumps(json_root.get(), JSON_COMPACT));
    }

    if (fputs(output.get(), fp) == EOF) {
        fprintf(stderr, "Failed to write JSON: %s\n", strerror(errno));
    }

    if (output_file) {
        if (fclose(fp) != 0) {
            fprintf(stderr, "%s: Failed to close file: %s\n",
                    output_file, strerror(errno));
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
