#ifndef _HDB_UTILS_H
#define _HDB_UTILS_H

#include <memory.h>
#include <assert.h>
#ifdef _WIN32
#include <windows.h> // required by sqlext.h for WIN32
#endif
#include <sqlext.h>
#include <time.h>
#include <vector>
#include <string>
#include "hdb_types.h"
#include "hdb_columns.h"
#include "hdb_odbc.h"

namespace hdb {

typedef struct {
    std::string create_sql;
    bool column;
    std::string schema;
    std::string table_name;
    std::vector<std::string> col_strs;
    std::vector<std::string> col_names;
    std::vector<DATA_TYPE_T> col_types;
    std::vector<std::string> col_str_types;
    std::vector<DATA_ATTR_T> col_attrs;
} PARSED_TABLE_T;

void GetCurTm(struct tm &stm);
void GetCurTimestamp(SQL_TIMESTAMP_STRUCT &st);
void GetCurDate(SQL_DATE_STRUCT &date);
void GetCurTime(SQL_TIME_STRUCT &time);

std::string &TrimStr(std::string &str, const char *whitespace = " \t");
std::string &ReduceStr(std::string& str, const char *fill= " ", const char *whitespace =" \t");
void StrToUpper(std::string& str);
void StrToLower(std::string& str);
hdb::string16 StrToWStr(const std::string &str);
std::string WStrToStr(const string16 &wstr);
void ReplaceCharInStr(std::string& str, char ch1, char ch2);
void StringReplace(std::string &strBase, const std::string &strSrc, const std::string &strDes);

void CsvLinePopulate(std::vector<std::string> &record, const std::string &line, char delimiter);
bool ParseTableFromSql(const char *create_sql, PARSED_TABLE_T &table, std::string &err_str);

long get_time_in_ms();
std::string FormatTimeStr(unsigned long uTimeMs);
std::string ElapsedTimeStr();

void UnImplemented(const char *desc);

}// namespace hdb

#endif // _HDB_UTILS_H
