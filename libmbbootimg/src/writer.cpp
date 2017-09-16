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
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "mbcommon/file.h"
#include "mbcommon/file/standard.h"
#include "mbcommon/string.h"

#include "mbbootimg/entry.h"
#include "mbbootimg/header.h"
#include "mbbootimg/writer_p.h"

#define GET_PIMPL_OR_RETURN(RETVAL) \
    MB_PRIVATE(Writer); \
    do { \
        if (!priv) { \
            return RETVAL; \
        } \
    } while (0)

#define ENSURE_STATE_OR_RETURN(STATES, RETVAL) \
    do { \
        if (!(priv->state & (STATES))) { \
            set_error(make_error_code(WriterError::InvalidState), \
                      "%s: Invalid state: "\
                      "expected 0x%" PRIx8 ", actual: 0x%" PRIx8, \
                      __func__, static_cast<uint8_t>(STATES), \
                      static_cast<uint8_t>(priv->state)); \
            return RETVAL; \
        } \
    } while (0)

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

///

namespace mb
{
namespace bootimg
{

static struct
{
    int code;
    const char *name;
    int (Writer::*func)();
} writer_formats[] = {
    {
        FORMAT_ANDROID,
        FORMAT_NAME_ANDROID,
        &Writer::set_format_android
    }, {
        FORMAT_BUMP,
        FORMAT_NAME_BUMP,
        &Writer::set_format_bump
    }, {
        FORMAT_LOKI,
        FORMAT_NAME_LOKI,
        &Writer::set_format_loki
    }, {
        FORMAT_MTK,
        FORMAT_NAME_MTK,
        &Writer::set_format_mtk
    }, {
        FORMAT_SONY_ELF,
        FORMAT_NAME_SONY_ELF,
        &Writer::set_format_sony_elf
    }, {
        0,
        nullptr,
        nullptr
    },
};

FormatWriter::FormatWriter(Writer &writer)
    : _writer(writer)
{
}

FormatWriter::~FormatWriter() = default;

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

int FormatWriter::finish_entry(File &file)
{
    (void) file;
    return RET_OK;
}

int FormatWriter::close(File &file)
{
    (void) file;
    return RET_OK;
}

WriterPrivate::WriterPrivate(Writer *writer)
    : _pub_ptr(writer)
    , state(WriterState::New)
    , owned_file()
    , file()
    , format()
{
}

/*!
 * \brief Register a format writer
 *
 * Register a format writer with a Writer. The Writer will take ownership of
 * \p format.
 *
 * \param format FormatWriter to register
 *
 * \return
 *   * #RET_OK if the format is successfully registered
 *   * \<= #RET_FAILED if an error occurs
 */
int WriterPrivate::register_format(std::unique_ptr<FormatWriter> format)
{
    MB_PUBLIC(Writer);

    if (state != WriterState::New) {
        pub->set_error(make_error_code(WriterError::InvalidState),
                       "%s: Invalid state: expected 0x%" PRIx8
                       ", actual: 0x%" PRIx8,
                       __func__, static_cast<uint8_t>(WriterState::New),
                       static_cast<uint8_t>(state));
        return RET_FAILED;
    }

    int ret = format->init();
    if (ret != RET_OK) {
        return ret;
    }

    this->format = std::move(format);
    return RET_OK;
}

/*!
 * \brief Construct new Writer.
 */
Writer::Writer()
    : _priv_ptr(new WriterPrivate(this))
{
}

/*!
 * \brief Free a Writer.
 *
 * If the writer has not been closed, it will be closed. Since this is the
 * destructor, it is not possible to get the result of the close operation. To
 * get the result of the close operation, call Writer::close() manually.
 */
Writer::~Writer()
{
    MB_PRIVATE(Writer);

    if (priv) {
        if (priv->state != WriterState::Closed) {
            close();
        }
    }
}

Writer::Writer(Writer &&other) noexcept
    : _priv_ptr(std::move(other._priv_ptr))
{
}

Writer & Writer::operator=(Writer &&rhs) noexcept
{
    close();

    _priv_ptr = std::move(rhs._priv_ptr);

    return *this;
}

/*!
 * \brief Open boot image from filename (MBS).
 *
 * \param filename MBS filename
 *
 * \return
 *   * #RET_OK if the boot image is successfully opened
 *   * \<= #RET_WARN if an error occurs
 */
int Writer::open_filename(const std::string &filename)
{
    GET_PIMPL_OR_RETURN(RET_FATAL);
    ENSURE_STATE_OR_RETURN(WriterState::New, RET_FATAL);

    std::unique_ptr<File> file(
            new StandardFile(filename, FileOpenMode::READ_WRITE_TRUNC));

    // Open in read/write mode since some formats need to reread the file
    if (!file->is_open()) {
        set_error(file->error(),
                  "Failed to open for writing: %s",
                  file->error_string().c_str());
        return RET_FAILED;
    }

    return open(std::move(file));
}

/*!
 * \brief Open boot image from filename (WCS).
 *
 * \param filename WCS filename
 *
 * \return
 *   * #RET_OK if the boot image is successfully opened
 *   * \<= #RET_WARN if an error occurs
 */
int Writer::open_filename_w(const std::wstring &filename)
{
    GET_PIMPL_OR_RETURN(RET_FATAL);
    ENSURE_STATE_OR_RETURN(WriterState::New, RET_FATAL);

    std::unique_ptr<File> file(
            new StandardFile(filename, FileOpenMode::READ_WRITE_TRUNC));

    // Open in read/write mode since some formats need to reread the file
    if (!file->is_open()) {
        set_error(file->error(),
                  "Failed to open for writing: %s",
                  file->error_string().c_str());
        return RET_FAILED;
    }

    return open(std::move(file));
}

/*!
 * \brief Open boot image from File handle.
 *
 * This function will take ownership of the file handle. When the Writer is
 * closed, the file handle will also be closed.
 *
 * \param file File handle
 *
 * \return
 *   * #RET_OK if the boot image is successfully opened
 *   * \<= #RET_WARN if an error occurs
 */
int Writer::open(std::unique_ptr<File> file)
{
    GET_PIMPL_OR_RETURN(RET_FATAL);

    int ret = open(file.get());
    if (ret != RET_OK) {
        return ret;
    }

    // Underlying pointer is not invalidated during a move
    priv->owned_file = std::move(file);
    return RET_OK;
}

/*!
 * \brief Open boot image from File handle.
 *
 * This function will not take ownership of the file handle. When the Writer is
 * closed, the file handle will remain open.
 *
 * \param file File handle
 *
 * \return
 *   * #RET_OK if the boot image is successfully opened
 *   * \<= #RET_WARN if an error occurs
 */
int Writer::open(File *file)
{
    GET_PIMPL_OR_RETURN(RET_FATAL);
    ENSURE_STATE_OR_RETURN(WriterState::New, RET_FATAL);

    if (!priv->format) {
        set_error(make_error_code(WriterError::NoFormatRegistered));
        return RET_FAILED;
    }

    priv->state = WriterState::Header;
    priv->file = file;

    return RET_OK;
}

/*!
 * \brief Close a Writer.
 *
 * This function will close a Writer if it is open. Regardless of the return
 * value, the writer is closed and can no longer be used for further operations.
 *
 * \return
 *   * #RET_OK if no error is encountered when closing the writer
 *   * \<= #RET_WARN if the writer is opened and an error occurs while
 *     closing the writer
 */
int Writer::close()
{
    GET_PIMPL_OR_RETURN(RET_FATAL);

    int ret = RET_OK;

    // Avoid double-closing or closing nothing
    if (!(priv->state & (WriterState::Closed | WriterState::New))) {
        if (!!priv->format) {
            ret = priv->format->close(*priv->file);
        }

        if (priv->owned_file) {
            if (!priv->owned_file->close()) {
                if (RET_FAILED < ret) {
                    ret = RET_FAILED;
                }
            }
        }

        priv->owned_file.reset();
        priv->file = nullptr;

        // Don't change state to WriterState::FATAL if RET_FATAL is returned.
        // Otherwise, we risk double-closing the boot image. CLOSED and FATAL
        // are the same anyway, aside from the fact that boot images can be
        // closed in the latter state.
    }

    priv->state = WriterState::Closed;

    return ret;
}

/*!
 * \brief Prepare boot image header instance.
 *
 * Prepare a Header instance for use with Writer::write_header().
 *
 * \param[out] header Header instance to initialize
 *
 * \return
 *   * #RET_OK if no error occurs
 *   * \<= #RET_WARN if an error occurs
 */
int Writer::get_header(Header &header)
{
    GET_PIMPL_OR_RETURN(RET_FATAL);
    ENSURE_STATE_OR_RETURN(WriterState::Header, RET_FATAL);
    int ret;

    header.clear();

    ret = priv->format->get_header(*priv->file, header);
    if (ret == RET_OK) {
        // Don't alter state
    } else if (ret <= RET_FATAL) {
        priv->state = WriterState::Fatal;
    }

    return ret;
}

/*!
 * \brief Write boot image header
 *
 * Write a header to the boot image. It is recommended to use the Header
 * instance provided by Writer::get_header(), but it is not strictly necessary.
 * Fields that are not supported by the boot image format will be silently
 * ignored.
 *
 * \param header Header instance to write
 *
 * \return
 *   * #RET_OK if the boot image header is successfully written
 *   * \<= #RET_WARN if an error occurs
 */
int Writer::write_header(const Header &header)
{
    GET_PIMPL_OR_RETURN(RET_FATAL);
    ENSURE_STATE_OR_RETURN(WriterState::Header, RET_FATAL);
    int ret;

    ret = priv->format->write_header(*priv->file, header);
    if (ret == RET_OK) {
        priv->state = WriterState::Entry;
    } else if (ret <= RET_FATAL) {
        priv->state = WriterState::Fatal;
    }

    return ret;
}

/*!
 * \brief Prepare boot image entry instance for the next entry.
 *
 * Prepare an Entry instance for the next entry.
 *
 * This function will return #RET_EOF when there are no more entries to write.
 * It is *strongly* recommended to check the return value of Writer::close()
 * when closing the boot image as additional steps for finalizing the boot image
 * could fail.
 *
 * \param[out] entry Entry instance to initialize
 *
 * \return
 *   * #RET_OK if no error occurs
 *   * \<= #RET_WARN if an error occurs
 */
int Writer::get_entry(Entry &entry)
{
    GET_PIMPL_OR_RETURN(RET_FATAL);
    ENSURE_STATE_OR_RETURN(WriterState::Entry | WriterState::Data, RET_FATAL);
    int ret;

    // Finish current entry
    if (priv->state == WriterState::Data) {
        ret = priv->format->finish_entry(*priv->file);
        if (ret == RET_OK) {
            priv->state = WriterState::Entry;
        } else if (ret <= RET_FATAL) {
            priv->state = WriterState::Fatal;
            return ret;
        } else {
            return ret;
        }
    }

    entry.clear();

    ret = priv->format->get_entry(*priv->file, entry);
    if (ret == RET_OK) {
        priv->state = WriterState::Entry;
    } else if (ret <= RET_FATAL) {
        priv->state = WriterState::Fatal;
    }

    return ret;
}

/*!
 * \brief Write boot image entry.
 *
 * Write an entry to the boot image. It is *strongly* recommended to use the
 * Entry instance provided by Writer::get_entry(), but it is not strictly
 * necessary. If a different instance of Entry is used, the type field *must*
 * match the type field of the instance returned by Writer::get_entry().
 *
 * \param entry Entry instance to write
 *
 * \return
 *   * #RET_OK if the boot image entry is successfully written
 *   * #RET_EOF if the boot image has no more entries
 *   * \<= #RET_WARN if an error occurs
 */
int Writer::write_entry(const Entry &entry)
{
    GET_PIMPL_OR_RETURN(RET_FATAL);
    ENSURE_STATE_OR_RETURN(WriterState::Entry, RET_FATAL);
    int ret;

    ret = priv->format->write_entry(*priv->file, entry);
    if (ret == RET_OK) {
        priv->state = WriterState::Data;
    } else if (ret <= RET_FATAL) {
        priv->state = WriterState::Fatal;
    }

    return ret;
}

/*!
 * \brief Write boot image entry data.
 *
 * \param[in] buf Input buffer
 * \param[in] size Size of input buffer
 * \param[out] bytes_written Pointer to store number of bytes written
 *
 * \return
 *   * #RET_OK if data is successfully written
 *   * \<= #RET_WARN if an error occurs
 */
int Writer::write_data(const void *buf, size_t size, size_t &bytes_written)
{
    GET_PIMPL_OR_RETURN(RET_FATAL);
    ENSURE_STATE_OR_RETURN(WriterState::Data, RET_FATAL);
    int ret;

    ret = priv->format->write_data(*priv->file, buf, size, bytes_written);
    if (ret == RET_OK) {
        // Do not alter state. Stay in WriterState::DATA
    } else if (ret <= RET_FATAL) {
        priv->state = WriterState::Fatal;
    }

    return ret;
}

/*!
 * \brief Get selected boot image format code.
 *
 * \return Boot image format code or -1 if no format is selected
 */
int Writer::format_code()
{
    GET_PIMPL_OR_RETURN(-1);

    if (!priv->format) {
        set_error(make_error_code(WriterError::NoFormatSelected));
        return -1;
    }

    return priv->format->type();
}

/*!
 * \brief Get selected boot image format name.
 *
 * \return Boot image format name or empty string if no format is selected
 */
std::string Writer::format_name()
{
    GET_PIMPL_OR_RETURN({});

    if (!priv->format) {
        set_error(make_error_code(WriterError::NoFormatSelected));
        return {};
    }

    return priv->format->name();
}

/*!
 * \brief Set boot image output format by its code.
 *
 * \param code Boot image format code (\ref MB_BI_FORMAT_CODES)
 *
 * \return
 *   * #RET_OK if the format is successfully enabled
 *   * \<= #RET_WARN if an error occurs
 */
int Writer::set_format_by_code(int code)
{
    GET_PIMPL_OR_RETURN(RET_FATAL);
    ENSURE_STATE_OR_RETURN(WriterState::New, RET_FATAL);

    for (auto it = writer_formats; it->func; ++it) {
        if ((code & FORMAT_BASE_MASK) == (it->code & FORMAT_BASE_MASK)) {
            return (this->*it->func)();
        }
    }

    set_error(make_error_code(WriterError::InvalidFormatCode),
              "Invalid format code: %d", code);
    return RET_FAILED;
}

/*!
 * \brief Set boot image output format by its name.
 *
 * \param name Boot image format name (\ref MB_BI_FORMAT_NAMES)
 *
 * \return
 *   * #RET_OK if the format is successfully enabled
 *   * \<= #RET_WARN if an error occurs
 */
int Writer::set_format_by_name(const std::string &name)
{
    GET_PIMPL_OR_RETURN(RET_FATAL);
    ENSURE_STATE_OR_RETURN(WriterState::New, RET_FATAL);

    for (auto it = writer_formats; it->func; ++it) {
        if (name == it->name) {
            return (this->*it->func)();
        }
    }

    set_error(make_error_code(WriterError::InvalidFormatName),
              "Invalid format name: %s", name.c_str());
    return RET_FAILED;
}

/*!
 * \brief Get error code for a failed operation.
 *
 * \note The return value is undefined if an operation did not fail.
 *
 * \return Error code for failed operation.
 */
std::error_code Writer::error()
{
    GET_PIMPL_OR_RETURN({});

    return priv->error_code;
}

/*!
 * \brief Get error string for a failed operation.
 *
 * \note The return value is undefined if an operation did not fail.
 *
 * \return Error string for failed operation. The string contents may be
 *         undefined, but will never be NULL or an invalid string.
 */
std::string Writer::error_string()
{
    GET_PIMPL_OR_RETURN({});

    return priv->error_string;
}

/*!
 * \brief Set error string for a failed operation.
 *
 * \sa Writer::set_error_v()
 *
 * \param ec Error code
 *
 * \return RET_OK if the error is successfully set or RET_FAILED if an
 *         error occurs
 */
int Writer::set_error(std::error_code ec)
{
    return set_error(ec, "%s", "");
}

/*!
 * \brief Set error string for a failed operation.
 *
 * \sa Writer::set_error_v()
 *
 * \param ec Error code
 * \param fmt `printf()`-style format string
 * \param ... `printf()`-style format arguments
 *
 * \return RET_OK if the error is successfully set or RET_FAILED if an
 *         error occurs
 */
int Writer::set_error(std::error_code ec, const char *fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = set_error_v(ec, fmt, ap);
    va_end(ap);

    return ret;
}

/*!
 * \brief Set error string for a failed operation.
 *
 * \sa Writer::set_error()
 *
 * \param ec Error code
 * \param fmt `printf()`-style format string
 * \param ap `printf()`-style format arguments as a va_list
 *
 * \return RET_OK if the error is successfully set or RET_FAILED if an
 *         error occurs
 */
int Writer::set_error_v(std::error_code ec, const char *fmt, va_list ap)
{
    GET_PIMPL_OR_RETURN(RET_FATAL);

    priv->error_code = ec;

    if (!format_v(priv->error_string, fmt, ap)) {
        return RET_FAILED;
    }

    if (!priv->error_string.empty()) {
        priv->error_string += ": ";
    }
    priv->error_string += ec.message();

    return RET_OK;
}

}
}
