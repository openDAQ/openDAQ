#include <opendaq/io_folder_impl.h>

BEGIN_NAMESPACE_OPENDAQ

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, IoFolder, IFolderConfig,
    IContext*, context,
    IComponent*, parent,
    IString*, localId)

#if !defined(BUILDING_STATIC_LIBRARY)
extern "C" daq::ErrCode PUBLIC_EXPORT createIoFolderWithDefaultPropertyMode(IFolderConfig** objTmp,
                                                                            IContext* context,
                                                                            IComponent* parent,
                                                                            IString* localId,
                                                                            Int propertyMode)
{
    return daq::createObject<IFolderConfig, IoFolderImpl, IContext*, IComponent*, IString*, IString*, ComponentStandardProps>(
        objTmp,
        context,
        parent,
        localId,
        nullptr,
        static_cast<ComponentStandardProps>(propertyMode));
}
#endif

END_NAMESPACE_OPENDAQ
