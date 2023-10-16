#include <cmath>
#include <stdexcept>
#include "opcuashared/opcuacommon.h"
#include <mutex>
#include <open62541/types_generated_handling.h>

#include "open62541/plugin/nodestore.h"

#ifdef OPCUA_ENABLE_ENCRYPTION
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <openssl/x509v3.h>
#endif

using namespace std::chrono;

BEGIN_NAMESPACE_OPENDAQ_OPCUA

namespace utils
{
    double ToSeconds(const UA_DateTime& time)
    {
        return (double) (time) / (double) UA_DATETIME_SEC;
    }

    UA_StatusCode ToUaVariant(double value, const UA_NodeId& dataTypeNodeId, UA_Variant* var)
    {
        if (dataTypeNodeId.namespaceIndex == 0)
        {
            auto dataType = UA_findDataType(&dataTypeNodeId);
            if (dataType)
            {
                switch (dataType->typeKind)
                {
                    case UA_TYPES_DOUBLE:
                        return UA_Variant_setScalarCopy(var, &value, dataType);
                    case UA_TYPES_FLOAT:
                    {
                        float f = (float) value;
                        return UA_Variant_setScalarCopy(var, &f, dataType);
                    }
                    case UA_TYPES_BOOLEAN:
                    {
                        bool b = (value != 0);
                        return UA_Variant_setScalarCopy(var, &b, dataType);
                    }
                    case UA_TYPES_INT16:
                    {
                        int16_t i16 = (int16_t) std::round(value);
                        return UA_Variant_setScalarCopy(var, &i16, dataType);
                    }
                    case UA_TYPES_UINT16:
                    {
                        uint16_t i16 = (uint16_t) std::round(value);
                        return UA_Variant_setScalarCopy(var, &i16, dataType);
                    }
                    case UA_TYPES_INT32:
                    {
                        int32_t i32 = (int32_t) std::round(value);
                        return UA_Variant_setScalarCopy(var, &i32, dataType);
                    }
                    case UA_TYPES_UINT32:
                    {
                        int32_t i32 = (uint32_t) std::round(value);
                        return UA_Variant_setScalarCopy(var, &i32, dataType);
                    }
                    case UA_TYPES_INT64:
                    case UA_TYPES_UINT64:
                    {
                        int64_t i64 = (int64_t) std::round(value);
                        return UA_Variant_setScalarCopy(var, &i64, dataType);
                    }
                }
            }
        }
        throw std::runtime_error("C Exception: unsupported value!");
    }

    void ToUaVariant(const std::string& value, const UA_NodeId& dataTypeNodeId, UA_Variant* var)
    {
        if (dataTypeNodeId.namespaceIndex == 0)
        {
            auto dataType = UA_findDataType(&dataTypeNodeId);
            if (dataType)
            {
                switch (dataType->typeKind)
                {
                    case UA_TYPES_STRING:
                    {
                        UA_String str = UA_STRING((char*) value.c_str());
                        UA_Variant_setScalarCopy(var, &str, dataType);
                        return;
                    }
                    case UA_TYPES_LOCALIZEDTEXT:
                    {
                        UA_LocalizedText str = UA_LOCALIZEDTEXT((char*) "en_US", (char*) value.c_str());
                        UA_Variant_setScalarCopy(var, &str, dataType);
                        return;
                    }
                    default:
                        return;
                }
            }
        }
        throw std::runtime_error("C Exception: unsupported value!");
    }

    DurationTimeStamp GetDurationTimeStamp()
    {
        return std::chrono::steady_clock::now();
    }

    OpcUaObject<UA_ByteString> LoadFile(const std::string& path)
    {
        OpcUaObject<UA_ByteString> fileContentsObj = UA_BYTESTRING_NULL;
        UA_ByteString& fileContents = fileContentsObj.getValue();

        /* Open the file */
        FILE* fp = fopen(path.c_str(), "rb");
        if (!fp)
        {
            errno = 0; /* We read errno also from the tcp layer... */
            throw std::invalid_argument("Can not open file " + path);
        }

        /* Get the file length, allocate the data and read */
        fseek(fp, 0, SEEK_END);
        fileContents.length = (size_t) ftell(fp);
        fileContents.data = (UA_Byte*) UA_malloc(fileContents.length * sizeof(UA_Byte));
        if (fileContents.data)
        {
            fseek(fp, 0, SEEK_SET);
            size_t read = fread(fileContents.data, sizeof(UA_Byte), fileContents.length, fp);
            if (read != fileContents.length)
                UA_ByteString_clear(&fileContents);
        }
        else
        {
            fileContents.length = 0;
        }
        fclose(fp);

        return fileContentsObj;
    }
}

END_NAMESPACE_OPENDAQ_OPCUA
