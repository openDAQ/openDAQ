//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CGenerator v0.7.0) on 03.06.2025 22:07:22.
// </auto-generated>
//------------------------------------------------------------------------------

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

#ifdef __cplusplus
extern "C"
{
#endif

#include <ccommon.h>

    typedef struct daqBlockReaderBuilder daqBlockReaderBuilder;
    typedef struct daqBlockReader daqBlockReader;
    typedef struct daqSignal daqSignal;
    typedef struct daqInputPort daqInputPort;

    EXPORTED extern const daqIntfID DAQ_BLOCK_READER_BUILDER_INTF_ID;

    daqErrCode EXPORTED daqBlockReaderBuilder_build(daqBlockReaderBuilder* self, daqBlockReader** blockReader);
    daqErrCode EXPORTED daqBlockReaderBuilder_setOldBlockReader(daqBlockReaderBuilder* self, daqBlockReader* blockReader);
    daqErrCode EXPORTED daqBlockReaderBuilder_getOldBlockReader(daqBlockReaderBuilder* self, daqBlockReader** blockReader);
    daqErrCode EXPORTED daqBlockReaderBuilder_setSignal(daqBlockReaderBuilder* self, daqSignal* signal);
    daqErrCode EXPORTED daqBlockReaderBuilder_getSignal(daqBlockReaderBuilder* self, daqSignal** signal);
    daqErrCode EXPORTED daqBlockReaderBuilder_setInputPort(daqBlockReaderBuilder* self, daqInputPort* port);
    daqErrCode EXPORTED daqBlockReaderBuilder_getInputPort(daqBlockReaderBuilder* self, daqInputPort** port);
    daqErrCode EXPORTED daqBlockReaderBuilder_setValueReadType(daqBlockReaderBuilder* self, daqSampleType type);
    daqErrCode EXPORTED daqBlockReaderBuilder_getValueReadType(daqBlockReaderBuilder* self, daqSampleType* type);
    daqErrCode EXPORTED daqBlockReaderBuilder_setDomainReadType(daqBlockReaderBuilder* self, daqSampleType type);
    daqErrCode EXPORTED daqBlockReaderBuilder_getDomainReadType(daqBlockReaderBuilder* self, daqSampleType* type);
    daqErrCode EXPORTED daqBlockReaderBuilder_setReadMode(daqBlockReaderBuilder* self, daqReadMode mode);
    daqErrCode EXPORTED daqBlockReaderBuilder_getReadMode(daqBlockReaderBuilder* self, daqReadMode* mode);
    daqErrCode EXPORTED daqBlockReaderBuilder_setBlockSize(daqBlockReaderBuilder* self, daqSizeT size);
    daqErrCode EXPORTED daqBlockReaderBuilder_getBlockSize(daqBlockReaderBuilder* self, daqSizeT* size);
    daqErrCode EXPORTED daqBlockReaderBuilder_setOverlap(daqBlockReaderBuilder* self, daqSizeT overlap);
    daqErrCode EXPORTED daqBlockReaderBuilder_getOverlap(daqBlockReaderBuilder* self, daqSizeT* overlap);
    daqErrCode EXPORTED daqBlockReaderBuilder_setSkipEvents(daqBlockReaderBuilder* self, daqBool skipEvents);
    daqErrCode EXPORTED daqBlockReaderBuilder_getSkipEvents(daqBlockReaderBuilder* self, daqBool* skipEvents);
    daqErrCode EXPORTED daqBlockReaderBuilder_createBlockReaderBuilder(daqBlockReaderBuilder** obj);

#ifdef __cplusplus
}
#endif
