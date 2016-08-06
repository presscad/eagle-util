#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS // Disable security warning message on MSVC
#endif

#include "hdb_utils.h"

using namespace hdb;
using namespace std;

namespace hdb {

////////////////////////////////////////////////////////////////////////////////////////////////////

void PrintOdbcError(SQLSMALLINT handletype, const SQLHANDLE& handle)
{
    SQLCHAR sqlstate[1024];
    SQLCHAR message[1024];
    if(SQL_SUCCESS == SQLGetDiagRec(handletype, handle, 1, sqlstate, NULL, message, 1024, NULL)) {
        printf("Message: %s SQLSTATE: %s\n", message, sqlstate);
    }
}

std::string GetOdbcError(SQLSMALLINT handletype, const SQLHANDLE& handle)
{
    SQLCHAR sqlstate[1024];
    SQLCHAR message[1024];
    if(SQL_SUCCESS == SQLGetDiagRec(handletype, handle, 1, sqlstate, NULL, message, 1024, NULL)) {
        char buff[1024];
        sprintf(buff, "Message: %s SQLSTATE: %s", message, sqlstate);
        return buff;
    }
    return "";
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<char, T_TINYINT> &col)
{
    SQLLEN *ind_vec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : NULL;
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_TINYINT, SQL_TINYINT,
        0, 0, (SQLPOINTER)col.GetData(), 0, ind_vec);
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<short, T_SMALLINT> &col)
{
    SQLLEN *ind_vec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : NULL;
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_SHORT, SQL_SMALLINT,
        0, 0, (SQLPOINTER)col.GetData(), 0, ind_vec);
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<int, T_INTEGER> &col)
{
    SQLLEN *ind_vec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : NULL;
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER,
        0, 0, (SQLPOINTER)col.GetData(), 0, ind_vec);
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<SQLBIGINT, T_BIGINT> &col)
{
    SQLLEN *ind_vec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : NULL;
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT,
        0, 0, (SQLPOINTER)col.GetData(), 0, ind_vec);
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<float, T_REAL> &col)
{
    SQLLEN *ind_vec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : NULL;
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_FLOAT, SQL_REAL,
        0, 0, (SQLPOINTER)col.GetData(), 0, ind_vec);
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<double, T_DOUBLE> &col)
{
    SQLLEN *ind_vec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : NULL;
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_DOUBLE, SQL_DOUBLE,
        0, 0, (SQLPOINTER)col.GetData(), 0, ind_vec);
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<double, T_FLOAT> &col)
{
    SQLLEN *ind_vec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : NULL;
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_DOUBLE, SQL_FLOAT,
        0, 0, (SQLPOINTER)col.GetData(), 0, ind_vec);
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<SQL_DATE_STRUCT, T_DATE> &col)
{
    UnImplemented("SqlBindInParam");
    return 0;
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<SQL_TIME_STRUCT, T_TIME> &col)
{
    UnImplemented("SqlBindInParam");
    return 0;
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<SQL_TIMESTAMP_STRUCT, T_TIMESTAMP> &col)
{
    SQLLEN *ind_vec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : NULL;
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP,
        0, 0, (SQLPOINTER)col.GetData(), 0, ind_vec);
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<SQL_TIMESTAMP_STRUCT, T_SECONDDATE> &col)
{
    SQLLEN *ind_vec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : NULL;
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP,
        0, 0, (SQLPOINTER)col.GetData(), 0, ind_vec);
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<double, T_SMALLDECIMAL> &col)
{
    SQLLEN *ind_vec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : NULL;
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_DOUBLE, SQL_DOUBLE,
        0, 0, (SQLPOINTER)col.GetData(), 0, ind_vec);
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<double, T_DECIMAL> &col)
{
    SQLLEN *ind_vec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : NULL;
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_DOUBLE, SQL_DOUBLE,
        0, 0, (SQLPOINTER)col.GetData(), 0, ind_vec);
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<double, T_DECIMAL_PS> &col)
{
    SQLLEN *ind_vec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : NULL;
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_DOUBLE, SQL_DOUBLE,
        0, 0, (SQLPOINTER)col.GetData(), 0, ind_vec);
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const CharColT<SQLWCHAR, T_CHAR> &col)
{
    SQLULEN ColumnSize = col.GetDataAttr().a; // http://msdn.microsoft.com/en-us/library/ms711786.aspx
    SQLLEN BufferLength = 2 * (col.GetDataAttr().a + 1); // http://msdn.microsoft.com/en-us/library/ms710963.aspx, see "BufferLength Argument"
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_CHAR,
        ColumnSize, 0, (SQLPOINTER)col.GetData(), BufferLength, (SQLLEN *)col.GetStrLenOrIndVec());
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const CharColT<SQLWCHAR, T_VARCHAR> &col) {
    SQLULEN ColumnSize = col.GetDataAttr().a;
    SQLLEN BufferLength = 2 * (col.GetDataAttr().a + 1);
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_VARCHAR,
        ColumnSize, 0, (SQLPOINTER)col.GetData(), BufferLength, (SQLLEN *)col.GetStrLenOrIndVec());
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const CharColT<SQLWCHAR, T_NCHAR> &col)
{
    SQLULEN ColumnSize = col.GetDataAttr().a;
    SQLLEN BufferLength = 2 * (col.GetDataAttr().a + 1);
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WCHAR,
        ColumnSize, 0, (SQLPOINTER)col.GetData(), BufferLength, (SQLLEN *)col.GetStrLenOrIndVec());
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const CharColT<SQLWCHAR, T_NVARCHAR> &col)
{
    SQLULEN ColumnSize = col.GetDataAttr().a;
    SQLLEN BufferLength = 2 * (col.GetDataAttr().a + 1);
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR,
        ColumnSize, 0, (SQLPOINTER)col.GetData(), BufferLength, (SQLLEN *)col.GetStrLenOrIndVec());
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const CharColT<SQLCHAR, T_ALPHANUM> &col)
{
    SQLULEN ColumnSize = col.GetDataAttr().a;
    SQLLEN BufferLength = col.GetDataAttr().a + 1;
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR,
        ColumnSize, 0, (SQLPOINTER)col.GetData(), BufferLength, (SQLLEN *)col.GetStrLenOrIndVec());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class OdbcConn

