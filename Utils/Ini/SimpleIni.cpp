
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "SimpleIni.h"
#include "FileUtil.h"
extern "C" {
#include "iniparser/src/iniparser.h"
}

#if defined(_MSC_VER) && defined(_DEBUG)
#define new DEBUG_NEW
#endif

namespace utils {

CSimpleIni::CSimpleIni(const char * pathname)
{
    if (pathname != NULL)
        m_strPathname = pathname;
    else
        m_strPathname.clear();

    if (m_strPathname.empty())
    {
        std::string modulePathName = GetCurModulePathname();
        m_strPathname = GetFilePath(modulePathName.c_str()) + 
            GetFileTitle(modulePathName.c_str()) + ".ini";
    }

    if (!FileExists(m_strPathname.c_str()))
    {
        std::string path = GetFilePath(m_strPathname.c_str());
        if (!DirectoryExists(path.c_str()))
        {
            RecursiveCreateDirectory(path.c_str());
        }

        // create a dummy ini file
        FILE *fp = fopen(m_strPathname.c_str(), "wb");
        if (fp)
        {
            fwrite(" ", 1, 1, fp);
            fclose(fp);
        }
    }

    m_iniFd = (void *)iniparser_load(m_strPathname.c_str());
}

CSimpleIni::~CSimpleIni(void)
{
    if (m_iniFd)
    {
        iniparser_freedict((dictionary *)m_iniFd);
        m_iniFd = NULL;
    }
}

std::string CSimpleIni::GetString(const char* section, const char* entry, const char* defaultValue)
{
    std::string strValue;
    if (defaultValue != NULL)
        strValue = defaultValue;
    if (section == NULL)
        return strValue;

    if (m_iniFd != NULL)
    {
        std::string key = section;
        if (entry != NULL && entry[0] != NULL)
        {
            key += ":";
            key += entry;
        }

        char *value = iniparser_getstring((dictionary *)m_iniFd, key.c_str(), (char *)defaultValue);
        if (value != NULL)
            strValue = value;
    }
    return strValue;
}

bool CSimpleIni::WriteString(const char* section, const char* entry, const char* value)
{
    bool result = false;
    if (section == NULL)
        return result;

    if (m_iniFd != NULL)
    {
        std::string key = section;
        if (entry != NULL && entry[0] != NULL)
        {
            key += ":";
            key += entry;
        }

        if (0 == iniparser_set((dictionary *)m_iniFd, key.c_str(), value))
            result = true;
    }
    return result;
}

int CSimpleIni::GetInt(const char* section, const char * entry, int defaultValue)
{
    int value;
    if (section == NULL)
        return defaultValue;

    if (m_iniFd != NULL)
    {
        std::string key = section;
        if (entry != NULL && entry[0] != NULL)
        {
            key += ":";
            key += entry;
        }

        value = iniparser_getint((dictionary *)m_iniFd, key.c_str(), defaultValue);
    }
    return value;
}

bool CSimpleIni::WriteInt(const char* section, const char* entry, int value)
{
    char buff[64];
    sprintf(buff, "%d", value);
    return WriteString(section, entry, buff);
}

bool CSimpleIni::GetBoolean(const char* section, const char* entry, bool defaultValue)
{
    bool value;
    if (section == NULL)
        return defaultValue;

    if (m_iniFd != NULL)
    {
        std::string key = section;
        if (entry != NULL && entry[0] != NULL)
        {
            key += ":";
            key += entry;
        }

        int r = iniparser_getint((dictionary *)m_iniFd, key.c_str(), defaultValue ? 1:0);
        value = (r!=0);
    }
    return value;
}

bool CSimpleIni::WriteBoolean(const char* section, const char* entry, bool value)
{
    char buff[2] = "";
    buff[0] = value ? '1' : '0';
    return WriteString(section, entry, buff);
}

} // namespace utils
