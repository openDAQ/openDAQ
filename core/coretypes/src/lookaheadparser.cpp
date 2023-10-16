#include "lookaheadparser.h"

namespace rapidjson
{
bool LookaheadParser::EnterObject()
{
    if (state != enteringObject)
    {
        state = error;
        return false;
    }

    ParseNext();
    return true;
}

bool LookaheadParser::EnterArray()
{
    if (state != enteringArray)
    {
        state = error;
        return false;
    }

    ParseNext();
    return true;
}

const char* LookaheadParser::NextObjectKey()
{
    if (state == hasKey)
    {
        const char* result = value.GetString();
        ParseNext();
        return result;
    }

    if (state != exitingObject)
    {
        state = error;
        return nullptr;
    }

    ParseNext();
    return nullptr;
}

bool LookaheadParser::NextArrayValue()
{
    if (state == exitingArray)
    {
        ParseNext();
        return false;
    }

    if (state == error || state == exitingObject || state == hasKey)
    {
        state = error;
        return false;
    }

    return true;
}

int LookaheadParser::GetInt()
{
    if (state != hasNumber || !value.IsInt())
    {
        state = error;
        return 0;
    }

    int result = value.GetInt();
    ParseNext();
    return result;
}

double LookaheadParser::GetDouble()
{
    if (state != hasNumber)
    {
        state = error;
        return 0.;
    }

    double result = value.GetDouble();
    ParseNext();
    return result;
}

bool LookaheadParser::GetBool()
{
    if (state != hasBool)
    {
        state = error;
        return false;
    }

    bool result = value.GetBool();
    ParseNext();
    return result;
}

void LookaheadParser::GetNull()
{
    if (state != hasNull)
    {
        state = error;
        return;
    }

    ParseNext();
}

const char* LookaheadParser::GetString()
{
    if (state != hasString)
    {
        state = error;
        return nullptr;
    }

    const char* result = value.GetString();
    ParseNext();
    return result;
}

void LookaheadParser::SkipOut(int depth)
{
    do
    {
        if (state == enteringArray || state == enteringObject)
        {
            ++depth;
        }
        else if (state == exitingArray || state == exitingObject)
        {
            --depth;
        }
        else if (state == error)
        {
            return;
        }

        ParseNext();
    }
    while (depth > 0);
}

void LookaheadParser::SkipValue()
{
    SkipOut(0);
}

void LookaheadParser::SkipArray()
{
    SkipOut(1);
}

void LookaheadParser::SkipObject()
{
    SkipOut(1);
}

Value* LookaheadParser::PeekValue()
{
    if (state >= hasNull && state <= hasKey)
    {
        return &value;
    }

    return nullptr;
}

int LookaheadParser::PeekType() const
{
    if (state >= hasNull && state <= hasKey)
    {
        return value.GetType();
    }

    if (state == enteringArray)
    {
        return kArrayType;
    }

    if (state == enteringObject)
    {
        return kObjectType;
    }

    return -1;
}

bool LookaheadParser::IsValid() const
{
    return state != error;
}
}
