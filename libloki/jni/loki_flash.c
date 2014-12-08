/*
 * loki_flash
 *
 * A sample utility to validate and flash .lok files
 *
 * by Dan Rosenberg (@djrbliss)
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "loki.h"

int loki_flash(const char* partition_label, const char* loki_image)
{
	int ifd, aboot_fd, ofd, recovery, offs, match;
	void *orig, *aboot, *patch;
	struct stat st;
	struct boot_img_hdr *hdr;
	struct loki_hdr *loki_hdr;
	char outfile[1024];

	if (!strcmp(partition_label, "boot")) {
		recovery = 0;
	} else if (!strcmp(partition_label, "recovery")) {
		recovery = 1;
	} else {
		LOGE("[+] First argument must be \"boot\" or \"recovery\".");
		return 1;
	}

	/* Verify input file */
	aboot_fd = open(ABOOT_PARTITION, O_RDONLY);
	if (aboot_fd < 0) {
		LOGE("[-] Failed to open aboot for reading.");
		return 1;
	}

	ifd = open(loki_image, O_RDONLY);
	if (ifd < 0) {
		LOGE("[-] Failed to open %s for reading.", loki_image);
		return 1;
	}

	/* Map the image to be flashed */
	if (fstat(ifd, &st)) {
		LOGE("[-] fstat() failed.");
		return 1;
	}

	orig = mmap(0, (st.st_size + 0x2000 + 0xfff) & ~0xfff, PROT_READ, MAP_PRIVATE, ifd, 0);
	if (orig == MAP_FAILED) {
		LOGE("[-] Failed to mmap Loki image.");
		return 1;
	}

	hdr = orig;
	loki_hdr = orig + 0x400;

	/* Verify this is a Loki image */
	if (memcmp(loki_hdr->magic, "LOKI", 4)) {
		LOGE("[-] Input file is not a Loki image.");
		return 1;
	}

	/* Verify this is the right type of image */
	if (loki_hdr->recovery != recovery) {
		LOGE("[-] Loki image is not a %s image.", recovery ? "recovery" : "boot");
		return 1;
	}

	/* Verify the to-be-patched address matches the known code pattern */
	aboot = mmap(0, 0x40000, PROT_READ, MAP_PRIVATE, aboot_fd, 0);
	if (aboot == MAP_FAILED) {
		LOGE("[-] Failed to mmap aboot.");
		return 1;
	}

	match = 0;

	for (offs = 0; offs < 0x10; offs += 0x4) {

		patch = NULL;

		if (hdr->ramdisk_addr > ABOOT_BASE_LG)
			patch = hdr->ramdisk_addr - ABOOT_BASE_LG + aboot + offs;
		else if (hdr->ramdisk_addr > ABOOT_BASE_SAMSUNG)
			patch = hdr->ramdisk_addr - ABOOT_BASE_SAMSUNG + aboot + offs;
		else if (hdr->ramdisk_addr > ABOOT_BASE_VIPER)
			patch = hdr->ramdisk_addr - ABOOT_BASE_VIPER + aboot + offs;
		else if (hdr->ramdisk_addr > ABOOT_BASE_G2)
			patch = hdr->ramdisk_addr - ABOOT_BASE_G2 + aboot + offs;

		if (patch < aboot || patch > aboot + 0x40000 - 8) {
			LOGE("[-] Invalid .lok file.");
			return 1;
		}

		if (!memcmp(patch, PATTERN1, 8) ||
			!memcmp(patch, PATTERN2, 8) ||
			!memcmp(patch, PATTERN3, 8) ||
			!memcmp(patch, PATTERN4, 8) ||
			!memcmp(patch, PATTERN5, 8) ||
			!memcmp(patch, PATTERN6, 8)) {

			match = 1;
			break;
		}
	}

	if (!match) {
		LOGE("[-] Loki aboot version does not match device.");
		return 1;
	}

	LOGV("[+] Loki validation passed, flashing image.");

	snprintf(outfile, sizeof(outfile),
			 "%s",
			 recovery ? RECOVERY_PARTITION : BOOT_PARTITION);

	ofd = open(outfile, O_WRONLY);
	if (ofd < 0) {
		LOGE("[-] Failed to open output block device.");
		return 1;
	}

	if (write(ofd, orig, st.st_size) != st.st_size) {
		LOGE("[-] Failed to write to block device.");
		return 1;
	}

	LOGV("[+] Loki flashing complete!");

	close(ifd);
	close(aboot_fd);
	close(ofd);

	return 0;
}
