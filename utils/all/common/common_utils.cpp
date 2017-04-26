
#include "common_utils.h"
#include <ctime>
#include <cctype>
#include <sys/stat.h>
#include <cstring>
#include <fstream>
#include <algorithm>
#include <functional>
#if (COMM_UTIL_WITH_ZLIB == 1)
#include "zfstream.h"
#endif

#ifdef _WIN32
#  include <direct.h>
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <Windows.h>
#else
#  include <unistd.h>
#  include <sys/time.h>
#  include <dirent.h>
#  include <regex.h>
#endif

using namespace std;

#ifdef _WIN32
extern "C" WINBASEAPI ULONGLONG WINAPI GetTickCount64(void);
#endif


UTIL_BEGIN_NAMESPACE
#ifdef _WIN32
static const char SPLASH = '\\', OTHER_SPLASH = '/';
#else
static const char SPLASH = '/', OTHER_SPLASH = '\\';
#endif

string GetCurrentPath()
{
#ifdef _WIN32
    TCHAR path[MAX_PATH];
    ::GetCurrentDirectory(MAX_PATH, path);
    return path;
#else
    char cwd[1024];
    if (NULL != ::getcwd(cwd, sizeof(cwd))) {
    return cwd;
    }
    return string();
#endif
}

std::string GetCurrentProcessName()
{
    std::string name;
#ifdef _WIN32
    name = __argv[0];
    if (name.rfind(".exe") == name.length() - 4) {
        name.resize(name.length() - 4);
    }
    util::StringReplaceChar(name, '\\', '/');
#elif defined(__APPLE__) || defined(__FreeBSD__)
    name = getprogname();
#else
    name = program_invocation_name;
#endif

    auto n = name.rfind('/');
    if (n != string::npos) {
        name = name.substr(n + 1);
    }
    return name;
}

bool GetFileModifyTime(const char *path, time_t& mtime)
{
    struct stat statbuf;
    if (stat(path, &statbuf) == -1) {
        return false;
    }
    mtime = statbuf.st_mtime;
    return true;
}

bool FileExists(const string& file_path)
{
    ifstream file(file_path);
    return file.good();
}

long long FileSize(const string& file_path)
{
    ifstream file(file_path, ios::binary | ios::ate);
    return (long long)file.tellg();
}


static string WildcardsToRegex(string wildcard_pattern)
{
    // Escape all regex special chars
    StringReplace(wildcard_pattern, "\\", "\\\\");
    StringReplace(wildcard_pattern, "^", "\\^");
    StringReplace(wildcard_pattern, ".", "\\.");
    StringReplace(wildcard_pattern, "$", "\\$");
    StringReplace(wildcard_pattern, "|", "\\|");
    StringReplace(wildcard_pattern, "(", "\\(");
    StringReplace(wildcard_pattern, ")", "\\)");
    StringReplace(wildcard_pattern, "[", "\\[");
    StringReplace(wildcard_pattern, "]", "\\]");
    StringReplace(wildcard_pattern, "*", "\\*");
    StringReplace(wildcard_pattern, "+", "\\+");
    StringReplace(wildcard_pattern, "?", "\\?");
    StringReplace(wildcard_pattern, "/", "\\/");

    // Convert chars '*?' back to their regex equivalents
    StringReplace(wildcard_pattern, "\\?", ".");
    StringReplace(wildcard_pattern, "\\*", ".*");

    return wildcard_pattern;
}

static bool GetSubsInDir(string directory, const string& specifier,
    vector<tuple<string, bool>>& items)
{
    if (directory.back() == SPLASH || directory.back() == OTHER_SPLASH) {
        directory.pop_back();
    }

#ifdef _WIN32
    // specifier is natively supported on Windows
    WIN32_FIND_DATA data;
    HANDLE h = ::FindFirstFileA((directory + SPLASH + specifier).c_str(), &data);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (strcmp(".", data.cFileName) == 0 || strcmp("..", data.cFileName) == 0) {
                continue;
            }
            bool is_dir = (data.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY);
            items.push_back(make_tuple(data.cFileName, is_dir));
        } while (::FindNextFileA(h, &data));
        ::FindClose(h);
    }
    else {
        return false;
    }
#else
    DIR *dir;
    class dirent *ent;
    class stat st;

    regex_t re;
    const bool with_wildcards = specifier.find('*') != string::npos ||
        specifier.find('?') != string::npos;
    if (with_wildcards) {
        if (regcomp(&re, WildcardsToRegex(specifier).c_str(), REG_EXTENDED | REG_NOSUB) != 0) {
            return false;
        }
    }

    dir = opendir(directory.c_str());
    if (NULL == dir) {
        regfree(&re);
        return false;
    }
    string file_name, full_file_name;
    while ((ent = readdir(dir)) != NULL) {
        file_name = ent->d_name;
        full_file_name = directory + SPLASH + file_name;

        if (file_name.front() == '.') {
            continue;
        }
        if (stat(full_file_name.c_str(), &st) == -1) {
            continue;
        }
        const bool is_dir = (st.st_mode & S_IFDIR) != 0;

        if (with_wildcards) {
            int status = regexec(&re, file_name.c_str(), (size_t)0, NULL, 0);
            if (status == 0) {
                items.push_back(make_tuple(file_name, is_dir));
            }
        }
        else {
            items.push_back(make_tuple(file_name, is_dir));
            // no need to continue the loop
            break;
        }
    }
    regfree(&re);
    closedir(dir);
#endif

    return true;
}

// path_specifier: e.g., abc/*.*, tuple: (file name/dir name, is directory)
bool FindFiles(const string& path_specifier, vector<tuple<string, bool>>& items)
{
    items.clear();
    auto path_spec = path_specifier;
    if (path_spec == "*" || path_spec == "*.*") {
        return GetSubsInDir(".", path_spec, items);
    }
    if (path_spec == ".") {
        return GetSubsInDir(path_spec, "*.*", items);
    }

    // get all the sub files/dirs in the folder
    util::StringReplaceChar(path_spec, OTHER_SPLASH, SPLASH);
    auto f = path_spec.rfind(SPLASH);
    string directory, specifier;
    if (f == string::npos) {
        directory = ".";
        specifier = path_spec;
    }
    else {
        directory = path_spec.substr(0, f);
        specifier = path_spec.substr(f + 1);
    }
    return GetSubsInDir(directory, specifier, items);
}

bool FindFiles(const string& path_specifier, vector<string>& filenames)
{
    vector<tuple<string, bool>> sub_items;
    if (false == FindFiles(path_specifier, sub_items)) {
        return false;
    }

    filenames.clear();
    for (const auto& item : sub_items) {
        if (get<1>(item) == false) {
            filenames.push_back(get<0>(item));
        }
    }
    return true;
}

bool GetSubsInFolder(const string& pathname,
    vector<tuple<string, bool>>& sub_items)
{
    if (pathname.empty() || pathname.back() == SPLASH || pathname.back() == OTHER_SPLASH) {
        return FindFiles(pathname + "*.*", sub_items);
    }
    else {
        return FindFiles(pathname + "/*.*", sub_items);
    }
}

bool GetFilesInFolder(const string& pathname, vector<string>& filenames)
{
    vector<tuple<string, bool>> sub_items;
    if (false == GetSubsInFolder(pathname, sub_items)) {
        return false;
    }

    filenames.clear();
    for (const auto& item : sub_items) {
        if (get<1>(item) == false) {
            filenames.push_back(get<0>(item));
        }
    }
    return true;
}

