/*
 * Copyright (C) 2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbbootimg/reader.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "mbcommon/file.h"
#include "mbcommon/file/standard.h"
#include "mbcommon/string.h"

#include "mbbootimg/entry.h"
#include "mbbootimg/header.h"
#include "mbbootimg/reader_p.h"

/*!
 * \file mbbootimg/reader.h
 * \brief Boot image reader API
 */

/*!
 * \file mbbootimg/reader_p.h
 * \brief Boot image reader private API
 */

/*!
 * \defgroup MB_BI_READER_FORMAT_CALLBACKS Format reader callbacks
 */

/*!
 * \typedef FormatReaderBidder
 * \ingroup MB_BI_READER_FORMAT_CALLBACKS
 *
 * \brief Format reader callback to place bid
 *
 * Place a bid based on the confidence in which the format reader can parse the
 * boot image. The bid is usually the number of bits the reader is confident
 * that conform to the file format (eg. magic string). The file position will be
 * set to the beginning of the file before this function is called.
 *
 * \param bir MbBiReader
 * \param userdata User callback data
 * \param best_bid Current best bid
 *
 * \return
 *   * Return a non-negative integer to place a bid
 *   * Return #RET_WARN to indicate that the bid cannot be won
 *   * Return \<= #RET_FAILED if an error occurs.
 */

/*!
 * \typedef FormatReaderSetOption
 * \ingroup MB_BI_READER_FORMAT_CALLBACKS
 *
 * \brief Format reader callback to set option
 *
 * \param bir MbBiReader
 * \param userdata User callback data
 * \param key Option key
 * \param value Option value
 *
 * \return
 *   * Return #RET_OK if the option is handled successfully
 *   * Return #RET_WARN if the option cannot be handled
 *   * Return \<= #RET_FAILED if an error occurs
 */

/*!
 * \typedef FormatReaderReadHeader
 * \ingroup MB_BI_READER_FORMAT_CALLBACKS
 *
 * \brief Format reader callback to read header
 *
 * \param[in] bir MbBiReader
 * \param[in] userdata User callback data
 * \param[out] header Header instance to write header values
 *
 * \return
 *   * Return #RET_OK if the header is successfully read
 *   * Return \<= #RET_WARN if an error occurs
 */

/*!
 * \typedef FormatReaderReadEntry
 * \ingroup MB_BI_READER_FORMAT_CALLBACKS
 *
 * \brief Format reader callback to read next entry
 *
 * \note This callback *must* be able to skip to the next entry if the user does
 *       not read or finish reading the entry data with reader_read_data().
 *
 * \param[in] bir MbBiReader
 * \param[in] userdata User callback data
 * \param[out] entry Entry instance to write entry values
 *
 * \return
 *   * Return #RET_OK if the entry is successfully read
 *   * Return \<= #RET_WARN if an error occurs
 */

/*!
 * \typedef FormatReaderGoToEntry
 * \ingroup MB_BI_READER_FORMAT_CALLBACKS
 *
 * \brief Format reader callback to seek to a specific entry
 *
 * \param[in] bir MbBiReader
 * \param[in] userdata User callback data
 * \param[out] entry Entry instance to write entry values
 * \param[in] entry_type Entry type to seek to
 *
 * \return
 *   * Return #RET_OK if the entry is successfully read
 *   * Return #RET_EOF if the entry cannot be found
 *   * Return \<= #RET_WARN if an error occurs
 */

/*!
 * \typedef FormatReaderReadData
 * \ingroup MB_BI_READER_FORMAT_CALLBACKS
 *
 * \brief Format reader callback to read entry data
 *
 * \note This function *may* return less than \p buf_size bytes, but *must*
 *       return more than 0 bytes unless EOF is reached.
 *
 * \param[in] bir MbBiReader
 * \param[in] userdata User callback data
 * \param[out] buf Output buffer to write data
 * \param[in] buf_size Size of output buffer
 * \param[out] bytes_read Output number of bytes that are read
 *
 * \return
 *   * Return #RET_OK if the entry is successfully read
 *   * Return #RET_EOF if the end of the curent entry has been reached
 *   * Return \<= #RET_WARN if an error occurs
 */

