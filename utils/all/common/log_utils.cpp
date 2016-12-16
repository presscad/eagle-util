
#if defined(_WIN32) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#define REPEATING_LOG_CHECK_WINDOW  5000 // in mini sec.

#include <cstdarg>
#include <cassert>
#include <cstring>
#include <algorithm>
#include <memory>
#include "log_utils.h"
#include "common_utils.h"

#include <mutex>
#ifndef _WIN32
#  include <sys/types.h>
#  include <sys/stat.h>
#else
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <Windows.h>
#endif

UTIL_BEGIN_NAMESPACE

struct LOG_CORE
{
    LOG_CORE()
        : log_fp(NULL), last_log_time(0), last_log_repeat(0),
        to_stderr(false), to_file(false), append_endl(true), is_inited(false),
        log_level(LOG_INFO), log_level_to_be(0)
    {
        counts_by_level.resize(LOG_FATAL + 1);
#ifdef PROJ_SIM_AFL
        to_stderr = true;
        to_file = false;
        append_endl = false;
#endif
    }

    ~LOG_CORE()
    {
        if (log_fp) {
            fclose(log_fp);
            log_fp = NULL;
        }
    }

public:
    FILE *log_fp;
    std::string last_log;
    unsigned long long last_log_time;
    int last_log_repeat;

    bool to_stderr;
    bool to_file;
    bool append_endl;
    bool is_inited;

    int log_level;
    int log_level_to_be;

    std::vector<int> counts_by_level;
};

static LOG_CORE& GetLogCore()
{
    static LOG_CORE sLogCore;
    return sLogCore;
}


// static variable in static lib may not get initilized, use workaround below
static std::mutex& GetLogMutex()
{
    static std::mutex sLogMutex;
    return sLogMutex;
}

static char *GetCurTimeStr_Log(char buff[], int size)
{
    time_t t;
    time(&t);
    struct tm stm;
#ifdef _WIN32
    localtime_s(&stm, &t);
#else
    localtime_r(&t, &stm);
#endif
    snprintf(buff, size - 1, "%02d-%02d-%02d %02d:%02d:%02d",
        stm.tm_year + 1900 - 2000,
        stm.tm_mon + 1,
        stm.tm_mday,
        stm.tm_hour,
        stm.tm_min,
        stm.tm_sec);
    return buff;
}

static std::string& FormatPath(std::string &path)
{
#ifdef _WIN32
    std::replace(path.begin(), path.end(), '/', '\\');
#else
    std::replace(path.begin(), path.end(), '\\', '/');
#endif
    return path;
}

static void LogCleanUp()
{
    const int KEEP_NUM = 30;
    std::string sLogPath = GetCurrentPath() + "/Log";
    FormatPath(sLogPath);

    std::vector<std::string> filenames;
    std::string tLogPath = sLogPath + "/20*.log";
    if (true == util::FindFiles(FormatPath(tLogPath), filenames)) {
        if ((int)filenames.size() > KEEP_NUM) {
            std::sort(filenames.begin(), filenames.end());
            for (int i = 0; i < (int)filenames.size() - KEEP_NUM; ++i) {
                tLogPath = sLogPath + '/' + filenames[i];
                ::remove(FormatPath(tLogPath).c_str());
            }
        }
    }
}

std::string GetLastLog()
{
    std::lock_guard<std::mutex> lock(GetLogMutex());
    return GetLogCore().last_log;
}

int GetLastLogRepeatNum()
{
    std::lock_guard<std::mutex> lock(GetLogMutex());
    return GetLogCore().last_log_repeat;
}

int GetLogCount(int level)
{
    if (level < 0 || level > LOG_ERROR) {
        return 0;
    }
    std::lock_guard<std::mutex> lock(GetLogMutex());
    return GetLogCore().counts_by_level[level];
}

bool LogInit(bool to_file)
{
    return LogInit(true, to_file, true, LOG_INFO);
}

bool LogInit(bool to_stderr, bool to_file)
{
    return LogInit(to_stderr, to_file, true, LOG_INFO);
}

