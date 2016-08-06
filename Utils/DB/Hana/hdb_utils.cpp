#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS // Disable security warning message on MSVC
#endif

#include <algorithm>
#ifndef _WIN32
#include <sys/time.h>
#endif
#include "hdb_utils.h"

using namespace hdb;
using namespace std;

#ifndef _COUNOF
#define _COUNOF(a)  sizeof(a)/sizeof(*a)
#endif

#ifdef _WIN32
#define STRICMP _stricmp
#else
#define STRICMP strcasecmp
#endif

namespace hdb {

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
    "TEXT",
};

DATA_ATTR_T GenDataAttr(DATA_TYPE_T type, bool null_able, int param1, int param2)
{
    DATA_ATTR_T t;
    t.type = type;
    t.null_able = null_able;
    t.p = param1;
    t.s = param2;
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
        {"DEC",         T_DECIMAL},
        {"DAYDATE",     T_DATE},
        {"SECONDTIME",  T_TIME},
        {"LONGDATE",    T_TIMESTAMP},
    };
    for (unsigned int i = 0; i < _COUNOF(MORE_TYPES); i++) {
        if (typestr == MORE_TYPES[i].type_str) {
            return MORE_TYPES[i].type;
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
    };
    for (unsigned int i = 0; i < _COUNOF(MORE_TYPES_BEGIN_WITH); i++) {
        if (typestr.find(MORE_TYPES_BEGIN_WITH[i].type_str_sub) == 0) {
            return MORE_TYPES_BEGIN_WITH[i].type;
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
	st.fraction = rand();
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

bool StrToValue(const std::string &s, SQL_DATE_STRUCT &v)
{
    if (s.empty()) return false;
    int year, month, day;
    if (3 == sscanf(s.c_str(), "%d-%d-%d", &year, &month, &day)) {
        v.year = year;
        v.month = month;
        v.day = day;
        if (v.year < 0 || v.year > 3000) return false;
        if (v.month <= 0 || v.month > 12) return false;
        if (v.day <= 0 || v.day > 31) return false;
        return true;
    }
    return false;
}

bool StrToValue(const std::string &s, SQL_TIME_STRUCT &v)
{
    if (s.empty()) return false;
    int hour, minute, second;
    if (3 == sscanf(s.c_str(), "%d:%d:%d", &hour, &minute, &second)) {
        v.hour = hour;
        v.minute = minute;
        v.second = second;
        if (v.hour > 24) return false;
        if (v.minute >= 60) return false;
        if (v.second >= 60) return false;
        return true;
    }
    return false;
}

bool StrToValue(const std::string &s, SQL_TIMESTAMP_STRUCT &v)
{
    if (s.empty()) return false;

    int year, month, day, hour, minute, second, fraction;

    int r = sscanf(s.c_str(), "%d-%d-%d%*[ -]%d%*[:.]%d%*[:.]%d%*[:.]%d",
        &year, &month, &day, &hour, &minute, &second, &fraction);
    if (r == 5 || r == 6 || r == 7) {
        if (r == 5) {
            second = fraction = 0;
        } else if (r == 6) {
            fraction = 0;
        }

        if (year <= 0 || year > 3000) return false;
        if (month < 0 || month > 12) {
            return false;
        } else if (month == 0) {
            month = 1;
        }
        if (day < 0 || day > 31) {
            return false;
        } else if (day == 0) {
            day = 1;
        }
        if (hour > 24) return false;
        if (minute > 60) return false;
        if (second > 60) return false;

        v.year = year;
        v.month = month;
        v.day = day;
        v.hour = hour;
        v.minute = minute;
        v.second = second;
        v.fraction = fraction;

        return true;
    }
    return false;
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
    int slength = (int)s.length() + 1;
    len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), slength, 0, 0); 
    string16 ws(len, (unsigned short)0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), slength, (LPWSTR)&ws[0], len);
    return ws;
}

static std::string ws2utf8(const string16& ws)
{
    int len;
    int slength = (int)ws.length() + 1;
    len = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)ws.c_str(), slength, 0, 0, 0, 0); 
    std::string r(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)ws.c_str(), slength, &r[0], len, 0, 0);
    return r;
}
#endif

string16 StrToWStr(const std::string &str)
{
#ifdef _WIN32
    return utf82ws(str);
#else
    return string16(str.begin(), str.end());
#endif
}

