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

#include "mbbootimg/writer.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "mbcommon/file.h"
#include "mbcommon/file/filename.h"
#include "mbcommon/string.h"

#include "mbbootimg/entry.h"
#include "mbbootimg/header.h"
#include "mbbootimg/writer_p.h"

/*!
 * \file mbbootimg/writer.h
 * \brief Boot image writer API
 */

/*!
 * \file mbbootimg/writer_p.h
 * \brief Boot image writer private API
 */

/*!
 * \defgroup MB_BI_WRITER_FORMAT_CALLBACKS Format writer callbacks
 */

/*!
 * \typedef FormatWriterSetOption
 * \ingroup MB_BI_WRITER_FORMAT_CALLBACKS
 *
 * \brief Format writer callback to set option
 *
 * \param biw MbBiWriter
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
 * \typedef FormatWriterGetHeader
 * \ingroup MB_BI_WRITER_FORMAT_CALLBACKS
 *
 * \brief Format writer callback to get MbBiHeader instance
 *
 * \param[in] biw MbBiWriter
 * \param[in] userdata User callback data
 * \param[out] header Pointer to store MbBiHeader instance
 *
 * \return
 *   * Return #MB_BI_OK if successful
 *   * Return \<= #MB_BI_WARN if an error occurs
 */

/*!
 * \typedef FormatWriterWriteHeader
 * \ingroup MB_BI_WRITER_FORMAT_CALLBACKS
 *
 * \brief Format writer callback to write header
 *
 * \param biw MbBiWriter
 * \param userdata User callback data
 * \param header MbBiHeader instance to write
 *
 * \return
 *   * Return #MB_BI_OK if the header is successfully written
 *   * Return \<= #MB_BI_WARN if an error occurs
 */

/*!
 * \typedef FormatWriterGetEntry
 * \ingroup MB_BI_WRITER_FORMAT_CALLBACKS
 *
 * \brief Format writer callback to get MbBiEntry instance
 *
 * \param[in] biw MbBiWriter
 * \param[in] userdata User callback data
 * \param[out] entry Pointer to store MbBiEntry instance
 *
 * \return
 *   * Return #MB_BI_OK if successful
 *   * Return \<= #MB_BI_WARN if an error occurs
 */

/*!
 * \typedef FormatWriterWriteEntry
 * \ingroup MB_BI_WRITER_FORMAT_CALLBACKS
 *
 * \brief Format writer callback to write entry
 *
 * \param biw MbBiWriter
 * \param userdata User callback data
 * \param entry MbBiEntry instance to write
 *
 * \return
 *   * Return #MB_BI_OK if the entry is successfully written
 *   * Return \<= #MB_BI_WARN if an error occurs
 */

/*!
 * \typedef FormatWriterWriteData
 * \ingroup MB_BI_WRITER_FORMAT_CALLBACKS
 *
 * \brief Format writer callback to write entry data
 *
 * \note The callback function *must* write \p buf_size bytes or return an error
 *       if it cannot do so.
 *
 * \param[in] biw MbBiWriter
 * \param[in] userdata User callback data
 * \param[in] buf Input buffer
 * \param[in] buf_size Size of input buffer
 * \param[out] bytes_written Output number of bytes that were written
 *
 * \return
 *   * Return #MB_BI_OK if the entry is successfully written
 *   * Return \<= #MB_BI_WARN if an error occurs
 */

/*!
 * \typedef FormatWriterFinishEntry
 * \ingroup MB_BI_WRITER_FORMAT_CALLBACKS
 *
 * \brief Format writer callback to complete the writing of an entry
 *
 * \param biw MbBiWriter
 * \param userdata User callback data
 *
 * \return
 *   * Return #MB_BI_OK if successful
 *   * Return \<= #MB_BI_WARN if an error occurs
 */

/*!
 * \typedef FormatWriterClose
 * \ingroup MB_BI_WRITER_FORMAT_CALLBACKS
 *
 * \brief Format writer callback to finalize and close boot image
 *
 * \param biw MbBiWriter
 * \param userdata User callback data
 *
 * \return
 *   * Return #MB_BI_OK if no errors occur while closing the boot image
 *   * Return \<= #MB_BI_WARN if an error occurs
 */

