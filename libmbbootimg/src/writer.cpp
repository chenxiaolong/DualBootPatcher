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
#include "mbcommon/file/standard.h"
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
 *   * Return #RET_OK if the option is handled successfully
 *   * Return #RET_WARN if the option cannot be handled
 *   * Return \<= #RET_FAILED if an error occurs
 */

/*!
 * \typedef FormatWriterGetHeader
 * \ingroup MB_BI_WRITER_FORMAT_CALLBACKS
 *
 * \brief Format writer callback to get Header instance
 *
 * \param[in] biw MbBiWriter
 * \param[in] userdata User callback data
 * \param[out] header Pointer to store Header instance
 *
 * \return
 *   * Return #RET_OK if successful
 *   * Return \<= #RET_WARN if an error occurs
 */

/*!
 * \typedef FormatWriterWriteHeader
 * \ingroup MB_BI_WRITER_FORMAT_CALLBACKS
 *
 * \brief Format writer callback to write header
 *
 * \param biw MbBiWriter
 * \param userdata User callback data
 * \param header Header instance to write
 *
 * \return
 *   * Return #RET_OK if the header is successfully written
 *   * Return \<= #RET_WARN if an error occurs
 */

/*!
 * \typedef FormatWriterGetEntry
 * \ingroup MB_BI_WRITER_FORMAT_CALLBACKS
 *
 * \brief Format writer callback to get Entry instance
 *
 * \param[in] biw MbBiWriter
 * \param[in] userdata User callback data
 * \param[out] entry Pointer to store Entry instance
 *
 * \return
 *   * Return #RET_OK if successful
 *   * Return \<= #RET_WARN if an error occurs
 */

/*!
 * \typedef FormatWriterWriteEntry
 * \ingroup MB_BI_WRITER_FORMAT_CALLBACKS
 *
 * \brief Format writer callback to write entry
 *
 * \param biw MbBiWriter
 * \param userdata User callback data
 * \param entry Entry instance to write
 *
 * \return
 *   * Return #RET_OK if the entry is successfully written
 *   * Return \<= #RET_WARN if an error occurs
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
 *   * Return #RET_OK if the entry is successfully written
 *   * Return \<= #RET_WARN if an error occurs
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
 *   * Return #RET_OK if successful
 *   * Return \<= #RET_WARN if an error occurs
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
 *   * Return #RET_OK if no errors occur while closing the boot image
 *   * Return \<= #RET_WARN if an error occurs
 */

/*!
 * \typedef FormatWriterFree
 *
 * \brief Format writer callback to clean up resources
 *
 * This function will be called during a call to writer_free(), regardless
 * of the current state. It is guaranteed to only be called once. The function
 * may return any valid status code, but the resources *must* be cleaned up.
 *
 * \param biw MbBiWriter
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
    int (*func)(MbBiWriter *);
} writer_formats[] = {
    {
        FORMAT_ANDROID,
        FORMAT_NAME_ANDROID,
        writer_set_format_android
    }, {
        FORMAT_BUMP,
        FORMAT_NAME_BUMP,
        writer_set_format_bump
    }, {
        FORMAT_LOKI,
        FORMAT_NAME_LOKI,
        writer_set_format_loki
    }, {
        FORMAT_MTK,
        FORMAT_NAME_MTK,
        writer_set_format_mtk
    }, {
        FORMAT_SONY_ELF,
        FORMAT_NAME_SONY_ELF,
        writer_set_format_sony_elf
    }, {
        0,
        nullptr,
        nullptr
    },
};

FormatWriter::FormatWriter(MbBiWriter *biw)
    : _biw(biw)
{
}

FormatWriter::~FormatWriter()
{
}

int FormatWriter::init()
{
    return RET_OK;
}

int FormatWriter::set_option(const char *key, const char *value)
{
    (void) key;
    (void) value;
    return RET_OK;
}

int FormatWriter::finish_entry()
{
    return RET_OK;
}

int FormatWriter::close()
{
    return RET_OK;
}

/*!
 * \brief Register a format writer
 *
 * Register a format writer with an MbBiWriter. The MbBiWriter will take
 * ownership of \p format and will clean up its resources automatically
 * (regardless if this function fails).
 *
 * \param biw MbBiWriter
 * \param format FormatWriter to register
 *
 * \return
 *   * #RET_OK if the format is successfully registered
 *   * \<= #RET_FAILED if an error occurs
 */
