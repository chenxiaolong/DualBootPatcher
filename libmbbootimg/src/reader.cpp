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

#define GET_PIMPL_OR_RETURN(RETVAL) \
    MB_PRIVATE(Reader); \
    do { \
        if (!priv) { \
            return RETVAL; \
        } \
    } while (0)

#define GET_PIMPL_OR_GOTO(RETURN_VAR, LABEL) \
    MB_PRIVATE(Reader); \
    do { \
        if (!priv) { \
            (RETURN_VAR) = RET_FATAL; \
            goto LABEL; \
        } \
    } while (0)

#define ENSURE_STATE_OR_RETURN(STATES, RETVAL) \
    do { \
        if (!(priv->state & (STATES))) { \
            set_error(ERROR_PROGRAMMER_ERROR, \
                      "%s: Invalid state: "\
                      "expected 0x%x, actual: 0x%hx", \
                      __func__, (STATES), priv->state); \
            return RETVAL; \
        } \
    } while (0)

#define ENSURE_STATE_OR_GOTO(STATES, RETURN_VAR, LABEL) \
    do { \
        if (!(priv->state & (STATES))) { \
            set_error(ERROR_PROGRAMMER_ERROR, \
                      "%s: Invalid state: " \
                      "expected 0x%x, actual: 0x%hx", \
                      __func__, (STATES), priv->state); \
            priv->state = ReaderState::FATAL; \
            (RETURN_VAR) = RET_FATAL; \
            goto LABEL; \
        } \
    } while (0)

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
 *       not read or finish reading the entry data with Reader::read_data().
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

///

namespace mb
{
namespace bootimg
{

static struct
{
    int code;
    const char *name;
    int (Reader::*func)();
} reader_formats[] = {
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
    }, {
        0,
        nullptr,
        nullptr
    },
};

FormatReader::FormatReader(Reader &reader)
    : _reader(reader)
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

int FormatReader::go_to_entry(File &file, Entry &entry, int entry_type)
{
    (void) file;
    (void) entry;
    (void) entry_type;
    return RET_UNSUPPORTED;
}

ReaderPrivate::ReaderPrivate(Reader *reader)
    : _pub_ptr(reader)
    , state(ReaderState::NEW)
    , file()
    , file_owned()
    , error_code()
    , error_string()
    , formats()
    , format()
{
}

int ReaderPrivate::register_format(std::unique_ptr<FormatReader> format)
{
    MB_PUBLIC(Reader);

    if (state != ReaderState::NEW) {
        pub->set_error(ERROR_PROGRAMMER_ERROR,
                       "%s: Invalid state: expected 0x%x, actual: 0x%hx",
                      __func__, ReaderState::NEW, state);
        return RET_FAILED;
    }

    if (formats.size() == MAX_FORMATS) {
        pub->set_error(ERROR_PROGRAMMER_ERROR, "Too many formats enabled");
        return RET_FAILED;
    }

    for (auto const &f : formats) {
        if ((FORMAT_BASE_MASK & f->type())
                == (FORMAT_BASE_MASK & format->type())) {
            pub->set_error(ERROR_PROGRAMMER_ERROR,
                           "%s format (0x%x) already enabled",
                           format->name().c_str(), format->type());
            return RET_WARN;
        }
    }

    formats.push_back(std::move(format));
    return RET_OK;
}

/*!
 * \brief Construct new Reader.
 */
Reader::Reader()
    : _priv_ptr(new ReaderPrivate(this))
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
    MB_PRIVATE(Reader);

    if (priv) {
        if (priv->state != ReaderState::CLOSED) {
            close();
        }
    }
}

Reader::Reader(Reader &&other) : _priv_ptr(std::move(other._priv_ptr))
{
}