/*!
 * \typedef FormatWriterFree
 *
 * \brief Format writer callback to clean up resources
 *
 * This function will be called during a call to mb_bi_writer_free(), regardless
 * of the current state. It is guaranteed to only be called once. The function
 * may return any valid status code, but the resources *must* be cleaned up.
 *
 * \param biw MbBiWriter
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
    int (*func)(MbBiWriter *);
} writer_formats[] = {
    {
        MB_BI_FORMAT_ANDROID,
        MB_BI_FORMAT_NAME_ANDROID,
        mb_bi_writer_set_format_android
    }, {
        MB_BI_FORMAT_BUMP,
        MB_BI_FORMAT_NAME_BUMP,
        mb_bi_writer_set_format_bump
    }, {
        MB_BI_FORMAT_LOKI,
        MB_BI_FORMAT_NAME_LOKI,
        mb_bi_writer_set_format_loki
    }, {
        MB_BI_FORMAT_MTK,
        MB_BI_FORMAT_NAME_MTK,
        mb_bi_writer_set_format_mtk
    }, {
        MB_BI_FORMAT_SONY_ELF,
        MB_BI_FORMAT_NAME_SONY_ELF,
        mb_bi_writer_set_format_sony_elf
    }, {
        0,
        nullptr,
        nullptr
    },
};

/*!
 * \brief Register a format writer
 *
 * Register a format writer with an MbBiWriter. If the function fails,
 * \p free_cb will be called to clean up the resources.
 *
 * \param biw MbBiWriter
 * \param userdata User callback data
 * \param type Format type (must be one of \ref MB_BI_FORMAT_CODES)
 * \param name Format name (must be one of \ref MB_BI_FORMAT_NAMES)
 * \param set_option_cb Set option callback (optional)
 * \param get_header_cb Get header callback (required)
 * \param write_header_cb Write header callback (required)
 * \param get_entry_cb Get entry callback (required)
 * \param write_entry_cb Write entry callback (required)
 * \param write_data_cb Write data callback (required)
 * \param finish_entry_cb Finish entry callback (optional)
 * \param close_cb Close callback (optional)
 * \param free_cb Free callback (optional)
 *
 * \return
 *   * #MB_BI_OK if the format is successfully registered
 *   * \<= #MB_BI_FAILED if an error occurs
 */
int _mb_bi_writer_register_format(MbBiWriter *biw,
                                  void *userdata,
                                  int type,
                                  const char *name,
                                  FormatWriterSetOption set_option_cb,
                                  FormatWriterGetHeader get_header_cb,
                                  FormatWriterWriteHeader write_header_cb,
                                  FormatWriterGetEntry get_entry_cb,
                                  FormatWriterWriteEntry write_entry_cb,
                                  FormatWriterWriteData write_data_cb,
                                  FormatWriterFinishEntry finish_entry_cb,
                                  FormatWriterClose close_cb,
                                  FormatWriterFree free_cb)
{
    int ret;
    FormatWriter format;

    // Check state, but ensure that format is freed if this function is called
    // in an incorect state
    WRITER_ENSURE_STATE_GOTO(biw, WriterState::NEW, ret, done);

    format.type = type;
    format.name = strdup(name);
    format.set_option_cb = set_option_cb;
    format.get_header_cb = get_header_cb;
    format.write_header_cb = write_header_cb;
    format.get_entry_cb = get_entry_cb;
    format.write_entry_cb = write_entry_cb;
    format.write_data_cb = write_data_cb;
    format.finish_entry_cb = finish_entry_cb;
    format.close_cb = close_cb;
    format.free_cb = free_cb;
    format.userdata = userdata;

    if (!format.name) {
        mb_bi_writer_set_error(biw, -errno, "%s", strerror(errno));
        ret = MB_BI_FAILED;
        goto done;
    }

    // Clear old format
    if (biw->format_set) {
        _mb_bi_writer_free_format(biw, &biw->format);
    }

    // Set new format
    biw->format = format;
    biw->format_set = true;

    ret = MB_BI_OK;

done:
    if (ret != MB_BI_OK) {
        int ret2 = _mb_bi_writer_free_format(biw, &format);
        if (ret2 < ret) {
            ret = ret2;
        }
    }

    return ret;
}

/*!
 * \brief Free a format writer
 *
 * \note Regardless of the return value, the format writer's resources will have
 *       been cleaned up. The return value only exists to indicate if an error
 *       occurs in the process of cleaning up.
 *
 * \note If \p format is allocated on the heap, it is up to the caller to free
 *       its memory.
 *
 * \param biw MbBiWriter
 * \param format Format writer to clean up
 *
 * \return
 *   * #MB_BI_OK if the resources for \p format are successfully freed
 *   * \<= #MB_BI_WARN if an error occurs (though the resources will still be
 *     cleaned up)
 */
