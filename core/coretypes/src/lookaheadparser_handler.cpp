#include "lookaheadparser_handler.h"

namespace rapidjson
{

inline LookaheadParserHandler::LookaheadParserHandler(char* str) : value()
                                                                 , reader()
                                                                 , stream(str)
                                                                 , state(init)
{
    reader.IterativeParseInit();
    ParseNext();
}

bool LookaheadParserHandler::Null()
{
    state = hasNull;
    value.SetNull();
    return true;
}

bool LookaheadParserHandler::Bool(bool b)
{
    state = hasBool;
    value.SetBool(b);
    return true;
}

bool LookaheadParserHandler::Int(int i)
{
    state = hasNumber;
    value.SetInt(i);
    return true;
}

bool LookaheadParserHandler::Uint(unsigned u)
{
    state = hasNumber;
    value.SetUint(u);
    return true;
}

bool LookaheadParserHandler::Int64(int64_t i)
{
    state = hasNumber;
    value.SetInt64(i);
    return true;
}

bool LookaheadParserHandler::Uint64(uint64_t u)
{
    state = hasNumber;
    value.SetUint64(u);
    return true;
}

bool LookaheadParserHandler::Double(double d)
{
    state = hasNumber;
    value.SetDouble(d);
    return true;
}

bool LookaheadParserHandler::RawNumber(const char*, SizeType, bool)
{
    return false;
}

bool LookaheadParserHandler::String(const char* str, SizeType length, bool)
{
    state = hasString;
    value.SetString(str, length);
    return true;
}

bool LookaheadParserHandler::StartObject()
{
    state = enteringObject;
    return true;
}

bool LookaheadParserHandler::Key(const char* str, SizeType length, bool)
{
    state = hasKey;
    value.SetString(str, length);
    return true;
}

bool LookaheadParserHandler::EndObject(SizeType)
{
    state = exitingObject;
    return true;
}

bool LookaheadParserHandler::StartArray()
{
    state = enteringArray;
    return true;
}

bool LookaheadParserHandler::EndArray(SizeType)
{
    state = exitingArray;
    return true;
}

inline void LookaheadParserHandler::ParseNext()
{
    if (reader.HasParseError())
    {
        state = error;
        return;
    }

    reader.IterativeParseNext<parseFlags>(stream, *this);
}

}