Reader & Reader::operator=(Reader &&rhs)
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
int Reader::open_filename(const std::string &filename)
{
    GET_PIMPL_OR_RETURN(RET_FATAL);
    ENSURE_STATE_OR_RETURN(ReaderState::NEW, RET_FATAL);

    File *file = new StandardFile(filename, FileOpenMode::READ_ONLY);
    if (!file->is_open()) {
        set_error(file->error().value() /* TODO */,
                  "Failed to open for reading: %s",
                  file->error_string().c_str());
        delete file;
        return RET_FAILED;
    }

    return open(file, true);
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
int Reader::open_filename_w(const std::wstring &filename)
{
    GET_PIMPL_OR_RETURN(RET_FATAL);
    ENSURE_STATE_OR_RETURN(ReaderState::NEW, RET_FATAL);

    File *file = new StandardFile(filename, FileOpenMode::READ_ONLY);
    if (!file->is_open()) {
        set_error(file->error().value() /* TODO */,
                  "Failed to open for reading: %s",
                  file->error_string().c_str());
        delete file;
        return RET_FAILED;
    }

    return open(file, true);
}

/*!
 * \brief Open boot image from File handle.
 *
 * If \p owned is true, then the File handle will be closed and freed when the
 * Reader is closed and freed. This is true even if this function fails. In
 * other words, if \p owned is true and this function fails, File::close() will
 * be called.
 *
 * \param file File handle
 * \param owned Whether the Reader should take ownership of the File handle
 *
 * \return
 *   * #RET_OK if the boot image is successfully opened
 *   * \<= #RET_WARN if an error occurs
 */
int Reader::open(File *file, bool owned)
{
    int ret;
    int best_bid = 0;
    bool forced_format; // TODO TODO TODO Undefined behavior

    // Ensure that the file is freed even if called in an incorrect state
    GET_PIMPL_OR_GOTO(ret, done);
    ENSURE_STATE_OR_GOTO(ReaderState::NEW, ret, done);

    forced_format = !!priv->format;

    priv->file = file;
    priv->file_owned = owned;

    if (priv->formats.empty()) {
        set_error(ERROR_PROGRAMMER_ERROR, "No reader formats registered");
        ret = RET_FAILED;
        goto done;
    }

    // Perform bid if a format wasn't explicitly chosen
    if (!priv->format) {
        FormatReader *format = nullptr;

        for (auto &f : priv->formats) {
            // Seek to beginning
            if (!priv->file->seek(0, SEEK_SET, nullptr)) {
                set_error(priv->file->error().value() /* TODO */,
                          "Failed to seek file: %s",
                          priv->file->error_string().c_str());
                ret = priv->file->is_fatal() ? RET_FATAL : RET_FAILED;
                goto done;
            }

            // Call bidder
            ret = f->bid(*priv->file, best_bid);
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
            priv->format = format;
        } else {
            set_error(ERROR_FILE_FORMAT,
                      "Failed to determine boot image format");
            ret = RET_FAILED;
            goto done;
        }
    }

    priv->state = ReaderState::HEADER;

    ret = RET_OK;

done:
    if (ret != RET_OK) {
        if (owned) {
            delete file;
        }

        priv->file = nullptr;
        priv->file_owned = false;

        if (!forced_format) {
            priv->format = nullptr;
        }
    }
    return ret;
}

/*!
 * \brief Close a Reader.
 *
 * This function will close a Reader if it is open. Regardless of the return
 * value, the reader is closed and can no longer be used for further operations.
 *
 * \return
 *   * #RET_OK if the reader is successfully closed
 *   * \<= #RET_WARN if the reader is opened and an error occurs while
 *     closing the reader
 */
int Reader::close()
{
    GET_PIMPL_OR_RETURN(RET_FATAL);

    int ret = RET_OK;

    // Avoid double-closing or closing nothing
    if (!(priv->state & (ReaderState::CLOSED | ReaderState::NEW))) {
        if (priv->file && priv->file_owned) {
            if (!priv->file->close()) {
                if (RET_FAILED < ret) {
                    ret = RET_FAILED;
                }
            }

            delete priv->file;
        }

        priv->file = nullptr;
        priv->file_owned = false;

        // Don't change state to ReaderState::FATAL if RET_FATAL is returned.
        // Otherwise, we risk double-closing the boot image. CLOSED and FATAL
        // are the same anyway, aside from the fact that boot images can be
        // closed in the latter state.
    }

    priv->state = ReaderState::CLOSED;

    return ret;
}

/*!
 * \brief Read boot image header.
 *
 * Read the header from the boot image and store the header values to a Header.
 *
 * \param[out] header Reference to Header for storing header values
 *
 * \return
 *   * #RET_OK if the boot image header is successfully read
 *   * \<= #RET_WARN if an error occurs
 */
int Reader::read_header(Header &header)
{
    GET_PIMPL_OR_RETURN(RET_FATAL);
    ENSURE_STATE_OR_RETURN(ReaderState::HEADER, RET_FATAL);
    int ret;

    // Seek to beginning
    if (!priv->file->seek(0, SEEK_SET, nullptr)) {
        set_error(priv->file->error().value() /* TODO */,
                  "Failed to seek file: %s",
                  priv->file->error_string().c_str());
        return priv->file->is_fatal() ? RET_FATAL : RET_FAILED;
    }

    header.clear();

    ret = priv->format->read_header(*priv->file, header);
    if (ret == RET_OK) {
        priv->state = ReaderState::ENTRY;
    } else if (ret <= RET_FATAL) {
        priv->state = ReaderState::FATAL;
    }

    return ret;
}

/*!
 * \brief Read next boot image entry.
 *
 * Read the next entry from the boot image and store the entry values to an
 * Entry.
 *
 * \param[out] entry Reference to Entry for storing entry values
 *
 * \return
 *   * #RET_OK if the boot image entry is successfully read
 *   * #RET_EOF if the boot image has no more entries
 *   * #RET_UNSUPPORTED if the boot image format does not support seeking
 *   * \<= #RET_WARN if an error occurs
 */
int Reader::read_entry(Entry &entry)
{
    GET_PIMPL_OR_RETURN(RET_FATAL);
    // Allow skipping to the next entry without reading the data
    ENSURE_STATE_OR_RETURN(ReaderState::ENTRY | ReaderState::DATA, RET_FATAL);
    int ret;

    entry.clear();

    ret = priv->format->read_entry(*priv->file, entry);
    if (ret == RET_OK) {
        priv->state = ReaderState::DATA;
    } else if (ret <= RET_FATAL) {
        priv->state = ReaderState::FATAL;
    }

    return ret;
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
 * \return
 *   * #RET_OK if the boot image entry is successfully read
 *   * #RET_EOF if the boot image entry is not found
 *   * \<= #RET_WARN if an error occurs
 */
int Reader::go_to_entry(Entry &entry, int entry_type)
{
    GET_PIMPL_OR_RETURN(RET_FATAL);
    // Allow skipping to an entry without reading the data
    ENSURE_STATE_OR_RETURN(ReaderState::ENTRY | ReaderState::DATA, RET_FATAL);
    int ret;

    entry.clear();

    ret = priv->format->go_to_entry(*priv->file, entry, entry_type);
    if (ret == RET_OK) {
        priv->state = ReaderState::DATA;
    } else if (ret <= RET_FATAL) {
        priv->state = ReaderState::FATAL;
    }

    return ret;
}

/*!
 * \brief Read current boot image entry data.
 *
 * \param[out] buf Output buffer
 * \param[in] size Size of output buffer
 * \param[out] bytes_read Pointer to store number of bytes read
 *
 * \return
 *   * #RET_OK if data is successfully read
 *   * #RET_EOF if EOF is reached for the current entry
 *   * \<= #RET_WARN if an error occurs
 */
int Reader::read_data(void *buf, size_t size, size_t &bytes_read)
{
    GET_PIMPL_OR_RETURN(RET_FATAL);
    ENSURE_STATE_OR_RETURN(ReaderState::DATA, RET_FATAL);
    int ret;

    ret = priv->format->read_data(*priv->file, buf, size, bytes_read);
    if (ret == RET_OK) {
        // Do not alter state. Stay in ReaderState::DATA
    } else if (ret <= RET_FATAL) {
        priv->state = ReaderState::FATAL;
    }

    return ret;
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
    GET_PIMPL_OR_RETURN(-1);

    if (!priv->format) {
        set_error(ERROR_PROGRAMMER_ERROR, "No format selected");
        return -1;
    }

    return priv->format->type();
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
    GET_PIMPL_OR_RETURN({});

    if (!priv->format) {
        set_error(ERROR_PROGRAMMER_ERROR, "No format selected");
        return {};
    }

    return priv->format->name();
}

/*!
 * \brief Force support for a boot image format by its code.
 *
 * Calling this function causes the bidding process to be skipped. The chosen
 * format will be used regardless of which formats are enabled.
 *
 * \param code Boot image format code (\ref MB_BI_FORMAT_CODES)
 *
 * \return
 *   * #RET_OK if the format is successfully enabled
 *   * \<= #RET_WARN if an error occurs
 */
int Reader::set_format_by_code(int code)
{
    GET_PIMPL_OR_RETURN(RET_FATAL);
    ENSURE_STATE_OR_RETURN(ReaderState::NEW, RET_FATAL);
    FormatReader *format = nullptr;

    int ret = enable_format_by_code(code);
    if (ret < 0 && ret != RET_WARN) {
        return ret;
    }

    for (auto &f : priv->formats) {
        if ((FORMAT_BASE_MASK & f->type() & code)
                == (FORMAT_BASE_MASK & code)) {
            format = f.get();
            break;
        }
    }

    if (!format) {
        set_error(ERROR_INTERNAL_ERROR, "Enabled format not found");
        priv->state = ReaderState::FATAL;
        return RET_FATAL;
    }

    priv->format = format;

    return RET_OK;
}

/*!
 * \brief Force support for a boot image format by its name.
 *
 * Calling this function causes the bidding process to be skipped. The chosen
 * format will be used regardless of which formats are enabled.
 *
 * \param name Boot image format name (\ref MB_BI_FORMAT_NAMES)
 *
 * \return
 *   * #RET_OK if the format is successfully set
 *   * \<= #RET_WARN if an error occurs
 */
int Reader::set_format_by_name(const std::string &name)
{
    GET_PIMPL_OR_RETURN(RET_FATAL);
    ENSURE_STATE_OR_RETURN(ReaderState::NEW, RET_FATAL);
    FormatReader *format = nullptr;

    int ret = enable_format_by_name(name);
    if (ret < 0 && ret != RET_WARN) {
        return ret;
    }

    for (auto &f : priv->formats) {
        if (f->name() == name) {
            format = f.get();
            break;
        }
    }

    if (!format) {
        set_error(ERROR_INTERNAL_ERROR, "Enabled format not found");
        priv->state = ReaderState::FATAL;
        return RET_FATAL;
    }

    priv->format = format;

    return RET_OK;
}

/*!
 * \brief Enable support for all boot image formats.
 *
 * \return
 *   * #RET_OK if all formats are successfully enabled
 *   * \<= #RET_FAILED if an error occurs while enabling a format
 */
int Reader::enable_format_all()
{
    GET_PIMPL_OR_RETURN(RET_FATAL);
    ENSURE_STATE_OR_RETURN(ReaderState::NEW, RET_FATAL);

    for (auto it = reader_formats; it->func; ++it) {
        int ret = (this->*it->func)();
        if (ret != RET_OK && ret != RET_WARN) {
            return ret;
        }
    }

    return RET_OK;
}

/*!
 * \brief Enable support for a boot image format by its code.
 *
 * \param code Boot image format code (\ref MB_BI_FORMAT_CODES)
 *
 * \return
 *   * #RET_OK if the format is successfully enabled
 *   * #RET_WARN if the format is already enabled
 *   * \<= #RET_FAILED if an error occurs
 */
int Reader::enable_format_by_code(int code)
{
    GET_PIMPL_OR_RETURN(RET_FATAL);
    ENSURE_STATE_OR_RETURN(ReaderState::NEW, RET_FATAL);

    for (auto it = reader_formats; it->func; ++it) {
        if ((code & FORMAT_BASE_MASK) == (it->code & FORMAT_BASE_MASK)) {
            return (this->*it->func)();
        }
    }

    set_error(ERROR_PROGRAMMER_ERROR, "Invalid format code: %d", code);
    return RET_FAILED;
}

/*!
 * \brief Enable support for a boot image format by its name.
 *
 * \param name Boot image format name (\ref MB_BI_FORMAT_NAMES)
 *
 * \return
 *   * #RET_OK if the format was successfully enabled
 *   * #RET_WARN if the format is already enabled
 *   * \<= #RET_FAILED if an error occurs
 */
int Reader::enable_format_by_name(const std::string &name)
{
    GET_PIMPL_OR_RETURN(RET_FATAL);
    ENSURE_STATE_OR_RETURN(ReaderState::NEW, RET_FATAL);

    for (auto it = reader_formats; it->func; ++it) {
        if (name == it->name) {
            return (this->*it->func)();
        }
    }

    set_error(ERROR_PROGRAMMER_ERROR, "Invalid format name: %s", name.c_str());
    return RET_FAILED;
}

/*!
 * \brief Get error code for a failed operation.
 *
 * \note The return value is undefined if an operation did not fail.
 *
 * \return Error code for failed operation. If \>= 0, then the value is one of
 *         the \ref MB_BI_ERROR_CODES entries. If \< 0, then the error code is
 *         implementation-defined (usually `-errno` or `-GetLastError()`).
 */
int Reader::error()
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
 *         undefined.
 */
std::string Reader::error_string()
{
    GET_PIMPL_OR_RETURN({});

    return priv->error_string;
}

/*!
 * \brief Set error string for a failed operation.
 *
 * \sa Reader::set_error_v()
 *
 * \param error_code Error code
 * \param fmt `printf()`-style format string
 * \param ... `printf()`-style format arguments
 *
 * \return RET_OK if the error is successfully set or RET_FAILED if an
 *         error occurs
 */
int Reader::set_error(int error_code, const char *fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = set_error_v(error_code, fmt, ap);
    va_end(ap);

    return ret;
}

/*!
 * \brief Set error string for a failed operation.
 *
 * \sa Reader::set_error()
 *
 * \param error_code Error code
 * \param fmt `printf()`-style format string
 * \param ap `printf()`-style format arguments as a va_list
 *
 * \return RET_OK if the error is successfully set or RET_FAILED if an
 *         error occurs
 */
int Reader::set_error_v(int error_code, const char *fmt, va_list ap)
{
    GET_PIMPL_OR_RETURN(RET_FATAL);

    priv->error_code = error_code;
    return format_v(priv->error_string, fmt, ap) ? RET_OK : RET_FAILED;
}

}
}
