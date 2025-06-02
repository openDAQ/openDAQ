#include "py_opendaq/py_instance_builder_impl.h"

BEGIN_NAMESPACE_OPENDAQ

ErrCode PythonInstanceBuilderImpl::build(IInstance** instance)
{
    OPENDAQ_PARAM_NOT_NULL(instance);

    const auto builderPtr = this->borrowPtr<InstanceBuilderPtr>();
    return createInstanceFromBuilder(instance, builderPtr);
}

// patch to override the default Context factory
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY,
    PythonInstanceBuilder,
    IInstanceBuilder,
    createInstanceBuilder
)
END_NAMESPACE_OPENDAQ