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
#include <opendaq/recorder_ptr.h>
#include <opendaq/component_ptr.h>

namespace daq::config_protocol
{

class ConfigServerRecorder
{
public:
    static BaseObjectPtr startRecording(const RpcContext& context, const RecorderPtr& recorder, const ParamsDictPtr& params);
    static BaseObjectPtr stopRecording(const RpcContext& context, const RecorderPtr& recorder, const ParamsDictPtr& params);
    static BaseObjectPtr getIsRecording(const RpcContext& context, const RecorderPtr& recorder, const ParamsDictPtr& params);
};

inline BaseObjectPtr ConfigServerRecorder::startRecording(const RpcContext& context, const RecorderPtr& recorder, const ParamsDictPtr& /*params*/)
{
    ComponentPtr recorderComponent = recorder.asPtr<IComponent>();
    ConfigServerAccessControl::protectViewOnlyConnection(context.connectionType);
    ConfigServerAccessControl::protectLockedComponent(recorderComponent);
    ConfigServerAccessControl::protectObject(recorderComponent, context.user, {Permission::Read, Permission::Write});

    recorder.startRecording();
    return nullptr;
}

inline BaseObjectPtr ConfigServerRecorder::stopRecording(const RpcContext& context, const RecorderPtr& recorder, const ParamsDictPtr& /*params*/)
{
    ComponentPtr recorderComponent = recorder.asPtr<IComponent>();
    ConfigServerAccessControl::protectViewOnlyConnection(context.connectionType);
    ConfigServerAccessControl::protectLockedComponent(recorderComponent);
    ConfigServerAccessControl::protectObject(recorderComponent, context.user, {Permission::Read, Permission::Write});

    recorder.stopRecording();
    return nullptr;
}

inline BaseObjectPtr ConfigServerRecorder::getIsRecording(const RpcContext& context, const RecorderPtr& recorder, const ParamsDictPtr& /*params*/)
{
    ComponentPtr recorderComponent = recorder.asPtr<IComponent>();
    ConfigServerAccessControl::protectObject(recorderComponent, context.user, Permission::Read);

    return Boolean(recorder.getIsRecording());
}

}
