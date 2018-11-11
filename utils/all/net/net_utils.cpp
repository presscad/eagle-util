#include "net_utils.h"
#include <cctype>

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <direct.h>
#  include <WinSock2.h>
#  pragma comment(lib, "ws2_32.lib")
#else
#  include <unistd.h>
#  include <sys/time.h>
#  include <sys/socket.h>
#  include <netinet/in.h>
#endif

namespace net_util {

bool LoadSocketLib()
{
#ifdef _WIN32
    WSADATA wsaData;
    int nResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (NO_ERROR != nResult) {
        printf("failed to init Winsock!\n");
        return false;
    }
#endif
    return true;
}

bool SetSendTimeOutInMs(SOCKET sockfd, long timeout)
{
    int ret;
#ifdef _WIN32
    ret = setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout));
#else
    struct timeval tv;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;
    ret = setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO,
        (struct timeval *)&tv, sizeof(struct timeval));
#endif
    return ret == 0;
}

bool SetRecvTimeOutInMs(SOCKET sockfd, long timeout)
{
    int ret;
#ifdef _WIN32
    ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));
#else
    struct timeval tv;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;
    ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
        (struct timeval *)&tv, sizeof(struct timeval));
#endif
    return ret == 0;
}


static inline unsigned char ToHex(unsigned char x)   
{   
    return  x > 9 ? x + 55 : x + 48;
}  
  
static unsigned char FromHex(unsigned char x)   
{   
    if (x >= 'A' && x <= 'Z') {
        return x - 'A' + 10;
    }
    else if (x >= 'a' && x <= 'z') {
        return x - 'a' + 10;
    }
    else if (x >= '0' && x <= '9') {
        return x - '0';
    }

    return (unsigned char)255; // error
}  

std::string UrlEncode(const std::string& str)
{
    std::string result;
    const size_t length = str.length();
    result.reserve(length * 3);

    for (size_t i = 0; i < length; i++) {
        const auto& ch = str[i];
        if (std::isalnum((unsigned char)ch) || (ch == '-') || (ch == '_') ||
            (ch == '.') || (ch == '~')) {
            result += ch;
        }
        else if (ch == ' ') {
            result += "+";
        }
        else {
            char buf[4];
            buf[0] = '%';
            buf[1] = (char)ToHex((unsigned char)ch >> 4);
            buf[2] = (char)ToHex((unsigned char)ch % 16);
            buf[3] = '\0';
            result += buf;
        }
    }

    return result;
}

std::string UrlDecode(const std::string& str)
{
    std::string result;
    const size_t length = str.length();
    result.reserve(length);

    for (size_t i = 0; i < length; i++) {
        const auto& ch = str[i];
        if (ch == '+') {
            result += ' ';
        }
        else if (ch == '%') {
            if (i + 2 >= length) {
                return std::string();
            }
            unsigned char high = FromHex((unsigned char)str[++i]);
            unsigned char low = FromHex((unsigned char)str[++i]);
            if (high == (unsigned char)255 || low == (unsigned char)255) {
                return std::string();
            }
            result += high * 16 + low;
        }
        else {
            result += ch;
        }
    }

    return result;
}

} // namespace net_util
