/*
 * Copyright (C) 2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "process.h"

#include <memory>
#include <regex>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <getopt.h>

#include "mbcommon/locale.h"
#include "mbcommon/string.h"

#ifdef _WIN32
#  define MAIN_FUNC wmain
#  define STR(x)    L##x
#else
#  define MAIN_FUNC main
#  define STR(x)    x
#endif


using namespace mb;

using ScopedFILE = std::unique_ptr<FILE, decltype(fclose) *>;

static bool preprocess(const std::vector<NativeString> &compiler_args,
                       std::string_view in, std::string &out)
{
    auto args = compiler_args;
    args.insert(args.end(), {
        STR("-x"), STR("c++"), STR("-"), STR("-E"), STR("-P"),
    });

    if (auto r = run_process(args, in, out); !r) {
        fprintf(stderr, "Failed to run compiler: %s\n",
                r.error().message().c_str());
        return false;
    } else if (r.value() != 0) {
        fprintf(stderr, "Compiler returned exit status %d\n", r.value());
        return false;
    }

    return true;
}

static bool generate_signals(const std::vector<NativeString> &compiler_args,
                             std::string &out)
{
    auto args = compiler_args;
    args.push_back(STR("-dM"));

    std::string temp;

    if (!preprocess(args, "#include <signal.h>\n", temp)) {
        return false;
    }

    std::regex re("^#[ \t]*define[ \t]+(SIG[A-Z0-9]+)[ \t]+([0-9]+)");
    std::smatch match;

    out.clear();

    for (auto const &line : split(temp, '\n')) {
        if (std::regex_match(line, match, re)) {
            out += "{\"";
            out += match[1];
            out += "\", ";
            out += match[2];
            out += "},\n";
        }
    }

    out += "{nullptr, 0},\n";

    return true;
}

static bool generate_syscalls(const std::vector<NativeString> &compiler_args,
                              std::string &out)
{
    auto args = compiler_args;
    args.push_back(STR("-dM"));

    constexpr char preprocess_input[] =
R"(#include <sys/syscall.h>

// Removed in Linux commit db695c0509d6ec9046ee5e4c520a19fa17d9fce2, but many
// Android devices run older kernels
//#if defined(__arm__) && !defined(__aarch64__) && !defined(__ARM_NR_cmpxchg)
#if defined(__ARM_NR_BASE) && !defined(__ARM_NR_cmpxchg)
#  define __ARM_NR_cmpxchg (__ARM_NR_BASE + 0x00fff0)
#endif
)";

    std::string temp;

    // First round to build list of syscalls
    if (!preprocess(args, preprocess_input, temp)) {
        return false;
    }

    std::regex re("^#[ \t]*define[ \t]+(SYS_|__ARM_NR_)([^ \t]+)[ \t]+(.+)");
    std::smatch match;
    std::string input =
R"(#include <sys/syscall.h>

struct { const char *name; long number; } syscalls[] = {
)";

    for (auto const &line : split(temp, '\n')) {
        if (std::regex_match(line, match, re) && match[2] != "BASE") {
            input += "{\"";
            input += match[2];
            input += "\", ";
            input += match[3];
            input += "},\n";
        }
    }

    input +=
R"({nullptr, 0},
};
)";

    // Second round to evaluate macros
    if (!preprocess(compiler_args, input, temp)) {
        return false;
    }

    out.clear();

    for (auto const &line : split_sv(temp, '\n')) {
        if (starts_with(line, "{")) {
            out += line;
            out += '\n';
        }
    }

    return true;
}

static void usage(FILE *stream, const char *prog_name)
{
    fprintf(stream,
            "Usage: %s -t TYPE [OPTION...] -- <compiler args>\n"
            "\n"
            "Options:\n"
            "  -o, --output FILE  Output file (stdout by default)\n"
            "  -t, --type TYPE    Type of file to generate (signals, syscalls)\n"
            "  -h, --help         Display this help message\n",
            prog_name);
}

int MAIN_FUNC(int argc, NativeChar *argv[], NativeChar *envp[])
{
    (void) envp;

    // Convert args to UTF-8 for getopt
#ifdef _WIN32
    std::vector<std::string> utf8_args;
    for (int i = 0; i < argc; ++i) {
        utf8_args.push_back(wcs_to_utf8(argv[i]).value());
    }

    std::vector<char *> utf8_argv;
    for (auto &arg : utf8_args) {
        utf8_argv.push_back(arg.data());
    }
    utf8_argv.push_back(nullptr);

    auto new_argv = utf8_argv.data();
#else
    auto new_argv = argv;
#endif

    int opt;

    static const char *short_options = "o:t:h";
    static struct option long_options[] = {
        {"output", required_argument, nullptr, 'o'},
        {"type",   required_argument, nullptr, 't'},
        {"help",   no_argument,       nullptr, 'h'},
        {nullptr,  0,                 nullptr, 0},
    };

    const char *output_file = nullptr;
    const char *type = nullptr;

    while ((opt = getopt_long(argc, new_argv, short_options,
                              long_options, nullptr)) != -1) {
        switch (opt) {
        case 'o':
            output_file = optarg;
            break;
        case 't':
            type = optarg;
            break;
        case 'h':
            usage(stdout, new_argv[0]);
            return EXIT_SUCCESS;
        default:
            usage(stderr, new_argv[0]);
            return EXIT_FAILURE;
        }
    }

    // Remaining args are compiler arguments
    if (argc - optind == 0 || !type) {
        usage(stderr, new_argv[0]);
        return EXIT_FAILURE;
    }

    std::string out;
    std::vector<NativeString> args;

    for (int i = optind; i < argc; ++i) {
#ifdef _WIN32
        args.push_back(utf8_to_wcs(new_argv[i]).value());
#else
        args.push_back(new_argv[i]);
#endif
    }

    if (strcmp(type, "signals") == 0) {
        if (!generate_signals(args, out)) {
            return EXIT_FAILURE;
        }
    } else if (strcmp(type, "syscalls") == 0) {
        if (!generate_syscalls(args, out)) {
            return EXIT_FAILURE;
        }
    } else {
        fprintf(stderr, "Invalid type: %s\n", type);
        return EXIT_FAILURE;
    }

    if (output_file) {
        ScopedFILE fp(fopen(output_file, "wb"), fclose);
        if (!fp) {
            fprintf(stderr, "%s: Failed to open for writing: %s\n",
                    output_file, strerror(errno));
            return EXIT_FAILURE;
        }

        if (fputs(out.c_str(), fp.get()) == EOF) {
            fprintf(stderr, "%s: Failed to write data: %s\n",
                    output_file, strerror(errno));
            return EXIT_FAILURE;
        }

        if (fclose(fp.release()) < 0) {
            fprintf(stderr, "%s: Error while closing file: %s\n",
                    output_file, strerror(errno));
            return EXIT_FAILURE;
        }
    } else {
        if (fputs(out.c_str(), stdout) == EOF) {
            fprintf(stderr, "Failed to write data: %s\n",
                    strerror(errno));
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
