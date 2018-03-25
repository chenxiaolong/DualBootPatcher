/*
 * Copyright (C) 2015-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "switcher.h"

#include <array>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <dirent.h>
#include <sys/stat.h>

#include <openssl/sha.h>

#include "mbcommon/finally.h"
#include "mbcommon/string.h"
#include "mblog/logging.h"
#include "mbutil/chmod.h"
#include "mbutil/chown.h"
#include "mbutil/copy.h"
#include "mbutil/directory.h"
#include "mbutil/file.h"
#include "mbutil/path.h"
#include "mbutil/properties.h"
#include "mbutil/string.h"

#include "multiboot.h"
#include "roms.h"

#define LOG_TAG "mbtool/switcher"

#define CHECKSUMS_PATH "/data/multiboot/checksums.prop"

namespace mb
{

/*!
 * \brief Check a checksum property
 *
 * \param props Pointer to properties map
 * \param rom_id ROM ID
 * \param image Image filename (without directory)
 * \param sha512_out SHA512 hex digest output
 *
 * \return ChecksumsGetResult::Found if the hash was successfully retrieved,
 *         ChecksumsGetResult::NotFound if the hash does not exist in the map,
 *         ChecksumsGetResult::Malformed if the property has an invalid format
 */
ChecksumsGetResult checksums_get(std::unordered_map<std::string, std::string> *props,
                                 const std::string &rom_id,
                                 const std::string &image,
                                 std::string *sha512_out)
{
    std::string checksums_path = get_raw_path(CHECKSUMS_PATH);

    std::string key(rom_id);
    key += "/";
    key += image;

    if (props->find(key) == props->end()) {
        return ChecksumsGetResult::NotFound;
    }

    const std::string &value = (*props)[key];

    std::size_t pos = value.find(":");
    if (pos != std::string::npos) {
        std::string algo = value.substr(0, pos);
        std::string hash = value.substr(pos + 1);
        if (algo != "sha512") {
            LOGE("%s: Invalid hash algorithm: %s",
                 checksums_path.c_str(), algo.c_str());
            return ChecksumsGetResult::Malformed;
        }

        *sha512_out = hash;
        return ChecksumsGetResult::Found;
    } else {
        LOGE("%s: Invalid checksum property: %s=%s",
             checksums_path.c_str(), key.c_str(), value.c_str());
        return ChecksumsGetResult::Malformed;
    }
}

/*!
 * \brief Update a checksum property
 *
 * \param props Pointer to properties map
 * \param rom_id ROM ID
 * \param image Image filename (without directory)
 * \param sha512 SHA512 hex digest
 */
void checksums_update(std::unordered_map<std::string, std::string> *props,
                      const std::string &rom_id,
                      const std::string &image,
                      const std::string &sha512)
{
    std::string key(rom_id);
    key += "/";
    key += image;

    (*props)[key] = "sha512:";
    (*props)[key] += sha512;
}

/*!
 * \brief Read checksums properties from \a /data/multiboot/checksums.prop
 *
 * \param props Pointer to properties map
 *
 * \return True if the file was successfully read. Otherwise, false.
 */
bool checksums_read(std::unordered_map<std::string, std::string> *props)
{
    std::string checksums_path = get_raw_path(CHECKSUMS_PATH);

    if (!util::property_file_get_all(checksums_path, *props)) {
        LOGE("%s: Failed to load properties", checksums_path.c_str());
        return false;
    }
    return true;
}

/*!
 * \brief Write checksums properties to \a /data/multiboot/checksums.prop
 *
 * \param props Properties map
 *
 * \return True if successfully written. Otherwise, false.
 */
bool checksums_write(const std::unordered_map<std::string, std::string> &props)
{
    std::string checksums_path = get_raw_path(CHECKSUMS_PATH);

    if (remove(checksums_path.c_str()) < 0 && errno != ENOENT) {
        LOGW("%s: Failed to remove file: %s",
             checksums_path.c_str(), strerror(errno));
    }

    (void) util::mkdir_parent(checksums_path, 0755);
    (void) util::create_empty_file(checksums_path);

    if (auto r = util::chown(checksums_path, 0, 0, 0); !r) {
        LOGW("%s: Failed to chown file: %s",
             checksums_path.c_str(), r.error().message().c_str());
    }
    if (chmod(checksums_path.c_str(), 0700) < 0) {
        LOGW("%s: Failed to chmod file: %s",
             checksums_path.c_str(), strerror(errno));
    }

    if (!util::property_file_write_all(checksums_path, props)) {
        LOGW("%s: Failed to write new properties: %s",
             checksums_path.c_str(), strerror(errno));
        return false;
    }

    return true;
}

struct Flashable
{
    std::string image;
    std::string block_dev;
    std::string expected_hash;
    std::string hash;
    std::vector<unsigned char> data;
};

