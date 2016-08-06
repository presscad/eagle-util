#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <string>
#include <direct.h>
#include <io.h>
#include <fstream>
#include "HotSquare.h"
#include "hdb/hdb_utils.h"

using namespace std;


bool GetLine(std::ifstream &fs, std::string &line) {
    line.clear();
    do{
        if(getline(fs, line)) {
            if(fs.good() && line.empty()){
                continue;
            }
            return true;
        } else {
            return false;
        }
    } while(true);
    return false;
}

std::string FormatTimeStr(unsigned long uTimeMs)
{
    char buff[64];
    sprintf(buff, "%2d:%02d:%03d",
        (uTimeMs/60000), (uTimeMs/1000) % 60, uTimeMs % 1000);
    return buff;
}

static unsigned int g_dwStart = ::GetTickCount();
std::string ElapsedTimeStr() {
    return FormatTimeStr(::GetTickCount() - g_dwStart);
}

// Get the distance on earth surface between two points
// Refer to http://blog.sina.com.cn/s/blog_65d859c30101akih.html
double GetDistanceInMeter(double lat1, double lng1, double lat2, double lng2)
{
    double radLat1 = rad(lat1);
    double radLat2 = rad(lat2);
    double a = radLat1 - radLat2;
    double b = rad(lng1) - rad(lng2);
    double sin_a_2 = sin(a/2);
    double sin_b_2 = sin(b/2);
    double s = 2 * asin(sqrt(sin_a_2 * sin_a_2 + cos(radLat1) * cos(radLat2) * (sin_b_2 * sin_b_2)));
    return s * R_EARTH;
}

// Get the distance on earth surface between two points with the same lat coordinate
double GetDistanceSameLatInMeter(double lat, double lng1, double lng2)
{
    double r = (R_EARTH * M_PI / 180) * cos(rad(lat)) * (lng2 - lng1);
    return r < 0 ? -r : r;
}

// Get the distance on earth surface between two points with the same lng coordinate
double GetDistanceSameLngInMeter(double lat1, double lat2)
{
    double r = (R_EARTH * M_PI / 180) * (lat2 - lat1);
    return r < 0 ? -r : r;
}

void StringReplace(string &strBase, const string &strSrc, const string &strDes)
{
    string::size_type pos = 0;
    string::size_type srcLen = strSrc.size();
    string::size_type desLen = strDes.size();
    pos = strBase.find(strSrc, pos); 
    while ((pos != string::npos)) {
        strBase.replace(pos, srcLen, strDes);
        pos=strBase.find(strSrc, (pos+desLen));
    }
}

bool CreateDirNested(const char *pDir)
{
    int iRet = 0;
    int iLen;
    char* pszDir;

    if(NULL == pDir) {
        return false;
    }
    if (_access(pDir, 0) == 0) {
        return true;
    }

    pszDir = _strdup(pDir);
    iLen = (int)strlen(pszDir);

    for (int i = 0;i < iLen; i++) {
        if (pszDir[i] == '\\' || pszDir[i] == '/') { 
            pszDir[i] = '\0';

            // create the file if it does not exist
            iRet = _access(pszDir, 0);
            if (iRet != 0) {
                iRet = _mkdir(pszDir);
                if (iRet != 0) {
                    return false;
                }
            }
            pszDir[i] = '/'; // Replace '\' with '/' to support Linux
        }
    }
    
    if (_access(pszDir, 0) != 0) {
        iRet = _mkdir(pszDir);
    }
    free(pszDir);
    return iRet == 0;
}

void CsvLinePopulate(vector<string> &record, const string &line, char delimiter)
{
    util::ParseCsvLine(record, line.c_str(), delimiter);
}

bool read_ini_int(dictionary *dict, const char*section, int &value) {
    value = iniparser_getint(dict, section, 0xCDCDCDCD);
    if ((unsigned int)value == 0xCDCDCDCD) {
        printf("ERROR: cannot find %s in config file!\n", section);
        return false;
    }
    return true;
}

bool read_ini_string(dictionary *dict, const char*section, std::string &str) {
    char *value = iniparser_getstring(dict, section, NULL);
    if (NULL == value) {
        printf("ERROR: cannot find %s in config file!\n", section);
        return false;
    }
    str = value;
    return true;
}