int _mb_bi_writer_free_format(MbBiWriter *biw, FormatWriter *format)
{
    WRITER_ENSURE_STATE(biw, WriterState::ANY);
    int ret = MB_BI_OK;

    if (format) {
        if (format->free_cb) {
            ret = format->free_cb(biw, format->userdata);
        }

        free(format->name);
    }

    return ret;
}

/*!
 * \brief Allocate new MbBiWriter.
 *
 * \return New MbBiWriter or NULL if memory could not be allocated. If the
 *         function fails, `errno` will be set accordingly.
 */
MbBiWriter * mb_bi_writer_new()
{
    MbBiWriter *biw = static_cast<MbBiWriter *>(calloc(1, sizeof(MbBiWriter)));
    if (biw) {
        biw->state = WriterState::NEW;
        biw->header = mb_bi_header_new();
        biw->entry = mb_bi_entry_new();

        if (!biw->header || !biw->entry) {
            mb_bi_header_free(biw->header);
            mb_bi_entry_free(biw->entry);
            free(biw);
            biw = nullptr;
        }
    }
    return biw;
}

/*!
 * \brief Free an MbBiWriter.
 *
 * If the writer has not been closed, it will be closed and the result of
 * mb_bi_writer_close() will be returned. Otherwise, #MB_BI_OK will be returned.
 * Regardless of the return value, the writer will always be freed and should no
 * longer be used.
 *
 * \param biw MbBiWriter
 * \return
 *   * #MB_BI_OK if the writer is successfully freed
 *   * \<= #MB_BI_WARN if an error occurs (though the resources will still be
 *     freed)
 */
int mb_bi_writer_free(MbBiWriter *biw)
{
    int ret = MB_BI_OK, ret2;

    if (biw) {
        if (biw->state != WriterState::CLOSED) {
            ret = mb_bi_writer_close(biw);
        }

        if (biw->format_set) {
            ret2 = _mb_bi_writer_free_format(biw, &biw->format);
            if (ret2 < ret) {
                ret = ret2;
            }
        }

        mb_bi_header_free(biw->header);
        mb_bi_entry_free(biw->entry);

        free(biw->error_string);
        free(biw);
    }

    return ret;
}

/*!
 * \brief Open boot image from filename (MBS).
 *
 * \param biw MbBiWriter
 * \param filename MBS filename
 *
 * \return
 *   * #MB_BI_OK if the boot image is successfully opened
 *   * \<= #MB_BI_WARN if an error occurs
 */
int mb_bi_writer_open_filename(MbBiWriter *biw, const char *filename)
{
    WRITER_ENSURE_STATE(biw, WriterState::NEW);
    int ret;

    MbFile *file = mb_file_new();
    if (!file) {
        mb_bi_writer_set_error(biw, MB_BI_ERROR_INTERNAL_ERROR,
                               "%s", strerror(errno));
        return MB_BI_FAILED;
    }

    // Open in read/write mode since some formats need to reread the file
    ret = mb_file_open_filename(file, filename, MB_FILE_OPEN_READ_WRITE_TRUNC);
    if (ret != MB_FILE_OK) {
        // Always return MB_BI_FAILED as MB_FILE_FATAL would not affect us
        // at this point
        mb_bi_writer_set_error(biw, mb_file_error(file),
                               "Failed to open for writing: %s",
                               mb_file_error_string(file));
        mb_file_free(file);
        return MB_BI_FAILED;
    }

    return mb_bi_writer_open(biw, file, true);
}

/*!
 * \brief Open boot image from filename (WCS).
 *
 * \param biw MbBiWriter
 * \param filename WCS filename
 *
 * \return
 *   * #MB_BI_OK if the boot image is successfully opened
 *   * \<= #MB_BI_WARN if an error occurs
 */
