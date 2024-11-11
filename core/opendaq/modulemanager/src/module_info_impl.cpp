#include <coretypes/impl.h>
#include <opendaq/module_info_factory.h>
#include <opendaq/module_info_impl.h>

BEGIN_NAMESPACE_OPENDAQ

namespace detail
{
static const StructTypePtr moduleInfoStructType = ModuleInfoStructType();
}

ModuleInfoImpl::ModuleInfoImpl(const VersionInfoPtr& versionInfo, const StringPtr& name, const StringPtr& id)
    : GenericStructImpl<IModuleInfo, IStruct>(detail::moduleInfoStructType,
                                              Dict<IString, IBaseObject>({{"VersionInfo", versionInfo}, {"Name", name}, {"Id", id}}))
{
}

ErrCode INTERFACE_FUNC ModuleInfoImpl::getVersionInfo(IVersionInfo** versionInfo)
{
    if (!versionInfo)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *versionInfo = this->fields.get("VersionInfo").asPtr<IVersionInfo>().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC ModuleInfoImpl::getName(IString** name)
{
    if (!name)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *name = this->fields.get("Name").asPtr<IString>().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC ModuleInfoImpl::getId(IString** id)
{
    if (!id)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *id = this->fields.get("Name").asPtr<IString>().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC ModuleInfoImpl::serialize(ISerializer* serializer)
{
    serializer->startTaggedObject(this);
    {
        const VersionInfoPtr versionInfo = this->fields.get("VersionInfo");
        if (versionInfo.assigned())
        {
            serializer->key("versionInfo");
            versionInfo.serialize(serializer);
        }

        const StringPtr name = this->fields.get("Name");
        if (name.assigned())
        {
            serializer->key("name");
            serializer->writeString(name.getCharPtr(), name.getLength());
        }

        const StringPtr id = this->fields.get("Id");
        if (id.assigned())
        {
            serializer->key("id");
            serializer->writeString(id.getCharPtr(), id.getLength());
        }
    }

    serializer->endObject();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC ModuleInfoImpl::getSerializeId(ConstCharPtr* id) const
{
    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

ConstCharPtr ModuleInfoImpl::SerializeId()
{
    return "ModuleInfo";
}

ErrCode ModuleInfoImpl::Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
{
    SerializedObjectPtr serializedObj = SerializedObjectPtr::Borrow(serialized);

    BaseObjectPtr versionInfo;
    ErrCode err = serializedObj->readObject(String("versionInfo"), context, factoryCallback, &versionInfo);
    if (OPENDAQ_FAILED(err) && err != OPENDAQ_ERR_NOTFOUND)
        return err;

    StringPtr name;
    err = serializedObj->readString(String("name"), &name);
    if (OPENDAQ_FAILED(err) && err != OPENDAQ_ERR_NOTFOUND)
        return err;

    StringPtr id;
    err = serializedObj->readString(String("id"), &id);
    if (OPENDAQ_FAILED(err) && err != OPENDAQ_ERR_NOTFOUND)
        return err;

    return createObject<IModuleInfo, ModuleInfoImpl, VersionInfoPtr, StringPtr, StringPtr>(reinterpret_cast<IModuleInfo**>(obj), versionInfo, name, id);
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, ModuleInfo, IVersionInfo*, versionInfo, IString*, name, IString*, id)

END_NAMESPACE_OPENDAQ
