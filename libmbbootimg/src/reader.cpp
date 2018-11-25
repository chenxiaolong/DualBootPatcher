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
 * \param[in] file Reference to file handle
 * \param[out] header Header instance to write header values
 *
 * \return
 *   * Return nothing if the header is successfully read
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
 * \param[in] file Reference to file handle
 * \param[out] entry Entry instance to write entry values
 *
 * \return
 *   * Return nothing if the entry is successfully read
 *   * Return ReaderError::EndOfEntries if there are no more entries
 *   * Return a specific error code if an error occurs
 */

/*!
 * \fn FormatReader::go_to_entry
 *
 * \brief Format reader callback to seek to a specific entry
 *
 * \param[in] file Reference to file handle
 * \param[out] entry Entry instance to write entry values
 * \param[in] entry_type Entry type to seek to
 *
 * \return
 *   * Return nothing if the entry is successfully read
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

static struct
{
    int code;
    const char *name;
    oc::result<void> (Reader::*func)();
} g_reader_formats[] = {
    {
        FORMAT_ANDROID,
        FORMAT_NAME_ANDROID,
        &Reader::enable_format_android
    }, {
        FORMAT_BUMP,
        FORMAT_NAME_BUMP,
        &Reader::enable_format_bump
    }, {
        FORMAT_LOKI,
        FORMAT_NAME_LOKI,
        &Reader::enable_format_loki
    }, {
        FORMAT_MTK,
        FORMAT_NAME_MTK,
        &Reader::enable_format_mtk
    }, {
        FORMAT_SONY_ELF,
        FORMAT_NAME_SONY_ELF,
        &Reader::enable_format_sony_elf
    },
};

FormatReader::FormatReader(Reader &reader)
    : m_reader(reader)
{
}

FormatReader::~FormatReader() = default;

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

oc::result<void> FormatReader::go_to_entry(File &file, Entry &entry,
                                           int entry_type)
{
    (void) file;
    (void) entry;
    (void) entry_type;
    return ReaderError::UnsupportedGoTo;
}

/*!
 * \brief Construct new Reader.
 */
Reader::Reader()
    : m_state(ReaderState::New)
    , m_owned_file()
    , m_file()
    , m_format()
    , m_format_user_set(false)
{
}

/*!
 * \brief Destroy a Reader.
 *
 * If the reader has not been closed, it will be closed. Since this is the
 * destructor, it is not possible to get the result of the close operation. To
 * get the result of the close operation, call Reader::close() manually.
 */
Reader::~Reader()
{
    (void) close();
}

Reader::Reader(Reader &&other) noexcept
    : m_state(other.m_state)
    , m_owned_file(std::move(other.m_owned_file))
    , m_file(other.m_file)
    , m_formats(std::move(other.m_formats))
    , m_format(other.m_format)
    , m_format_user_set(other.m_format_user_set)
{
    other.m_state = ReaderState::Moved;
}