bool GetPathnamesInFolder(const string& folder_pathname,
    vector<string>& pathnames)
{
    vector<string> names;
    if (false == GetFilesInFolder(folder_pathname, names)) {
        return false;
    }

    const char *p = strrchr(folder_pathname.c_str(), SPLASH);
    if (!p) {
        pathnames.swap(names);
    }
    else {
        string path(folder_pathname);
        path.resize(p - folder_pathname.c_str() + 1);

        pathnames.clear();
        pathnames.reserve(names.size());
        for (size_t i = 0; i < names.size(); ++i) {
            pathnames.push_back(path + names[i]);
        }
    }
    return true;
}

bool DirExists(const string& pathname)
{
    struct stat info;
    if (stat(pathname.c_str(), &info) != 0) {
        return false;
    }
    return (info.st_mode & S_IFDIR) ? true : false;
}

bool MakeDir(const string& pathname)
{
#ifdef _WIN32
    return 0 == _mkdir(pathname.c_str());
#else
    return mkdir(pathname.c_str(), 0);
#endif
}

bool RmDir(const string& pathname, bool recursive)
{
    if (recursive) {
        vector<tuple<string, bool>> sub_items;
        if (false == GetSubsInFolder(pathname, sub_items)) {
            return false;
        }

        for (const auto& item : sub_items) {
            string sub_name(pathname + '/');
            sub_name += get<0>(item);

            if (get<1>(item) == true) {
                // sub folder
                RmDir(sub_name, true);
            }
            else {
                // is file
                if (0 != ::remove(sub_name.c_str())) {
                    return false;
                }
            }
        }
    }

#ifdef _WIN32
    return 0 == _rmdir(pathname.c_str());
#else
    return rmdir(pathname.c_str());
#endif
}

// Remove files/folders, specifier: file specifier with *, ?
bool Rm(const string& specifier, bool recursive)
{
    vector<tuple<string, bool>> sub_items;
    if (false == FindFiles(specifier, sub_items)) {
        return false;
    }
    if (sub_items.empty()) {
        return true;
    }

    // get the path part. e.g., "abc/def*.*" => "abc/"
    auto pos = specifier.rfind(SPLASH);
    if (pos == string::npos) {
        pos = specifier.rfind(OTHER_SPLASH);
    }
    string path;
    if (pos != string::npos) {
        path = specifier.substr(0, pos + 1);
    }
    else {
        path += '.';
        path += SPLASH;
    }

    for (const auto& item : sub_items) {
        string sub_name(path);
        sub_name += get<0>(item);

        if (get<1>(item) == true) {
            // sub folder
            RmDir(sub_name, recursive);
        }
        else {
            // is file
            if (0 != ::remove(sub_name.c_str())) {
                return false;
            }
        }
    }
    return true;
}

void SleepMs(long ms)
{
#ifdef _WIN32
    ::Sleep(ms);
#else
    ::usleep((unsigned long)ms * 1000);
#endif
}

unsigned long long GetTimeInMs64()
{
#ifdef _WIN32
    SYSTEMTIME now;
    GetLocalTime(&now);
    return static_cast<uint64_t>(now.wSecond) * 1000 + now.wMilliseconds;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
#endif
}

