#include <opendaq/io_folder_impl.h>
#include <opendaq/channel.h>

BEGIN_NAMESPACE_OPENDAQ

IoFolderImpl::IoFolderImpl(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& localId)
    : Super(context, parent, localId)
{
}

ErrCode INTERFACE_FUNC IoFolderImpl::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

ConstCharPtr IoFolderImpl::SerializeId()
{
    return "IoFolder";
}

ErrCode IoFolderImpl::Deserialize(ISerializedObject* serialized, IBaseObject* context, IBaseObject** obj)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

bool IoFolderImpl::addItemInternal(const ComponentPtr& component)
{
    if (!component.supportsInterface<IIoFolderConfig>() && !component.supportsInterface<IChannel>())
        throw InvalidParameterException("Type of item not allowed in the folder");

    return Super::addItemInternal(component);
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, IoFolder, IFolderConfig, 
    IContext*, context,
    IComponent*, parent,
    IString*, localId)

END_NAMESPACE_OPENDAQ
