/*----------------------------------------------------------------------*
 * Copyright(c) 2015 SAP SE. All rights reserved
 * Author      : SAP Custom Development
 * Description : Utility for data read/write between CSV and C++ tuple
 *----------------------------------------------------------------------*
 * Change - History : Change history
 * Developer  Date      Description
 * I078212    20140929  Initial creation
 * I078212    20151022  NJSMARTTRAFFIC-206: multiple CSV files support
 *----------------------------------------------------------------------*/

#ifndef _CSV_TO_TUPLES_H
#define _CSV_TO_TUPLES_H

#include <stdlib.h>
#include <string>
#include <istream>
#include <ostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <memory>
#include "common/common_utils.h"
#include "common/csv_reader.hpp"


namespace util {
namespace csv_tuple {


template <typename T>
class NullType
{
public:
    explicit NullType()
        : is_null_(true) // by default, NULL value
    {}

    explicit NullType(const NullType& src)
        : is_null_(src.is_null_), value_(src.value_)
    {}

    explicit NullType(const T& value)
        : is_null_(false), value_(value)
    {}

    ~NullType()
    {}

    bool IsNull() const
    {
        return this->is_null_;
    }

    const T& Value() const
    {
        return value_;
    }

    void Reset()
    {
        // not all types can be reset by assigning of 0
        *this = NullType();
    }

    NullType& operator=(const NullType& src)
    {
        this->is_null_ = src.is_null;
        this->value_ = src.value;
        return *this;
    }