int mb_bi_writer_open_filename_w(MbBiWriter *biw, const wchar_t *filename)
{
    WRITER_ENSURE_STATE(biw, WriterState::NEW);
    int ret;

    MbFile *file = mb_file_new();
    if (!file) {
        mb_bi_writer_set_error(biw, MB_BI_ERROR_INTERNAL_ERROR,
                               "%s", strerror(errno));
        return MB_BI_FAILED;
    }

    // Open in read/write mode since some formats need to reread the file
    ret = mb_file_open_filename_w(file, filename,
                                  MB_FILE_OPEN_READ_WRITE_TRUNC);
    if (ret != MB_FILE_OK) {
        // Always return MB_BI_FAILED as MB_FILE_FATAL would not affect us
        // at this point
        mb_bi_writer_set_error(biw, mb_file_error(file),
                               "Failed to open for writing: %s",
                               mb_file_error_string(file));
        mb_file_free(file);
        return MB_BI_FAILED;
    }

    return mb_bi_writer_open(biw, file, true);
}

/*!
 * Open boot image from MbFile handle.
 *
 * If \p owned is true, then the MbFile handle will be closed and freed when the
 * MbBiWriter is closed and freed. This is true even if this function fails. In
 * other words, if \p owned is true and this function fails, mb_file_free() will
 * be called.
 *
 * \param biw MbBiWriter
 * \param file MbFile handle
 * \param owned Whether the MbBiWriter should take ownership of the MbFile
 *              handle
 *
 * \return
 *   * #MB_BI_OK if the boot image is successfully opened
 *   * \<= #MB_BI_WARN if an error occurs
 */
int mb_bi_writer_open(MbBiWriter *biw, MbFile *file, bool owned)
{
    int ret;

    // Ensure that the file is freed even if called in an incorrect state
    WRITER_ENSURE_STATE_GOTO(biw, WriterState::NEW, ret, done);

    if (!biw->format_set) {
        mb_bi_writer_set_error(biw, MB_BI_ERROR_PROGRAMMER_ERROR,
                               "No writer format registered");
        ret = MB_BI_FAILED;
        goto done;
    }

    biw->file = file;
    biw->file_owned = owned;
    biw->state = WriterState::HEADER;

    ret = MB_BI_OK;

done:
    if (ret != MB_BI_OK) {
        if (owned) {
            mb_file_free(file);
        }
    }
    return ret;
}

/*!
 * \brief Close an MbBiWriter.
 *
 * This function will close an MbBiWriter if it is open. Regardless of the
 * return value, the writer is closed and can no longer be used for further
 * operations. It should be freed with mb_bi_writer_free().
 *
 * \param biw MbBiWriter
 *
 * \return
 *   * #MB_BI_OK if no error is encountered when closing the writer
 *   * \<= #MB_BI_WARN if the writer is opened and an error occurs while
 *     closing the writer
 */
int mb_bi_writer_close(MbBiWriter *biw)
{
    int ret = MB_BI_OK, ret2;

    // Avoid double-closing or closing nothing
    if (!(biw->state & (WriterState::CLOSED | WriterState::NEW))) {
        if (biw->format_set && biw->format.close_cb) {
            ret = biw->format.close_cb(biw, biw->format.userdata);
        }

        if (biw->file && biw->file_owned) {
            ret2 = mb_file_free(biw->file);
            if (ret2 < 0) {
                ret2 = ret2 == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
            }
            if (ret2 < ret) {
                ret = ret2;
            }
        }

        biw->file = nullptr;
        biw->file_owned = false;

        // Don't change state to WriterState::FATAL if MB_BI_FATAL is returned.
        // Otherwise, we risk double-closing the boot image. CLOSED and FATAL
        // are the same anyway, aside from the fact that boot images can be
        // closed in the latter state.
    }

    biw->state = WriterState::CLOSED;

    return ret;
}

/*!
 * \brief Get boot image header instance
 *
 * Get a prepared MbBiHeader instance for use with mb_bi_writer_write_header()
 * and store a reference to it in \p header. The value of \p header after a
 * successful call to this function should *never* be deallocated with
 * mb_bi_header_free(). It is tracked internally and will be freed when the
 * MbBiWriter is freed.
 *
 * \param[in] biw MbBiWriter
 * \param[out] header Pointer to store MbBiHeader instance
 *
 * \return
 *   * #MB_BI_OK if no error occurs
 *   * \<= #MB_BI_WARN if an error occurs
 */
int mb_bi_writer_get_header(MbBiWriter *biw, MbBiHeader **header)
{
    // State will be checked by mb_bi_writer_get_header2()
    int ret;

    ret = mb_bi_writer_get_header2(biw, biw->header);
    if (ret == MB_BI_OK) {
        *header = biw->header;
    }

    return ret;
}

