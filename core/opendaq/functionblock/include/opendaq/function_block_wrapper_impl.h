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

#include <opendaq/function_block_wrapper.h>
#include <opendaq/function_block_ptr.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/context_ptr.h>
#include <unordered_set>
#include <unordered_map>

BEGIN_NAMESPACE_OPENDAQ

class FunctionBlockWrapperImpl : public FunctionBlockImpl<IFunctionBlock, IFunctionBlockWrapper>
{
public:
    using Self = FunctionBlockWrapperImpl;
    using Super = FunctionBlockImpl<IFunctionBlock, IFunctionBlockWrapper>;

    FunctionBlockWrapperImpl(
        const FunctionBlockPtr& functionBlock,
        bool includeInputPortsByDefault,
        bool includeSignalsByDefault,
        bool includePropertiesByDefault,
        bool includeFunctionBlocksByDefault);

    ErrCode INTERFACE_FUNC includeInputPort(IString* inputPortName) override;
    ErrCode INTERFACE_FUNC excludeInputPort(IString* inputPortName) override;
    ErrCode INTERFACE_FUNC includeSignal(IString* signalLocalId) override;
    ErrCode INTERFACE_FUNC excludeSignal(IString* signalLocalId) override;
    ErrCode INTERFACE_FUNC includeProperty(IString* propertyName) override;
    ErrCode INTERFACE_FUNC excludeProperty(IString* propertyName) override;
    ErrCode INTERFACE_FUNC includeFunctionBlock(IString* functionBlockLocalId) override;
    ErrCode INTERFACE_FUNC excludeFunctionBlock(IString* functionBlockLocalId) override;
    ErrCode INTERFACE_FUNC setPropertyCoercer(IString* propertyName, ICoercer* coercer) override;
    ErrCode INTERFACE_FUNC setPropertyValidator(IString* propertyName, IValidator* validator) override;
    ErrCode INTERFACE_FUNC setPropertySelectionValues(IString* propertyName, IList* enumValues) override;
    ErrCode INTERFACE_FUNC getWrappedFunctionBlock(IFunctionBlock** functionBlock) override;

    ErrCode INTERFACE_FUNC getInputPorts(IList** ports) override;
    ErrCode INTERFACE_FUNC getSignals(IList** signals) override;
    ErrCode INTERFACE_FUNC getFunctionBlocks(IList** functionBlocks) override;

    ErrCode INTERFACE_FUNC setPropertyValue(IString* propertyName, IBaseObject* value) override;
    ErrCode INTERFACE_FUNC getPropertyValue(IString* propertyName, IBaseObject** value) override;
    ErrCode INTERFACE_FUNC getPropertySelectionValue(IString* propertyName, IBaseObject** value) override;
    ErrCode INTERFACE_FUNC clearPropertyValue(IString* propertyName) override;

    ErrCode INTERFACE_FUNC hasProperty(IString* propertyName, Bool* hasProperty) override;
    ErrCode INTERFACE_FUNC getProperty(IString* propertyName, IProperty** property) override;
    ErrCode INTERFACE_FUNC addProperty(IProperty* property) override;
    ErrCode INTERFACE_FUNC removeProperty(IString* propertyName) override;

    ErrCode INTERFACE_FUNC getVisibleProperties(IList** properties) override;
    ErrCode INTERFACE_FUNC getAllProperties(IList** properties) override;

private:
    FunctionBlockPtr functionBlock;
    std::unordered_set<std::string> includedInputPorts;
    std::unordered_set<std::string> excludedInputPorts;
    bool includeInputPortsByDefault;
    std::unordered_set<std::string> includedSignals;
    std::unordered_set<std::string> excludedSignals;
    bool includeSignalsByDefault;
    std::unordered_set<std::string> includedProperties;
    std::unordered_set<std::string> excludedProperties;
    bool includePropertiesByDefault;
    std::unordered_set<std::string> includedFbs;
    std::unordered_set<std::string> excludedFbs;
    bool includeFunctionsBlocksByDefault;
    std::unordered_map<std::string, ValidatorPtr> validators;
    std::unordered_map<std::string, CoercerPtr> coercers;
    std::unordered_map<std::string, std::unordered_set<size_t>> enumValuesMap;

    ErrCode includeObject(IString* objectName,
                          std::unordered_set<std::string>& includedObjects,
                          std::unordered_set<std::string>& excludedObjects,
                          bool includeObjectsByDefault);

    ErrCode excludeObject(IString* objectName,
                          std::unordered_set<std::string>& includedObjects,
                          std::unordered_set<std::string>& excludedObjects,
                          bool includeObjectsByDefault);

    template <class TInterface, class GetName>
    ListPtr<TInterface> getObjects(const ListPtr<TInterface> innerObjects,
                                   std::unordered_set<std::string>& includedObjects,
                                   std::unordered_set<std::string>& excludedObjects,
                                   bool includeObjectsByDefault,
                                   GetName&& getName);

    ErrCode getProperties(const ListPtr<IProperty>& innerProperties, IList** properties);

    bool isPropertyVisible(IString* propertyName);

    template<class TInterface, class TSmartPtr = typename InterfaceToSmartPtr<TInterface>::SmartPtr>
    ErrCode setOverridenObject(IString* propertyName, std::unordered_map<std::string, TSmartPtr>& objects, TInterface* object);
    bool isSelectionAvailable(const StringPtr& propertyNameStr, const BaseObjectPtr& value);
    PropertyPtr wrapProperty(const PropertyPtr& property);
    PropertyPtr wrapProperty(const StringPtr& propertyName);
    ListPtr<IProperty> wrapProperties(const ListPtr<IProperty>& properties);
};

template <class TInterface, class TSmartPtr>
ErrCode FunctionBlockWrapperImpl::setOverridenObject(
        IString* propertyName,
        std::unordered_map<std::string, TSmartPtr>& objects,
        TInterface* object)
{
    std::scoped_lock lock(sync);

    return wrapHandler(
        [this, &propertyName, &objects, &object]()
        {
            auto propertyNameStr = StringPtr::Borrow(propertyName);

            if (!isPropertyVisible(propertyNameStr))
                throw NotFoundException();

            if (!functionBlock.hasProperty(propertyNameStr))
                throw NotFoundException();

            auto objectPtr = TSmartPtr::Borrow(object);

            if (objectPtr.assigned())
                objects.insert_or_assign(propertyNameStr, objectPtr);
            else
                objects.erase(propertyNameStr);

        });
}


END_NAMESPACE_OPENDAQ
