/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <config_protocol/config_protocol_client.h>
#include <coretypes/validation.h>

namespace daq::config_protocol
{

class ConfigClientProcedureImpl : public ImplementationOf<ICoreType, IProcedure>
{
public:
    ConfigClientProcedureImpl(const ConfigProtocolClientCommPtr& clientComm,
                              const StringPtr& globalId,
                              const StringPtr& path,
                              const StringPtr& name);

    ErrCode INTERFACE_FUNC dispatch(IBaseObject* args) override;
    ErrCode INTERFACE_FUNC getCoreType(CoreType* coreType) override;

private:
    StringPtr globalId;
    StringPtr path;
    StringPtr name;
    ConfigProtocolClientCommPtr clientComm;
};

inline ConfigClientProcedureImpl::ConfigClientProcedureImpl(
    const ConfigProtocolClientCommPtr& clientComm,
    const StringPtr& globalId,
    const StringPtr& path,
    const StringPtr& name)
    : globalId(globalId)
    , path(path)
    , name(name)
    , clientComm(clientComm)
{
}

inline ErrCode ConfigClientProcedureImpl::dispatch(IBaseObject* args)
{
    return daqTry([this, &args]
    {
        auto propName = name.toStdString();
        if (path.assigned() && path != "")
            propName = path.toStdString() + "." + propName;
        clientComm->callProperty(globalId, propName, args);
    });
}

inline ErrCode ConfigClientProcedureImpl::getCoreType(CoreType* coreType)
{
    OPENDAQ_PARAM_NOT_NULL(coreType);

    *coreType = CoreType::ctProc;
    return OPENDAQ_SUCCESS;
}

}
