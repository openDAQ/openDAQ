#include <sftp_file_transfer/sftp_arguments.h>

#include <sftp_file_transfer/sftp_authentication_config_factory.h>
#include <coretypes/exceptions.h>
#include <fmt/format.h>
#include <opendaq/security_exceptions.h>

#include <charconv>

namespace daq::sftp
{

namespace detail
{

std::uint16_t parsePort(std::string_view text, std::string_view address)
{
    unsigned int port = 0;
    const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), port);
    if (ec != std::errc{} || end != text.data() + text.size() || port == 0 || port > 65535)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Invalid port in SFTP address \"{}\"", address);
    return static_cast<std::uint16_t>(port);
}

}

// ---------------------------------------------------------------------------
// Parsing
// ---------------------------------------------------------------------------

Endpoint parseEndpoint(const StringPtr& addressString)
{
    const std::string_view address = addressString.toView();
    Endpoint endpoint;

    if (!address.empty() && address.front() == '[')
    {
        const auto close = address.find(']');
        if (close == std::string_view::npos || close == 1)
            DAQ_THROW_EXCEPTION(InvalidParameterException, "Invalid SFTP address \"{}\"", address);
        const std::string_view host = address.substr(1, close - 1);
        endpoint.host = String(host.data(), host.size());

        const auto rest = address.substr(close + 1);
        if (!rest.empty())
        {
            if (rest.front() != ':')
                DAQ_THROW_EXCEPTION(InvalidParameterException, "Invalid SFTP address \"{}\"", address);
            endpoint.port = detail::parsePort(rest.substr(1), address);
        }
        return endpoint;
    }

    const auto colon = address.find(':');
    if (colon != std::string_view::npos && address.find(':', colon + 1) == std::string_view::npos)
    {
        endpoint.host = String(address.data(), colon);
        endpoint.port = detail::parsePort(address.substr(colon + 1), address);
    }
    else
    {
        // No colon, or several: a host name or a bare IPv6 address without a port.
        endpoint.host = addressString;
    }

    if (endpoint.host.getLength() == 0)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "SFTP address \"{}\" has no host", address);
    return endpoint;
}

// ---------------------------------------------------------------------------
// Factory arguments
// ---------------------------------------------------------------------------

void validateArguments(const StringPtr& address, const AuthenticationConfigPtr& authenticationConfig, const LoggerPtr& logger)
{
    if (!logger.assigned())
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Logger is not assigned");
    if (!address.assigned())
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Address is not assigned");
    if (!authenticationConfig.assigned())
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Authentication config is not assigned");

    const StringPtr methodId = authenticationConfig.getSelectedAuthenticationMethod().getId();
    const DictPtr<IString, IString> supplied = authenticationConfig.getSuppliedCredential();
    if (!supplied.assigned() || supplied.getCount() == 0)
        DAQ_THROW_EXCEPTION(CredentialsRequiredException, "No credential supplied for the \"{}\" authentication method", methodId);
    if (methodId != SftpPrivateKeyFileId && methodId != SftpUserNamePasswordId)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Authentication method \"{}\" is not supported by SFTP file transfer", methodId);
}

}