void GetCurTimestamp(TIMESTAMP_STRUCT &st)
{
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

TIMESTAMP_STRUCT GetCurTimestamp()
{
    TIMESTAMP_STRUCT ts;
    GetCurTimestamp(ts);
    return ts;
}

string GetCurTimestampStr(bool with_fraction)
{
    TIMESTAMP_STRUCT ts;
    GetCurTimestamp(ts);
    return TimestampToStr(ts, with_fraction);
}

void TimeToTimestamp(time_t tt, TIMESTAMP_STRUCT &ts)
{
    struct tm stm;
    TimeToTM(tt, stm);

    ts.year = stm.tm_year + 1900;
    ts.month = stm.tm_mon + 1;
    ts.day = stm.tm_mday;
    ts.hour = stm.tm_hour;
    ts.minute = stm.tm_min;
    ts.second = stm.tm_sec;
    ts.fraction = 0;
}

void TimeToTM(time_t tt, tm& stm)
{
#ifdef _WIN32
    localtime_s(&stm, &tt);
#else
    localtime_r(&tt, &stm);
#endif
}

tm TimeToTM(time_t tt)
{
    tm stm;
    TimeToTM(tt, stm);
    return stm;
}

time_t TimestampToTime(const TIMESTAMP_STRUCT &st)
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

time_t GetCurTimeT()
{
    TIMESTAMP_STRUCT tm_struct;
    GetCurTimestamp(tm_struct);
    return TimestampToTime(tm_struct);
}

string& TimestampToStr(const TIMESTAMP_STRUCT &st, bool with_fraction, string& str)
{
    char buff[128];
    if (with_fraction) {
        snprintf(buff, sizeof(buff) - 1, "%04d-%02u-%02u %02u:%02u:%02u.%06lu",
            st.year, st.month, st.day, st.hour, st.minute, st.second, st.fraction);
    }
    else {
        snprintf(buff, sizeof(buff) - 1, "%04d-%02u-%02u %02u:%02u:%02u",
            st.year, st.month, st.day, st.hour, st.minute, st.second);
    }
    str = buff;
    return str;
}

string TimestampToStr(const TIMESTAMP_STRUCT &st, bool with_fraction)
{
    string str;
    return TimestampToStr(st, with_fraction, str);
}

bool StrToTimestamp(const string &s, TIMESTAMP_STRUCT &v)
{
    if (s.empty()) return false;

    int year, month, day, hour, minute, second, fraction;
    int r = sscanf(s.c_str(), "%d-%d-%d%*[T -]%d%*[:.]%d%*[:.]%d%*[:.]%d",
        &year, &month, &day, &hour, &minute, &second, &fraction);
    if (r == 3 || r == 5 || r == 6 || r == 7) {
        if (r == 3) {
            hour = minute = second = 0;
            fraction = 0;
        }
        else if (r == 5) {
            second = fraction = 0;
        }
        else if (r == 6) {
            fraction = 0;
        }

        if (year <= 0 || year > 3000) return false;
        if (month < 0 || month > 12) {
            return false;
        }
        else if (month == 0) {
            month = 1;
        }
        if (day < 0 || day > 31) {
            return false;
        }
        else if (day == 0) {
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

string TimeTToStr(time_t tm)
{
    string str;
    return TimeTToStr(tm, str);
}

string& TimeTToStr(time_t tm, string& str)
{
    util::TIMESTAMP_STRUCT ts;
    util::TimeToTimestamp(tm, ts);

    char buff[128];
    snprintf(buff, sizeof(buff) - 1, "%04d-%02u-%02u %02u:%02u:%02u",
        ts.year, ts.month, ts.day, ts.hour, ts.minute, ts.second);
    str = buff;
    return str;
}

time_t StrToTimeT(const string &str)
{
    TIMESTAMP_STRUCT timestamp;
    if (false == StrToTimestamp(str, timestamp)) {
        return 0;
    }
    return TimestampToTime(timestamp);
}

long LocalUtcTimeDiff()
{
    time_t secs;
    time(&secs);  // Current time in GMT

#ifdef _WIN32
    struct tm tm_data {};
    struct tm *tptr = &tm_data;
    localtime_s(tptr, &secs);
#else
    struct tm *tptr = localtime(&secs);
#endif

    time_t local_secs = mktime(tptr);
#ifdef _WIN32
    gmtime_s(tptr, &secs);
#else
    tptr = gmtime(&secs);
#endif
    time_t gmt_secs = mktime(tptr);
    long diff_secs = long(local_secs - gmt_secs);
    return diff_secs;
}


static const char daysOfMonth[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
static inline bool isLeapYearAbap(int year)
{
    // gregorian calculator only since 1582:
    return year % 4 == 0 && (year <= 1582 || year % 100 != 0 || year % 400 == 0);
}

bool ParseTimestamp(const string &s, TIMESTAMP_STRUCT &timestamp)
{
    return ParseTimestamp(s.c_str(), timestamp);
}

bool ParseTimestamp(const char* s, TIMESTAMP_STRUCT& timestamp, DatePrecision& precision)
{
    // for ESP, the time format could be like "2015-05-16T00:16:49" ...
    if (s[10] == 'T') {
        ((char*)s)[10] = ' ';
    }

    auto& dr = timestamp;

    // count digits
    // decide wether to read delimited or non delimited format
    // if we have '.', try 24.12., 24.12.2009
    // year has 4 digits
    // month, day, hour, minute, second have up to two digits, each
    // in delimited format, if month is zero, all the rest should be zero
    // we allow leading whitespace (for the sign?), but no trailing whitespace

    dr.year = 0;
    dr.month = 0;
    dr.day = 0;
    dr.hour = 0;
    dr.minute = 0;
    dr.second = 0;
    precision = PRECISION_YEAR;

    const char *p = s;
    while (*p == ' ')
        ++p;
    bool isNegative = *p == '-';
    if (isNegative)
        ++p;
    const char *p0 = p;
    if (*p >= '0' && *p <= '9') {
        dr.year = *p - '0';
        do {
            ++p;
        } while (*p >= '0' && *p <= '9');
    }
    else if (!*p) {
        precision = PRECISION_NULL;
        return true;
    }
    else
        return false;
    unsigned digits = (unsigned)(p - p0);
    if (*p) {
        if (*p == '-' || *p == '/') {
            for (const char *p1 = p0 + 1; p1 < p; ++p1)
                dr.year = 10 * dr.year + (*p1 - '0');
            if ((unsigned)(dr.month = p[1] - '0') > 9)
                return false;
            p += 2;
            if (*p >= '0' && *p <= '9') {
                dr.month = 10 * dr.month + (*p++ - '0');
                if (dr.month > 12)
                    return false;
            }
            if (dr.month == 0) {
                if (dr.year == 0 && dr.month == 0) {
                    if ((*p == '-' || *p == '/') && p[1] == '0') {
                        p += p[2] == '0' ? 3 : 2;
                        if (*p == ' ' && p[1] == '0') {
                            p += p[2] == '0' ? 3 : 2;
                            if (*p == ':' && p[1] == '0') {
                                p += p[2] == '0' ? 3 : 2;
                                if (*p == ':' && p[1] == '0') {
                                    p += p[2] == '0' ? 3 : 2;
                                    if ((*p == '.' || *p == ',') && p[1] == '0')
                                        for (p += 2; *p == '0'; ++p)
                                            ;
                                }
                            }
                        }
                    }
                    return *p ? false : true;
                }
                else
                    return false;
            }
            precision = PRECISION_MONTH;
            if (*p == '-' || *p == '/') {
                precision = PRECISION_DAY;
                if ((unsigned)(dr.day = p[1] - '0') > 9)
                    return false;
                p += 2;
                if (*p >= '0' && *p <= '9')
                    dr.day = 10 * dr.day + (*p++ - '0');
                if ((unsigned char)(dr.day - 1) >= daysOfMonth[dr.month])
                    if (dr.month != 2 || dr.day != 29 || !isLeapYearAbap(dr.year))
                        return false;
            }
            else if (*p)
                return false;
            else
                goto notime;
        }
        else if (*p == '.') {
            if (isNegative)
                return false;
            if (digits == 14)
                goto digitonly;
            else if (digits == 1 && dr.year == 0) {
                if (*p == '.') {
                    ++p;
                    while (*p == '0')
                        ++p;
                }
                if (*p)
                    return false;
                return true;
            }
            else {
                precision = PRECISION_DAY;
                dr.day = dr.year;
                if (digits == 2)
                    dr.day = 10 * dr.day + (p0[1] - '0');
                else if (digits != 1)
                    return false;
                if ((unsigned)(dr.month = p[1] - '0') > 9)
                    return false;
                p += 2;
                if (*p >= '0' && *p <= '9')
                    dr.month = 10 * dr.month + (*p++ - '0');
                if (dr.month < 1 || dr.month > 12 || *p++ != '.')
                    return false;
                if ((unsigned)(dr.year = *p++ - '0') > 9)
                    return false;
                while (*p >= '0' && *p <= '9')
                    if ((dr.year = 10 * dr.year + (*p++ - '0')) > 99999)
                        return false;
                if ((unsigned char)(dr.day - 1) >= daysOfMonth[dr.month])
                    if (dr.month != 2 || dr.day != 29 || !isLeapYearAbap(dr.year))
                        return false;
            }
        }
        else
            return false;
        if (*p) {
            if (*p == ' ' || *p == 'T' || *p == 't')
                ++p;
            else if (*p == ',') {
                ++p;
                if (*p == ' ')
                    ++p;
            }
            else
                return false;
            precision = PRECISION_HOUR;
            if ((unsigned)(dr.hour = *p++ - '0') > 9)
                return false;
            if (*p >= '0' && *p <= '9')
                dr.hour = 10 * dr.hour + (*p++ - '0');
            if (*p == ':') {
                precision = PRECISION_MINUTE;
                if ((unsigned)(dr.minute = p[1] - '0') > 9)
                    return false;
                p += 2;
                if (*p >= '0' && *p <= '9') {
                    dr.minute = 10 * dr.minute + (*p++ - '0');
                    if (dr.minute > 59)
                        return false;
                }
                if (*p == ':') {
                    precision = PRECISION_SECOND;
                    if ((unsigned)(dr.second = p[1] - '0') > 9)
                        return false;
                    p += 2;
                    if (*p >= '0' && *p <= '9') {
                        dr.second = 10 * dr.second + (*p++ - '0');
                        if (dr.second > 59)
                            return false;
                    }
                }
            }
        }
    notime:
        ;
    }
    else if (digits >= 6) { // year, month:
    digitonly:
        if (digits > 14 || (digits & 1) != 0)
            return false;
        dr.year = 1000 * dr.year + 100 * (p0[1] - '0') + 10 * (p0[2] - '0') + (p0[3] - '0');
        dr.month = 10 * (p0[4] - '0') + (p0[5] - '0');
        if (dr.month == 0) {
            for (p0 += 6; p0 < p; ++p0)
                if (*p0 != '0')
                    return false;
        }
        else {
            if (dr.month > 12)
                return false;
            precision = PRECISION_MONTH;
            if (digits >= 8) {
                precision = PRECISION_DAY;
                dr.day = 10 * (p0[6] - '0') + (p0[7] - '0');
                if ((unsigned char)(dr.day - 1) >= daysOfMonth[dr.month])
                    if (dr.month != 2 || dr.day != 29 || !isLeapYearAbap(dr.year))
                        return false;
                if (digits >= 10) {
                    precision = PRECISION_HOUR;
                    dr.hour = 10 * (p0[8] - '0') + (p0[9] - '0');
                    if (digits >= 12) {
                        precision = PRECISION_MINUTE;
                        if (p0[10] > '5')
                            return false;
                        dr.minute = 10 * (p0[10] - '0') + (p0[11] - '0');
                        if (digits == 14) { //
                            precision = PRECISION_SECOND;
                            if (p0[12] > '5')
                                return false;
                            dr.second = 10 * (p0[12] - '0') + (p0[13] - '0');
                        }
                    }
                }
            }
        }
    }
    else {
        for (const char *p1 = p0 + 1; p1 < p; ++p1)
            dr.year = 10 * dr.year + (*p1 - '0');
        goto nomore;
    }
    if ((*p == '.' || *p == ',')
        && precision == PRECISION_SECOND) {
        precision = PRECISION_MSEC;
        if (p[1] >= '0' && p[1] <= '9') {
            dr.fraction = 1000000 * (p[1] - '0');
            if (p[2] >= '0' && p[2] <= '9') {
                dr.fraction += 100000 * (p[2] - '0');
                if (p[3] >= '0' && p[3] <= '9') {
                    dr.fraction += 10000 * (p[3] - '0');
                    if (p[4] >= '0' && p[4] <= '9') {
                        precision = PRECISION_USEC;
                        dr.fraction += 1000 * (p[4] - '0');
                        if (p[5] >= '0' && p[5] <= '9') {
                            dr.fraction += 100 * (p[5] - '0');
                            if (p[6] >= '0' && p[6] <= '9') {
                                dr.fraction += 10 * (p[6] - '0');
                                if (p[7] >= '0' && p[7] <= '9') {
                                    dr.fraction += p[7] - '0';
                                    precision = PRECISION_NANO100;
                                    p += 8;
                                }
                                else
                                    p += 7;
                            }
                            else
                                p += 6;
                        }
                        else
                            p += 5;
                    }
                    else
                        p += 4;
                }
                else
                    p += 3;
            }
            else
                p += 2;
        }
        else
            ++p;
    }
    if (precision >= PRECISION_HOUR) {
        const char *p1 = p;
        if (*p1 == ' ')
            ++p1;
        if (*p1 == 'p' || *p1 == 'P') {
            if (dr.hour < 1)
                return false;
            else if (dr.hour < 12)
                dr.hour += 12;
            else if (dr.hour > 12)
                return false;
        }
        else if (*p1 == 'a' || *p1 == 'A') {
            if (dr.hour < 1)
                return false;
            else if (dr.hour == 12)
                dr.hour = 0;
            else if (dr.hour > 12)
                return false;
        }
        else
            goto nous;
        if (p1[1] != 'm' && p1[1] != 'M')
            return false;
        p = p1 + 2;
    nous:
        ;
    }
nomore:
    if (isNegative)
        dr.year = -dr.year;
    if (dr.hour >= 24) {
        if (dr.hour > 24 || dr.minute != 0 || dr.second != 0)
            return false;
        dr.hour = 0;
        // dr.addDays(1);
        time_t tt = TimestampToTime(dr);
        TimeToTimestamp(tt + 24 * 60 * 26, dr);
    }
    return true;
}

bool ParseTimestamp(const char *s, TIMESTAMP_STRUCT &timestamp)
{
    DatePrecision precision;
    return ParseTimestamp(s, timestamp, precision);
}

bool ParseTime(const string &s, TIME_STRUCT &time_struct)
{
    return ParseTime(s.c_str(), time_struct);
}

bool ParseTime(const char *s, TIME_STRUCT &time_struct)
{
    DatePrecision precision;
    return ParseTime(s, time_struct, precision);
}

bool ParseTime(const char *s, TIME_STRUCT &time_struct, DatePrecision& precision)
{
    time_struct.hour = time_struct.minute = time_struct.second = 0;
    TIMESTAMP_STRUCT dr;

    // count digits
    // decide wether to read delimited or non delimited format
    // hour has up to 5 digits
    // minute, second have one or two digits, each
    // we allow leading whitespace (for the sign?), but no trailing whitespace

    dr.year = 0;
    dr.month = 0;
    dr.day = 0;
    dr.hour = 0;
    dr.minute = 0;
    dr.second = 0;
    dr.fraction = 0;
    precision = PRECISION_HOUR;

    const char *p = s;
    while (*p == ' ')
        ++p;
    bool isNegative = *p == '-';
    if (isNegative)
        ++p;
    const char *p0 = p;
    if (*p >= '0' && *p <= '9') {
        dr.hour = *p - '0';
        do {
            ++p;
        } while (*p >= '0' && *p <= '9');
    }
    else if (!*p) {
        precision = PRECISION_NULL;
        return true;
    }
    else
        return false;
    if (*p) {
        if (*p != ':' || p - p0 > 6)
            return false;
        for (const char *p1 = p0 + 1; p1 != p; ++p1)
            dr.hour = 10 * dr.hour + (*p1 - '0');
        if (*++p) {
            precision = PRECISION_MINUTE;
            if ((unsigned)(dr.minute = *p++ - '0') > 9)
                return false;
            if (*p >= '0' && *p <= '9') {
                dr.minute = 10 * dr.minute + (*p++ - '0');
                if (dr.minute > 59)
                    return false;
            }
            if (*p) {
                if (*p++ != ':')
                    return false;
                precision = PRECISION_SECOND;
                if ((unsigned)(dr.second = *p++ - '0') > 9)
                    return false;
                if (*p >= '0' && *p <= '9') {
                    dr.second = 10 * dr.second + (*p++ - '0');
                    if (dr.second > 59)
                        return false;
                }
                // ignore subseconds:
                if (*p == '.' || *p == ',')
                    for (++p; *p >= '0' && *p <= '9'; ++p)
                        ;
                // accept am and pm:
                const char *p1 = p;
                if (*p1 == ' ')
                    ++p1;
                if (*p1 == 'p' || *p1 == 'P') {
                    if (dr.hour < 1)
                        return false;
                    else if (dr.hour < 12)
                        dr.hour += 12;
                    else if (dr.hour > 12)
                        return false;
                }
                else if (*p1 == 'a' || *p1 == 'A') {
                    if (dr.hour < 1)
                        return false;
                    else if (dr.hour == 12)
                        dr.hour = 0;
                    else if (dr.hour > 12)
                        return false;
                }
                else
                    goto nous;
                if (p1[1] != 'm' && p1[1] != 'M')
                    return false;
                p = p1 + 2;
                if (isNegative /*|| dr.hour < 0*/ ||
                    (dr.hour > 23 && !(dr.hour == 24 && dr.minute == 0 && dr.second == 0)))
                    return false;
            nous:
                if (*p)
                    return false;
            }
        }
    }
    else if (!isNegative)
        if (p - p0 == 4) {
            precision = PRECISION_MINUTE;
            dr.hour = 10 * dr.hour + (p0[1] - '0');
            dr.minute = 10 * (p0[2] - '0') + (p0[3] - '0');
            if (dr.minute > 59)
                return false;
        }
        else if (p - p0 == 6) {
            precision = PRECISION_SECOND;
            dr.hour = 10 * dr.hour + (p0[1] - '0');
            dr.minute = 10 * (p0[2] - '0') + (p0[3] - '0');
            dr.second = 10 * (p0[4] - '0') + (p0[5] - '0');
            if (dr.minute > 59 || dr.second > 59)
                return false;
        }
        else
            goto allyear;
    else {
    allyear:
        if (p - p0 <= 6)
            for (const char *p1 = p0 + 1; p1 != p; ++p1)
                dr.hour = 10 * dr.hour + (*p1 - '0');
        else
            return false;
    }
    if (isNegative) {
        dr.hour = -dr.hour;
        dr.minute = -dr.minute;
        dr.second = -dr.second;
    }

    time_struct.hour = dr.hour;
    time_struct.minute = dr.minute;
    time_struct.second = dr.second;
    return true;
}

void MakeUpper(string &str)
{
    transform(str.begin(), str.end(), str.begin(), ::toupper);
}

void MakeLower(string &str)
{
    transform(str.begin(), str.end(), str.begin(), ::tolower);
}

static inline bool not_space(char ch)
{
    switch (ch) {
    case ' ':
    case '\t':
    case '\n':
    case '\v':
    case '\f':
    case '\r':
        return false;
    default:
        return true;
    }
}

// trim from start
static inline string &ltrim(string &s) {
    s.erase(s.begin(), find_if(s.begin(), s.end(), not_space));
    return s;
}

// trim from end
static inline string &rtrim(string &s) {
    s.erase(find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
    return s;
}

string& LeftTrimString(string& str)
{
    return ltrim(str);
}

string& RightTrimString(string& str)
{
    return rtrim(str);
}

string& TrimString(string& str)
{
    return ltrim(rtrim(str));
}

// this version of read_line handles mixed dos/unix '\n'
bool GetLine(istream& is, string& line)
{
    line.clear();

    istream::sentry se(is, true);
    streambuf* sb = is.rdbuf();

    for (;;) {
        int c = sb->sbumpc();
        switch (c) {
        case '\n': {
            return true;
        }
        case '\r':
            if (sb->sgetc() == '\n') {
                sb->sbumpc();
            }
            return true;
        case EOF:
            // Also handle the case when the last line has no line ending
            if (line.empty()) {
                is.setstate(ios::eofbit);
            }
            return !line.empty(); // because of EOF
        default:
            line += (char)c;
        }
    }
    return true;
}

bool GetLine(ifstream &fs, string &line)
{
    istream &is = fs;
    return GetLine(is, line);
}

void ParseCsvLine(vector<string> &sub_strs, const string& line, char delimiter)
{
    ParseCsvLine(sub_strs, line.c_str(), delimiter);
}

// Optimized version
void ParseCsvLine(vector<string>& sub_strs, const char* line, char delimiter)
{
    if (nullptr == line || '\0' == line[0]) {
        sub_strs.clear();
        return;
    }

    int linepos = 0;
    bool inquotes = false;
    int linemax = (int)strlen(line);

    char *curstring;
    char local_buff[4096];
    if (linemax < (int)sizeof(local_buff)) {
        curstring = local_buff;
    }
    else {
        curstring = new char[linemax + 4];
    }
    int cur_cur = 0;
    int rec_num = 0, rec_size = (int)sub_strs.size();

    int i_quote_end = -1; // index of end of the quote in the curstring
    auto trim_and_push_back = [&sub_strs, &rec_num, &rec_size, &i_quote_end](const char* s) {
        // no need to trim the whitespace at begin, it was done outside of this function

        if (rec_num >= rec_size) {
            sub_strs.push_back(s);
            rec_size++;
        }
        else {
            sub_strs[rec_num] = s;
        }

        auto& str = sub_strs[rec_num];
        if (i_quote_end >= 0) { // ending qoute found
            while ((str.size() > (size_t)i_quote_end + 1) &&
                (str.back() == ' ' || str.back() == '\t')) {
                str.pop_back();
            }
        }
        else {
            while (!str.empty() && (str.back() == ' ' || str.back() == '\t')) {
                str.pop_back();
            }
        }

        rec_num++;
        i_quote_end = -1;
    };

    // skip the white spaces at the line begin
    while (linepos < linemax && (line[linepos] == ' ' || line[linepos] == '\t')) {
        linepos++;
    }

    while (linepos < linemax) {
        const auto c = line[linepos];

        if (!inquotes && cur_cur == 0 && c == '"') {
            //beginquotechar
            inquotes = true;
        }
        else if (inquotes && c == '"') {
            //quotechar
            if ((linepos + 1 < linemax) && (line[linepos + 1] == '"'))  {
                //encountered 2 double quotes in a row (resolves to 1 double quote)
                curstring[cur_cur++] = c;
                linepos++;
            }
            else {
                inquotes = false; //endquotechar
                i_quote_end = cur_cur;
            }
        }
        else if (!inquotes && c == delimiter) {
            //end of field
            curstring[cur_cur] = '\0';
            trim_and_push_back(curstring);
            cur_cur = 0;

            // found space before quote, e.g., '1, "with quote", 3'
            while (linepos + 1 < linemax &&
                (line[linepos + 1] == ' ' || line[linepos + 1] == '\t')) {
                linepos++;
            }
        }
        else if (!inquotes && (c == '\r' || c == '\n')) {
            break;
        }
        else {
            curstring[cur_cur++] = c;
        }
        linepos++;
    }

    // for the last sub-string
    curstring[cur_cur] = '\0';
    trim_and_push_back(curstring);

    if (rec_size > rec_num) {
        sub_strs.resize(rec_num);
    }
    if (curstring != local_buff) {
        delete[] curstring;
    }
    return;
}

void ParseCsvLineInPlace(vector<char *> &strs, char *line, char delimiter,
    bool ignore_double_quote)
{
    strs.clear();
    if (!line || line[0] == '\0') {
        return;
    }

    int linepos = 0;
    bool inquotes = false;
    int linemax = (int)strlen(line);
    char *curstring = line;
    int curpos = 0;

    int i_quote_end = -1; // index of end of the quote in the curstring
    auto trim_and_push_back = [&strs, &i_quote_end](char* s, int s_len) {
        // no need to trim the whitespace at begin, it was done outside of this function
        strs.push_back(s);

        auto& str = strs.back();
        if (i_quote_end >= 0) { // ending qoute found
            // trim end white spaces after ending quote
            while ((s_len > i_quote_end + 1) &&
                (str[s_len - 1] == ' ' || str[s_len - 1] == '\t')) {
                s_len--;
                str[s_len] = '\0';
            }
            if (str[i_quote_end] != '\0') {
                str[i_quote_end] = '\0';
            }
        }
        else {
            // trim end white spaces
            while ((s_len != 0) && (str[s_len - 1] == ' ' || str[s_len - 1] == '\t')) {
                s_len--;
                str[s_len] = '\0';
            }
        }

        i_quote_end = -1;
    };

    // skip the white spaces at the line begin
    while (linepos < linemax && (line[linepos] == ' ' || line[linepos] == '\t')) {
        linepos++;
        curstring++;
    }

    while (linepos < linemax && line[linepos] != 0) {
        const auto c = line[linepos];

        if (!ignore_double_quote && !inquotes && curpos == 0 && c == '"') {
            //beginquotechar
            inquotes = true;
            curstring++;
        }
        else if (inquotes && c == '"') {
            //quotechar
            if ((linepos + 1 < linemax) && (line[linepos + 1] == '"')) {
                //encountered 2 double quotes in a row (resolves to 1 double quote)
                curpos++;
                linepos++;
            }
            else {
                //endquotechar
                inquotes = false;
                i_quote_end = curpos++;
            }
        }
        else if (!inquotes && c == delimiter) {
            //end of field
            line[linepos] = '\0';
            trim_and_push_back(curstring, curpos);
            curpos = 0;

            // found space before quote, e.g., '1, "with quote", 3'
            while (linepos + 1 < linemax &&
                (line[linepos + 1] == ' ' || line[linepos + 1] == '\t')) {
                linepos++;
            }
            curstring = &line[linepos + 1];
        }
        // if delimiter is '\n', do not check line end
        else if ((delimiter != '\n') && !inquotes && (c == '\r' || c == '\n')) {
            line[linepos] = '\0';
            trim_and_push_back(curstring, curpos);
            return;
        }
        else {
            curpos++;
        }

        linepos++;
    }

    trim_and_push_back(curstring, curpos);
}

vector<string> StringSplit(const string &line, char delimiter)
{
    vector<string> strs;
    ParseCsvLine(strs, line, delimiter);
    return strs;
}

vector<char*> StringSplitInPlace(string& line, char delimiter)
{
    vector<char*> strs;
    ParseCsvLineInPlace(strs, (char *)line.c_str(), delimiter);
    return strs;
}

// return the tuple (start, end), start <= index <= end
vector<tuple<int, int>> SplitIntoSubsBlocks(int task_count, int element_count)
{
    vector<tuple<int, int>> blocks(task_count);

    const int sub_width = element_count / task_count;
    for (int i = 0; i < task_count; ++i) {
        blocks[i] = make_tuple(sub_width * i, sub_width * (i + 1) - 1);
    }
    get<1>(blocks.back()) = element_count - 1;

    return blocks;
}

// preconditions
//  - data is not NULL
//  - delimiter must be existing between blocks
bool SplitBigData(char *data, size_t len, char delimiter, int n_blocks,
    vector<char *>& blocks)
{
    if (n_blocks == 0) {
        return false;
    }

    blocks.clear();
    if (n_blocks == 1 || len < 50 * 1024) {
        blocks.push_back(data);
    }
    else {
        const size_t block_size = len / n_blocks;
        const char *p_end = &data[len];
        blocks.push_back(data);

        for (int i = 1; i < n_blocks; ++i) {
            char *p_block = data + block_size * i;
            const char *p_next_block = p_block + block_size;
            const char *limit = p_next_block < p_end ? p_next_block : p_end;

            while (*p_block != delimiter && p_block < limit) {
                ++p_block;
            }
            if (p_block >= p_end) {
                break;
            }
            else if (p_block >= limit) {
                continue;
            }
            else {
                *p_block = '\0';
                blocks.push_back(p_block + 1);
            }
        }

        // remove duplicated blocks
        for (int i = (int)blocks.size() - 1; i >= 1; --i) {
            if (blocks[i] == blocks[i - 1]) {
                blocks.erase(blocks.begin() + i);
            }
        }
    }

    return !blocks.empty();
}

bool ReadAllFromFile(const string& pathname, string &data)
{
    long long file_size = FileSize(pathname);
    if (file_size == 0) {
        return false;
    }

    if (pathname.size() > 3 && pathname.substr(pathname.size() - 3) == ".gz") {
#if (COMM_UTIL_WITH_ZLIB == 1)
        gzifstream gzin(pathname.c_str());
        if (!gzin.good()) {
            return false;
        }
        // for CSV text, 5-time compression rate is typical value
        auto init_buff_size = file_size * 5;
        data.resize(init_buff_size);
        gzin.read((char *)data.data(), init_buff_size);
        if (gzin.eof()) {
            auto read_size = gzin.gcount();
            if (read_size <= 0) {
                // shall not happen
                data = move(string());
                return false;
            }
            else {
                data.resize(read_size);
                return true;
            }
        }
        else {
            // read the rest of the data
            string rest_data;
            rest_data.reserve(file_size * 2);

            const size_t BLOCK_SIZE = 32 * 1024;
            string buff;
            buff.resize(BLOCK_SIZE);

            while (gzin.good()) {
                gzin.read((char *)buff.data(), BLOCK_SIZE);
                auto read_size = gzin.gcount();
                if (read_size > 0) {
                    auto old_size = rest_data.size();
                    rest_data.resize(old_size + read_size);
                    memcpy((char *)rest_data.data() + old_size, (char *)buff.data(), read_size);
                }
                if (gzin.eof()) {
                    break;
                }
            };

            auto old_size = data.size();
            data.resize(old_size + rest_data.size());
            memcpy((char *)data.data() + old_size, (char *)rest_data.data(), rest_data.size());

            return true;
        }
#else
        printf("Error in %s(): *.gz file is not supported!\n", __FUNCTION__);
        return false;
#endif // COMM_UTIL_WITH_ZLIB == 1
    }

    ifstream in(pathname.c_str());
    if (!in.good()) {
        return false;
    }
    data.resize(file_size);
    in.read((char *)data.data(), file_size);

    return true;
}

bool WriteStrToFile(const string& pathname, const string &data)
{
    ofstream out(pathname);
    if (!out.good()) return false;
    out << data;
    return true;
}

template <typename STR>
int StringReplaceImpl(STR& base, const STR& src, const STR& dest)
{
    int count = 0;
    typename STR::size_type pos = 0;
    auto src_len = src.size();
    auto des_len = dest.size();
    pos = base.find(src, pos);

    while (pos != STR::npos) {
        ++count;
        base.replace(pos, src_len, dest);
        pos = base.find(src, pos + des_len);
    }

    return count;
}

int StringReplace(string &strBase, const string &strSrc, const string &strDes)
{
    return StringReplaceImpl(strBase, strSrc, strDes);
}

int StringReplace(wstring &wstrBase, const wstring &wstrSrc,
    const wstring &wstrDes)
{
    return StringReplaceImpl(wstrBase, wstrSrc, wstrDes);
}

template <typename STR, typename CHAR>
int StringReplaceCharImpl(STR& base, CHAR ch_src, CHAR ch_des)
{
    int count = 0;
    for (CHAR& ch : base) {
        if (ch == ch_src) {
            ch = ch_des;
            ++count;
        }
    }
    return count;
}

int StringReplaceChar(string& str_base, char ch_src, char ch_des)
{
    return StringReplaceCharImpl(str_base, ch_src, ch_des);
}

int StringReplaceChar(wstring& wstr_base, wchar_t ch_src, wchar_t ch_des)
{
    return StringReplaceCharImpl(wstr_base, ch_src, ch_des);
}
#ifdef _WIN32
wstring utf82ws(const char* src)
{
    wstring ws;
    return utf82ws(src, ws);
}

wstring& utf82ws(const char* s, wstring& ws)
{
    int slength = (int)strlen(s);
    int len = MultiByteToWideChar(CP_UTF8, 0, s, slength, 0, 0);
    ws.clear();
    ws.resize(len, '\0');
    MultiByteToWideChar(CP_UTF8, 0, s, slength, (LPWSTR)&ws[0], len);
    return ws;
}

string ws2utf8(const wchar_t *ws)
{
    string s;
    return ws2utf8(ws, s);
}

string& ws2utf8(const wchar_t* ws, string& s)
{
    const int slength = (int)wcslen(ws);
    const int len = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)ws, slength, 0, 0, 0, 0);
    s.clear();
    s.resize(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)ws, slength, &s[0], len, 0, 0);
    return s;
}

string ws2gb2312(const wchar_t *wstr)
{
    int n = WideCharToMultiByte(CP_ACP, 0, wstr, -1, 0, 0, 0, 0);
    string result(n, '\0');
    ::WideCharToMultiByte(CP_ACP, 0, wstr, -1, &result[0], n, 0, 0);
    return result;
}

wstring gb2312_2_ws(const char *src)
{
    int n = MultiByteToWideChar(CP_ACP, 0, src, -1, NULL, 0);
    wstring result(n, '\0');
    ::MultiByteToWideChar(CP_ACP, 0, src, -1, (LPWSTR)result.c_str(), n);
    return result;
}

#endif


wstring& StrToWStr(const string& str, wstring& wstr)
{
#ifdef _WIN32
    return utf82ws(str.c_str(), wstr);
#else
    wstr = wstring(str.begin(), str.end());
    return wstr;

    wstr.clear();
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

string& WStrToStr(const wstring& wstr, string& str)
{
#ifdef _WIN32
    return ws2utf8(wstr.c_str(), str);
#else
    str.clear();
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

wstring StrToWStr(const string &str)
{
    wstring wstr;
    return StrToWStr(str, wstr);
}

string WStrToStr(const wstring &wstr)
{
    string str;
    return WStrToStr(wstr, str);
}

static const string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ" \
    "abcdefghijklmnopqrstuvwxyz" \
    "0123456789+/";

void Base64Encode(const char* str, string& ret)
{
    int i = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    int in_len = (int)strlen(str);
    ret.clear();
    ret.reserve(4 * (in_len / 3) + 4);

    while (in_len--) {
        char_array_3[i++] = *(str++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; (i < 4); i++) {
                ret += base64_chars[char_array_4[i]];
            }
            i = 0;
        }
    }

    if (i) {
        for (int j = i; j < 3; j++) {
            char_array_3[j] = '\0';
        }
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (int j = 0; (j < i + 1); j++) {
            ret += base64_chars[char_array_4[j]];
        }
        while ((i++ < 3)) {
            ret += '=';
        }
    }
}

void Base64Decode(const char* str, string& ret)
{
    int in_len = (int)strlen(str);
    ret.clear();
    ret.reserve(in_len * 3 / 4);

    int i = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];

    auto is_base64 = [](unsigned char c) {
        return (isalnum(c) || (c == '+') || (c == '/'));
    };

    while (in_len-- && (str[in_] != '=') && is_base64(str[in_])) {
        char_array_4[i++] = str[in_]; in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++) {
                char_array_4[i] = (unsigned char)base64_chars.find(char_array_4[i]);
            }
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; (i < 3); i++) {
                ret += char_array_3[i];
            }
            i = 0;
        }
    }

    if (i) {
        for (int j = i; j < 4; j++) {
            char_array_4[j] = 0;
        }
        for (int j = 0; j < 4; j++) {
            char_array_4[j] = (unsigned char)base64_chars.find(char_array_4[j]);
        }

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (int j = 0; (j < i - 1); j++) {
            ret += char_array_3[j];
        }
    }
}

