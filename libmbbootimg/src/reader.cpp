/*
 * Copyright (C) 2017-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <cassert>
#include <cerrno>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "mbcommon/file.h"
#include "mbcommon/file/standard.h"
#include "mbcommon/finally.h"

#include "mbbootimg/entry.h"
#include "mbbootimg/format/android_reader_p.h"
#include "mbbootimg/format/loki_reader_p.h"
#include "mbbootimg/format/mtk_reader_p.h"
#include "mbbootimg/format/sony_elf_reader_p.h"
#include "mbbootimg/header.h"

#define ENSURE_STATE_OR_RETURN(STATES, RETVAL) \
    do { \
        if (!(m_state & (STATES))) { \
            return RETVAL; \
        } \
    } while (0)

#define ENSURE_STATE_OR_RETURN_ERROR(STATES) \
    ENSURE_STATE_OR_RETURN(STATES, ReaderError::InvalidState)

/*!
 * \file mbbootimg/reader.h
 * \brief Boot image reader API
 */

/*!
 * \file mbbootimg/reader_p.h
 * \brief Boot image reader private API
 */

/*!
 * \fn FormatReader::set_option
 *
 * \brief Format reader callback to set option
 *
 * \note This is currently not exposed in the public API. There is no way to
 *       access this function.
 *
 * \param key Option key
 * \param value Option value
 *
 * \return
 *   * Return nothing the option is handled successfully
 *   * Return ReaderError::UnknownOption if the option cannot be handled
 *   * Return a specific error code if an error occurs
 */

/*!
 * \fn FormatReader::open
 *
 * \brief Format reader callback to open a boot image
 *
 * This function returns a bid based on the confidence in which the format
 * reader can parse the boot image. The bid is usually the number of bits the
 * reader is confident that conform to the file format (eg. magic string). The
 * file position is set to the beginning of the file before this function is
 * called and also after.
 *
 * If this function returns an error code or if the bid is lost, close() will be
 * called in Reader::open() to clean up any state. Otherwise, close() will be
 * called in Reader::close() when the user closes the Reader.
 *
 * \param file Reference to file handle
 * \param best_bid Current best bid
 *
 * \return
 *   * Return a non-negative integer to place a bid
 *   * Return a negative integer to indicate that the bid cannot be won
 *   * Return a specific error code if an error occurs
 */

/*!
 * \fn FormatReader::close
 *
 * \brief Format reader callback to finalize and close boot image
 *
 * This function will be called to clean up the state regardless of whether the
 * file is successfully opened. It is guaranteed that this function will only
 * ever be called once after a call to open(). If an error code is returned, the
 * user cannot reattempt the close operation.
 *
 * \param file Reference to file handle
 *
 * \return
 *   * Return nothing if no errors occur while closing the boot image
 *   * Return a specific error code if an error occurs
 */

/*!
 * \fn FormatReader::read_header
 *
 * \brief Format reader callback to read header
 *
 * The file position is guaranteed to be set to the beginning of the file before
 * this function is called.
 *
 * \param file Reference to file handle
 *
 * \return
 *   * Return a Header instance if it is successfully read
 *   * Return a specific error code if an error occurs
 */

/*!
 * \fn FormatReader::read_entry
 *
 * \brief Format reader callback to read next entry
 *
 * \note This callback *must* be able to skip to the next entry if the user does
 *       not read or finish reading the entry data with Reader::read_data().
 *
 * \param file Reference to file handle
 *
 * \return
 *   * Return an Entry instance if it is successfully read
 *   * Return ReaderError::EndOfEntries if there are no more entries
 *   * Return a specific error code if an error occurs
 */

/*!
 * \fn FormatReader::go_to_entry
 *
 * \brief Format reader callback to seek to a specific entry
 *
 * \param file Reference to file handle
 * \param entry_type Entry type to seek to
 *
 * \return
 *   * Return an Entry instance if it is successfully read
 *   * Return ReaderError::EndOfEntries if the entry cannot be found
 *   * Return a specific error code if an error occurs
 */

/*!
 * \fn FormatReader::read_data
 *
 * \brief Format reader callback to read entry data
 *
 * \note This function *must* read \p buf_size bytes unless EOF is reached.
 *
 * \param[in] file Reference to file handle
 * \param[out] buf Output buffer to write data
 * \param[in] buf_size Size of output buffer
 *
 * \return
 *   * Return number of bytes read if the entry is successfully read
 *   * Return a specific error code if an error occurs
 */

///

