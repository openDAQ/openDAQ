/*
 * Copyright 2022-2025 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file    intfid.h
 * @author  Martin Kraner
 * @date    03/03/2019
 * @version 1.0
 *
 * @brief Compile-time UUID version 5 calculation
 *
 */
#pragma once
#include <array>
#include <cstdint>
#include <coretypes/constexpr_string.h>

#ifndef BEGIN_NAMESPACE_OPENDAQ
    #define BEGIN_NAMESPACE_OPENDAQ       \
        namespace daq \
        {
#endif

#ifndef END_NAMESPACE_OPENDAQ
    #define END_NAMESPACE_OPENDAQ \
        }
#endif

#include <coretypes/sha1.h>

BEGIN_NAMESPACE_OPENDAQ

namespace Detail
{
    using Byte = uint8_t;

    constexpr void swapBytes(Byte* guid, int left, int right)
    {
        Byte temp = guid[left];
        guid[left] = guid[right];
        guid[right] = temp;
    }

    constexpr void swapByteOrder(Byte* const guid)
    {
        swapBytes(guid, 0, 3);
        swapBytes(guid, 1, 2);
        swapBytes(guid, 4, 5);
        swapBytes(guid, 6, 7);
    }
}  // namespace Detail

constexpr size_t combineHashCodes(size_t h1, size_t h2)
{
    return (((h1 << 5u) + h1) ^ h2);
}

struct IntfID
{
    uint32_t Data1{};
    uint16_t Data2{};
    uint16_t Data3{};

    union
    {
        std::array<uint8_t, 8> Data4;
        uint64_t Data4_UInt64;
    };

    template <std::size_t N>
    static constexpr IntfID FromTypeName(std::array<char, N> in)
    {
        // reinterpret casting not allowed have to manually convert
        unsigned char bytes[N]{};
        for (size_t i = 0; i < N; ++i)
        {
            bytes[i] = (unsigned char) in[i];
        }

        return FromTypeName<true, N>(bytes);
    }

    template <std::size_t N1, std::size_t N2>
    static constexpr IntfID FromTypeName(const char (&interfaceName)[N1], const char (&namespaceName)[N2])
    {
        constexpr auto N = N1 + N2 + 1 - 2; // + 1 separator - 2 trailing zeros

        // reinterpret casting not allowed have to manually convert
        unsigned char bytes[N]{};
        for (size_t i = 0; i < N1 - 1; ++i)
        {
            bytes[i] = (unsigned char) interfaceName[i];
        }

        bytes[N1 - 1] = '.';

        for (size_t i = N1, j = 0; i < N; ++i, ++j)
        {
            bytes[i] = (unsigned char) namespaceName[j];
        }

        return FromTypeName<false, N>(bytes);
    }

    template <std::size_t N>
    static constexpr IntfID FromTypeName(const char (&in)[N])
    {
        // reinterpret casting not allowed have to manually convert
        unsigned char bytes[N]{};
        for (size_t i = 0; i < N; ++i)
        {
            bytes[i] = (unsigned char) in[i];
        }

        return FromTypeName<true, N>(bytes);
    }

    template <std::size_t N>
    static constexpr IntfID FromTypeName(const ConstexprString<N>& in)
    {
        // reinterpret casting not allowed have to manually convert
        unsigned char bytes[N]{};
        for (size_t i = 0; i < N; ++i)
        {
            bytes[i] = (unsigned char) in[i];
        }

        return FromTypeName<false, N>(bytes);
    }

