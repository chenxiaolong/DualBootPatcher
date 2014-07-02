/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "common.h"

#include <cstring>
#include <iostream>

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

bool exists_directory(std::string path) {
    struct stat s;
    if (stat(path.c_str(), &s) != 0) {
        return false;
    }

    return S_ISDIR(s.st_mode);
}

bool exists_file(std::string path) {
    struct stat s;
    if (stat(path.c_str(), &s) != 0) {
        return false;
    }

    return S_ISREG(s.st_mode);
}

bool is_same_inode(std::string path1, std::string path2) {
    struct stat s1;
    struct stat s2;

    if (stat(path1.c_str(), &s1) != 0 || stat(path2.c_str(), &s2) != 0) {
        return false;
    }

    return s1.st_ino == s2.st_ino;
}

int recursively_delete(std::string directory) {
    if (!exists_directory(directory)) {
        return 0;
    }

    DIR *dir = opendir(directory.c_str());

    if (dir) {
        struct dirent *e;
        while ((e = readdir(dir)) != NULL) {
            if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) {
                continue;
            }

            std::string path = directory + SEP + std::string(e->d_name);

            struct stat s;
            if (stat(path.c_str(), &s) != 0) {
                std::cout << "Failed to stat " << path << std::endl;
                continue;
            }

            int ret = 0;

            if (S_ISDIR(s.st_mode)) {
                ret = recursively_delete(path);
            } else {
                ret = unlink(path.c_str());
            }

            if (ret != 0) {
                std::cout << "Failed to remove " << path << std::endl;
            }
        }
    }

    // Ignore return values from recursive calls since rmdir() will fail anyway
    return rmdir(directory.c_str());
}

std::string dirname2(std::string path) {
    // Ignore case where path ends in '/'

    size_t pos = path.rfind(SEP);
    if (pos == std::string::npos) {
        return ".";
    }

    path.erase(pos, path.size() - 1);
    return path;
}

std::string basename2(std::string path) {
    size_t pos = path.rfind(SEP);
    if (pos == std::string::npos) {
        return path;
    }

    path.erase(0, pos + 1);
    return path;
}

std::string search_directory(std::string directory, std::string begin) {
    DIR *dir = opendir(directory.c_str());

    if (dir) {
        struct dirent *e;
        while ((e = readdir(dir)) != NULL) {
            std::string name = e->d_name;
            if (name.size() >= begin.size()
                    && name.substr(0, begin.size()) == begin) {
                closedir(dir);
                return name;
            }
        }

        closedir(dir);
    }

    return "";
}