Reader & Reader::operator=(Reader &&rhs) noexcept
{
    (void) close();

    m_state = rhs.m_state;
    m_owned_file.swap(rhs.m_owned_file);
    m_file = rhs.m_file;
    m_formats.swap(rhs.m_formats);
    m_format = rhs.m_format;
    m_format_user_set = rhs.m_format_user_set;

    rhs.m_state = ReaderState::Moved;

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

    // Perform bid if a format wasn't explicitly chosen
    if (!m_format) {
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
    }

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
    ENSURE_STATE_OR_RETURN_ERROR(~ReaderStates(ReaderState::Moved));

    auto reset_state = finally([&] {
        m_state = ReaderState::New;

        m_owned_file.reset();
        m_file = nullptr;

        // Reset auto-detected format
        if (!m_format_user_set) {
            m_format = nullptr;
        }
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
 * Read the header from the boot image and store the header values to a Header.
 *
 * \param[out] header Reference to Header for storing header values
 *
 * \return Nothing if the boot image header is successfully read. Otherwise, a
 *         specific error code.
 */
oc::result<void> Reader::read_header(Header &header)
{
    ENSURE_STATE_OR_RETURN_ERROR(ReaderState::Header);

    // Seek to beginning
    OUTCOME_TRYV(m_file->seek(0, SEEK_SET));

    header.clear();

    OUTCOME_TRYV(m_format->read_header(*m_file, header));

    m_state = ReaderState::Entry;
    return oc::success();
}

/*!
 * \brief Read next boot image entry.
 *
 * Read the next entry from the boot image and store the entry values to an
 * Entry.
 *
 * \param[out] entry Reference to Entry for storing entry values
 *
 * \return Nothing if the boot image entry is successfully read. If there are no
 *         more entries, this function returns ReaderError::EndOfEntries. If any
 *         other error occurs, a specific error code will be returned.
 */
oc::result<void> Reader::read_entry(Entry &entry)
{
    // Allow skipping to the next entry without reading the data
    ENSURE_STATE_OR_RETURN_ERROR(ReaderState::Entry | ReaderState::Data);

    entry.clear();

    OUTCOME_TRYV(m_format->read_entry(*m_file, entry));

    m_state = ReaderState::Data;
    return oc::success();
}

/*!
 * \brief Seek to specific boot image entry.
 *
 * Seek to the specified entry in the boot image and store the entry values to
 * an Entry.
 *
 * \param[out] entry Reference to Entry for storing entry values
 * \param[in] entry_type Entry type to seek to (0 for first entry)
 *
 * \return Nothing if the boot image entry is successfully read. If the boot
 *         image entry is not found, this function returns
 *         ReaderError::EndOfEntries. If any other error occurs, a specific
 *         error code will be returned.
 */
oc::result<void> Reader::go_to_entry(Entry &entry, int entry_type)
{
    // Allow skipping to an entry without reading the data
    ENSURE_STATE_OR_RETURN_ERROR(ReaderState::Entry | ReaderState::Data);

    entry.clear();

    OUTCOME_TRYV(m_format->go_to_entry(*m_file, entry, entry_type));

    m_state = ReaderState::Data;
    return oc::success();
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
 * \brief Get detected or forced boot image format code.
 *
 * * If enable_format_*() was used, then the detected boot image format code is
 *   returned.
 * * If set_format_*() was used, then the forced boot image format code is
 *   returned.
 *
 * \note The return value is meaningful only after the boot image has been
 *       successfully opened. Otherwise, an error will be returned.
 *
 * \return Boot image format code or -1 if the boot image is not open
 */
int Reader::format_code()
{
    ENSURE_STATE_OR_RETURN(~ReaderStates(ReaderState::Moved), -1);

    if (!m_format) {
        // ReaderError::NoFormatSelected
        return -1;
    }

    return m_format->type();
}

/*!
 * \brief Get detected or forced boot image format name.
 *
 * * If enable_format_*() was used, then the detected boot image format name is
 *   returned.
 * * If set_format_*() was used, then the forced boot image format name is
 *   returned.
 *
 * \note The return value is meaningful only after the boot image has been
 *       successfully opened. Otherwise, an error will be returned.
 *
 * \return Boot image format name or empty string if the boot image is not open
 */
std::string Reader::format_name()
{
    ENSURE_STATE_OR_RETURN(~ReaderStates(ReaderState::Moved), {});

    if (!m_format) {
        // ReaderError::NoFormatSelected
        return {};
    }

    return m_format->name();
}

/*!
 * \brief Force support for a boot image format by its code.
 *
 * Calling this function causes the bidding process to be skipped. The chosen
 * format will be used regardless of which formats are enabled.
 *
 * \param code Boot image format code (\ref MB_BI_FORMAT_CODES)
 *
 * \return Nothing if the format is successfully set. Otherwise, the error code.
 */
oc::result<void> Reader::set_format_by_code(int code)
{
    ENSURE_STATE_OR_RETURN_ERROR(ReaderState::New);
    FormatReader *format = nullptr;

    auto ret = enable_format_by_code(code);
    if (!ret && ret.error() != ReaderError::FormatAlreadyEnabled) {
        return ret.as_failure();
    }

    for (auto &f : m_formats) {
        if ((FORMAT_BASE_MASK & f->type() & code)
                == (FORMAT_BASE_MASK & code)) {
            format = f.get();
            break;
        }
    }

    assert(format);

    m_format = format;
    m_format_user_set = true;

    return oc::success();
}

/*!
 * \brief Force support for a boot image format by its name.
 *
 * Calling this function causes the bidding process to be skipped. The chosen
 * format will be used regardless of which formats are enabled.
 *
 * \param name Boot image format name (\ref MB_BI_FORMAT_NAMES)
 *
 * \return Nothing if the format is successfully set. Otherwise, the error code.
 */
oc::result<void> Reader::set_format_by_name(const std::string &name)
{
    ENSURE_STATE_OR_RETURN_ERROR(ReaderState::New);
    FormatReader *format = nullptr;

    auto ret = enable_format_by_name(name);
    if (!ret && ret.error() != ReaderError::FormatAlreadyEnabled) {
        return ret.as_failure();
    }

    for (auto &f : m_formats) {
        if (f->name() == name) {
            format = f.get();
            break;
        }
    }

    assert(format);

    m_format = format;
    m_format_user_set = true;

    return oc::success();
}

/*!
 * \brief Enable support for all boot image formats.
 *
 * \return Nothing if all formats are successfully enabled. Otherwise, the error
 *         code.
 */
oc::result<void> Reader::enable_format_all()
{
    ENSURE_STATE_OR_RETURN_ERROR(ReaderState::New);

    for (auto const &format : g_reader_formats) {
        auto ret = (this->*format.func)();
        if (!ret && ret.error() != ReaderError::FormatAlreadyEnabled) {
            return ret.as_failure();
        }
    }

    return oc::success();
}

/*!
 * \brief Enable support for a boot image format by its code.
 *
 * \param code Boot image format code (\ref MB_BI_FORMAT_CODES)
 *
 * \return Nothing if the format is successfully enabled. Otherwise, the error
 *         code.
 */
oc::result<void> Reader::enable_format_by_code(int code)
{
    ENSURE_STATE_OR_RETURN_ERROR(ReaderState::New);

    for (auto const &format : g_reader_formats) {
        if ((code & FORMAT_BASE_MASK) == (format.code & FORMAT_BASE_MASK)) {
            return (this->*format.func)();
        }
    }

    return ReaderError::InvalidFormatCode;
}

/*!
 * \brief Enable support for a boot image format by its name.
 *
 * \param name Boot image format name (\ref MB_BI_FORMAT_NAMES)
 *
 * \return Nothing if the format is successfully enabled. Otherwise, the error
 *         code.
 */
oc::result<void> Reader::enable_format_by_name(const std::string &name)
{
    ENSURE_STATE_OR_RETURN_ERROR(ReaderState::New);

    for (auto const &format : g_reader_formats) {
        if (name == format.name) {
            return (this->*format.func)();
        }
    }

    return ReaderError::InvalidFormatName;
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

oc::result<void> Reader::register_format(std::unique_ptr<FormatReader> format)
{
    ENSURE_STATE_OR_RETURN_ERROR(ReaderState::New);

    for (auto const &f : m_formats) {
        if ((FORMAT_BASE_MASK & f->type())
                == (FORMAT_BASE_MASK & format->type())) {
            return ReaderError::FormatAlreadyEnabled;
        }
    }

    m_formats.push_back(std::move(format));
    return oc::success();
}

}
