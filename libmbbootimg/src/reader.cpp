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
#include "mbcommon/file/filename.h"
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
 *   * Return #MB_BI_WARN to indicate that the bid cannot be won
 *   * Return \<= #MB_BI_FAILED if an error occurs.
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
 *   * Return #MB_BI_OK if the option is handled successfully
 *   * Return #MB_BI_WARN if the option cannot be handled
 *   * Return \<= #MB_BI_FAILED if an error occurs
 */

/*!
 * \typedef FormatReaderReadHeader
 * \ingroup MB_BI_READER_FORMAT_CALLBACKS
 *
 * \brief Format reader callback to read header
 *
 * \param[in] bir MbBiReader
 * \param[in] userdata User callback data
 * \param[out] header MbBiHeader instance to write header values
 *
 * \return
 *   * Return #MB_BI_OK if the header is successfully read
 *   * Return \<= #MB_BI_WARN if an error occurs
 */

/*!
 * \typedef FormatReaderReadEntry
 * \ingroup MB_BI_READER_FORMAT_CALLBACKS
 *
 * \brief Format reader callback to read next entry
 *
 * \note This callback *must* be able to skip to the next entry if the user does
 *       not read or finish reading the entry data with
 *       mb_bi_reader_read_data().
 *
 * \param[in] bir MbBiReader
 * \param[in] userdata User callback data
 * \param[out] entry MbBiEntry instance to write entry values
 *
 * \return
 *   * Return #MB_BI_OK if the entry is successfully read
 *   * Return \<= #MB_BI_WARN if an error occurs
 */

/*!
 * \typedef FormatReaderGoToEntry
 * \ingroup MB_BI_READER_FORMAT_CALLBACKS
 *
 * \brief Format reader callback to seek to a specific entry
 *
 * \param[in] bir MbBiReader
 * \param[in] userdata User callback data
 * \param[out] entry MbBiEntry instance to write entry values
 * \param[in] entry_type Entry type to seek to
 *
 * \return
 *   * Return #MB_BI_OK if the entry is successfully read
 *   * Return #MB_BI_EOF if the entry cannot be found
 *   * Return \<= #MB_BI_WARN if an error occurs
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
 *   * Return #MB_BI_OK if the entry is successfully read
 *   * Return #MB_BI_EOF if the end of the curent entry has been reached
 *   * Return \<= #MB_BI_WARN if an error occurs
 */

/*!
 * \typedef FormatReaderFree
 * \ingroup MB_BI_READER_FORMAT_CALLBACKS
 *
 * \brief Format reader callback to clean up resources
 *
 * This function will be called during a call to mb_bi_reader_free(), regardless
 * of the current state. It is guaranteed to only be called once. The function
 * may return any valid status code, but the resources *must* be cleaned up.
 *
 * \param bir MbBiReader
 * \param userdata User callback data
 *
 * \return
 *   * Return #MB_BI_OK if the cleanup completes without error
 *   * Return \<= #MB_BI_WARN if an error occurs during cleanup
 */

///

MB_BEGIN_C_DECLS

static struct
{
    int code;
    const char *name;
    int (*func)(MbBiReader *);
} reader_formats[] = {
    {
        MB_BI_FORMAT_ANDROID,
        MB_BI_FORMAT_NAME_ANDROID,
        mb_bi_reader_enable_format_android
    }, {
        MB_BI_FORMAT_BUMP,
        MB_BI_FORMAT_NAME_BUMP,
        mb_bi_reader_enable_format_bump
    }, {
        MB_BI_FORMAT_LOKI,
        MB_BI_FORMAT_NAME_LOKI,
        mb_bi_reader_enable_format_loki
    }, {
        MB_BI_FORMAT_MTK,
        MB_BI_FORMAT_NAME_MTK,
        mb_bi_reader_enable_format_mtk
    }, {
        MB_BI_FORMAT_SONY_ELF,
        MB_BI_FORMAT_NAME_SONY_ELF,
        mb_bi_reader_enable_format_sony_elf
    }, {
        0,
        nullptr,
        nullptr
    },
};

