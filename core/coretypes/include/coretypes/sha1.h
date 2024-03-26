/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
 * @file    sha1.h
 * @author  Martin Kraner
 * @date    04/03/2019
 * @version 1.0
 *
 * @brief Compile time SHA-1 hash calculation.
 *
 */

#pragma once
#include <cstdint>
#include <array>
#include <stdexcept>

BEGIN_NAMESPACE_OPENDAQ

namespace Detail
{
    enum class Sha1Status
    {
        Ok,
        InvalidArgument,
        Overflow,
    };

    struct Sha1State
    {
        constexpr Sha1State()
            : length(0)
            , state{ 0x67452301UL, 0xefcdab89UL, 0x98badcfeUL, 0x10325476UL, 0xc3d2e1f0UL }
            , currentLength(0)
            , buffer{}
        {
        }

        uint64_t length;
        uint32_t state[5];
        uint32_t currentLength;
        unsigned char buffer[64];
    };

    #define F0(x,y,z)   (z ^ (x & (y ^ z)))
    #define F1(x,y,z)   (x ^ y ^ z)
    #define F2(x,y,z)   ((x & y) | (z & (x | y)))
    #define F3(x,y,z)   (x ^ y ^ z)

    /* rotates the hard way */
    #define ROL(x, y)   ( (((uint32_t)(x)<<(uint32_t)((y)&31)) | (((uint32_t)(x)&0xFFFFFFFFUL)>>(uint32_t)((32-((y)&31))&31))) & 0xFFFFFFFFUL)
    #define ROR(x, y)   ( ((((uint32_t)(x)&0xFFFFFFFFUL)>>(uint32_t)((y)&31)) | ((uint32_t)(x)<<(uint32_t)((32-((y)&31))&31))) & 0xFFFFFFFFUL)
    #define ROLc(x, y)  ( (((uint32_t)(x)<<(uint32_t)((y)&31)) | (((uint32_t)(x)&0xFFFFFFFFUL)>>(uint32_t)((32-((y)&31))&31))) & 0xFFFFFFFFUL)
    #define RORc(x, y)  ( ((((uint32_t)(x)&0xFFFFFFFFUL)>>(uint32_t)((y)&31)) | ((uint32_t)(x)<<(uint32_t)((32-((y)&31))&31))) & 0xFFFFFFFFUL)

    #define STORE64H(x, y)                                                                         \
        do { (y)[0] = (unsigned char)(((x)>>56)&255); (y)[1] = (unsigned char)(((x)>>48)&255);     \
             (y)[2] = (unsigned char)(((x)>>40)&255); (y)[3] = (unsigned char)(((x)>>32)&255);     \
             (y)[4] = (unsigned char)(((x)>>24)&255); (y)[5] = (unsigned char)(((x)>>16)&255);     \
             (y)[6] = (unsigned char)(((x)>>8)&255); (y)[7] = (unsigned char)((x)&255); } while(0)

    #define STORE32H(x, y)                                                                         \
        do { (y)[0] = (unsigned char)(((x)>>24)&255); (y)[1] = (unsigned char)(((x)>>16)&255);     \
             (y)[2] = (unsigned char)(((x)>>8)&255); (y)[3] = (unsigned char)((x)&255); } while(0)

    #define LOAD32H(x, y)                         \
        do { x = ((uint32_t)((y)[0] & 255)<<24) | \
                 ((uint32_t)((y)[1] & 255)<<16) | \
                 ((uint32_t)((y)[2] & 255)<<8)  | \
                 ((uint32_t)((y)[3] & 255)); } while(0)

    #ifndef MIN
        #define MIN(x, y)   ( ((x)<(y))?(x):(y) )
    #endif

