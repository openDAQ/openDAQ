#include <licensing_module/licensing_module_impl.h>
#include <licensing_module/passthrough_fb_impl.h>
#include <fstream>
#include <regex>

const std::regex tokenLineRegex(R"(^\s*(\w+)\s*[:-]?\s*(\d+)\s*$)");

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

Bool LicensingModule::onLoadLicense(IPropertyObject* licenseConfig)
{
    auto ptr = PropertyObjectPtr::Borrow(licenseConfig);
    std::string path = ptr.getPropertyValue("LicensePath");
    std::string vendor_key = ptr.getPropertyValue("VendorKey");

    std::string secret_key = "my_secret_key";

    _authenticated = vendor_key == secret_key;

    if (!_authenticated)
    {
        LOG_W("Authentication with \"{}\" failed, invalid key!", path);
        return false;
    }
    else
    {
        LOG_I("Authentication successful!");
    }

    std::ifstream file(path);
    if (!file.is_open())
    {
        LOG_W("License file \"{}\" not found!", path);
        return false;
    }

    std::map<std::string, unsigned int> tokens;

    std::string line;

    // Read each line
    auto lineNumber = 0;
    while (std::getline(file, line))
    {
        lineNumber++;
        if (line.empty() || line[0] == '#')
            continue;  // Skip empty lines and comments

        std::smatch match;
        if (std::regex_match(line, match, tokenLineRegex))
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
    auto licenseConfig = PropertyObject();
    licenseConfig.addProperty(StringProperty("LicensePath", ""));
    licenseConfig.addProperty(StringProperty("VendorKey", ""));

    return licenseConfig;
}

Bool LicensingModule::onLicenseLoaded()
{
    return _licenseChecker == nullptr;
}

END_NAMESPACE_LICENSING_MODULE
