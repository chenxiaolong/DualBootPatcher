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

#include "private/fileutils.h"

#include <algorithm>
#include <fstream>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "private/logging.h"


namespace mbp
{

/*!
    \brief Read contents of a file into memory

    \param path Path to file
    \param contents Output vector (not modified unless reading succeeds)

    \return Success or not
 */
PatcherError FileUtils::readToMemory(const std::string &path,
                                     std::vector<unsigned char> *contents)
{
    std::ifstream file(path, std::ios::binary);

    if (file.fail()) {
        return PatcherError::createIOError(ErrorCode::FileOpenError, path);
    }

    file.seekg(0, std::ios::end);
    auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<unsigned char> data(size);

    if (!file.read(reinterpret_cast<char *>(data.data()), size)) {
        return PatcherError::createIOError(ErrorCode::FileReadError, path);
    }

    data.swap(*contents);

    return PatcherError();
}

/*!
    \brief Read contents of a file into a string

    \param path Path to file
    \param contents Output string (not modified unless reading succeeds)

    \return Success or not
 */
PatcherError FileUtils::readToString(const std::string &path,
                                     std::string *contents)
{
    std::ifstream file(path, std::ios::binary);

    if (file.fail()) {
        return PatcherError::createIOError(ErrorCode::FileOpenError, path);
    }

    file.seekg(0, std::ios::end);
    auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string data;
    data.resize(size);

    if (!file.read(&data[0], size)) {
        return PatcherError::createIOError(ErrorCode::FileReadError, path);
    }

    data.swap(*contents);

    return PatcherError();
}

PatcherError FileUtils::writeFromMemory(const std::string &path,
                                        const std::vector<unsigned char> &contents)
{
    std::ofstream file(path, std::ios::binary);

    if (file.fail()) {
        return PatcherError::createIOError(ErrorCode::FileOpenError, path);
    }

    file.write(reinterpret_cast<const char *>(contents.data()), contents.size());

    return PatcherError();
}

PatcherError FileUtils::writeFromString(const std::string &path,
                                        const std::string &contents)
{
    std::ofstream file(path, std::ios::binary);

    if (file.fail()) {
        return PatcherError::createIOError(ErrorCode::FileOpenError, path);
    }

    file.write(contents.data(), contents.size());

    return PatcherError();
}

std::string FileUtils::createTemporaryDir(const std::string &directory)
{
#ifdef __ANDROID__
    // For whatever reason boost::filesystem::unique_path() returns an empty
    // path on pre-lollipop ROMs

    boost::filesystem::path dir(directory);
    dir /= "mbp-XXXXXX";
    const std::string dirTemplate = dir.string();

    // mkdtemp modifies buffer
    std::vector<char> buf(dirTemplate.begin(), dirTemplate.end());
    buf.push_back('\0');

    if (mkdtemp(buf.data())) {
        return std::string(buf.data());
    }

    return std::string();
#else
    int count = 256;

    while (count > 0) {
        boost::filesystem::path dir(directory);
        dir /= "mbp-%%%%%%";
        auto path = boost::filesystem::unique_path(dir);
        if (boost::filesystem::create_directory(path)) {
            return path.string();
        }
        count--;
    }

    // Like Qt, we'll assume that 256 tries is enough ...
    return std::string();
#endif
}

/*!
    \brief Read contents of a file from libarchive into memory

    \param aInput libarchive input file pointer
    \param entry libarchive entry to file being read
    \param output Output vector (not modified unless reading succeeds)

    \return Success or not
 */
bool FileUtils::laReadToByteArray(archive *aInput,
                                  archive_entry *entry,
                                  std::vector<unsigned char> *output,
                                  void (*cb)(uint64_t bytes, void *), void *userData)
{
    std::vector<unsigned char> data;
    data.reserve(archive_entry_size(entry));

    int r;
    __LA_INT64_T offset;
    const char *buff;
    size_t bytes_read;

    while ((r = archive_read_data_block(aInput,
            reinterpret_cast<const void **>(&buff),
                    &bytes_read, &offset)) == ARCHIVE_OK) {
        if (cb) {
            cb(data.size() + bytes_read, userData);
        }

        data.insert(data.end(), buff, buff + bytes_read);
    }

    if (r < ARCHIVE_WARN) {
        return false;
    }

    data.swap(*output);
    return true;
}

/*!
    \brief Copy single file's contents from libarchive input file to libarchive
           output file

    \param aInput libarchive input file pointer
    \param aOutput libarchive output file pointer

    \return Success or not
 */
bool FileUtils::laCopyData(archive *aInput, archive *aOutput,
                           void (*cb)(uint64_t bytes, void *), void *userData)
{
    uint64_t bytes = 0;

    // Exceeds Android's default stack size, unfortunately, so allocate
    // on the heap
    std::vector<unsigned char> buf(1024 * 1024); // 1MiB
    size_t bytes_read;

    while ((bytes_read = archive_read_data(
            aInput, buf.data(), buf.size())) > 0) {
        bytes += bytes_read;
        if (cb) {
            cb(bytes, userData);
        }

        archive_write_data(aOutput, buf.data(), bytes_read);
    }

    return bytes_read == 0;
}

bool FileUtils::laExtractFile(archive *aInput,
                              archive_entry *entry,
                              const std::string &directory)
{
    int r;
    __LA_INT64_T offset;
    const void *buff;
    size_t bytes_read;

    std::string fullPath = directory + "/" + archive_entry_pathname(entry);
    boost::filesystem::path path(fullPath);
    boost::filesystem::create_directories(path.parent_path());

    std::ofstream file(fullPath, std::ios::binary);

    if (file.fail()) {
        return false;
    }

    while ((r = archive_read_data_block(aInput, &buff,
            &bytes_read, &offset)) == ARCHIVE_OK) {
        file.write(reinterpret_cast<const char *>(buff), bytes_read);
    }

    file.close();

    return r >= ARCHIVE_WARN;
}

/*!
    \brief Writes file to libarchive output file

    The added file will have the following attributes in the archive (where
    applicable):

    - uid: 0
    - gid: 0
    - nlink: 1
    - mtime: 0
    - devmajor: 0
    - devminor: 0
    - rdevmajor: 0
    - rdevminor: 0
    - filetype: IFREG
    - mode: 0644

    \param aOutput libarchive output file pointer
    \param name Filename
    \param contents File contents

    \return PatcherError::NoError on success, or the appropriate error code on
            failure
 */
PatcherError FileUtils::laAddFile(archive * const aOutput,
                                  const std::string &name,
                                  const std::vector<unsigned char> &contents)
{
    archive_entry *entry = archive_entry_new();

    archive_entry_set_uid(entry, 0);
    archive_entry_set_gid(entry, 0);
    archive_entry_set_nlink(entry, 1);
    archive_entry_set_mtime(entry, 0, 0);
    archive_entry_set_devmajor(entry, 0);
    archive_entry_set_devminor(entry, 0);
    archive_entry_set_rdevmajor(entry, 0);
    archive_entry_set_rdevminor(entry, 0);

    archive_entry_set_pathname(entry, name.c_str());
    archive_entry_set_size(entry, contents.size());

    archive_entry_set_filetype(entry, AE_IFREG);
    archive_entry_set_perm(entry, 0644);

    // Write header to new file
    if (archive_write_header(aOutput, entry) != ARCHIVE_OK) {
        FLOGW("libarchive: {}", archive_error_string(aOutput));

        archive_entry_free(entry);

        return PatcherError::createArchiveError(
                ErrorCode::ArchiveWriteHeaderError, name);
    }

    // Write contents
    unsigned int size = archive_write_data(aOutput, contents.data(), contents.size());
    if (size != contents.size()) {
        archive_entry_free(entry);

        return PatcherError::createArchiveError(
                ErrorCode::ArchiveWriteDataError, name);
    }

    archive_entry_free(entry);

    return PatcherError();
}

/*!
    \brief Reads file from disk and writes it to the libarchive output file

    The added file will have the following attributes in the archive (where
    applicable):

    - uid: 0
    - gid: 0
    - nlink: 1
    - mtime: 0
    - devmajor: 0
    - devminor: 0
    - rdevmajor: 0
    - rdevminor: 0
    - filetype: IFREG
    - mode: 0644

    \param aOutput libarchive output file pointer
    \param name Filename
    \param path Path to input file

    \return PatcherError::NoError on success, or the appropriate error code on
            failure
 */
PatcherError FileUtils::laAddFile(archive * const aOutput,
                                  const std::string &name,
                                  const std::string &path)
{
    // Copy file into archive
    std::ifstream file(path, std::ios::binary);
    if (file.fail()) {
        return PatcherError::createIOError(
                ErrorCode::FileOpenError, path);
    }

    file.seekg(0, std::ios::end);
    auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    archive_entry *entry = archive_entry_new();

    archive_entry_set_uid(entry, 0);
    archive_entry_set_gid(entry, 0);
    archive_entry_set_nlink(entry, 1);
    archive_entry_set_mtime(entry, 0, 0);
    archive_entry_set_devmajor(entry, 0);
    archive_entry_set_devminor(entry, 0);
    archive_entry_set_rdevmajor(entry, 0);
    archive_entry_set_rdevminor(entry, 0);

    archive_entry_set_pathname(entry, name.c_str());
    archive_entry_set_size(entry, size);

    archive_entry_set_filetype(entry, AE_IFREG);
    archive_entry_set_perm(entry, 0644);

    // Write header to new file
    if (archive_write_header(aOutput, entry) != ARCHIVE_OK) {
        FLOGW("libarchive: {}", archive_error_string(aOutput));
        archive_entry_free(entry);

        return PatcherError::createArchiveError(
                ErrorCode::ArchiveWriteHeaderError, name);
    }

    archive_entry_free(entry);

    // Write data to file
    char buf[32768];
    std::streamsize n;

    while (!file.eof()) {
        file.read(buf, 32768);
        n = file.gcount();

        if (archive_write_data(aOutput, buf, n) != n) {
            file.close();
            return PatcherError::createArchiveError(
                    ErrorCode::ArchiveWriteDataError, name);
        }
    }

    if (file.bad()) {
        file.close();
        return PatcherError::createIOError(
                ErrorCode::FileReadError, path);
    }

    file.close();

    return PatcherError();
}

/*!
    \brief Get some stats from an archive

    \param name Path to archive
    \param stats Pointer to ArchiveStats variable

    \return PatcherError::NoError on success, or the appropriate error code on
            failure
 */
PatcherError FileUtils::laArchiveStats(const std::string &path,
                                       ArchiveStats *stats,
                                       std::vector<std::string> ignore)
{
    assert(stats != nullptr);

    archive *aInput = archive_read_new();
    archive_read_support_filter_none(aInput);
    archive_read_support_format_zip(aInput);
    //archive_read_support_filter_all(aInput);
    //archive_read_support_format_all(aInput);

    int ret = archive_read_open_filename(aInput, path.c_str(), 10240);
    if (ret != ARCHIVE_OK) {
        FLOGW("libarchive: {}", archive_error_string(aInput));
        archive_read_free(aInput);

        return PatcherError::createArchiveError(
                ErrorCode::ArchiveReadOpenError, path);
    }

    archive_entry *entry;

    uint64_t count = 0;
    uint64_t totalSize = 0;

    while (archive_read_next_header(aInput, &entry) == ARCHIVE_OK) {
        const std::string name = archive_entry_pathname(entry);

        if (std::find(ignore.begin(), ignore.end(), name) == ignore.end()) {
            ++count;
            totalSize += archive_entry_size(entry);
        }
        archive_read_data_skip(aInput);
    }

    archive_read_free(aInput);

    stats->files = count;
    stats->totalSize = totalSize;

    return PatcherError();
}

}