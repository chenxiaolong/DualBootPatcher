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

#include "mbp/cwrapper/ccpiofile.h"

#include <cassert>

#include "mbp/cwrapper/private/util.h"

#include "mbp/cpiofile.h"


#define CAST(x) \
    assert(x != nullptr); \
    mbp::CpioFile *cf = reinterpret_cast<mbp::CpioFile *>(x);
#define CCAST(x) \
    assert(x != nullptr); \
    const mbp::CpioFile *cf = reinterpret_cast<const mbp::CpioFile *>(x);


/*!
 * \file ccpiofile.h
 * \brief C Wrapper for CpioFile
 *
 * Please see the documentation for CpioFile from the C++ API for more details.
 * The C functions directly correspond to the CpioFile member functions.
 *
 * \sa CpioFile
 */

extern "C" {

/*!
 * \brief Create a new CCpioFile object.
 *
 * \note The returned object must be freed with mbp_cpiofile_destroy().
 *
 * \return New CCpioFile
 */
CCpioFile * mbp_cpiofile_create(void)
{
    return reinterpret_cast<CCpioFile *>(new mbp::CpioFile());
}

/*!
 * \brief Destroys a CCpioFile object.
 *
 * \param cpio CCpioFile to destroy
 */
void mbp_cpiofile_destroy(CCpioFile *cpio)
{
    CAST(cpio);
    delete cf;
}

/*!
 * \brief Get the error information
 *
 * \note The returned ErrorCode is filled with valid data only if a
 *       CCpioFile operation has failed.
 *
 * \note The returned ErrorCode should be freed with mbp_error_destroy()
 *       when it is no longer needed.
 *
 * \param cpio CCpioFile object
 *
 * \return ErrorCode
 *
 * \sa CpioFile::error()
 */
/* enum ErrorCode */ int mbp_cpiofile_error(const CCpioFile *cpio)
{
    CCAST(cpio);
    return static_cast<int>(cf->error());
}

/*!
 * \brief Load cpio archive from binary data
 *
 * \param cpio CCpioFile object
 * \param data Byte array containing binary data
 * \param size Size of byte array
 *
 * \return true on success or false on failure and error set appropriately
 *
 * \sa CpioFile::load()
 */
bool mbp_cpiofile_load_data(CCpioFile *cpio,
                            const unsigned char *data, size_t size)
{
    CAST(cpio);
    return cf->load(data, size);
}

/*!
 * \brief Constructs the cpio archive
 *
 * \note The output data is dynamically allocated. It should be `free()`'d
 *       when it is no longer needed.
 *
 * \param cpio CCpioFile object
 * \param data Output data
 * \param size Size of output data
 *
 * \return true on success or false on failure and error set appropriately
 *
 * \sa CpioFile::createData()
 */
bool mbp_cpiofile_create_data(CCpioFile *cpio,
                              unsigned char **data, size_t *size)
{
    CAST(cpio);
    std::vector<unsigned char> vData;
    if (cf->createData(&vData)) {
        vector_to_data(vData, reinterpret_cast<void **>(data), size);
        return true;
    } else {
        return false;
    }
}

/*!
 * \brief Check if a file exists in the cpio archive
 *
 * \param cpio CCpioFile object
 * \param filename Filename
 *
 * \return true if file exists, otherwise false
 *
 * \sa CpioFile::exists()
 */
bool mbp_cpiofile_exists(const CCpioFile *cpio, const char *filename)
{
    CCAST(cpio);
    return cf->exists(filename);
}

/*!
 * \brief Remove a file from the cpio archive
 *
 * \param cpio CCpioFile object
 * \param filename File to remove
 *
 * \return true if file was removed, otherwise false
 *
 * \sa CpioFile::remove()
 */
bool mbp_cpiofile_remove(CCpioFile *cpio, const char *filename)
{
    CAST(cpio);
    return cf->remove(filename);
}

/*!
 * \brief List of files in the cpio archive
 *
 * \note The returned array should be freed with `mbp_free_array()` when it
 *       is no longer needed.
 *
 * \return A NULL-terminated array containing the filenames
 *
 * \sa CpioFile::filenames()
 */
char ** mbp_cpiofile_filenames(const CCpioFile *cpio)
{
    CCAST(cpio);
    return vector_to_cstring_array(cf->filenames());
}

bool mbp_cpiofile_is_symlink(const CCpioFile *cpio, const char *filename)
{
    CCAST(cpio);
    return cf->isSymlink(filename);
}

char * mbp_cpiofile_symlink_path(const CCpioFile *cpio, const char *filename)
{
    CCAST(cpio);
    std::string path;
    if (cf->symlinkPath(filename, &path)) {
        return string_to_cstring(path);
    } else {
        return nullptr;
    }
}

/*!
 * \brief Get contents of a file in the archive
 *
 * \param cpio CCpioFile object
 * \param filename Filename
 * \param data Output data
 * \param size Size of output data
 *
 * \return true on success or false on failure and error set appropriately
 *
 * \sa CpioFile::contents()
 */
bool mbp_cpiofile_contents(const CCpioFile *cpio,
                           const char *filename,
                           const unsigned char **data, size_t *size)
{
    CCAST(cpio);
    return cf->contentsC(filename, data, size);
}

/*!
 * \brief Set contents of a file in the archive
 *
 * \param cpio CCpioFile object
 * \param filename Filename
 * \param data Byte array containing binary data
 * \param size Size of byte array
 *
 * \return true on success or false on failure and error set appropriately
 *
 * \sa CpioFile::setContents()
 */
bool mbp_cpiofile_set_contents(CCpioFile *cpio,
                               const char *filename,
                               const unsigned char *data, size_t size)
{
    CAST(cpio);
    return cf->setContentsC(filename, data, size);
}

/*!
 * \brief Add a symbolic link to the archive
 *
 * \param cpio CCpioFile object
 * \param source Source path
 * \param target Target path
 *
 * \return true if the symlink was added, otherwise false and the error set
 *         appropriately
 *
 * \sa CpioFile::addSymlink()
 */
bool mbp_cpiofile_add_symlink(CCpioFile *cpio,
                              const char *source, const char *target)
{
    CAST(cpio);
    return cf->addSymlink(source, target);
}

/*!
 * \brief Add a file to the archive
 *
 * \param cpio CCpioFile object
 * \param path Path to file to add
 * \param name Target path in archive
 * \param perms Octal unix permissions
 *
 * \return true if the file was added, otherwise false and the error set
 *         appropriately
 *
 * \sa CpioFile::addFile(const std::string &, const std::string &, unsigned int)
 */
bool mbp_cpiofile_add_file(CCpioFile *cpio,
                           const char *path, const char *name,
                           unsigned int perms)
{
    CAST(cpio);
    return cf->addFile(path, name, perms);
}

/*!
 * \brief Add a file (from binary data) to the archive
 *
 * \param cpio CCpioFile object
 * \param data Byte array containing binary data
 * \param size Size of byte array
 * \param name Target path in archive
 * \param perms Octal unix permissions
 *
 * \return true if the file was added, otherwise false and the error set
 *         appropriately
 *
 * \sa CpioFile::addFile(std::vector<unsigned char>, const std::string &, unsigned int)
 */
bool mbp_cpiofile_add_file_from_data(CCpioFile *cpio,
                                     const unsigned char *data, size_t size,
                                     const char *name, unsigned int perms)
{
    CAST(cpio);
    return cf->addFileC(data, size, name, perms);
}

}
