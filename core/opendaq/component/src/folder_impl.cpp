#include <opendaq/component_impl.h>
#include <opendaq/folder_impl.h>

BEGIN_NAMESPACE_OPENDAQ

using FolderInternalImpl = FolderImpl<IFolderConfig>;

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, FolderInternal, IFolderConfig, createFolder,
    IContext*, context,
    IComponent*, parent,
    IString*, localId)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, FolderInternal, IFolderConfig, createFolderWithItemType,
    IntfID, itemId,
    IContext*, context,
    IComponent*, parent,
    IString*, localId)

#if !defined(BUILDING_STATIC_LIBRARY)
extern "C" daq::ErrCode PUBLIC_EXPORT createFolderWithDefaultPropertyMode(IFolderConfig** objTmp,
                                                                          IContext* context,
                                                                          IComponent* parent,
                                                                          IString* localId,
                                                                          Int propertyMode)
{
    return daq::createObject<IFolderConfig, FolderInternalImpl, IContext*, IComponent*, IString*, IString*, ComponentStandardProps>(
        objTmp,
        context,
        parent,
        localId,
        nullptr,
        static_cast<ComponentStandardProps>(propertyMode));
}

extern "C" daq::ErrCode PUBLIC_EXPORT createFolderWithItemTypeAndDefaultPropertyMode(IFolderConfig** objTmp,
                                                                                     IntfID itemType,
                                                                                     IContext* context,
                                                                                     IComponent* parent,
                                                                                     IString* localId,
                                                                                     Int propertyMode)
{
    return daq::createObject<IFolderConfig, FolderInternalImpl, IntfID, IContext*, IComponent*, IString*, IString*, ComponentStandardProps>(
        objTmp,
        itemType,
        context,
        parent,
        localId,
        nullptr,
        static_cast<ComponentStandardProps>(propertyMode));
}
#endif

END_NAMESPACE_OPENDAQ