namespace mb::bootimg
{

using namespace detail;

FormatReader::FormatReader() noexcept = default;

FormatReader::~FormatReader() noexcept = default;

oc::result<void> FormatReader::set_option(const char *key, const char *value)
{
    (void) key;
    (void) value;
    return ReaderError::UnknownOption;
}

oc::result<void> FormatReader::close(File &file)
{
    (void) file;
    return oc::success();
}

oc::result<Entry> FormatReader::go_to_entry(File &file,
                                            std::optional<EntryType> entry_type)
{
    (void) file;
    (void) entry_type;
    return ReaderError::UnsupportedGoTo;
}

/*!
 * \brief Construct new Reader.
 */
Reader::Reader() noexcept
    : m_state(ReaderState::New)
    , m_owned_file()
    , m_file()
    , m_format()
{
}

/*!
 * \brief Destroy a Reader.
 *
 * If the reader has not been closed, it will be closed. Since this is the
 * destructor, it is not possible to get the result of the close operation. To
 * get the result of the close operation, call Reader::close() manually.
 */
Reader::~Reader() noexcept
{
    (void) close();
}

Reader::Reader(Reader &&other) noexcept
    : Reader()
{
    std::swap(m_state, other.m_state);
    std::swap(m_owned_file, other.m_owned_file);
    std::swap(m_file, other.m_file);
    std::swap(m_formats, other.m_formats);
    std::swap(m_format, other.m_format);
}

Reader & Reader::operator=(Reader &&rhs) noexcept
{
    if (this != &rhs) {
        (void) close();

        // close() doesn't reset enabled formats
        m_formats.clear();

        std::swap(m_state, rhs.m_state);
        std::swap(m_owned_file, rhs.m_owned_file);
        std::swap(m_file, rhs.m_file);
        std::swap(m_formats, rhs.m_formats);
        std::swap(m_format, rhs.m_format);
    }

    return *this;
}

/*!
 * \brief Open boot image from filename (MBS).
 *
 * \param filename MBS filename
 *
 * \return Nothing if the boot image is successfully opened. Otherwise, a
 *         specific error code.
 */
oc::result<void> Reader::open_filename(const std::string &filename)
{
    ENSURE_STATE_OR_RETURN_ERROR(ReaderState::New);

    auto file = std::make_unique<StandardFile>();
    OUTCOME_TRYV(file->open(filename, FileOpenMode::ReadOnly));

    return open(std::move(file));
}

/*!
 * \brief Open boot image from filename (WCS).
 *
 * \param filename WCS filename
 *
 * \return Nothing if the boot image is successfully opened. Otherwise, a
 *         specific error code.
 */
oc::result<void> Reader::open_filename_w(const std::wstring &filename)
{
    ENSURE_STATE_OR_RETURN_ERROR(ReaderState::New);

    auto file = std::make_unique<StandardFile>();
    OUTCOME_TRYV(file->open(filename, FileOpenMode::ReadOnly));

    return open(std::move(file));
}

/*!
 * \brief Open boot image from File handle.
 *
 * This function will take ownership of the file handle. When the Reader is
 * closed, the file handle will also be closed.
 *
 * \param file File handle
 *
 * \return Nothing if the boot image is successfully opened. Otherwise, a
 *         specific error code.
 */
oc::result<void> Reader::open(std::unique_ptr<File> file)
{
    ENSURE_STATE_OR_RETURN_ERROR(ReaderState::New);

    OUTCOME_TRYV(open(file.get()));

    // Underlying pointer is not invalidated during a move
    m_owned_file = std::move(file);
    return oc::success();
}

/*!
 * \brief Open boot image from File handle.
 *
 * This function will not take ownership of the file handle. When the Reader is
 * closed, the file handle will remain open.
 *
 * \param file File handle
 *
 * \return Nothing if the boot image is successfully opened. Otherwise, a
 *         specific error code.
 */
oc::result<void> Reader::open(File *file)
{
    ENSURE_STATE_OR_RETURN_ERROR(ReaderState::New);

    if (m_formats.empty()) {
        return ReaderError::NoFormatsRegistered;
    }

    int best_bid = 0;
    FormatReader *format = nullptr;

    auto close_format = finally([&] {
        if (format) {
            (void) format->close(*file);
        }
    });

    // Perform bid for autodetection
    for (auto &f : m_formats) {
        // Seek to beginning
        OUTCOME_TRYV(file->seek(0, SEEK_SET));

        auto close_f = finally([&] {
            (void) f->close(*file);
        });

        // Call bidder
        OUTCOME_TRY(bid, f->open(*file, best_bid));

        if (bid > best_bid) {
            // Close previous best format
            if (format) {
                (void) format->close(*file);
            }

            // Don't close this format
            close_f.dismiss();

            best_bid = bid;
            format = f.get();
        }
    }

    if (!format) {
        return ReaderError::UnknownFileFormat;
    }

    // We've found a matching format, so don't close it
    close_format.dismiss();

    m_format = format;

    m_state = ReaderState::Header;
    m_file = file;

    return oc::success();
}

/*!
 * \brief Close a Reader.
 *
 * This function will close a Reader if it is open. Regardless of the return
 * value, the reader is closed and can no longer be used for further operations.
 *
 * \return Nothing if the reader is successfully closed. Otherwise, a specific
 *         error code.
 */
oc::result<void> Reader::close()
{
    auto reset_state = finally([&] {
        m_state = ReaderState::New;

        m_owned_file.reset();
        m_file = nullptr;

        m_format = nullptr;
    });

    oc::result<void> ret = oc::success();

    if (m_state != ReaderState::New) {
        ret = m_format->close(*m_file);

        if (m_owned_file) {
            auto close_ret = m_owned_file->close();
            if (ret && !close_ret) {
                ret = std::move(close_ret);
            }
        }
    }

    return ret;
}

/*!
 * \brief Read boot image header.
 *
 * \return Header instance if the boot image header is successfully read.
 *         Otherwise, a specific error code.
 */
oc::result<Header> Reader::read_header()
{
    ENSURE_STATE_OR_RETURN_ERROR(ReaderState::Header);

    // Seek to beginning
    OUTCOME_TRYV(m_file->seek(0, SEEK_SET));

    OUTCOME_TRY(header, m_format->read_header(*m_file));

    m_state = ReaderState::Entry;
    return std::move(header);
}

/*!
 * \brief Read next boot image entry.
 *
 * \return The next boot image entry if the boot image entry is successfully
 *         read. If there are no more entries, this function returns
 *         ReaderError::EndOfEntries. If any other error occurs, a specific
 *         error code will be returned.
 */
oc::result<Entry> Reader::read_entry()
{
    // Allow skipping to the next entry without reading the data
    ENSURE_STATE_OR_RETURN_ERROR(ReaderState::Entry | ReaderState::Data);

    OUTCOME_TRY(entry, m_format->read_entry(*m_file));

    m_state = ReaderState::Data;
    return std::move(entry);
}

/*!
 * \brief Seek to specific boot image entry.
 *
 * \param entry_type Entry type to seek to (std::nullopt for first entry)
 *
 * \return The specified boot image entry if it exists and is successfully
 *         read. If the boot image entry is not found, this function returns
 *         ReaderError::EndOfEntries. If any other error occurs, a specific
 *         error code will be returned.
 */
oc::result<Entry> Reader::go_to_entry(std::optional<EntryType> entry_type)
{
    // Allow skipping to an entry without reading the data
    ENSURE_STATE_OR_RETURN_ERROR(ReaderState::Entry | ReaderState::Data);

    OUTCOME_TRY(entry, m_format->go_to_entry(*m_file, entry_type));

    m_state = ReaderState::Data;
    return std::move(entry);
}

/*!
 * \brief Read current boot image entry data.
 *
 * \param[out] buf Output buffer
 * \param[in] size Size of output buffer
 *
 * \return Number of bytes read. If EOF is reached, the return value will be
 *         less than \p size. Otherwise, it's guaranteed to equal \p size. If an
 *         error occurs, a specific error code will be returned.
 */
oc::result<size_t> Reader::read_data(void *buf, size_t size)
{
    ENSURE_STATE_OR_RETURN_ERROR(ReaderState::Data);

    // Do not alter state. Stay in ReaderState::DATA
    return m_format->read_data(*m_file, buf, size);
}

/*!
 * \brief Get detected boot image format code.
 *
 * \note The return value is meaningful only after the boot image has been
 *       successfully opened.
 *
 * \return Boot image format code
 */
std::optional<Format> Reader::format()
{
    if (!m_format) {
        return std::nullopt;
    }

    return m_format->type();
}

static std::unique_ptr<FormatReader> _construct_format(Format format)
{
    switch (format) {
        case Format::Android:
            return std::make_unique<android::AndroidFormatReader>(false);
        case Format::Bump:
            return std::make_unique<android::AndroidFormatReader>(true);
        case Format::Loki:
            return std::make_unique<loki::LokiFormatReader>();
        case Format::Mtk:
            return std::make_unique<mtk::MtkFormatReader>();
        case Format::SonyElf:
            return std::make_unique<sonyelf::SonyElfFormatReader>();
        default:
            MB_UNREACHABLE("Invalid format");
    }
}

/*!
 * \brief Enable support for boot image formats.
 *
 * \note Calling this function replaces previously enabled formats. It is not
 *       cumulative.
 *
 * \param formats Formats to enable
 *
 * \return Nothing if the formats are successfully enabled. Otherwise, the error
 *         code.
 */
oc::result<void> Reader::enable_formats(Formats formats)
{
    ENSURE_STATE_OR_RETURN_ERROR(ReaderState::New);

    if (formats != (formats & ALL_FORMATS)) {
        return std::errc::invalid_argument;
    }

    // Remove formats that shouldn't be enabled
    for (auto it = m_formats.begin(); it != m_formats.end();) {
        auto format = (*it)->type();
        if (formats & format) {
            formats.set_flag(format, false);
            ++it;
        } else {
            it = m_formats.erase(it);
        }
    }

    // Enable what's left to be enabled
    for (auto const format : formats) {
        m_formats.push_back(_construct_format(format));
    }

    return oc::success();
}

/*!
 * \brief Enable support for all boot image formats.
 *
 * This is equivalent to calling `enable_formats(ALL_FORMATS)`.
 *
 * \return Nothing if all formats are successfully enabled. Otherwise, the error
 *         code.
 */
oc::result<void> Reader::enable_formats_all()
{
    return enable_formats(ALL_FORMATS);
}

/*!
 * \brief Check whether reader is opened
 *
 * \return Whether reader is opened
 */
bool Reader::is_open()
{
    return m_state != ReaderState::New;
}

}
