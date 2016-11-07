#ifndef _SIMPLE_INI_H_
#define _SIMPLE_INI_H_

#include <string>

namespace utils {

class CSimpleIni
{
public:
    CSimpleIni(const char* pathname = NULL);
    virtual ~CSimpleIni(void);

public:
    std::string GetString(const char* section, const char* entry, const char * defaultValue);
    bool WriteString(const char* section, const char * entry, const char * value);

    int  GetInt(const char* section, const char * entry, int defaultValue);
    bool WriteInt(const char* section, const char * entry, int value);

    bool GetBoolean(const char* section, const char * entry, bool defaultValue);
    bool WriteBoolean(const char* section, const char * entry, bool value);

protected:
    std::string     m_strPathname;
    void *          m_iniFd;
};

}

#endif // _SIMPLE_INI_H_