struct LangEntry {
    char spras;
    char laspez;
    char lahq;
    const char *laiso;
    const char *sptxt;
};
const static LangEntry locale_table[] = {
// SPRAS LASPEZ LAHQ LAISO SPTXT
    { '0', 'S', '0', "SR", "Serbian" },
    { '1', 'D', '0', "ZH", "Chinese" },
    { '2', 'M', '0', "TH", "Thai" },
    { '3', 'D', '0', "KO", "Korean" },
    { '4', 'S', '0', "RO", "Romanian" },
    { '5', 'S', '0', "SL", "Slovenian" },
    { '6', 'S', '0', "HR", "Croatian" },
    { '7', 'S', '4', "MS", "Malaysian" },
    { '8', 'S', '0', "UK", "Ukrainian" },
    { '9', 'S', '0', "ET", "Estonian" },
    { 'A', 'L', '0', "AR", "Arabic" },
    { 'B', 'L', '0', "HE", "Hebrew" },
    { 'C', 'S', '4', "CS", "Czech" },
    { 'D', 'S', '1', "DE", "German" },
    { 'E', 'S', '1', "EN", "English" },
    { 'F', 'S', '2', "FR", "French" },
    { 'G', 'S', '0', "EL", "Greek" },
    { 'H', 'S', '4', "HU", "Hungarian" },
    { 'I', 'S', '2', "IT", "Italian" },
    { 'J', 'D', '2', "JA", "Japanese" },
    { 'K', 'S', '3', "DA", "Danish" },
    { 'L', 'S', '0', "PL", "Polish" },
    { 'M', 'D', '0', "ZF", "Chinese trad." },
    { 'N', 'S', '2', "NL", "Dutch" },
    { 'O', 'S', '3', "NO", "Norwegian" },
    { 'P', 'S', '4', "PT", "Portuguese" },
    { 'Q', 'S', '0', "SK", "Slovakian" },
    { 'R', 'S', '3', "RU", "Russian" },
    { 'S', 'S', '2', "ES", "Spanish" },
    { 'T', 'S', '0', "TR", "Turkish" },
    { 'U', 'S', '3', "FI", "Finnish" },
    { 'V', 'S', '3', "SV", "Swedish" },
    { 'W', 'S', '4', "BG", "Bulgarian" },
    { 'X', 'S', '0', "LT", "Lithuanian" },
    { 'Y', 'S', '0', "LV", "Latvian" },
    { 'Z', 'S', '0', "Z1", "Customer reserve" },
    { 'a', 'S', '0', "AF", "Afrikaans" },
    { 'b', 'S', '0', "IS", "Icelandic" },
    { 'c', 'S', '4', "CA", "Catalan" },
    { 'd', 'S', '4', "SH", "Serbian (Latin)" },
    { 'i', 'S', '0', "ID", "Indonesian" }
};

