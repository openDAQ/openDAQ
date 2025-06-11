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

#include <memory>

#include <coretypes/exceptions.h>
#include <coretypes/filesystem.h>
#include <opendaq/opendaq.h>

#include <basic_csv_recorder_module/common.h>
#include <basic_csv_recorder_module/csv_writer.h>
#include <basic_csv_recorder_module/parquet_writer.h>

BEGIN_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE

/*!
 * @brief A generic writer class for writing data to CSV or Parquet files.
 */
class Writer
{
public:
    Writer(const fs::path& filename, const std::string& format)
    {
        if (format == Props::Format::FORMAT_CSV)
        {
            csvWriter = std::make_unique<CsvWriter>(filename);
        }
        else if (format == Props::Format::FORMAT_PARQUET)
        {
            parquetWriter = std::make_unique<ParquetWriter>(filename);
        }
        else
        {
            throw InvalidParameterException("Unsupported format: " + format);
        }
    }

    /*!
     * @brief Propagates the headers to the underlying writer.
     *
     * @param domainName The name of the domain column.
     * @param valueName The name of the value column.
     * @param auxDomainName The auxiliary (second line) name of the domain column.
     * @param auxValueName The auxiliary (second line) name of the value column.
     *
     */
    virtual void headers(const char* domainName,
                         const char* valueName,
                         const char* auxDomainName = nullptr,
                         const char* auxValueName = nullptr)
    {
        if (parquetWriter)
        {
            parquetWriter->headers(domainName, valueName, auxDomainName, auxValueName);
        }
        else if (csvWriter)
        {
            csvWriter->headers(domainName, valueName, auxDomainName, auxValueName);
        }
        else
        {
            throw InvalidStateException("No writer initialized");
        }
    }

    /*!
     * @brief Writes a data line using the underlying writer.
     *
     * @tparam Domain The type of the @p domainValue argument.
     * @tparam Sample The type of the @p sample argument.
     *
     * @param domainValue The domain value to write.
     * @param sample The sample value to write.
     *
     */

    template <typename Domain, typename Sample>
    void write(Domain domainValue, Sample sample)
    {
        if (parquetWriter)
        {
            parquetWriter->write(domainValue, sample);
        }
        else if (csvWriter)
        {
            csvWriter->write(domainValue, sample);
        }
        else
        {
            throw InvalidStateException("No writer initialized");
        }
    }

private:
    std::unique_ptr<ParquetWriter> parquetWriter;
    std::unique_ptr<CsvWriter> csvWriter;
};

END_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE
