#ifndef __INSERT_SIM_UTILS_H_
#define __INSERT_SIM_UTILS_H_

#ifdef _WIN32
#include "windows.h"
#endif
#include "sqlext.h"
#include <string>
#include <fstream>

// for debug purpose only, comment it out for real DB connection
//#define FAKE_DB_CONN

#define CONFIG_FILE "TestHanaOdbc.ini"


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

bool init_globals(const char *config_file);


#endif // __INSERT_SIM_UTILS_H_
