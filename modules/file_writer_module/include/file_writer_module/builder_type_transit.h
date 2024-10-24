/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <file_writer_module/common.h>
#include <opendaq/module_impl.h>
#include <opendaq/sample_type.h>
#include <arrow/api.h>
#include <arrow/io/api.h>

BEGIN_NAMESPACE_FILE_WRITER_MODULE

//////////////////////////
//
//   SampleToBuilderType
//
/////////////////////////

template <SampleType Type>
struct SampleToBuilderType;

template <>
struct SampleToBuilderType<SampleType::Invalid>
{
    using Type = void;
};

template <>
struct SampleToBuilderType<SampleType::UInt8>
{
    using Type = arrow::UInt8Builder;
};

template <>
struct SampleToBuilderType<SampleType::Int8>
{
    using Type = arrow::Int8Builder;
};

template <>
struct SampleToBuilderType<SampleType::UInt16>
{
    using Type = arrow::UInt16Builder;
};

template <>
struct SampleToBuilderType<SampleType::Int16>
{
    using Type = arrow::Int16Builder;
};

template <>
struct SampleToBuilderType<SampleType::UInt32>
{
    using Type = arrow::UInt32Builder;
};

template <>
struct SampleToBuilderType<SampleType::Int32>
{
    using Type = arrow::Int32Builder;
};

template <>
struct SampleToBuilderType<SampleType::UInt64>
{
    using Type = arrow::UInt64Builder;
};

template <>
struct SampleToBuilderType<SampleType::Int64>
{
    using Type = arrow::Int64Builder;
};

template <>
struct SampleToBuilderType<SampleType::Float32>
{
    using Type = arrow::FloatBuilder;
};

template <>
struct SampleToBuilderType<SampleType::Float64>
{
    using Type = arrow::DoubleBuilder;
};

END_NAMESPACE_FILE_WRITER_MODULE