int _writer_register_format(MbBiWriter *biw,
                            std::unique_ptr<FormatWriter> format)
{
    WRITER_ENSURE_STATE(biw, WriterState::NEW);

    int ret = format->init();
    if (ret != RET_OK) {
        return ret;
    }

    biw->format = std::move(format);
    return RET_OK;
}

/*!
 * \brief Allocate new MbBiWriter.
 *
 * \return New MbBiWriter or NULL if memory could not be allocated. If the
 *         function fails, `errno` will be set accordingly.
 */
MbBiWriter * writer_new()
{
    MbBiWriter *biw = new MbBiWriter();
    biw->state = WriterState::NEW;
    return biw;
}

/*!
 * \brief Free an MbBiWriter.
 *
 * If the writer has not been closed, it will be closed and the result of
 * writer_close() will be returned. Otherwise, #RET_OK will be returned.
 * Regardless of the return value, the writer will always be freed and should no
 * longer be used.
 *
 * \param biw MbBiWriter
 * \return
 *   * #RET_OK if the writer is successfully freed
 *   * \<= #RET_WARN if an error occurs (though the resources will still be
 *     freed)
 */
int writer_free(MbBiWriter *biw)
{
    int ret = RET_OK;

    if (biw) {
        if (biw->state != WriterState::CLOSED) {
            ret = writer_close(biw);
        }

        delete biw;
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
 *   * #RET_OK if the boot image is successfully opened
 *   * \<= #RET_WARN if an error occurs
 */
int writer_open_filename(MbBiWriter *biw, const std::string &filename)
{
    WRITER_ENSURE_STATE(biw, WriterState::NEW);

    File *file = new StandardFile(filename, FileOpenMode::READ_WRITE_TRUNC);

    // Open in read/write mode since some formats need to reread the file
    if (!file->is_open()) {
        writer_set_error(biw, file->error().value() /* TODO */,
                         "Failed to open for writing: %s",
                         file->error_string().c_str());
        delete file;
        return RET_FAILED;
    }

    return writer_open(biw, file, true);
}

/*!
 * \brief Open boot image from filename (WCS).
 *
 * \param biw MbBiWriter
 * \param filename WCS filename
 *
 * \return
 *   * #RET_OK if the boot image is successfully opened
 *   * \<= #RET_WARN if an error occurs
 */
int writer_open_filename_w(MbBiWriter *biw, const std::wstring &filename)
{
    WRITER_ENSURE_STATE(biw, WriterState::NEW);

    File *file = new StandardFile(filename, FileOpenMode::READ_WRITE_TRUNC);

    // Open in read/write mode since some formats need to reread the file
    if (!file->is_open()) {
        writer_set_error(biw, file->error().value() /* TODO */,
                         "Failed to open for writing: %s",
                         file->error_string().c_str());
        delete file;
        return RET_FAILED;
    }

    return writer_open(biw, file, true);
}

/*!
 * Open boot image from File handle.
 *
 * If \p owned is true, then the File handle will be closed and freed when the
 * MbBiWriter is closed and freed. This is true even if this function fails. In
 * other words, if \p owned is true and this function fails, File::close() will
 * be called.
 *
 * \param biw MbBiWriter
 * \param file File handle
 * \param owned Whether the MbBiWriter should take ownership of the File handle
 *
 * \return
 *   * #RET_OK if the boot image is successfully opened
 *   * \<= #RET_WARN if an error occurs
 */
int writer_open(MbBiWriter *biw, File *file, bool owned)
{
    int ret;

    // Ensure that the file is freed even if called in an incorrect state
    WRITER_ENSURE_STATE_GOTO(biw, WriterState::NEW, ret, done);

    if (!biw->format) {
        writer_set_error(biw, ERROR_PROGRAMMER_ERROR,
                         "No writer format registered");
        ret = RET_FAILED;
        goto done;
    }

    biw->file = file;
    biw->file_owned = owned;
    biw->state = WriterState::HEADER;

    ret = RET_OK;

done:
    if (ret != RET_OK) {
        if (owned) {
            delete file;
        }
    }
    return ret;
}

/*!
 * \brief Close an MbBiWriter.
 *
 * This function will close an MbBiWriter if it is open. Regardless of the
 * return value, the writer is closed and can no longer be used for further
 * operations. It should be freed with writer_free().
 *
 * \param biw MbBiWriter
 *
 * \return
 *   * #RET_OK if no error is encountered when closing the writer
 *   * \<= #RET_WARN if the writer is opened and an error occurs while
 *     closing the writer
 */
int writer_close(MbBiWriter *biw)
{
    int ret = RET_OK;

    // Avoid double-closing or closing nothing
    if (!(biw->state & (WriterState::CLOSED | WriterState::NEW))) {
        if (!!biw->format) {
            ret = biw->format->close();
        }

        if (biw->file && biw->file_owned) {
            if (!biw->file->close()) {
                if (RET_FAILED < ret) {
                    ret = RET_FAILED;
                }
            }

            delete biw->file;
        }

        biw->file = nullptr;
        biw->file_owned = false;

        // Don't change state to WriterState::FATAL if RET_FATAL is returned.
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
 * Get a prepared Header instance for use with writer_write_header()
 * and store a reference to it in \p header. The value of \p header after a
 * successful call to this function should *never* be deallocated manually.
 * It is tracked internally and will be freed when the MbBiWriter is freed.
 *
 * \param[in] biw MbBiWriter
 * \param[out] header Pointer to store Header instance
 *
 * \return
 *   * #RET_OK if no error occurs
 *   * \<= #RET_WARN if an error occurs
 */
int writer_get_header(MbBiWriter *biw, Header *&header)
{
    // State will be checked by writer_get_header2()
    int ret;

    ret = writer_get_header2(biw, biw->header);
    if (ret == RET_OK) {
        header = &biw->header;
    }

    return ret;
}

/*!
 * \brief Prepare boot image header instance.
 *
 * Prepare a Header instance for use with writer_write_header(). The
 * caller is responsible for deallocating \p header when it is no longer needed.
 *
 * \param[in] biw MbBiWriter
 * \param[out] header Header instance to initialize
 *
 * \return
 *   * #RET_OK if no error occurs
 *   * \<= #RET_WARN if an error occurs
 */
int writer_get_header2(MbBiWriter *biw, Header &header)
{
    WRITER_ENSURE_STATE(biw, WriterState::HEADER);
    int ret;

    header.clear();

    ret = biw->format->get_header(header);
    if (ret == RET_OK) {
        // Don't alter state
    } else if (ret <= RET_FATAL) {
        biw->state = WriterState::FATAL;
    }

    return ret;
}

/*!
 * \brief Write boot image header
 *
 * Write a header to the boot image. It is recommended to use the Header
 * instance provided by writer_get_header(), but it is not strictly
 * necessary. Fields that are not supported by the boot image format will be
 * silently ignored.
 *
 * \param biw MbBiWriter
 * \param header Header instance to write
 *
 * \return
 *   * #RET_OK if the boot image header is successfully written
 *   * \<= #RET_WARN if an error occurs
 */
int writer_write_header(MbBiWriter *biw, const Header &header)
{
    WRITER_ENSURE_STATE(biw, WriterState::HEADER);
    int ret;

    ret = biw->format->write_header(header);
    if (ret == RET_OK) {
        biw->state = WriterState::ENTRY;
    } else if (ret <= RET_FATAL) {
        biw->state = WriterState::FATAL;
    }

    return ret;
}

/*!
 * \brief Get boot image entry instance for the next entry
 *
 * Get an Entry instance for the next entry and store a reference to it in
 * \p entry. The value of \p entry after a successful call to this function
 * should *never* be deallocated manually. It is tracked internally and will be
 * freed when the MbBiWriter is freed.
 *
 * This function will return #RET_EOF when there are no more entries to write.
 * It is *strongly* recommended to check the return value of writer_close() or
 * writer_free() when closing the boot image as additional steps for finalizing
 * the boot image could fail.
 *
 * \param[in] biw MbBiWriter
 * \param[out] entry Pointer to store Entry instance
 *
 * \return
 *   * #RET_OK if no error occurs
 *   * #RET_EOF if the boot image has no more entries
 *   * \<= #RET_WARN if an error occurs
 */
int writer_get_entry(MbBiWriter *biw, Entry *&entry)
{
    // State will be checked by writer_get_entry2()
    int ret;

    ret = writer_get_entry2(biw, biw->entry);
    if (ret == RET_OK) {
        entry = &biw->entry;
    }

    return ret;
}

/*!
 * \brief Prepare boot image entry instance for the next entry.
 *
 * Prepare an Entry instance for the next entry. The caller is responsible
 * for deallocating \p entry when it is no longer needed.
 *
 * \param[in] biw MbBiWriter
 * \param[out] entry Entry instance to initialize
 *
 * \return
 *   * #RET_OK if no error occurs
 *   * \<= #RET_WARN if an error occurs
 */
int writer_get_entry2(MbBiWriter *biw, Entry &entry)
{
    WRITER_ENSURE_STATE(biw, WriterState::ENTRY | WriterState::DATA);
    int ret;

    // Finish current entry
    if (biw->state == WriterState::DATA) {
        ret = biw->format->finish_entry();
        if (ret == RET_OK) {
            biw->state = WriterState::ENTRY;
        } else if (ret <= RET_FATAL) {
            biw->state = WriterState::FATAL;
            return ret;
        } else {
            return ret;
        }
    }

    entry.clear();

    ret = biw->format->get_entry(entry);
    if (ret == RET_OK) {
        biw->state = WriterState::ENTRY;
    } else if (ret <= RET_FATAL) {
        biw->state = WriterState::FATAL;
    }

    return ret;
}

/*!
 * \brief Write boot image entry.
 *
 * Write an entry to the boot image. It is *strongly* recommended to use the
 * Entry instance provided by writer_get_entry(), but it is not strictly
 * necessary. If a different instance of Entry is used, the type field *must*
 * match the type field of the instance returned by writer_get_entry().
 *
 * \param biw MbBiWriter
 * \param entry Entry instance to write
 *
 * \return
 *   * #RET_OK if the boot image entry is successfully written
 *   * #RET_EOF if the boot image has no more entries
 *   * \<= #RET_WARN if an error occurs
 */
int writer_write_entry(MbBiWriter *biw, const Entry &entry)
{
    WRITER_ENSURE_STATE(biw, WriterState::ENTRY);
    int ret;

    ret = biw->format->write_entry(entry);
    if (ret == RET_OK) {
        biw->state = WriterState::DATA;
    } else if (ret <= RET_FATAL) {
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
 *   * #RET_OK if data is successfully written
 *   * \<= #RET_WARN if an error occurs
 */
int writer_write_data(MbBiWriter *biw, const void *buf, size_t size,
                      size_t &bytes_written)
{
    WRITER_ENSURE_STATE(biw, WriterState::DATA);
    int ret;

    ret = biw->format->write_data(buf, size, bytes_written);
    if (ret == RET_OK) {
        // Do not alter state. Stay in WriterState::DATA
    } else if (ret <= RET_FATAL) {
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
int writer_format_code(MbBiWriter *biw)
{
    if (!biw->format) {
        writer_set_error(biw, ERROR_PROGRAMMER_ERROR,
                         "No format selected");
        return -1;
    }

    return biw->format->type();
}

/*!
 * \brief Get selected boot image format name.
 *
 * \param biw MbBiWriter
 *
 * \return Boot image format name or empty string if no format is selected
 */
std::string writer_format_name(MbBiWriter *biw)
{
    if (!biw->format) {
        writer_set_error(biw, ERROR_PROGRAMMER_ERROR,
                         "No format selected");
        return {};
    }

    return biw->format->name();
}

/*!
 * \brief Set boot image output format by its code.
 *
 * \param biw MbBiWriter
 * \param code Boot image format code (\ref MB_BI_FORMAT_CODES)
 *
 * \return
 *   * #RET_OK if the format is successfully enabled
 *   * \<= #RET_WARN if an error occurs
 */
int writer_set_format_by_code(MbBiWriter *biw, int code)
{
    WRITER_ENSURE_STATE(biw, WriterState::NEW);

    for (auto it = writer_formats; it->func; ++it) {
        if ((code & FORMAT_BASE_MASK) == (it->code & FORMAT_BASE_MASK)) {
            return it->func(biw);
        }
    }

    writer_set_error(biw, ERROR_PROGRAMMER_ERROR,
                     "Invalid format code: %d", code);
    return RET_FAILED;
}

/*!
 * \brief Set boot image output format by its name.
 *
 * \param biw MbBiWriter
 * \param name Boot image format name (\ref MB_BI_FORMAT_NAMES)
 *
 * \return
 *   * #RET_OK if the format is successfully enabled
 *   * \<= #RET_WARN if an error occurs
 */
int writer_set_format_by_name(MbBiWriter *biw, const std::string &name)
{
    WRITER_ENSURE_STATE(biw, WriterState::NEW);

    for (auto it = writer_formats; it->func; ++it) {
        if (name == it->name) {
            return it->func(biw);
        }
    }

    writer_set_error(biw, ERROR_PROGRAMMER_ERROR,
                     "Invalid format name: %s", name.c_str());
    return RET_FAILED;
}

/*!
 * \brief Get error code for a failed operation.
 *
 * \note The return value is undefined if an operation did not fail.
 *
 * \param biw MbBiWriter
 *
 * \return Error code for failed operation. If \>= 0, then the value is one of
 *         the \ref MB_BI_ERROR_CODES entries. If \< 0, then the error code is
 *         implementation-defined (usually `-errno` or `-GetLastError()`).
 */
int writer_error(MbBiWriter *biw)
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
const char * writer_error_string(MbBiWriter *biw)
{
    return biw->error_string.c_str();
}

/*!
 * \brief Set error string for a failed operation.
 *
 * \sa writer_set_error_v()
 *
 * \param biw MbBiWriter
 * \param error_code Error code
 * \param fmt `printf()`-style format string
 * \param ... `printf()`-style format arguments
 *
 * \return RET_OK if the error is successfully set or RET_FAILED if an
 *         error occurs
 */
int writer_set_error(MbBiWriter *biw, int error_code, const char *fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = writer_set_error_v(biw, error_code, fmt, ap);
    va_end(ap);

    return ret;
}

/*!
 * \brief Set error string for a failed operation.
 *
 * \sa writer_set_error()
 *
 * \param biw MbBiWriter
 * \param error_code Error code
 * \param fmt `printf()`-style format string
 * \param ap `printf()`-style format arguments as a va_list
 *
 * \return RET_OK if the error is successfully set or RET_FAILED if an
 *         error occurs
 */
int writer_set_error_v(MbBiWriter *biw, int error_code, const char *fmt,
                       va_list ap)
{
    biw->error_code = error_code;
    return format_v(biw->error_string, fmt, ap)
            ? RET_OK : RET_FAILED;
}

}
}