/*!
 * \brief Prepare boot image header instance.
 *
 * Prepare an MbBiHeader instance for use with mb_bi_writer_write_header(). The
 * caller is responsible for deallocating \p header when it is no longer needed.
 *
 * \param[in] biw MbBiWriter
 * \param[out] header MbBiHeader instance to initialize
 *
 * \return
 *   * #MB_BI_OK if no error occurs
 *   * \<= #MB_BI_WARN if an error occurs
 */
int mb_bi_writer_get_header2(MbBiWriter *biw, MbBiHeader *header)
{
    WRITER_ENSURE_STATE(biw, WriterState::HEADER);
    int ret;

    mb_bi_header_clear(header);

    if (!biw->format.get_header_cb) {
        mb_bi_writer_set_error(biw, MB_BI_ERROR_INTERNAL_ERROR,
                               "Missing format get_header_cb");
        biw->state = WriterState::FATAL;
        return MB_BI_FATAL;
    }

    ret = biw->format.get_header_cb(biw, biw->format.userdata, header);
    if (ret == MB_BI_OK) {
        // Don't alter state
    } else if (ret <= MB_BI_FATAL) {
        biw->state = WriterState::FATAL;
    }

    return ret;
}

/*!
 * \brief Write boot image header
 *
 * Write a header to the boot image. It is recommended to use the MbBiHeader
 * instance provided by mb_bi_writer_get_header(), but it is not strictly
 * necessary. Fields that are not supported by the boot image format will be
 * silently ignored.
 *
 * \param biw MbBiWriter
 * \param header MbBiHeader instance to write
 *
 * \return
 *   * #MB_BI_OK if the boot image header is successfully written
 *   * \<= #MB_BI_WARN if an error occurs
 */
int mb_bi_writer_write_header(MbBiWriter *biw, MbBiHeader *header)
{
    WRITER_ENSURE_STATE(biw, WriterState::HEADER);
    int ret;

    if (!biw->format.write_header_cb) {
        mb_bi_writer_set_error(biw, MB_BI_ERROR_INTERNAL_ERROR,
                               "Missing format write_header_cb");
        biw->state = WriterState::FATAL;
        return MB_BI_FATAL;
    }

    ret = biw->format.write_header_cb(biw, biw->format.userdata, header);
    if (ret == MB_BI_OK) {
        biw->state = WriterState::ENTRY;
    } else if (ret <= MB_BI_FATAL) {
        biw->state = WriterState::FATAL;
    }

    return ret;
}

/*!
 * \brief Get boot image entry instance for the next entry
 *
 * Get an MbBiEntry instance for the next entry and store a reference to it in
 * \p entry. The value of \p entry after a successful call to this function
 * should *never* be deallocated with mb_bi_entry_free(). It is tracked
 * internally and will be freed when the MbBiWriter is freed.
 *
 * This function will return #MB_BI_EOF when there are no more entries to write.
 * It is *strongly* recommended to check the return value of
 * mb_bi_writer_close() or mb_bi_writer_free() when closing the boot image as
 * additional steps for finalizing the boot image could fail.
 *
 * \param[in] biw MbBiWriter
 * \param[out] entry Pointer to store MbBiEntry instance
 *
 * \return
 *   * #MB_BI_OK if no error occurs
 *   * #MB_BI_EOF if the boot image has no more entries
 *   * \<= #MB_BI_WARN if an error occurs
 */
int mb_bi_writer_get_entry(MbBiWriter *biw, MbBiEntry **entry)
{
    // State will be checked by mb_bi_writer_get_entry2()
    int ret;

    ret = mb_bi_writer_get_entry2(biw, biw->entry);
    if (ret == MB_BI_OK) {
        *entry = biw->entry;
    }

    return ret;
}

/*!
 * \brief Prepare boot image entry instance for the next entry.
 *
 * Prepare an MbBiEntry instance for the next entry. The caller is responsible
 * for deallocating \p entry when it is no longer needed.
 *
 * \param[in] biw MbBiWriter
 * \param[out] entry MbBiEntry instance to initialize
 *
 * \return
 *   * #MB_BI_OK if no error occurs
 *   * \<= #MB_BI_WARN if an error occurs
 */
