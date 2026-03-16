#include <opendaq/module_check_dependencies.h>
#include <coretypes/stringobject_factory.h>
#include <opendaq/opendaq_config.h>

BEGIN_NAMESPACE_OPENDAQ

// C-style wrapper that forwards to lambda
static void enumerateMetadataFieldWrapper(const char* key, const char* value, void* userData)
{
    auto* map = static_cast<std::unordered_map<std::string, std::string>*>(userData);
    (*map)[key] = value;
}

extern "C"
ErrCode PUBLIC_EXPORT daqCoreValidateVersionMetadata(const GetCoreVersionMetadataFunc& getMetadata, IString** logMessage)
{
    OPENDAQ_PARAM_NOT_NULL(logMessage);
    OPENDAQ_PARAM_NOT_NULL(getMetadata);

    std::unordered_map<std::string, std::string> coreVersionMetadata;

    ErrCode errCode = getMetadata(&enumerateMetadataFieldWrapper, &coreVersionMetadata);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    auto runningSdkMetadataAsString =
        fmt::format(R"([[ 'major': '{}'; 'minor': '{}'; 'patch': '{}'; 'branch': '{}'; 'sha': '{}'; ]])",
                    OPENDAQ_OPENDAQ_MAJOR_VERSION_STR,
                    OPENDAQ_OPENDAQ_MINOR_VERSION_STR,
                    OPENDAQ_OPENDAQ_PATCH_VERSION_STR,
                    OPENDAQ_OPENDAQ_BRANCH_NAME,
                    OPENDAQ_OPENDAQ_REVISION_HASH);
    std::string inModuleMetadataAsString("[[ ");
    for (const auto& [key, value] : coreVersionMetadata)
    {
        inModuleMetadataAsString += fmt::format(R"('{}': '{}'; )", key, value);
    }
    inModuleMetadataAsString += "]]";

    auto message = fmt::format(R"("The core libraries version metadata of loading module "{}".)", inModuleMetadataAsString);

    if (auto iter = coreVersionMetadata.find("major"); iter != coreVersionMetadata.end())
    {
        if (iter->second != OPENDAQ_OPENDAQ_MAJOR_VERSION_STR)
        {
            return DAQ_MAKE_ERROR_INFO(
                OPENDAQ_ERR_NO_COMPATIBLE_VERSION,
                fmt::format(R"(The running version of openDAQ is: "{}"; the core SDK libraries version has been used to build module is incompatible: "{}" - the major number mismatches.)",
                            runningSdkMetadataAsString,
                            inModuleMetadataAsString
                )
            );
        }
    }
    else
    {
        return DAQ_MAKE_ERROR_INFO(
            OPENDAQ_ERR_NO_COMPATIBLE_VERSION,
            fmt::format(R"(The module has not provided the version major number of core SDK libraries has been used to build it, in the version metadata: "{}")",
                        inModuleMetadataAsString
            )
        );
    }

    if (auto iter = coreVersionMetadata.find("minor"); iter != coreVersionMetadata.end())
    {
        if (iter->second != OPENDAQ_OPENDAQ_MINOR_VERSION_STR)
        {
            return DAQ_MAKE_ERROR_INFO(
                OPENDAQ_ERR_NO_COMPATIBLE_VERSION,
                fmt::format(R"(The running version of openDAQ is: "{}"; the core SDK libraries version has been used to build module is incompatible: "{}" - the minor number mismatches.)",
                            runningSdkMetadataAsString,
                            inModuleMetadataAsString
                )
            );
        }
    }
    else
    {
        return DAQ_MAKE_ERROR_INFO(
            OPENDAQ_ERR_NO_COMPATIBLE_VERSION,
            fmt::format(R"(The module has not provided the version minor number of core SDK libraries has been used to build it, in the version metadata: "{}")",
                        inModuleMetadataAsString
            )
        );
    }

    if (auto iter = coreVersionMetadata.find("patch"); iter != coreVersionMetadata.end())
    {
        if (iter->second != OPENDAQ_OPENDAQ_PATCH_VERSION_STR)
        {
            message +=
                fmt::format("\nThe running version of openDAQ is: \"{}\";\nthe core SDK libraries version has been used to build module differs: \"{}\" - the patch number mismatches.",
                            runningSdkMetadataAsString,
                            inModuleMetadataAsString
                );
            errCode = OPENDAQ_PARTIAL_SUCCESS;
        }
    }
    else
    {
        message +=
            fmt::format("\nThe module has not provided the version patch number of core SDK libraries has been used to build it, in the version metadata: \"{}\"",
                        inModuleMetadataAsString);
        errCode = OPENDAQ_PARTIAL_SUCCESS;
    }

    if (auto iter = coreVersionMetadata.find("branch"); iter != coreVersionMetadata.end())
    {
        if (iter->second != OPENDAQ_OPENDAQ_BRANCH_NAME)
        {
            message +=
                fmt::format("\nThe running version of openDAQ is: \"{}\";\nthe core SDK libraries version has been used to build module differs: \"{}\" - the git branch name mismatches.",
                            runningSdkMetadataAsString,
                            inModuleMetadataAsString
                );
            errCode = OPENDAQ_PARTIAL_SUCCESS;
        }
    }
    else
    {
        message +=
            fmt::format("\nThe module has not provided the git branch name as part of version metadata for SDK libraries has been used to build it: \"{}\"",
                        inModuleMetadataAsString
            );
        errCode = OPENDAQ_PARTIAL_SUCCESS;
    }

    if (auto iter = coreVersionMetadata.find("sha"); iter != coreVersionMetadata.end())
    {
        if (iter->second != OPENDAQ_OPENDAQ_REVISION_HASH)
        {
            message +=
                fmt::format("\nThe running version of openDAQ is: \"{}\";\nthe core SDK libraries version has been used to build module differs: \"{}\" - the git commit sha mismatches.",
                            runningSdkMetadataAsString,
                            inModuleMetadataAsString
                );
            errCode = OPENDAQ_PARTIAL_SUCCESS;
        }
    }
    else
    {
        message +=
            fmt::format("\nThe module has not provided the git commit sha as part of version metadata for SDK libraries has been used to build it: \"{}\"",
                        inModuleMetadataAsString
            );
        errCode = OPENDAQ_PARTIAL_SUCCESS;
    }

    *logMessage = String(message).detach();
    return errCode;
}

END_NAMESPACE_OPENDAQ
