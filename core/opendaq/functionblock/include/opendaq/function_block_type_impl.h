/*
 * Copyright 2022-2026 openDAQ d.o.o.
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
#include <opendaq/function_block_type.h>
#include <opendaq/component_type_impl.h>
#include <opendaq/component_type_builder_ptr.h>
#include <opendaq/function_block_type_factory.h>

BEGIN_NAMESPACE_OPENDAQ

class FunctionBlockTypeImpl : public GenericComponentTypeImpl<IFunctionBlockType>
{
public:
    using Self = FunctionBlockTypeImpl;
    using Super = GenericComponentTypeImpl<IFunctionBlockType>;

    explicit FunctionBlockTypeImpl(const StringPtr& id,
                                   const StringPtr& name,
                                   const StringPtr& description,
                                   const PropertyObjectPtr& defaultConfig);

    explicit FunctionBlockTypeImpl(const StringPtr& id,
                                   const StringPtr& name,
                                   const StringPtr& description,
                                   const PropertyObjectPtr& defaultConfig,
                                   Bool alwaysEmpty,
                                   Bool singleton,
                                   const StringPtr& commonSettingsType);

    explicit FunctionBlockTypeImpl(const ComponentTypeBuilderPtr& builder);

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

    ErrCode INTERFACE_FUNC getAlwaysEmptyInput(Bool* alwaysEmpty) override;
    ErrCode INTERFACE_FUNC getSingleton(Bool* singleton) override;
    ErrCode INTERFACE_FUNC getCommonSettingsTypeId(IString** typeId) override;

private:
    Bool alwaysEmptyInput = False;
    Bool isSingleton = False;
    StringPtr commonSettingsTypeId = nullptr;
};


inline FunctionBlockTypeImpl::FunctionBlockTypeImpl(const StringPtr& id,
                                                    const StringPtr& name,
                                                    const StringPtr& description,
                                                    const PropertyObjectPtr& defaultConfig)
    : FunctionBlockTypeImpl(id, name, description, defaultConfig, False, False, nullptr)
{
}

inline FunctionBlockTypeImpl::FunctionBlockTypeImpl(const StringPtr& id,
                                                    const StringPtr& name,
                                                    const StringPtr& description,
                                                    const PropertyObjectPtr& defaultConfig,
                                                    Bool alwaysEmpty,
                                                    Bool singleton,
                                                    const StringPtr& commonSettingsType)
    : Super(FunctionBlockTypeStructType(), id, name, description, defaultConfig)
    , alwaysEmptyInput(alwaysEmpty)
    , isSingleton(singleton)
    , commonSettingsTypeId(commonSettingsType)
{
}

inline FunctionBlockTypeImpl::FunctionBlockTypeImpl(const ComponentTypeBuilderPtr& builder)
    : FunctionBlockTypeImpl(builder.getId(), builder.getName(), builder.getDescription(), builder.getDefaultConfig(), builder.getAlwaysEmptyInput(), builder.getSingleton(), builder.getCommonSettingsTypeId())
{
}

inline ErrCode FunctionBlockTypeImpl::serialize(ISerializer* serializer)
{
    OPENDAQ_PARAM_NOT_NULL(serializer);

    const auto serializerPtr = SerializerPtr::Borrow(serializer);

    const ErrCode errCode = daqTry([this, &serializerPtr]
    {
        serializerPtr.startTaggedObject(borrowPtr<SerializablePtr>());
        {
            serializerPtr.key("id");
            serializerPtr.writeString(id);

            if (name.assigned())
            {
                serializerPtr.key("name");
                serializerPtr.writeString(name);
            }

            if (description.assigned())
            {
                serializerPtr.key("description");
                serializerPtr.writeString(description);
            }

            if (defaultConfig.assigned())
            {
                serializerPtr.key("defaultConfig");
                defaultConfig.serialize(serializerPtr);
            }

            if (moduleInfo.assigned())
            {
                serializerPtr.key("moduleInfo");
                moduleInfo.serialize(serializerPtr);
            }

            serializerPtr.key("alwaysEmptyInput");
            serializerPtr.writeBool(alwaysEmptyInput);

            serializerPtr.key("singleton");
            serializerPtr.writeBool(isSingleton);

            if (commonSettingsTypeId.assigned())
            {
                serializerPtr.key("commonSettingsTypeId");
                serializerPtr.writeString(commonSettingsTypeId);
            }
        }

        serializerPtr.endObject();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

inline ErrCode FunctionBlockTypeImpl::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

inline ConstCharPtr FunctionBlockTypeImpl::SerializeId()
{
    return "FunctionBlockType";
}

inline ErrCode FunctionBlockTypeImpl::Deserialize(ISerializedObject* serialized,
                                                  IBaseObject* context,
                                                  IFunction* factoryCallback,
                                                  IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(serialized);
    OPENDAQ_PARAM_NOT_NULL(obj);

    const auto serializedObj = SerializedObjectPtr::Borrow(serialized);
    const auto contextPtr = BaseObjectPtr::Borrow(context);
    const auto factoryCallbackPtr = FunctionPtr::Borrow(factoryCallback);

    const ErrCode errCode = daqTry([&serializedObj, &contextPtr, &factoryCallbackPtr, &obj]
    {
        const auto id = serializedObj.readString("id");

        StringPtr name;
        if (serializedObj.hasKey("name"))
            name = serializedObj.readString("name");

        StringPtr description;
        if (serializedObj.hasKey("description"))
            description = serializedObj.readString("description");

        PropertyObjectPtr defaultConfig;
        if (serializedObj.hasKey("defaultConfig"))
            defaultConfig = serializedObj.readObject("defaultConfig", contextPtr, factoryCallbackPtr);

        Bool alwaysEmpty = False;
        if (serializedObj.hasKey("alwaysEmptyInput"))
        {
            alwaysEmpty = serializedObj.readBool("alwaysEmptyInput");
        }

        Bool isSingleton = False;
        if (serializedObj.hasKey("singleton"))
        {
            isSingleton = serializedObj.readBool("singleton");
        }

        StringPtr commonSettingsTypeId = nullptr;
        if (serializedObj.hasKey("commonSettingsTypeId"))
        {
            commonSettingsTypeId = serializedObj.readString("commonSettingsTypeId");
        }

        auto functionBlockType =
            createWithImplementation<IFunctionBlockType, FunctionBlockTypeImpl>(id, name, description, defaultConfig, alwaysEmpty, isSingleton, commonSettingsTypeId);

        ModuleInfoPtr moduleInfo;
        if (serializedObj.hasKey("moduleInfo"))
        {
            moduleInfo = serializedObj.readObject("moduleInfo", contextPtr, factoryCallbackPtr);
            functionBlockType.asPtr<IComponentTypePrivate>()->setModuleInfo(moduleInfo);
        }

        *obj = functionBlockType.detach();

    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

inline ErrCode FunctionBlockTypeImpl::getAlwaysEmptyInput(Bool* alwaysEmpty)
{
    OPENDAQ_PARAM_NOT_NULL(alwaysEmpty);

    *alwaysEmpty = this->alwaysEmptyInput;
    return OPENDAQ_SUCCESS;
}

inline ErrCode FunctionBlockTypeImpl::getSingleton(Bool* singleton)
{
    OPENDAQ_PARAM_NOT_NULL(singleton);

    *singleton = this->isSingleton;
    return OPENDAQ_SUCCESS;
}

inline ErrCode FunctionBlockTypeImpl::getCommonSettingsTypeId(IString** typeId)
{
    OPENDAQ_PARAM_NOT_NULL(typeId);

    *typeId = this->commonSettingsTypeId.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(FunctionBlockTypeImpl)

END_NAMESPACE_OPENDAQ