bool LogInit(bool to_stderr, bool to_file, bool append_endl, int log_level)
{
    std::lock_guard<std::mutex> lock(GetLogMutex());

    GetLogCore().to_stderr = to_stderr;
    GetLogCore().to_file = to_file;
    GetLogCore().append_endl = append_endl;
    if (log_level >= LOG_DEBUG && log_level <= LOG_FATAL) {
        GetLogCore().log_level = log_level;
    }

    LogCleanUp();

    if (to_file) {
        std::string sLogPath = GetCurrentPath() + "/Log";
        FormatPath(sLogPath);
#ifdef _WIN32
        if (!(::CreateDirectory(sLogPath.c_str(), NULL) ||
            ERROR_ALREADY_EXISTS == ::GetLastError())) {
            Log(LOG_ERROR, "cannot create log directory: %s!", sLogPath.c_str());
            return false;
        }
#else
        mkdir(sLogPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif

        TIMESTAMP_STRUCT ts;
        GetCurTimestamp(ts);
        char buff[128];
        snprintf(buff, sizeof(buff)-1, "/%04d%02u%02u-%02u.%02u.%02u.log",
            ts.year, ts.month, ts.day, ts.hour, ts.minute, ts.second);
        sLogPath += buff;
        FormatPath(sLogPath);

        if (GetLogCore().log_fp) {
            fclose(GetLogCore().log_fp);
        }
        GetLogCore().log_fp = fopen(sLogPath.c_str(), "at");
        if (NULL == GetLogCore().log_fp) {
            Log(LOG_ERROR, "cannot create log file: %s!", sLogPath.c_str());
            return false;
        }
    }

    GetLogCore().is_inited = true;
    return true;
}

int LogSetLevel(int log_level)
{
    std::lock_guard<std::mutex> lock(GetLogMutex());

    auto old_level = GetLogCore().log_level;
    if (log_level >= LOG_DEBUG && log_level <= LOG_FATAL) {
        if (GetLogCore().is_inited) {
            GetLogCore().log_level = log_level;
            GetLogCore().log_level_to_be = 0;
        }
        else {
            GetLogCore().log_level_to_be = log_level;
        }
    }
    return old_level;
}

int LogGetLevel()
{
    std::lock_guard<std::mutex> lock(GetLogMutex());

    if (GetLogCore().log_level_to_be != 0) {
        return GetLogCore().log_level_to_be;
    }
    else {
        return GetLogCore().log_level;
    }
}

void LogClose()
{
    if (GetLogCore().log_fp) {
        fclose(GetLogCore().log_fp);
        GetLogCore().log_fp = NULL;
    }
    GetLogCore().is_inited = false;
}

void _Log_(const char *file, int line, int level, const char *fmt, ...)
{
    const int TIMESTAMP_LEN = 17;
    const int CATEGORY_LEN = 5;
    va_list argptr;
    char buffer[1024 * 4];
    int cur_len;

    if (level < LOG_DEBUG || level > LOG_FATAL) {
        return;
    }

    std::lock_guard<std::mutex> lock(GetLogMutex());
    if (!GetLogCore().is_inited) {
        return;
    }

    // increase the summary info
    GetLogCore().counts_by_level[level]++;
    GetLogCore().counts_by_level[0]++; // 0 for all levels

    GetCurTimeStr_Log(buffer, sizeof(buffer));
    cur_len = TIMESTAMP_LEN;
    char *buffer_no_time = buffer + cur_len;

    switch (level) {
    case LOG_DEBUG: strncat(buffer, " [D] ", sizeof(buffer) - cur_len); break;
    case LOG_INFO: strncat(buffer, " [I] ", sizeof(buffer) - cur_len); break;
    case LOG_WARNING: strncat(buffer, " [W] ", sizeof(buffer) - cur_len); break;
    case LOG_ERROR: strncat(buffer, " [E] ", sizeof(buffer) - cur_len); break;
    case LOG_FATAL: strncat(buffer, " [F] ", sizeof(buffer) - cur_len); break;
    default:
        assert(false);
        return;
    }
    cur_len += CATEGORY_LEN;

    va_start(argptr, fmt);
    int ret = vsnprintf(buffer + cur_len, sizeof(buffer) - cur_len - 1, fmt, argptr);
    va_end(argptr);
    if (ret == -1 || ret >= (int)sizeof(buffer)-cur_len) {
        // truncated
        buffer[sizeof(buffer)-1] = '\0';
    }

   
    if (GetLogCore().log_level_to_be != 0 && GetLogCore().is_inited) {
        GetLogCore().log_level = GetLogCore().log_level_to_be;
        GetLogCore().log_level_to_be = 0;
    }
    if (level < GetLogCore().log_level) {
        return;
    }

    unsigned long long now = util::GetTimeInMs64();
    long diff = (long)(now - GetLogCore().last_log_time);
    GetLogCore().last_log_time = now;

    if (diff <= REPEATING_LOG_CHECK_WINDOW && GetLogCore().last_log == buffer_no_time) {
        GetLogCore().last_log_repeat++;
        if (GetLogCore().last_log_repeat >= 10) {
            // found repeating log
            if ((GetLogCore().last_log_repeat % 10) == 0) {
                char tmp[128];
                snprintf(tmp, sizeof(tmp) - 1, " [This log was repeated for %d times in short period]",
                    GetLogCore().last_log_repeat);
                tmp[sizeof(tmp)-1] = '\0';
                strncat(buffer, tmp, sizeof(buffer) - strlen(buffer));
            }
            else {
                return; // ignore
            }
        }
    }
    else {
        GetLogCore().last_log_repeat = 0;
        GetLogCore().last_log = buffer_no_time;
    }

    if (GetLogCore().append_endl) {
        strncat(buffer, "\n", sizeof(buffer) - strlen(buffer));
    }

#ifdef _WIN32
    {
        char buff[1204 * 4];
        std::wstring wstr = utf82ws(buffer); // assume buffer could be UTF-8
        snprintf(buff, sizeof(buff) - 1, "%s(%d): %s", file, line,
            ws2gb2312(wstr.c_str()).c_str());
        buff[sizeof(buff) - 1] = '\0';
        ::OutputDebugStringA(buff);
    }
#endif


    if (GetLogCore().to_stderr) {
#ifdef _WIN32
        std::wstring wstr = utf82ws(buffer); // assume buffer could be UTF-8
        printf("%s(%d): %s", file, line, ws2gb2312(wstr.c_str()).c_str());
#else
        printf("%s(%d): %s", file, line, buffer);
#endif
    }
    if (GetLogCore().to_file && GetLogCore().log_fp) {
        fprintf(GetLogCore().log_fp, "%s(%d): %s", file, line, buffer);
        fflush(GetLogCore().log_fp);
    }


    if (LOG_FATAL == level) {
        exit(-1);
    }
}

void LogFlush()
{
    std::lock_guard<std::mutex> lock(GetLogMutex());

    if (GetLogCore().log_fp) {
        fflush(GetLogCore().log_fp);
    }
}

UTIL_END_NAMESPACE
