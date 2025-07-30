#include <licensing_module/license_checker_impl.h>

BEGIN_NAMESPACE_LICENSING_MODULE

LicenseChecker::LicenseChecker(const std::map<std::string, unsigned int>& tokens) 
    : _featureTokens(tokens)
    , _mutex()
{
}

ErrCode LicenseChecker::getComponentTypes(IList** componentTypes)
{
    OPENDAQ_PARAM_NOT_NULL(componentTypes);

    ListPtr<IString> componentTypesLocal;
    for (auto pair : _featureTokens)
    {
        StringPtr ptr;
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
        int nTokens = _featureTokens.at(featureName);
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
    std::lock_guard<std::mutex> lock(_mutex);

    const std::string featureName = daq::StringPtr::Borrow(feature);
    const auto it = _featureTokens.find(featureName);
    if (it != _featureTokens.cend())
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
    std::lock_guard<std::mutex> lock(_mutex);

    const std::string featureName = daq::StringPtr::Borrow(feature);
    const auto it = _featureTokens.find(featureName);
    if (it != _featureTokens.cend())
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
