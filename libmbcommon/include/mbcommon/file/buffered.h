/*
 * Copyright (C) 2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#pragma once

#include <functional>
#include <optional>
#include <type_traits>
#include <vector>

#include "mbcommon/file.h"
#include "mbcommon/span.h"

namespace mb
{

class MB_EXPORT BufferedFile : public File
{
public:
    static inline size_t DEFAULT_BUFFER_SIZE = 8 * 1024;
    static inline unsigned char DEFAULT_DELIM = '\n';

    BufferedFile(size_t buf_size = DEFAULT_BUFFER_SIZE);
    BufferedFile(File &file, size_t buf_size = DEFAULT_BUFFER_SIZE);
    virtual ~BufferedFile();

    BufferedFile(BufferedFile &&other) noexcept;
    BufferedFile & operator=(BufferedFile &&rhs) noexcept;

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(BufferedFile)

    oc::result<void> open(File &file);

    oc::result<void> close() override;

    oc::result<size_t> read(void *buf, size_t size) override;
    oc::result<size_t> write(const void *buf, size_t size) override;
    oc::result<uint64_t> seek(int64_t offset, int whence) override;
    oc::result<void> truncate(uint64_t size) override;

    bool is_open() override;

    oc::result<void> sync_pos();
    oc::result<void> flush();

    bool set_buf_size(size_t size);
    oc::result<void> fill_rbuf();
    void consume_rbuf(size_t n);
    span<unsigned char> rbuf();
    span<unsigned char> wbuf();

    template<
        typename Container,
        class = std::void_t<
            std::enable_if_t<sizeof(std::byte)
                    == sizeof(typename Container::value_type)>,
            std::enable_if_t<std::alignment_of_v<std::byte>
                    == std::alignment_of_v<typename Container::value_type>>
        >
    >
    oc::result<size_t> read_line(Container &buf,
                                 unsigned char delim = DEFAULT_DELIM)
    {
        using V = typename Container::value_type;

        return get_delim([&buf](span<const std::byte> data) {
            buf.insert(buf.end(), reinterpret_cast<const V *>(data.begin()),
                       reinterpret_cast<const V *>(data.end()));
        }, std::nullopt, delim);
    }

    template<
        typename Container,
        class = std::void_t<
            std::enable_if_t<sizeof(std::byte)
                    == sizeof(typename Container::value_type)>,
            std::enable_if_t<std::alignment_of_v<std::byte>
                    == std::alignment_of_v<typename Container::value_type>>
        >
    >
    oc::result<size_t> read_sized_line(Container &buf, size_t max_size,
                                       unsigned char delim = DEFAULT_DELIM)
    {
        using V = typename Container::value_type;

        return get_delim([&buf](span<const std::byte> data) {
            buf.insert(buf.end(), reinterpret_cast<const V *>(data.begin()),
                       reinterpret_cast<const V *>(data.end()));
        }, max_size, delim);
    }

    oc::result<size_t> read_sized_line(span<std::byte> buf,
                                       unsigned char delim = DEFAULT_DELIM);

private:
    /*! \cond INTERNAL */
    oc::result<size_t> read_underlying(void *buf, size_t size);
    oc::result<size_t> write_underlying(const void *buf, size_t size);
    oc::result<void> write_exact_underlying(const void *buf, size_t size);
    oc::result<uint64_t> seek_underlying(int64_t offset, int whence);

    using GetDelimFn = std::function<void(span<const std::byte>)>;

    oc::result<size_t> get_delim(const GetDelimFn &f,
                                 std::optional<size_t> max_size,
                                 unsigned char delim = DEFAULT_DELIM);

    void clear() noexcept;

    File *m_file;
    uint64_t m_fpos;
    std::vector<unsigned char> m_rbuf;
    size_t m_rpos;
    size_t m_rcap;
    std::vector<unsigned char> m_wbuf;
    /*! \endcond */
};

}