/*!
 * \brief Perform non-recursive search for a block device
 *
 * This function will non-recursively search \a search_dirs for a block device
 * named \a partition. \a /dev/block/ is implicitly added to the search paths.
 *
 * \param search_dirs Search paths
 * \param partition Block device name
 *
 * \return Block device path if found. Otherwise, an empty string.
 */
static std::string find_block_dev(const std::vector<std::string> &search_dirs,
                                  const std::string &partition)
{
    struct stat sb;

    if (starts_with(partition, "mmcblk")) {
        std::string path("/dev/block/");
        path += partition;

        if (stat(path.c_str(), &sb) == 0) {
            return path;
        }
    }

    for (auto const &path : search_dirs) {
        std::string block_dev(path);
        block_dev += "/";
        block_dev += partition;

        if (stat(block_dev.c_str(), &sb) == 0) {
            return block_dev;
        }
    }

    return {};
}

static bool add_extra_images(const std::string &multiboot_dir,
                             const std::vector<std::string> &block_dev_dirs,
                             std::vector<Flashable> *flashables)
{
    DIR *dir;
    dirent *ent;

    dir = opendir(multiboot_dir.c_str());
    if (!dir) {
        return false;
    }

    auto close_directory = finally([&]{
        closedir(dir);
    });

    while ((ent = readdir(dir))) {
        std::string name(ent->d_name);
        if (name == ".img" || !ends_with(name, ".img")) {
            // Skip non-images
            continue;
        }
        if (starts_with(name, "boot.img")) {
            // Skip boot images, which are handled separately
            continue;
        }

        std::string partition = name.substr(0, name.size() - 4);
        std::string path(multiboot_dir);
        path += "/";
        path += name;

        // Blacklist non-modem partitions
        if (partition != "mdm"
                && partition != "modem"
                && partition != "apnhlos") {
            LOGW("Partition %s is not whitelisted for flashing",
                 partition.c_str());
            continue;
        }

        std::string block_dev = find_block_dev(block_dev_dirs, partition);
        if (block_dev.empty()) {
            LOGW("Couldn't find block device for partition %s",
                 partition.c_str());
            continue;
        }

        LOGD("Found extra image:");
        LOGD("- Source: %s", path.c_str());
        LOGD("- Target: %s", block_dev.c_str());

        flashables->emplace_back();
        flashables->back().image = std::move(path);
        flashables->back().block_dev = std::move(block_dev);
    }

    return true;
}

/*!
 * \brief Switch to another ROM
 *
 * \note If the checksum is missing for some images to be flashed and invalid
 *       for some other images to be flashed, this function will always return
 *       SwitchRomResult::ChecksumInvalid.
 *
 * \param id ROM ID to switch to
 * \param boot_blockdev Block device path of the boot partition
 * \param blockdev_base_dirs Search paths (non-recursive) for block devices
 *                           corresponding to extra flashable images in
 *                           /sdcard/MultiBoot/[ROM ID]/ *.img
 *
 * \return SwitchRomResult::Succeeded if the switching succeeded,
 *         SwitchRomResult::Failed if the switching failed,
 *         SwitchRomResult::ChecksumNotFound if the checksum for some image is missing,
 *         SwitchRomResult::ChecksumInvalid if the checksum for some image is invalid
 *
 */
