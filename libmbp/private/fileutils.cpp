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

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "private/logging.h"
#include "private/io.h"
#include "private/utf8.h"

#ifdef _WIN32
#define USEWIN32IOAPI
#include "external/minizip/iowin32.h"
#else
#include <sys/stat.h>
#endif


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
    io::File file;
    if (!file.open(path, io::File::OpenRead)) {
        FLOGE("%s: Failed to open for reading: %s",
              path.c_str(), file.errorString().c_str());
        return PatcherError::createIOError(ErrorCode::FileOpenError, path);
    }

    uint64_t size;
    file.seek(0, io::File::SeekEnd);
    file.tell(&size);
    file.seek(0, io::File::SeekBegin);

    std::vector<unsigned char> data(size);

    uint64_t bytesRead;
    if (!file.read(data.data(), data.size(), &bytesRead) || bytesRead != size) {
        FLOGE("%s: Failed to read file: %s",
              path.c_str(), file.errorString().c_str());
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
    io::File file;
    if (!file.open(path, io::File::OpenRead)) {
        FLOGE("%s: Failed to open for reading: %s",
              path.c_str(), file.errorString().c_str());
        return PatcherError::createIOError(ErrorCode::FileOpenError, path);
    }

    uint64_t size;
    file.seek(0, io::File::SeekEnd);
    file.tell(&size);
    file.seek(0, io::File::SeekBegin);

    std::string data;
    data.resize(size);

    uint64_t bytesRead;
    if (!file.read(&data[0], size, &bytesRead) || bytesRead != size) {
        FLOGE("%s: Failed to read file: %s",
              path.c_str(), file.errorString().c_str());
        return PatcherError::createIOError(ErrorCode::FileReadError, path);
    }

    data.swap(*contents);

    return PatcherError();
}

PatcherError FileUtils::writeFromMemory(const std::string &path,
                                        const std::vector<unsigned char> &contents)
{
    io::File file;
    if (!file.open(path, io::File::OpenWrite)) {
        FLOGE("%s: Failed to open for writing: %s",
              path.c_str(), file.errorString().c_str());
        return PatcherError::createIOError(ErrorCode::FileOpenError, path);
    }

    uint64_t bytesWritten;
    if (!file.write(contents.data(), contents.size(), &bytesWritten)) {
        FLOGE("%s: Failed to write file: %s",
              path.c_str(), file.errorString().c_str());
        return PatcherError::createIOError(ErrorCode::FileWriteError, path);
    }

    return PatcherError();
}

