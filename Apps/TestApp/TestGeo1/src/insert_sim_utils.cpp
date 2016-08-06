#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS // Disable security warning message on MSVC
#endif

#include <time.h>
#ifndef _WIN32
#include <sys/time.h>
#endif
#include <string>
#include "insert_sim_utils.h"

using namespace std;

UTIL_GLOBALS GLOBALS;


bool init_globals()
{

    return true;
}
