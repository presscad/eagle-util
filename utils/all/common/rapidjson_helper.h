/*----------------------------------------------------------------------*
 * Copyright(c) 2015 SAP SE. All rights reserved
 * Author      : SAP Custom Development
 * Description : Utility for auto clean-up at scope exit
 *----------------------------------------------------------------------*
 * Change - History : Change history
 * Developer  Date      Description
 * I078212    20160425  Initial creation
 *----------------------------------------------------------------------*/

#ifndef _RAPIDJSON_HELPER_H_
#define _RAPIDJSON_HELPER_H_

#include <string>
#include <rapidjson/rapidjson.h>

#define RPAIDJSON_UTIL_BEGIN_NAMESPACE namespace util {
#define RPAIDJSON_UTIL_END_NAMESPACE }

RPAIDJSON_UTIL_BEGIN_NAMESPACE

// WritePairNull
template <class Writer>
bool WritePairNull(Writer& writer, const char *key)
{
    return writer.Key(key) && writer.Null();
}

template <class Writer>
bool WritePairNull(Writer& writer, const std::string& key)
{
    return writer.Key(key.c_str(), (rapidjson::SizeType)key.size()) &&
        writer.Null();
}

// WritePairBool
template <class Writer>
bool WritePairBool(Writer& writer, const char *key, bool value)
{
    return writer.Key(key) && writer.Bool(value);
}

template <class Writer>
bool WritePairBool(Writer& writer, const std::string& key, bool value)
{
    return writer.Key(key.c_str(), (rapidjson::SizeType)key.size()) &&
        writer.Bool(value);
}

// WritePairInt
template <class Writer>
bool WritePairInt64(Writer& writer, const char* key, int value)
{
    return writer.Key(key) && writer.Int(value);
}

template <class Writer>
bool WritePairInt64(Writer& writer, const std::string& key, int value)
{
    return writer.Key(key.c_str(), (rapidjson::SizeType)key.size()) &&
        writer.Int(value);
}

// WritePairUint
template <class Writer>
bool WritePairInt64(Writer& writer, const char* key, unsigned value)
{
    return writer.Key(key) && writer.Uint(value);
}

template <class Writer>
bool WritePairInt64(Writer& writer, const std::string& key, unsigned value)
{
    return writer.Key(key.c_str(), (rapidjson::SizeType)key.size()) &&
        writer.Uint(value);
}

// WritePairInt64
template <class Writer>
bool WritePairInt64(Writer& writer, const char* key, int64_t value)
{
    return writer.Key(key) && writer.Int64(value);
}

template <class Writer>
bool WritePairInt64(Writer& writer, const std::string& key, int64_t value)
{
    return writer.Key(key.c_str(), (rapidjson::SizeType)key.size()) &&
        writer.Int64(value);
}

// WritePairUint64
template <class Writer>
bool WritePairUint64(Writer& writer, const char* key, uint64_t value)
{
    return writer.Key(key) && writer.Uint64(value);
}

template <class Writer>
bool WritePairUint64(Writer& writer, const std::string& key, uint64_t value)
{
    return writer.Key(key.c_str(), (rapidjson::SizeType)key.size()) &&
        writer.Uint64(value);
}

// WritePairDouble
template <class Writer>
bool WritePairDouble(Writer& writer, const char* key, double value)
{
    return writer.Key(key) && writer.Double(value);
}

template <class Writer>
bool WritePairDouble(Writer& writer, const std::string& key, double value)
{
    return writer.Key(key.c_str(), (rapidjson::SizeType)key.size()) &&
        writer.Double(value);
}

// WritePairString
template <class Writer>
bool WritePairString(Writer& writer, const char* key, const char* value)
{
    return writer.Key(key) && writer.String(value);
}

template <class Writer>
bool WritePairString(Writer& writer, const std::string& key, const char* value)
{
    return writer.Key(key.c_str(), (rapidjson::SizeType)key.size()) &&
        writer.String(value);
}

template <class Writer>
bool WritePairString(Writer& writer, const char* key, const std::string& value)
{
    return writer.Key(key) &&
        writer.String(value.c_str(), (rapidjson::SizeType)value.size());
}

template <class Writer>
bool WritePairString(Writer& writer, const std::string& key, const std::string& value)
{
    return writer.Key(key.c_str(), (rapidjson::SizeType)key.size()) &&
        writer.String(value.c_str(), (rapidjson::SizeType)value.size());
}

RPAIDJSON_UTIL_END_NAMESPACE

#endif // _RAPIDJSON_HELPER_H_