    constexpr Sha1Status sha1Compress(Sha1State& md, const unsigned char* buffer)
    {
        uint32_t W[80]{};
        uint32_t i = 0;

        /* copy the state into 512-bits into W[0..15] */
        for (i = 0; i < 16; i++)
        {
            LOAD32H(W[i], buffer + (4 * i));
        }

        /* copy state */
        uint64_t a = md.state[0];
        uint64_t b = md.state[1];
        uint64_t c = md.state[2];
        uint64_t d = md.state[3];
        uint64_t e = md.state[4];

        /* expand it */
        for (i = 16; i < 80; i++)
        {
            W[i] = ROL(W[i - 3] ^ W[i - 8] ^ W[i - 14] ^ W[i - 16], 1);
        }

#pragma push_macro("FF0")
#pragma push_macro("FF1")
#pragma push_macro("FF2")
#pragma push_macro("FF3")

        #undef FF0
        #undef FF1
        #undef FF2
        #undef FF3

        /* compress */
        /* round one */
        #define FF0(a,b,c,d,e,i) e = (ROLc(a, 5) + F0(b,c,d) + e + W[i] + 0x5a827999UL); b = ROLc(b, 30);
        #define FF1(a,b,c,d,e,i) e = (ROLc(a, 5) + F1(b,c,d) + e + W[i] + 0x6ed9eba1UL); b = ROLc(b, 30);
        #define FF2(a,b,c,d,e,i) e = (ROLc(a, 5) + F2(b,c,d) + e + W[i] + 0x8f1bbcdcUL); b = ROLc(b, 30);
        #define FF3(a,b,c,d,e,i) e = (ROLc(a, 5) + F3(b,c,d) + e + W[i] + 0xca62c1d6UL); b = ROLc(b, 30);

        for (i = 0; i < 20;)
        {
            FF0(a, b, c, d, e, i++);
            FF0(e, a, b, c, d, i++);
            FF0(d, e, a, b, c, i++);
            FF0(c, d, e, a, b, i++);
            FF0(b, c, d, e, a, i++);
        }

        /* round two */
        for (; i < 40;)
        {
            FF1(a, b, c, d, e, i++);
            FF1(e, a, b, c, d, i++);
            FF1(d, e, a, b, c, i++);
            FF1(c, d, e, a, b, i++);
            FF1(b, c, d, e, a, i++);
        }

        /* round three */
        for (; i < 60;)
        {
            FF2(a, b, c, d, e, i++);
            FF2(e, a, b, c, d, i++);
            FF2(d, e, a, b, c, i++);
            FF2(c, d, e, a, b, i++);
            FF2(b, c, d, e, a, i++);
        }

        /* round four */
        for (; i < 80;)
        {
            FF3(a, b, c, d, e, i++);
            FF3(e, a, b, c, d, i++);
            FF3(d, e, a, b, c, i++);
            FF3(c, d, e, a, b, i++);
            FF3(b, c, d, e, a, i++);
        }

        #undef FF0
        #undef FF1
        #undef FF2
        #undef FF3

#pragma push_macro("FF0")
#pragma push_macro("FF1")
#pragma push_macro("FF2")
#pragma push_macro("FF3")

        /* store */
        md.state[0] = (uint32_t)(md.state[0] + a);
        md.state[1] = (uint32_t)(md.state[1] + b);
        md.state[2] = (uint32_t)(md.state[2] + c);
        md.state[3] = (uint32_t)(md.state[3] + d);
        md.state[4] = (uint32_t)(md.state[4] + e);

        return Sha1Status::Ok;
    }

