#include <licensing_module/licensing_module_impl.h>
#include <licensing_module/passthrough_fb_impl.h>
#include <fstream>

BEGIN_NAMESPACE_LICENSING_MODULE

LicensingModule::LicensingModule(const ContextPtr& context)
    : Module("LicensingModule",
             daq::VersionInfo(LICENSING_MODULE_MAJOR_VERSION, LICENSING_MODULE_MINOR_VERSION, LICENSING_MODULE_PATCH_VERSION),
            context,
            "LicensingModule")
    , _licenseChecker(nullptr)
    , _authenticated(false)
{
}

DictPtr<IString, IFunctionBlockType> LicensingModule::onGetAvailableFunctionBlockTypes()
{
    auto types = Dict<IString, IFunctionBlockType>();

    auto type = PassthroughFbImpl::CreateType();
    types.set(type.getId(), type);

    return types;
}

FunctionBlockPtr LicensingModule::onCreateFunctionBlock(const StringPtr& id,
                                                        const ComponentPtr& parent,
                                                        const StringPtr& localId,
                                                        const PropertyObjectPtr& config)
{
    if (!_authenticated)
    {
        LOG_W("Module not authenticated, cannot create function block!");
        return nullptr;
    }

    if (id == PassthroughFbImpl::CreateType().getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, PassthroughFbImpl>(context, parent, localId, _licenseChecker);
        return fb;
    }

    LOG_W("Function block \"{}\" not found", id);
    DAQ_THROW_EXCEPTION(NotFoundException, "Function block not found");
}

Bool LicensingModule::onAuthenticate(IPropertyObject* authenticationConfig)
{
    auto ptr = PropertyObjectPtr::Borrow(authenticationConfig);
    std::string path = ptr.getPropertyValue("AuthenticationKeyPath");

    std::string secret_key = "my_secret_key";
    std::string path_key;
    std::ifstream file(path);
    if (!file.is_open())
    {
        LOG_W("Authentication file \"{}\" not found!", path);
        return false;
    }
    std::getline(file, path_key);

    _authenticated = path_key == secret_key;

    if (!_authenticated)
    {
        LOG_W("Authentication with \"{}\" failed, invalid key!", path);
    }
    else
    {
        LOG_I("Authentication successful!");
    }

    return _authenticated;
}

PropertyObjectPtr LicensingModule::onGetAuthenticationConfig()
{
    return nullptr;
}

Bool LicensingModule::onIsAuthenticated()
{
    return _authenticated;
}

Bool LicensingModule::onLoadLicense(IPropertyObject* licenseConfig)
{
    auto ptr = PropertyObjectPtr::Borrow(licenseConfig);
    std::string path = ptr.getPropertyValue("LicensePath");

    std::ifstream file(path);
    if (!file.is_open())
    {
        return false;
    }

    std::map<std::string, unsigned int> tokens;

    const std::regex lineRegex(R"(^\s*(\w+)\s*[:-]?\s*(\d+)\s*$)");
    std::string line;

    // Read each line
    auto lineNumber = 0;
    while (std::getline(file, line))
    {
        lineNumber++;
        if (line.empty() || line[0] == '#')
            continue;  // Skip empty lines and comments

        std::smatch match;
        if (std::regex_match(line, match, lineRegex))
        {
            // Extract feature name and count, and build map
            const auto featureName = match[1].str();
            const auto count = static_cast<SizeT>(std::stoull(match[2].str()));

            tokens[featureName] = count;
        }
        else
        {
            LOG_W("Unexpected line #{} in license file: {}", lineNumber, line);
        }
    }
    if (tokens.size() == 0)
    {
        LOG_W("Invalid license, no function blocks / components founds!");
        return false;
    }
    _licenseChecker = std::make_shared<LicenseChecker>(tokens);

    return true;
}

PropertyObjectPtr LicensingModule::onGetLicenseConfig()
{
    if (!_licenseChecker)
    {
        return nullptr;
    }

    ListPtr<IString> list;
    _licenseChecker->getComponentTypes(&list);

    PropertyObjectPtr propertyObject;
    Int value;

    for (auto item : list)
    {
        propertyObject.addProperty(PropertyPtr(item));
        _licenseChecker->getNumberOfAvailableTokens(item,&value);
        propertyObject.setPropertyValue(item, value);
    }

    return propertyObject;
}

Bool LicensingModule::onLicenseLoaded()
{
    return _licenseChecker == nullptr;
}

END_NAMESPACE_LICENSING_MODULE
