#ifndef _NET_UTILS_H
#define _NET_UTILS_H

#include <string>

namespace net_util {

#ifdef _WIN32
    typedef unsigned long long SOCKET;
#else
    typedef int SOCKET;
#endif

bool LoadSocketLib();
bool SetSendTimeOutInMs(SOCKET sockfd, long timeout);
bool SetRecvTimeOutInMs(SOCKET sockfd, long timeout);

std::string UrlEncode(const std::string& src);
std::string UrlDecode(const std::string& src);

}

#endif // _NET_UTILS_H
