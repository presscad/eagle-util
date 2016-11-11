
#ifndef _COMMON_UTILS_H
#define _COMMON_UTILS_H

#ifdef PROJ_SIM_AFL
#   define UTIL_BEGIN_NAMESPACE namespace sim_afl { namespace util {
#   define UTIL_END_NAMESPACE }}
#   define USING_UTIL_NAMESPACE using namespace sim_afl::util;
#else
#   define UTIL_BEGIN_NAMESPACE namespace util {
#   define UTIL_END_NAMESPACE }
#   define USING_UTIL_NAMESPACE using namespace util;
#endif


#include <cstdio>
#include <string>
#include <vector>
#include <tuple>
#include <iostream>
#include <fstream>


UTIL_BEGIN_NAMESPACE

#ifdef _WIN32
typedef unsigned long long SOCKET;
#else
typedef int SOCKET;
#endif


typedef void dictionary;

typedef struct TIMESTAMP_STRUCT
{
    short           year;
    unsigned short  month;
    unsigned short  day;
    unsigned short  hour;
    unsigned short  minute;
    unsigned short  second;
    unsigned long   fraction;
} TIMESTAMP_STRUCT;

typedef struct TIME_STRUCT
{
    unsigned short  hour;
    unsigned short  minute;
    unsigned short  second;
} TIME_STRUCT;

void GetCurTimestamp(TIMESTAMP_STRUCT &st);
time_t TimestampToTime(const TIMESTAMP_STRUCT &st);
time_t GetCurTimeT();
void TimeToTimestamp(time_t tm, TIMESTAMP_STRUCT &st);
bool StrToTimestamp(const std::string &str, TIMESTAMP_STRUCT &timestamp);
void TimestampToStr(const TIMESTAMP_STRUCT &st, bool with_fraction, std::string& str);
std::string TimestampToStr(const TIMESTAMP_STRUCT &st, bool with_fraction);
std::string TimeTToStr(time_t tm);
time_t StrToTimeT(const std::string &str);
long LocalUtcTimeDiff();

bool ParseTimestamp(const char *str, TIMESTAMP_STRUCT &timestamp);
bool ParseTimestamp(const std::string &str, TIMESTAMP_STRUCT &timestamp);
bool ParseTime(const char *str, TIME_STRUCT &time);
bool ParseTime(const std::string &str, TIME_STRUCT &time);

void SleepMs(long ms);
unsigned long long GetTimeInMs64();
bool GetFileModifyTime(const char *path, time_t& mtime);
std::string GetCurrentPath();
bool FileExists(const std::string& file_path);
long long FileSize(const std::string& file_path);

// find files (including folders) using file specifier
// path_specifier: e.g., abc/*.*, tuple: (file name/dir name, is directory)
bool FindFiles(const std::string& path_specifier,
    std::vector<std::tuple<std::string, bool>>& items);
bool FindFiles(const std::string& path_specifier,
    std::vector<std::string>& filenames);
// tuple: (file name/dir name, is directory)
bool GetSubsInFolder(const std::string& folder_pathname,
    std::vector<std::tuple<std::string, bool>>& sub_items);
bool GetFilesInFolder(const std::string& folder_pathname,
    std::vector<std::string>& filenames);
bool GetPathnamesInFolder(const std::string& folder_pathname,
    std::vector<std::string>& pathnames);

bool DirExists(const std::string& pathname);
bool MakeDir(const std::string& pathname);
bool RmDir(const std::string& pathname, bool recursive);
// Remove files/folders, specifier: file specifier with *, ?
bool Rm(const std::string& specifier, bool recursive);

bool GetLine(std::istream &is, std::string &line);
bool GetLine(std::ifstream &fs, std::string &line);
void ParseCsvLine(std::vector<std::string> &strs, const std::string &line, char delimiter);
void ParseCsvLineInPlace(std::vector<char *> &pstrs, char *line, char delimiter,
    bool ignore_double_quote = false);

// return the tuple (start, end), start <= index <= end
std::vector<std::tuple<int, int>> SplitIntoSubsBlocks(int task_count, int element_count);
// n_parts: is the suggested count of the parts. the actural may be different
bool SplitBigData(char *data, size_t len, char delimiter, int n_parts,
    std::vector<char *>& blocks);
bool ReadAllFromFile(const std::string& pathname, std::string &data);
bool WriteStrToFile(const std::string& path, const std::string &data);
void StringReplace(std::string &strBase, const std::string &strSrc,
    const std::string &strDes);
void StringReplace(std::wstring &wstrBase, const std::wstring &wstrSrc,
    const std::wstring &wstrDes);
void MakeUpper(std::string &str);
void MakeLower(std::string &str);

std::wstring StrToWStr(const std::string &str);
std::string WStrToStr(const std::wstring &wstr);
#ifdef _WIN32
std::wstring utf82ws(const char *src);
std::string ws2utf8(const wchar_t *ws);
std::string ws2gb2312(const wchar_t *wstr);
std::wstring gb2312_2_ws(const char *src);
#endif

bool IsUtf8String(const std::string& str);
std::string Gb2312HanziToPinyin(const std::string& hanzi_str, bool each_first_cap);

dictionary * IniInitDict(const std::string& ini_name);
void IniFreeDict(dictionary * dict);
bool ReadIniInt(dictionary *dict, const char*section, int &value,
    std::string &err);
bool ReadIniString(dictionary *dict, const char *section, std::string &str,
    std::string &err);
bool ReadIniDouble(dictionary *dict, const char *section, double &value,
    std::string &err);
bool ReadIniInt(dictionary *dict, const std::string& section, int &value,
    std::string &err);
bool ReadIniDouble(dictionary *dict, const std::string& section, double &value,
    std::string &err);
bool ReadIniString(dictionary *dict, const std::string& section, std::string &str,
    std::string &err);

bool LoadSocketLib();
bool SetSendTimeOutInMs(SOCKET sockfd, long timeout);
bool SetRecvTimeOutInMs(SOCKET sockfd, long timeout);

UTIL_END_NAMESPACE

#endif // _COMMON_UTILS_H
