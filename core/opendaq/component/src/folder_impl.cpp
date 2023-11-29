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

END_NAMESPACE_OPENDAQ