/*!
 * \brief Register a format reader
 *
 * Register a format reader with an MbBiReader. If the function fails,
 * \p free_cb will be called to clean up the resources.
 *
 * \note The \p bidder_cb callback is optional. If a bidder callback is not
 *       provided, then the format can only be used if explicitly forced via
 *       mb_bi_reader_set_format_by_code() or mb_bi_reader_set_format_by_name().
 *
 * \param bir MbBiReader
 * \param userdata User callback data
 * \param type Format type (must be one of \ref MB_BI_FORMAT_CODES)
 * \param name Format name (must be one of \ref MB_BI_FORMAT_NAMES)
 * \param bidder_cb Bidder callback (optional)
 * \param set_option_cb Set option callback (optional)
 * \param read_header_cb Read header callback (required)
 * \param read_entry_cb Read entry callback (required)
 * \param go_to_entry_cb Go to entry callback (optional)
 * \param read_data_cb Read data callback (required)
 * \param free_cb Free callback (optional)
 *
 * \return
 *   * #MB_BI_OK if the format is successfully registered
 *   * #MB_BI_WARN if the format has already been registered before
 *   * \<= #MB_BI_FAILED if an error occurs
 */
int _mb_bi_reader_register_format(MbBiReader *bir,
                                  void *userdata,
                                  int type,
                                  const char *name,
                                  FormatReaderBidder bidder_cb,
                                  FormatReaderSetOption set_option_cb,
                                  FormatReaderReadHeader read_header_cb,
                                  FormatReaderReadEntry read_entry_cb,
                                  FormatReaderGoToEntry go_to_entry_cb,
                                  FormatReaderReadData read_data_cb,
                                  FormatReaderFree free_cb)
{
    int ret;
    FormatReader format;

    // Check state, but ensure that format is freed if this function is called
    // in an incorect state
    READER_ENSURE_STATE_GOTO(bir, ReaderState::NEW, ret, done);

    format.type = type;
    format.name = strdup(name);
    format.bidder_cb = bidder_cb;
    format.set_option_cb = set_option_cb;
    format.read_header_cb = read_header_cb;
    format.read_entry_cb = read_entry_cb;
    format.go_to_entry_cb = go_to_entry_cb;
    format.read_data_cb = read_data_cb;
    format.free_cb = free_cb;
    format.userdata = userdata;

    if (!format.name) {
        mb_bi_reader_set_error(bir, -errno, "%s", strerror(errno));
        ret = MB_BI_FAILED;
        goto done;
    }

    if (bir->formats_len == MAX_FORMATS) {
        mb_bi_reader_set_error(bir, MB_BI_ERROR_PROGRAMMER_ERROR,
                               "Too many formats enabled");
        ret = MB_BI_FAILED;
        goto done;
    }

    for (size_t i = 0; i < bir->formats_len; ++i) {
        if ((MB_BI_FORMAT_BASE_MASK & bir->formats[i].type)
                == (MB_BI_FORMAT_BASE_MASK & type)) {
            mb_bi_reader_set_error(bir, MB_BI_ERROR_PROGRAMMER_ERROR,
                                   "%s format (0x%x) already enabled",
                                   name, type);
            ret = MB_BI_WARN;
            goto done;
        }
    }

    bir->formats[bir->formats_len] = format;
    ++bir->formats_len;

    ret = MB_BI_OK;

done:
    if (ret != MB_BI_OK) {
        int ret2 = _mb_bi_reader_free_format(bir, &format);
        if (ret2 < ret) {
            ret = ret2;
        }
    }

    return ret;
}

/*!
 * \brief Free a format reader
 *
 * \note Regardless of the return value, the format reader's resources will have
 *       been cleaned up. The return value only exists to indicate if an error
 *       occurs in the process of cleaning up.
 *
 * \note If \p format is allocated on the heap, it is up to the caller to free
 *       its memory.
 *
 * \param bir MbBiReader
 * \param format Format reader to clean up
 *
 * \return
 *   * #MB_BI_OK if the resources for \p format are successfully freed
 *   * \<= #MB_BI_WARN if an error occurs (though the resources will still be
 *     cleaned up)
 */