bool LocaleToLangChar(const string& locale, char& lang_char)
{
    return LocaleToLangChar(locale.c_str(), lang_char);
}

bool LocaleToLangChar(const char* locale, char& lang_char)
{
    if (locale == nullptr) return false;
    if (strlen(locale) != 2) return false;
    string locale_upper = locale;
    util::MakeUpper(locale_upper);

    for (auto& e : locale_table) {
        if (e.laiso[0] == locale_upper.front() && e.laiso[1] == locale_upper.at(1)) {
            lang_char = e.spras;
            return true;
        }
    }

    return false;
}

bool IsUtf8String(const string& str)
{
    const int MAX_UTF8_CHECK_LENGTH = 20;

    int length = (int)str.length();
    int check_sub = 0;
    int i = 0;

    if (length > MAX_UTF8_CHECK_LENGTH) {
        length = MAX_UTF8_CHECK_LENGTH;
    }

    for (; i < length; i++) {
        const auto& char_i = str[i];
        if (check_sub == 0) {
            if ((char_i >> 7) == 0) { //0xxx xxxx
                continue;
            }
            else if ((char_i & 0xE0) == 0xC0) { //110x xxxx
                check_sub = 1;
            }
            else if ((char_i & 0xF0) == 0xE0) { //1110 xxxx
                check_sub = 2;
            }
            else if ((char_i & 0xF8) == 0xF0) { //1111 0xxx
                check_sub = 3;
            }
            else if ((char_i & 0xFC) == 0xF8) { //1111 10xx
                check_sub = 4;
            }
            else if ((char_i & 0xFE) == 0xFC) { //1111 110x
                check_sub = 5;
            }
            else {
                return false;
            }
        }
        else {
            if ((char_i & 0xC0) != 0x80) {
                return false;
            }
            check_sub--;
        }
    }
    return true;
}


