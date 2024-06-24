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
#include <opendaq/function_block_type.h>
#include <coreobjects/component_type_impl.h>
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
                                   const PropertyObjectPtr& defaultConfig,
                                   const ListPtr<IString>& altIds = nullptr);

    explicit FunctionBlockTypeImpl(const ComponentTypeBuilderPtr& builder);

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);
};


inline FunctionBlockTypeImpl::FunctionBlockTypeImpl(const StringPtr& id,
                                                    const StringPtr& name,
                                                    const StringPtr& description,
                                                    const PropertyObjectPtr& defaultConfig,
                                                    const ListPtr<IString>& altIds)
    : Super(FunctionBlockTypeStructType(), id, name, description, defaultConfig, altIds)
{
}

inline FunctionBlockTypeImpl::FunctionBlockTypeImpl(const ComponentTypeBuilderPtr& builder)
    : FunctionBlockTypeImpl(builder.getId(), builder.getName(), builder.getDescription(), builder.getDefaultConfig(), builder.getAltIds())
{
}

inline ErrCode FunctionBlockTypeImpl::serialize(ISerializer* serializer)
{
    OPENDAQ_PARAM_NOT_NULL(serializer);

    const auto serializerPtr = SerializerPtr::Borrow(serializer);

    return daqTry(
        [this, &serializerPtr]
        {
            serializerPtr.startTaggedObject(borrowPtr<SerializablePtr>());
            {
                serializerPtr.key("Id");
                serializerPtr.writeString(id);

                if (name.assigned())
                {
                    serializerPtr.key("Name");
                    serializerPtr.writeString(name);
                }

                if (description.assigned())
                {
                    serializerPtr.key("Description");
                    serializerPtr.writeString(description);
                }

                if (defaultConfig.assigned())
                {
                    serializerPtr.key("DefaultConfig");
                    defaultConfig.serialize(serializerPtr);
                }
            }

            serializerPtr.endObject();
        });
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

    return daqTry(
        [&serializedObj, &contextPtr, &factoryCallbackPtr, &obj]
        {
            const auto id = serializedObj.readString("Id");

            StringPtr name;
            if (serializedObj.hasKey("Name"))
                name = serializedObj.readString("Name");

            StringPtr description;
            if (serializedObj.hasKey("Description"))
                description = serializedObj.readString("Description");

            PropertyObjectPtr defaultConfig;
            if (serializedObj.hasKey("DefaultConfig"))
                defaultConfig = serializedObj.readObject("DefaultConfig", contextPtr, factoryCallbackPtr);

            *obj = createWithImplementation<IFunctionBlockType, FunctionBlockTypeImpl>(id, name, description, defaultConfig).detach();
        });
}

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(FunctionBlockTypeImpl)

END_NAMESPACE_OPENDAQ
