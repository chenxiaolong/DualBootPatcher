/*
 * Copyright (C) 2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <memory>
#include <stack>
#include <vector>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <getopt.h>

#include <rapidjson/error/en.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/reader.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "mbcommon/common.h"
#include "mbcommon/string.h"

using namespace rapidjson;

template <typename OutputHandler>
class UnneededKeyFilter
{
public:
    UnneededKeyFilter(OutputHandler &out)
        : _out(out), _filter_depth(), _filter_key_count()
    {
    }

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(UnneededKeyFilter)
    MB_DISABLE_MOVE_CONSTRUCT_AND_ASSIGN(UnneededKeyFilter)

    bool Null()
    {
        return _filter_depth > 0 ? EndValue() : _out.Null();
    }

    bool Bool(bool b)
    {
        return _filter_depth > 0 ? EndValue() : _out.Bool(b);
    }

    bool Int(int i)
    {
        return _filter_depth > 0 ? EndValue() : _out.Int(i);
    }

    bool Uint(unsigned u)
    {
        return _filter_depth > 0 ? EndValue() : _out.Uint(u);
    }

    bool Int64(int64_t i)
    {
        return _filter_depth > 0 ? EndValue() : _out.Int64(i);
    }

    bool Uint64(uint64_t u)
    {
        return _filter_depth > 0 ? EndValue() : _out.Uint64(u);
    }

    bool Double(double d)
    {
        return _filter_depth > 0 ? EndValue() : _out.Double(d);
    }

    bool RawNumber(const char *str, SizeType length, bool copy)
    {
        return _filter_depth > 0 ? EndValue()
                : _out.RawNumber(str, length, copy);
    }

    bool String(const char *str, SizeType length, bool copy)
    {
        return _filter_depth > 0 ? EndValue() : _out.String(str, length, copy);
    }

    bool StartObject()
    {
        if (_filter_depth > 0) {
            ++_filter_depth;
            return true;
        } else {
            _filter_key_count.push(0);
            return _out.StartObject();
        }
    }

    bool Key(const char *str, SizeType length, bool copy)
    {
        if (_filter_depth > 0) {
            return true;
        } else if ((length == 5 && memcmp(str, "title", 5) == 0)
                || (length == 11 && memcmp(str, "description", 11) == 0)) {
            _filter_depth = 1;
            return true;
        } else {
            ++_filter_key_count.top();
            return _out.Key(str, length, copy);
        }
    }

    bool EndObject(SizeType member_count)
    {
        (void) member_count;

        if (_filter_depth > 0) {
            --_filter_depth;
            return EndValue();
        } else {
            member_count = _filter_key_count.top();
            _filter_key_count.pop();
            return _out.EndObject(member_count);
        }
    }

    bool StartArray()
    {
        if (_filter_depth > 0) {
            _filter_depth++;
            return true;
        } else {
            return _out.StartArray();
        }
    }

    bool EndArray(SizeType element_count)
    {
        if (_filter_depth > 0) {
            --_filter_depth;
            return EndValue();
        } else {
            return _out.EndArray(element_count);
        }
    }

private:
    bool EndValue()
    {
        if (_filter_depth == 1) {
            _filter_depth = 0;
        }
        return true;
    }

    OutputHandler &_out;
    unsigned _filter_depth;
    std::stack<SizeType> _filter_key_count;
};

static std::string base_name(const std::string &path)
{
    auto slash = path.find_last_of("/\\");
    if (slash != std::string::npos) {
        return path.substr(slash + 1);
    } else {
        return path;
    }
}

static void escape_print(FILE *fp, const std::string &str)
{
    static const char digits[] = "0123456789abcdef";

    for (char c : str) {
        if (c == '\\') {
            fputs("\\\\", fp);
        } else if (c == '"') {
            fputs("\\\"", fp);
        } else if (isprint(c)) {
            fputc(c, fp);
        } else if (c == '\a') {
            fputs("\\a", fp);
        } else if (c == '\b') {
            fputs("\\b", fp);
        } else if (c == '\f') {
            fputs("\\f", fp);
        } else if (c == '\n') {
            fputs("\\n", fp);
        } else if (c == '\r') {
            fputs("\\r", fp);
        } else if (c == '\t') {
            fputs("\\t", fp);
        } else if (c == '\v') {
            fputs("\\v", fp);
        } else {
            unsigned char uc = static_cast<unsigned char>(c);
            fputs("\\x", fp);
            fputc(digits[(uc >> 4) & 0xf], fp);
            fputc(digits[uc & 0xf], fp);
        }
    }
}

static void usage(FILE *stream)
{
    fprintf(stream, R"raw(\
Usage: schemas2cpp [OPTION]... [FILE]...

Options:
  -o, --output <path>  Output cpp file path
  -h, --help           Display this help message

The -o option specifies the output path for the cpp file. A corresponding header
file will also be written. If the cpp output path ends in ".cpp", the header
will be written to the same path except ".cpp" is replaced with ".h". Otherwise,
".h" is simply appended to the path.
)raw");
}

int main(int argc, char *argv[])
{
    int opt;

    static const char short_options[] = "o:h";

    static struct option long_options[] = {
        {"output", required_argument, 0, 'o'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    const char *output_path = nullptr;

    while ((opt = getopt_long(argc, argv, short_options,
                              long_options, &long_index)) != -1) {
        switch (opt) {
        case 'o':
            output_path = optarg;
            break;

        case 'h':
            usage(stdout);
            return EXIT_SUCCESS;

        default:
            usage(stderr);
            return EXIT_FAILURE;
        }
    }

    if (!output_path) {
        fprintf(stderr, "No output path specified.\n");
        return EXIT_FAILURE;
    }

    using ScopedFILE = std::unique_ptr<FILE, decltype(fclose) *>;

    char read_buf[65536];
    StringBuffer sb;

    std::vector<std::pair<std::string, std::string>> results;

    for (int i = optind; i < argc; ++i) {
        ScopedFILE fp(fopen(argv[i], "re"), &fclose);
        if (!fp) {
            fprintf(stderr, "%s: Failed to open for reading: %s\n",
                    argv[i], strerror(errno));
            return EXIT_FAILURE;
        }

        sb.Clear();

        Reader reader;
        FileReadStream is(fp.get(), read_buf, sizeof(read_buf));

        Writer<StringBuffer> writer(sb);
        UnneededKeyFilter<Writer<StringBuffer>> filter(writer);

        if (!reader.Parse(is, filter)) {
            fprintf(stderr, "Error at offset %zu: %s\n",
                    reader.GetErrorOffset(),
                    GetParseError_En(reader.GetParseErrorCode()));
            return EXIT_FAILURE;
        }

        results.emplace_back(base_name(argv[i]),
                             std::string{sb.GetString(), sb.GetSize()});
    }

    // Write cpp file
    {
        ScopedFILE fp(fopen(output_path, "we"), &fclose);
        if (!fp) {
            fprintf(stderr, "%s: Failed to open for writing: %s\n",
                    output_path, strerror(errno));
            return EXIT_FAILURE;
        }

        fprintf(fp.get(),
                "#include <array>\n"
                "#include <utility>\n"
                // Make -Wshadow happy
                "extern std::array<std::pair<const char *, const char *>, %zu> g_schemas;\n"
                "std::array<std::pair<const char *, const char *>, %zu> g_schemas{{\n",
                results.size(), results.size());

        for (auto const &r : results) {
            fprintf(fp.get(), "    { \"");
            escape_print(fp.get(), r.first);
            fprintf(fp.get(), "\", \"");
            escape_print(fp.get(), r.second);
            fprintf(fp.get(), "\" },\n");
        }

        fprintf(fp.get(), "}};\n");
    }

    // Write header file
    {
        std::string hpp_path{output_path};
        if (mb::ends_with(hpp_path, ".cpp")) {
            hpp_path.resize(hpp_path.size() - 4);
        }
        hpp_path += ".h";

        ScopedFILE fp(fopen(hpp_path.c_str(), "we"), &fclose);
        if (!fp) {
            fprintf(stderr, "%s: Failed to open for writing: %s\n",
                    hpp_path.c_str(), strerror(errno));
            return EXIT_FAILURE;
        }

        fprintf(fp.get(),
                "#pragma once\n"
                "#include <array>\n"
                "#include <utility>\n"
                "extern std::array<std::pair<const char *, const char *>, %zu> g_schemas;\n",
                results.size());
    }

    return EXIT_SUCCESS;
}
