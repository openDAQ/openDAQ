#include <audio_device_module/audio_device_utils.h>
#include <coreobjects/unit_factory.h>
#include <audio_device_module/version.h>
#include <boost/locale.hpp>

BEGIN_NAMESPACE_AUDIO_DEVICE_MODULE

namespace utils
{

LoggerComponentPtr getLoggerComponent(const LoggerPtr& logger)
{
    if (!logger.assigned())
        throw ArgumentNullException("Logger must not be null!");

    return logger.getOrAddComponent("AudioDeviceModule");
}

ma_device_id getIdFromConnectionString(const std::string& connectionString)
{
    const std::string prefix = "miniaudio://";

    if (auto pos = connectionString.find(prefix); pos == std::string::npos)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Miniaudio device connection strings must have the \"miniaudio://\" prefix.");

    auto rest = connectionString.substr(prefix.size(), std::string::npos);
    auto pos = rest.find("/");
    if (pos == std::string::npos)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Miniaudio connection string lacks a \"/\" delimiter.");

    auto backendId = rest.substr(0, pos);
    auto id = rest.substr(pos + 1, std::string::npos);

    ma_device_id deviceId{};
    if (backendId == "wasapi")
    {
        std::wstring wasapiId = boost::locale::conv::utf_to_utf<wchar_t>(id);
        std::memcpy(&deviceId.wasapi[0], wasapiId.c_str(), wasapiId.size() * sizeof(wchar_t));
        deviceId.wasapi[wasapiId.size()] = '\0';
    }
    else if (backendId == "dsound")
    {
        for (size_t i = 0; i < 16; ++i)
        {
            auto subStr = id.substr(i * 2, 2);
            deviceId.dsound[i] = static_cast<ma_uint8>(std::stoul(subStr, nullptr, 16));
        }
    }
    else if (backendId == "winmm")
    {
        deviceId.winmm = std::stoul(id, nullptr, 16);
    }
    else if (backendId == "coreaudio")
    {
        std::strcpy(deviceId.coreaudio, id.c_str());
    }
    else if (backendId == "alsa")
    {
        std::strcpy(deviceId.alsa, id.c_str());
    }
    else if (backendId == "pulseaudio")
    {
        std::strcpy(deviceId.pulse, id.c_str());
    }
    else if (backendId == "jack")
    {
        deviceId.jack = std::stoul(id, nullptr, 16);
    }
    else
    {
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Unsupported Miniaudio backend {}.", backendId);
    }

    return deviceId;
}

std::string getConnectionStringFromId(ma_backend backend, ma_device_id id)
{
    std::string connectionString = "miniaudio://";

    switch (backend)
    {
        case ma_backend_wasapi:
            connectionString += "wasapi/";
            connectionString += boost::locale::conv::utf_to_utf<char>(id.wasapi);
            break;

        case ma_backend_dsound:
            connectionString += "dsound/";
            for (unsigned char& i : id.dsound)
                connectionString += fmt::format("{:02x}", i);
            break;

        case ma_backend_winmm:
            connectionString += "winmm/";
            connectionString += fmt::format("{:x}", id.winmm);
            break;

        case ma_backend_coreaudio:
            connectionString += "coreaudio/";
            connectionString += id.coreaudio;
            break;

        case ma_backend_sndio:
            connectionString += "sndio/";
            break;

        case ma_backend_audio4:
            connectionString += "audio4/";
            break;

        case ma_backend_oss:
            connectionString += "oss/";
            break;

        case ma_backend_pulseaudio:
            connectionString += "pulseaudio/";
            connectionString += id.pulse;
            break;

        case ma_backend_alsa:
            connectionString += "alsa/";
            connectionString += id.alsa;
            break;

        case ma_backend_jack:
            connectionString += "jack/";
            connectionString += fmt::format("{:x}", id.jack);
            break;

        case ma_backend_aaudio:
            connectionString += "aaudio/";
            break;

        case ma_backend_opensl:
            connectionString += "opensl/";
            break;

        default:
            connectionString += "unknown/";
    }

    return connectionString;
}
}

END_NAMESPACE_AUDIO_DEVICE_MODULE
