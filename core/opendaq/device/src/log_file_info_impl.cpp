

#include <opendaq/log_file_info_impl.h>
#include <coretypes/validation.h>
#include <coretypes/filesystem.h>
#include <chrono>
#include <fstream>

BEGIN_NAMESPACE_OPENDAQ

LogFileInfoImpl::LogFileInfoImpl(const StringPtr& localPath,
                                 const StringPtr& name,
                                 const StringPtr& description,
                                 LogFileEncodingType encoding)
    : localPath(localPath),
      name(name),
      description(description),
      encoding(encoding)
{
    if (name == nullptr || name.getLength() == 0)
        throw InvalidParameterException("Log file name is not assigned or empty.");

    if (localPath.assigned())
        this->id = localPath + "/" + name;
    else
        this->id = name;
}

LogFileInfoImpl::LogFileInfoImpl(const LogFileInfoBuilderPtr& builder)
{
    if (builder == nullptr)
        throw InvalidParameterException();

    this->localPath = builder.getLocalPath();
    this->name = builder.getName();
    this->description = builder.getDescription();
    this->encoding = builder.getEncoding();

    if (this->name == nullptr || this->name.getLength() == 0)
        throw InvalidParameterException("Log file name is not assigned or empty.");

    if (localPath.assigned())
        this->id = localPath + "/" + name;
    else
        this->id = name;
}

ErrCode LogFileInfoImpl::getId(IString** id)
{
    OPENDAQ_PARAM_NOT_NULL(id);
    *id = this->id.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode LogFileInfoImpl::getLocalPath(IString** localPath)
{
    OPENDAQ_PARAM_NOT_NULL(localPath);
    *localPath = this->localPath.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode LogFileInfoImpl::getName(IString** name)
{
    OPENDAQ_PARAM_NOT_NULL(name);
    *name = this->name.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode LogFileInfoImpl::getDescription(IString** description)
{
    OPENDAQ_PARAM_NOT_NULL(description);
    *description = this->description.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode LogFileInfoImpl::getSize(SizeT* size)
{
    OPENDAQ_PARAM_NOT_NULL(size);

    try 
    {
        fs::path path(id);

        if (!fs::exists(path)) 
        {
           this->makeErrorInfo(OPENDAQ_ERR_NOTFOUND, "Log file \"%s\" does not exist.", id.getCharPtr());
        }

        *size = fs::file_size(path);
    } 
    catch (const fs::filesystem_error& e) 
    {
       return this->makeErrorInfo(OPENDAQ_ERR_GENERALERROR, "Error getting size for log file \"%s\": %s", id.getCharPtr(), e.what());
    }

    return OPENDAQ_SUCCESS;
}

ErrCode LogFileInfoImpl::getEncoding(LogFileEncodingType* encoding)
{
    OPENDAQ_PARAM_NOT_NULL(encoding);
    *encoding = this->encoding;
    return OPENDAQ_SUCCESS;
}

std::string toIso8601(const std::chrono::system_clock::time_point& timePoint) 
{
    std::time_t time = std::chrono::system_clock::to_time_t(timePoint);
    std::tm tm = *std::gmtime(&time);  // Use gmtime for UTC

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ"); // ISO 8601 format
    return oss.str();
}

ErrCode LogFileInfoImpl::getLastModified(IString** lastModified)
{
    OPENDAQ_PARAM_NOT_NULL(lastModified);

    try 
    {
        fs::path path(id);

        if (!fs::exists(path)) 
        {
           this->makeErrorInfo(OPENDAQ_ERR_NOTFOUND, "Log file \"%s\" does not exist.", id.getCharPtr());
        }

        auto ftime = fs::last_write_time(path);

        // Convert to time_point
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
        );

        *lastModified = String(toIso8601(sctp)).detach();
    } 
    catch (const fs::filesystem_error& e) 
    {
       return this->makeErrorInfo(OPENDAQ_ERR_GENERALERROR, "Error getting last modified time for log file \"%s\": %s", id.getCharPtr(), e.what());
    }

    return OPENDAQ_SUCCESS;
}

#if !defined(BUILDING_STATIC_LIBRARY)

extern "C"
ErrCode PUBLIC_EXPORT createLogFileInfo(ILogFileInfo** objTmp, IString* localPath, IString* name, IString* description, LogFileEncodingType encoding)
{
    return daq::createObject<ILogFileInfo, LogFileInfoImpl>(objTmp, localPath, name, description, encoding);
}

extern "C"
ErrCode PUBLIC_EXPORT createLogFileInfoFromBuilder(ILogFileInfo** objTmp, ILogFileInfoBuilder* builder)
{
    return daq::createObject<ILogFileInfo, LogFileInfoImpl>(objTmp, builder);
}

#endif

END_NAMESPACE_OPENDAQ

