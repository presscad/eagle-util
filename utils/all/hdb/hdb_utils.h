/*----------------------------------------------------------------------*
 * Copyright(c) 2015 SAP SE. All rights reserved
 * Description : Misc HDB utilities
 *----------------------------------------------------------------------*
 * Change - History : Change history
 * Developer  Date      Description
 * I078212    20140806  Initial creation
 *----------------------------------------------------------------------*/

#ifndef _HDB_UTILS_H
#define _HDB_UTILS_H

#include <cassert>
#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <Windows.h> // required by sqlext.h for WIN32
#endif
#include <sqlext.h>
#include <ctime>
#include <vector>
#include <string>
#include "hdb_types.h"
#include "hdb_columns.h"
#include "hdb_odbc.h"
#include "common/common_utils.h"


HDB_BEGIN_NAMESPACE

struct PARSED_TABLE_T {
    std::string create_sql;
    bool column;
    std::string schema;
    std::string table_name;
    std::vector<std::string> col_strs;
    std::vector<std::string> col_names;
    std::vector<bool>        col_names_case_sensitive;
    std::vector<DATA_TYPE_T> col_types;
    std::vector<std::string> col_str_types;
    std::vector<DATA_ATTR_T> col_attrs;
};

void GetCurTm(struct tm &stm);

std::string &TrimStr(std::string &str, const char *whitespace = " \t");
std::string &ReduceStr(std::string& str, const char *fill= " ", const char *whitespace =" \t");
void StrToUpper(std::string& str);
void StrToLower(std::string& str);
std::string WStrToStr(const SQLWCHAR *wstr);
void ReplaceCharInStr(std::string& str, char ch1, char ch2);
void StringReplace(std::string &strBase, const std::string &strSrc, const std::string &strDes);

bool ParseTableFromSql(const char *create_sql, PARSED_TABLE_T &table, std::string &err_str);

std::string FormatTimeStr(unsigned long uTimeMs);
std::string ElapsedTimeStr();

HDB_END_NAMESPACE

#endif // _HDB_UTILS_H
