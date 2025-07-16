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
#include <licensing_module/common.h>
#include <opendaq/license_checker.h>
#include <coretypes/intfs.h>
#include <map>
#include <string>

BEGIN_NAMESPACE_LICENSING_MODULE

class LicenseChecker : public ImplementationOf<ILicenseChecker>{
public:
    explicit LicenseChecker(const std::map<std::string, unsigned int> &featureTokens);

    ErrCode getComponentTypes(IList** componentTypes) override;
    ErrCode getNumberOfAvailableTokens(IString* componentId, Int* availableTokens) override;
    ErrCode checkOut(IString* feature, SizeT count) override;
    ErrCode checkIn(IString* feature, SizeT count) override;

private:
    std::map<std::string, unsigned int> mFeatureTokens;
    std::mutex mutex;
};

END_NAMESPACE_LICENSING_MODULE