PatcherError FileUtils::writeFromString(const std::string &path,
                                        const std::string &contents)
{
    io::File file;
    if (!file.open(path, io::File::OpenWrite)) {
        FLOGE("%s: Failed to open for writing: %s",
              path.c_str(), file.errorString().c_str());
        return PatcherError::createIOError(ErrorCode::FileOpenError, path);
    }

    uint64_t bytesWritten;
    if (!file.write(contents.data(), contents.size(), &bytesWritten)) {
        FLOGE("%s: Failed to write file: %s",
              path.c_str(), file.errorString().c_str());
        return PatcherError::createIOError(ErrorCode::FileWriteError, path);
    }

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

unzFile FileUtils::mzOpenInputFile(const std::string &path)
{
#ifdef USEWIN32IOAPI
    zlib_filefunc64_def zFunc;
    memset(&zFunc, 0, sizeof(zFunc));
    fill_win32_filefunc64W(&zFunc);
    return unzOpen2_64(utf8::utf8ToUtf16(path).c_str(), &zFunc);
#else
    return unzOpen64(path.c_str());
#endif
}

zipFile FileUtils::mzOpenOutputFile(const std::string &path)
{
#ifdef USEWIN32IOAPI
    zlib_filefunc64_def zFunc;
    memset(&zFunc, 0, sizeof(zFunc));
    fill_win32_filefunc64W(&zFunc);
    return zipOpen2_64(utf8::utf8ToUtf16(path).c_str(), 0, nullptr, &zFunc);
#else
    return zipOpen64(path.c_str(), 0);
#endif
}

int FileUtils::mzCloseInputFile(unzFile uf)
{
    return unzClose(uf);
}

int FileUtils::mzCloseOutputFile(zipFile zf)
{
    return zipClose(zf, nullptr);
}

PatcherError FileUtils::mzArchiveStats(const std::string &path,
                                       FileUtils::ArchiveStats *stats,
                                       std::vector<std::string> ignore)
{
    assert(stats != nullptr);

    unzFile uf = mzOpenInputFile(path);

    if (!uf) {
        FLOGE("minizip: Failed to open for reading: %s", path.c_str());
        return PatcherError::createArchiveError(
                ErrorCode::ArchiveReadOpenError, path);
    }

    uint64_t count = 0;
    uint64_t totalSize = 0;
    std::string name;
    unz_file_info64 fi;
    memset(&fi, 0, sizeof(fi));

    int ret = unzGoToFirstFile(uf);
    if (ret != UNZ_OK) {
        mzCloseInputFile(uf);
        return PatcherError::createArchiveError(
                ErrorCode::ArchiveReadHeaderError, std::string());
    }

    do {
        if (!mzGetInfo(uf, &fi, &name)) {
            mzCloseInputFile(uf);
            return PatcherError::createArchiveError(
                    ErrorCode::ArchiveReadHeaderError, std::string());
        }

        if (std::find(ignore.begin(), ignore.end(), name) == ignore.end()) {
            ++count;
            totalSize += fi.uncompressed_size;
        }
    } while ((ret = unzGoToNextFile(uf)) == UNZ_OK);

    if (ret != UNZ_END_OF_LIST_OF_FILE) {
        mzCloseInputFile(uf);
        return PatcherError::createArchiveError(
                ErrorCode::ArchiveReadHeaderError, std::string());
    }

    mzCloseInputFile(uf);

    stats->files = count;
    stats->totalSize = totalSize;

    return PatcherError();
}

bool FileUtils::mzGetInfo(unzFile uf,
                          unz_file_info64 *fi,
                          std::string *filename)
{
    unz_file_info64 info;
    memset(&info, 0, sizeof(info));

    // First query to get filename size
    int ret = unzGetCurrentFileInfo64(
        uf,                     // file
        &info,                  // pfile_info
        nullptr,                // filename
        0,                      // filename_size
        nullptr,                // extrafield
        0,                      // extrafield_size
        nullptr,                // comment
        0                       // comment_size
    );

    if (ret != UNZ_OK) {
        return false;
    }

    if (filename) {
        std::vector<char> buf(info.size_filename + 1);

        ret = unzGetCurrentFileInfo64(
            uf,             // file
            &info,          // pfile_info
            buf.data(),     // filename
            buf.size(),     // filename_size
            nullptr,        // extrafield
            0,              // extrafield_size
            nullptr,        // comment
            0               // comment_size
        );

        if (ret != UNZ_OK) {
            return false;
        }

        *filename = buf.data();
    }

    if (fi) {
        *fi = info;
    }

    return true;
}

bool FileUtils::mzCopyFileRaw(unzFile uf,
                              zipFile zf,
                              const std::string &name,
                              void (*cb)(uint64_t bytes, void *), void *userData)
{
    unz_file_info64 ufi;

    if (!mzGetInfo(uf, &ufi, nullptr)) {
        return false;
    }

    bool zip64 = ufi.uncompressed_size >= ((1ull << 32) - 1);

    zip_fileinfo zfi;
    memset(&zfi, 0, sizeof(zfi));

    zfi.dosDate = ufi.dosDate;
    zfi.tmz_date.tm_sec = ufi.tmu_date.tm_sec;
    zfi.tmz_date.tm_min = ufi.tmu_date.tm_min;
    zfi.tmz_date.tm_hour = ufi.tmu_date.tm_hour;
    zfi.tmz_date.tm_mday = ufi.tmu_date.tm_mday;
    zfi.tmz_date.tm_mon = ufi.tmu_date.tm_mon;
    zfi.tmz_date.tm_year = ufi.tmu_date.tm_year;
    zfi.internal_fa = ufi.internal_fa;
    zfi.external_fa = ufi.external_fa;

    int method;
    int level;

    // Open raw file in input zip
    int ret = unzOpenCurrentFile2(
        uf,                     // file
        &method,                // method
        &level,                 // level
        1                       // raw
    );
    if (ret != UNZ_OK) {
        return false;
    }

    // Open raw file in output zip
    ret = zipOpenNewFileInZip2_64(
        zf,             // file
        name.c_str(),   // filename
        &zfi,           // zip_fileinfo
        nullptr,        // extrafield_local
        0,              // size_extrafield_local
        nullptr,        // extrafield_global
        0,              // size_extrafield_global
        nullptr,        // comment
        method,         // method
        level,          // level
        1,              // raw
        zip64           // zip64
    );
    if (ret != ZIP_OK) {
        unzCloseCurrentFile(uf);
        return false;
    }

    uint64_t bytes = 0;

    // Exceeds Android's default stack size, unfortunately, so allocate
    // on the heap
    std::vector<unsigned char> buf(1024 * 1024); // 1MiB
    int bytes_read;
    double ratio;

    while ((bytes_read = unzReadCurrentFile(uf, buf.data(), buf.size())) > 0) {
        bytes += bytes_read;
        if (cb) {
            // Scale this to the uncompressed size for the purposes of a
            // progress bar
            ratio = (double) bytes / ufi.compressed_size;
            cb(ratio * ufi.uncompressed_size, userData);
        }

        ret = zipWriteInFileInZip(zf, buf.data(), bytes_read);
        if (ret != ZIP_OK) {
            unzCloseCurrentFile(uf);
            zipCloseFileInZip(zf);
            return false;
        }
    }

    unzCloseCurrentFile(uf);
    zipCloseFileInZipRaw64(zf, ufi.uncompressed_size, ufi.crc);

    return bytes_read == 0;
}

bool FileUtils::mzReadToMemory(unzFile uf,
                               std::vector<unsigned char> *output,
                               void (*cb)(uint64_t bytes, void *), void *userData)
{
    unz_file_info64 fi;

    if (!mzGetInfo(uf, &fi, nullptr)) {
        return false;
    }

    std::vector<unsigned char> data;
    data.reserve(fi.uncompressed_size);

    int ret = unzOpenCurrentFile(uf);
    if (ret != UNZ_OK) {
        return false;
    }

    int n;
    char buf[32768];

    while ((n = unzReadCurrentFile(uf, buf, sizeof(buf))) > 0) {
        if (cb) {
            cb(data.size() + n, userData);
        }

        data.insert(data.end(), buf, buf + n);
    }

    unzCloseCurrentFile(uf);

    if (n != 0) {
        return false;
    }

    data.swap(*output);
    return true;
}

bool FileUtils::mzExtractFile(unzFile uf,
                              const std::string &directory)
{
    unz_file_info64 fi;
    std::string filename;

    if (!mzGetInfo(uf, &fi, &filename)) {
        return false;
    }

    std::string fullPath(directory);
    fullPath += "/";
    fullPath += filename;

    boost::filesystem::path path(fullPath);
    boost::filesystem::create_directories(path.parent_path());

    io::File file;
    if (!file.open(fullPath, io::File::OpenWrite)) {
        FLOGE("%s: Failed to open for writing: %s",
              path.c_str(), file.errorString().c_str());
        return false;
    }

    int ret = unzOpenCurrentFile(uf);
    if (ret != UNZ_OK) {
        return false;
    }

    int n;
    char buf[32768];
    uint64_t bytesWritten;

    while ((n = unzReadCurrentFile(uf, buf, sizeof(buf))) > 0) {
        if (!file.write(buf, n, &bytesWritten)) {
            FLOGE("%s: Failed to write file: %s",
                  path.c_str(), file.errorString().c_str());
            return false;
        }
    }

    unzCloseCurrentFile(uf);

    return n == 0;
}

static bool mzGetFileTime(const std::string &filename,
                          tm_zip *tmzip, uLong *dostime)
{
    // Don't fail when building with -Werror
    (void) filename;
    (void) tmzip;
    (void) dostime;

#ifdef _WIN32
    FILETIME ftLocal;
    HANDLE hFind;
    WIN32_FIND_DATAA ff32;

    hFind = FindFirstFileA(filename.c_str(), &ff32);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        FileTimeToLocalFileTime(&ff32.ftLastWriteTime, &ftLocal);
        FileTimeToDosDateTime(&ftLocal,
                              ((LPWORD) dostime) + 1,
                              ((LPWORD) dostime) + 0);
        FindClose(hFind);
        return true;
    }
#elif defined unix || defined __APPLE__ /* || defined __ANDROID__ */
    struct stat sb;
    struct tm *t;

    if (stat(filename.c_str(), &sb) == 0) {
        t = localtime(&sb.st_mtime);

        tmzip->tm_sec  = t->tm_sec;
        tmzip->tm_min  = t->tm_min;
        tmzip->tm_hour = t->tm_hour;
        tmzip->tm_mday = t->tm_mday;
        tmzip->tm_mon  = t->tm_mon ;
        tmzip->tm_year = t->tm_year;

        return true;
    }
#endif
    return false;
}

