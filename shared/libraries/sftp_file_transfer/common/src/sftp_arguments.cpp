#include <sftp_file_transfer/sftp_arguments.h>

#include <sftp_file_transfer/sftp_authentication_config_factory.h>
#include <coretypes/exceptions.h>
#include <fmt/format.h>
#include <opendaq/security_exceptions.h>

#include <regex>

namespace daq::sftp
{

namespace detail
{

// The host patterns of the openDAQ client modules (native, OPC UA, websocket), without their scheme and path parts.
// The port is limited to 5 digits so std::stoi cannot overflow; its range is checked by the caller.
const std::regex Ipv6Address(R"(^\[([a-fA-F0-9:]+(?:%[a-zA-Z0-9_.~-]+)?)\](?::(\d{1,5}))?$)");
const std::regex HostAddress(R"(^([^:/\s]+)(?::(\d{1,5}))?$)");

BaseObjectPtr propertyValue(const PropertyObjectPtr& config, const StringPtr& name, CoreType type, const char* typeName)
{
    if (!config.hasProperty(name))
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Config has no property \"{}\"", name);
    const BaseObjectPtr value = config.getPropertyValue(name);
    if (!value.assigned() || value.getCoreType() != type)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Config property \"{}\" must be {}", name, typeName);
    return value;
}

}

// ---------------------------------------------------------------------------
// Parsing
// ---------------------------------------------------------------------------

Endpoint parseEndpoint(const StringPtr& address)
{
    const std::string text = address.toStdString();
    std::smatch match;
    if (!std::regex_match(text, match, detail::Ipv6Address) && !std::regex_match(text, match, detail::HostAddress))
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Invalid SFTP address \"{}\"", address);

    Endpoint endpoint;
    endpoint.host = String(match.str(1));
    if (match[2].matched)
    {
        const int port = std::stoi(match.str(2));
        if (port == 0 || port > 65535)
            DAQ_THROW_EXCEPTION(InvalidParameterException, "Invalid port in SFTP address \"{}\"", address);
        endpoint.port = static_cast<std::uint16_t>(port);
    }
    return endpoint;
}

// ---------------------------------------------------------------------------
// Factory arguments
// ---------------------------------------------------------------------------

void validateArguments(const StringPtr& address,
                       const AuthenticationConfigPtr& authenticationConfig,
                       const LoggerPtr& logger,
                       const PropertyObjectPtr& config)
{
    if (!logger.assigned())
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Logger is not assigned");
    if (!address.assigned())
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Address is not assigned");
    if (!authenticationConfig.assigned())
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Authentication config is not assigned");
    if (!config.assigned())
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Config is not assigned");

    const StringPtr methodId = authenticationConfig.getSelectedAuthenticationMethod().getId();
    const DictPtr<IString, IString> supplied = authenticationConfig.getSuppliedCredential();
    if (!supplied.assigned() || supplied.getCount() == 0)
        DAQ_THROW_EXCEPTION(CredentialsRequiredException, "No credential supplied for the \"{}\" authentication method", methodId);
    if (methodId != SftpPrivateKeyFileId && methodId != SftpUserNamePasswordId)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Authentication method \"{}\" is not supported by SFTP file transfer", methodId);
}

// ---------------------------------------------------------------------------
// Backend config
// ---------------------------------------------------------------------------

Int readIntPropertyValue(const PropertyObjectPtr& config, const StringPtr& name, Int minValue)
{
    const Int value = detail::propertyValue(config, name, ctInt, "an integer");
    if (value < minValue)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Config property \"{}\" is {}, must be at least {}", name, value, minValue);
    return value;
}

StringPtr readStringPropertyValue(const PropertyObjectPtr& config, const StringPtr& name)
{
    return detail::propertyValue(config, name, ctString, "a string");
}

}
