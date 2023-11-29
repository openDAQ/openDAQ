#include <opendaq/signal_impl.h>

BEGIN_NAMESPACE_OPENDAQ

#if !defined(BUILDING_STATIC_LIBRARY)

extern "C"
daq::ErrCode PUBLIC_EXPORT createSignal(
    ISignalConfig** objTmp, IContext* context,
    IComponent* parent, IString* localId, 
    IString* className)
{
	return daq::createObject<ISignalConfig, SignalImpl, IContext*, IDataDescriptor*, IComponent*, IString*, IString*>(
		objTmp, context, nullptr, parent, localId, className);
}

using SignalWithDescriptorImpl = SignalImpl;

extern "C"
daq::ErrCode PUBLIC_EXPORT createSignalWithDescriptor(
	ISignalConfig** objTmp, IContext* context, 
	IDataDescriptor* descriptor, IComponent* parent, 
	IString* localId, IString* className)
{
	return daq::createObject<ISignalConfig, SignalWithDescriptorImpl, IContext*, IDataDescriptor*, IComponent*, IString*, IString*>(
		objTmp, context, descriptor, parent, localId, className);
}

#endif

END_NAMESPACE_OPENDAQ
