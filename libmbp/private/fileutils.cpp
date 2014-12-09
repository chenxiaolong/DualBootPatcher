/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
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
        return PatcherError::createIOError(MBP::ErrorCode::FileOpenError, path);
    }

    file.seekg(0, std::ios::end);
    auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<unsigned char> data(size);

    if (!file.read(reinterpret_cast<char *>(data.data()), size)) {
        file.close();

        return PatcherError::createIOError(MBP::ErrorCode::FileReadError, path);
    }

    file.close();

    data.swap(*contents);

    return PatcherError();
}

PatcherError FileUtils::writeFromMemory(const std::string &path,
                                        const std::vector<unsigned char> &contents)
{
    std::ofstream file(path, std::ios::binary);

    if (file.fail()) {
        return PatcherError::createIOError(MBP::ErrorCode::FileOpenError, path);
    }

    file.write(reinterpret_cast<const char *>(contents.data()), contents.size());
    file.close();

    return PatcherError();
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
                                  std::vector<unsigned char> *output)
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
bool FileUtils::laCopyData(archive *aInput, archive *aOutput)
{
    int r;
    __LA_INT64_T offset;
    const void *buff;
    size_t bytes_read;

    while ((r = archive_read_data_block(aInput, &buff,
            &bytes_read, &offset)) == ARCHIVE_OK) {
        archive_write_data(aOutput, buff, bytes_read);
    }

    return r >= ARCHIVE_WARN;
}

bool FileUtils::laExtractFile(archive *aInput,
                              archive_entry *entry,
                              const std::string directory)
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
        Log::log(Log::Warning, "libarchive: %s", archive_error_string(aOutput));

        return PatcherError::createArchiveError(
                MBP::ErrorCode::ArchiveWriteHeaderError, name);
    }

    // Write contents
    unsigned int size = archive_write_data(aOutput, contents.data(), contents.size());
    if (size != contents.size()) {
        return PatcherError::createArchiveError(
                MBP::ErrorCode::ArchiveWriteDataError, name);
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
                MBP::ErrorCode::FileOpenError, path);
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
        Log::log(Log::Warning, "libarchive: %s", archive_error_string(aOutput));
        archive_entry_free(entry);

        return PatcherError::createArchiveError(
                MBP::ErrorCode::ArchiveWriteHeaderError, name);
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
                    MBP::ErrorCode::ArchiveWriteDataError, name);
        }
    }

    if (file.bad()) {
        file.close();
        return PatcherError::createIOError(
                MBP::ErrorCode::FileReadError, path);
    }

    file.close();

    return PatcherError();
}

/*!
    \brief Counts number of files inside an archive

    \param name Path to archive
    \param count Pointer to output variable

    \return PatcherError::NoError on success, or the appropriate error code on
            failure
 */
PatcherError FileUtils::laCountFiles(const std::string &path,
                                     unsigned int *count,
                                     std::vector<std::string> ignore)
{
    archive *aInput = archive_read_new();
    archive_read_support_filter_all(aInput);
    archive_read_support_format_all(aInput);

    int ret = archive_read_open_filename(aInput, path.c_str(), 10240);
    if (ret != ARCHIVE_OK) {
        Log::log(Log::Warning, "libarchive: %s", archive_error_string(aInput));
        archive_read_free(aInput);

        return PatcherError::createArchiveError(
                MBP::ErrorCode::ArchiveReadOpenError, path);
    }

    archive_entry *entry;

    int i = 0;

    while (archive_read_next_header(aInput, &entry) == ARCHIVE_OK) {
        const std::string name = archive_entry_pathname(entry);

        if (std::find(ignore.begin(), ignore.end(), name) == ignore.end()) {
            ++i;
        }
        archive_read_data_skip(aInput);
    }

    archive_read_free(aInput);

    *count = i;

    return PatcherError();
}
