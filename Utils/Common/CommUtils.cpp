#ifdef _WIN32
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif


#include <assert.h>
#include <time.h>
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <fstream>
#include "CommUtils.h"


std::string GetCurrentPath()
{
    boost::filesystem::path full_path( boost::filesystem::current_path());
    return full_path.string();
}

unsigned long long GetTimeInMs64() {
#ifdef _WIN32
    return ::GetTickCount64();
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
#endif
}

void GetCurTimestamp(SQL_TIMESTAMP_STRUCT &st) {
    time_t t;
    time(&t);
    struct tm stm;
#ifdef _WIN32
    localtime_s(&stm, &t);
#else
    localtime_r(&t, &stm);
#endif

    st.year = stm.tm_year + 1900;
    st.month = stm.tm_mon + 1;
    st.day = stm.tm_mday;
    st.hour = stm.tm_hour;
    st.minute = stm.tm_min;
    st.second = stm.tm_sec;
    st.fraction = 0;
}

time_t TimestampToTime(const SQL_TIMESTAMP_STRUCT &st)
{
    struct tm sourcedate;
    memset(&sourcedate, 0, sizeof(sourcedate));

    sourcedate.tm_sec = st.second;
    sourcedate.tm_min = st.minute;
    sourcedate.tm_hour = st.hour;
    sourcedate.tm_mday = st.day;
    sourcedate.tm_mon = st.month - 1;
    sourcedate.tm_year = st.year - 1900;
    return mktime(&sourcedate);
}

bool StrToTimestamp(const std::string &s, SQL_TIMESTAMP_STRUCT &v)
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
        if (hour > 24 || minute > 60 || second > 60 || hour < 0 || minute < 0 || second < 0) {
            return false;
        }

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

bool LoadSocketLib()
{
#ifdef _WIN32
    WSADATA wsaData;
    int nResult = WSAStartup(MAKEWORD(2,2), &wsaData);

    if (NO_ERROR != nResult) {
        printf("failed to init Winsock!\n");
        return false;
    }
#endif
    return true;
}

void SleepMs(long ms)
{
    boost::this_thread::sleep( boost::posix_time::milliseconds(ms) );
}

bool SetSendTimeOutInMs(SOCKET sockfd, long timeout)
{
    int ret;
#ifdef _WIN32
    unsigned int tout = timeout;
    ret = setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout) );
#else
    struct timeval tv;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;
    ret = setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (struct timeval *)&tv, sizeof(struct timeval));
#endif
    return ret == 0;
}

bool SetRecvTimeOutInMs(SOCKET sockfd, long timeout)
{
    int ret;
#ifdef _WIN32
    unsigned int tout = timeout;
    ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout) );
#else
    struct timeval tv;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;
    ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tv, sizeof(struct timeval));
#endif
    return ret == 0;
}

bool ReadIniString(dictionary *dict, const char *section, std::string &str, std::string &err)
{
    char *value = iniparser_getstring(dict, section, NULL);
    if (NULL == value) {
        char buff[128];
        snprintf(buff, sizeof(buff), "cannot find %s in config file", section);
        err = buff;
        return false;
    }
    str = value;
    return true;
}

bool ReadIniInt(dictionary *dict, const char*section, int &value, std::string &err)
{
    value = iniparser_getint(dict, section, 0xCDCDCDCD);
    if ((unsigned int)value == 0xCDCDCDCD) {
        char buff[128];
        snprintf(buff, sizeof(buff), "cannot find %s in config file", section);
        err = buff;
        return false;
    }
    return true;
}

bool ReadIniDouble(dictionary *dict, const char *section, double &value, std::string &err)
{
    std::string str;
    if (!ReadIniString(dict, section, str, err)) {
        return false;
    }

    try {
        value = boost::lexical_cast<double>(str);
    } catch(boost::bad_lexical_cast &e) {
        err = e.what();
        return false;
    }
    return true;
}

bool GetLine(std::ifstream &fs, std::string &line)
{
    line.clear();
    do{
        if(getline(fs, line)) {
            if(fs.good() && line.empty()){
                continue;
            }
            return true;
        } else {
            return false;
        }
    } while(true);
    return false;
}


static inline
void trim_left_and_push_back(std::vector<std::string> &strs, std::string &str)
{
    const char *s = str.c_str();
    while (*s == ' ' || *s == '\t') s++;
    strs.push_back(s);
}

void CsvLinePopulate(std::vector<std::string> &record, const std::string &line, char delimiter)
{
    int linepos = 0;
    int inquotes = false;
    char c;
    int linemax = (int)line.length();
    std::string curstring;
    record.clear();

    while (line[linepos] != 0 && linepos < linemax) {

        c = line[linepos];

        if (!inquotes && curstring.length() == 0 && c == '"') {
            //beginquotechar
            inquotes = true;
        } else if (inquotes && c == '"') {
            //quotechar
            if ((linepos + 1 < linemax) && (line[linepos + 1] == '"')) {
                //encountered 2 double quotes in a row (resolves to 1 double quote)
                curstring.push_back(c);
                linepos++;
            } else {
                //endquotechar
                inquotes = false;
            }
        } else if (!inquotes && c == delimiter) {
            //end of field
            trim_left_and_push_back(record, curstring);
            curstring = "";
        } else if (!inquotes && (c == '\r' || c == '\n')) {
            trim_left_and_push_back(record, curstring);
            return;
        } else {
            curstring.push_back(c);
        }
        linepos++;
    }

    trim_left_and_push_back(record, curstring);
}

bool WriteStrToFile(const char *path, const std::string &data)
{
    std::ofstream out(path);
    if (!out.good()) return false;
    out << data;
    return true;
}

void StringReplace(std::string &strBase, const std::string &strSrc, const std::string &strDes)
{
    using namespace std;
    string::size_type pos = 0;
    string::size_type srcLen = strSrc.size();
    string::size_type desLen = strDes.size();
    pos = strBase.find(strSrc, pos); 
    while ((pos != string::npos)) {
        strBase.replace(pos, srcLen, strDes);
        pos=strBase.find(strSrc, (pos+desLen));
    }
}
