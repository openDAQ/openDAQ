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

#ifdef __cplusplus
extern "C"
{
#endif

#include <ccommon.h>

    typedef struct daqDurationTailReader daqDurationTailReader;
    typedef struct daqTailReaderStatus daqTailReaderStatus;
    typedef struct daqSignal daqSignal;
    typedef struct daqInputPortConfig daqInputPortConfig;

    EXPORTED extern const daqIntfID DAQ_DURATION_TAIL_READER_INTF_ID;
    void EXPORTED daqDurationTailReader_getInterfaceId(daqIntfID* intfId);

    daqErrCode EXPORTED daqDurationTailReader_read(daqDurationTailReader* self, void* values, daqSizeT* count, daqTailReaderStatus** status);
    daqErrCode EXPORTED daqDurationTailReader_readWithDomain(daqDurationTailReader* self, void* values, void* domain, daqSizeT* count, daqTailReaderStatus** status);
    daqErrCode EXPORTED daqDurationTailReader_getHistoryDurationMs(daqDurationTailReader* self, daqUInt* milliseconds);
    daqErrCode EXPORTED daqDurationTailReader_setHistoryDurationMs(daqDurationTailReader* self, daqUInt milliseconds);
    daqErrCode EXPORTED daqDurationTailReader_createDurationTailReader(daqDurationTailReader** obj, daqSignal* signal, daqUInt historyDurationMs, daqSampleType valueReadType, daqSampleType domainReadType, daqReadMode mode);
    daqErrCode EXPORTED daqDurationTailReader_createDurationTailReaderFromPort(daqDurationTailReader** obj, daqInputPortConfig* port, daqUInt historyDurationMs, daqSampleType valueReadType, daqSampleType domainReadType, daqReadMode mode);

#ifdef __cplusplus
}
#endif
