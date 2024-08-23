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
#include <opendaq/data_descriptor.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [include(ISampleType)]
 * [interfaceLibrary(IUnit, CoreObjects)]
 */

/*!
 * @ingroup opendaq_signals
 * @addtogroup opendaq_data_descriptor Data descriptor
 * @{
 */

/*!
 * @brief Builder component of Data descriptor objects. Contains setter methods that allow for Data descriptor
 * parameter configuration, and a `build` method that builds the Data descriptor.
 */
DECLARE_OPENDAQ_INTERFACE(IDataDescriptorBuilder, IBaseObject)
{
    /*!
     * @brief Builds and returns a Data descriptor object using the currently set values of the Builder.
     * @param[out] dataDescriptor The built Data descriptor.
     */
    virtual ErrCode INTERFACE_FUNC build(IDataDescriptor** dataDescriptor) = 0;

    // [returnSelf]
    /*!
     * @brief Sets a descriptive name for the signal's value.
     * @param name The name of the signal value.
     *
     * When, for example, describing the amplitude values of spectrum data, the name would be `Amplitude`.
     */
    virtual ErrCode INTERFACE_FUNC setName(IString* name) = 0;

    /*!
     * @brief Gets a descriptive name for the signal's value.
     * @param[out] name The name of the signal value.
     */
    virtual ErrCode INTERFACE_FUNC getName(IString** name) = 0;

    // [elementType(dimensions, IDimension), returnSelf]
    /*!
     * @brief Sets the list of the descriptor's dimension's.
     * @param dimensions The list of dimensions.
     *
     * The number of dimensions defines the rank of the signal's data (eg. Vector, Matrix).
     */
    virtual ErrCode INTERFACE_FUNC setDimensions(IList* dimensions) = 0;

    // [elementType(dimensions, IDimension)]
    /*!
     * @brief Gets the list of the descriptor's dimension's.
     * @param[out] dimensions The list of dimensions.
     */
    virtual ErrCode INTERFACE_FUNC getDimensions(IList** dimensions) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the descriptor's sample type.
     * @param sampleType The descriptor's sample type.
     */
    virtual ErrCode INTERFACE_FUNC setSampleType(SampleType sampleType) = 0;

    /*!
     * @brief Gets the descriptor's sample type.
     * @param[out] sampleType The descriptor's sample type.
     */
    virtual ErrCode INTERFACE_FUNC getSampleType(SampleType* sampleType) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the unit of the data in a signal's packets.
     * @param unit The unit specified by the descriptor.
     */
    virtual ErrCode INTERFACE_FUNC setUnit(IUnit* unit) = 0;

    /*!
     * @brief Gets the unit of the data in a signal's packets.
     * @param[out] unit The unit specified by the descriptor.
     */
    virtual ErrCode INTERFACE_FUNC getUnit(IUnit** unit) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the value range of the data in a signal's packets defining the lowest and highest expected values.
     * @param range The value range the signal's data.
     *
     * The range is not enforced by openDAQ.
     */
    virtual ErrCode INTERFACE_FUNC setValueRange(IRange* range) = 0;

    /*!
     * @brief Gets the value range of the data in a signal's packets defining the lowest and highest expected values.
     * @param[out] range The value range the signal's data.
     */
    virtual ErrCode INTERFACE_FUNC getValueRange(IRange** range) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the value Data rule.
     * @param rule The value Data rule.
     *
     * If explicit, the values will be contained in the packet buffer. Otherwise they are calculated
     * using the packet parameter as the input into the rule.
     */
    virtual ErrCode INTERFACE_FUNC setRule(IDataRule* rule) = 0;

    /*!
     * @brief Gets the value Data rule.
     * @param[out] rule The value Data rule.
     */
    virtual ErrCode INTERFACE_FUNC getRule(IDataRule** rule) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the absolute origin of a signal value component.
     * @param origin The absolute origin.
     *
     * An origin can be an arbitrary string that determines the starting point of the signal data.
     * All explicit or implicit values are multiplied by the resolution and added to the origin to obtain
     * absolute data instead of relative.
     *
     * Most commonly a time epoch is used, in which case it should be formatted according to the ISO 8601 standard.
     */
    virtual ErrCode INTERFACE_FUNC setOrigin(IString* origin) = 0;

    /*!
     * @brief Gets the absolute origin of a signal value component.
     * @param[out] origin The absolute origin.
     */
    virtual ErrCode INTERFACE_FUNC getOrigin(IString** origin) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the Resolution which scales the an explicit or implicit value to the physical unit defined in `unit`.
     * @param tickResolution The Resolution.
     */
    virtual ErrCode INTERFACE_FUNC setTickResolution(IRatio* tickResolution) = 0;

    /*!
     * @brief Gets the Resolution which scales the an explicit or implicit value to the physical unit defined in `unit`.
     * @param[out] tickResolution The Resolution.
     */
    virtual ErrCode INTERFACE_FUNC getTickResolution(IRatio** tickResolution) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the scaling rule that needs to be applied to explicit/implicit data by readers.
     * @param scaling The scaling rule.
     *
     * The OutputDataType of the rule matches the value descriptor's sample type. The InputDataType defines the sample type
     * of either the explicit data in packet buffers, or the packet's implicit value's sample type.
     */
    virtual ErrCode INTERFACE_FUNC setPostScaling(IScaling* scaling) = 0;

    /*!
     * @brief Gets the scaling rule that needs to be applied to explicit/implicit data by readers.
     * @param[out] scaling The scaling rule.
     */
    virtual ErrCode INTERFACE_FUNC getPostScaling(IScaling** scaling) = 0;

    // [elementType(structFields, IDataDescriptor), returnSelf]
    /*!
     * @brief Sets the fields of the struct, forming a recursive value descriptor definition.
     * @param structFields The list of data descriptors forming the struct fields.
     *
     * Contains a list of value descriptors, defining the data layout: the data described by the first DataDescriptor
     * of the list is at the start, followed by the data described by the second and so on.
     */
    virtual ErrCode INTERFACE_FUNC setStructFields(IList* structFields) = 0;

    // [elementType(structFields, IDataDescriptor)]
    /*!
     * @brief Gets the fields of the struct, forming a recursive value descriptor definition.
     * @param[out] structFields The list of data descriptors forming the struct fields.
     */
    virtual ErrCode INTERFACE_FUNC getStructFields(IList** structFields) = 0;

    // [templateType(metadata, IString, IString), returnSelf]
    /*!
     * @brief Sets any extra metadata defined by the data descriptor.
     * @param metadata Additional metadata of the descriptor as a dictionary.
     *
     * All objects in the metadata dictionary must be serializable.
     */
    virtual ErrCode INTERFACE_FUNC setMetadata(IDict* metadata) = 0;

    // [templateType(metadata, IString, IString)]
    /*!
     * @brief Gets any extra metadata defined by the data descriptor.
     * @param[out] metadata Additional metadata of the descriptor as a dictionary.
     */
    virtual ErrCode INTERFACE_FUNC getMetadata(IDict** metadata) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the Reference Domain Info.
     * @param referenceDomainInfo The Reference Domain Info.
     *
     * If set, gives additional information about the reference domain.
     */
    virtual ErrCode INTERFACE_FUNC setReferenceDomainInfo(IReferenceDomainInfo* referenceDomainInfo) = 0;

    /*!
     * @brief Gets the Reference Domain Info.
     * @param[out] referenceDomainInfo The Reference Domain Info.
     *
     * If set, gives additional information about the reference domain.
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
 * @brief Data descriptor builder factory that creates a builder object with no parameters configured.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, DataDescriptorBuilder, IDataDescriptorBuilder
)

/*!
 * @brief Data descriptor copy factory that creates a Data descriptor builder object from a
 * different Data descriptor, copying its parameters.
 * @param descriptorToCopy The Data descriptor of which configuration should be copied.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, DataDescriptorBuilderFromExisting, IDataDescriptorBuilder,
    IDataDescriptor*, descriptorToCopy
)

/*!@}*/

END_NAMESPACE_OPENDAQ
