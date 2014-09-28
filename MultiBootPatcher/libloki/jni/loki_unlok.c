/*
 * loki_unlok
 *
 * A utility to revert the changes made by loki_patch.
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

static unsigned char patch[] = PATCH;

/* Find the original address of the ramdisk, which
 * was embedded in the shellcode. */
int find_ramdisk_addr(void *img, int sz)
{

	int i, ramdisk = 0;

	for (i = 0; i < sz - (sizeof(patch) - 9); i++) {
		if (!memcmp((char *)img + i, patch, sizeof(patch)-9)) {
			ramdisk = *(int *)(img + i + sizeof(patch) - 5);
			break;
		}
	}

	return ramdisk;
}

int loki_unlok(const char* in_image, const char* out_image)
{
	int ifd, ofd;
	unsigned int orig_ramdisk_size, orig_kernel_size, orig_ramdisk_addr;
	unsigned int page_kernel_size, page_ramdisk_size, page_size, page_mask, fake_size;
	void *orig;
	struct stat st;
	struct boot_img_hdr *hdr;
	struct loki_hdr *loki_hdr;

	ifd = open(in_image, O_RDONLY);
	if (ifd < 0) {
		LOGE("[-] Failed to open %s for reading.", in_image);
		return 1;
	}

	ofd = open(out_image, O_WRONLY|O_CREAT|O_TRUNC, 0644);
	if (ofd < 0) {
		LOGE("[-] Failed to open %s for writing.", out_image);
		return 1;
	}

	/* Map the original boot/recovery image */
	if (fstat(ifd, &st)) {
		LOGE("[-] fstat() failed.");
		return 1;
	}

	orig = mmap(0, (st.st_size + 0x2000 + 0xfff) & ~0xfff, PROT_READ|PROT_WRITE, MAP_PRIVATE, ifd, 0);
	if (orig == MAP_FAILED) {
		LOGE("[-] Failed to mmap input file.");
		return 1;
	}

	hdr = orig;
	loki_hdr = orig + 0x400;

	if (memcmp(loki_hdr->magic, "LOKI", 4)) {
		LOGE("[-] Input file is not a Loki image.");

		/* Copy the entire file to the output transparently */
		if (write(ofd, orig, st.st_size) != st.st_size) {
			LOGE("[-] Failed to copy Loki image.");
			return 1;
		}

		LOGV("[+] Copied Loki image to %s.", out_image);

		return 0;
	}

	page_size = hdr->page_size;
	page_mask = hdr->page_size - 1;

	/* Infer the size of the fake block based on the newer ramdisk address */
	if (hdr->ramdisk_addr > 0x88f00000 || hdr->ramdisk_addr < 0xfa00000)
		fake_size = page_size;
	else
		fake_size = 0x200;

	orig_ramdisk_addr = find_ramdisk_addr(orig, st.st_size);
	if (orig_ramdisk_addr == 0) {
		LOGE("[-] Failed to find original ramdisk address.");
		return 1;
	}

	/* Restore the original header values */
	hdr->ramdisk_addr = orig_ramdisk_addr;
	hdr->kernel_size = orig_kernel_size = loki_hdr->orig_kernel_size;
	hdr->ramdisk_size = orig_ramdisk_size = loki_hdr->orig_ramdisk_size;

	/* Erase the loki header */
	memset(loki_hdr, 0, sizeof(*loki_hdr));

	/* Write the image header */
	if (write(ofd, orig, page_size) != page_size) {
		LOGE("[-] Failed to write header to output file.");
		return 1;
	}

	page_kernel_size = (orig_kernel_size + page_mask) & ~page_mask;

	/* Write the kernel */
	if (write(ofd, orig + page_size, page_kernel_size) != page_kernel_size) {
		LOGE("[-] Failed to write kernel to output file.");
		return 1;
	}

	page_ramdisk_size = (orig_ramdisk_size + page_mask) & ~page_mask;

	/* Write the ramdisk */
	if (write(ofd, orig + page_size + page_kernel_size, page_ramdisk_size) != page_ramdisk_size) {
		LOGE("[-] Failed to write ramdisk to output file.");
		return 1;
	}

	/* Write the device tree if needed */
	if (hdr->dt_size) {

		LOGV("[+] Writing device tree.");

		/* Skip an additional fake_size (page_size of 0x200) bytes */
		if (write(ofd, orig + page_size + page_kernel_size + page_ramdisk_size + fake_size, hdr->dt_size) != hdr->dt_size) {
			LOGE("[-] Failed to write device tree to output file.");
			return 1;
		}
	}

	close(ifd);
	close(ofd);

	LOGV("[+] Output file written to %s", out_image);

	return 0;
}
