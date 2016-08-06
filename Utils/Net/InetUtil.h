#ifndef _INET_UTIL_H
#define _INET_UTIL_H

#include <string>

namespace utils {

// For IE
bool EnableIEProxy(bool bEnable, const char *sIP = NULL, unsigned int nPort = NULL, const char *sBypass = NULL);
bool GetIEProxy(bool &bEnable, std::string &proxy, std::string &byPass);
void ShowNetworkProxySettings();

}
#endif // _INET_UTIL_H