int _mb_bi_reader_free_format(MbBiReader *bir, FormatReader *format)
{
    READER_ENSURE_STATE(bir, ReaderState::ANY);
    int ret = MB_BI_OK;

    if (format) {
        if (format->free_cb) {
            ret = format->free_cb(bir, format->userdata);
        }

        free(format->name);
    }

    return ret;
}

/*!
 * \brief Allocate new MbBiReader.
 *
 * \return New MbBiReader or NULL if memory could not be allocated. If the
 *         function fails, `errno` will be set accordingly.
 */
MbBiReader * mb_bi_reader_new()
{
    MbBiReader *bir = static_cast<MbBiReader *>(calloc(1, sizeof(MbBiReader)));
    if (bir) {
        bir->state = ReaderState::NEW;
        bir->header = mb_bi_header_new();
        bir->entry = mb_bi_entry_new();

        if (!bir->header || !bir->entry) {
            mb_bi_header_free(bir->header);
            mb_bi_entry_free(bir->entry);
            free(bir);
            bir = nullptr;
        }
    }
    return bir;
}

/*!
 * \brief Free an MbBiReader.
 *
 * If the reader has not been closed, it will be closed and the result of
 * mb_bi_reader_close() will be returned. Otherwise, #MB_BI_OK will be returned.
 * Regardless of the return value, the reader will always be freed and should no
 * longer be used.
 *
 * \param bir MbBiReader
 * \return
 *   * #MB_BI_OK if the reader is successfully freed
 *   * \<= #MB_BI_WARN if an error occurs (though the resources will still be
 *     freed)
 */
