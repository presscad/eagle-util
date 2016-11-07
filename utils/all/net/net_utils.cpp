#include "net_utils.h"
#include <cctype>


namespace net_util {

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