PatcherError FileUtils::mzAddFile(zipFile zf,
                                  const std::string &name,
                                  const std::vector<unsigned char> &contents)
{
    // Obviously never true, but we'll keep it here just in case
    bool zip64 = (uint64_t) contents.size() >= ((1ull << 32) - 1);

    zip_fileinfo zi;
    memset(&zi, 0, sizeof(zi));

    int ret = zipOpenNewFileInZip2_64(
        zf,                     // file
        name.c_str(),           // filename
        &zi,                    // zip_fileinfo
        nullptr,                // extrafield_local
        0,                      // size_extrafield_local
        nullptr,                // extrafield_global
        0,                      // size_extrafield_global
        nullptr,                // comment
        Z_DEFLATED,             // method
        Z_DEFAULT_COMPRESSION,  // level
        0,                      // raw
        zip64                   // zip64
    );

    if (ret != ZIP_OK) {
        FLOGW("minizip: Failed to add file (error code: %d): [memory]", ret);

        return PatcherError::createArchiveError(
                ErrorCode::ArchiveWriteDataError, name);
    }

    // Write data to file
    ret = zipWriteInFileInZip(zf, contents.data(), contents.size());
    if (ret != ZIP_OK) {
        FLOGW("minizip: Failed to write data (error code: %d): [memory]", ret);
        zipCloseFileInZip(zf);

        return PatcherError::createArchiveError(
                ErrorCode::ArchiveWriteDataError, name);
    }

    zipCloseFileInZip(zf);

    return PatcherError();
}

