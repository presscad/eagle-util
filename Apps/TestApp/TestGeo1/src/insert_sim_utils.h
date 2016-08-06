#ifndef __INSERT_SIM_UTILS_H_
#define __INSERT_SIM_UTILS_H_

#include <string>
#include "geo/geo_utils.h"


struct UTIL_GLOBALS {
    // config
    std::string DSN;
    std::string USER;
    std::string PASSWORD;
    std::string TABLENAME;
    std::string CREATE_TABLE_SQL;

    int  N_RECORDS;
    std::string CSV_PATH;
};

extern UTIL_GLOBALS GLOBALS;

bool init_globals();


#endif // __INSERT_SIM_UTILS_H_
