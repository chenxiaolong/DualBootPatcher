/*
 * This is taken from "Apache-2.0 WITH LLVM-exception"-licensed code from LLVM.
 * The license can be found at: https://llvm.org/LICENSE.txt
 *
 * The code has been modified to better integrate into the DualBootPatcher
 * libraries.
 */

#pragma once

#include "mbbootimg/guard_p.h"

#include <array>
#include <cstdint>

#include "mbcommon/span.h"

namespace mb::bootimg::detail
{

/// A class that wrap the SHA1 algorithm.
class SHA1
{
public:
    static constexpr size_t HASH_LENGTH = 20;

    SHA1();

    /// Reinitialize the internal state
    void init();

    /// Digest more data.
    void update(span<const unsigned char> Data);

    /// Return a reference to the current raw 160-bits SHA1 for the digested
    /// data since the last call to init(). This call will add data to the
    /// internal state and as such is not suited for getting an intermediate
    /// result (see result()).
    span<const unsigned char> final();

    /// Return a reference to the current raw 160-bits SHA1 for the digested
    /// data since the last call to init(). This is suitable for getting the
    /// SHA1 at any time without invalidating the internal state so that more
    /// calls can be made into update.
    span<const unsigned char> result();

    /// Returns a raw 160-bit SHA1 hash for the given data.
    static std::array<uint8_t, HASH_LENGTH> hash(span<const unsigned char> Data);

private:
    static constexpr size_t BLOCK_LENGTH = 64;

    // Internal State
    struct
    {
        union
        {
            uint8_t C[BLOCK_LENGTH];
            uint32_t L[BLOCK_LENGTH / 4];
        } Buffer;
        uint32_t State[HASH_LENGTH / 4];
        uint32_t ByteCount;
        uint8_t BufferOffset;
    } InternalState;

    // Internal copy of the hash, populated and accessed on calls to result()
    uint32_t HashResult[HASH_LENGTH / 4];

    // Helper
    void writebyte(uint8_t data);
    void hashBlock();
    void addUncounted(uint8_t data);
    void pad();
};

}
