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

#include "cwrapper/ccpiofile.h"

#include <cassert>

#include <cwrapper/private/util.h>

#include "cpiofile.h"


#define CAST(x) \
    assert(x != nullptr); \
    CpioFile *cf = reinterpret_cast<CpioFile *>(x);
#define CCAST(x) \
    assert(x != nullptr); \
    const CpioFile *cf = reinterpret_cast<const CpioFile *>(x);


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
    return reinterpret_cast<CCpioFile *>(new CpioFile());
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
 * \note The returned CPatcherError is filled with valid data only if a
 *       CCpioFile operation has failed.
 *
 * \note The returned CPatcherError should be freed with mbp_error_destroy()
 *       when it is no longer needed.
 *
 * \param cpio CCpioFile object
 *
 * \return CPatcherError
 *
 * \sa CpioFile::error()
 */
CPatcherError * mbp_cpiofile_error(const CCpioFile *cpio)
{
    CCAST(cpio);
    PatcherError *pe = new PatcherError(cf->error());
    return reinterpret_cast<CPatcherError *>(pe);
}

/*!
 * \brief Load cpio archive from binary data
 *
 * \param cpio CCpioFile object
 * \param data Byte array containing binary data
 * \param size Size of byte array
 *
 * \return 0 on success or -1 on failure and error set appropriately
 *
 * \sa CpioFile::load()
 */
int mbp_cpiofile_load_data(CCpioFile *cpio,
                           const void *data, size_t size)
{
    CAST(cpio);
    bool ret = cf->load(data_to_vector(data, size));
    return ret ? 0 : -1;
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
 * \return 0 on success or -1 on failure and error set appropriately
 *
 * \sa CpioFile::createData()
 */
int mbp_cpiofile_create_data(CCpioFile *cpio,
                             void **data, size_t *size)
{
    CAST(cpio);
    std::vector<unsigned char> vData;
    if (!cf->createData(&vData)) {
        return -1;
    }

    vector_to_data(vData, data, size);
    return 0;
}

/*!
 * \brief Check if a file exists in the cpio archive
 *
 * \param cpio CCpioFile object
 * \param filename Filename
 *
 * \return 0 if file exists, otherwise -1
 *
 * \sa CpioFile::exists()
 */
int mbp_cpiofile_exists(const CCpioFile *cpio, const char *filename)
{
    CCAST(cpio);
    bool ret = cf->exists(filename);
    return ret ? 0 : -1;
}

/*!
 * \brief Remove a file from the cpio archive
 *
 * \param cpio CCpioFile object
 * \param filename File to remove
 *
 * \return 0 if file was removed, otherwise -1
 *
 * \sa CpioFile::remove()
 */
int mbp_cpiofile_remove(CCpioFile *cpio, const char *filename)
{
    CAST(cpio);
    bool ret = cf->remove(filename);
    return ret ? 0 : -1;
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

/*!
 * \brief Get contents of a file in the archive
 *
 * \note The output data is dynamically allocated. It should be free()'d
 *       when it is no longer needed.
 *
 * \param cpio CCpioFile object
 * \param filename Filename
 * \param data Output data
 * \param size Size of output data
 *
 * \return 0 on success or -1 on failure and error set appropriately
 *
 * \sa CpioFile::contents()
 */
int mbp_cpiofile_contents(const CCpioFile *cpio,
                          const char *filename,
                          void **data, size_t *size)
{
    CCAST(cpio);
    std::vector<unsigned char> vData;
    if (!cf->contents(filename, &vData)) {
        return -1;
    }

    vector_to_data(vData, data, size);
    return 0;
}

/*!
 * \brief Set contents of a file in the archive
 *
 * \param cpio CCpioFile object
 * \param filename Filename
 * \param data Byte array containing binary data
 * \param size Size of byte array
 *
 * \return 0 on success or -1 on failure and error set appropriately
 *
 * \sa CpioFile::setContents()
 */
int mbp_cpiofile_set_contents(CCpioFile *cpio,
                              const char *filename,
                              const void *data, size_t size)
{
    CAST(cpio);
    bool ret = cf->setContents(filename, data_to_vector(data, size));
    return ret ? 0 : -1;
}

/*!
 * \brief Add a symbolic link to the archive
 *
 * \param cpio CCpioFile object
 * \param source Source path
 * \param target Target path
 *
 * \return 0 if the symlink was added, otherwise -1 and the error set
 *         appropriately
 *
 * \sa CpioFile::addSymlink()
 */
int mbp_cpiofile_add_symlink(CCpioFile *cpio,
                             const char *source, const char *target)
{
    CAST(cpio);
    bool ret = cf->addSymlink(source, target);
    return ret ? 0 : -1;
}

/*!
 * \brief Add a file to the archive
 *
 * \param cpio CCpioFile object
 * \param path Path to file to add
 * \param name Target path in archive
 * \param perms Octal unix permissions
 *
 * \return 0 if the file was added, otherwise -1 and the error set
 *         appropriately
 *
 * \sa CpioFile::addFile(const std::string &, const std::string &, unsigned int)
 */
int mbp_cpiofile_add_file(CCpioFile *cpio,
                          const char *path, const char *name,
                          unsigned int perms)
{
    CAST(cpio);
    bool ret = cf->addFile(path, name, perms);
    return ret ? 0 : -1;
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
 * \return 0 if the file was added, otherwise -1 and the error set
 *         appropriately
 *
 * \sa CpioFile::addFile(std::vector<unsigned char>, const std::string &, unsigned int)
 */
int mbp_cpiofile_add_file_from_data(CCpioFile *cpio,
                                    const void *data, size_t size,
                                    const char *name, unsigned int perms)
{
    CAST(cpio);
    bool ret = cf->addFile(data_to_vector(data, size), name, perms);
    return ret ? 0 : -1;
}

}
