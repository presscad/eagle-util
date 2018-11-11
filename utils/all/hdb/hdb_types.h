/*----------------------------------------------------------------------*
 * Copyright(c) 2015 SAP SE. All rights reserved
 * Description : HDB utility related to data types
 *----------------------------------------------------------------------*
 * Change - History : Change history
 * Developer  Date      Description
 * I078212    20140806  Initial creation
 *----------------------------------------------------------------------*/

#ifndef _HDB_TYPES_H
#define _HDB_TYPES_H

#include <cstdlib>

#ifdef PROJ_SIM_AFL
#   define HDB_BEGIN_NAMESPACE namespace sim_afl { namespace hdb {
#   define HDB_END_NAMESPACE }}
#   define USING_HDB_NAMESPACE using namespace sim_afl::hdb;
#else
#   define HDB_BEGIN_NAMESPACE namespace hdb {
#   define HDB_END_NAMESPACE }
#   define USING_HDB_NAMESPACE using namespace hdb;
#endif

HDB_BEGIN_NAMESPACE

enum DATA_TYPE_T {
    T_UNKNOWN = 0,
    T_TINYINT = 1,
    T_SMALLINT,
    T_INTEGER,
    T_BIGINT,
    T_REAL,
    T_DOUBLE,
    T_FLOAT,
    T_DATE,
    T_TIME,
    T_TIMESTAMP,
    T_SECONDDATE,
    T_CHAR,     // CHAR(a)
    T_NCHAR,    // NCHAR(a)
    T_VARCHAR,  // VARCHAR(a)
    T_NVARCHAR, // NVARCHAR(a)
    T_ALPHANUM, // ALPHANUM(a)
    T_SMALLDECIMAL,
    T_DECIMAL,
    T_DECIMAL_PS,  // DECIMAL(p,s)
    T_BINARY,
    T_VARBINARY,
    T_BLOB,
    T_CLOB,
    T_NCLOB,
    T_TEXT,
    T_ST_GEOMETRY,
    T_MAX = T_ST_GEOMETRY
};

struct DATA_ATTR_T {
    DATA_TYPE_T type{};
    int a{}; // for CHAR(a), VARCHAR(a), NCHAR(a), etc.
    int p{}, s{}; // for DECIMAL(p,s)
    bool null_able{};
};


void UnImplemented(const char *desc);
DATA_ATTR_T GenDataAttr(DATA_TYPE_T type, bool null_able, int p, int s);
const char *DataTypeToStr(DATA_TYPE_T type);
DATA_TYPE_T StrToDataType(const char *type_str);

static inline bool StrToValue(const std::string &s, std::string& v)
{
    v = s;
    return true;
}

static inline bool StrToValue(const std::string &s, char &v)
{
    if (s.empty()) {
        return false;
    }
    v = static_cast<char>(atoi(s.c_str()));
    return true;
}

static inline bool StrToValue(const char *s, char &v)
{
    if (s == nullptr) {
        return false;
    }
    v = static_cast<char>(atoi(s));
    return true;
}

static inline void ValueToStr(char v, std::string& str)
{
    char buff[8];
    snprintf(buff, sizeof(buff)-1, "%d", static_cast<int>(v));
    str = buff;
}

// used by TINYINT
static inline bool StrToValue(const std::string &s, unsigned char &v)
{
    if (s.empty()) {
        return false;
    }
    v = static_cast<unsigned char>(atoi(s.c_str()));
    return true;
};

static inline bool StrToValue(const char *s, unsigned char &v)
{
    if (s == nullptr) {
        return false;
    }
    v = static_cast<unsigned char>(atoi(s));
    return true;
};

static inline void ValueToStr(unsigned char &v, std::string& str)
{
    char buff[8];
    snprintf(buff, sizeof(buff)-1, "%d", static_cast<int>(v));
    str = buff;
};

static inline bool StrToValue(const std::string &s, short &v)
{
    if (s.empty()) {
        return false;
    }
    v = static_cast<short>(atoi(s.c_str()));
    return true;
}

static inline bool StrToValue(const char *s, short &v)
{
    if (s == nullptr) {
        return false;
    }
    v = static_cast<short>(atoi(s));
    return true;
}

static inline void ValueToStr(short v, std::string& str)
{
    char buff[8];
    snprintf(buff, sizeof(buff)-1, "%d", static_cast<int>(v));
    str = buff;
}

bool StrToValue(const std::string &s, int &v);
bool StrToValue(const char *s, int &v);

static inline void ValueToStr(int v, std::string& str)
{
    char buff[24];
    snprintf(buff, sizeof(buff)-1, "%d", v);
    str = buff;
}

bool StrToValue(const std::string &s, SQLBIGINT &v);
bool StrToValue(const char *s, SQLBIGINT &v);

static inline void ValueToStr(SQLBIGINT v, std::string& str)
{
    char buff[32];
    snprintf(buff, sizeof(buff)-1, "%lld", static_cast<long long>(v));
    str = buff;
}

static inline bool StrToValue(const std::string &s, float &v)
{
    if (s.empty()) {
        return false;
    }
    v = static_cast<float>(atof(s.c_str()));
    return true;
}

static inline bool StrToValue(const char *s, float &v)
{
    if (s == nullptr) {
        return false;
    }
    v = static_cast<float>(atof(s));
    return true;
}

static inline void ValueToStr(float v, std::string& str)
{
    char buff[32];
    snprintf(buff, sizeof(buff)-1, "%f", v);
    str = buff;
}

static inline bool StrToValue(const std::string &s, double &v)
{
    if (s.empty()) {
        return false;
    }
    v = atof(s.c_str());
    return true;
}

static inline bool StrToValue(const char *s, double &v)
{
    if (s == nullptr) {
        return false;
    }
    v = atof(s);
    return true;
}

static inline void ValueToStr(double v, std::string& str)
{
    char buff[32];
    snprintf(buff, sizeof(buff)-1, "%lf", v);
    str = buff;
}

static inline bool StrToValue(const std::string &s, SQLWCHAR &v)
{
    UnImplemented(__FUNCTION__);
    if (!s.empty()) {
        v = static_cast<SQLWCHAR>(atoi(s.c_str()));
        return true;
    }
    return false;
};

static inline bool StrToValue(const char *s, SQLWCHAR &v)
{
    UnImplemented(__FUNCTION__);
    if (s != nullptr) {
        v = static_cast<SQLWCHAR>(atoi(s));
        return true;
    }
    return false;
};

static inline void ValueToStr(SQLWCHAR &v, std::string& str)
{
    UnImplemented(__FUNCTION__);
    str = std::string(1, static_cast<char>(v));
};

bool StrToValue(const std::string &s, SQL_DATE_STRUCT &v);
bool StrToValue(const char *s, SQL_DATE_STRUCT &v);
bool StrToValue(const std::string &s, SQL_TIME_STRUCT &v);
bool StrToValue(const char *s, SQL_TIME_STRUCT &v);
bool StrToValue(const std::string &s, SQL_TIMESTAMP_STRUCT &v);
bool StrToValue(const char *s, SQL_TIMESTAMP_STRUCT &v);
void ValueToStr(const SQL_DATE_STRUCT& v, std::string& str);
void ValueToStr(const SQL_TIME_STRUCT& v, std::string& str);
void ValueToStr(const SQL_TIMESTAMP_STRUCT& v, std::string& str);

HDB_END_NAMESPACE

#endif // _HDB_TYPES_H
