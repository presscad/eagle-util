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

#ifdef PROJ_SIM_AFL
namespace sim_afl {
namespace util {
    void ParseCsvLine(std::vector<std::string> &record, const std::string &line, char delimiter);
}
}
#else
namespace util {
    void ParseCsvLine(std::vector<std::string> &record, const std::string &line, char delimiter);
}
#endif

HDB_BEGIN_NAMESPACE

typedef enum {
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
} DATA_TYPE_T;

typedef struct DATA_ATTR_T {
    DATA_TYPE_T type{};
    int a{}; // for CHAR(a), VARCHAR(a), NCHAR(a), etc.
    int p{}, s{}; // for DECIMAL(p,s)
    bool null_able{};
} DATA_ATTR_T;


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
    if (s.empty()) return false;
    v = (char)atoi(s.c_str());
    return true;
}

static inline bool StrToValue(const char *s, char &v)
{
    if (s == nullptr) return false;
    v = (char)atoi(s);
    return true;
}

static inline std::string ValueToStr(char v)
{
    char buff[8];
    snprintf(buff, sizeof(buff)-1, "%d", (int)v);
    return buff;
}

// used by TINYINT
static inline bool StrToValue(const std::string &s, unsigned char &v)
{
    if (s.empty()) return false;
    v = (unsigned char)atoi(s.c_str());
    return true;
};

static inline bool StrToValue(const char *s, unsigned char &v)
{
    if (s == nullptr) return false;
    v = (unsigned char)atoi(s);
    return true;
};

static inline std::string ValueToStr(unsigned char &v)
{
    char buff[8];
    snprintf(buff, sizeof(buff)-1, "%d", (int)v);
    return buff;
};

static inline bool StrToValue(const std::string &s, short &v)
{
    if (s.empty()) return false;
    v = (short)atoi(s.c_str());
    return true;
}

static inline bool StrToValue(const char *s, short &v)
{
    if (s == nullptr) return false;
    v = (short)atoi(s);
    return true;
}

static inline std::string ValueToStr(short v)
{
    char buff[8];
    snprintf(buff, sizeof(buff)-1, "%d", (int)v);
    return buff;
}

bool StrToValue(const std::string &s, int &v);
bool StrToValue(const char *s, int &v);

static inline std::string ValueToStr(int v)
{
    char buff[24];
    snprintf(buff, sizeof(buff)-1, "%d", v);
    return buff;
}

bool StrToValue(const std::string &s, SQLBIGINT &v);
bool StrToValue(const std::string &s, long long &v);
bool StrToValue(const char *s, SQLBIGINT &v);

static inline std::string ValueToStr(SQLBIGINT v)
{
    char buff[32];
    snprintf(buff, sizeof(buff)-1, "%lld", (long long)v);
    return buff;
}

static inline bool StrToValue(const std::string &s, float &v)
{
    if (s.empty()) return false;
    v = (float)atof(s.c_str());
    return true;
}

static inline bool StrToValue(const char *s, float &v)
{
    if (s == nullptr) return false;
    v = (float)atof(s);
    return true;
}

static inline std::string ValueToStr(float v)
{
    char buff[32];
    snprintf(buff, sizeof(buff)-1, "%f", v);
    return buff;
}

static inline bool StrToValue(const std::string &s, double &v)
{
    if (s.empty()) return false;
    v = atof(s.c_str());
    return true;
}

static inline bool StrToValue(const char *s, double &v)
{
    if (s == nullptr) return false;
    v = atof(s);
    return true;
}

static inline std::string ValueToStr(double v)
{
    char buff[32];
    snprintf(buff, sizeof(buff)-1, "%lf", v);
    return buff;
}

static inline bool StrToValue(const std::string &s, SQLWCHAR &v)
{
    UnImplemented(__FUNCTION__);
    if (!s.empty()) {
        v = (SQLWCHAR)atoi(s.c_str());
        return true;
    }
    return false;
};

static inline bool StrToValue(const char *s, SQLWCHAR &v)
{
    UnImplemented(__FUNCTION__);
    if (s != nullptr) {
        v = (SQLWCHAR)atoi(s);
        return true;
    }
    return false;
};

static inline std::string ValueToStr(SQLWCHAR &v)
{
    UnImplemented(__FUNCTION__);
    return std::string(1, static_cast<char>(v));
};

bool StrToValue(const std::string &s, SQL_DATE_STRUCT &v);
bool StrToValue(const char *s, SQL_DATE_STRUCT &v);
bool StrToValue(const std::string &s, SQL_TIME_STRUCT &v);
bool StrToValue(const char *s, SQL_TIME_STRUCT &v);
bool StrToValue(const std::string &s, SQL_TIMESTAMP_STRUCT &v);
bool StrToValue(const char *s, SQL_TIMESTAMP_STRUCT &v);
std::string ValueToStr(const SQL_DATE_STRUCT& v);
std::string ValueToStr(const SQL_TIME_STRUCT& v);
std::string ValueToStr(const SQL_TIMESTAMP_STRUCT& v);

HDB_END_NAMESPACE

#endif // _HDB_TYPES_H
