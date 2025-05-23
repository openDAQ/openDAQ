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
#include <opendaq/input_port_ptr.h>

namespace daq::config_protocol
{

class ConfigServerInputPort
{
public:
    static BaseObjectPtr connect(const RpcContext& context, const InputPortPtr& inputPort, const SignalPtr& signal, const ParamsDictPtr& params);
    static BaseObjectPtr disconnect(const RpcContext& context, const InputPortPtr& inputPort, const ParamsDictPtr& params);
	static BaseObjectPtr accepts(const RpcContext& context, const InputPortPtr& inputPort, const SignalPtr& signal, const UserPtr& user);
};

inline BaseObjectPtr ConfigServerInputPort::connect(const RpcContext& context,
                                                    const InputPortPtr& inputPort,
                                                    const SignalPtr& signal,
                                                    const ParamsDictPtr& params)
{
    if (!signal.assigned())
        DAQ_THROW_EXCEPTION(NotFoundException, "Cannot connect requested signal. Signal not found");

    ConfigServerAccessControl::protectViewOnlyConnection(context.connectionType);
    ConfigServerAccessControl::protectLockedComponent(inputPort);
    ConfigServerAccessControl::protectObject(inputPort, context.user, {Permission::Read, Permission::Write});
    ConfigServerAccessControl::protectObject(signal, context.user, Permission::Read);

    inputPort.connect(signal);
    return nullptr;
}

inline BaseObjectPtr ConfigServerInputPort::disconnect(const RpcContext& context,
                                                       const InputPortPtr& inputPort,
                                                       const ParamsDictPtr& params)
{
    ConfigServerAccessControl::protectLockedComponent(inputPort);
    ConfigServerAccessControl::protectObject(inputPort, context.user, {Permission::Read, Permission::Write});
    ConfigServerAccessControl::protectViewOnlyConnection(context.connectionType);

    inputPort.disconnect();
    return nullptr;
}

inline BaseObjectPtr ConfigServerInputPort::accepts(const RpcContext& context,
                                                    const InputPortPtr& inputPort,
                                                    const SignalPtr& signal,
                                                    const UserPtr& user)
{
    if (!signal.assigned())
        DAQ_THROW_EXCEPTION(NotFoundException, "Cannot connect requested signal. Signal not found");

    ConfigServerAccessControl::protectObject(inputPort, user, Permission::Read);
    ConfigServerAccessControl::protectObject(signal, user, Permission::Read);

    return Boolean(inputPort.acceptsSignal(signal));
}

}
