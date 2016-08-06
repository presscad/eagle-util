#ifndef _LOG_UTILS_H
#define _LOG_UTILS_H

#include <string>

///////////////////////////////////////////////////////////////////////////////////////////////////
// Define USE_GLOG to 1 in project settings if want to map the log to google log!
///////////////////////////////////////////////////////////////////////////////////////////////////


#define LOG_INFO        1
#define LOG_WARNING     2
#define LOG_ERROR       3

#ifdef _WIN32
#define Log(level, fmt, ...) _Log_(__FILE__ , __LINE__, level, fmt, __VA_ARGS__)
#else
#define Log(level, fmt, args...) _Log_(__FILE__ , __LINE__, level, fmt, ##args)
#endif

bool LogInit(bool to_file);
bool LogInit(bool to_stderr, bool to_file);
void LogClose();
void LogFlush();
void _Log_(const char *file, int line, int level, const char *fmt, ...);
std::string GetLastLog();
int GetLastLogRepeatNum();


#endif // _LOG_UTILS_H