SwitchRomResult switch_rom(const std::string &id,
                           const std::string &boot_blockdev,
                           const std::vector<std::string> &blockdev_base_dirs,
                           bool force_update_checksums)
{
    LOGD("Attempting to switch to %s", id.c_str());
    LOGD("Force update checksums: %d", force_update_checksums);

    // Path for all of the images
    std::string multiboot_path(get_raw_path(MULTIBOOT_DIR));
    multiboot_path += "/";
    multiboot_path += id;

    std::string bootimg_path(multiboot_path);
    bootimg_path += "/boot.img";

    // Verify ROM ID
    Roms roms;
    roms.add_installed();

    if (!roms.find_by_id(id)) {
        LOGE("Invalid ROM ID: %s", id.c_str());
        return SwitchRomResult::Failed;
    }

    if (auto r = util::mkdir_recursive(multiboot_path, 0775); !r) {
        LOGE("%s: Failed to create directory: %s",
             multiboot_path.c_str(), r.error().message().c_str());
        return SwitchRomResult::Failed;
    }

    // We'll read the files we want to flash into memory so a malicious app
    // can't change the file between the hash verification step and flashing
    // step.

    std::vector<Flashable> flashables;

    flashables.emplace_back();
    flashables.back().image = bootimg_path;
    flashables.back().block_dev = boot_blockdev;

    if (!add_extra_images(multiboot_path, blockdev_base_dirs, &flashables)) {
        LOGW("Failed to find extra images");
    }

    std::unordered_map<std::string, std::string> props;
    checksums_read(&props);

    for (Flashable &f : flashables) {
        // If memory becomes an issue, an alternative method is to create a
        // temporary directory in /data/multiboot/ that's only writable by root
        // and copy the images there.
        if (auto r = util::file_read_all(f.image)) {
            f.data = std::move(r.value());
        } else {
            LOGE("%s: Failed to read image: %s",
                 f.image.c_str(), r.error().message().c_str());
            return SwitchRomResult::Failed;
        }

        // Get actual sha512sum
        std::array<unsigned char, SHA512_DIGEST_LENGTH> digest;
        SHA512(f.data.data(), f.data.size(), digest.data());
        f.hash = util::hex_string(digest.data(), digest.size());

        if (force_update_checksums) {
            checksums_update(&props, id, util::base_name(f.image), f.hash);
        }

        // Get expected sha512sum
        ChecksumsGetResult ret = checksums_get(
                &props, id, util::base_name(f.image), &f.expected_hash);
        if (ret == ChecksumsGetResult::Malformed) {
            return SwitchRomResult::ChecksumInvalid;
        }

        // Verify hashes if we have an expected hash
        if (ret == ChecksumsGetResult::Found && f.expected_hash != f.hash) {
            LOGE("%s: Checksum (%s) does not match expected (%s)",
                 f.image.c_str(), f.hash.c_str(), f.expected_hash.c_str());
            return SwitchRomResult::ChecksumInvalid;
        }
    }

    // Fail if we're missing expected hashes. We do this last to make sure
    // CHECKSUM_INVALID is returned if some checksums don't match (for the ones
    // that aren't missing).
    for (Flashable &f : flashables) {
        if (f.expected_hash.empty()) {
            LOGE("%s: Checksum does not exist", f.image.c_str());
            return SwitchRomResult::ChecksumNotFound;
        }
    }

    // Now we can flash the images
    for (Flashable &f : flashables) {
        if (auto r = util::file_write_data(
                f.block_dev, f.data.data(), f.data.size()); !r) {
            LOGE("%s: Failed to write image: %s",
                 f.block_dev.c_str(), r.error().message().c_str());
            return SwitchRomResult::Failed;
        }
    }

    if (force_update_checksums) {
        LOGD("Updating checksums file");
        checksums_write(props);
    }

    if (!fix_multiboot_permissions()) {
        //return SwitchRomResult::Failed;
    }

    return SwitchRomResult::Succeeded;
}

/*!
 * \brief Set the kernel for a ROM
 *
 * \note This will update the checksum for the image in
 *       \a /data/multiboot/checksums.prop.
 *
 * \param id ROM ID to set the kernel for
 * \param boot_blockdev Block device path of the boot partition
 *
 * \return True if the kernel was successfully set. Otherwise, false.
 */
bool set_kernel(const std::string &id, const std::string &boot_blockdev)
{
    LOGD("Attempting to set the kernel for %s", id.c_str());

    // Path for all of the images
    std::string multiboot_path(get_raw_path(MULTIBOOT_DIR));
    multiboot_path += "/";
    multiboot_path += id;

    std::string bootimg_path(multiboot_path);
    bootimg_path += "/boot.img";

    // Verify ROM ID
    Roms roms;
    roms.add_installed();

    if (!roms.find_by_id(id)) {
        LOGE("Invalid ROM ID: %s", id.c_str());
        return false;
    }

    if (auto r = util::mkdir_recursive(multiboot_path, 0775); !r) {
        LOGE("%s: Failed to create directory: %s",
             multiboot_path.c_str(), r.error().message().c_str());
        return false;
    }

    auto data = util::file_read_all(boot_blockdev);
    if (!data) {
        LOGE("%s: Failed to read block device: %s",
             boot_blockdev.c_str(), data.error().message().c_str());
        return false;
    }

    // Get actual sha512sum
    std::array<unsigned char, SHA512_DIGEST_LENGTH> digest;
    SHA512(data.value().data(), data.value().size(), digest.data());
    std::string hash = util::hex_string(digest.data(), digest.size());

    // Add to checksums.prop
    std::unordered_map<std::string, std::string> props;
    checksums_read(&props);
    checksums_update(&props, id, "boot.img", hash);

    // NOTE: This function isn't responsible for updating the checksums for
    //       any extra images. We don't want to mask any malicious changes.

    // Cast is okay. The data is just passed to fwrite (ie. no signed
    // extension issues)
    if (auto r = util::file_write_data(
            bootimg_path, data.value().data(), data.value().size()); !r) {
        LOGE("%s: Failed to write image: %s",
             bootimg_path.c_str(), r.error().message().c_str());
        return false;
    }

    LOGD("Updating checksums file");
    checksums_write(props);

    if (!fix_multiboot_permissions()) {
        //return false;
    }

    return true;
}

}
