/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "roms.h"
#include "util/hash.h"

namespace mb
{

class Installer {
public:
    Installer(std::string zip_file, std::string chroot_dir,
              std::string temp_dir, int interface, int output_fd);
    ~Installer();

    bool start_installation();


protected:
    static const std::string HELPER_TOOL;
    static const std::string UPDATE_BINARY;
    static const std::string MULTIBOOT_AROMA;
    static const std::string MULTIBOOT_BBWRAPPER;
    static const std::string MULTIBOOT_INFO_PROP;
    static const std::string TEMP_SYSTEM_IMAGE;
    static const std::string CANCELLED;

    enum class ProceedState {
        Continue,
        Fail,
        Cancel
    };

    virtual void display_msg(const std::string &msg);
    virtual void updater_print(const std::string &msg);
    virtual void command_output(const std::string &line);
    virtual std::string get_install_type() = 0;
    virtual ProceedState on_initialize();
    virtual ProceedState on_created_chroot();
    virtual ProceedState on_checked_device();
    virtual ProceedState on_set_up_chroot();
    virtual ProceedState on_mounted_filesystems();
    virtual ProceedState on_pre_install();
    virtual ProceedState on_post_install(bool install_ret);
    virtual ProceedState on_unmounted_filesystems();
    virtual ProceedState on_finished();
    virtual void on_cleanup(ProceedState ret);

    std::string _zip_file;
    std::string _chroot;
    std::string _temp;
    int _interface;
    int _output_fd;
    bool _passthrough;

    std::string _device;
    std::string _boot_block_dev;
    std::string _recovery_block_dev;
    unsigned char _boot_hash[SHA_DIGEST_SIZE];
    std::shared_ptr<Rom> _rom;

    std::unordered_map<std::string, std::string> _prop;

    bool _has_block_image;
    bool _is_aroma;

    std::string in_chroot(const std::string &path) const;

    static bool is_aroma(const std::string &path);


private:
    static void output_cb(const std::string &msg, void *data);
    int run_command(const std::vector<std::string> &argv);
    int run_command_chroot(const std::string &dir,
                           const std::vector<std::string> &argv);

    bool create_chroot();
    bool destroy_chroot() const;

    bool extract_multiboot_files();
    bool set_up_busybox_wrapper();
    bool create_temporary_image(const std::string &path);
    bool system_image_copy(const std::string &source,
                           const std::string &image, bool reverse);
    bool run_real_updater();

    ProceedState install_stage_initialize();
    ProceedState install_stage_create_chroot();
    ProceedState install_stage_set_up_environment();
    ProceedState install_stage_check_device();
    ProceedState install_stage_get_install_type();
    ProceedState install_stage_set_up_chroot();
    ProceedState install_stage_mount_filesystems();
    ProceedState install_stage_installation();
    ProceedState install_stage_unmount_filesystems();
    ProceedState install_stage_finish();
    void install_stage_cleanup(ProceedState ret);
};

int update_binary_main(int argc, char *argv[]);

}