/*!
 * \typedef FormatReaderFree
 * \ingroup MB_BI_READER_FORMAT_CALLBACKS
 *
 * \brief Format reader callback to clean up resources
 *
 * This function will be called during a call to reader_free(), regardless
 * of the current state. It is guaranteed to only be called once. The function
 * may return any valid status code, but the resources *must* be cleaned up.
 *
 * \param bir MbBiReader
 * \param userdata User callback data
 *
 * \return
 *   * Return #RET_OK if the cleanup completes without error
 *   * Return \<= #RET_WARN if an error occurs during cleanup
 */

///

namespace mb
{
namespace bootimg
{

static struct
{
    int code;
    const char *name;
    int (*func)(MbBiReader *);
} reader_formats[] = {
    {
        FORMAT_ANDROID,
        FORMAT_NAME_ANDROID,
        reader_enable_format_android
    }, {
        FORMAT_BUMP,
        FORMAT_NAME_BUMP,
        reader_enable_format_bump
    }, {
        FORMAT_LOKI,
        FORMAT_NAME_LOKI,
        reader_enable_format_loki
    }, {
        FORMAT_MTK,
        FORMAT_NAME_MTK,
        reader_enable_format_mtk
    }, {
        FORMAT_SONY_ELF,
        FORMAT_NAME_SONY_ELF,
        reader_enable_format_sony_elf
    }, {
        0,
        nullptr,
        nullptr
    },
};

FormatReader::FormatReader(MbBiReader *bir)
    : _bir(bir)
{
}

FormatReader::~FormatReader()
{
}

int FormatReader::init()
{
    return RET_OK;
}

int FormatReader::set_option(const char *key, const char *value)
{
    (void) key;
    (void) value;
    return RET_OK;
}

int FormatReader::go_to_entry(Entry &entry, int entry_type)
{
    (void) entry;
    (void) entry_type;
    return RET_UNSUPPORTED;
}

int _reader_register_format(MbBiReader *bir,
                            std::unique_ptr<FormatReader> format)
{
    READER_ENSURE_STATE(bir, ReaderState::NEW);

    if (bir->formats.size() == MAX_FORMATS) {
        reader_set_error(bir, ERROR_PROGRAMMER_ERROR,
                         "Too many formats enabled");
        return RET_FAILED;
    }

    for (auto const &f : bir->formats) {
        if ((FORMAT_BASE_MASK & f->type())
                == (FORMAT_BASE_MASK & format->type())) {
            reader_set_error(bir, ERROR_PROGRAMMER_ERROR,
                             "%s format (0x%x) already enabled",
                             format->name().c_str(), format->type());
            return RET_WARN;
        }
    }

    bir->formats.push_back(std::move(format));
    return RET_OK;
}

/*!
 * \brief Allocate new MbBiReader.
 *
 * \return New MbBiReader or NULL if memory could not be allocated. If the
 *         function fails, `errno` will be set accordingly.
 */
MbBiReader * reader_new()
{
    MbBiReader *bir = new MbBiReader();
    bir->state = ReaderState::NEW;
    return bir;
}

/*!
 * \brief Free an MbBiReader.
 *
 * If the reader has not been closed, it will be closed and the result of
 * reader_close() will be returned. Otherwise, #RET_OK will be returned.
 * Regardless of the return value, the reader will always be freed and should no
 * longer be used.
 *
 * \param bir MbBiReader
 * \return
 *   * #RET_OK if the reader is successfully freed
 *   * \<= #RET_WARN if an error occurs (though the resources will still be
 *     freed)
 */
int reader_free(MbBiReader *bir)
{
    int ret = RET_OK;

    if (bir) {
        if (bir->state != ReaderState::CLOSED) {
            ret = reader_close(bir);
        }

        delete bir;
    }

    return ret;
}

/*!
 * \brief Open boot image from filename (MBS).
 *
 * \param bir MbBiReader
 * \param filename MBS filename
 *
 * \return
 *   * #RET_OK if the boot image is successfully opened
 *   * \<= #RET_WARN if an error occurs
 */
int reader_open_filename(MbBiReader *bir, const std::string &filename)
{
    READER_ENSURE_STATE(bir, ReaderState::NEW);

    File *file = new StandardFile(filename, FileOpenMode::READ_ONLY);
    if (!file->is_open()) {
        reader_set_error(bir, file->error().value() /* TODO */,
                         "Failed to open for reading: %s",
                         file->error_string().c_str());
        delete file;
        return RET_FAILED;
    }

    return reader_open(bir, file, true);
}

/*!
 * \brief Open boot image from filename (WCS).
 *
 * \param bir MbBiReader
 * \param filename WCS filename
 *
 * \return
 *   * #RET_OK if the boot image is successfully opened
 *   * \<= #RET_WARN if an error occurs
 */
int reader_open_filename_w(MbBiReader *bir, const std::wstring &filename)
{
    READER_ENSURE_STATE(bir, ReaderState::NEW);

    File *file = new StandardFile(filename, FileOpenMode::READ_ONLY);
    if (!file->is_open()) {
        reader_set_error(bir, file->error().value() /* TODO */,
                         "Failed to open for reading: %s",
                         file->error_string().c_str());
        delete file;
        return RET_FAILED;
    }

    return reader_open(bir, file, true);
}

/*!
 * \brief Open boot image from File handle.
 *
 * If \p owned is true, then the File handle will be closed and freed when the
 * MbBiReader is closed and freed. This is true even if this function fails. In
 * other words, if \p owned is true and this function fails, File::close() will
 * be called.
 *
 * \param bir MbBiReader
 * \param file File handle
 * \param owned Whether the MbBiReader should take ownership of the File handle
 *
 * \return
 *   * #RET_OK if the boot image is successfully opened
 *   * \<= #RET_WARN if an error occurs
 */
int reader_open(MbBiReader *bir, File *file, bool owned)
{
    int ret;
    int best_bid = 0;
    bool forced_format = !!bir->format;

    // Ensure that the file is freed even if called in an incorrect state
    READER_ENSURE_STATE_GOTO(bir, ReaderState::NEW, ret, done);

    bir->file = file;
    bir->file_owned = owned;

    if (bir->formats.empty()) {
        reader_set_error(bir, ERROR_PROGRAMMER_ERROR,
                         "No reader formats registered");
        ret = RET_FAILED;
        goto done;
    }

    // Perform bid if a format wasn't explicitly chosen
    if (!bir->format) {
        FormatReader *format = nullptr;

        for (auto &f : bir->formats) {
            // Seek to beginning
            if (!bir->file->seek(0, SEEK_SET, nullptr)) {
                reader_set_error(bir, bir->file->error().value() /* TODO */,
                                 "Failed to seek file: %s",
                                 bir->file->error_string().c_str());
                ret = bir->file->is_fatal() ? RET_FATAL : RET_FAILED;
                goto done;
            }

            // Call bidder
            ret = f->bid(best_bid);
            if (ret > best_bid) {
                best_bid = ret;
                format = f.get();
            } else if (ret == RET_WARN) {
                continue;
            } else if (ret < 0) {
                goto done;
            }
        }

        if (format) {
            bir->format = format;
        } else {
            reader_set_error(bir, ERROR_FILE_FORMAT,
                             "Failed to determine boot image format");
            ret = RET_FAILED;
            goto done;
        }
    }

    bir->state = ReaderState::HEADER;

    ret = RET_OK;

done:
    if (ret != RET_OK) {
        if (owned) {
            delete file;
        }

        bir->file = nullptr;
        bir->file_owned = false;

        if (!forced_format) {
            bir->format = nullptr;
        }
    }
    return ret;
}

/*!
 * \brief Close an MbBiReader.
 *
 * This function will close an MbBiReader if it is open. Regardless of the
 * return value, the reader is closed and can no longer be used for further
 * operations. It should be freed with reader_free().
 *
 * \param bir MbBiReader
 *
 * \return
 *   * #RET_OK if the reader is successfully closed
 *   * \<= #RET_WARN if the reader is opened and an error occurs while
 *     closing the reader
 */
int reader_close(MbBiReader *bir)
{
    int ret = RET_OK;

    // Avoid double-closing or closing nothing
    if (!(bir->state & (ReaderState::CLOSED | ReaderState::NEW))) {
        if (bir->file && bir->file_owned) {
            if (!bir->file->close()) {
                if (RET_FAILED < ret) {
                    ret = RET_FAILED;
                }
            }

            delete bir->file;
        }

        bir->file = nullptr;
        bir->file_owned = false;

        // Don't change state to ReaderState::FATAL if RET_FATAL is returned.
        // Otherwise, we risk double-closing the boot image. CLOSED and FATAL
        // are the same anyway, aside from the fact that boot images can be
        // closed in the latter state.
    }

    bir->state = ReaderState::CLOSED;

    return ret;
}

/*!
 * \brief Read boot image header.
 *
 * Read the header from the boot image and store a reference to the Header
 * in \p header. The value of \p header after a successful call to this function
 * should *never* be deallocated manually. It is tracked internally and will be
 * freed when the MbBiReader is freed.
 *
 * \param[in] bir MbBiReader
 * \param[out] header Pointer to store Header reference
 *
 * \return
 *   * #RET_OK if the boot image header is successfully read
 *   * \<= #RET_WARN if an error occurs
 */
int reader_read_header(MbBiReader *bir, Header *&header)
{
    // State will be checked by reader_read_header2()
    int ret;

    ret = reader_read_header2(bir, bir->header);
    if (ret == RET_OK) {
        header = &bir->header;
    }

    return ret;
}

/*!
 * \brief Read boot image header.
 *
 * Read the header from the boot image and store the header values to a
 * Header instance allocated by the caller. The caller is responsible for
 * deallocating \p header when it is no longer needed.
 *
 * \param[in] bir MbBiReader
 * \param[out] header Pointer to Header for storing header values
 *
 * \return
 *   * #RET_OK if the boot image header is successfully read
 *   * \<= #RET_WARN if an error occurs
 */
int reader_read_header2(MbBiReader *bir, Header &header)
{
    READER_ENSURE_STATE(bir, ReaderState::HEADER);
    int ret;

    // Seek to beginning
    if (!bir->file->seek(0, SEEK_SET, nullptr)) {
        reader_set_error(bir, bir->file->error().value() /* TODO */,
                         "Failed to seek file: %s",
                         bir->file->error_string().c_str());
        return bir->file->is_fatal() ? RET_FATAL : RET_FAILED;
    }

    header.clear();

    ret = bir->format->read_header(header);
    if (ret == RET_OK) {
        bir->state = ReaderState::ENTRY;
    } else if (ret <= RET_FATAL) {
        bir->state = ReaderState::FATAL;
    }

    return ret;
}

/*!
 * \brief Read next boot image entry.
 *
 * Read the next entry from the boot image and store a reference to the
 * Entry in \p entry. The value of \p entry after a successful call to this
 * function should *never* be deallocated manually. It is tracked internally and
 * will be freed when the MbBiReader is freed.
 *
 * \param[in] bir MbBiReader
 * \param[out] entry Pointer to store Entry reference
 *
 * \return
 *   * #RET_OK if the boot image entry is successfully read
 *   * #RET_EOF if the boot image has no more entries
 *   * #RET_UNSUPPORTED if the boot image format does not support seeking
 *   * \<= #RET_WARN if an error occurs
 */
int reader_read_entry(MbBiReader *bir, Entry *&entry)
{
    // State will be checked by reader_read_entry2()
    int ret;

    ret = reader_read_entry2(bir, bir->entry);
    if (ret == RET_OK) {
        entry = &bir->entry;
    }

    return ret;
}

/*!
 * \brief Read next boot image entry.
 *
 * Read the next entry from the boot image and store the entry values to an
 * Entry instance allocated by the caller. The caller is responsible for
 * deallocating \p entry when it is no longer needed.
 *
 * \param[in] bir MbBiReader
 * \param[out] entry Pointer to Entry for storing entry values
 *
 * \return
 *   * #RET_OK if the boot image entry is successfully read
 *   * #RET_EOF if the boot image has no more entries
 *   * #RET_UNSUPPORTED if the boot image format does not support seeking
 *   * \<= #RET_WARN if an error occurs
 */
int reader_read_entry2(MbBiReader *bir, Entry &entry)
{
    // Allow skipping to the next entry without reading the data
    READER_ENSURE_STATE(bir, ReaderState::ENTRY | ReaderState::DATA);
    int ret;

    entry.clear();

    ret = bir->format->read_entry(entry);
    if (ret == RET_OK) {
        bir->state = ReaderState::DATA;
    } else if (ret <= RET_FATAL) {
        bir->state = ReaderState::FATAL;
    }

    return ret;
}

/*!
 * \brief Seek to specific boot image entry.
 *
 * Seek to the specified entry in the boot image and store a reference to the
 * Entry in \p entry. The vlaue of \p entry after a successful call to this
 * function should *never* be deallocated manually. It is tracked internally and
 * will be freed when the MbBiReader is freed.
 *
 * \param[in] bir MbBiReader
 * \param[out] entry Pointer to store Entry reference
 * \param[in] entry_type Entry type to seek to (0 for first entry)
 *
 * \return
 *   * #RET_OK if the boot image entry is successfully read
 *   * #RET_EOF if the boot image entry is not found
 *   * \<= #RET_WARN if an error occurs
 */
int reader_go_to_entry(MbBiReader *bir, Entry *&entry, int entry_type)
{
    // State will be checked by reader_go_to_entry2()
    int ret;

    ret = reader_go_to_entry2(bir, bir->entry, entry_type);
    if (ret == RET_OK) {
        entry = &bir->entry;
    }

    return ret;
}

/*!
 * \brief Seek to specific boot image entry.
 *
 * Seek to the specified entry in the boot image and store the entry values to
 * an Entry instance allocated by the caller. The caller is responsible for
 * deallocating \p entry when it is no longer needed.
 *
 * \param[in] bir MbBiReader
 * \param[out] entry Pointer to Entry for storing entry values
 * \param[in] entry_type Entry type to seek to (0 for first entry)
 *
 * \return
 *   * #RET_OK if the boot image entry is successfully read
 *   * #RET_EOF if the boot image entry is not found
 *   * \<= #RET_WARN if an error occurs
 */
int reader_go_to_entry2(MbBiReader *bir, Entry &entry, int entry_type)
{
    // Allow skipping to an entry without reading the data
    READER_ENSURE_STATE(bir, ReaderState::ENTRY | ReaderState::DATA);
    int ret;

    entry.clear();

    ret = bir->format->go_to_entry(entry, entry_type);
    if (ret == RET_OK) {
        bir->state = ReaderState::DATA;
    } else if (ret <= RET_FATAL) {
        bir->state = ReaderState::FATAL;
    }

    return ret;
}

/*!
 * \brief Read current boot image entry data.
 *
 * \param[in] bir MbBiReader
 * \param[out] buf Output buffer
 * \param[in] size Size of output buffer
 * \param[out] bytes_read Pointer to store number of bytes read
 *
 * \return
 *   * #RET_OK if data is successfully read
 *   * #RET_EOF if EOF is reached for the current entry
 *   * \<= #RET_WARN if an error occurs
 */
int reader_read_data(MbBiReader *bir, void *buf, size_t size,
                     size_t &bytes_read)
{
    READER_ENSURE_STATE(bir, ReaderState::DATA);
    int ret;

    ret = bir->format->read_data(buf, size, bytes_read);
    if (ret == RET_OK) {
        // Do not alter state. Stay in ReaderState::DATA
    } else if (ret <= RET_FATAL) {
        bir->state = ReaderState::FATAL;
    }

    return ret;
}

/*!
 * \brief Get detected or forced boot image format code.
 *
 * * If reader_enable_format_*() was used, then the detected boot image
 *   format code is returned.
 * * If reader_set_format_*() was used, then the forced boot image format
 *   code is returned.
 *
 * \note The return value is meaningful only after the boot image has been
 *       successfully opened. Otherwise, an error will be returned.
 *
 * \param bir MbBiReader
 *
 * \return Boot image format code or -1 if the boot image is not open
 */
int reader_format_code(MbBiReader *bir)
{
    if (!bir->format) {
        reader_set_error(bir, ERROR_PROGRAMMER_ERROR,
                         "No format selected");
        return -1;
    }

    return bir->format->type();
}

/*!
 * \brief Get detected or forced boot image format name.
 *
 * * If reader_enable_format_*() was used, then the detected boot image
 *   format name is returned.
 * * If reader_set_format_*() was used, then the forced boot image format
 *   name is returned.
 *
 * \note The return value is meaningful only after the boot image has been
 *       successfully opened. Otherwise, an error will be returned.
 *
 * \param bir MbBiReader
 *
 * \return Boot image format name or empty string if the boot image is not open
 */
std::string reader_format_name(MbBiReader *bir)
{
    if (!bir->format) {
        reader_set_error(bir, ERROR_PROGRAMMER_ERROR,
                         "No format selected");
        return {};
    }

    return bir->format->name();
}

/*!
 * \brief Force support for a boot image format by its code.
 *
 * Calling this function causes the bidding process to be skipped. The chosen
 * format will be used regardless of which formats are enabled.
 *
 * \param bir MbBiReader
 * \param code Boot image format code (\ref MB_BI_FORMAT_CODES)
 *
 * \return
 *   * #RET_OK if the format is successfully enabled
 *   * \<= #RET_WARN if an error occurs
 */
int reader_set_format_by_code(MbBiReader *bir, int code)
{
    READER_ENSURE_STATE(bir, ReaderState::NEW);
    int ret;
    FormatReader *format = nullptr;

    ret = reader_enable_format_by_code(bir, code);
    if (ret < 0 && ret != RET_WARN) {
        return ret;
    }

    for (auto &f : bir->formats) {
        if ((FORMAT_BASE_MASK & f->type() & code)
                == (FORMAT_BASE_MASK & code)) {
            format = f.get();
            break;
        }
    }

    if (!format) {
        reader_set_error(bir, ERROR_INTERNAL_ERROR,
                         "Enabled format not found");
        bir->state = ReaderState::FATAL;
        return RET_FATAL;
    }

    bir->format = format;

    return RET_OK;
}

/*!
 * \brief Force support for a boot image format by its name.
 *
 * Calling this function causes the bidding process to be skipped. The chosen
 * format will be used regardless of which formats are enabled.
 *
 * \param bir MbBiReader
 * \param name Boot image format name (\ref MB_BI_FORMAT_NAMES)
 *
 * \return
 *   * #RET_OK if the format is successfully set
 *   * \<= #RET_WARN if an error occurs
 */
int reader_set_format_by_name(MbBiReader *bir, const std::string &name)
{
    READER_ENSURE_STATE(bir, ReaderState::NEW);
    int ret;
    FormatReader *format = nullptr;

    ret = reader_enable_format_by_name(bir, name);
    if (ret < 0 && ret != RET_WARN) {
        return ret;
    }

    for (auto &f : bir->formats) {
        if (f->name() == name) {
            format = f.get();
            break;
        }
    }

    if (!format) {
        reader_set_error(bir, ERROR_INTERNAL_ERROR,
                         "Enabled format not found");
        bir->state = ReaderState::FATAL;
        return RET_FATAL;
    }

    bir->format = format;

    return RET_OK;
}

/*!
 * \brief Enable support for all boot image formats.
 *
 * \param bir MbBiReader
 *
 * \return
 *   * #RET_OK if all formats are successfully enabled
 *   * \<= #RET_FAILED if an error occurs while enabling a format
 */
int reader_enable_format_all(MbBiReader *bir)
{
    READER_ENSURE_STATE(bir, ReaderState::NEW);

    for (auto it = reader_formats; it->func; ++it) {
        int ret = it->func(bir);
        if (ret != RET_OK && ret != RET_WARN) {
            return ret;
        }
    }

    return RET_OK;
}

/*!
 * \brief Enable support for a boot image format by its code.
 *
 * \param bir MbBiReader
 * \param code Boot image format code (\ref MB_BI_FORMAT_CODES)
 *
 * \return
 *   * #RET_OK if the format is successfully enabled
 *   * #RET_WARN if the format is already enabled
 *   * \<= #RET_FAILED if an error occurs
 */
int reader_enable_format_by_code(MbBiReader *bir, int code)
{
    READER_ENSURE_STATE(bir, ReaderState::NEW);

    for (auto it = reader_formats; it->func; ++it) {
        if ((code & FORMAT_BASE_MASK) == (it->code & FORMAT_BASE_MASK)) {
            return it->func(bir);
        }
    }

    reader_set_error(bir, ERROR_PROGRAMMER_ERROR,
                     "Invalid format code: %d", code);
    return RET_FAILED;
}

/*!
 * \brief Enable support for a boot image format by its name.
 *
 * \param bir MbBiReader
 * \param name Boot image format name (\ref MB_BI_FORMAT_NAMES)
 *
 * \return
 *   * #RET_OK if the format was successfully enabled
 *   * #RET_WARN if the format is already enabled
 *   * \<= #RET_FAILED if an error occurs
 */
int reader_enable_format_by_name(MbBiReader *bir, const std::string &name)
{
    READER_ENSURE_STATE(bir, ReaderState::NEW);

    for (auto it = reader_formats; it->func; ++it) {
        if (name == it->name) {
            return it->func(bir);
        }
    }

    reader_set_error(bir, ERROR_PROGRAMMER_ERROR,
                     "Invalid format name: %s", name.c_str());
    return RET_FAILED;
}

/*!
 * \brief Get error code for a failed operation.
 *
 * \note The return value is undefined if an operation did not fail.
 *
 * \param bir MbBiReader
 *
 * \return Error code for failed operation. If \>= 0, then the value is one of
 *         the \ref MB_BI_ERROR_CODES entries. If \< 0, then the error code is
 *         implementation-defined (usually `-errno` or `-GetLastError()`).
 */
int reader_error(MbBiReader *bir)
{
    return bir->error_code;
}

/*!
 * \brief Get error string for a failed operation.
 *
 * \note The return value is undefined if an operation did not fail.
 *
 * \param bir MbBiReader
 *
 * \return Error string for failed operation. The string contents may be
 *         undefined, but will never be NULL or an invalid string.
 */
const char * reader_error_string(MbBiReader *bir)
{
    return bir->error_string.c_str();
}

/*!
 * \brief Set error string for a failed operation.
 *
 * \sa reader_set_error_v()
 *
 * \param bir MbBiReader
 * \param error_code Error code
 * \param fmt `printf()`-style format string
 * \param ... `printf()`-style format arguments
 *
 * \return RET_OK if the error is successfully set or RET_FAILED if an
 *         error occurs
 */
int reader_set_error(MbBiReader *bir, int error_code, const char *fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = reader_set_error_v(bir, error_code, fmt, ap);
    va_end(ap);

    return ret;
}

/*!
 * \brief Set error string for a failed operation.
 *
 * \sa reader_set_error()
 *
 * \param bir MbBiReader
 * \param error_code Error code
 * \param fmt `printf()`-style format string
 * \param ap `printf()`-style format arguments as a va_list
 *
 * \return RET_OK if the error is successfully set or RET_FAILED if an
 *         error occurs
 */
int reader_set_error_v(MbBiReader *bir, int error_code,
                       const char *fmt, va_list ap)
{
    bir->error_code = error_code;
    return format_v(bir->error_string, fmt, ap)
            ? RET_OK : RET_FAILED;
}

}
}
