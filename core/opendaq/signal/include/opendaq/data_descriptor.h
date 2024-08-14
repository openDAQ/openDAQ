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
#include <coretypes/listobject.h>
#include <coretypes/stringobject.h>
#include <coretypes/ratio.h>
#include <opendaq/range.h>
#include <coreobjects/unit.h>
#include <opendaq/data_rule.h>
#include <opendaq/scaling.h>
#include <opendaq/sample_type.h>
#include <opendaq/reference_domain_info.h>

BEGIN_NAMESPACE_OPENDAQ

struct IDataDescriptorBuilder;

/*#
 * [interfaceLibrary(INumber, CoreTypes)]
 * [interfaceLibrary(IUnit, CoreObjects)]
 */

/*!
 * @ingroup opendaq_signals
 * @addtogroup opendaq_data_descriptor Data descriptor
 * @{
 */

/*#
 * [include(ISampleType)]
 */
/*!
 * @brief Describes the data sent by a signal, defining how they are to be interpreted by anyone receiving the signal's packets.
 *
 * The data descriptor provides all information required on how to process data buffers, and how to interpret the data.
 * It contains the following fields:
 *   - `Name`: A descriptive name of the data being described. In example, when the values describe the amplitude of spectrum data, the name
 *             would be `Amplitude`.
 *   - `Dimensions`: A list of dimensions of the signal. A sample descriptor can have 0 or more dimensions. A signal with 1 dimension
 *                   has vector data. A signal with 2 dimensions has matrix data, a signal with 0 has a single value for each sample.
 *   - `SampleType`: An enum value that specifies the underlying data type (eg. Float64, Int32, String,...)
 *   - `Unit`: The unit of the data in the signal's packets.
 *   - `ValueRange`: The value range of the data in a signal's packets defining the lowest and highest expected values. The range is not
 * enforced.
 *   - `Rule`: The data rule that defines how a signal value is calculated from an implicit initialization value when the rule type is not
 *             `Explicit`.
 *   - `Origin`: Defines the starting point of the signal. If set, all values are added to the absolute origin when read.
 *   - `TickResolution`: Used to scale signal ticks into their physical unit. In example, a resolution of 1/1000 with the unit being `seconds`
 *                       states that a value of 1 correlates to 1 millisecond.
 *   - `PostScaling`: Defines a scaling transformation, which should be applied to data carried by the signal's packets when read. If
 *                   `PostScaling` is used, the `Rule`, `Resolution`, and `Origin` must not be configured. The `SampleType` must match the
 *                   `outputDataType` of the `PostScaling`.
 *   - `StructFields`: A list of DataDescriptor. The descriptor list is used to define complex data samples. When defined, the sample
 *                     buffer is laid out so that the data described by the first DataDescriptor is at the start, followed by the data
 *                     described by the second and so on. If the list is not empty, all descriptor parameters except for `Name` and
 *                     `Dimensions` should not be configured. See below for a explanation of Structure data descriptors.
 *
 * @subsection data_descriptor_dimensions Dimensions
 * The list of dimensions determines the rank of values described by the descriptor. In example, if the list contains 1 dimension, the data
 * values are vectors. If it contains 2, the data values are matrices, if 0, the descriptor describes a single value.
 *
 * When the data is placed into packet buffers, the values are laid out in packet buffers linearly, where each value fills up
 * `sizeof(sampleType) * (dimensionSize1 * dimensionSize2 *...)` bytes.
 *
 * Data descriptor objects implement the Struct methods internally and are Core type `ctStruct`.
 * 
 * @subsection data_descriptor_struct_fields Struct fields
 *
 * A DataDescriptor with the `StructFields` field filled with other descriptors is used to represent signal data in the form of structures.
 * It allows for custom and complex structures to be described and sent by a signal.
 *
 * When evaluating a struct descriptor, their struct field values are laid out in the packet buffers in the order they are placed in
 * the `structFields` list. Eg. a struct with 3 fields, of types [int64_t, float, double] will have a buffer composed of an int64_t value,
 * followed by a float, and lastly a double value.
 *
 * Note that if the Dimensions field of the DataDescriptor is not empty, struct data can also be of a higher rank
 * (eg. a vector/matrix of structs).
 *
 * @subsection data_descriptor_calculation_order Value calculation
 * Besides the struct fields and dimensions, the Value descriptor also provides 4 fields which need to be taken into account and calculated
 * when reading packet buffers: `Rule`, `Resolution`, `Origin`, and `PostScaling`.
 *
 * @subsubsection data_descriptor_calculation_without_scaling Without `PostScaling`
 *
 * 1. Check and apply `Rule`:
 *    - If the rule is `explicit`, the values of a packet are present in the packet's data buffer
 *    - If not `explicit`, the packet's values need to be calculated. To calculate them, take the PacketOffset and SampleCount of the
 *      packet. Use the PacketOffset, as well as the index of the sample in a packet to calculate the rule's output value: `Value = PacketOffset + Rule(Index)`.
 *      Eg. `Value = PacketOffset + Delta * Index + Start`
 * 2. Apply `TickResolution`:
 *    - If the `TickResolution` is set, multiply the value from 1. with the `Resolution`. This scales the value into the `Unit` of the value
 *      descriptor.
 *    - If not set, keep the value from 1.
 * 3. Add `Origin`:
 *    - If the `Origin` is set, take the value from 2. and add it to the `Origin`.
 *      In example, if using the Unix Epoch, a value `1669279690` in seconds would mean Thursday, 24 November 2022 08:48:10 GMT.
 *    - If not set, keep the value from 2.
 *
 * @subsubsection data_descriptor_calculation_with_scaling With `PostScaling`
 *
 * If `PostScaling` is set, the `Rule` must be explicit, while `Resolution` and `Origin` must not be configured.
 *
 * To calculate the value with `PostScaling` configured, take the values of the packet's data buffer and apply the post scaling to each
 * value in the buffer: `Value = PostScaling(Value)`, eg. `Value = Value * Scale + Offset`
 */