    /**
       Process a block of memory though the hash
       @tparam HasTrailingZero Whether the input data is a null terminated string
       @tparam N The length of the input data array.
       @param md The hash state
       @param in The data to hash
       @return Sha1Status::Ok if successful
    */
    template <bool HasTrailingZero = false, std::uint32_t N>
    constexpr Sha1Status sha1Process(Sha1State& md, const unsigned char(&in)[N])
    {
        std::uint32_t inLength = HasTrailingZero ? N - 1 : N;

        const unsigned char* ptr = in;

        if (md.currentLength > sizeof(md.buffer))
        {
            return Sha1Status::InvalidArgument;
        }

        if ((md.length + inLength) < md.length)
        {
            return Sha1Status::Overflow;
        }

        Sha1Status err = Sha1Status::Ok;
        while (inLength > 0)
        {
            if (md.currentLength == 0 && inLength >= 64)
            {
                if ((err = sha1Compress(md, ptr)) != Sha1Status::Ok)
                {
                    return err;
                }
                md.length += 64 * 8;
                ptr += 64;
                inLength -= 64;
            }
            else
            {
                const uint32_t n = MIN(inLength, (64 - md.currentLength));
                unsigned char* const dest = md.buffer + md.currentLength;

                for (std::size_t i = 0; i < n; i++)
                {
                    dest[i] = ptr[i];
                }

                md.currentLength += n;
                ptr += n;
                inLength -= n;

                if (md.currentLength == 64)
                {
                    if ((err = sha1Compress(md, (const unsigned char*)&md.buffer)) != Sha1Status::Ok)
                    {
                        return err;
                    }
                    md.length += 8 * 64;
                    md.currentLength = 0;
                }
            }
        }
        return Sha1Status::Ok;
    }

    /**
       Terminate the hash to get the digest
       @param md  The hash state
       @param out [out] The destination of the hash (20 bytes)
       @return Sha1Status::Ok if successful
    */
    constexpr Sha1Status sha1Done(Sha1State& md, unsigned char* const out)
    {
        if (md.currentLength >= sizeof(md.buffer))
        {
            return Sha1Status::InvalidArgument;
        }

        /* increase the length of the message */
        md.length += md.currentLength * 8;

        /* append the '1' bit */
        md.buffer[md.currentLength++] = 0x80;

        /* if the length is currently above 56 bytes we append zeros
         * then compress. Then we can fall back to padding zeros and length
         * encoding like normal.
         */
        if (md.currentLength > 56)
        {
            while (md.currentLength < 64)
            {
                md.buffer[md.currentLength++] = 0;
            }
            sha1Compress(md, md.buffer);
            md.currentLength = 0;
        }

        /* pad up to 56 bytes of zeroes */
        while (md.currentLength < 56)
        {
            md.buffer[md.currentLength++] = 0;
        }

        /* store length */
        STORE64H(md.length, md.buffer + 56);
        sha1Compress(md, md.buffer);

        /* copy output */
        for (int i = 0; i < 5; i++)
        {
            STORE32H(md.state[i], out + (4 * i));
        }
        return Sha1Status::Ok;
    }

}

/**
 * Calculates the SHA-1 digest in compile-time if the arguments are constexpr.
 * @tparam HasTrailingZero Whether the input data is a null terminated string
 * @tparam N The length of the input data array.
 * @param in The input data to hash.
 * @return The 20 bytes representing a Sha1 digest of the input data.
 */
template <bool HasTrailingZero = false, std::size_t N>
constexpr std::array<unsigned char, 20> ct_sha1(const unsigned char (&in)[N])
{
    Detail::Sha1State md;
    Detail::Sha1Status status = Detail::sha1Process<HasTrailingZero, N>(md, in);
    if (status == Detail::Sha1Status::InvalidArgument)
    {
        throw std::runtime_error("Invalid argument");
    }

    if (status == Detail::Sha1Status::Overflow)
    {
        throw std::runtime_error("Overflow");
    }

#if __cplusplus >= 201703L
    std::array<unsigned char, 20> tmp{};
    Detail::sha1Done(md, &tmp[0]);
#else
    // Undefined behaviour hack
    // but works on MSVC/GCC but not on Clang.
    const std::array<unsigned char, 20> tmp{};
    Detail::sha1Done(md, (unsigned char* const) &tmp[0]);
#endif

    return tmp;
}

END_NAMESPACE_OPENDAQ
