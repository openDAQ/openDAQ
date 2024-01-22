#include <opendaq/io_folder_impl.h>

BEGIN_NAMESPACE_OPENDAQ

using StandardIoFolderImpl = IoFolderImpl<>;

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(StandardIoFolderImpl)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY,
    StandardIoFolder,
    IFolderConfig,
    createIoFolder,
    IContext*, context,
    IComponent*, parent,
    IString*, localId)

END_NAMESPACE_OPENDAQ
