#include <licensing_module/license_checker_impl.h>
#include <coretypes/string_ptr.h>
#include <coretypes/listobject_factory.h>

BEGIN_NAMESPACE_LICENSING_MODULE

LicenseChecker::LicenseChecker(const std::map<std::string, unsigned int>& tokens) 
    : _featureTokens(tokens)
    , _mutex()
{
}

ErrCode LicenseChecker::getComponentTypes(IList** componentTypes)
{
    OPENDAQ_PARAM_NOT_NULL(componentTypes);

    auto componentTypesLocal = List<IString>();
    for (const auto& [key, _] : _featureTokens)
        componentTypesLocal.pushBack(key);

    *componentTypes = componentTypesLocal.detach();

    return OPENDAQ_SUCCESS;
}

ErrCode LicenseChecker::getNumberOfAvailableTokens(IString* componentId, Int* availableTokens)
{
    OPENDAQ_PARAM_NOT_NULL(componentId);
    OPENDAQ_PARAM_NOT_NULL(availableTokens);

    const std::string featureName = daq::StringPtr::Borrow(componentId);

    const auto it = _featureTokens.find(featureName);
    if (it != _featureTokens.cend())
    {
        *availableTokens = it->second;
        return OPENDAQ_SUCCESS;
    }

    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND);
}

ErrCode LicenseChecker::checkOut(IString* feature, SizeT count)
{
    OPENDAQ_PARAM_NOT_NULL(feature);
    if (count == 0)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER);

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

    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND);
}

ErrCode LicenseChecker::checkIn(IString* feature, SizeT count)
{
    OPENDAQ_PARAM_NOT_NULL(feature);
    if (count == 0)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER);

    // Take lock to protect the dictionary from race conditions
    std::lock_guard<std::mutex> lock(_mutex);

    const std::string featureName = daq::StringPtr::Borrow(feature);
    const auto it = _featureTokens.find(featureName);
    if (it != _featureTokens.cend())
    {
        it->second += count;
        return OPENDAQ_SUCCESS;
    }

    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND);
}

END_NAMESPACE_LICENSING_MODULE
