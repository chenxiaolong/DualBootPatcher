/*
 * loki_patch
 *
 * A utility to patch unsigned boot and recovery images to make
 * them suitable for booting on the AT&T/Verizon Samsung
 * Galaxy S4, Galaxy Stellar, and various locked LG devices
 *
 * by Dan Rosenberg (@djrbliss)
 *
 */

#include <stdio.h>
#include <string.h>
#include "loki.h"

static int print_help(const char* cmd) {
    printf("Usage\n");
    printf("> Patch partition file image:\n");
    printf("%s [patch] [boot|recovery] [aboot.img] [in.img] [out.lok]\n", cmd);
    printf("\n");
    printf("> Flash loki image to boot|recovery:\n");
    printf("%s [flash] [boot|recovery] [in.lok]\n", cmd);
    printf("\n");
    printf("> Find offset from aboot image:\n");
    printf("%s [find] [aboot.img]\n", cmd);
    printf("\n");
    printf("> Revert Loki patching:\n");
    printf("%s [unlok] [in.lok] [out.img]\n", cmd);
    printf("\n");
    return 1;
}

int main(int argc, char **argv) {
    printf("Loki tool v%s\n", VERSION);

    if (argc == 6 && strcmp(argv[1], "patch") == 0) {
        // argv[2]: partition_label
        // argv[3]: aboot_image
        // argv[4]: in_image
        // argv[5]: out_image
        return loki_patch(argv[2], argv[3], argv[4], argv[5]);
    } else if (argc == 4 && strcmp(argv[1], "flash") == 0) {
        // argv[2]: partition_label
        // argv[3]: loki_image
        return loki_flash(argv[2], argv[3]);
    } else if (argc == 3 && strcmp(argv[1], "find") == 0) {
        // argv[2]: aboot_image
        return loki_find(argv[2]);
    } else if (argc == 4 && strcmp(argv[1], "unlok") == 0) {
        // argv[2]: in_image
        // argv[3]: out_image
        return loki_unlok(argv[2], argv[3]);
    }

    return print_help(argv[0]);
}