int mb_bi_reader_free(MbBiReader *bir)
{
    int ret = MB_BI_OK, ret2;

    if (bir) {
        if (bir->state != ReaderState::CLOSED) {
            ret = mb_bi_reader_close(bir);
        }

        for (size_t i = 0; i < bir->formats_len; ++i) {
            ret2 = _mb_bi_reader_free_format(bir, &bir->formats[i]);
            if (ret2 < ret) {
                ret = ret2;
            }
        }

        mb_bi_header_free(bir->header);
        mb_bi_entry_free(bir->entry);

        free(bir->error_string);
        free(bir);
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
 *   * #MB_BI_OK if the boot image is successfully opened
 *   * \<= #MB_BI_WARN if an error occurs
 */
int mb_bi_reader_open_filename(MbBiReader *bir, const char *filename)
{
    READER_ENSURE_STATE(bir, ReaderState::NEW);
    int ret;

    MbFile *file = mb_file_new();
    if (!file) {
        mb_bi_reader_set_error(bir, MB_BI_ERROR_INTERNAL_ERROR,
                               "%s", strerror(errno));
        return MB_BI_FAILED;
    }

    ret = mb_file_open_filename(file, filename, MB_FILE_OPEN_READ_ONLY);
    if (ret != MB_FILE_OK) {
        // Always return MB_BI_FAILED as MB_FILE_FATAL would not affect us
        // at this point
        mb_bi_reader_set_error(bir, mb_file_error(file),
                               "Failed to open for reading: %s",
                               mb_file_error_string(file));
        mb_file_free(file);
        return MB_BI_FAILED;
    }

    return mb_bi_reader_open(bir, file, true);
}

/*!
 * \brief Open boot image from filename (WCS).
 *
 * \param bir MbBiReader
 * \param filename WCS filename
 *
 * \return
 *   * #MB_BI_OK if the boot image is successfully opened
 *   * \<= #MB_BI_WARN if an error occurs
 */
int mb_bi_reader_open_filename_w(MbBiReader *bir, const wchar_t *filename)
{
    READER_ENSURE_STATE(bir, ReaderState::NEW);
    int ret;

    MbFile *file = mb_file_new();
    if (!file) {
        mb_bi_reader_set_error(bir, MB_BI_ERROR_INTERNAL_ERROR,
                               "%s", strerror(errno));
        return MB_BI_FAILED;
    }

    ret = mb_file_open_filename_w(file, filename, MB_FILE_OPEN_READ_ONLY);
    if (ret != MB_FILE_OK) {
        // Always return MB_BI_FAILED as MB_FILE_FATAL would not affect us
        // at this point
        mb_bi_reader_set_error(bir, mb_file_error(file),
                               "Failed to open for reading: %s",
                               mb_file_error_string(file));
        mb_file_free(file);
        return MB_BI_FAILED;
    }

    return mb_bi_reader_open(bir, file, true);
}

/*!
 * \brief Open boot image from MbFile handle.
 *
 * If \p owned is true, then the MbFile handle will be closed and freed when the
 * MbBiReader is closed and freed. This is true even if this function fails. In
 * other words, if \p owned is true and this function fails, mb_file_free() will
 * be called.
 *
 * \param bir MbBiReader
 * \param file MbFile handle
 * \param owned Whether the MbBiReader should take ownership of the MbFile
 *              handle
 *
 * \return
 *   * #MB_BI_OK if the boot image is successfully opened
 *   * \<= #MB_BI_WARN if an error occurs
 */
int mb_bi_reader_open(MbBiReader *bir, MbFile *file, bool owned)
{
    int ret;
    int best_bid = 0;
    bool forced_format = !!bir->format;

    // Ensure that the file is freed even if called in an incorrect state
    READER_ENSURE_STATE_GOTO(bir, ReaderState::NEW, ret, done);

    bir->file = file;
    bir->file_owned = owned;

    if (bir->formats_len == 0) {
        mb_bi_reader_set_error(bir, MB_BI_ERROR_PROGRAMMER_ERROR,
                               "No reader formats registered");
        ret = MB_BI_FAILED;
        goto done;
    }

    // Perform bid if a format wasn't explicitly chosen
    if (!bir->format) {
        FormatReader *format = nullptr, *cur;

        for (size_t i = 0; i < bir->formats_len; ++i) {
            cur = &bir->formats[i];

            if (cur->bidder_cb) {
                // Seek to beginning
                ret = mb_file_seek(bir->file, 0, SEEK_SET, nullptr);
                if (ret < 0) {
                    mb_bi_reader_set_error(bir, mb_file_error(bir->file),
                                           "Failed to seek file: %s",
                                           mb_file_error_string(bir->file));
                    goto done;
                }

                // Call bidder
                ret = cur->bidder_cb(bir, cur->userdata, best_bid);
                if (ret > best_bid) {
                    best_bid = ret;
                    format = cur;
                } else if (ret == MB_BI_WARN) {
                    continue;
                } else if (ret < 0) {
                    goto done;
                }
            }
        }

        if (format) {
            bir->format = format;
        } else {
            mb_bi_reader_set_error(bir, MB_BI_ERROR_FILE_FORMAT,
                                   "Failed to determine boot image format");
            ret = MB_BI_FAILED;
            goto done;
        }
    }

    bir->state = ReaderState::HEADER;

    ret = MB_BI_OK;

done:
    if (ret != MB_BI_OK) {
        if (owned) {
            mb_file_free(file);
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
 * operations. It should be freed with mb_bi_reader_free().
 *
 * \param bir MbBiReader
 *
 * \return
 *   * #MB_BI_OK if the reader is successfully closed
 *   * \<= #MB_BI_WARN if the reader is opened and an error occurs while
 *     closing the reader
 */
int mb_bi_reader_close(MbBiReader *bir)
{
    int ret = MB_BI_OK, ret2;

    // Avoid double-closing or closing nothing
    if (!(bir->state & (ReaderState::CLOSED | ReaderState::NEW))) {
        if (bir->file && bir->file_owned) {
            ret2 = mb_file_free(bir->file);
            if (ret2 < 0) {
                ret2 = ret2 == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
            }
            if (ret2 < ret) {
                ret = ret2;
            }
        }

        bir->file = nullptr;
        bir->file_owned = false;

        // Don't change state to ReaderState::FATAL if MB_BI_FATAL is returned.
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
 * Read the header from the boot image and store a reference to the MbBiHeader
 * in \p header. The value of \p header after a successful call to this function
 * should *never* be deallocated with mb_bi_header_free(). It is tracked
 * internally and will be freed when the MbBiReader is freed.
 *
 * \param[in] bir MbBiReader
 * \param[out] header Pointer to store MbBiHeader reference
 *
 * \return
 *   * #MB_BI_OK if the boot image header is successfully read
 *   * \<= #MB_BI_WARN if an error occurs
 */
int mb_bi_reader_read_header(MbBiReader *bir, MbBiHeader **header)
{
    // State will be checked by mb_bi_reader_read_header2()
    int ret;

    ret = mb_bi_reader_read_header2(bir, bir->header);
    if (ret == MB_BI_OK) {
        *header = bir->header;
    }

    return ret;
}

/*!
 * \brief Read boot image header.
 *
 * Read the header from the boot image and store the header values to an
 * MbBiHeader instance allocated by the caller. The caller is responsible for
 * deallocating \p header when it is no longer needed.
 *
 * \param[in] bir MbBiReader
 * \param[out] header Pointer to MbBiHeader for storing header values
 *
 * \return
 *   * #MB_BI_OK if the boot image header is successfully read
 *   * \<= #MB_BI_WARN if an error occurs
 */
int mb_bi_reader_read_header2(MbBiReader *bir, MbBiHeader *header)
{
    READER_ENSURE_STATE(bir, ReaderState::HEADER);
    int ret;

    // Seek to beginning
    ret = mb_file_seek(bir->file, 0, SEEK_SET, nullptr);
    if (ret < 0) {
        mb_bi_reader_set_error(bir, mb_file_error(bir->file),
                               "Failed to seek file: %s",
                               mb_file_error_string(bir->file));
        return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
    }

    mb_bi_header_clear(header);

    if (!bir->format->read_header_cb) {
        mb_bi_reader_set_error(bir, MB_BI_ERROR_INTERNAL_ERROR,
                               "Missing format read_header_cb");
        bir->state = ReaderState::FATAL;
        return MB_BI_FATAL;
    }

    ret = bir->format->read_header_cb(bir, bir->format->userdata, header);
    if (ret == MB_BI_OK) {
        bir->state = ReaderState::ENTRY;
    } else if (ret <= MB_BI_FATAL) {
        bir->state = ReaderState::FATAL;
    }

    return ret;
}

/*!
 * \brief Read next boot image entry.
 *
 * Read the next entry from the boot image and store a reference to the
 * MbBiEntry in \p entry. The value of \p entry after a successful call to this
 * function should *never* be deallocated with mb_bi_entry_free(). It is tracked
 * internally and will be freed when the MbBiReader is freed.
 *
 * \param[in] bir MbBiReader
 * \param[out] entry Pointer to store MbBiEntry reference
 *
 * \return
 *   * #MB_BI_OK if the boot image entry is successfully read
 *   * #MB_BI_EOF if the boot image has no more entries
 *   * #MB_BI_UNSUPPORTED if the boot image format does not support seeking
 *   * \<= #MB_BI_WARN if an error occurs
 */
int mb_bi_reader_read_entry(MbBiReader *bir, MbBiEntry **entry)
{
    // State will be checked by mb_bi_reader_read_entry2()
    int ret;

    ret = mb_bi_reader_read_entry2(bir, bir->entry);
    if (ret == MB_BI_OK) {
        *entry = bir->entry;
    }

    return ret;
}

/*!
 * \brief Read next boot image entry.
 *
 * Read the next entry from the boot image and store the entry values to an
 * MbBiEntry instance allocated by the caller. The caller is responsible for
 * deallocating \p entry when it is no longer needed.
 *
 * \param[in] bir MbBiReader
 * \param[out] entry Pointer to MbBiEntry for storing entry values
 *
 * \return
 *   * #MB_BI_OK if the boot image entry is successfully read
 *   * #MB_BI_EOF if the boot image has no more entries
 *   * #MB_BI_UNSUPPORTED if the boot image format does not support seeking
 *   * \<= #MB_BI_WARN if an error occurs
 */
int mb_bi_reader_read_entry2(MbBiReader *bir, MbBiEntry *entry)
{
    // Allow skipping to the next entry without reading the data
    READER_ENSURE_STATE(bir, ReaderState::ENTRY | ReaderState::DATA);
    int ret;

    mb_bi_entry_clear(entry);

    if (!bir->format->read_entry_cb) {
        mb_bi_reader_set_error(bir, MB_BI_ERROR_INTERNAL_ERROR,
                               "Missing format read_entry_cb");
        bir->state = ReaderState::FATAL;
        return MB_BI_FATAL;
    }

    ret = bir->format->read_entry_cb(bir, bir->format->userdata, entry);
    if (ret == MB_BI_OK) {
        bir->state = ReaderState::DATA;
    } else if (ret <= MB_BI_FATAL) {
        bir->state = ReaderState::FATAL;
    }

    return ret;
}

/*!
 * \brief Seek to specific boot image entry.
 *
 * Seek to the specified entry in the boot image and store a reference to the
 * MbBiEntry in \p entry. The vlaue of \p entry after a successful call to this
 * function should *never* be deallocated with mb_bi_entry_free(). It is tracked
 * internally and will be freed when the MbBiReader is freed.
 *
 * \param[in] bir MbBiReader
 * \param[out] entry Pointer to store MbBiEntry reference
 * \param[in] entry_type Entry type to seek to (0 for first entry)
 *
 * \return
 *   * #MB_BI_OK if the boot image entry is successfully read
 *   * #MB_BI_EOF if the boot image entry is not found
 *   * \<= #MB_BI_WARN if an error occurs
 */
int mb_bi_reader_go_to_entry(MbBiReader *bir, MbBiEntry **entry, int entry_type)
{
    // State will be checked by mb_bi_reader_go_to_entry2()
    int ret;

    ret = mb_bi_reader_go_to_entry2(bir, bir->entry, entry_type);
    if (ret == MB_BI_OK) {
        *entry = bir->entry;
    }

    return ret;
}

/*!
 * \brief Seek to specific boot image entry.
 *
 * Seek to the specified entry in the boot image and store the entry values to
 * an MbBiEntry instance allocated by the caller. The caller is responsible for
 * deallocating \p entry when it is no longer needed.
 *
 * \param[in] bir MbBiReader
 * \param[out] entry Pointer to MbBiEntry for storing entry values
 * \param[in] entry_type Entry type to seek to (0 for first entry)
 *
 * \return
 *   * #MB_BI_OK if the boot image entry is successfully read
 *   * #MB_BI_EOF if the boot image entry is not found
 *   * \<= #MB_BI_WARN if an error occurs
 */
int mb_bi_reader_go_to_entry2(MbBiReader *bir, MbBiEntry *entry, int entry_type)
{
    // Allow skipping to an entry without reading the data
    READER_ENSURE_STATE(bir, ReaderState::ENTRY | ReaderState::DATA);
    int ret;

    mb_bi_entry_clear(entry);

    if (!bir->format->go_to_entry_cb) {
        mb_bi_reader_set_error(bir, MB_BI_ERROR_UNSUPPORTED,
                               "go_to_entry_cb not defined");
        return MB_BI_UNSUPPORTED;
    }

    ret = bir->format->go_to_entry_cb(bir, bir->format->userdata, entry,
                                      entry_type);
    if (ret == MB_BI_OK) {
        bir->state = ReaderState::DATA;
    } else if (ret <= MB_BI_FATAL) {
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
 *   * #MB_BI_OK if data is successfully read
 *   * #MB_BI_EOF if EOF is reached for the current entry
 *   * \<= #MB_BI_WARN if an error occurs
 */
int mb_bi_reader_read_data(MbBiReader *bir, void *buf, size_t size,
                           size_t *bytes_read)
{
    READER_ENSURE_STATE(bir, ReaderState::DATA);
    int ret;

    if (!bir->format->read_data_cb) {
        mb_bi_reader_set_error(bir, MB_BI_ERROR_INTERNAL_ERROR,
                               "Missing format read_data_cb");
        bir->state = ReaderState::FATAL;
        return MB_BI_FATAL;
    }

    ret = bir->format->read_data_cb(bir, bir->format->userdata, buf, size,
                                    bytes_read);
    if (ret == MB_BI_OK) {
        // Do not alter state. Stay in ReaderState::DATA
    } else if (ret <= MB_BI_FATAL) {
        bir->state = ReaderState::FATAL;
    }

    return ret;
}

/*!
 * \brief Get detected or forced boot image format code.
 *
 * * If mb_bi_reader_enable_format_*() was used, then the detected boot image
 *   format code is returned.
 * * If mb_bi_reader_set_format_*() was used, then the forced boot image format
 *   code is returned.
 *
 * \note The return value is meaningful only after the boot image has been
 *       successfully opened. Otherwise, an error will be returned.
 *
 * \param bir MbBiReader
 *
 * \return Boot image format code or -1 if the boot image is not open
 */
int mb_bi_reader_format_code(MbBiReader *bir)
{
    if (!bir->format) {
        mb_bi_reader_set_error(bir, MB_BI_ERROR_PROGRAMMER_ERROR,
                               "No format selected");
        return -1;
    }

    return bir->format->type;
}

/*!
 * \brief Get detected or forced boot image format name.
 *
 * * If mb_bi_reader_enable_format_*() was used, then the detected boot image
 *   format name is returned.
 * * If mb_bi_reader_set_format_*() was used, then the forced boot image format
 *   name is returned.
 *
 * \note The return value is meaningful only after the boot image has been
 *       successfully opened. Otherwise, an error will be returned.
 *
 * \param bir MbBiReader
 *
 * \return Boot image format name or NULL if the boot image is not open
 */
const char * mb_bi_reader_format_name(MbBiReader *bir)
{
    if (!bir->format) {
        mb_bi_reader_set_error(bir, MB_BI_ERROR_PROGRAMMER_ERROR,
                               "No format selected");
        return NULL;
    }

    return bir->format->name;
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
 *   * #MB_BI_OK if the format is successfully enabled
 *   * \<= #MB_BI_WARN if an error occurs
 */
int mb_bi_reader_set_format_by_code(MbBiReader *bir, int code)
{
    READER_ENSURE_STATE(bir, ReaderState::NEW);
    int ret;
    FormatReader *format = nullptr;

    ret = mb_bi_reader_enable_format_by_code(bir, code);
    if (ret < 0 && ret != MB_BI_WARN) {
        return ret;
    }

    for (size_t i = 0; i < bir->formats_len; ++i) {
        if ((MB_BI_FORMAT_BASE_MASK & bir->formats[i].type & code)
                == (MB_BI_FORMAT_BASE_MASK & code)) {
            format = &bir->formats[i];
            break;
        }
    }

    if (!format) {
        mb_bi_reader_set_error(bir, MB_BI_ERROR_INTERNAL_ERROR,
                               "Enabled format not found");
        bir->state = ReaderState::FATAL;
        return MB_BI_FATAL;
    }

    bir->format = format;

    return MB_BI_OK;
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
 *   * #MB_BI_OK if the format is successfully set
 *   * \<= #MB_BI_WARN if an error occurs
 */
int mb_bi_reader_set_format_by_name(MbBiReader *bir, const char *name)
{
    READER_ENSURE_STATE(bir, ReaderState::NEW);
    int ret;
    FormatReader *format = nullptr;

    ret = mb_bi_reader_enable_format_by_name(bir, name);
    if (ret < 0 && ret != MB_BI_WARN) {
        return ret;
    }

    for (size_t i = 0; i < bir->formats_len; ++i) {
        if (strcmp(name, bir->formats[i].name) == 0) {
            format = &bir->formats[i];
            break;
        }
    }

    if (!format) {
        mb_bi_reader_set_error(bir, MB_BI_ERROR_INTERNAL_ERROR,
                               "Enabled format not found");
        bir->state = ReaderState::FATAL;
        return MB_BI_FATAL;
    }

    bir->format = format;

    return MB_BI_OK;
}

/*!
 * \brief Enable support for all boot image formats.
 *
 * \param bir MbBiReader
 *
 * \return
 *   * #MB_BI_OK if all formats are successfully enabled
 *   * \<= #MB_BI_FAILED if an error occurs while enabling a format
 */
int mb_bi_reader_enable_format_all(MbBiReader *bir)
{
    READER_ENSURE_STATE(bir, ReaderState::NEW);

    for (auto it = reader_formats; it->func; ++it) {
        int ret = it->func(bir);
        if (ret != MB_BI_OK && ret != MB_BI_WARN) {
            return ret;
        }
    }

    return MB_BI_OK;
}

/*!
 * \brief Enable support for a boot image format by its code.
 *
 * \param bir MbBiReader
 * \param code Boot image format code (\ref MB_BI_FORMAT_CODES)
 *
 * \return
 *   * #MB_BI_OK if the format is successfully enabled
 *   * #MB_BI_WARN if the format is already enabled
 *   * \<= #MB_BI_FAILED if an error occurs
 */
int mb_bi_reader_enable_format_by_code(MbBiReader *bir, int code)
{
    READER_ENSURE_STATE(bir, ReaderState::NEW);

    for (auto it = reader_formats; it->func; ++it) {
        if ((code & MB_BI_FORMAT_BASE_MASK)
                == (it->code & MB_BI_FORMAT_BASE_MASK)) {
            return it->func(bir);
        }
    }

    mb_bi_reader_set_error(bir, MB_BI_ERROR_PROGRAMMER_ERROR,
                           "Invalid format code: %d", code);
    return MB_BI_FAILED;
}

/*!
 * \brief Enable support for a boot image format by its name.
 *
 * \param bir MbBiReader
 * \param name Boot image format name (\ref MB_BI_FORMAT_NAMES)
 *
 * \return
 *   * #MB_BI_OK if the format was successfully enabled
 *   * #MB_BI_WARN if the format is already enabled
 *   * \<= #MB_BI_FAILED if an error occurs
 */
int mb_bi_reader_enable_format_by_name(MbBiReader *bir, const char *name)
{
    READER_ENSURE_STATE(bir, ReaderState::NEW);

    for (auto it = reader_formats; it->func; ++it) {
        if (strcmp(name, it->name) == 0) {
            return it->func(bir);
        }
    }

    mb_bi_reader_set_error(bir, MB_BI_ERROR_PROGRAMMER_ERROR,
                           "Invalid format name: %s", name);
    return MB_BI_FAILED;
}

/*!
 * \brief Get error code for a failed operation.
 *
 * \note The return value is undefined if an operation did not fail.
 *
 * \param bir MbBiReader
 *
 * \return Error code for failed operation. If \>= 0, then the value is one of
 *         the MB_BI_* entries. If \< 0, then the error code is
 *         implementation-defined (usually `-errno` or `-GetLastError()`).
 */
int mb_bi_reader_error(MbBiReader *bir)
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
const char * mb_bi_reader_error_string(MbBiReader *bir)
{
    return bir->error_string ? bir->error_string : "";
}

/*!
 * \brief Set error string for a failed operation.
 *
 * \sa mb_bi_reader_set_error_v()
 *
 * \param bir MbBiReader
 * \param error_code Error code
 * \param fmt `printf()`-style format string
 * \param ... `printf()`-style format arguments
 *
 * \return MB_BI_OK if the error is successfully set or MB_BI_FAILED if an
 *         error occurs
 */
int mb_bi_reader_set_error(MbBiReader *bir, int error_code,
                           const char *fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = mb_bi_reader_set_error_v(bir, error_code, fmt, ap);
    va_end(ap);

    return ret;
}

/*!
 * \brief Set error string for a failed operation.
 *
 * \sa mb_bi_reader_set_error()
 *
 * \param bir MbBiReader
 * \param error_code Error code
 * \param fmt `printf()`-style format string
 * \param ap `printf()`-style format arguments as a va_list
 *
 * \return MB_BI_OK if the error is successfully set or MB_BI_FAILED if an
 *         error occurs
 */
int mb_bi_reader_set_error_v(MbBiReader *bir, int error_code,
                             const char *fmt, va_list ap)
{
    free(bir->error_string);

    char *dup = mb_format_v(fmt, ap);
    if (!dup) {
        return MB_BI_FAILED;
    }

    bir->error_code = error_code;
    bir->error_string = dup;
    return MB_BI_OK;
}

MB_END_C_DECLS
