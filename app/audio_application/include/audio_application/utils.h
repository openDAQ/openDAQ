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
#include <opendaq/opendaq.h>

using DeviceInfoList = daq::ListPtr<daq::IDeviceInfo>;
using DeviceInfoDict = daq::DictPtr<daq::IString, daq::IDeviceInfo>;

DeviceInfoList filterDevicesInfos(const DeviceInfoDict& deviceInfoDict, const std::string& prefix);
void printDevices(std::ostream& stream, const DeviceInfoList& deviceInfoList);
bool getLastValue(const daq::PacketReaderPtr& packetReader, float& lastValue);
float getNormalizedLogValue(float value);
void printValueBar(std::ostream& stream, float normalizedValue, size_t maxVal);
void hideCursor(std::ostream& stream);
void printLastValueBar(std::ostream& stream, const daq::PacketReaderPtr& packetReader);
