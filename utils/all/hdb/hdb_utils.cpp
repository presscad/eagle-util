#if defined(_WIN32) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS // Disable security warning message on MSVC
#endif

#include <algorithm>
#include <cmath>
#include <cstdlib>
#ifndef _WIN32
#include <sys/time.h>
#endif
#include "hdb_utils.h"


USING_HDB_NAMESPACE;
using namespace std;

#ifndef _COUNOF
#define _COUNOF(a)  sizeof(a)/sizeof(*a)
#endif

#ifdef _WIN32
#define STRICMP _stricmp
#else
#define STRICMP strcasecmp
#endif

HDB_BEGIN_NAMESPACE

static const char *TYPE_STRS[] = {
    "UNKNOWN",
    "TINYINT",
    "SMALLINT",
    "INTEGER",
    "BIGINT",
    "REAL",
    "DOUBLE",
    "FLOAT",
    "DATE",
    "TIME",
    "TIMESTAMP",
    "SECONDDATE",
    "CHAR",
    "NCHAR",
    "VARCHAR",
    "NVARCHAR",
    "ALPHANUM",
    "SMALLDECIMAL",
    "DECIMAL",
    "DECIMAL_PS",
    "BINARY",
    "VARBINARY",
    "BLOB",
    "CLOB",
    "NCLOB",
    "TEXT",
    "ST_GEOMETRY",
};

DATA_ATTR_T GenDataAttr(DATA_TYPE_T type, bool null_able, int p, int s)
{
    DATA_ATTR_T t;
    t.type = type;
    t.null_able = null_able;
    t.p = p;
    t.s = s;
    if (type == T_DECIMAL_PS && p > 0) {
        // plus bytes for sign and decimal point, e.g.,
        // for DECIMAL(4,2), -32.77 needs  4 + 2 bytes
        // for DECIMAL(4,4), -0.1234 needs  4 + 3 bytes
        t.a = p + 3;
    }
    else {
        t.a = p;
    }
    return t;
};

const char *DataTypeToStr(DATA_TYPE_T type)
{
    return ((unsigned int)type > T_MAX) ? TYPE_STRS[0] : TYPE_STRS[(unsigned int)type];
}

DATA_TYPE_T StrToDataType(const char *type_str)
{
    std::string typestr(type_str);
    StrToUpper(typestr);

    for (int i = 0; i <= T_MAX; i++) {
        if (typestr == TYPE_STRS[i]) {
            return (DATA_TYPE_T)i;
        }
    }

    static const struct {
        const char *type_str;
        DATA_TYPE_T type;
    } MORE_TYPES[] = {
        {"INT",         T_INTEGER},
        {"DEC",         T_DECIMAL},
        {"DAYDATE",     T_DATE},
        {"SECONDTIME",  T_TIME},
        {"LONGDATE",    T_TIMESTAMP},
    };
    for (auto& e : MORE_TYPES) {
        if (typestr == e.type_str) {
            return e.type;
        }
    }

    static const struct {
        const char *type_str_sub;
        DATA_TYPE_T type;
    } MORE_TYPES_BEGIN_WITH[] = {
        {"FLOAT(",      T_FLOAT},
        {"DECIMAL(",    T_DECIMAL_PS},
        {"DEC(",        T_DECIMAL_PS},
        {"VARCHAR(",    T_VARCHAR},
        {"NVARCHAR(",   T_NVARCHAR},
        {"CHAR(",       T_CHAR},
        {"NCHAR(",      T_NCHAR},
        {"ALPHANUM(",   T_ALPHANUM},
        {"ST_GEOMETRY(",T_ST_GEOMETRY},
    };
    for (auto& e : MORE_TYPES_BEGIN_WITH) {
        if (typestr.compare(0, std::strlen(e.type_str_sub), e.type_str_sub) == 0) {
            return e.type;
        }
    }
    return T_UNKNOWN;
}

void GetCurTm(struct tm &stm) {
    time_t t;
    time(&t);
#ifdef _WIN32
    localtime_s(&stm, &t);
#else
    localtime_r(&t, &stm);
#endif
};

