#ifndef _HDB_TYPES_H
#define _HDB_TYPES_H

#include <stdlib.h>

namespace hdb {

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
    T_TEXT,
    T_MAX = T_TEXT
} DATA_TYPE_T;

typedef struct {
#ifdef _DEBUG
    DATA_TYPE_T type;
#else
    unsigned char type; // DATA_TYPE_T
#endif
    union {
        unsigned char a; // for CHAR(a) and NCHAR(a)
        unsigned char p; // p,s for DECIMAL(p,s)
    };
    unsigned char s;
    bool null_able;
} DATA_ATTR_T;


DATA_ATTR_T GenDataAttr(DATA_TYPE_T type, bool null_able, int param1, int param2);
const char *DataTypeToStr(DATA_TYPE_T type);
DATA_TYPE_T StrToDataType(const char *type_str);

static inline bool StrToValue(const std::string &s, char &v) {
    if (s.empty()) return false;
    v = (char)atoi(s.c_str());
    return true;
}
static inline bool StrToValue(const std::string &s, short &v) {
    if (s.empty()) return false;
    v = (short)atoi(s.c_str());
    return true;
}
static inline bool StrToValue(const std::string &s, int &v) {
    if (s.empty()) return false;
    v = atoi(s.c_str());
    return true;
}
static inline bool StrToValue(const std::string &s, SQLBIGINT &v) {
    if (s.empty()) return false;
#ifdef _WIN32
    v = _atoi64(s.c_str());
#else
    v = atoll(s.c_str());
#endif
    return true;
}
static inline bool StrToValue(const std::string &s, float &v) {
    if (s.empty()) return false;
    v = (float)atof(s.c_str());
    return true;
}
static inline bool StrToValue(const std::string &s, double &v) {
    if (s.empty()) return false;
    v = atof(s.c_str());
    return true;
}
bool StrToValue(const std::string &s, SQL_DATE_STRUCT &v);
bool StrToValue(const std::string &s, SQL_TIME_STRUCT &v);
bool StrToValue(const std::string &s, SQL_TIMESTAMP_STRUCT &v);

}// namespace hdb

#endif // _HDB_TYPES_H