int mb_bi_writer_get_entry2(MbBiWriter *biw, MbBiEntry *entry)
{
    WRITER_ENSURE_STATE(biw, WriterState::ENTRY | WriterState::DATA);
    int ret;

    // Finish current entry
    if (biw->state == WriterState::DATA && biw->format.finish_entry_cb) {
        ret = biw->format.finish_entry_cb(biw, biw->format.userdata);

        if (ret == MB_BI_OK) {
            biw->state = WriterState::ENTRY;
        } else if (ret <= MB_BI_FATAL) {
            biw->state = WriterState::FATAL;
            return ret;
        } else {
            return ret;
        }
    }

    mb_bi_entry_clear(entry);

    if (!biw->format.get_entry_cb) {
        mb_bi_writer_set_error(biw, MB_BI_ERROR_INTERNAL_ERROR,
                               "Missing format get_entry_cb");
        biw->state = WriterState::FATAL;
        return MB_BI_FATAL;
    }

    ret = biw->format.get_entry_cb(biw, biw->format.userdata, entry);
    if (ret == MB_BI_OK) {
        biw->state = WriterState::ENTRY;
    } else if (ret <= MB_BI_FATAL) {
        biw->state = WriterState::FATAL;
    }

    return ret;
}

/*!
 * \brief Write boot image entry.
 *
 * Write an entry to the boot image. It is *strongly* recommended to use the
 * MbBiEntry instance provided by mb_bi_writer_get_entry(), but it is not
 * strictly necessary. If a different instance of MbBiEntry is used, the type
 * field *must* match the type field of the instance returned by
 * mb_bi_writer_get_entry().
 *
 * \param biw MbBiWriter
 * \param entry MbBiEntry instance to write
 *
 * \return
 *   * #MB_BI_OK if the boot image entry is successfully written
 *   * #MB_BI_EOF if the boot image has no more entries
 *   * \<= #MB_BI_WARN if an error occurs
 */
int mb_bi_writer_write_entry(MbBiWriter *biw, MbBiEntry *entry)
{
    WRITER_ENSURE_STATE(biw, WriterState::ENTRY);
    int ret;

    if (!biw->format.write_entry_cb) {
        mb_bi_writer_set_error(biw, MB_BI_ERROR_INTERNAL_ERROR,
                               "Missing format write_entry_cb");
        biw->state = WriterState::FATAL;
        return MB_BI_FATAL;
    }

    ret = biw->format.write_entry_cb(biw, biw->format.userdata, entry);
    if (ret == MB_BI_OK) {
        biw->state = WriterState::DATA;
    } else if (ret <= MB_BI_FATAL) {
        biw->state = WriterState::FATAL;
    }

    return ret;
}

/*!
 * \brief Write boot image entry data.
 *
 * \param[in] biw MbBiWriter
 * \param[in] buf Input buffer
 * \param[in] size Size of input buffer
 * \param[out] bytes_written Pointer to store number of bytes written
 *
 * \return
 *   * #MB_BI_OK if data is successfully written
 *   * \<= #MB_BI_WARN if an error occurs
 */
int mb_bi_writer_write_data(MbBiWriter *biw, const void *buf, size_t size,
                            size_t *bytes_written)
{
    WRITER_ENSURE_STATE(biw, WriterState::DATA);
    int ret;

    if (!biw->format.write_data_cb) {
        mb_bi_writer_set_error(biw, MB_BI_ERROR_INTERNAL_ERROR,
                               "Missing format write_data_cb");
        biw->state = WriterState::FATAL;
        return MB_BI_FATAL;
    }

    ret = biw->format.write_data_cb(biw, biw->format.userdata, buf, size,
                                    bytes_written);
    if (ret == MB_BI_OK) {
        // Do not alter state. Stay in WriterState::DATA
    } else if (ret <= MB_BI_FATAL) {
        biw->state = WriterState::FATAL;
    }

    return ret;
}

/*!
 * \brief Get selected boot image format code.
 *
 * \param biw MbBiWriter
 *
 * \return Boot image format code or -1 if no format is selected
 */
int mb_bi_writer_format_code(MbBiWriter *biw)
{
    if (!biw->format_set) {
        mb_bi_writer_set_error(biw, MB_BI_ERROR_PROGRAMMER_ERROR,
                               "No format selected");
        return -1;
    }

    return biw->format.type;
}

/*!
 * \brief Get selected boot image format name.
 *
 * \param biw MbBiWriter
 *
 * \return Boot image format name or NULL if no format is selected
 */
