/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
#include "mbdevice/validate.h"


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

static void print_json_error(const char *path, MbDeviceJsonError *error)
{
    fprintf(stderr, "%s: Error: ", path);

    switch (error->type) {
    case MB_DEVICE_JSON_STANDARD_ERROR:
        fprintf(stderr, "Internal error\n");
        break;
    case MB_DEVICE_JSON_PARSE_ERROR:
        fprintf(stderr, "Failed to parse generated JSON\n");
        break;
    case MB_DEVICE_JSON_MISMATCHED_TYPE:
        fprintf(stderr, "Expected %s, but found %s at %s\n",
                error->expected_type, error->actual_type, error->context);
        break;
    case MB_DEVICE_JSON_UNKNOWN_KEY:
        fprintf(stderr, "Unknown key at %s\n", error->context);
        break;
    case MB_DEVICE_JSON_UNKNOWN_VALUE:
        fprintf(stderr, "Unknown value at %s\n", error->context);
        break;
    default:
        fprintf(stderr, "Unknown error\n");
        break;
    }
}

static void print_validation_error(const char *path, const char *id,
                                   uint64_t flags)
{
    fprintf(stderr, "%s: [%s] Error during validation (0x%" PRIx64 "):\n",
            path, id ? id : "unknown", flags);

    struct mapping {
        uint64_t flag;
        const char *msg;
    } mappings[] = {
        { MB_DEVICE_MISSING_ID,                        "Missing device ID" },
        { MB_DEVICE_MISSING_CODENAMES,                 "Missing device codenames" },
        { MB_DEVICE_MISSING_NAME,                      "Missing device name" },
        { MB_DEVICE_MISSING_ARCHITECTURE,              "Missing device architecture" },
        { MB_DEVICE_MISSING_SYSTEM_BLOCK_DEVS,         "Missing system block device paths" },
        { MB_DEVICE_MISSING_CACHE_BLOCK_DEVS,          "Missing cache block device paths" },
        { MB_DEVICE_MISSING_DATA_BLOCK_DEVS,           "Missing data block device paths" },
        { MB_DEVICE_MISSING_BOOT_BLOCK_DEVS,           "Missing boot block device paths" },
        { MB_DEVICE_MISSING_RECOVERY_BLOCK_DEVS,       "Missing recovery block device paths" },
        { MB_DEVICE_MISSING_BOOT_UI_THEME,             "Missing Boot UI theme" },
        { MB_DEVICE_MISSING_BOOT_UI_GRAPHICS_BACKENDS, "Missing Boot UI graphics backends" },
        { 0, nullptr }
    };

    for (auto it = mappings; it->flag; ++it) {
        if (flags & it->flag) {
            fprintf(stderr, "- %s\n", it->msg);
            flags &= ~it->flag;
        }
    }

    if (flags) {
        fprintf(stderr, "- Unknown remaining flags (0x%" PRIx64 ")", flags);
    }
}

static bool validate(const char *path, const char *json, bool is_array)
{
    MbDeviceJsonError error;

    if (is_array) {
        Device **devices = mb_device_new_list_from_json(json, &error);
        if (!devices) {
            print_json_error(path, &error);
            return false;
        }

        bool failed = false;

        for (Device **iter = devices; *iter; ++iter) {
            uint64_t flags = mb_device_validate(*iter);
            if (flags) {
                print_validation_error(path, mb_device_id(*iter), flags);
                failed = true;
            }
            mb_device_free(*iter);
        }
        free(devices);

        if (failed) {
            return false;
        }
    } else {
        Device *device = mb_device_new_from_json(json, &error);
        if (!device) {
            print_json_error(path, &error);
            return false;
        }

        bool failed = false;
        uint64_t flags = mb_device_validate(device);

        if (flags) {
            print_validation_error(path, mb_device_id(device), flags);
            failed = true;
        }

        mb_device_free(device);

        if (failed) {
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

    json_t *json_root = json_array();

    for (int i = optind; i < argc; ++i) {
        try {
            YAML::Node root = YAML::LoadFile(argv[i]);
            json_t *node = yaml_node_to_json_node(root);

            char *output = json_dumps(node, JSON_COMPACT);
            bool valid = validate(argv[i], output, json_is_array(node));
            free(output);
            if (!valid) {
                return EXIT_FAILURE;
            }

            if (json_is_array(node)) {
                size_t index;
                json_t *elem;

                json_array_foreach(node, index, elem) {
                    json_array_append(json_root, elem);
                }

                json_decref(node);
            } else {
                json_array_append_new(json_root, node);
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

    char *output;
    if (styled) {
        output = json_dumps(json_root, JSON_INDENT(4) | JSON_SORT_KEYS);
    } else {
        output = json_dumps(json_root, JSON_COMPACT);
    }

    if (fputs(output, fp) == EOF) {
        fprintf(stderr, "Failed to write JSON: %s\n", strerror(errno));
    }

    free(output);
    json_decref(json_root);

    if (output_file) {
        if (fclose(fp) != 0) {
            fprintf(stderr, "%s: Failed to close file: %s\n",
                    output_file, strerror(errno));
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
