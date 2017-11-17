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
#include "mbcommon/finally.h"
#include "mbcommon/string.h"

#include "mbbootimg/entry.h"
#include "mbbootimg/header.h"

#define ENSURE_STATE_OR_RETURN(STATES, RETVAL) \
    do { \
        if (!(m_state & (STATES))) { \
            set_error(WriterError::InvalidState, \
                      "%s: Invalid state: "\
                      "expected 0x%" PRIx8 ", actual: 0x%" PRIx8, \
                      __func__, static_cast<uint8_t>(STATES), \
                      static_cast<uint8_t>(m_state)); \
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
 *   * Return true if the option is handled successfully
 *   * Return TODO if the option cannot be handled
 *   * Return false if an error occurs
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
 *   * Return true if successful
 *   * Return false if an error occurs
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
 *   * Return true if the header is successfully written
 *   * Return false if an error occurs
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
 *   * Return true if successful
 *   * Return false if an error occurs
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
 *   * Return true if the entry is successfully written
 *   * Return false if an error occurs
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
 *   * Return true if the entry is successfully written
 *   * Return false if an error occurs
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
 *   * Return true if successful
 *   * Return false if an error occurs
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
 *   * Return true if no errors occur while closing the boot image
 *   * Return false if an error occurs
 */

///

namespace mb
{
namespace bootimg
{

using namespace detail;

static struct
{
    int code;
    const char *name;
    bool (Writer::*func)();
} g_writer_formats[] = {
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
    },
};

FormatWriter::FormatWriter(Writer &writer)
    : _writer(writer)
{
}

FormatWriter::~FormatWriter() = default;

bool FormatWriter::init()
{
    return true;
}

bool FormatWriter::set_option(const char *key, const char *value)
{
    (void) key;
    (void) value;
    return false;
}

bool FormatWriter::finish_entry(File &file)
{
    (void) file;
    return true;
}

bool FormatWriter::close(File &file)
{
    (void) file;
    return true;
}

/*!
 * \brief Construct new Writer.
 */
Writer::Writer()
    : m_state(WriterState::New)
    , m_owned_file()
    , m_file()
    , m_format()
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
    close();
}

Writer::Writer(Writer &&other) noexcept
    : m_state(other.m_state)
    , m_owned_file(std::move(other.m_owned_file))
    , m_file(other.m_file)
    , m_error_code(other.m_error_code)
    , m_error_string(std::move(other.m_error_string))
    , m_format(std::move(other.m_format))
{
    other.m_state = WriterState::Moved;
}

Writer & Writer::operator=(Writer &&rhs) noexcept
{
    (void) close();

    m_state = rhs.m_state;
    m_owned_file.swap(rhs.m_owned_file);
    m_file = rhs.m_file;
    m_error_code = rhs.m_error_code;
    m_error_string.swap(rhs.m_error_string);
    m_format.swap(rhs.m_format);

    rhs.m_state = WriterState::Moved;

    return *this;
}

/*!
 * \brief Open boot image from filename (MBS).
 *
 * \param filename MBS filename
 *
 * \return Whether the boot image is successfully opened
 */
bool Writer::open_filename(const std::string &filename)
{
    ENSURE_STATE_OR_RETURN(WriterState::New, false);

    auto file = std::make_unique<StandardFile>();
    auto ret = file->open(filename, FileOpenMode::ReadWriteTrunc);

    // Open in read/write mode since some formats need to reread the file
    if (!ret) {
        set_error(ret.error(),
                  "Failed to open for writing: %s",
                  ret.error().message().c_str());
        return false;
    }

    return open(std::move(file));
}

/*!
 * \brief Open boot image from filename (WCS).
 *
 * \param filename WCS filename
 *
 * \return Whether the boot image is successfully opened
 */
bool Writer::open_filename_w(const std::wstring &filename)
{
    ENSURE_STATE_OR_RETURN(WriterState::New, false);

    auto file = std::make_unique<StandardFile>();
    auto ret = file->open(filename, FileOpenMode::ReadWriteTrunc);

    // Open in read/write mode since some formats need to reread the file
    if (!ret) {
        set_error(ret.error(),
                  "Failed to open for writing: %s",
                  ret.error().message().c_str());
        return false;
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
 * \return Whether the boot image is successfully opened
 */
bool Writer::open(std::unique_ptr<File> file)
{
    ENSURE_STATE_OR_RETURN(WriterState::New, false);

    if (!open(file.get())) {
        return false;
    }

    // Underlying pointer is not invalidated during a move
    m_owned_file = std::move(file);
    return true;
}

/*!
 * \brief Open boot image from File handle.
 *
 * This function will not take ownership of the file handle. When the Writer is
 * closed, the file handle will remain open.
 *
 * \param file File handle
 *
 * \return Whether the boot image is successfully opened
 */
bool Writer::open(File *file)
{
    ENSURE_STATE_OR_RETURN(WriterState::New, false);

    if (!m_format) {
        set_error(WriterError::NoFormatRegistered);
        return false;
    }

    m_state = WriterState::Header;
    m_file = file;

    return true;
}

/*!
 * \brief Close a Writer.
 *
 * This function will close a Writer if it is open. Regardless of the return
 * value, the writer is closed and can no longer be used for further operations.
 *
 * It is important to check the return value of this function instead of relying
 * on the destructor since some formats require steps to finalize the boot image
 * when the writer is closed.
 *
 * \return Whether the writer is successfully closed
 */
bool Writer::close()
{
    ENSURE_STATE_OR_RETURN(~WriterStates(WriterState::Moved), false);

    auto reset_state = finally([&] {
        m_state = WriterState::New;

        m_owned_file.reset();
        m_file = nullptr;
    });

    bool ret = true;

    if (m_state != WriterState::New) {
        if (!!m_format) {
            ret = m_format->close(*m_file);
        }

        if (m_owned_file && !m_owned_file->close()) {
            ret = false;
        }
    }

    return ret;
}

/*!
 * \brief Prepare boot image header instance.
 *
 * Prepare a Header instance for use with Writer::write_header().
 *
 * \param[out] header Header instance to initialize
 *
 * \return Whether the header instance is successfully fetched
 */
bool Writer::get_header(Header &header)
{
    ENSURE_STATE_OR_RETURN(WriterState::Header, false);

    header.clear();

    // Don't alter state
    return m_format->get_header(*m_file, header);
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
 * \return Whether the boot image header is successfully written
 */
bool Writer::write_header(const Header &header)
{
    ENSURE_STATE_OR_RETURN(WriterState::Header, false);

    if (!m_format->write_header(*m_file, header)) {
        return false;
    }

    m_state = WriterState::Entry;
    return true;
}

/*!
 * \brief Prepare boot image entry instance for the next entry.
 *
 * Prepare an Entry instance for the next entry.
 *
 * This function will return false and set the error to
 * WriterError::EndOfEntries when there are no more entries to write. It is
 * strongly* recommended to check the return value of Writer::close() when
 * closing the boot image as additional steps for finalizing the boot image
 * could fail.
 *
 * \param[out] entry Entry instance to initialize
 *
 * \return Whether the next entry is successfully fetched
 */
bool Writer::get_entry(Entry &entry)
{
    ENSURE_STATE_OR_RETURN(WriterState::Entry | WriterState::Data, false);

    // Finish current entry
    if (m_state == WriterState::Data) {
        if (!m_format->finish_entry(*m_file)) {
            return false;
        }

        m_state = WriterState::Entry;
    }

    entry.clear();

    if (!m_format->get_entry(*m_file, entry)) {
        return false;
    }

    m_state = WriterState::Entry;
    return true;
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
 * \return Whether the boot image entry is successfully written
 */
bool Writer::write_entry(const Entry &entry)
{
    ENSURE_STATE_OR_RETURN(WriterState::Entry, false);

    if (!m_format->write_entry(*m_file, entry)) {
        return false;
    }

    m_state = WriterState::Data;
    return true;
}

/*!
 * \brief Write boot image entry data.
 *
 * \param[in] buf Input buffer
 * \param[in] size Size of input buffer
 * \param[out] bytes_written Pointer to store number of bytes written
 *
 * \return Whether the data is successfully written
 */
bool Writer::write_data(const void *buf, size_t size, size_t &bytes_written)
{
    ENSURE_STATE_OR_RETURN(WriterState::Data, false);

    // Do not alter state. Stay in WriterState::DATA
    return m_format->write_data(*m_file, buf, size, bytes_written);
}

/*!
 * \brief Get selected boot image format code.
 *
 * \return Boot image format code or -1 if no format is selected
 */
int Writer::format_code()
{
    ENSURE_STATE_OR_RETURN(~WriterStates(WriterState::Moved), -1);

    if (!m_format) {
        set_error(WriterError::NoFormatSelected);
        return -1;
    }

    return m_format->type();
}

/*!
 * \brief Get selected boot image format name.
 *
 * \return Boot image format name or empty string if no format is selected
 */
std::string Writer::format_name()
{
    ENSURE_STATE_OR_RETURN(~WriterStates(WriterState::Moved), {});

    if (!m_format) {
        set_error(WriterError::NoFormatSelected);
        return {};
    }

    return m_format->name();
}

/*!
 * \brief Set boot image output format by its code.
 *
 * \param code Boot image format code (\ref MB_BI_FORMAT_CODES)
 *
 * \return Whether the format is successfully set
 */
bool Writer::set_format_by_code(int code)
{
    ENSURE_STATE_OR_RETURN(WriterState::New, false);

    for (auto const &format : g_writer_formats) {
        if ((code & FORMAT_BASE_MASK) == (format.code & FORMAT_BASE_MASK)) {
            return (this->*format.func)();
        }
    }

    set_error(WriterError::InvalidFormatCode,
              "Invalid format code: %d", code);
    return false;
}

/*!
 * \brief Set boot image output format by its name.
 *
 * \param name Boot image format name (\ref MB_BI_FORMAT_NAMES)
 *
 * \return Whether the format is successfully set
 */
bool Writer::set_format_by_name(const std::string &name)
{
    ENSURE_STATE_OR_RETURN(WriterState::New, false);

    for (auto const &format : g_writer_formats) {
        if (name == format.name) {
            return (this->*format.func)();
        }
    }

    set_error(WriterError::InvalidFormatName,
              "Invalid format name: %s", name.c_str());
    return false;
}

/*!
 * \brief Check whether writer is opened
 *
 * \return Whether writer is opened
 */
bool Writer::is_open()
{
    return m_state != WriterState::New;
}

/*!
 * \brief Check whether the writer is in a fatal state
 *
 * If the writer is in a fatal state, the only valid operation is to call
 * close().
 *
 * \return Whether writer is in a fatal state
 */
bool Writer::is_fatal()
{
    return m_state == WriterState::Fatal;
}

/*!
 * \brief Set whether writer is in a fatal state
 *
 * This function only has an effect if the writer is opened.
 *
 * If the writer is in a fatal state, the only valid operation is to call
 * close().
 */
void Writer::set_fatal()
{
    if (!(m_state & (WriterState::New | WriterState::Moved))) {
        m_state = WriterState::Fatal;
    }
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
    ENSURE_STATE_OR_RETURN(~WriterStates(WriterState::Moved), {});

    return m_error_code;
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
    ENSURE_STATE_OR_RETURN(~WriterStates(WriterState::Moved), {});

    return m_error_string;
}

/*!
 * \brief Set error string for a failed operation.
 *
 * \sa Writer::set_error_v()
 *
 * \param ec Error code
 *
 * \return Whether the error is successfully set
 */
bool Writer::set_error(std::error_code ec)
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
 * \return Whether the error is successfully set
 */
bool Writer::set_error(std::error_code ec, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    bool ret = set_error_v(ec, fmt, ap);
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
 * \return Whether the error is successfully set
 */
bool Writer::set_error_v(std::error_code ec, const char *fmt, va_list ap)
{
    ENSURE_STATE_OR_RETURN(~WriterStates(WriterState::Moved), false);

    m_error_code = ec;

    auto result = format_v_safe(fmt, ap);
    if (!result) {
        return false;
    }
    m_error_string = std::move(result.value());

    if (!m_error_string.empty()) {
        m_error_string += ": ";
    }
    m_error_string += ec.message();

    return true;
}

/*!
 * \brief Register a format writer
 *
 * Register a format writer with a Writer. The Writer will take ownership of
 * \p format.
 *
 * \param format FormatWriter to register
 *
 * \return Whether the format is successfully registered
 */
bool Writer::register_format(std::unique_ptr<FormatWriter> format)
{
    ENSURE_STATE_OR_RETURN(WriterState::New, false);

    if (!format->init()) {
        return false;
    }

    m_format = std::move(format);
    return true;
}

}
}
