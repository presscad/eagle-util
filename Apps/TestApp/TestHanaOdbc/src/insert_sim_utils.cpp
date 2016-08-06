#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS // Disable security warning message on MSVC
#endif

#include <time.h>
#ifndef _WIN32
#include <sys/time.h>
#endif
#include <string>
#include "insert_sim_utils.h"
extern "C" {
#include "iniparser.h"
}

using namespace std;

UTIL_GLOBALS GLOBALS;

static
bool read_ini_string(dictionary *dict, const char*section, string &buff) {
    char *value = iniparser_getstring(dict, section, NULL);
    if (NULL == value) {
        printf("ERROR: cannot find %s in config file!\n", section);
        return false;
    }
    buff = value;
    return true;
}

static
bool read_ini_int(dictionary *dict, const char*section, int &value) {
    value = iniparser_getint(dict, section, 0xCDCDCDCD);
    if ((unsigned int)value == 0xCDCDCDCD) {
        printf("ERROR: cannot find %s in config file!\n", section);
        return false;
    }
    return true;
}

static
bool read_configs_from_ini(const char *config_file, UTIL_GLOBALS *p_globals) {
    dictionary *dict = iniparser_load(config_file);
    if (NULL == dict) {
        printf("ERROR: cannot open config file: %s\n", config_file);
        return false;
    }

    if (false == read_ini_string(dict, "DB:DSN", p_globals->DSN)) {
        iniparser_freedict(dict);
        return false;
    }

    if (false == read_ini_string(dict, "DB:USER", p_globals->USER)) {
        iniparser_freedict(dict);
        return false;
    }

    if (false == read_ini_string(dict, "DB:PASSWORD", p_globals->PASSWORD)) {
        iniparser_freedict(dict);
        return false;
    }

    if (false == read_ini_string(dict, "DB:CREATE_TABLE_SQL", p_globals->CREATE_TABLE_SQL)) {
        iniparser_freedict(dict);
        return false;
    }

    if (false == read_ini_int(dict, "main:N_RECORDS", p_globals->N_RECORDS)) {
        iniparser_freedict(dict);
        return false;
    }

    if (false == read_ini_string(dict, "main:CSV_PATH", p_globals->CSV_PATH)) {
        iniparser_freedict(dict);
        return false;
    }

    iniparser_freedict(dict);
    return true;
}

bool init_globals(const char *config_file) {
    if (false == read_configs_from_ini(config_file, &GLOBALS))
        return false;

    return true;
}
