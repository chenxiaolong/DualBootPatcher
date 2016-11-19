#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compile.h"
#include "decompile.h"

static void usage(FILE *stream, const char *progname)
{
    // argv[0] can be NULL
    progname = progname ? progname : "(none)";

    fprintf(stream,
            "Usage: %s compile [option...] <source file> <target file>\n"
            "       %s decompile [option...] <source file> <target file>\n"
            "\n"
            "Options:\n"
            "  -p, --pcre <PCRE lib path>\n"
            "                   Path to PCRE shared library\n"
            "  -h, --help       Display this help message\n",
            progname, progname);
}

int main(int argc, char *argv[])
{
    int opt;
    int long_index = 0;
    const char *action = NULL;
    const char *source_path = NULL;
    const char *target_path = NULL;
    const char *pcre_path = NULL;
    struct pcre_shim shim;
    int ret;

    static const char *short_options = "p:h";
    static struct option long_options[] = {
        {"pcre", required_argument, 0, 'p'},
        {"help", no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, short_options,
            long_options, &long_index)) != -1) {
        switch (opt) {
        case 'p':
            pcre_path = optarg;
            break;
        case 'h':
            usage(stdout, argv[0]);
            return EXIT_SUCCESS;
        default:
            usage(stderr, argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (argc - optind != 3) {
        usage(stderr, argv[0]);
        return EXIT_FAILURE;
    }

    action = argv[optind];
    source_path = argv[optind + 1];
    target_path = argv[optind + 2];

    // If pcre library path is not provided, try environment variable
    if (!pcre_path) {
        pcre_path = getenv("PCRE_LIBRARY");
        if (!pcre_path) {
            fprintf(stderr, "-p/--pcre must be provided or PCRE_LIBRARY set\n");
            return EXIT_FAILURE;
        }
    }

    if (pcre_shim_load(&shim, pcre_path) < 0) {
        return EXIT_FAILURE;
    }

    if (strcmp(action, "compile") == 0) {
        ret = compile(&shim, source_path, target_path);
    } else if (strcmp(action, "decompile") == 0) {
        ret = decompile(&shim, source_path, target_path);
    } else {
        usage(stderr, argv[0]);
        ret = -1;
    }

    pcre_shim_unload(&shim);

    return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