DECLARE_OPENDAQ_INTERFACE(IDataDescriptor, IBaseObject)
{
    /*!
     * @brief Gets a descriptive name of the signal value.
     * @param[out] name The name of the signal value.
     *
     * When, for example, describing the amplitude values of spectrum data, the name would be `Amplitude`.
     */
    virtual ErrCode INTERFACE_FUNC getName(IString** name) = 0;

    // [elementType(dimensions, IDimension)]
    /*!
     * @brief Gets the list of the descriptor's dimension's.
     * @param[out] dimensions The list of dimensions.
     *
     * The number of dimensions defines the rank of the signal's data (eg. Vector, Matrix).
     */
    virtual ErrCode INTERFACE_FUNC getDimensions(IList** dimensions) = 0;

     /*!
     * @brief Gets the descriptor's sample type.
     * @param[out] sampleType The descriptor's sample type.
     */
    virtual ErrCode INTERFACE_FUNC getSampleType(SampleType* sampleType) = 0;

    /*!
     * @brief Gets the unit of the data in a signal's packets.
     * @param[out] unit The unit specified by the descriptor.
     */
    virtual ErrCode INTERFACE_FUNC getUnit(IUnit** unit) = 0;

    /*!
     * @brief Gets the value range of the data in a signal's packets defining the lowest and highest expected values.
     * @param[out] range The value range the signal's data.
     *
     * The range is not enforced by openDAQ.
     */
    virtual ErrCode INTERFACE_FUNC getValueRange(IRange** range) = 0;

    /*!
     * @brief Gets the value Data rule.
     * @param[out] rule The value Data rule.
     *
     * If explicit, the values will be contained in the packet buffer. Otherwise they are calculated
     * using the offset packet parameter as the input into the rule.
     */
    virtual ErrCode INTERFACE_FUNC getRule(IDataRule** rule) = 0;

    /*!
     * @brief Gets the absolute origin of a signal value component.
     * @param[out] origin The absolute origin.
     *
     * An origin can be an arbitrary string that determines the starting point of the signal data.
     * All explicit or implicit values are multiplied by the resolution and added to the origin to obtain
     * absolute data instead of relative.
     *
     * Most commonly a time reference is used, in which case it should be formatted according to the ISO 8601 standard.
     */
    virtual ErrCode INTERFACE_FUNC getOrigin(IString** origin) = 0;

    /*!
     * @brief Gets the Resolution which scales the explicit or implicit value to the physical unit defined in `unit`.
     * It is defined as domain (usually time) between two consecutive ticks.
     *
     * @param[out] tickResolution The Resolution.
     */
    virtual ErrCode INTERFACE_FUNC getTickResolution(IRatio** tickResolution) = 0;

    /*
     * @brief Scaling rule that needs to be applied to explicit/implicit data by readers.
     * @param[out] scaling The scaling rule.
     *
     * The OutputDataType of the rule matches the value descriptor's sample type. The InputDataType defines the sample type
     * of either the explicit data in packet buffers, or the packet's implicit value's sample type.
     */
    virtual ErrCode INTERFACE_FUNC getPostScaling(IScaling** scaling) = 0;


    // [elementType(structFields, IDataDescriptor)]
    /*!
     * @brief Gets the fields of the struct, forming a recursive value descriptor definition.
     * @param[out] structFields The list of data descriptors forming the struct fields.
     *
     * Contains a list of value descriptors, defining the data layout: the data described by the first DataDescriptor
     * of the list is at the start, followed by the data described by the second and so on.
     */
    virtual ErrCode INTERFACE_FUNC getStructFields(IList** structFields) = 0;

    // [templateType(metadata, IString, IString)]
    /*!
     * @brief Gets any extra metadata defined by the data descriptor.
     * @param[out] metadata Additional metadata of the descriptor as a dictionary.
     *
     * All objects in the metadata dictionary must be key value pairs of <String, String>.
     */
    virtual ErrCode INTERFACE_FUNC getMetadata(IDict** metadata) = 0;

    /*!
     * @brief Gets the size of one sample in bytes.
     * @param[out] sampleSize The size of one sample in bytes.
     *
     * The size of one sample is calculated on constructor of the data descriptor object.
     */
    virtual ErrCode INTERFACE_FUNC getSampleSize(SizeT* sampleSize) = 0;

    /*!
     * @brief Gets the actual sample size in buffer of one sample in bytes.
     * @param[out] sampleSize The actual size of one sample in buffer in bytes.
     *
     * The actual size of one sample is calculated on constructor of the data descriptor object.
     * Actual sample size is the sample size that is used in buffer. If the data descriptor includes
     * implicitly generated samples, the actual sample size is less than sample size.
     */
    virtual ErrCode INTERFACE_FUNC getRawSampleSize(SizeT* rawSampleSize) = 0;

    /*!
     * @brief Gets the Reference Domain Info.
     * @param[out] referenceDomainInfo The Reference Domain Info.
     *
     * TODO description
     */
    virtual ErrCode INTERFACE_FUNC getReferenceDomainInfo(IReferenceDomainInfo** referenceDomainInfo) = 0;

};
/*!@}*/


/*!
 * @ingroup opendaq_data_descriptor
 * @addtogroup opendaq_data_descriptor_factories Factories
 * @{
 */

/*!
 * @brief Creates a Data Descriptor using Builder
 * @param builder Data Descriptor Builder
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, DataDescriptorFromBuilder, IDataDescriptor,
    IDataDescriptorBuilder*, builder
)

/*!@}*/

END_NAMESPACE_OPENDAQ
