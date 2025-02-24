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
#include <filesystem>
#include <fstream>
#include <sstream>

#include <opendaq/opendaq.h>

#include <basic_csv_recorder_module/common.h>

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
class CsvWriter
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
        CsvWriter(const std::filesystem::path& filename)
        {
            if (filename.has_parent_path())
                std::filesystem::create_directories(filename.parent_path());

            file.exceptions(std::ios::failbit | std::ios::badbit);
            file.open(filename);
        }

        /*!
         * @brief Writes a header line to the CSV file.
         *
         * There is no protection against writing headers after data has been written, or against
         * writing multiple header lines; this is the responsibility of the caller.
         *
         * @param domainName The name of the domain column.
         * @param valueName The name of the value column.
         *
         * @throws std::ios_base::failure The header line could not be written to the file due to
         *     an I/O error.
         */
        void headers(const char *domainName, const char *valueName)
        {
            file << quoteHeader(domainName) << ',' << quoteHeader(valueName) << '\n';
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
            // invoking operator+() promotes character types to numerically-printable types
            file << +domainValue << ',' << +sample << '\n';
        }

    private:

        /*!
         * @brief Quotes a header string by enclosing it in double-quotes.
         *
         * Double-quotes within the @p header itself are escaped by replacing them with two
         * double-quotes.
         *
         * @param header The string to quote.
         * @return The quoted string.
         */
        std::string quoteHeader(const std::string& header)
        {
            std::ostringstream quoted;
            quoted << '"';
            for (const auto& ch : header)
                if (ch == '"')
                    quoted << "\"\"";
                else
                    quoted << ch;
            quoted << '"';
            return quoted.str();
        }

        std::ofstream file;
};

END_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE
