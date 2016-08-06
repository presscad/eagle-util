#ifndef _NET_UTILS_H
#define _NET_UTILS_H

#include <string>

namespace net_util {

std::string UrlEncode(const std::string& src);
std::string UrlDecode(const std::string& src);

}

#endif // _NET_UTILS_H
