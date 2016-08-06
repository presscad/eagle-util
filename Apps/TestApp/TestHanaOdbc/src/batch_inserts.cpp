#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS // Disable security warning message on MSVC
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include "hdb_utils.h"
#include "insert_sim_utils.h"


using namespace std;
using namespace hdb;

static OdbcConn *gpOdbcConn = NULL;
static PARSED_TABLE_T gParsedTable;

static std::ifstream gInCsv;

int bulk_insert_destory() {
    if (gpOdbcConn) {
        delete gpOdbcConn;
        gpOdbcConn = NULL;
    }

    return 0;
}

bool bulk_insert_init() {
    bulk_insert_destory();

    string err;
    if (!ParseTableFromSql(GLOBALS.CREATE_TABLE_SQL.c_str(), gParsedTable, err)) {
        printf("Error in parsing SQL: %s\n", GLOBALS.CREATE_TABLE_SQL.c_str());
        if (!err.empty()) {
            printf("Error: %s\n", err.c_str());
        }
        return false;
    }

    gpOdbcConn = new OdbcConn(GLOBALS.DSN.c_str(), GLOBALS.USER.c_str(), GLOBALS.PASSWORD.c_str());
    if (!gpOdbcConn) return false;

    if (!gpOdbcConn->Connect()) {
        printf("Error in SQLConnect(): %s\n", gpOdbcConn->GetDbcErrorStr().c_str());
        return false;
    }

    return true;
}

static void PrintStatus(size_t rec_count)
{
    static CONSOLE_SCREEN_BUFFER_INFO sConsoleInfo;
    static size_t s_total_count = 0;
    if (sConsoleInfo.dwSize.X == 0) {
        ::GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &sConsoleInfo);
    }
    ::SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), sConsoleInfo.dwCursorPosition);
    s_total_count += rec_count;
    printf("\r%s: processing #%lld\n", ElapsedTimeStr().c_str(), (long long)s_total_count);
}

static
void *insert_executor(void *arg) {
    SQLRETURN rc;
    long t_index = (long)arg;
    printf("%s: Enter insert_executor()\n", ElapsedTimeStr().c_str());

    InsertExecutor ins_exe(gpOdbcConn);
    ColRecords records;
    if (false == records.AddColsFromCreateSql(GLOBALS.CREATE_TABLE_SQL.c_str())) {
        printf("Error in parsing SQL: %s\n", GLOBALS.CREATE_TABLE_SQL.c_str());
        return false;
    }

    std::string ins_stmt;
    ins_exe.GetInsStmt(records.GetColumns(), (gParsedTable.schema + '.' + gParsedTable.table_name).c_str(), ins_stmt);
    if (ins_stmt.empty()) {
        printf("Error in getting of insert statement: %s\n");
        return false;
    }

    while(true) {
        rc = SQLPrepare(ins_exe.GetHStmt(), (SQLCHAR *)ins_stmt.c_str(), SQL_NTS);
        if (rc == SQL_ERROR) {
            printf("Error: %s\n", ins_exe.GetErrorStr().c_str());
            return NULL;
        }

        /* Prepare rows to insert */
        records.ClearAllRows();
        if (0 == records.AddRows(gInCsv, GLOBALS.N_RECORDS)) {
            break;
        }
        PrintStatus(records.GetRowCount());

        if (false == ins_exe.ExecuteInsert(records)) {
            printf("Error: %s\n", ins_exe.GetErrorStr().c_str());
            return NULL;
        }
    }

    PrintStatus(records.GetRowCount());
    return NULL;
}

bool bulk_insert(const char *csv)
{
    gInCsv.open(csv);
    if (!gInCsv.good()) {
        printf("Error: cannot open file %s\n", csv);
        return false;
    }

    if (false == bulk_insert_init()) {
        bulk_insert_destory();
        return false;
    }

    long t = 0;
    insert_executor(&t);
    bulk_insert_destory();
    gInCsv.close();
    return true;
}
