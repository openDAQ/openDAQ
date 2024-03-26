/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <opendaq/signal_ptr.h>
#include <opendaq/data_descriptor_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class AppSignal
{
public:
    static bool processCommand(BaseObjectPtr& signal, const std::vector<std::string>& command);

private:
    static bool print(const SignalPtr& signal, const std::vector<std::string>& command);
    static bool list(const SignalPtr& signal, const std::vector<std::string>& command);
    static bool set(const SignalPtr& signal, const std::vector<std::string>& command);
    static bool select(BaseObjectPtr& signal, const std::vector<std::string>& command);
    static bool help();

    static void printDataDescriptor(const DataDescriptorPtr& descriptor, std::streamsize indent, int indentLevel);
    static void printDimensions(const ListPtr<IDimension>& dimensions, std::streamsize indent, int indentLevel);
    static void printDataRule(const DataRulePtr& rule, std::streamsize indent, int indentLevel);
    static void printDimensionRule(const DimensionRulePtr& rule, std::streamsize indent, int indentLevel);
    static void printUnit(const UnitPtr& unit, std::streamsize indent, int indentLevel);
    static void printScaling(const ScalingPtr& scaling, std::streamsize indent, int indentLevel);
    static void printTags(const TagsPtr& tags, std::streamsize indent, int indentLevel);
    static void printMetadata(const DictPtr<IString, IString>& metadata, std::streamsize indent, int indentLevel);
};

END_NAMESPACE_OPENDAQ
