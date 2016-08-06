#ifndef _COMMON_UTILS_H
#define _COMMON_UTILS_H

#ifdef _WIN32
#define snprintf _snprintf
#endif

#include <stdio.h>
#include <string>
#include <vector>
#ifdef _WIN32
#include <WinSock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
typedef int SOCKET;
#endif
#include "sqlext.h"
extern "C" {
#include "iniparser.h"
}
#include "LogUtils.h"


void GetCurTimestamp(SQL_TIMESTAMP_STRUCT &st);
time_t TimestampToTime(const SQL_TIMESTAMP_STRUCT &st);
bool StrToTimestamp(const std::string &str, SQL_TIMESTAMP_STRUCT &timestamp);

void SleepMs(long ms);
unsigned long long GetTimeInMs64();
std::string GetCurrentPath();
bool GetLine(std::ifstream &fs, std::string &line);
void CsvLinePopulate(std::vector<std::string> &record, const std::string &line, char delimiter);
bool WriteStrToFile(const char *path, const std::string &data);
void StringReplace(std::string &strBase, const std::string &strSrc, const std::string &strDes);

bool ReadIniInt(dictionary *dict, const char*section, int &value, std::string &err);
bool ReadIniString(dictionary *dict, const char *section, std::string &str, std::string &err);
bool ReadIniDouble(dictionary *dict, const char *section, double &value, std::string &err);

bool LoadSocketLib();
bool SetSendTimeOutInMs(SOCKET sockfd, long timeout);
bool SetRecvTimeOutInMs(SOCKET sockfd, long timeout);

#endif // _COMMON_UTILS_H