const char * mb_bi_writer_format_name(MbBiWriter *biw)
{
    if (!biw->format_set) {
        mb_bi_writer_set_error(biw, MB_BI_ERROR_PROGRAMMER_ERROR,
                               "No format selected");
        return NULL;
    }

    return biw->format.name;
}

/*!
 * \brief Set boot image output format by its code.
 *
 * \param biw MbBiWriter
 * \param code Boot image format code (\ref MB_BI_FORMAT_CODES)
 *
 * \return
 *   * #MB_BI_OK if the format is successfully enabled
 *   * \<= #MB_BI_WARN if an error occurs
 */
int mb_bi_writer_set_format_by_code(MbBiWriter *biw, int code)
{
    WRITER_ENSURE_STATE(biw, WriterState::NEW);

    for (auto it = writer_formats; it->func; ++it) {
        if ((code & MB_BI_FORMAT_BASE_MASK)
                == (it->code & MB_BI_FORMAT_BASE_MASK)) {
            return it->func(biw);
        }
    }

    mb_bi_writer_set_error(biw, MB_BI_ERROR_PROGRAMMER_ERROR,
                           "Invalid format code: %d", code);
    return MB_BI_FAILED;
}

/*!
 * \brief Set boot image output format by its name.
 *
 * \param biw MbBiWriter
 * \param name Boot image format name (\ref MB_BI_FORMAT_NAMES)
 *
 * \return
 *   * #MB_BI_OK if the format is successfully enabled
 *   * \<= #MB_BI_WARN if an error occurs
 */
int mb_bi_writer_set_format_by_name(MbBiWriter *biw, const char *name)
{
    WRITER_ENSURE_STATE(biw, WriterState::NEW);

    for (auto it = writer_formats; it->func; ++it) {
        if (strcmp(name, it->name) == 0) {
            return it->func(biw);
        }
    }

    mb_bi_writer_set_error(biw, MB_BI_ERROR_PROGRAMMER_ERROR,
                           "Invalid format name: %s", name);
    return MB_BI_FAILED;
}

/*!
 * \brief Get error code for a failed operation.
 *
 * \note The return value is undefined if an operation did not fail.
 *
 * \param biw MbBiWriter
 *
 * \return Error code for failed operation. If \>= 0, then the value is one of
 *         the MB_BI_* entries. If \< 0, then the error code is
 *         implementation-defined (usually `-errno` or `-GetLastError()`).
 */
int mb_bi_writer_error(MbBiWriter *biw)
{
    return biw->error_code;
}

/*!
 * \brief Get error string for a failed operation.
 *
 * \note The return value is undefined if an operation did not fail.
 *
 * \param biw MbBiWriter
 *
 * \return Error string for failed operation. The string contents may be
 *         undefined, but will never be NULL or an invalid string.
 */
const char * mb_bi_writer_error_string(MbBiWriter *biw)
{
    return biw->error_string ? biw->error_string : "";
}

/*!
 * \brief Set error string for a failed operation.
 *
 * \sa mb_bi_writer_set_error_v()
 *
 * \param biw MbBiWriter
 * \param error_code Error code
 * \param fmt `printf()`-style format string
 * \param ... `printf()`-style format arguments
 *
 * \return MB_BI_OK if the error is successfully set or MB_BI_FAILED if an
 *         error occurs
 */
int mb_bi_writer_set_error(MbBiWriter *biw, int error_code,
                           const char *fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = mb_bi_writer_set_error_v(biw, error_code, fmt, ap);
    va_end(ap);

    return ret;
}

/*!
 * \brief Set error string for a failed operation.
 *
 * \sa mb_bi_writer_set_error()
 *
 * \param biw MbBiWriter
 * \param error_code Error code
 * \param fmt `printf()`-style format string
 * \param ap `printf()`-style format arguments as a va_list
 *
 * \return MB_BI_OK if the error is successfully set or MB_BI_FAILED if an
 *         error occurs
 */
int mb_bi_writer_set_error_v(MbBiWriter *biw, int error_code,
                             const char *fmt, va_list ap)
{
    free(biw->error_string);

    char *dup = mb_format_v(fmt, ap);
    if (!dup) {
        return MB_BI_FAILED;
    }

    biw->error_code = error_code;
    biw->error_string = dup;
    return MB_BI_OK;
}

MB_END_C_DECLS