    template <bool HasTrailingZero, std::size_t N>
    static constexpr IntfID FromTypeName(const unsigned char (&in)[N])
    {
        constexpr size_t additionalSize = HasTrailingZero ? 15 : 16;
        constexpr size_t arraySize = N + additionalSize;

        // Prepend Namespace GUID in Network order
        Detail::Byte array[arraySize]{ 0x6b, 0xa7, 0xb8, 0x10, 0x9d, 0xad, 0x11, 0xd1, 0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8 };

        // copy input param bytes
        for (size_t i = 16, j = 0; i < arraySize; i++, j++)
        {
            array[i] = in[j];
        }

        const std::array<unsigned char, 20> sha1 = ct_sha1<false>(array);

        Detail::Byte guid[16]{};

        // copy first 16 bytes to destination guid
        for (int i = 0; i < 16; i++)
        {
            guid[i] = sha1[i];
        }

        // set the four most significant bits (bits 12 through 15) of the time_hi_and_version field to the appropriate 4-bit version number
        // from Section 4.1.3 (step 8)
        guid[6] = (guid[6] & 0x0F) | (5 << 4);

        // set the two most significant bits (bits 6 and 7) of the clock_seq_hi_and_reserved to zero and one, respectively (step 10)
        guid[8] = (guid[8] & 0x3F) | 0x80;

        // convert the resulting UUID to local byte order (step 13)
        Detail::swapByteOrder(guid);

        // create an uint32 from bytes
        uint32_t data1{};
        for (int i = 3; i >= 0; i--)
        {
            data1 |= guid[i];

            if (i != 0)
            {
                data1 <<= 8;
            }
        }

        // create an uint16 from bytes
        uint16_t data2{};
        for (int i = 1; i >= 0; i--)
        {
            data2 |= guid[i + 4];

            if (i != 0)
            {
                data2 <<= 8;
            }
        }

        // create an uint16 from bytes
        uint16_t data3{};
        for (int i = 1; i >= 0; i--)
        {
            data3 |= guid[i + 6];

            if (i != 0)
            {
                data3 <<= 8;
            }
        }

        IntfID id{};
        id.Data1 = data1;
        id.Data2 = data2;
        id.Data3 = data3;
        id.Data4[0] = guid[8];
        id.Data4[1] = guid[9];
        id.Data4[2] = guid[10];
        id.Data4[3] = guid[11];
        id.Data4[4] = guid[12];
        id.Data4[5] = guid[13];
        id.Data4[6] = guid[14];
        id.Data4[7] = guid[15];

        return id;
    }

    size_t getHashCode() const
    {
#if UINTPTR_MAX == UINT64_MAX
        const size_t* part1 = reinterpret_cast<const size_t*>(this);
        const size_t* part2 = reinterpret_cast<const size_t*>(&this->Data4_UInt64);
        return combineHashCodes(*part1, *part2);
#else
        const size_t* part1 = reinterpret_cast<const size_t*>(this);
        const size_t* part2 = reinterpret_cast<const size_t*>(&this->Data2);
        const size_t* part3 = reinterpret_cast<const size_t*>(&this->Data4[0]);
        const size_t* part4 = reinterpret_cast<const size_t*>(&this->Data4[4]);
        return combineHashCodes(*part1, combineHashCodes(*part2, combineHashCodes(*part3, *part4)));
#endif
    }

    friend bool operator==(const IntfID& lhs, const IntfID& rhs)
    {
#if UINTPTR_MAX == UINT64_MAX
        const size_t* lhs1 = reinterpret_cast<const size_t*>(&lhs.Data1);
        const size_t* lhs2 = reinterpret_cast<const size_t*>(&lhs.Data4_UInt64);
        const size_t* rhs1 = reinterpret_cast<const size_t*>(&rhs.Data1);
        const size_t* rhs2 = reinterpret_cast<const size_t*>(&rhs.Data4_UInt64);
        return (*lhs1 == *rhs1 && *lhs2 == *rhs2);
#else
        const size_t* lhs1 = reinterpret_cast<const size_t*>(&lhs.Data1);
        const size_t* lhs2 = reinterpret_cast<const size_t*>(&lhs.Data2);
        const size_t* lhs3 = reinterpret_cast<const size_t*>(&lhs.Data4[0]);
        const size_t* lhs4 = reinterpret_cast<const size_t*>(&lhs.Data4[4]);
        const size_t* rhs1 = reinterpret_cast<const size_t*>(&rhs.Data1);
        const size_t* rhs2 = reinterpret_cast<const size_t*>(&rhs.Data2);
        const size_t* rhs3 = reinterpret_cast<const size_t*>(&rhs.Data4[0]);
        const size_t* rhs4 = reinterpret_cast<const size_t*>(&rhs.Data4[4]);
        return (*lhs1 == *rhs1 && *lhs2 == *rhs2 && *lhs3 == *rhs3 && *lhs4 == *rhs4);
#endif
    }

    // this function is needed for compile time compare of UUIDs, because casting and accessing non active union members (Data4_UInt4)
    // is not allowed in compile time
    static constexpr bool Compare(const IntfID& lhs, const IntfID& rhs)
    {
        return lhs.Data1 == rhs.Data1 &&
               lhs.Data2 == rhs.Data2 &&
               lhs.Data3 == rhs.Data3 &&
               lhs.Data4[0] == rhs.Data4[0] &&
               lhs.Data4[1] == rhs.Data4[1] &&
               lhs.Data4[2] == rhs.Data4[2] &&
               lhs.Data4[3] == rhs.Data4[3] &&
               lhs.Data4[4] == rhs.Data4[4] &&
               lhs.Data4[5] == rhs.Data4[5] &&
               lhs.Data4[6] == rhs.Data4[6] &&
               lhs.Data4[7] == rhs.Data4[7];
    }
};

END_NAMESPACE_OPENDAQ
