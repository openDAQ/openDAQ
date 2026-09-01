/*
 * Copyright 2022-2026 openDAQ d.o.o.
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

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>
#include <string>

#include <opendaq/opendaq.h>

#include <file_recorder_module/common.h>

BEGIN_NAMESPACE_OPENDAQ_FILE_RECORDER_MODULE

/*!
 * @brief The on-disk format shared by FileRecorderFbImpl (writer) and FilePlayerFbImpl (reader).
 *
 * A file is laid out as follows, and is self-contained: a recording which is split across several
 * files (because of a size limit or a descriptor change) repeats the full header in every part, so
 * any part can be replayed on its own.
 *
 * @verbatim
 * "DAQREC\0\0"        MAGIC_SIZE bytes
 * uint16              format version, little-endian
 * uint32              header length in bytes, little-endian
 * header              JSON, `headerLength` bytes; an openDAQ Dict serialized with JsonSerializer
 *
 * ... followed by a sequence of blocks, one per recorded data packet:
 *
 * uint8               DomainKind
 * uint64              sample count, little-endian
 * domain start        only when DomainKind::Implicit; raw, in the domain descriptor's sample type
 * value samples       sampleCount * valueDescriptor.getRawSampleSize() bytes, raw
 * domain samples      only when DomainKind::Explicit; sampleCount * domain raw sample size, raw
 * @endverbatim
 *
 * Sample payloads are written in the host's native byte order, exactly as they are laid out in
 * packet memory. The header records which order that was, and the reader refuses a file whose
 * order does not match the host rather than returning silently wrong values.
 */
namespace file_format
{

inline constexpr std::size_t MAGIC_SIZE = 8;
inline constexpr char MAGIC[MAGIC_SIZE] = {'D', 'A', 'Q', 'R', 'E', 'C', '\0', '\0'};

/*!
 * @brief The version of the format written by this module. The reader accepts this version only.
 */
inline constexpr std::uint16_t VERSION = 1;

/*!
 * @brief The filename extension used for recordings, including the leading period.
 */
inline constexpr const char* FILE_EXTENSION = ".daqrec";

/*!
 * @brief The number of bytes preceding the JSON header: magic, version and header length.
 */
inline constexpr std::size_t PREAMBLE_SIZE = MAGIC_SIZE + sizeof(std::uint16_t) + sizeof(std::uint32_t);

/*!
 * @brief Describes how the domain of a block's samples is stored.
 */
enum class DomainKind : std::uint8_t
{
    /*!
     * @brief The recorded signal had no domain signal. The block carries value samples only.
     */
    None = 0,

    /*!
     * @brief The domain is implicit (a linear rule). The block carries the absolute domain value
     *     of its first sample; the remaining ticks follow from the rule's delta, which is part of
     *     the domain descriptor in the file header.
     */
    Implicit = 1,

    /*!
     * @brief The domain is explicit. The block carries one domain value per sample.
     */
    Explicit = 2,
};

/*!
 * @brief The keys of the JSON header object.
 */
namespace header_key
{
inline constexpr const char* SIGNAL_NAME = "signalName";
inline constexpr const char* SIGNAL_GLOBAL_ID = "signalGlobalId";
inline constexpr const char* VALUE_DESCRIPTOR = "valueDescriptor";
inline constexpr const char* DOMAIN_DESCRIPTOR = "domainDescriptor";
inline constexpr const char* PART_INDEX = "partIndex";
inline constexpr const char* CREATED_AT = "createdAt";
inline constexpr const char* LITTLE_ENDIAN_SAMPLES = "littleEndianSamples";
}

/*!
 * @brief The decoded contents of a file header.
 */
struct Header
{
    /*!
     * @brief The name of the recorded signal, at the time recording started.
     */
    StringPtr signalName;

    /*!
     * @brief The global ID of the recorded signal, at the time recording started.
     */
    StringPtr signalGlobalId;

    /*!
     * @brief The descriptor of the recorded signal's value.
     */
    DataDescriptorPtr valueDescriptor;

    /*!
     * @brief The descriptor of the recorded signal's domain signal, or an unassigned pointer if
     *     the signal had no domain signal.
     */
    DataDescriptorPtr domainDescriptor;

    /*!
     * @brief The 1-based index of this file within the recording it belongs to.
     */
    Int partIndex = 1;