PatcherError FileUtils::mzAddFile(zipFile zf,
                                  const std::string &name,
                                  const std::string &path)
{
    // Copy file into archive
    io::File file;
    if (!file.open(path, io::File::OpenRead)) {
        FLOGE("%s: Failed to open for reading: %s",
              path.c_str(), file.errorString().c_str());
        return PatcherError::createIOError(ErrorCode::FileOpenError, path);
    }

    uint64_t size;
    file.seek(0, io::File::SeekEnd);
    file.tell(&size);
    file.seek(0, io::File::SeekBegin);

    bool zip64 = size >= ((1ull << 32) - 1);

    zip_fileinfo zi;
    memset(&zi, 0, sizeof(zi));

    mzGetFileTime(path, &zi.tmz_date, &zi.dosDate);

    int ret = zipOpenNewFileInZip2_64(
        zf,                     // file
        name.c_str(),           // filename
        &zi,                    // zip_fileinfo
        nullptr,                // extrafield_local
        0,                      // size_extrafield_local
        nullptr,                // extrafield_global
        0,                      // size_extrafield_global
        nullptr,                // comment
        Z_DEFLATED,             // method
        Z_DEFAULT_COMPRESSION,  // level
        0,                      // raw
        zip64                   // zip64
    );

    if (ret != ZIP_OK) {
        FLOGW("minizip: Failed to add file (error code: %d): %s",
              ret, path.c_str());

        return PatcherError::createArchiveError(
                ErrorCode::ArchiveWriteDataError, name);
    }

    // Write data to file
    char buf[32768];
    uint64_t bytesRead;

    while (file.read(buf, sizeof(buf), &bytesRead)) {
        ret = zipWriteInFileInZip(zf, buf, bytesRead);
        if (ret != ZIP_OK) {
            FLOGW("minizip: Failed to write data (error code: %d): %s",
                  ret, path.c_str());
            zipCloseFileInZip(zf);

            return PatcherError::createArchiveError(
                    ErrorCode::ArchiveWriteDataError, name);
        }
    }
    if (file.error() != io::File::ErrorEndOfFile) {
        zipCloseFileInZip(zf);

        FLOGE("%s: Failed to read file: %s",
              path.c_str(), file.errorString().c_str());
        return PatcherError::createIOError(ErrorCode::FileReadError, path);
    }

    zipCloseFileInZip(zf);

    return PatcherError();
}

}
