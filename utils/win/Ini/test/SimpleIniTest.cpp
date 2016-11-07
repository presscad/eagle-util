
#if !defined(_MSC_VER)
#error currently support MS Visual C++ only
#endif

#include "afx.h"
#include "Ini/SimpleIni.h"

#if defined(_MSC_VER) && defined(_DEBUG)
#define new DEBUG_NEW
#endif

namespace utils {

void DoIniTest()
{
    CSimpleIni sini;
    sini.WriteBoolean("test", "bool", true);
    bool bValue = sini.GetBoolean("test", "bool", false);
    VERIFY(bValue == true);

    int iValue;
    iValue = sini.GetInt("test", "not-existing", 20);
    VERIFY(iValue == 20);
    sini.WriteInt("test", "int", -100);
    iValue = sini.GetInt("test", "int", 0);
    VERIFY(iValue == -100);

    sini.WriteInt("test", "int", 1);
    bValue = sini.GetBoolean("test", "int", false);
    VERIFY(bValue == true);
    sini.WriteInt("test", "int", 0);
    bValue = sini.GetBoolean("test", "int", true);
    VERIFY(bValue == false);

    std::string strValue = sini.GetString("test", "not-existing", "");
    VERIFY(strValue == "");
    strValue = sini.GetString("test", "not-existing", "abc");
    VERIFY(strValue == "abc");
    sini.WriteString("test", "str", "test-string");
    strValue = sini.GetString("test", "str", "");
    VERIFY(strValue == "test-string");
}

} // namespace utils