void GetCurTimestamp(SQL_TIMESTAMP_STRUCT &st) {
    struct tm stm;
    GetCurTm(stm);
    st.year = stm.tm_year + 1900;
    st.month = stm.tm_mon + 1;
    st.day = stm.tm_mday;
    st.hour = stm.tm_hour;
    st.minute = stm.tm_min;
    st.second = stm.tm_sec;
    st.fraction = 0;
};

void GetCurDate(SQL_DATE_STRUCT &date) {
    struct tm stm;
    GetCurTm(stm);
    date.year = stm.tm_year + 1900;
    date.month = stm.tm_mon + 1;
    date.day = stm.tm_mday;
};

void GetCurTime(SQL_TIME_STRUCT &time) {
    struct tm stm;
    GetCurTm(stm);
    time.hour = stm.tm_hour;
    time.minute = stm.tm_min;
    time.second = stm.tm_sec;
};

// NOTE: to impprove performance, below StrToValue functions assume the string are valid
//       and dose not handle invalid cases

bool StrToValue(const std::string &s, int &v)
{
    if (s.empty()) return false;
    if (s.front() == '0') {
        if (s.size() > 2 && (s[1] == 'x' || s[1] == 'X')) {
            v = std::strtol(s.c_str() + 2, nullptr, 16);
            return true;
        }
    }
    v = std::atoi(s.c_str());
    return true;
}

bool StrToValue(const char *s, int &v)
{
    if (s == nullptr) return false;
    if (s[0] == '0') {
        if (strlen(s) > 2 && (s[1] == 'x' || s[1] == 'X')) {
            v = std::strtol(s + 2, nullptr, 16);
            return true;
        }
    }
    v = std::atoi(s);
    return true;
}

bool StrToValue(const std::string &s, SQLBIGINT &v)
{
    if (s.empty()) return false;
    if (s.front() == '0') {
        if (s.size() > 2 && (s[1] == 'x' || s[1] == 'X')) {
            v = std::strtoll(s.c_str() + 2, nullptr, 16);
            return true;
        }
    }
    v = std::strtoll(s.c_str(), nullptr, 10);
    return true;
}

#ifndef _WIN32
bool StrToValue(const std::string &s, long long &v)
{
    if (s.empty()) return false;
    if (s.front() == '0') {
        if (s.size() > 2 && (s[1] == 'x' || s[1] == 'X')) {
            v = std::strtoll(s.c_str() + 2, nullptr, 16);
            return true;
        }
    }
    v = std::strtoll(s.c_str(), nullptr, 10);
    return true;
}
#endif

bool StrToValue(const char *s, SQLBIGINT &v)
{
    if (s == nullptr) return false;
    if (s[0] == '0') {
        if (strlen(s) > 2 && (s[1] == 'x' || s[1] == 'X')) {
            v = std::strtoll(s + 2, nullptr, 16);
            return true;
        }
    }
    v = std::strtoll(s, nullptr, 10);
    return true;
}

bool StrToValue(const std::string &s, SQL_DATE_STRUCT &v)
{
    return StrToValue(s.c_str(), v);
}

bool StrToValue(const char *s, SQL_DATE_STRUCT &v)
{
    util::TIMESTAMP_STRUCT ts;
    if (false == util::ParseTimestamp(s, ts)) {
        return false;
    }
    v.year = ts.year;
    v.month = ts.month;
    v.day = ts.day;
    return true;
}

bool StrToValue(const std::string &s, SQL_TIME_STRUCT &v)
{
    return util::ParseTime(s, reinterpret_cast<util::TIME_STRUCT&>(v));
}

bool StrToValue(const char *s, SQL_TIME_STRUCT &v)
{
    return util::ParseTime(s, reinterpret_cast<util::TIME_STRUCT&>(v));
}

bool StrToValue(const std::string &s, SQL_TIMESTAMP_STRUCT &v)
{
    return util::ParseTimestamp(s, reinterpret_cast<util::TIMESTAMP_STRUCT&>(v));
}

