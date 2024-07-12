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
#include <opendaq/reader.h>
#include <coretypes/function.h>
#include <opendaq/sample_type.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_readers
 * @addtogroup opendaq_sample_reader Sample Reader
 * @{
 */

enum class ReadMode
{
    Unscaled,
    Scaled,
    RawValue
};

/*#
 * [templated(defaultAliasName: SampleReader)]
 * [interfaceSmartPtr(ISampleReader, GenericSampleReaderPtr)]
 * [interfaceSmartPtr(IReader, GenericReaderPtr)]
 * [include(ISampleType)]
 */

/*!
 * @brief A basic signal reader that simplifies reading the signals's samples.
 */
DECLARE_OPENDAQ_INTERFACE(ISampleReader, IReader)
{
    /*!
     * @brief Gets the sample-type the signal value samples will be converted to when read
     * or @c SampleType::Invalid if read-type has not been determined yet.
     * @param[out] sampleType The sample-type type of the read samples otherwise @c SampleType::Invalid.
     */
    virtual ErrCode INTERFACE_FUNC getValueReadType(SampleType* sampleType) = 0;

    /*!
     * @brief Gets the sample-type the signal domain samples will be converted to when read
     * or @c SampleType::Invalid if read-type has not been determined yet.
     * @param[out] sampleType The sample-type type of the read samples otherwise @c SampleType::Invalid.
     */
    virtual ErrCode INTERFACE_FUNC getDomainReadType(SampleType* sampleType) = 0;

    /*!
     * @brief Sets the transform function that will be called with the read value-data and currently valid Signal-Descriptor
     * giving the user the chance add a custom post-processing step. The function should have a signature compatible with:
     * @code
     * transform(Int inputBuffer, Int outputBuffer, SizeT toRead, IDataDescriptor* descriptor)
     * @endcode
     * @param transform The function performing the post-processing.
     */
    virtual ErrCode INTERFACE_FUNC setValueTransformFunction(IFunction* transform) = 0;

    /*!
     * @brief Sets the transform function that will be called with the read domain-data and currently valid Signal-Descriptor
     * giving the user the chance add a custom post-processing step. The function should have a signature compatible with:
     * @code
     * transform(Int inputBuffer, Int outputBuffer, SizeT toRead, IDataDescriptor* descriptor)
     * @endcode
     * @param transform The function performing the post-processing. 
     */
    virtual ErrCode INTERFACE_FUNC setDomainTransformFunction(IFunction* transform) = 0;

    /*!
     * @brief Gets the reader's read mode which determines if the reader will also scale the read data or not.
     * @param[out] mode The mode the reader is in (either Raw or Scaled)
     */
    virtual ErrCode INTERFACE_FUNC getReadMode(ReadMode* mode) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