const static int pyvalue[] = { -20319, -20317, -20304, -20295, -20292, -20283, -20265, -20257, -20242, -20230, -20051, -20036, -20032, -20026,
-20002, -19990, -19986, -19982, -19976, -19805, -19784, -19775, -19774, -19763, -19756, -19751, -19746, -19741, -19739, -19728,
-19725, -19715, -19540, -19531, -19525, -19515, -19500, -19484, -19479, -19467, -19289, -19288, -19281, -19275, -19270, -19263,
-19261, -19249, -19243, -19242, -19238, -19235, -19227, -19224, -19218, -19212, -19038, -19023, -19018, -19006, -19003, -18996,
-18977, -18961, -18952, -18783, -18774, -18773, -18763, -18756, -18741, -18735, -18731, -18722, -18710, -18697, -18696, -18526,
-18518, -18501, -18490, -18478, -18463, -18448, -18447, -18446, -18239, -18237, -18231, -18220, -18211, -18201, -18184, -18183,
-18181, -18012, -17997, -17988, -17970, -17964, -17961, -17950, -17947, -17931, -17928, -17922, -17759, -17752, -17733, -17730,
-17721, -17703, -17701, -17697, -17692, -17683, -17676, -17496, -17487, -17482, -17468, -17454, -17433, -17427, -17417, -17202,
-17185, -16983, -16970, -16942, -16915, -16733, -16708, -16706, -16689, -16664, -16657, -16647, -16474, -16470, -16465, -16459,
-16452, -16448, -16433, -16429, -16427, -16423, -16419, -16412, -16407, -16403, -16401, -16393, -16220, -16216, -16212, -16205,
-16202, -16187, -16180, -16171, -16169, -16158, -16155, -15959, -15958, -15944, -15933, -15920, -15915, -15903, -15889, -15878,
-15707, -15701, -15681, -15667, -15661, -15659, -15652, -15640, -15631, -15625, -15454, -15448, -15436, -15435, -15419, -15416,
-15408, -15394, -15385, -15377, -15375, -15369, -15363, -15362, -15183, -15180, -15165, -15158, -15153, -15150, -15149, -15144,
-15143, -15141, -15140, -15139, -15128, -15121, -15119, -15117, -15110, -15109, -14941, -14937, -14933, -14930, -14929, -14928,
-14926, -14922, -14921, -14914, -14908, -14902, -14894, -14889, -14882, -14873, -14871, -14857, -14678, -14674, -14670, -14668,
-14663, -14654, -14645, -14630, -14594, -14429, -14407, -14399, -14384, -14379, -14368, -14355, -14353, -14345, -14170, -14159,
-14151, -14149, -14145, -14140, -14137, -14135, -14125, -14123, -14122, -14112, -14109, -14099, -14097, -14094, -14092, -14090,
-14087, -14083, -13917, -13914, -13910, -13907, -13906, -13905, -13896, -13894, -13878, -13870, -13859, -13847, -13831, -13658,
-13611, -13601, -13406, -13404, -13400, -13398, -13395, -13391, -13387, -13383, -13367, -13359, -13356, -13343, -13340, -13329,
-13326, -13318, -13147, -13138, -13120, -13107, -13096, -13095, -13091, -13076, -13068, -13063, -13060, -12888, -12875, -12871,
-12860, -12858, -12852, -12849, -12838, -12831, -12829, -12812, -12802, -12607, -12597, -12594, -12585, -12556, -12359, -12346,
-12320, -12300, -12120, -12099, -12089, -12074, -12067, -12058, -12039, -11867, -11861, -11847, -11831, -11798, -11781, -11604,
-11589, -11536, -11358, -11340, -11339, -11324, -11303, -11097, -11077, -11067, -11055, -11052, -11045, -11041, -11038, -11024,
-11020, -11019, -11018, -11014, -10838, -10832, -10815, -10800, -10790, -10780, -10764, -10587, -10544, -10533, -10519, -10331,
-10329, -10328, -10322, -10315, -10309, -10307, -10296, -10281, -10274, -10270, -10262, -10260, -10256, -10254 };