bool StrToValue(const char *s, SQL_TIMESTAMP_STRUCT &v)
{
    return util::ParseTimestamp(s, reinterpret_cast<util::TIMESTAMP_STRUCT&>(v));
}

std::string ValueToStr(const SQL_DATE_STRUCT& v)
{
    char buff[64];
    snprintf(buff, sizeof(buff)-1, "%04d-%02d-%02d", v.year, v.month, v.day);
    return buff;
}

std::string ValueToStr(const SQL_TIME_STRUCT& v)
{
    char buff[64];
    snprintf(buff, sizeof(buff)-1, "%02d:%02d:%02d", v.hour, v.minute, v.second);
    return buff;
}

std::string ValueToStr(const SQL_TIMESTAMP_STRUCT& v)
{
    char buff[64];
    if (v.fraction) {
        snprintf(buff, sizeof(buff) - 1, "%04d-%02d-%02d %02d:%02d:%02d.%06d",
            v.year, v.month, v.day, v.hour, v.minute, v.second, v.fraction);
    }
    else {
        snprintf(buff, sizeof(buff) - 1, "%04d-%02d-%02d %02d:%02d:%02d",
            v.year, v.month, v.day, v.hour, v.minute, v.second);
    }
    return buff;
}

// "    too much\t   \tspace\t\t\t  " => "too much\t   \tspace"
std::string &TrimStr(std::string& str, const char *whitespace /*= " \t"*/)
{
    const size_t strBegin = str.find_first_not_of(whitespace);
    if (strBegin == std::string::npos) {
        str.clear(); // no content
    } else {
        const size_t strEnd = str.find_last_not_of(whitespace);
        const size_t strRange = strEnd - strBegin + 1;
        str = str.substr(strBegin, strRange);
    }
    return str;
}

// "    too much\t   \tspace\t\t\t  " => "too-much-space" if fill is "-"
std::string &ReduceStr(std::string& str, const char *fill/*= " "*/, const char *whitespace /*=" \t"*/)
{
    // trim first
    TrimStr(str, whitespace);

    // replace sub ranges
    size_t beginSpace = str.find_first_of(whitespace);
    while (beginSpace != std::string::npos)
    {
        const size_t endSpace = str.find_first_not_of(whitespace, beginSpace);
        const size_t range = endSpace - beginSpace;

        str.replace(beginSpace, range, fill);

        const size_t newStart = beginSpace + strlen(fill);
        beginSpace = str.find_first_of(whitespace, newStart);
    }

    return str;
}

void StrToUpper(std::string& str)
{
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
}

void StrToLower(std::string& str)
{
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
}

#ifdef _WIN32
static string16 utf82ws(const std::string& s)
{
    int len;
    int slength = (int)s.length();
    len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), slength, 0, 0);
    string16 ws(len, (unsigned short)0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), slength, (LPWSTR)&ws[0], len);
    return ws;
}

static std::string ws2utf8(const SQLWCHAR *ws)
{
    int len;
    int slength = (int)wcslen(ws);
    len = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)ws, slength, 0, 0, 0, 0);
    std::string r(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)ws, slength, &r[0], len, 0, 0);
    return r;
}
#endif

string16 StrToWStr(const std::string &str)
{
#ifdef _WIN32
    return utf82ws(str);
#else
    string16 wstr;
    wstr.reserve(str.size());
    auto in = str.c_str();
    if (in == nullptr)
        return wstr;

    unsigned int codepoint = 0;
    while (*in != 0) {
        unsigned char ch = static_cast<unsigned char>(*in);
        if (ch <= 0x7f) {
            codepoint = ch;
        }
        else if (ch <= 0xbf) {
            codepoint = (codepoint << 6) | (ch & 0x3f);
        }
        else if (ch <= 0xdf) {
            codepoint = ch & 0x1f;
        }
        else if (ch <= 0xef) {
            codepoint = ch & 0x0f;
        }
        else {
            codepoint = ch & 0x07;
        }
        ++in;
        if (((*in & 0xc0) != 0x80) && (codepoint <= 0x10ffff)) {
            if (codepoint > 0xffff) {
                wstr.append(1, static_cast<wchar_t>(0xd800 + (codepoint >> 10)));
                wstr.append(1, static_cast<wchar_t>(0xdc00 + (codepoint & 0x03ff)));
            }
            else if (codepoint < 0xd800 || codepoint >= 0xe000) {
                wstr.append(1, static_cast<wchar_t>(codepoint));
            }
        }
    }
    return wstr;
#endif
}

