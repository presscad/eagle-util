#ifndef _LOG_UTILS_H
#define _LOG_UTILS_H

#include <string>
#include "common_utils.h"


#define LOG_DEBUG       1
#define LOG_INFO        2
#define LOG_WARNING     3
#define LOG_ERROR       4
#define LOG_FATAL       5

#ifdef PROJ_SIM_AFL

#ifdef _WIN32
#   define Log(level, fmt, ...) sim_afl::util::_Log_(__FILE__ , __LINE__, level, fmt, __VA_ARGS__)
#   define Log2(file, line, level, fmt, ...) sim_afl::::util::_Log_(file, line, level, fmt, __VA_ARGS__)
#else
#   define Log(level, fmt, args...) sim_afl::util::_Log_(__FILE__ , __LINE__, level, fmt, ##args)
#   define Log2(file, line, level, fmt, args...) sim_afl::::util::_Log_(file, line, level, fmt, ##args)
#endif

#else

#ifdef _WIN32
#   define Log(level, fmt, ...) ::util::_Log_(__FILE__ , __LINE__, level, fmt, __VA_ARGS__)
#   define Log2(file, line, level, fmt, ...) ::util::_Log_(file, line, level, fmt, __VA_ARGS__)
#else
#   define Log(level, fmt, args...) ::util::_Log_(__FILE__ , __LINE__, level, fmt, ##args)
#   define Log2(file, line, level, fmt, args...) ::util::_Log_(file, line, level, fmt, ##args)
#endif

#endif


UTIL_BEGIN_NAMESPACE

bool LogInit(bool to_file);
bool LogInit(bool to_stderr, bool to_file);
bool LogInit(bool to_stderr, bool to_file, bool append_endl, int log_level);
int  LogSetLevel(int log_level);
int  LogGetLevel();
void LogClose();
void LogFlush();
void _Log_(const char *file, int line, int level, const char *fmt, ...);
std::string GetLastLog();
int GetLastLogRepeatNum();
int GetLogCount(int level); // level: from LOG_DEBUG to LOG_ERROR, 0 for all

UTIL_END_NAMESPACE

#endif // _LOG_UTILS_H
