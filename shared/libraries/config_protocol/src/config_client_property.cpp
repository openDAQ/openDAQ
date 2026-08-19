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

#include <config_protocol/config_client_property.h>

namespace daq::config_protocol
{

ErrCode DeserializeConfigClientProperty(ISerializedObject* serialized,
                                         IBaseObject* context,
                                         IFunction* factoryCallback,
                                         IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);

    return daqTry([&obj, &serialized, &context, &factoryCallback]()
    {
        if (!serialized)
            DAQ_THROW_EXCEPTION(ArgumentNullException, "Serialized object not assigned");

        const auto contextPtr = BaseObjectPtr::Borrow(context);
        if (!contextPtr.assigned())
            DAQ_THROW_EXCEPTION(ArgumentNullException, "Deserialization context not assigned");

        const auto ctx = contextPtr.asPtrOrNull<IConfigProtocolDeserializeContext>(true);
        if (!ctx.assigned())
            DAQ_THROW_EXCEPTION(InvalidParameterException, "Invalid deserialization context");

        const ConfigProtocolClientCommPtr clientComm = ctx->getClientComm();
        const auto protocolVersion = clientComm->getProtocolVersion();

        StringPtr name;
        ErrCode errCode = serialized->readString(String("name"), &name);
        OPENDAQ_RETURN_IF_FAILED_EXCEPT(errCode, OPENDAQ_ERR_NOTFOUND);

        const auto builder = PropertyBuilder(name);
        PropertyImpl::ReadBuilderDeserializeValues(builder, serialized, context, factoryCallback);

        Bool hasOnReadListeners = False;
        errCode = serialized->readBool(String("HasOnReadListeners"), &hasOnReadListeners);
        OPENDAQ_RETURN_IF_FAILED_EXCEPT(errCode, OPENDAQ_ERR_NOTFOUND);

        Bool hasSelectionValuesListener = False;
        errCode = serialized->readBool(String("HasSelectionValuesListeners"), &hasSelectionValuesListener);
        OPENDAQ_RETURN_IF_FAILED_EXCEPT(errCode, OPENDAQ_ERR_NOTFOUND);

        Bool hasSuggestedValuesListeners = False;
        errCode = serialized->readBool(String("HasSuggestedValuesListeners"), &hasSuggestedValuesListeners);
        OPENDAQ_RETURN_IF_FAILED_EXCEPT(errCode, OPENDAQ_ERR_NOTFOUND);

        if (hasOnReadListeners)
        {
            builder.getOnPropertyValueRead() += [](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& arg)
            {
                // just dummy event, so IProperty::getHasOnReadListeners will return true
            };
        }

        if (hasSuggestedValuesListeners && protocolVersion > 16)
        {
            builder.getOnSuggestedValuesRead() += [clientComm](PropertyPtr& prop, PropertyMetadataReadArgsPtr& args)
            {
                if (!clientComm->getConnected())
                    return;

                auto owner = prop.asPtr<IPropertyInternal>(true).getOwner();
                if (!owner.assigned())
                    return;

                if (auto configObj = dynamic_cast<ConfigClientObjectImpl*>(owner.getObject()); configObj)
                {
                    const StringPtr path = owner.asPtr<IPropertyObjectInternal>(true).getPath();
                    args.setValue(clientComm->getSuggestedValues(configObj->getRemoteGlobalId(), path, prop.getName()));
                }
            };
        }

        if (hasSelectionValuesListener && protocolVersion > 16)
        {
            builder.getOnSelectionValuesRead() += [clientComm](PropertyPtr& prop, PropertyMetadataReadArgsPtr& args)
            {
                if (!clientComm->getConnected())
                    return;

                auto owner = prop.asPtr<IPropertyInternal>(true).getOwner();
                if (!owner.assigned())
                    return;

                if (auto configObj = dynamic_cast<ConfigClientObjectImpl*>(owner.getObject()); configObj)
                {
                    const StringPtr path = owner.asPtr<IPropertyObjectInternal>(true).getPath();
                    args.setValue(clientComm->getSelectionValues(configObj->getRemoteGlobalId(), path, prop.getName()));
                }
            };
        }

        PropertyPtr prop = builder.build();
        *obj = prop.detach();
        return OPENDAQ_SUCCESS;
    });
}

}
