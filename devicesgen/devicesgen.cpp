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

#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/writer.h>
#include <yaml-cpp/yaml.h>

#include "mbdevice/json.h"
#include "mbdevice/schema.h"


using namespace rapidjson;

using namespace mb::device;

template<typename Allocator>
static Value yaml_node_to_json(const YAML::Node &yaml_node, Allocator &alloc)
{
    switch (yaml_node.Type()) {
    case YAML::NodeType::Null:
        return Value(kNullType);

    case YAML::NodeType::Scalar: {
        Value value;
        if (yaml_node.Tag() != "!") {
            try {
                value.SetInt64(yaml_node.as<int64_t>());
                return value;
            } catch (const YAML::BadConversion &e) {}
            try {
                value.SetDouble(yaml_node.as<double>());
                return value;
            } catch (const YAML::BadConversion &e) {}
            try {
                value.SetBool(yaml_node.as<bool>());
                return value;
            } catch (const YAML::BadConversion &e) {}
        }
        try {
            value.SetString(yaml_node.as<std::string>(), alloc);
            return value;
        } catch (const YAML::BadConversion &e) {}
        throw std::runtime_error("Cannot convert scalar value to known type");
    }

    case YAML::NodeType::Sequence: {
        Value array(kArrayType);

        for (auto const &item : yaml_node) {
            array.PushBack(yaml_node_to_json(item, alloc), alloc);
        }

        return array;
    }

    case YAML::NodeType::Map: {
        Value object(kObjectType);

        for (auto const &item : yaml_node) {
            Value key;
            key.SetString(item.first.as<std::string>(), alloc);
            object.AddMember(key, yaml_node_to_json(item.second, alloc), alloc);
        }

        return object;
    }

    case YAML::NodeType::Undefined:
    default:
        throw std::runtime_error("Unknown YAML node type");
    }
}

template<typename Allocator>
static bool convert_files(int argc, char *argv[], Value &value,
                          Allocator &alloc)
{
    value.SetArray();

    for (int i = 0; i < argc; ++i) {
        try {
            YAML::Node root = YAML::LoadFile(argv[i]);

            if (root.Type() != YAML::NodeType::Sequence) {
                fprintf(stderr, "%s: Root is not an array\n", argv[i]);
                return false;
            }

            for (auto const &item : root) {
                value.PushBack(yaml_node_to_json(item, alloc), alloc);
            }
        } catch (const std::exception &e) {
            fprintf(stderr, "%s: Failed to convert file: %s\n",
                    argv[i], e.what());
            return false;
        }
    }

    return true;
}

template<typename Writer>
static bool validate_and_write(Document &d, const SchemaDocument &sd,
                               Writer &writer)
{
    GenericSchemaValidator<SchemaDocument, Writer> sv(sd, writer);
    if (!d.Accept(sv)) {
        if (!sv.IsValid()) {
            char write_buf[65536];
            FileWriteStream os(stderr, write_buf, sizeof(write_buf));
            PrettyWriter<FileWriteStream> error_writer(os);

            // Copy error data to add context
            Value error(sv.GetError(), d.GetAllocator());

            for (auto &item : error.GetObject()) {
                auto const &instance_ref = item.value["instanceRef"];
                Pointer instance_ptr(instance_ref.GetString(),
                                     instance_ref.GetStringLength());

                if (auto const *value = instance_ptr.Get(d)) {
                    item.value.AddMember("instanceData",
                                         Value(*value, d.GetAllocator()),
                                         d.GetAllocator());
                }
            }

            fprintf(stderr, "Schema validation failed:\n");
            error.Accept(error_writer);
            os.Put('\n');
            os.Flush();
        } else {
            fprintf(stderr, "Failed to write JSON\n");
        }

        return false;
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

    FILE *fp = stdout;

    if (output_file) {
        fp = fopen(output_file, "we");
        if (!fp) {
            fprintf(stderr, "%s: Failed to open file: %s\n",
                    output_file, strerror(errno));
            return EXIT_FAILURE;
        }
    }

    char write_buf[65536];
    FileWriteStream os(fp, write_buf, sizeof(write_buf));
    bool ret;

    DeviceSchemaProvider<> sp;
    const SchemaDocument *sd = sp.GetSchema("device_list.json");
    if (!sd) {
        assert(false);
        return EXIT_FAILURE;
    }

    Document d;
    auto &alloc = d.GetAllocator();

    if (!convert_files(argc - optind, argv + optind, d, alloc)) {
        return EXIT_FAILURE;
    }

    if (styled) {
        PrettyWriter<FileWriteStream> writer(os);
        ret = validate_and_write(d, *sd, writer);
    } else {
        Writer<FileWriteStream> writer(os);
        ret = validate_and_write(d, *sd, writer);
    }

    if (output_file) {
        if (fclose(fp) != 0) {
            fprintf(stderr, "%s: Failed to close file: %s\n",
                    output_file, strerror(errno));
            return EXIT_FAILURE;
        }
    }

    return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
