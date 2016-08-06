#ifdef _WIN32
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#if (USE_GLOG == 1)
#ifdef _WIN32
#define GOOGLE_GLOG_DLL_DECL // for static link of the glog code on window
#endif
#endif

#define REPEATING_LOG_CHECK_WINDOW  5000 // in mini sec.


#if (USE_GLOG == 1)
#ifdef _WIN32 // fix the ERROR macro confliction on Windows
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef ERROR
#endif
#include <glog/logging.h>
#endif

#include <stdarg.h>
#include <boost/thread/mutex.hpp>
#include <boost/atomic.hpp>
#include <boost/filesystem.hpp>
#include "LogUtils.h"
#include "CommUtils.h"



static FILE *gLogFp = NULL;
static std::string gLastLog;
static unsigned long long gLastLogTime = 0;
static int gLastLogRepeat = 0;
static bool gToStderr = false;
static bool gToFile = false;
static boost::mutex gLogMutex;


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
    snprintf(buff, size, "%02d-%02d-%02d %02d:%02d:%02d",
        stm.tm_year + 1900 - 2000,
        stm.tm_mon + 1,
        stm.tm_mday,
        stm.tm_hour,
        stm.tm_min,
        stm.tm_sec);
    return buff;
}

static void FormatPath(std::string &path)
{
#ifdef _WIN32
    std::replace(path.begin(), path.end(), '/', '\\');
#else
    std::replace(path.begin(), path.end(), '\\', '/');
#endif
}

std::string GetLastLog()
{
    boost::mutex::scoped_lock lock(gLogMutex);
    return gLastLog;
}

int GetLastLogRepeatNum()
{
    boost::mutex::scoped_lock lock(gLogMutex);
    return gLastLogRepeat;
}

bool LogInit(bool to_file)
{
    return LogInit(true, to_file);
}

bool LogInit(bool to_stderr, bool to_file)
{
    boost::mutex::scoped_lock lock(gLogMutex);
    gToStderr = to_stderr;
    gToFile = to_file;

    if (to_file) {
        std::string sLogPath = GetCurrentPath() + "/Log";
        FormatPath(sLogPath);
        if (!boost::filesystem::exists(sLogPath)) {
            if (false == boost::filesystem::create_directories(sLogPath)) {
                Log(LOG_ERROR, "cannot create log directory: %s!", sLogPath.c_str());
                return false;
            }
        }

#if (USE_GLOG == 1)
        {
#ifdef _WIN32
            google::InitGoogleLogging(__argv[0]);
#else
            char exe[1024];
            int len = (int)readlink("/proc/self/exe", exe, sizeof(exe) - 1);
            exe[len] = '\0';
            google::InitGoogleLogging(exe);
#endif
            std::string logpath(sLogPath + '/');
            std::replace(logpath.begin(), logpath.end(), '\\', '/');
            google::SetLogDestination(google::INFO, logpath.c_str());

            if (gToStderr) {
                google::SetStderrLogging(google::INFO);
            }
        }
#else
        SQL_TIMESTAMP_STRUCT ts;
        GetCurTimestamp(ts);
        char buff[128];
        snprintf(buff, sizeof(buff), "/%04d%02d%02d-%02d.%02d.%02d.log",
            ts.year, ts.month, ts.day, ts.hour, ts.minute, ts.second);
        sLogPath += buff;
        FormatPath(sLogPath);

        gLogFp = fopen(sLogPath.c_str(), "at");
        if (NULL == gLogFp) {
            Log(LOG_ERROR, "cannot create log file: %s!", sLogPath.c_str());
            return false;
        }
#endif
    }

    return true;
}

void LogClose()
{
    boost::mutex::scoped_lock lock(gLogMutex);
#if (USE_GLOG == 1)
    google::ShutdownGoogleLogging();
#else
    if (gLogFp) {
        fclose(gLogFp);
        gLogFp = NULL;
    }
#endif
}

void _Log_(const char *file, int line, int level, const char *fmt, ...)
{
    const int TIMESTAMP_LEN = 17;
    const int CATEGORY_LEN = 8;
    va_list argptr;
    char buffer[1024 * 4];
    int cur_len;

    GetCurTimeStr_Log(buffer, sizeof(buffer));
    cur_len = TIMESTAMP_LEN;
    char *buffer_no_time = buffer + cur_len;

    switch(level) {
    case LOG_INFO: strncat(buffer, " [INFO] ", sizeof(buffer)); break;
    case LOG_WARNING: strncat(buffer, " [WARN] ", sizeof(buffer)); break;
    case LOG_ERROR: strncat(buffer, "[ERROR] ", sizeof(buffer)); break;
    default:
        assert(false);
        return;
    }
    cur_len += CATEGORY_LEN;

    va_start(argptr, fmt);
    int ret = vsnprintf(buffer + cur_len, sizeof(buffer) - cur_len, fmt, argptr);
    va_end(argptr);
    if (ret == -1 || ret >= (int)sizeof(buffer) - cur_len) {
        // truncated
        buffer[sizeof(buffer) - 1] = '\0';
    }

    unsigned long long now = ::GetTimeInMs64();
    boost::mutex::scoped_lock lock(gLogMutex);
    long diff = (long)(now - gLastLogTime);
    gLastLogTime = now;

    if (diff <= REPEATING_LOG_CHECK_WINDOW && gLastLog == buffer_no_time) {
        gLastLogRepeat++;
        if (gLastLogRepeat >= 10) {
            // found repeating log
            if ((gLastLogRepeat % 10) == 0) {
                char tmp[128];
                snprintf(tmp, sizeof(tmp), " [This log was repeated for %d times in short period]", gLastLogRepeat);
                strncat(buffer, tmp, sizeof(buffer));
            } else {
                return; // ignore
            }
        }
    } else {
        gLastLogRepeat = 0;
        gLastLog = buffer_no_time;
    }

#if (USE_GLOG == 1)
    lock.unlock();

    const char *lastLog = buffer_no_time + CATEGORY_LEN;
    switch(level) {
    case LOG_INFO:
        google::LogMessage(file, line, google::GLOG_INFO).stream() << lastLog << "\n";
        break;
    case LOG_WARNING:
        google::LogMessage(file, line, google::GLOG_WARNING).stream() << lastLog << "\n";
        break;
    case LOG_ERROR:
        google::LogMessage(file, line, google::GLOG_ERROR).stream() << lastLog << "\n";
        break;
    default:
        assert(false);
        return;
    }

#else
    if (gToStderr) {
        printf("%s\n", buffer);
    }
    if (gToFile && gLogFp) {
        fprintf(gLogFp, "%s\n", buffer);
        fflush(gLogFp);
    }
#endif
}

void LogFlush()
{
#if (USE_GLOG == 1)
    google::FlushLogFiles(google::INFO);
#else
    fflush(gLogFp);
#endif
}