    NullType& operator=(const T& value)
    {
        this->is_null_ = false;
        this->value_ = value;
        return *this;
    }

protected:
    bool is_null_;
    T value_;
};

typedef NullType<char> NullChar;
typedef NullType<unsigned char> NullUChar;
typedef NullType<short> NullShort;
typedef NullType<unsigned short> NullUShort;
typedef NullType<int> NullInt;
typedef NullType<unsigned int> NullUInt;
typedef NullType<long long> NullBigInt;
typedef NullType<unsigned long long> NullUBigInt;
typedef NullType<float> NullFloat;
typedef NullType<float> NullReal;
typedef NullType<double> NullDouble;

typedef NullType<bool> NullBool;
typedef NullType<std::string> NullString;


template <typename T>
void StringToValue(const std::string& str, NullType<T>& optional_value)
{
    if (str.empty()) {
        optional_value.Reset();
    }
    else {
        optional_value.is_null = false;
        StringToValue(str, optional_value.value);
    }
}

static inline void StringToValue(const std::string &str, char &value)
{
    value = (char)atoi(str.c_str());
}

static inline void StringToValue(const std::string &str, unsigned char &value)
{
    value = (unsigned char)atoi(str.c_str());
}

static inline void StringToValue(const std::string &str, short &value)
{
    value = (short)atoi(str.c_str());
}

static inline void StringToValue(const std::string &str, unsigned short &value)
{
    value = (unsigned short)atoi(str.c_str());
}

static inline void StringToValue(const std::string &str, int &value)
{
    value = atoi(str.c_str());
}

static inline void StringToValue(const std::string &str, unsigned int &value)
{
    value = (unsigned int)std::strtoul(str.c_str(), NULL, 10);
}

static inline void StringToValue(const std::string &str, long long &value)
{
#ifdef _WIN32
    value = _atoi64(str.c_str());
#else
    value = atoll(str.c_str());
#endif
}

static inline void StringToValue(const std::string &str, unsigned long long &value)
{
    value = (unsigned long long)std::strtoull(str.c_str(), NULL, 10);
}

static inline void StringToValue(const std::string &str, bool &value)
{
    value = atoi(str.c_str()) != 0;
}

static inline void StringToValue(const std::string& str, std::string& value)
{
    value = str;
}

static inline void StringToValue(const std::string& str, float& value)
{
    value = (float)atof(str.c_str());
}

static inline void StringToValue(const std::string& str, double& value)
{
    value = atof(str.c_str());
}

// add more versions of StringToValue() if needed


#define TUPLE_SET_ITEM(tp, i) \
    StringToValue(subs[i], std::get<i>(tp))

template<typename T0>
void FillTuple(const std::vector<std::string>& subs,
    std::tuple<T0>& tp)
{
    TUPLE_SET_ITEM(tp, 0);
}

template<typename T0, typename T1>
void FillTuple(const std::vector<std::string>& subs,
    std::tuple<T0, T1>& tp)
{
    TUPLE_SET_ITEM(tp, 0);
    TUPLE_SET_ITEM(tp, 1);
}

template<typename T0, typename T1, typename T2>
void FillTuple(const std::vector<std::string>& subs,
    std::tuple<T0, T1, T2>& tp)
{
    TUPLE_SET_ITEM(tp, 0);
    TUPLE_SET_ITEM(tp, 1);
    TUPLE_SET_ITEM(tp, 2);
}

template<typename T0, typename T1, typename T2, typename T3>
void FillTuple(const std::vector<std::string>& subs,
    std::tuple<T0, T1, T2, T3>& tp)
{
    TUPLE_SET_ITEM(tp, 0);
    TUPLE_SET_ITEM(tp, 1);
    TUPLE_SET_ITEM(tp, 2);
    TUPLE_SET_ITEM(tp, 3);
}

template<typename T0, typename T1, typename T2, typename T3, typename T4>
void FillTuple(const std::vector<std::string>& subs,
    std::tuple<T0, T1, T2, T3, T4>& tp)
{
    TUPLE_SET_ITEM(tp, 0);
    TUPLE_SET_ITEM(tp, 1);
    TUPLE_SET_ITEM(tp, 2);
    TUPLE_SET_ITEM(tp, 3);
    TUPLE_SET_ITEM(tp, 4);
}

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
void FillTuple(const std::vector<std::string>& subs,
    std::tuple<T0, T1, T2, T3, T4, T5>& tp)
{
    TUPLE_SET_ITEM(tp, 0);
    TUPLE_SET_ITEM(tp, 1);
    TUPLE_SET_ITEM(tp, 2);
    TUPLE_SET_ITEM(tp, 3);
    TUPLE_SET_ITEM(tp, 4);
    TUPLE_SET_ITEM(tp, 5);
}

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6>
    void FillTuple(const std::vector<std::string>& subs,
    std::tuple<T0, T1, T2, T3, T4, T5, T6>& tp)
{
    TUPLE_SET_ITEM(tp, 0);
    TUPLE_SET_ITEM(tp, 1);
    TUPLE_SET_ITEM(tp, 2);
    TUPLE_SET_ITEM(tp, 3);
    TUPLE_SET_ITEM(tp, 4);
    TUPLE_SET_ITEM(tp, 5);
    TUPLE_SET_ITEM(tp, 6);
}

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7>
    void FillTuple(const std::vector<std::string>& subs,
    std::tuple<T0, T1, T2, T3, T4, T5, T6, T7>& tp)
{
    TUPLE_SET_ITEM(tp, 0);
    TUPLE_SET_ITEM(tp, 1);
    TUPLE_SET_ITEM(tp, 2);
    TUPLE_SET_ITEM(tp, 3);
    TUPLE_SET_ITEM(tp, 4);
    TUPLE_SET_ITEM(tp, 5);
    TUPLE_SET_ITEM(tp, 6);
    TUPLE_SET_ITEM(tp, 7);
}

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8>
    void FillTuple(const std::vector<std::string>& subs,
    std::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8>& tp)
{
    TUPLE_SET_ITEM(tp, 0);
    TUPLE_SET_ITEM(tp, 1);
    TUPLE_SET_ITEM(tp, 2);
    TUPLE_SET_ITEM(tp, 3);
    TUPLE_SET_ITEM(tp, 4);
    TUPLE_SET_ITEM(tp, 5);
    TUPLE_SET_ITEM(tp, 6);
    TUPLE_SET_ITEM(tp, 7);
    TUPLE_SET_ITEM(tp, 8);
}

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9>
    void FillTuple(const std::vector<std::string>& subs,
    std::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9>& tp)
{
    TUPLE_SET_ITEM(tp, 0);
    TUPLE_SET_ITEM(tp, 1);
    TUPLE_SET_ITEM(tp, 2);
    TUPLE_SET_ITEM(tp, 3);
    TUPLE_SET_ITEM(tp, 4);
    TUPLE_SET_ITEM(tp, 5);
    TUPLE_SET_ITEM(tp, 6);
    TUPLE_SET_ITEM(tp, 7);
    TUPLE_SET_ITEM(tp, 8);
    TUPLE_SET_ITEM(tp, 9);
}

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10>
    void FillTuple(const std::vector<std::string>& subs,
    std::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10>& tp)
{
    TUPLE_SET_ITEM(tp, 0);
    TUPLE_SET_ITEM(tp, 1);
    TUPLE_SET_ITEM(tp, 2);
    TUPLE_SET_ITEM(tp, 3);
    TUPLE_SET_ITEM(tp, 4);
    TUPLE_SET_ITEM(tp, 5);
    TUPLE_SET_ITEM(tp, 6);
    TUPLE_SET_ITEM(tp, 7);
    TUPLE_SET_ITEM(tp, 8);
    TUPLE_SET_ITEM(tp, 9);
    TUPLE_SET_ITEM(tp, 10);
}

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10, typename T11>
    void FillTuple(const std::vector<std::string>& subs,
    std::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11>& tp)
{
    TUPLE_SET_ITEM(tp, 0);
    TUPLE_SET_ITEM(tp, 1);
    TUPLE_SET_ITEM(tp, 2);
    TUPLE_SET_ITEM(tp, 3);
    TUPLE_SET_ITEM(tp, 4);
    TUPLE_SET_ITEM(tp, 5);
    TUPLE_SET_ITEM(tp, 6);
    TUPLE_SET_ITEM(tp, 7);
    TUPLE_SET_ITEM(tp, 8);
    TUPLE_SET_ITEM(tp, 9);
    TUPLE_SET_ITEM(tp, 10);
    TUPLE_SET_ITEM(tp, 11);
}

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10, typename T11,
    typename T12>
    void FillTuple(const std::vector<std::string>& subs,
    std::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12>& tp)
{
    TUPLE_SET_ITEM(tp, 0);
    TUPLE_SET_ITEM(tp, 1);
    TUPLE_SET_ITEM(tp, 2);
    TUPLE_SET_ITEM(tp, 3);
    TUPLE_SET_ITEM(tp, 4);
    TUPLE_SET_ITEM(tp, 5);
    TUPLE_SET_ITEM(tp, 6);
    TUPLE_SET_ITEM(tp, 7);
    TUPLE_SET_ITEM(tp, 8);
    TUPLE_SET_ITEM(tp, 9);
    TUPLE_SET_ITEM(tp, 10);
    TUPLE_SET_ITEM(tp, 11);
    TUPLE_SET_ITEM(tp, 12);
}

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10, typename T11,
    typename T12, typename T13>
    void FillTuple(const std::vector<std::string>& subs,
    std::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13>& tp)
{
    TUPLE_SET_ITEM(tp, 0);
    TUPLE_SET_ITEM(tp, 1);
    TUPLE_SET_ITEM(tp, 2);
    TUPLE_SET_ITEM(tp, 3);
    TUPLE_SET_ITEM(tp, 4);
    TUPLE_SET_ITEM(tp, 5);
    TUPLE_SET_ITEM(tp, 6);
    TUPLE_SET_ITEM(tp, 7);
    TUPLE_SET_ITEM(tp, 8);
    TUPLE_SET_ITEM(tp, 9);
    TUPLE_SET_ITEM(tp, 10);
    TUPLE_SET_ITEM(tp, 11);
    TUPLE_SET_ITEM(tp, 12);
    TUPLE_SET_ITEM(tp, 13);
}

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10, typename T11,
    typename T12, typename T13, typename T14>
    void FillTuple(const std::vector<std::string>& subs,
    std::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14>& tp)
{
    TUPLE_SET_ITEM(tp, 0);
    TUPLE_SET_ITEM(tp, 1);
    TUPLE_SET_ITEM(tp, 2);
    TUPLE_SET_ITEM(tp, 3);
    TUPLE_SET_ITEM(tp, 4);
    TUPLE_SET_ITEM(tp, 5);
    TUPLE_SET_ITEM(tp, 6);
    TUPLE_SET_ITEM(tp, 7);
    TUPLE_SET_ITEM(tp, 8);
    TUPLE_SET_ITEM(tp, 9);
    TUPLE_SET_ITEM(tp, 10);
    TUPLE_SET_ITEM(tp, 11);
    TUPLE_SET_ITEM(tp, 12);
    TUPLE_SET_ITEM(tp, 13);
    TUPLE_SET_ITEM(tp, 14);
}

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10, typename T11,
    typename T12, typename T13, typename T14, typename T15>
    void FillTuple(const std::vector<std::string>& subs,
    std::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15>& tp)
{
    TUPLE_SET_ITEM(tp, 0);
    TUPLE_SET_ITEM(tp, 1);
    TUPLE_SET_ITEM(tp, 2);
    TUPLE_SET_ITEM(tp, 3);
    TUPLE_SET_ITEM(tp, 4);
    TUPLE_SET_ITEM(tp, 5);
    TUPLE_SET_ITEM(tp, 6);
    TUPLE_SET_ITEM(tp, 7);
    TUPLE_SET_ITEM(tp, 8);
    TUPLE_SET_ITEM(tp, 9);
    TUPLE_SET_ITEM(tp, 10);
    TUPLE_SET_ITEM(tp, 11);
    TUPLE_SET_ITEM(tp, 12);
    TUPLE_SET_ITEM(tp, 13);
    TUPLE_SET_ITEM(tp, 14);
    TUPLE_SET_ITEM(tp, 15);
}

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10, typename T11,
    typename T12, typename T13, typename T14, typename T15, typename T16>
    void FillTuple(const std::vector<std::string>& subs,
    std::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16>& tp)
{
    TUPLE_SET_ITEM(tp, 0);
    TUPLE_SET_ITEM(tp, 1);
    TUPLE_SET_ITEM(tp, 2);
    TUPLE_SET_ITEM(tp, 3);
    TUPLE_SET_ITEM(tp, 4);
    TUPLE_SET_ITEM(tp, 5);
    TUPLE_SET_ITEM(tp, 6);
    TUPLE_SET_ITEM(tp, 7);
    TUPLE_SET_ITEM(tp, 8);
    TUPLE_SET_ITEM(tp, 9);
    TUPLE_SET_ITEM(tp, 10);
    TUPLE_SET_ITEM(tp, 11);
    TUPLE_SET_ITEM(tp, 12);
    TUPLE_SET_ITEM(tp, 13);
    TUPLE_SET_ITEM(tp, 14);
    TUPLE_SET_ITEM(tp, 15);
    TUPLE_SET_ITEM(tp, 16);
}

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10, typename T11,
    typename T12, typename T13, typename T14, typename T15, typename T16, typename T17>
    void FillTuple(const std::vector<std::string>& subs,
        std::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17>& tp)
{
    TUPLE_SET_ITEM(tp, 0);
    TUPLE_SET_ITEM(tp, 1);
    TUPLE_SET_ITEM(tp, 2);
    TUPLE_SET_ITEM(tp, 3);
    TUPLE_SET_ITEM(tp, 4);
    TUPLE_SET_ITEM(tp, 5);
    TUPLE_SET_ITEM(tp, 6);
    TUPLE_SET_ITEM(tp, 7);
    TUPLE_SET_ITEM(tp, 8);
    TUPLE_SET_ITEM(tp, 9);
    TUPLE_SET_ITEM(tp, 10);
    TUPLE_SET_ITEM(tp, 11);
    TUPLE_SET_ITEM(tp, 12);
    TUPLE_SET_ITEM(tp, 13);
    TUPLE_SET_ITEM(tp, 14);
    TUPLE_SET_ITEM(tp, 15);
    TUPLE_SET_ITEM(tp, 16);
    TUPLE_SET_ITEM(tp, 17);
}

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10, typename T11,
    typename T12, typename T13, typename T14, typename T15, typename T16, typename T17,
    typename T18>
    void FillTuple(const std::vector<std::string>& subs,
        std::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18>& tp)
{
    TUPLE_SET_ITEM(tp, 0);
    TUPLE_SET_ITEM(tp, 1);
    TUPLE_SET_ITEM(tp, 2);
    TUPLE_SET_ITEM(tp, 3);
    TUPLE_SET_ITEM(tp, 4);
    TUPLE_SET_ITEM(tp, 5);
    TUPLE_SET_ITEM(tp, 6);
    TUPLE_SET_ITEM(tp, 7);
    TUPLE_SET_ITEM(tp, 8);
    TUPLE_SET_ITEM(tp, 9);
    TUPLE_SET_ITEM(tp, 10);
    TUPLE_SET_ITEM(tp, 11);
    TUPLE_SET_ITEM(tp, 12);
    TUPLE_SET_ITEM(tp, 13);
    TUPLE_SET_ITEM(tp, 14);
    TUPLE_SET_ITEM(tp, 15);
    TUPLE_SET_ITEM(tp, 16);
    TUPLE_SET_ITEM(tp, 17);
    TUPLE_SET_ITEM(tp, 18);
}

// add more versions of FillTuple() if needed

#undef TUPLE_SET_ITEM

template <typename T>
std::string ValueToStr(const NullType<T>& optional_value)
{
    if (optional_value.IsNull()) {
        return std::string();
    }
    else {
        return ValueToStr(optional_value.Value());
    }
}

static inline std::string ValueToStr(const char value)
{
    return std::to_string((int)value);
}

static inline std::string ValueToStr(const unsigned char value)
{
    return std::to_string((int)value);
}

static inline std::string ValueToStr(const short value)
{
    return std::to_string(value);
}

static inline std::string ValueToStr(const unsigned short value)
{
    return std::to_string(value);
}

static inline std::string ValueToStr(const int value)
{
    return std::to_string(value);
}

static inline std::string ValueToStr(const unsigned int value)
{
    return std::to_string(value);
}

static inline std::string ValueToStr(const long long value)
{
    return std::to_string(value);
}

static inline std::string ValueToStr(const unsigned long long value)
{
    return std::to_string(value);
}

static inline std::string ValueToStr(const bool value)
{
    return value ? "1" : "0";
}

static inline std::string ValueToStr(const std::string& value)
{
    if (value.empty()) {
        return value;
    }
    else if (value.find_first_of(" \t") == std::string::npos) {
        return value;
    }
    else {
        std::string ret("\"");
        ret += value;
        ret += '\"';
        return ret;
    }
}

static inline std::string ValueToStr(const float value)
{
    return std::to_string(value);
}

static inline std::string ValueToStr(const double value)
{
    return std::to_string(value);
}


template<typename T0>
void FillLine(const std::tuple<T0>& tp, char /*delimiter*/, std::string& line)
{
    line = ValueToStr(std::get<0>(tp));
}

template<typename T0, typename T1>
void FillLine(const std::tuple<T0, T1>& tp, char delimiter, std::string& line)
{
    line = ValueToStr(std::get<0>(tp));
    line += delimiter;
    line += ValueToStr(std::get<1>(tp));
}

template<typename T0, typename T1, typename T2>
void FillLine(const std::tuple<T0, T1, T2>& tp, char delimiter, std::string& line)
{
    line = ValueToStr(std::get<0>(tp));
    line += delimiter;
    line += ValueToStr(std::get<1>(tp));
    line += delimiter;
    line += ValueToStr(std::get<2>(tp));
}

template<typename T0, typename T1, typename T2, typename T3>
void FillLine(const std::tuple<T0, T1, T2, T3>& tp, char delimiter, std::string& line)
{
    line = ValueToStr(std::get<0>(tp));
    line += delimiter;
    line += ValueToStr(std::get<1>(tp));
    line += delimiter;
    line += ValueToStr(std::get<2>(tp));
    line += delimiter;
    line += ValueToStr(std::get<3>(tp));
}

template<typename T0, typename T1, typename T2, typename T3, typename T4>
void FillLine(const std::tuple<T0, T1, T2, T3, T4>& tp, char delimiter, std::string& line)
{
    line = ValueToStr(std::get<0>(tp));
    line += delimiter;
    line += ValueToStr(std::get<1>(tp));
    line += delimiter;
    line += ValueToStr(std::get<2>(tp));
    line += delimiter;
    line += ValueToStr(std::get<3>(tp));
    line += delimiter;
    line += ValueToStr(std::get<4>(tp));
}

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
void FillLine(const std::tuple<T0, T1, T2, T3, T4, T5>& tp, char delimiter, std::string& line)
{
    line = ValueToStr(std::get<0>(tp));
    line += delimiter;
    line += ValueToStr(std::get<1>(tp));
    line += delimiter;
    line += ValueToStr(std::get<2>(tp));
    line += delimiter;
    line += ValueToStr(std::get<3>(tp));
    line += delimiter;
    line += ValueToStr(std::get<4>(tp));
    line += delimiter;
    line += ValueToStr(std::get<5>(tp));
}

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
void FillLine(const std::tuple<T0, T1, T2, T3, T4, T5, T6>& tp, char delimiter, std::string& line)
{
    line = ValueToStr(std::get<0>(tp));
    line += delimiter;
    line += ValueToStr(std::get<1>(tp));
    line += delimiter;
    line += ValueToStr(std::get<2>(tp));
    line += delimiter;
    line += ValueToStr(std::get<3>(tp));
    line += delimiter;
    line += ValueToStr(std::get<4>(tp));
    line += delimiter;
    line += ValueToStr(std::get<5>(tp));
    line += delimiter;
    line += ValueToStr(std::get<6>(tp));
}

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6,
    typename T7>
    void FillLine(const std::tuple<T0, T1, T2, T3, T4, T5, T6, T7>& tp, char delimiter, std::string& line)
{
    line = ValueToStr(std::get<0>(tp));
    line += delimiter;
    line += ValueToStr(std::get<1>(tp));
    line += delimiter;
    line += ValueToStr(std::get<2>(tp));
    line += delimiter;
    line += ValueToStr(std::get<3>(tp));
    line += delimiter;
    line += ValueToStr(std::get<4>(tp));
    line += delimiter;
    line += ValueToStr(std::get<5>(tp));
    line += delimiter;
    line += ValueToStr(std::get<6>(tp));
    line += delimiter;
    line += ValueToStr(std::get<7>(tp));
}

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6,
    typename T7, typename T8>
    void FillLine(const std::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8>& tp,
    char delimiter, std::string& line)
{
    line = ValueToStr(std::get<0>(tp));
    line += delimiter;
    line += ValueToStr(std::get<1>(tp));
    line += delimiter;
    line += ValueToStr(std::get<2>(tp));
    line += delimiter;
    line += ValueToStr(std::get<3>(tp));
    line += delimiter;
    line += ValueToStr(std::get<4>(tp));
    line += delimiter;
    line += ValueToStr(std::get<5>(tp));
    line += delimiter;
    line += ValueToStr(std::get<6>(tp));
    line += delimiter;
    line += ValueToStr(std::get<7>(tp));
    line += delimiter;
    line += ValueToStr(std::get<8>(tp));
}

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6,
    typename T7, typename T8, typename T9>
    void FillLine(const std::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9>& tp,
    char delimiter, std::string& line)
{
    line = ValueToStr(std::get<0>(tp));
    line += delimiter;
    line += ValueToStr(std::get<1>(tp));
    line += delimiter;
    line += ValueToStr(std::get<2>(tp));
    line += delimiter;
    line += ValueToStr(std::get<3>(tp));
    line += delimiter;
    line += ValueToStr(std::get<4>(tp));
    line += delimiter;
    line += ValueToStr(std::get<5>(tp));
    line += delimiter;
    line += ValueToStr(std::get<6>(tp));
    line += delimiter;
    line += ValueToStr(std::get<7>(tp));
    line += delimiter;
    line += ValueToStr(std::get<8>(tp));
    line += delimiter;
    line += ValueToStr(std::get<9>(tp));
}

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6,
    typename T7, typename T8, typename T9, typename T10>
    void FillLine(const std::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10>& tp,
    char delimiter, std::string& line)
{
    line = ValueToStr(std::get<0>(tp));
    line += delimiter;
    line += ValueToStr(std::get<1>(tp));
    line += delimiter;
    line += ValueToStr(std::get<2>(tp));
    line += delimiter;
    line += ValueToStr(std::get<3>(tp));
    line += delimiter;
    line += ValueToStr(std::get<4>(tp));
    line += delimiter;
    line += ValueToStr(std::get<5>(tp));
    line += delimiter;
    line += ValueToStr(std::get<6>(tp));
    line += delimiter;
    line += ValueToStr(std::get<7>(tp));
    line += delimiter;
    line += ValueToStr(std::get<8>(tp));
    line += delimiter;
    line += ValueToStr(std::get<9>(tp));
    line += delimiter;
    line += ValueToStr(std::get<10>(tp));
}

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6,
    typename T7, typename T8, typename T9, typename T10, typename T11>
    void FillLine(const std::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11>& tp,
    char delimiter, std::string& line)
{
    line = ValueToStr(std::get<0>(tp));
    line += delimiter;
    line += ValueToStr(std::get<1>(tp));
    line += delimiter;
    line += ValueToStr(std::get<2>(tp));
    line += delimiter;
    line += ValueToStr(std::get<3>(tp));
    line += delimiter;
    line += ValueToStr(std::get<4>(tp));
    line += delimiter;
    line += ValueToStr(std::get<5>(tp));
    line += delimiter;
    line += ValueToStr(std::get<6>(tp));
    line += delimiter;
    line += ValueToStr(std::get<7>(tp));
    line += delimiter;
    line += ValueToStr(std::get<8>(tp));
    line += delimiter;
    line += ValueToStr(std::get<9>(tp));
    line += delimiter;
    line += ValueToStr(std::get<10>(tp));
    line += delimiter;
    line += ValueToStr(std::get<11>(tp));
}

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6,
    typename T7, typename T8, typename T9, typename T10, typename T11, typename T12>
    void FillLine(const std::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12>& tp,
    char delimiter, std::string& line)
{
    line = ValueToStr(std::get<0>(tp));
    line += delimiter;
    line += ValueToStr(std::get<1>(tp));
    line += delimiter;
    line += ValueToStr(std::get<2>(tp));
    line += delimiter;
    line += ValueToStr(std::get<3>(tp));
    line += delimiter;
    line += ValueToStr(std::get<4>(tp));
    line += delimiter;
    line += ValueToStr(std::get<5>(tp));
    line += delimiter;
    line += ValueToStr(std::get<6>(tp));
    line += delimiter;
    line += ValueToStr(std::get<7>(tp));
    line += delimiter;
    line += ValueToStr(std::get<8>(tp));
    line += delimiter;
    line += ValueToStr(std::get<9>(tp));
    line += delimiter;
    line += ValueToStr(std::get<10>(tp));
    line += delimiter;
    line += ValueToStr(std::get<11>(tp));
    line += delimiter;
    line += ValueToStr(std::get<12>(tp));
}

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6,
    typename T7, typename T8, typename T9, typename T10, typename T11, typename T12, typename T13>
    void FillLine(const std::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13>& tp,
    char delimiter, std::string& line)
{
    line = ValueToStr(std::get<0>(tp));
    line += delimiter;
    line += ValueToStr(std::get<1>(tp));
    line += delimiter;
    line += ValueToStr(std::get<2>(tp));
    line += delimiter;
    line += ValueToStr(std::get<3>(tp));
    line += delimiter;
    line += ValueToStr(std::get<4>(tp));
    line += delimiter;
    line += ValueToStr(std::get<5>(tp));
    line += delimiter;
    line += ValueToStr(std::get<6>(tp));
    line += delimiter;
    line += ValueToStr(std::get<7>(tp));
    line += delimiter;
    line += ValueToStr(std::get<8>(tp));
    line += delimiter;
    line += ValueToStr(std::get<9>(tp));
    line += delimiter;
    line += ValueToStr(std::get<10>(tp));
    line += delimiter;
    line += ValueToStr(std::get<11>(tp));
    line += delimiter;
    line += ValueToStr(std::get<12>(tp));
    line += delimiter;
    line += ValueToStr(std::get<13>(tp));
}

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6,
    typename T7, typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename T14>
    void FillLine(const std::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14>& tp,
    char delimiter, std::string& line)
{
    line = ValueToStr(std::get<0>(tp));
    line += delimiter;
    line += ValueToStr(std::get<1>(tp));
    line += delimiter;
    line += ValueToStr(std::get<2>(tp));
    line += delimiter;
    line += ValueToStr(std::get<3>(tp));
    line += delimiter;
    line += ValueToStr(std::get<4>(tp));
    line += delimiter;
    line += ValueToStr(std::get<5>(tp));
    line += delimiter;
    line += ValueToStr(std::get<6>(tp));
    line += delimiter;
    line += ValueToStr(std::get<7>(tp));
    line += delimiter;
    line += ValueToStr(std::get<8>(tp));
    line += delimiter;
    line += ValueToStr(std::get<9>(tp));
    line += delimiter;
    line += ValueToStr(std::get<10>(tp));
    line += delimiter;
    line += ValueToStr(std::get<11>(tp));
    line += delimiter;
    line += ValueToStr(std::get<12>(tp));
    line += delimiter;
    line += ValueToStr(std::get<13>(tp));
    line += delimiter;
    line += ValueToStr(std::get<14>(tp));
}

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
    typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename T14, typename T15>
    void FillLine(const std::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15>& tp,
    char delimiter, std::string& line)
{
    line = ValueToStr(std::get<0>(tp));
    line += delimiter;
    line += ValueToStr(std::get<1>(tp));
    line += delimiter;
    line += ValueToStr(std::get<2>(tp));
    line += delimiter;
    line += ValueToStr(std::get<3>(tp));
    line += delimiter;
    line += ValueToStr(std::get<4>(tp));
    line += delimiter;
    line += ValueToStr(std::get<5>(tp));
    line += delimiter;
    line += ValueToStr(std::get<6>(tp));
    line += delimiter;
    line += ValueToStr(std::get<7>(tp));
    line += delimiter;
    line += ValueToStr(std::get<8>(tp));
    line += delimiter;
    line += ValueToStr(std::get<9>(tp));
    line += delimiter;
    line += ValueToStr(std::get<10>(tp));
    line += delimiter;
    line += ValueToStr(std::get<11>(tp));
    line += delimiter;
    line += ValueToStr(std::get<12>(tp));
    line += delimiter;
    line += ValueToStr(std::get<13>(tp));
    line += delimiter;
    line += ValueToStr(std::get<14>(tp));
    line += delimiter;
    line += ValueToStr(std::get<15>(tp));
}

template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6,
    typename T7, typename T8, typename T9, typename T10, typename T11, typename T12, typename T13,
    typename T14, typename T15, typename T16>
    void FillLine(const std::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14, T15, T16>& tp,
    char delimiter, std::string& line)
{
    line = ValueToStr(std::get<0>(tp));
    line += delimiter;
    line += ValueToStr(std::get<1>(tp));
    line += delimiter;
    line += ValueToStr(std::get<2>(tp));
    line += delimiter;
    line += ValueToStr(std::get<3>(tp));
    line += delimiter;
    line += ValueToStr(std::get<4>(tp));
    line += delimiter;
    line += ValueToStr(std::get<5>(tp));
    line += delimiter;
    line += ValueToStr(std::get<6>(tp));
    line += delimiter;
    line += ValueToStr(std::get<7>(tp));
    line += delimiter;
    line += ValueToStr(std::get<8>(tp));
    line += delimiter;
    line += ValueToStr(std::get<9>(tp));
    line += delimiter;
    line += ValueToStr(std::get<10>(tp));
    line += delimiter;
    line += ValueToStr(std::get<11>(tp));
    line += delimiter;
    line += ValueToStr(std::get<12>(tp));
    line += delimiter;
    line += ValueToStr(std::get<13>(tp));
    line += delimiter;
    line += ValueToStr(std::get<14>(tp));
    line += delimiter;
    line += ValueToStr(std::get<15>(tp));
    line += delimiter;
    line += ValueToStr(std::get<16>(tp));
}

}

template<typename... Args>
bool CsvToTuplesLimited(std::istream& in, char delimiter, std::vector<std::tuple<Args...> >& tuples,
    size_t limit, std::string& err)
{
    const size_t N = sizeof...(Args);
    std::string line;
    std::vector<std::string> subs;

    size_t n_lines = 0;
    std::tuple<Args...> tp;
    while (util::GetLine(in, line)) {
        util::TrimString(line);
        if (line.empty() /*|| line[0] == '#'*/) { // ignore comment
            continue;
        }

        util::ParseCsvLine(subs, line, delimiter);
        if (subs.size() < N) {
            std::ostringstream oss;
            oss << "Error: unexpected column size. Expected: " << N << ", Actual: " << subs.size()
                << ", line data : " << line;
            err = oss.str();
            return false;
        }

        csv_tuple::FillTuple(subs, tp);
        tuples.push_back(tp);

        if (limit != 0) {
            ++n_lines;
            if (n_lines >= limit) {
                break;
            }
        }
    }

    return true;
}

template<typename... Args>
bool CsvToTuples(std::istream& in, char delimiter, std::vector<std::tuple<Args...> >& tuples,
    std::string& err)
{
    return CsvToTuplesLimited(in, delimiter, tuples, 0, err);
}


template<typename... Args>
bool CsvToTuples(const std::string& csv_pathname, char delimiter,
    std::vector<std::tuple<Args...> >& tuples, std::string& err)
{
    std::shared_ptr<util::csv_reader> cr(util::get_csv_reader(csv_pathname));
    if (cr == nullptr || !cr->good()) {
        err = "CsvToTuples: error in opening file \"" + csv_pathname + "\"";
        return false;
    }

    return CsvToTuples(cr->get_istream(), delimiter, tuples, err);
}

template<typename... Args>
bool CsvToTuplesSelectedCols(std::istream& in, char delimiter,
    const std::vector<int>& selected_cols, std::vector<std::tuple<Args...>>& tuples,
    std::string& err)
{
    const size_t N = sizeof...(Args);
    if (selected_cols.size() != N) {
        std::ostringstream oss;
        oss << "Error: unexpected selected column size. Expected: " << N
            << ", Actual: " << selected_cols.size();
        err = oss.str();
        return false;
    }

    std::string line;
    std::vector<std::string> subs, sel_subs;
    sel_subs.resize(selected_cols.size());

    std::tuple<Args...> tp;
    while (util::GetLine(in, line)) {
        util::TrimString(line);
        if (line.empty() /*|| line[0] == '#'*/) { // ignore comment
            continue;
        }

        util::ParseCsvLine(subs, line, delimiter);
        auto it_sel = sel_subs.begin();
        for (auto& i_sel : selected_cols) {
            *(it_sel++) = subs[i_sel];
        }

        csv_tuple::FillTuple(sel_subs, tp);
        tuples.push_back(tp);
    }

    return true;
}

template<typename... Args>
bool CsvToTuplesSelectedCols(const std::string& csv_pathname, char delimiter,
    const std::vector<int>& selected_cols, std::vector<std::tuple<Args...> >& tuples,
    std::string& err)
{
    std::shared_ptr<csv_reader> cr(util::get_csv_reader(csv_pathname));
    if (cr == nullptr || !cr->good()) {
        err = "CsvToTuplesSelectedCols: error in opening file \"" + csv_pathname + "\"";
        return false;
    }

    return CsvToTuplesSelectedCols(cr->get_istream(), delimiter, selected_cols, tuples, err);
}

template<typename... Args>
bool TuplesToCsv(const std::string& head, std::vector<std::tuple<Args...>>& tuples,
    std::ostream& out, char delimiter, std::string& err)
{
    if (!head.empty()) {
        out << head << std::endl;
    }

    const size_t buff_limit = 1024 * 8;
    std::string line;
    std::string buff;
    buff.reserve(buff_limit + 256);

    for (auto& t : tuples) {
        csv_tuple::FillLine(t, delimiter, line);
        buff += line;
        buff += '\n';
        if (buff.length() > buff_limit) {
            out << buff;
            buff.clear();
        }
    }
    if (!buff.empty()) {
        out << buff;
    }

    return true;
}

template<typename... Args>
bool TuplesToCsv(const std::string& head, std::vector<std::tuple<Args...> >& tuples,
    const std::string& csv_pathname, char delimiter, std::string& err)
{
    std::ofstream out(csv_pathname.c_str(), std::ios_base::binary | std::ios_base::out);
    if (!out.good()) {
        err = "Error in opening file \"" + csv_pathname + "\"";
        return false;
    }

    return TuplesToCsv(head, tuples, out, delimiter, err);
}


// pred: unary function to return the index of the file, into which the line is saved
//       signature: int pred(const std::tuple<Args...> &t);
//       if return value is negative, ignore the tuple
template<typename... Args, typename Pred>
bool TuplesToCsvs(const std::string &head, std::vector<std::tuple<Args...>> &tuples,
    const std::vector<std::string> &csv_pathnames, char delimiter, Pred pred,
    std::string &err)
{
    if (csv_pathnames.empty()) {
        err = "CSV pathnames are empty";
        return false;
    }
    else if (csv_pathnames.size() == 1) {
        return TuplesToCsv(head, tuples, csv_pathnames.front(), delimiter, err);
    }

    std::vector<std::shared_ptr<std::ofstream>> outs;
    for (auto& csv_pathname : csv_pathnames) {
        auto out = std::make_shared<std::ofstream>(csv_pathname.c_str(),
            std::ios_base::binary | std::ios_base::out);
        if (!out->good()) {
            err = "Error in opening file \"" + csv_pathname + "\"";
            return false;
        }
        if (!head.empty()) {
            *out << head << std::endl;
        }
        outs.emplace_back(out);
    }


    const size_t buff_limit = 1024 * 8;
    std::vector<std::string> lines;
    std::vector<std::string> buffs;
    lines.resize(csv_pathnames.size());
    buffs.resize(csv_pathnames.size());
    for (auto &buff : buffs) {
        buff.reserve(buff_limit + 256);
    }

    for (auto &t : tuples) {
        int i = pred(t);
        if (i < 0) {
            continue;
        }
        auto &line = lines[i];
        auto &buff = buffs[i];

        csv_tuple::FillLine(t, delimiter, line);
        buff += line;
        buff += '\n';
        if (buff.length() > buff_limit) {
            *outs[i] << buff;
            buff.clear();
        }
    }

    for (size_t i = 0; i < outs.size(); ++i) {
        if (!buffs[i].empty()) {
            *outs[i] << buffs[i];
        }
    }

    return true;
}

}

#endif //_CSV_TO_TUPLES_H