bool OdbcConn::Connect()
{
    SQLRETURN rc = SQL_SUCCESS;

    if (!mHenv) {
        rc = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &mHenv);
        if (SQL_SUCCEEDED(rc)) {
            rc = SQLSetEnvAttr(mHenv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
        }
    }
    if (!mHdbc) {
        if (SQL_SUCCEEDED(rc)) {
            rc = SQLAllocHandle(SQL_HANDLE_DBC, mHenv, &mHdbc);
        }
        if (SQL_SUCCEEDED(rc)) {
            rc = SQLSetConnectAttr(mHdbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, SQL_IS_UINTEGER);
        }
    }
#ifndef FAKE_DB_CONN
    /* Connect to the database */
    if (SQL_SUCCEEDED(rc)) {
        rc = SQLConnect(mHdbc, (SQLCHAR*)mDsn.c_str(), SQL_NTS,
            (SQLCHAR*)mUser.c_str(), SQL_NTS,
            (SQLCHAR*)mPassword.c_str(), SQL_NTS);
    }
#endif //FAKE_DB_CONN
    mConnected = SQL_SUCCEEDED(rc);
    return mConnected;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class InsertExecutor

bool InsertExecutor::GetInsStmt(const std::vector<BaseColumn *> &pCols, const char *table_name, std::string &stmt)
{
    char ins_into[1024] = "INSERT INTO ";
    char values[512] = "VALUES (";

    strncat(ins_into, table_name, sizeof(ins_into));
    strncat(ins_into, " (", sizeof(ins_into));

    size_t size = pCols.size();
    for (size_t i = 0; i < size; i++) {
        strncat(ins_into, pCols[i]->GetColName(), sizeof(ins_into));
        if (i == size - 1) {
            strncat(ins_into, ") ", sizeof(ins_into));
            strncat(values, "?)", sizeof(values));
        } else {
            strncat(ins_into, ", ", sizeof(ins_into));
            strncat(values, "?,", sizeof(values));
        }
    }
    strncat(ins_into, values, sizeof(ins_into));
    stmt = ins_into;
    return true;
}

bool InsertExecutor::PrepareInsStmt(const char *sSqlStmt) const
{
    SQLRETURN rc = SQL_SUCCESS;
#ifndef FAKE_DB_CONN
    rc = ::SQLPrepare(mHstmt, (SQLCHAR *)sSqlStmt, SQL_NTS);
#endif
    return SQL_SUCCEEDED(rc);
}

bool InsertExecutor::PrepareInsStmt(const std::vector<BaseColumn *> &pCols, const char *table_name) const
{
    std::string ins_into;
    GetInsStmt(pCols, table_name, ins_into);
    SQLRETURN rc = SQL_SUCCESS;
#ifndef FAKE_DB_CONN
    rc = ::SQLPrepare(mHstmt, (SQLCHAR *)ins_into.c_str(), SQL_NTS);
#endif
    return SQL_SUCCEEDED(rc);
}

bool InsertExecutor::ExecuteInsert(const ColRecords &records) const
{
    SQLULEN ParamsProcessed = 0;
    SQLRETURN rc;

    rc = SQLSetStmtAttr(mHstmt, SQL_ATTR_PARAM_BIND_TYPE, (SQLPOINTER)SQL_PARAM_BIND_BY_COLUMN, 0);
    if (SQL_SUCCEEDED(rc)) {
        SQLUINTEGER count = (SQLUINTEGER)records.GetRowCount();
        rc = SQLSetStmtAttr(mHstmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)count, 0);
    }
    if (SQL_SUCCEEDED(rc)) {
        rc = SQLSetStmtAttr(mHstmt, SQL_ATTR_PARAMS_PROCESSED_PTR, &ParamsProcessed, 0);
    }

    /* Bind the parameters in the column-wise fashion. */
    if (SQL_SUCCEEDED(rc)) {
        rc = records.BindAllInColumns(mHstmt);
    }

#ifndef FAKE_DB_CONN
    if (SQL_SUCCEEDED(rc)) {
        rc = SQLExecute(mHstmt);
    }
#endif
    if (SQL_SUCCEEDED(rc)) {
        rc = SQLEndTran(SQL_HANDLE_DBC, mpConn->GetHDbc(), SQL_COMMIT);
    }
    return SQL_SUCCEEDED(rc);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class FetchExecutor

static void PartialRecordsReady(FetchExecutor *executor, const ColRecords *partialRecords, void *pUser)
{
    ColRecords *pAllRecords = (ColRecords *)pUser;
    pAllRecords->AddRows(*partialRecords);
}

bool FetchExecutor::ExecuteFetchAll(ColRecords &records)
{
    SQLHSTMT hstmt = GetHStmt();
    SQLRETURN rc;

    rc = SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_BIND_TYPE, SQL_BIND_BY_COLUMN, 0);
/*
    SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_ARRAY_SIZE, ROW_ARRAY_SIZE, 0);
    SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_STATUS_PTR, RowStatusArray, 0);
    SQLSetStmtAttr(hstmt, SQL_ATTR_ROWS_FETCHED_PTR, &NumRowsFetched, 0);
*/
    if (SQL_SUCCEEDED(rc)) {
        rc = SQLExecDirect(hstmt, (SQLCHAR *)mSql.c_str(), SQL_NTS);
    }
    if (SQL_SUCCEEDED(rc)) {
        records.ClearAllCols();

        SQLUSMALLINT    ColumnNumber = 1;
        SQLCHAR         ColumnName[512];
        SQLSMALLINT     NameLength;
        SQLSMALLINT     DataType;
        SQLULEN         ColumnSize;
        SQLSMALLINT     DecimalDigits;
        SQLSMALLINT     Nullable;

        while (SQL_SUCCEEDED(rc)) {
            rc = SQLDescribeCol(hstmt, ColumnNumber, ColumnName, sizeof(ColumnName), &NameLength,
                &DataType, &ColumnSize, &DecimalDigits, &Nullable);
            if (SQL_SUCCEEDED(rc)) {
                ColumnName[sizeof(ColumnName)-1] = '\0';
                //DATA_ATTR_T attr;
                //hdb::UnImplemented("Get attr from SQLDescribeCol"); // TODO:
                //records.AddCol((const char *)ColumnName, attr);
            } else {
                break;
            }
            ColumnNumber++;
        }

        rc = SQL_SUCCESS;
    }

    if (SQL_SUCCEEDED(rc)) {
        records.ClearAllRows();
        ExecuteFetchInParts(PartialRecordsReady, &records, 5000);
    }
    return SQL_SUCCEEDED(rc);
}

// See http://msdn.microsoft.com/en-us/library/windows/desktop/ms713541(v=vs.85).aspx
bool FetchExecutor::ExecuteFetchInParts(OnPartialRecordsReady fun, void *pUser, int partialRowNum)
{
    ColRecords partials;

    //hdb::UnImplemented("FetchExecutor::ExecuteFetchInParts");
    return true;
}

} // end of namespace hdb
