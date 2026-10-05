#include <opendaq/multi_reader2_impl.h>
#include <opendaq/multi_reader2_params_impl.h>

BEGIN_NAMESPACE_OPENDAQ

// Kept apart from the implementations so the tests can compile those directly for white-box access
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, MultiReader2Params, IMultiReader2Params)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, MultiReader2, IMultiReader2, IMultiReader2Params*, params)

END_NAMESPACE_OPENDAQ
