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
        : log_fp_(nullptr) 
    {
        counts_by_level_.resize(LOG_FATAL + 1);
#ifdef PROJ_SIM_AFL
        to_stderr = true;
        to_file = false;
        append_endl = false;
#endif
    }

public:
    std::shared_ptr<FILE> log_fp_{};
    std::string last_log_;
    unsigned long long last_log_time_{};
    int last_log_repeat_{};

    bool to_stderr_{};
    bool to_file_{};
    bool append_endl_{true};
    bool is_inited_{};

    int log_level_{LOG_INFO};
    int log_level_to_be_{};

    std::vector<int> counts_by_level_;
    std::string process_name_;
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
    if (util::FindFiles(FormatPath(tLogPath), filenames)) {
        if (static_cast<int>(filenames.size()) > KEEP_NUM) {
            std::sort(filenames.begin(), filenames.end());
            for (int i = 0; i < static_cast<int>(filenames.size()) - KEEP_NUM; ++i) {
                tLogPath = sLogPath + '/' + filenames[i];
                ::remove(FormatPath(tLogPath).c_str());
            }
        }
    }
}

std::string GetLastLog()
{
    std::lock_guard<std::mutex> lock(GetLogMutex());
    return GetLogCore().last_log_;
}

int GetLastLogRepeatNum()
{
    std::lock_guard<std::mutex> lock(GetLogMutex());
    return GetLogCore().last_log_repeat_;
}

int GetLogCount(int level)
{
    if (level < 0 || level > LOG_ERROR) {
        return 0;
    }
    std::lock_guard<std::mutex> lock(GetLogMutex());
    return GetLogCore().counts_by_level_[level];
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
    auto& core = GetLogCore();

    core.to_stderr_ = to_stderr;
    core.to_file_ = to_file;
    core.append_endl_ = append_endl;
    if (log_level >= LOG_DEBUG && log_level <= LOG_FATAL) {
        core.log_level_ = log_level;
    }
    core.process_name_ = util::GetCurrentProcessName();
    util::StringReplace(core.process_name_, " ", ""); // remove spaces

    LogCleanUp();

    if (to_file) {
        std::string sLogPath = GetCurrentPath() + "/log";
        FormatPath(sLogPath);
#ifdef _WIN32
        if (!(::CreateDirectory(sLogPath.c_str(), nullptr) ||
            ERROR_ALREADY_EXISTS == ::GetLastError())) {
            Log(LOG_ERROR, "cannot create log directory: %s!", sLogPath.c_str());
            return false;
        }
#else
        mkdir(sLogPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif

        sLogPath += '/';
        if (!core.process_name_.empty()) {
            sLogPath += core.process_name_ + '-';
        }

        TIMESTAMP_STRUCT ts;
        GetCurTimestamp(ts);
        char buff[128];
        snprintf(buff, sizeof(buff)-1, "%04d%02u%02u-%02u.%02u.%02u.log",
            ts.year, ts.month, ts.day, ts.hour, ts.minute, ts.second);
        sLogPath += buff;
        FormatPath(sLogPath);

        if (nullptr != core.log_fp_) {
            fclose(core.log_fp_.get());
        }
        core.log_fp_ = std::shared_ptr<FILE>(fopen(sLogPath.c_str(), "at"), [=](FILE *fp) {
            if (fp) {
                fclose(fp);
            }
        });
        if (nullptr == core.log_fp_) {
            Log(LOG_ERROR, "cannot create log file: %s!", sLogPath.c_str());
            return false;
        }
    }

    core.is_inited_ = true;
    return true;
}

int LogSetLevel(int log_level)
{
    std::lock_guard<std::mutex> lock(GetLogMutex());

    auto old_level = GetLogCore().log_level_;
    if (log_level >= LOG_DEBUG && log_level <= LOG_FATAL) {
        if (GetLogCore().is_inited_) {
            GetLogCore().log_level_ = log_level;
            GetLogCore().log_level_to_be_ = 0;
        }
        else {
            GetLogCore().log_level_to_be_ = log_level;
        }
    }
    return old_level;
}

int LogGetLevel()
{
    std::lock_guard<std::mutex> lock(GetLogMutex());

    if (GetLogCore().log_level_to_be_ != 0) {
        return GetLogCore().log_level_to_be_;
    }
    
    return GetLogCore().log_level_;
}

void LogClose()
{
    GetLogCore().log_fp_ = nullptr;
    GetLogCore().is_inited_ = false;
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
    auto& core = GetLogCore();
    if (!core.is_inited_) {
        return;
    }

    // increase the summary info
    core.counts_by_level_[level]++;
    core.counts_by_level_[0]++; // 0 for all levels

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
    if (ret == -1 || ret >= static_cast<int>(sizeof(buffer))-cur_len) {
        // truncated
        buffer[sizeof(buffer)-1] = '\0';
    }

   
    if (core.log_level_to_be_ != 0 && core.is_inited_) {
        core.log_level_ = core.log_level_to_be_;
        core.log_level_to_be_ = 0;
    }
    if (level < core.log_level_) {
        return;
    }

    unsigned long long now = util::GetTimeInMs64();
    auto diff = static_cast<long>(now - core.last_log_time_);
    core.last_log_time_ = now;

    if (diff <= REPEATING_LOG_CHECK_WINDOW && core.last_log_ == buffer_no_time) {
        core.last_log_repeat_++;
        if (core.last_log_repeat_ >= 10) {
            // found repeating log
            if ((core.last_log_repeat_ % 10) == 0) {
                char tmp[128];
                snprintf(tmp, sizeof(tmp) - 1, " [This log was repeated for %d times in short period]",
                    core.last_log_repeat_);
                tmp[sizeof(tmp)-1] = '\0';
                strncat(buffer, tmp, sizeof(buffer) - strlen(buffer) - 1);
            }
            else {
                return; // ignore
            }
        }
    }
    else {
        core.last_log_repeat_ = 0;
        core.last_log_ = buffer_no_time;
    }

    if (core.append_endl_) {
        strncat(buffer, "\n", sizeof(buffer) - strlen(buffer) - 1);
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

    // strip out path
    std::string str_file(file);
    util::StringReplaceChar(str_file, '\\', '/');
    auto n = str_file.rfind('/');
    if (n != std::string::npos) {
        file += n + 1;
    }

    if (core.to_stderr_) {
#ifdef _WIN32
        std::wstring wstr = utf82ws(buffer); // assume buffer could be UTF-8
        printf("%s %s(%d): %s", core.process_name_.c_str(), file, line,
            ws2gb2312(wstr.c_str()).c_str());
#else
        printf("%s %s(%d): %s", core.process_name_.c_str(), file, line, buffer);
#endif
    }
    if (core.to_file_ && nullptr != core.log_fp_) {
        fprintf(core.log_fp_.get(), "%s %s(%d): %s", core.process_name_.c_str(),
            file, line, buffer);
        fflush(core.log_fp_.get());
    }


    if (LOG_FATAL == level) {
        exit(-1);
    }
}

void LogFlush()
{
    std::lock_guard<std::mutex> lock(GetLogMutex());

    if (nullptr != GetLogCore().log_fp_) {
        fflush(GetLogCore().log_fp_.get());
    }
}

UTIL_END_NAMESPACE
