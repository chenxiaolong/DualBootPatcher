/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#pragma once

#include <string>

class FileBase
{
public:
    enum OpenMode : int {
        OpenRead,
        OpenWrite,
        OpenAppend
    };

    enum SeekOrigin : int {
        SeekCurrent,
        SeekBegin,
        SeekEnd
    };

    enum Error : int {
        ErrorInvalidFilename,
        ErrorInvalidOpenMode,
        ErrorInvalidSeekOrigin,
        ErrorFileIsNotOpen,
        ErrorEndOfFile,
        ErrorPlatformError
    };

    /*!
     * \brief Destructor
     *
     * The destructor will automatically call close() if isOpen() returns true.
     */
    virtual ~FileBase();

    /*!
     * \brief Open a file
     *
     * \param filename UTF-8 encoded filename
     * \param mode File open mode; one of OpenRead, OpenWrite, or OpenAppend
     *
     * \return True if the file was successfully opened. False otherwise, with
     *         the error set appropriately.
     */
    virtual bool open(const char *filename, int mode) = 0;
    virtual bool open(const std::string &filename, int mode) = 0;

    /*!
     * \brief Close the file
     *
     * \return True if the file was successfully closed. False otherwise, with
     *         the error set appropriately.
     */
    virtual bool close() = 0;

    /*!
     * \brief Check if the file is open
     *
     * \return True if the file is open. False otherwise.
     */
    virtual bool isOpen() = 0;

    /*!
     * \brief Read bytes from the file
     *
     * If the end of file is reached or if an error occurs, false is returned.
     * Use "error() == ErrorEndOfFile" to check for EOF.
     *
     * For example:
     *
     *     char buf[10240];
     *     uint64_t bytesRead;
     *     while (file.read(buf, sizeof(buf), &bytesRead)) {
     *         doSomething(buf, bytesRead);
     *     }
     *     if (file.error() != ErrorEndOfFile) {
     *         printf("Oh noes! Something bad happened: %s",
     *                file.errorString());
     *     }
     *
     * \param buf Buffer to read into
     * \param size Buffer size
     * \param bytesRead Bytes read (output parameter)
     *
     * \return Whether the read was successful
     */
    virtual bool read(void *buf, uint64_t size, uint64_t *bytesRead) = 0;

    /*!
     * \brief Write bytes to the file
     *
     * If an error occurs, false is returned.
     *
     * Example:
     *
     *     uint64_t bytesWritten;
     *     if (!file.write(buf, sizeof(buf), &bytesWritten)) {
     *         printf("Oh noes! Something bad happened: %s",
     *                file.errorString());
     *     }
     *
     * \param buf Buffer to write from
     * \param size Buffer size
     * \param bytesWritten Bytes written (output parameter)
     *
     * \return Whether the write was successful
     */
    virtual bool write(const void *buf, uint64_t size, uint64_t *bytesWritten) = 0;

    /*!
     * \brief Get file pointer position
     *
     * \note: Behavior is undefined if the position overflows a signed 64-bit
     *        integer (int64_t).
     *
     * \param pos File pointer position (output parameter)
     *
     * \return True if successful. Otherwise, false if an error occurs with the
     *         error set appropriately.
     */
    virtual bool tell(uint64_t *pos) = 0;

    /*!
     * \brief Set file pointer position
     *
     * \param offset Offset
     * \param origin From where to apply the offset. Must be one of SeekCurrent,
     *               SeekBegin, or SeekEnd.
     *
     * \return True if the file pointer position was successfully changed.
     *         False otherwise, with the error set appropriately.
     */
    virtual bool seek(int64_t offset, int origin) = 0;

    /*!
     * \brief Get the error code
     *
     * \note: This value is valid only if the return value of another function
     *        indicates an error.
     *
     * \return Error code
     */
    virtual int error() = 0;

    /*!
     * \brief Get the error string
     *
     * \note: This value is valid only if the return value of another function
     *        indicates an error.
     *
     * \return Error string
     */
    virtual std::string errorString();

protected:
    virtual std::string platformErrorString() = 0;
};