const static char Pystr[396][7] = { "A", "Ai", "An", "Ang", "Ao", "Ba", "Bai", "Ban", "Bang", "Bao", "Bei", "Ben", "Beng", "Bi", "Bian", "Biao",
"Bie", "Bin", "Bing", "Bo", "Bu", "Ca", "Cai", "Can", "Cang", "Cao", "Ce", "Ceng", "Cha", "Chai", "Chan", "Chang", "Chao", "Che", "Chen",
"Cheng", "Chi", "Chong", "Chou", "Chu", "Chuai", "Chuan", "Chuang", "Chui", "Chun", "Chuo", "Ci", "Cong", "Cou", "Cu", "Cuan", "Cui",
"Cun", "Cuo", "Da", "Dai", "Dan", "Dang", "Dao", "De", "Deng", "Di", "Dian", "Diao", "Die", "Ding", "Diu", "Dong", "Dou", "Du", "Duan",
"Dui", "Dun", "Duo", "E", "En", "Er", "Fa", "Fan", "Fang", "Fei", "Fen", "Feng", "Fo", "Fou", "Fu", "Ga", "Gai", "Gan", "Gang", "Gao",
"Ge", "Gei", "Gen", "Geng", "Gong", "Gou", "Gu", "Gua", "Guai", "Guan", "Guang", "Gui", "Gun", "Guo", "Ha", "Hai", "Han", "Hang",
"Hao", "He", "Hei", "Hen", "Heng", "Hong", "Hou", "Hu", "Hua", "Huai", "Huan", "Huang", "Hui", "Hun", "Huo", "Ji", "Jia", "Jian",
"Jiang", "Jiao", "Jie", "Jin", "Jing", "Jiong", "Jiu", "Ju", "Juan", "Jue", "Jun", "Ka", "Kai", "Kan", "Kang", "Kao", "Ke", "Ken",
"Keng", "Kong", "Kou", "Ku", "Kua", "Kuai", "Kuan", "Kuang", "Kui", "Kun", "Kuo", "La", "Lai", "Lan", "Lang", "Lao", "Le", "Lei",
"Leng", "Li", "Lia", "Lian", "Liang", "Liao", "Lie", "Lin", "Ling", "Liu", "Long", "Lou", "Lu", "Lv", "Luan", "Lue", "Lun", "Luo",
"Ma", "Mai", "Man", "Mang", "Mao", "Me", "Mei", "Men", "Meng", "Mi", "Mian", "Miao", "Mie", "Min", "Ming", "Miu", "Mo", "Mou", "Mu",
"Na", "Nai", "Nan", "Nang", "Nao", "Ne", "Nei", "Nen", "Neng", "Ni", "Nian", "Niang", "Niao", "Nie", "Nin", "Ning", "Niu", "Nong",
"Nu", "Nv", "Nuan", "Nue", "Nuo", "O", "Ou", "Pa", "Pai", "Pan", "Pang", "Pao", "Pei", "Pen", "Peng", "Pi", "Pian", "Piao", "Pie",
"Pin", "Ping", "Po", "Pu", "Qi", "Qia", "Qian", "Qiang", "Qiao", "Qie", "Qin", "Qing", "Qiong", "Qiu", "Qu", "Quan", "Que", "Qun",
"Ran", "Rang", "Rao", "Re", "Ren", "Reng", "Ri", "Rong", "Rou", "Ru", "Ruan", "Rui", "Run", "Ruo", "Sa", "Sai", "San", "Sang",
"Sao", "Se", "Sen", "Seng", "Sha", "Shai", "Shan", "Shang", "Shao", "She", "Shen", "Sheng", "Shi", "Shou", "Shu", "Shua",
"Shuai", "Shuan", "Shuang", "Shui", "Shun", "Shuo", "Si", "Song", "Sou", "Su", "Suan", "Sui", "Sun", "Suo", "Ta", "Tai",
"Tan", "Tang", "Tao", "Te", "Teng", "Ti", "Tian", "Tiao", "Tie", "Ting", "Tong", "Tou", "Tu", "Tuan", "Tui", "Tun", "Tuo",
"Wa", "Wai", "Wan", "Wang", "Wei", "Wen", "Weng", "Wo", "Wu", "Xi", "Xia", "Xian", "Xiang", "Xiao", "Xie", "Xin", "Xing",
"Xiong", "Xiu", "Xu", "Xuan", "Xue", "Xun", "Ya", "Yan", "Yang", "Yao", "Ye", "Yi", "Yin", "Ying", "Yo", "Yong", "You",
"Yu", "Yuan", "Yue", "Yun", "Za", "Zai", "Zan", "Zang", "Zao", "Ze", "Zei", "Zen", "Zeng", "Zha", "Zhai", "Zhan", "Zhang",
"Zhao", "Zhe", "Zhen", "Zheng", "Zhi", "Zhong", "Zhou", "Zhu", "Zhua", "Zhuai", "Zhuan", "Zhuang", "Zhui", "Zhun", "Zhuo",
"Zi", "Zong", "Zou", "Zu", "Zuan", "Zui", "Zun", "Zuo" };

string Gb2312HanziToPinyin(const string& hanzi_str, bool each_first_cap)
{
    string result;

    for (size_t j = 0; j < hanzi_str.length();) {
        auto& ch = hanzi_str[j];
        if (ch == '\0') {
            break;
        }

        if (ch >= 0 && ch < 128) { // non hanzi
            result += ch;
            j++;
            continue;
        }

        int chrasc = ch * 256 + hanzi_str[j + 1] + 256;
        if (chrasc > 0 && chrasc < 160) {
            result += ch;
            j++;
        }
        else {
            for (int i = sizeof(pyvalue) / sizeof(pyvalue[0]) - 1; i >= 0; i--) {
                if (pyvalue[i] <= chrasc) {
                    if (each_first_cap) {
                        result += Pystr[i];
                    }
                    else {
                        char pystr[16];
                        pystr[0] = tolower(Pystr[i][0]);
                        strncpy(pystr + 1, Pystr[i] + 1, sizeof(pystr) - 1);
                        result += pystr;
                    }
                    break;
                }
            }
            j += 2;
        }
    }

    return result;
}

UTIL_END_NAMESPACE
