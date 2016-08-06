
#if defined(_WIN32) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif


#include "common_utils.h"
#include <cmath>
extern "C" {
#include "iniparser.h"
}

namespace util {

dictionary * IniInitDict(const std::string& ini_name)
{
    return iniparser_load(ini_name.c_str());
}

void IniFreeDict(dictionary * dict)
{
    iniparser_freedict((struct _dictionary_ *)dict);
}

bool ReadIniString(dictionary *dict, const char *section, std::string &str, std::string &err)
{
    char *value = iniparser_getstring((struct _dictionary_ *)dict, section, NULL);
    if (NULL == value)
    {
        char buff[128];
        snprintf(buff, sizeof(buff)-1, "cannot find %s in config file", section);
        err = buff;
        return false;
    }
    str = value;
    return true;
}

bool ReadIniInt(dictionary *dict, const char*section, int &value, std::string &err)
{
    value = iniparser_getint((struct _dictionary_ *)dict, section, 0xCDCDCDCD);
    if ((unsigned int)value == 0xCDCDCDCD)
    {
        char buff[128];
        snprintf(buff, sizeof(buff)-1, "cannot find %s in config file", section);
        err = buff;
        return false;
    }
    return true;
}

bool ReadIniDouble(dictionary *dict, const char *section, double &value, std::string &err)
{
    std::string str;
    if (!ReadIniString(dict, section, str, err)) {
        return false;
    }

    value = std::atof(str.c_str());
    return true;
}

bool ReadIniInt(dictionary *dict, const std::string& section, int &value, std::string &err)
{
    return ReadIniInt(dict, section.c_str(), value, err);
}

bool ReadIniDouble(dictionary *dict, const std::string& section, double &value, std::string &err)
{
    return ReadIniDouble(dict, section.c_str(), value, err);
}

bool ReadIniString(dictionary *dict, const std::string& section, std::string &value, std::string &err)
{
    return ReadIniString(dict, section.c_str(), value, err);
}

}
