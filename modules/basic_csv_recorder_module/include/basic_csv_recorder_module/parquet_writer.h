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

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <type_traits>

#include <arrow/api.h>
#include <arrow/type.h>
#include <arrow/io/file.h>
#include <arrow/util/key_value_metadata.h>
#include <parquet/stream_writer.h>

#include <basic_csv_recorder_module/common.h>
#include <basic_csv_recorder_module/type_resolver.h>

#include <coretypes/filesystem.h>
#include <opendaq/opendaq.h>

BEGIN_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE

/*!
 * @brief A header-only class for writing single-column CSV files.
 *
 * Writer objects own a std::ofstream handle, and the class provides templated functions for
 * writing data lines to the file as well as a function to write a header line.
 *
 * @remarks Platform-standard buffering and line endings from the C++ ostream library are used.
 *     Output is not explicitly flushed.
 *
 * @todo In the future, it may be desirable to add some control over the output formatting: for
 *     example, specifying the level of precision for floating-point values.
 * @todo Optimizations could be provided for signals with linear-rule domains.
 */
class ParquetWriter
{
public:
    /*!
     * @brief Opens a new CSV file.
     *
     * @param filename The path and filename (including extension) of the file to open.
     *     Relative paths are interpreted relative to the current working directory. If the
     *     file already exists, it is replaced by a new empty file.
     *
     * @throws std::ios_base::failure The file could not be opened.
     */
    ParquetWriter(const fs::path& filename): path(filename)
    {
    }

    /*!
     * @brief Writes a header line to the CSV file.
     *
     * Either one or two header lines can be emitted. If @p auxDomainName and @p auxValueName
     * are both empty strings, only one header line is emitted. Otherwise, a second line is
     * emitted using the auxiliary strings (if one of the parameters is a `nullptr` an empty
     * string is used for that column).
     *
     * There is no protection against writing headers after data has been written, or against
     * writing multiple header lines; this is the responsibility of the caller.
     *
     * @param domainName The name of the domain column.
     * @param valueName The name of the value column.
     * @param auxDomainName The auxiliary (second line) name of the domain column.
     * @param auxValueName The auxiliary (second line) name of the value column.
     *
     * @throws std::ios_base::failure The header line could not be written to the file due to
     *     an I/O error.
     */
    void headers(const char* domainName, const char* valueName, const char* auxDomainName = nullptr, const char* auxValueName = nullptr)
    {
        this->domainName = domainName ? domainName : "";
        this->valueName = valueName ? valueName : "";
        this->auxDomainName = auxDomainName ? auxDomainName : "";
        this->auxValueName = auxValueName ? auxValueName : "";
    }

    /*!
     * @brief Writes a data line to the CSV file.
     *
     * Standard output conversions are used for the specified data types. Generally this means
     * decimal values for integer types, and scientific-notation values for floating-point
     * types.
     *
     * @tparam Domain The type of the @p domainValue argument.
     * @tparam Sample The type of the @p sample argument.
     *
     * @param domainValue The domain value to write.
     * @param sample The sample value to write.
     *
     * @throws std::ios_base::failure The data line could not be written to the file due to an
     *     I/O error.
     */
    template <typename Domain, typename Sample>
    void write(Domain domainValue, Sample sample)
    {
        if (!initialized)
            initialized = initialize<Domain, Sample>();

        std::optional<std::string> metaDomain;
        std::optional<std::string> metaValue;
        if(!headersWritten) {
            if (!auxDomainName.empty() && !auxValueName.empty()) {
                metaDomain = auxDomainName;
                metaValue = auxValueName;
                headersWritten = true;
            }
        }

        os << metaDomain << metaValue << domainValue << sample << parquet::EndRow;
        ++rowCount;

        if (rowCount % 1000 == 0) {
            os << parquet::EndRowGroup;
            rowCount = 0;
        }
    }

private:

    template <typename Domain, typename Sample>
    bool initialize()
    {
        parquet::schema::NodeVector fields;

        if(!auxDomainName.empty()) {
            fields.push_back(parquet::schema::PrimitiveNode::Make(
                "meta_" + domainName, parquet::Repetition::OPTIONAL, parquet::Type::BYTE_ARRAY,
                parquet::ConvertedType::UTF8));
        }

        if(!auxValueName.empty()) {
            fields.push_back(parquet::schema::PrimitiveNode::Make(
                "meta_" + valueName, parquet::Repetition::OPTIONAL, parquet::Type::BYTE_ARRAY,
                parquet::ConvertedType::UTF8));
        }

        if constexpr (std::is_same_v<Domain, std::uint64_t>) {
            fields.push_back(parquet::schema::PrimitiveNode::Make(
                domainName, parquet::Repetition::REQUIRED, parquet::Type::INT64,
                parquet::ConvertedType::TIMESTAMP_MICROS));
        } else {
            fields.push_back(parquet::schema::PrimitiveNode::Make(
                domainName, parquet::Repetition::REQUIRED, get_parquet_type<Domain>(),
                get_parquet_converted_type<Domain>()));
        }

        fields.push_back(parquet::schema::PrimitiveNode::Make(
            valueName, parquet::Repetition::REQUIRED, get_parquet_type<Sample>(),
            get_parquet_converted_type<Sample>()));

        // open the file for writing
        if (path.has_parent_path())
            fs::create_directories(path.parent_path());

        parquet::WriterProperties::Builder builder;

        // builder.compression(arrow::Compression::SNAPPY);

        try {
            PARQUET_ASSIGN_OR_THROW(outfile, arrow::io::FileOutputStream::Open(path.string()));
        } catch (const std::exception& e)
        {
            throw std::ios_base::failure("Failed to create directories for Parquet file: " + std::string(e.what()));
        }

        os = parquet::StreamWriter{parquet::ParquetFileWriter::Open(outfile, std::static_pointer_cast<parquet::schema::GroupNode>(parquet::schema::GroupNode::Make("schema", parquet::Repetition::REQUIRED, fields)), builder.build())};

        return true;
    }

    fs::path path;

    std::string domainName;
    std::string valueName;
    std::string auxDomainName;
    std::string auxValueName;

    std::shared_ptr<arrow::io::FileOutputStream> outfile;
    parquet::StreamWriter os;

    uint64_t rowCount = 0;

    bool initialized = false;
    bool headersWritten = false;
};

END_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE
