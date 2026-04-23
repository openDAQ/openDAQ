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

#include <coretypes/updatable.h>
#include <coretypes/serializable.h>
#include <coretypes/ctutils.h>

static daq::ErrCode serializePropertyValueAsNull(daq::IString* name, daq::ISerializer* serializer)
{
    daq::ErrCode errCode = serializer->keyStr(name);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    errCode = serializer->writeNull();
    OPENDAQ_RETURN_IF_FAILED(errCode);

    return OPENDAQ_SUCCESS;
}

static daq::ErrCode serializePropertyValueAsUpdatable(daq::IString* name,
                                                      daq::IBaseObject* value,
                                                      daq::ISerializer* serializer)
{
    daq::IUpdatable* updatableValue;
    daq::ErrCode errCode = value->borrowInterface(daq::IUpdatable::Id, reinterpret_cast<void**>(&updatableValue));
    if (OPENDAQ_SUCCEEDED(errCode))
    {
        errCode = serializer->keyStr(name);
        OPENDAQ_RETURN_IF_FAILED(errCode);

        errCode = updatableValue->serializeForUpdate(serializer);
        OPENDAQ_RETURN_IF_FAILED(errCode);

        return OPENDAQ_SUCCESS;
    }

    if (errCode == OPENDAQ_ERR_NOINTERFACE)
    {
        daqClearErrorInfo();
        return OPENDAQ_ERR_NOINTERFACE;
    }

    return DAQ_EXTEND_ERROR_INFO(errCode);
}

static daq::ErrCode serializePropertyValueAsSerializable(daq::IString* name,
                                                         daq::IBaseObject* value,
                                                         daq::ISerializer* serializer)
{
    daq::ISerializable* serializableValue;
    daq::ErrCode errCode = value->borrowInterface(daq::ISerializable::Id, reinterpret_cast<void**>(&serializableValue));
    if (OPENDAQ_FAILED(errCode))
    {
        if (errCode == OPENDAQ_ERR_NOINTERFACE)
        {
            daqClearErrorInfo();
            return OPENDAQ_SUCCESS;
        }

        return DAQ_EXTEND_ERROR_INFO(errCode);
    }

    errCode = serializer->keyStr(name);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    errCode = serializableValue->serialize(serializer);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    return OPENDAQ_SUCCESS;
}

extern "C" PUBLIC_EXPORT daq::ErrCode daqSerializePropertyValue(daq::IString* name,
                                                                daq::IBaseObject* value,
                                                                daq::ISerializer* serializer,
                                                                daq::Bool forUpdate)
{
    if (value == nullptr)
        return serializePropertyValueAsNull(name, serializer);

    if (forUpdate)
    {
        const daq::ErrCode errCode = serializePropertyValueAsUpdatable(name, value, serializer);
        if (OPENDAQ_SUCCEEDED(errCode))
            return OPENDAQ_SUCCESS;

        if (errCode != OPENDAQ_ERR_NOINTERFACE)
            return errCode;
    }

    return serializePropertyValueAsSerializable(name, value, serializer);
}