    /*!
     * @brief The time the file was created, as an ISO 8601 UTC string.
     */
    StringPtr createdAt;
};

/*!
 * @brief The fixed-size part of a block, preceding its payload.
 */
struct BlockHeader
{
    DomainKind domainKind = DomainKind::None;
    std::uint64_t sampleCount = 0;
};

/*!
 * @brief Returns true if the host stores multi-byte scalars least-significant byte first.
 */
bool isHostLittleEndian();

/*!
 * @brief Formats @p timePoint as an ISO 8601 UTC string, e.g. "2026-09-01T12:00:00Z".
 */
StringPtr toIso8601(const std::chrono::system_clock::time_point& timePoint);

/*!
 * @brief Writes the magic, version, and JSON header to @p stream.
 * @param stream The stream to write to, positioned at the start of the file.
 * @param header The header to write.
 * @returns The number of bytes written.
 * @throws std::ios_base::failure Writing failed.
 */
std::size_t writeHeader(std::ostream& stream, const Header& header);

/*!
 * @brief Reads and validates the magic, version, and JSON header from @p stream.
 * @param stream The stream to read from, positioned at the start of the file.
 * @param header Populated with the decoded header on success.
 * @throws std::runtime_error The file is not a recording, its version is unsupported, its sample
 *     byte order does not match the host's, or the header is malformed or truncated.
 */
void readHeader(std::istream& stream, Header& header);

/*!
 * @brief Writes a block header to @p stream. The caller writes the payload that follows.
 * @throws std::ios_base::failure Writing failed.
 */
void writeBlockHeader(std::ostream& stream, const BlockHeader& blockHeader);

/*!
 * @brief Reads a block header from @p stream.
 * @param stream The stream to read from, positioned at the start of a block.
 * @param blockHeader Populated with the decoded block header on success.
 * @returns False if the stream was already at end of file, true if a block header was read.
 * @throws std::runtime_error The block header is truncated or its domain kind is unrecognized.
 */
bool readBlockHeader(std::istream& stream, BlockHeader& blockHeader);

/*!
 * @brief Returns the number of payload bytes following a block header, given the sizes of one
 *     raw value sample and one raw domain sample.
 */
std::size_t blockPayloadSize(const BlockHeader& blockHeader, std::size_t valueSampleSize, std::size_t domainSampleSize);

/*!
 * @brief Returns true if @p sampleType is Float32 or Float64.
 *
 * Domain ticks are almost always integers, but a floating-point domain is legal. The reader and
 * the player branch on this to do their tick arithmetic in the widest type that stays exact for
 * the domain at hand: `Int` for integer ticks, `double` for floating-point ones.
 */
bool isFloatingPointSampleType(SampleType sampleType);

/*!
 * @brief Reads one raw domain sample as a double.
 *
 * Used for pacing decisions only, where a few ulps do not matter. Values which must be emitted
 * into an output packet are read and written with readTickAsInt() / writeTickAsInt() instead, so
 * that integer ticks survive unchanged.
 *
 * @param raw The address of the sample.
 * @param sampleType The sample type described by the domain descriptor.
 * @throws std::runtime_error @p sampleType is not a supported domain sample type.
 */
double readTickAsDouble(const void* raw, SampleType sampleType);

/*!
 * @brief Reads one raw domain sample as an `Int`.
 * @throws std::runtime_error @p sampleType is not a supported integer domain sample type.
 */
Int readTickAsInt(const void* raw, SampleType sampleType);

/*!
 * @brief Writes @p tick into one raw domain sample.
 * @throws std::runtime_error @p sampleType is not a supported integer domain sample type.
 */
void writeTickAsInt(void* raw, SampleType sampleType, Int tick);

/*!
 * @brief Writes @p tick into one raw floating-point domain sample.
 * @throws std::runtime_error @p sampleType is neither Float32 nor Float64.
 */
void writeTickAsDouble(void* raw, SampleType sampleType, double tick);

/*!
 * @brief Returns the number of seconds one domain tick represents, according to the tick
 *     resolution of @p descriptor. Returns 0 if @p descriptor has no tick resolution, which
 *     leaves the caller to decide what an unspecified domain unit should mean.
 */
double tickResolutionSeconds(const DataDescriptorPtr& descriptor);

}

END_NAMESPACE_OPENDAQ_FILE_RECORDER_MODULE
