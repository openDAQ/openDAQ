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
#include <ref_template_device_module/common.h>
#include <opendaq_module_template/module_template.h>

BEGIN_NAMESPACE_REF_TEMPLATE_DEVICE_MODULE

class RefTemplateDeviceModuleBase final : public templates::ModuleTemplateHooks
{
public:
    RefTemplateDeviceModuleBase(const ContextPtr& context);
};

class RefTemplateDeviceModule final : public templates::ModuleTemplate
{
public:
    explicit RefTemplateDeviceModule(const ContextPtr& context);

protected:
    templates::ModuleParams buildModuleParams() override;
    std::vector<templates::DeviceTypeParams> getAvailableDeviceTypes(const DictPtr<IString, IBaseObject>& options) override;
    std::vector<templates::DeviceInfoParams> getAvailableDeviceInfo(const DictPtr<IString, IBaseObject>& options) override;
    DevicePtr createDevice(const templates::DeviceParams& params) override;

private:
    static PropertyObjectPtr createDefaultConfig();
    static DictPtr<IString, IBaseObject> createDefaultModuleOptions();
    size_t maxNumberOfDevices;
};

END_NAMESPACE_REF_TEMPLATE_DEVICE_MODULE