std::string WStrToStr(const string16 &wstr)
{
#ifdef _WIN32
    return ws2utf8((SQLWCHAR *)wstr.c_str());
#else
    std::string str;
    str.reserve(wstr.size() * 3);
    auto in = wstr.c_str();
    unsigned int codepoint = 0;

    for (; *in != 0; ++in) {
        if (*in >= 0xd800 && *in <= 0xdbff) {
            codepoint = ((*in - 0xd800) << 10) + 0x10000;
        }
        else {
            if (*in >= 0xdc00 && *in <= 0xdfff) {
                codepoint |= *in - 0xdc00;
            }
            else {
                codepoint = *in;
            }
            if (codepoint <= 0x7f) {
                str.append(1, static_cast<char>(codepoint));
            }
            else if (codepoint <= 0x7ff) {
                str.append(1, static_cast<char>(0xc0 | ((codepoint >> 6) & 0x1f)));
                str.append(1, static_cast<char>(0x80 | (codepoint & 0x3f)));
            }
            else if (codepoint <= 0xffff) {
                str.append(1, static_cast<char>(0xe0 | ((codepoint >> 12) & 0x0f)));
                str.append(1, static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
                str.append(1, static_cast<char>(0x80 | (codepoint & 0x3f)));
            }
            else {
                str.append(1, static_cast<char>(0xf0 | ((codepoint >> 18) & 0x07)));
                str.append(1, static_cast<char>(0x80 | ((codepoint >> 12) & 0x3f)));
                str.append(1, static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
                str.append(1, static_cast<char>(0x80 | (codepoint & 0x3f)));
            }
            codepoint = 0;
        }
    }
    return str;
#endif
}

std::string WStrToStr(const SQLWCHAR *wstr)
{
#ifdef _WIN32
    return ws2utf8(wstr);
#else
    std::string str;
    str.reserve(wcslen((const wchar_t*)wstr) * 3);
    auto in = wstr;
    unsigned int codepoint = 0;

    for (; *in != 0; ++in) {
        if (*in >= 0xd800 && *in <= 0xdbff) {
            codepoint = ((*in - 0xd800) << 10) + 0x10000;
        }
        else {
            if (*in >= 0xdc00 && *in <= 0xdfff) {
                codepoint |= *in - 0xdc00;
            }
            else {
                codepoint = *in;
            }
            if (codepoint <= 0x7f) {
                str.append(1, static_cast<char>(codepoint));
            }
            else if (codepoint <= 0x7ff) {
                str.append(1, static_cast<char>(0xc0 | ((codepoint >> 6) & 0x1f)));
                str.append(1, static_cast<char>(0x80 | (codepoint & 0x3f)));
            }
            else if (codepoint <= 0xffff) {
                str.append(1, static_cast<char>(0xe0 | ((codepoint >> 12) & 0x0f)));
                str.append(1, static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
                str.append(1, static_cast<char>(0x80 | (codepoint & 0x3f)));
            }
            else {
                str.append(1, static_cast<char>(0xf0 | ((codepoint >> 18) & 0x07)));
                str.append(1, static_cast<char>(0x80 | ((codepoint >> 12) & 0x3f)));
                str.append(1, static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
                str.append(1, static_cast<char>(0x80 | (codepoint & 0x3f)));
            }
            codepoint = 0;
        }
    }
    return str;
#endif
}

void ReplaceCharInStr(std::string& str, char ch1, char ch2)
{
    std::replace(str.begin(), str.end(), ch1, ch2);
}

void StringReplace(std::string &strBase, const std::string &strSrc, const std::string &strDes)
{
    std::string::size_type pos = 0;
    std::string::size_type srcLen = strSrc.size();
    std::string::size_type desLen = strDes.size();
    pos = strBase.find(strSrc, pos);
    while ((pos != std::string::npos)) {
        strBase.replace(pos, srcLen, strDes);
        pos=strBase.find(strSrc, (pos+desLen));
    }
}

static void SplitParams(std::vector<std::string> &record, const char *params)
{
    util::ParseCsvLine(record, params, ',');

    for (size_t i = 0; i < record.size(); i++) {
        if (record[i].find('(') != std::string::npos) {
            std::string upper(record[i]);
            StrToUpper(upper);

            // if starting with "DECIMAL(", could be "DECIMAL(p,s), DECIMAL(p)" 
            if (upper.find("DECIMAL(") != std::string::npos || upper.find("DEC(") != std::string::npos) {
                if (upper.back() != ')' && i + 1 < record.size()) {
                    record[i] += ',';
                    record[i] += record[i+1];
                    record.erase(record.begin() + i + 1);
                }
            }
        }
    }
}

static void ParseParamStr(const std::string &param, int &p1, int &p2)
{
    DATA_TYPE_T type = StrToDataType(param.c_str());
    assert(T_UNKNOWN != type);

    if (type == T_DECIMAL_PS) {
        const char *s1 = strchr(param.c_str(), '(');
        if (s1) {
            s1++;
            p1 = atoi(s1);

            const char *s2 = strchr(s1, ',');
            if (s2) {
                p2 = atoi(s2+1);
            }
        }
    } else if (T_CHAR == type || T_NCHAR == type || T_VARCHAR == type || T_NVARCHAR == type ||
        T_ALPHANUM == type || T_FLOAT == type)
    {
        const char *s1 = strchr(param.c_str(), '(');
        p1 = s1 ? atoi(s1+1) : 0;
    }
}

static void TrimRightFromNoCase(std::string &str, const char *sub)
{
    std::string strUpper(str), subUpper(sub);
    StrToUpper(strUpper);
    StrToUpper(subUpper);

    size_t pos = strUpper.find(subUpper);
    if (pos != std::string::npos) {
        str.resize(pos);
    }
}

bool ParseTableFromSql(const char *create_sql, PARSED_TABLE_T &table, std::string &err_str)
{
    if (create_sql == NULL) {
        err_str = "Null pointer for passed in SQL!";
        return false;
    }

    PARSED_TABLE_T parsed_table;
    parsed_table.create_sql = create_sql;
    parsed_table.column = false;

    const char *s_begin = strchr(create_sql, '(');
    if (s_begin == NULL) {
        err_str = "Not found '(' in SQL passed in.";
        return false;
    }

    {
        std::string create(create_sql);
        create.erase(s_begin - create_sql);
        ReduceStr(create); // Now, e.g., create = CREATE COLUMN TABLE "I078212"."GPS29"
        std::vector<std::string> subs;
        util::ParseCsvLine(subs, create.c_str(), ' ');

        size_t sub_count = subs.size();
        if (sub_count < 3) {
            err_str = "Too few items in \"" + create + '\"';
            return false;
        }
        if (sub_count > 3 && !STRICMP(subs[1].c_str(), "COLUMN")) {
            parsed_table.column = true;
        }

        {
            std::vector<std::string> strs;
            util::ParseCsvLine(strs, subs[sub_count - 1].c_str(), '.');
            if (strs.size() == 1) {
                parsed_table.schema.clear();
                parsed_table.table_name = strs[0];
            } else if (strs.size() == 2) {
                parsed_table.schema = strs[0];
                parsed_table.table_name = strs[1];
            } else {
                if (strs.size() == 0) {
                    err_str = "Too few items in \"" + subs[sub_count - 1] + '\"';
                } else {
                    err_str = "Too many items in \"" + subs[sub_count - 1] + "\". Try \"\" for scehma or table name";
                }
                return false;
            }

            bool add_quote = false;
            if ((parsed_table.schema.find('.') != string::npos) ||
                (parsed_table.table_name.find('.') != string::npos)) {
                    add_quote = true;
            }
            if (add_quote) {
                parsed_table.schema = '\"' + parsed_table.schema + '\"';
                parsed_table.table_name = '\"' + parsed_table.table_name + '\"';
            }
        }
    }

    s_begin++;
    std::string str(s_begin);
    TrimRightFromNoCase(str, "PARTITION BY"); // strip the sub-string starting from "PARTITION BY"
    TrimRightFromNoCase(str, "WITH PARAMETERS"); // strip the sub-string starting from "WITH PARAMETERS"

    const char *s_end = strrchr(str.c_str(), ')');
    if (!s_end) {
        err_str = "Not found ')' in SQL passed in.";
        return false;
    }
    str.erase(s_end - str.c_str());
    std::replace(str.begin(), str.end(), '\t', ' ');
    while(str.find(" (") != std::string::npos) {
        StringReplace(str, " (", "(");
    }

    SplitParams(parsed_table.col_strs, str.c_str());
    size_t col_count = parsed_table.col_strs.size();
    if (col_count == 0) {
        return false;
    }
    for (size_t i = 0; i < col_count; i++) {
        std::string& col_str = parsed_table.col_strs[i];
        ReduceStr(col_str);
        StringReplace(col_str, "( ", "(");
        StringReplace(col_str, " (", "(");
        StringReplace(col_str, ") ", ")");
        StringReplace(col_str, " )", ")");

        std::vector<std::string> subs;
        util::ParseCsvLine(subs, col_str.c_str(), ' ');
        if (subs.size() < 2) {
            err_str = "Error in parsing: " + col_str + ": too few items!";
            return false;
        }
        parsed_table.col_names.push_back(subs[0]);

        StrToUpper(subs[1]);
        parsed_table.col_str_types.push_back(subs[1]);

        int p1 = 0, p2 = 0;
        DATA_TYPE_T type = StrToDataType(subs[1].c_str());
        if (T_UNKNOWN == type) {
            err_str = "Unknown type: " + subs[1];
            return false;
        }
        parsed_table.col_types.push_back(type);

        if (T_FLOAT == type || T_DECIMAL_PS == type || T_CHAR == type || T_NCHAR == type || 
            T_VARCHAR == type || T_NVARCHAR == type || T_ALPHANUM == type) {
            ParseParamStr(subs[1], p1, p2);
        }

        // check null-able for the parameter
        bool null_able = true;
        if (subs.size() > 2) {
            std::string rest;
            for (size_t i = 2; i < subs.size(); i++) {
                rest += subs[i];
                rest += ' ';
            }
            StrToUpper(rest);
            if (NULL != strstr(rest.c_str(), "NOT NULL ")) {
                null_able = false;
            }
        }
        parsed_table.col_attrs.push_back(GenDataAttr(type, null_able, p1, p2));
    }

    // detect if there are same column names
    for (size_t i = 0; i < parsed_table.col_names.size(); ++i) {
        for (size_t j = 0; j < i; ++j) {
            if (parsed_table.col_names[i] == parsed_table.col_names[j]) {
                err_str = "Duplicated column names: " + parsed_table.col_names[i];
                return false;
            }
        }
    }

    table = parsed_table;
    return true;
}

void UnImplemented(const char *desc)
{
    if (desc) {
        std::cout << ElapsedTimeStr() << ": Unimplemented feature: " << desc << std::endl;
    } else {
        std::cout << ElapsedTimeStr() << ": Unimplemented feature" << std::endl;
    }
    std::cout << ElapsedTimeStr() << ": THIS APP IS CRASHING!" << std::endl;
    *(int *)0 = 0; // to crash!
}

std::string FormatTimeStr(unsigned long uTimeMs)
{
    char buff[64];
    sprintf(buff, "%2lu:%02lu:%03lu",
        (uTimeMs/60000), (uTimeMs/1000) % 60, uTimeMs % 1000);
    return buff;
}

static long get_time_in_ms()
{
#ifdef _WIN32
    return (long)GetTickCount();
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
#endif
}

static unsigned long g_dwStart = get_time_in_ms();
std::string ElapsedTimeStr()
{
    return FormatTimeStr(get_time_in_ms() - g_dwStart);
}

HDB_END_NAMESPACE
