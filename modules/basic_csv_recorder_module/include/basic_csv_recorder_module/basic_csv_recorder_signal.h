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

#include <filesystem>

#include <opendaq/opendaq.h>

#include <basic_csv_recorder_module/common.h>
#include <basic_csv_recorder_module/csv_writer.h>

BEGIN_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE

/*!
 * Records a single openDAQ signal in a CSV file. Objects are constructed with a reference to the
 * openDAQ signal object, and the filesystem path where the CSV file should be created. Then the
 * caller calls onPacketReceived() (ideally from a worker thread) to synchronously record packet
 * data. The object owns a CsvWriter object which handles the actual writing.
 */
class BasicCsvRecorderSignal
{
    public:

        /*!
         * Opens a CSV file for the specified signal.
         *
         * @param path The directory in which to open a CSV file for this signal. The filename
         *     is generated by sanitizing the @p signal object's global ID and appending a ".csv"
         *     suffix.
         * @param signal The openDAQ signal object to be recorded.
         *
         * @throws std::ios_base::failure The CSV file could not be opened.
         */
        BasicCsvRecorderSignal(
            std::filesystem::path path,
            const SignalPtr& signal);

        /*!
         * @brief Records the values in a packet to the CSV file.
         *
         * @param packet The packet received.
         *
         * @throws std::ios_base::failure A header or data line could not be written to the CSV
         *     file due to an I/O error.
         */
        void onPacketReceived(const PacketPtr& packet);

    private:

        /*!
         * @brief Responds to input port events.
         *
         * The first descriptor-change event received causes headers to be written to the CSV
         * file, if they have not already been written. Subsequent descriptor-change events, or
         * descriptor-change events when headers have already been written, cause the CSV file
         * to be closed, because descriptor changes during recording are not supported. All other
         * event types are ignored.
         *
         * @param packet The event packet.
         *
         * @throws std::ios_base::failure A header line could not be written to the CSV file due
         *     to an I/O error.
         */
        void onEventPacketReceived(const EventPacketPtr packet);

        /*!
         * @brief Records the values the specified packet to the CSV file.
         *
         * @todo Templated writer functions are selected based on the packet's sample type. This
         *     involves inspecting the descriptor for every packet. This could be optimized by
         *     inspecting the descriptor only when it changes, and caching pointers to the
         *     appropriate writer function.
         *
         * @param packet The data packet to record.
         *
         * @throws std::ios_base::failure A data line could not be written to the CSV file due to
         *     an I/O error.
         */
        void onDataPacketReceived(const DataPacketPtr packet);

        /*!
         * @brief Writes headers to the CSV file, or closes the file if headers were already
         *     written.
         *
         * @param descriptor The data descriptor of the value signal.
         * @param domainDescriptor The data descriptor of the domain signal.
         *
         * @throws std::ios_base::failure The header line could not be written to the CSV file due
         *     to an I/O error.
         */
        void tryWriteHeaders(
            const DataDescriptorPtr& descriptor,
            const DataDescriptorPtr& domainDescriptor);

        CsvWriter writer;
        bool headersWritten = false;
};

END_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE
