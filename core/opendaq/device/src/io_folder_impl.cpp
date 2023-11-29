#include <opendaq/io_folder_impl.h>

BEGIN_NAMESPACE_OPENDAQ

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, IoFolder, IFolderConfig,
    IContext*, context,
    IComponent*, parent,
    IString*, localId)

END_NAMESPACE_OPENDAQ
