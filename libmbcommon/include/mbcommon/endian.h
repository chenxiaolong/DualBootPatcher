/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#if defined(__linux__)
#  include <endian.h>

#  define mb_htobe16(x) htobe16(x)
#  define mb_htole16(x) htole16(x)
#  define mb_be16toh(x) be16toh(x)
#  define mb_le16toh(x) le16toh(x)

#  define mb_htobe32(x) htobe32(x)
#  define mb_htole32(x) htole32(x)
#  define mb_be32toh(x) be32toh(x)
#  define mb_le32toh(x) le32toh(x)

#  define mb_htobe64(x) htobe64(x)
#  define mb_htole64(x) htole64(x)
#  define mb_be64toh(x) be64toh(x)
#  define mb_le64toh(x) le64toh(x)

#  define MB_BYTE_ORDER    BYTE_ORDER
#  define MB_BIG_ENDIAN    BIG_ENDIAN
#  define MB_LITTLE_ENDIAN LITTLE_ENDIAN
#  define MB_PDP_ENDIAN    PDP_ENDIAN
#elif defined(__APPLE__)
#  include <libkern/OSByteOrder.h>

#  define mb_htobe16(x) OSSwapHostToBigInt16(x)
#  define mb_htole16(x) OSSwapHostToLittleInt16(x)
#  define mb_be16toh(x) OSSwapBigToHostInt16(x)
#  define mb_le16toh(x) OSSwapLittleToHostInt16(x)

#  define mb_htobe32(x) OSSwapHostToBigInt32(x)
#  define mb_htole32(x) OSSwapHostToLittleInt32(x)
#  define mb_be32toh(x) OSSwapBigToHostInt32(x)
#  define mb_le32toh(x) OSSwapLittleToHostInt32(x)

#  define mb_htobe64(x) OSSwapHostToBigInt64(x)
#  define mb_htole64(x) OSSwapHostToLittleInt64(x)
#  define mb_be64toh(x) OSSwapBigToHostInt64(x)
#  define mb_le64toh(x) OSSwapLittleToHostInt64(x)

#  define MB_BYTE_ORDER    BYTE_ORDER
#  define MB_BIG_ENDIAN    BIG_ENDIAN
#  define MB_LITTLE_ENDIAN LITTLE_ENDIAN
#  define MB_PDP_ENDIAN    PDP_ENDIAN
#elif defined(_WIN32)
#  include <sys/param.h>

#  if defined(_MSC_VER)
#    define MB_WIN32_SWAP_16 _byteswap_ushort
#    define MB_WIN32_SWAP_32 _byteswap_ulong
#    define MB_WIN32_SWAP_64 _byteswap_uint64
#  elif defined(__GNUC__)
#    define MB_WIN32_SWAP_16 __builtin_bswap16
#    define MB_WIN32_SWAP_32 __builtin_bswap32
#    define MB_WIN32_SWAP_64 __builtin_bswap64
#  else
#    error Unsupported compiler on Win32
#  endif

#  if BYTE_ORDER == LITTLE_ENDIAN
#    define mb_htobe16(x) MB_WIN32_SWAP_16(x)
#    define mb_htole16(x) (x)
#    define mb_be16toh(x) MB_WIN32_SWAP_16(x)
#    define mb_le16toh(x) (x)

#    define mb_htobe32(x) MB_WIN32_SWAP_32(x)
#    define mb_htole32(x) (x)
#    define mb_be32toh(x) MB_WIN32_SWAP_32(x)
#    define mb_le32toh(x) (x)

#    define mb_htobe64(x) MB_WIN32_SWAP_64(x)
#    define mb_htole64(x) (x)
#    define mb_be64toh(x) MB_WIN32_SWAP_64(x)
#    define mb_le64toh(x) (x)
#  elif BYTE_ORDER == BIG_ENDIAN
#    define mb_htobe16(x) (x)
#    define mb_htole16(x) MB_WIN32_SWAP_16(x)
#    define mb_be16toh(x) (x)
#    define mb_le16toh(x) MB_WIN32_SWAP_16(x)

#    define mb_htobe32(x) (x)
#    define mb_htole32(x) MB_WIN32_SWAP_32(x)
#    define mb_be32toh(x) (x)
#    define mb_le32toh(x) MB_WIN32_SWAP_32(x)

#    define mb_htobe64(x) (x)
#    define mb_htole64(x) MB_WIN32_SWAP_64(x)
#    define mb_be64toh(x) (x)
#    define mb_le64toh(x) MB_WIN32_SWAP_64(x)
#  else
#    error Unsupported byte order on Win32
#  endif

#  define MB_BYTE_ORDER    BYTE_ORDER
#  define MB_BIG_ENDIAN    BIG_ENDIAN
#  define MB_LITTLE_ENDIAN LITTLE_ENDIAN
#  define MB_PDP_ENDIAN    PDP_ENDIAN
#else
#  error Unsupported platform
#endif
