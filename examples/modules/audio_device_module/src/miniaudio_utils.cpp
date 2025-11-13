#define MA_COINIT_VALUE COINIT_APARTMENTTHREADED
#define MA_NO_NODE_GRAPH
#define MA_NO_RESOURCE_MANAGER
#define MINIAUDIO_IMPLEMENTATION

#include <audio_device_module/miniaudio_utils.h>
#include <boost/locale.hpp>
#include <coretypes/exceptions.h>

BEGIN_NAMESPACE_AUDIO_DEVICE_MODULE

namespace ma_utils
{

MiniaudioContext::MiniaudioContext()
{
    if (ma_context_init(nullptr, 0, nullptr, &context) != MA_SUCCESS)
        throw std::runtime_error("Failed to initialize miniaudio context");
}

MiniaudioContext::~MiniaudioContext()
{
    ma_context_uninit(&context);
}

ma_context* MiniaudioContext::getPtr()
{
    return &context;
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

void getMiniAudioDevices(ma_device_info** ppCaptureDeviceInfos, ma_uint32* pCaptureDeviceCount, ma_context* maContext)
{
    ma_result result;
#ifdef MA_WIN32
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
#endif

    result = ma_context_get_devices(maContext, nullptr, nullptr, ppCaptureDeviceInfos, pCaptureDeviceCount);
    if (result != MA_SUCCESS)
        DAQ_THROW_EXCEPTION(GeneralErrorException, "Miniaudio failed to retrieve device information: {}", ma_result_description(result));

#ifdef MA_WIN32
    CoUninitialize();
#endif
}

void getMiniAudioDeviceInfo(ma_device* pDevice, ma_device_info* pDeviceInfo)
{
#ifdef MA_WIN32
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
#endif

    ma_result result = ma_device_get_info(pDevice, ma_device_type_capture, pDeviceInfo);
    if (result != MA_SUCCESS)
        DAQ_THROW_EXCEPTION(CreateFailedException, "Failed to get Miniaudio device information: {}", ma_result_description(result));

#ifdef MA_WIN32
    CoUninitialize();
#endif
}

}

END_NAMESPACE_AUDIO_DEVICE_MODULE