std::string WStrToStr(const string16 &wstr)
{
#ifdef _WIN32
    return ws2utf8(wstr);
#else
    return std::string(wstr.begin(), wstr.end());
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

#if 0
void CsvLinePopulate(vector<string> &record, const char *line, char delimiter)
{
    int linepos = 0;
    bool inquotes = false;
    char c;
    int linemax = (int)strlen(line);
    string curstring;
    record.clear();

    while(line[linepos]!=0 && linepos < linemax)
    {
        c = line[linepos];

        if (!inquotes && curstring.length()==0 && c=='"')
        {
            //beginquotechar
            inquotes=true;
        }
        else if (inquotes && c=='"')
        {
            //quotechar
            if ( (linepos+1 <linemax) && (line[linepos+1]=='"') ) 
            {
                //encountered 2 double quotes in a row (resolves to 1 double quote)
                curstring.push_back(c);
                linepos++;
            }
            else
            {
                //endquotechar
                inquotes=false; 
            }
        }
        else if (!inquotes && c==delimiter)
        {
            //end of field
            record.push_back( curstring );
            curstring="";
        }
        else if (!inquotes && (c=='\r' || c=='\n') )
        {
            record.push_back( curstring );
            return;
        }
        else
        {
            curstring.push_back(c);
        }
        linepos++;
    }
    record.push_back( curstring );
    return;
}
#else
// Optimized version
void CsvLinePopulate(std::vector<std::string> &record, const std::string &line, char delimiter)
{
    int linepos = 0;
    bool inquotes = false;
    char c;
    int linemax = (int)line.size();

    char curstring[1024];
    int cur_cur = 0;
    int rec_num = 0, rec_size = (int)record.size();

    while(linepos < linemax)
    {
        c = line[linepos];

        if (!inquotes && cur_cur==0 && c=='"')
        {
            //beginquotechar
            inquotes=true;
        }
        else if (inquotes && c=='"')
        {
            //quotechar
            if ( (linepos+1 <linemax) && (line[linepos+1]=='"') ) 
            {
                //encountered 2 double quotes in a row (resolves to 1 double quote)
                curstring[cur_cur++] = c;
                linepos++;
            }
            else
            {
                //endquotechar
                inquotes=false; 
            }
        }
        else if (!inquotes && c==delimiter)
        {
            //end of field
            curstring[cur_cur] = '\0';
            if (rec_num >= rec_size) {
                record.push_back( curstring );
                rec_size++;
            } else {
                record[rec_num] = curstring;
            }
            rec_num++;

            cur_cur = 0;
        }
        else if (!inquotes && (c=='\r' || c=='\n') )
        {
            break;
        }
        else
        {
            curstring[cur_cur++] = c;
        }
        linepos++;
    }

    curstring[cur_cur] = '\0';
    if (rec_num >= rec_size) {
        record.push_back( curstring );
        rec_size++;
    } else {
        record[rec_num] = curstring;
    }
    rec_num++;

    if (rec_size > rec_num) {
        record.resize(rec_num);
    }
}
#endif

static void SplitParams(std::vector<std::string> &record, const char *params)
{
    CsvLinePopulate(record, params, ',');

    for (size_t i = 0; i < record.size(); i++) {
        if (record[i].find('(') != std::string::npos) {
            std::string upper(record[i]);
            StrToUpper(upper);
            if (upper.find("DECIMAL(") != std::string::npos || upper.find("DEC(") != std::string::npos) {
                if (i + 1 < record.size()) {
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
        CsvLinePopulate(subs, create.c_str(), ' ');

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
            CsvLinePopulate(strs, subs[sub_count - 1].c_str(), '.');
            if (strs.size() == 1) {
                parsed_table.schema.clear();
                parsed_table.table_name = strs[0];
            } else if (strs.size() == 2) {
                parsed_table.schema = strs[0];
                parsed_table.table_name = strs[1];
            } else {
                err_str = "Too few items in \"" + create + '\"';
                return false;
            }
            StrToUpper(parsed_table.schema);
            StrToUpper(parsed_table.table_name);
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
        std::string &col_str = parsed_table.col_strs[i];
        ReduceStr(col_str);

        std::vector<std::string> subs;
        CsvLinePopulate(subs, col_str.c_str(), ' ');
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

    table = parsed_table;
    return true;
}

void UnImplemented(const char *desc)
{
    if (desc) {
        printf("%s: Unimplemented feature: %s\n", ElapsedTimeStr().c_str(), desc);
    } else {
        printf("%s: Unimplemented feature\n", ElapsedTimeStr().c_str());
    }
    printf("%s: THIS APP WILL CRASH!\n", ElapsedTimeStr().c_str());
    *(int *)0 = 0; // to crash!
}


long get_time_in_ms() {
#ifdef _WIN32
    return (long)GetTickCount();
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
#endif
}

std::string FormatTimeStr(unsigned long uTimeMs)
{
    char buff[64];
    sprintf(buff, "%2ld:%02ld:%03ld",
        (uTimeMs/60000), (uTimeMs/1000) % 60, uTimeMs % 1000);
    return buff;
}

static unsigned long g_dwStart = get_time_in_ms();
std::string ElapsedTimeStr() {
    return FormatTimeStr(get_time_in_ms() - g_dwStart);
}

} // end of namespace hdb
