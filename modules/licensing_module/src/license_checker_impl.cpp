#include <licensing_module/license_checker_impl.h>

BEGIN_NAMESPACE_LICENSING_MODULE

LicenseChecker::LicenseChecker(const std::map<std::string, unsigned int>& featureTokens) 
    : mFeatureTokens(featureTokens)
    , mutex()
{
}

ErrCode LicenseChecker::getComponentTypes(IList** componentTypes)
{
    OPENDAQ_PARAM_NOT_NULL(componentTypes);

    ListPtr<IString> componentTypesLocal;
    for (auto pair : mFeatureTokens)
    {
        IString* ptr;
        createString(&ptr, pair.first.c_str() );
        componentTypesLocal->pushBack(ptr);
    }

    return ErrCode();
}

ErrCode LicenseChecker::getNumberOfAvailableTokens(IString* componentId, Int* availableTokens)
{
    OPENDAQ_PARAM_NOT_NULL(componentId);
    OPENDAQ_PARAM_NOT_NULL(availableTokens);

    const std::string featureName = daq::StringPtr::Borrow(componentId);

    try
    {
        int nTokens = mFeatureTokens.at(featureName);
        *availableTokens = nTokens;
    }
    catch (std::out_of_range e)
    {
        return OPENDAQ_ERR_INVALIDPARAMETER;
    }

    return OPENDAQ_SUCCESS;
}

ErrCode LicenseChecker::checkOut(IString* feature, SizeT count)
{
    OPENDAQ_PARAM_NOT_NULL(feature);
    OPENDAQ_PARAM_GT(count, 0);

    // Take lock to protect the dictionary from race conditions
    std::lock_guard<std::mutex> lock(mutex);

    const std::string featureName = daq::StringPtr::Borrow(feature);
    const auto it = mFeatureTokens.find(featureName);
    if (it != mFeatureTokens.cend())
    {
        auto& tokensAvailable = it->second;

        OPENDAQ_PARAM_GE(tokensAvailable, count);

        tokensAvailable -= count;

        return OPENDAQ_SUCCESS;
    }
    else
    {
        return OPENDAQ_ERR_INVALID_ARGUMENT;
    }
}

ErrCode LicenseChecker::checkIn(IString* feature, SizeT count)
{
    OPENDAQ_PARAM_NOT_NULL(feature);
    OPENDAQ_PARAM_GT(count, 0);

    // Take lock to protect the dictionary from race conditions
    std::lock_guard<std::mutex> lock(mutex);

    const std::string featureName = daq::StringPtr::Borrow(feature);
    const auto it = mFeatureTokens.find(featureName);
    if (it != mFeatureTokens.cend())
    {
        it->second += count;

        return OPENDAQ_SUCCESS;
    }
    else
    {
        return OPENDAQ_ERR_INVALID_ARGUMENT;
    }
}

END_NAMESPACE_LICENSING_MODULE
