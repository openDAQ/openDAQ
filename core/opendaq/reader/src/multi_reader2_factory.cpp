#include <opendaq/multi_reader2_impl.h>
#include <opendaq/multi_reader2_params_impl.h>

BEGIN_NAMESPACE_OPENDAQ

// Factory definitions live apart from the impl files: the test target compiles those directly
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, MultiReader2, IMultiReader2, IMultiReader2Params*, params)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, MultiReader2Params, IMultiReader2Params)

END_NAMESPACE_OPENDAQ
