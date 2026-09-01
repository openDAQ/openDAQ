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

#include <copendaq/reader/duration_tail_reader.h>

#include <opendaq/opendaq.h>

#include <copendaq_private.h>

const daqIntfID DAQ_DURATION_TAIL_READER_INTF_ID = { daq::IDurationTailReader::Id.Data1, daq::IDurationTailReader::Id.Data2, daq::IDurationTailReader::Id.Data3, daq::IDurationTailReader::Id.Data4_UInt64 };

void daqDurationTailReader_getInterfaceId(daqIntfID* intfId)
{
    *intfId = DAQ_DURATION_TAIL_READER_INTF_ID;
}

daqErrCode daqDurationTailReader_read(daqDurationTailReader* self, void* values, daqSizeT* count, daqTailReaderStatus** status)
{
    return reinterpret_cast<daq::IDurationTailReader*>(self)->read(values, count, reinterpret_cast<daq::ITailReaderStatus**>(status));
}

daqErrCode daqDurationTailReader_readWithDomain(daqDurationTailReader* self, void* values, void* domain, daqSizeT* count, daqTailReaderStatus** status)
{
    return reinterpret_cast<daq::IDurationTailReader*>(self)->readWithDomain(values, domain, count, reinterpret_cast<daq::ITailReaderStatus**>(status));
}

daqErrCode daqDurationTailReader_getHistoryDurationMs(daqDurationTailReader* self, daqUInt* milliseconds)
{
    return reinterpret_cast<daq::IDurationTailReader*>(self)->getHistoryDurationMs(milliseconds);
}

daqErrCode daqDurationTailReader_setHistoryDurationMs(daqDurationTailReader* self, daqUInt milliseconds)
{
    return reinterpret_cast<daq::IDurationTailReader*>(self)->setHistoryDurationMs(milliseconds);
}

daqErrCode daqDurationTailReader_createDurationTailReader(daqDurationTailReader** obj, daqSignal* signal, daqUInt historyDurationMs, daqSampleType valueReadType, daqSampleType domainReadType, daqReadMode mode)
{
    daq::IDurationTailReader* ptr = nullptr;
    daqErrCode err = daq::createDurationTailReader(&ptr, reinterpret_cast<daq::ISignal*>(signal), historyDurationMs, static_cast<daq::SampleType>(valueReadType), static_cast<daq::SampleType>(domainReadType), static_cast<daq::ReadMode>(mode));
    *obj = reinterpret_cast<daqDurationTailReader*>(ptr);
    return err;
}

daqErrCode daqDurationTailReader_createDurationTailReaderFromPort(daqDurationTailReader** obj, daqInputPortConfig* port, daqUInt historyDurationMs, daqSampleType valueReadType, daqSampleType domainReadType, daqReadMode mode)
{
    daq::IDurationTailReader* ptr = nullptr;
    daqErrCode err = daq::createDurationTailReaderFromPort(&ptr, reinterpret_cast<daq::IInputPortConfig*>(port), historyDurationMs, static_cast<daq::SampleType>(valueReadType), static_cast<daq::SampleType>(domainReadType), static_cast<daq::ReadMode>(mode));
    *obj = reinterpret_cast<daqDurationTailReader*>(ptr);
    return err;
}
