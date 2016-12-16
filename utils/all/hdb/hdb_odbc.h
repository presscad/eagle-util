#ifndef _HDB_ODBC_H
#define _HDB_ODBC_H

#include <memory>
#include <string>
#ifdef _WIN32
#include <windows.h> // required by sqlext.h for WIN32
#endif
#include <sqlext.h>
#include "hdb_columns.h"


HDB_BEGIN_NAMESPACE

////////////////////////////////////////////////////////////////////////////////////////////////////
void PrintOdbcError(SQLSMALLINT handletype, const SQLHANDLE& handle);
std::string GetOdbcError(SQLSMALLINT handletype, const SQLHANDLE& handle);

template<class T, DATA_TYPE_T data_type> class ColT;
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<unsigned char, T_TINYINT> &col);
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<short, T_SMALLINT> &col);
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<int, T_INTEGER> &col);
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<SQLBIGINT, T_BIGINT> &col);
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<float, T_REAL> &col);
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<double, T_DOUBLE> &col);
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<double, T_FLOAT> &col);
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<SQL_DATE_STRUCT, T_DATE> &col);
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<SQL_TIME_STRUCT, T_TIME> &col);
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<SQL_TIMESTAMP_STRUCT, T_TIMESTAMP> &col);
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<SQL_TIMESTAMP_STRUCT, T_SECONDDATE> &col);
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<double, T_SMALLDECIMAL> &col);
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<double, T_DECIMAL> &col);

SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<unsigned char, T_TINYINT> &col);
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<short, T_SMALLINT> &col);
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<int, T_INTEGER> &col);
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<SQLBIGINT, T_BIGINT> &col);
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<float, T_REAL> &col);
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<double, T_DOUBLE> &col);
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<double, T_FLOAT> &col);
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<SQL_DATE_STRUCT, T_DATE> &col);
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<SQL_TIME_STRUCT, T_TIME> &col);
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<SQL_TIMESTAMP_STRUCT, T_TIMESTAMP> &col);
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<SQL_TIMESTAMP_STRUCT, T_SECONDDATE> &col);
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<double, T_SMALLDECIMAL> &col);
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<double, T_DECIMAL> &col);


template<class T, DATA_TYPE_T data_type> class CharColT;
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const CharColT<SQLWCHAR, T_CHAR> &col);
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const CharColT<SQLWCHAR, T_VARCHAR> &col);
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const CharColT<SQLCHAR, T_CHAR> &col);
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const CharColT<SQLCHAR, T_VARCHAR> &col);
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const CharColT<SQLWCHAR, T_NCHAR> &col);
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const CharColT<SQLWCHAR, T_NVARCHAR> &col);
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const CharColT<SQLCHAR, T_ALPHANUM> &col);
// special: DECIMAL(p,s) mapped to chars instead of cumbersome SQL_NUMERIC_STRUCT
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const CharColT<SQLCHAR, T_DECIMAL_PS> &col);

SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, CharColT<SQLWCHAR, T_CHAR> &col);
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, CharColT<SQLWCHAR, T_VARCHAR> &col);
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, CharColT<SQLCHAR, T_CHAR> &col);
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, CharColT<SQLCHAR, T_VARCHAR> &col);
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, CharColT<SQLWCHAR, T_NCHAR> &col);
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, CharColT<SQLWCHAR, T_NVARCHAR> &col);
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, CharColT<SQLCHAR, T_ALPHANUM> &col);
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, CharColT<SQLCHAR, T_DECIMAL_PS> &col);


// Dummy functions for CharColT derived classes, do not use them!
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<SQLCHAR, T_CHAR> &col);
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<SQLCHAR, T_VARCHAR> &col);
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<SQLWCHAR, T_CHAR> &col);
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<SQLWCHAR, T_NCHAR> &col);
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<SQLWCHAR, T_VARCHAR> &col);
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<SQLWCHAR, T_NVARCHAR> &col);
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<SQLCHAR, T_ALPHANUM> &col);
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<SQLCHAR, T_DECIMAL_PS> &col);

SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<SQLCHAR, T_CHAR> &col);
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<SQLCHAR, T_VARCHAR> &col);
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<SQLWCHAR, T_CHAR> &col);
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<SQLWCHAR, T_NCHAR> &col);
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<SQLWCHAR, T_VARCHAR> &col);
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<SQLWCHAR, T_NVARCHAR> &col);
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<SQLCHAR, T_ALPHANUM> &col);
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<SQLCHAR, T_DECIMAL_PS> &col);


// Dummy functions for LongVarColT derived classes, do not use them!
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<unsigned char, T_BLOB> &col);
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<unsigned char, T_CLOB> &col);
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<SQLWCHAR, T_NCLOB> &col);
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<SQLCHAR, T_TEXT> &col);
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<unsigned char, T_ST_GEOMETRY> &col);

SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<unsigned char, T_BLOB> &col);
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<unsigned char, T_CLOB> &col);
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<SQLWCHAR, T_NCLOB> &col);
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<SQLCHAR, T_TEXT> &col);
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<unsigned char, T_ST_GEOMETRY> &col);


template<class T, DATA_TYPE_T data_type> class LongVarColT;
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const LongVarColT<unsigned char, T_BLOB> &col);
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const LongVarColT<unsigned char, T_CLOB> &col);
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const LongVarColT<SQLWCHAR, T_NCLOB> &col);
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const LongVarColT<SQLCHAR, T_TEXT> &col);
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const LongVarColT<unsigned char, T_ST_GEOMETRY> &col);

SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, LongVarColT<unsigned char, T_BLOB> &col);
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, LongVarColT<unsigned char, T_CLOB> &col);
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, LongVarColT<SQLWCHAR, T_NCLOB> &col);
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, LongVarColT<SQLCHAR, T_TEXT> &col);
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, LongVarColT<unsigned char, T_ST_GEOMETRY> &col);


////////////////////////////////////////////////////////////////////////////////////////////////////
class OdbcConn
{
public:
    explicit OdbcConn(const char* dsn, const char* user, const char* password)
        : mDsn(dsn), mUser(user), mPassword(password)
    {}
    explicit OdbcConn(const std::string& dsn, const std::string& user, const std::string& password)
        : mDsn(dsn), mUser(user), mPassword(password)
    {}
    ~OdbcConn()
    {
        DisConnect();
    }

    SQLHENV GetHEnv() const
    {
        return mHenv;
    }

    SQLHDBC GetHDbc() const
    {
        return mHdbc;
    }

    std::string GetDbcErrorStr() const
    {
        return GetOdbcError(SQL_HANDLE_DBC, mHdbc);
    }

    bool Connect();
    void DisConnect();

    bool IsConnected() const
    {
        return mConnected;
    };

protected:
    std::string mDsn;
    std::string mUser;
    std::string mPassword;
    SQLHENV mHenv{};
    SQLHDBC mHdbc{};
    bool mConnected{};
};

class BaseColumn;
typedef std::shared_ptr<BaseColumn> BaseColumn_SharedPtr;
class ColRecords;
class InsertExecutor
{
public:
    explicit InsertExecutor(OdbcConn *pConn)
        : mpConn(pConn)
    {
        SQLAllocHandle(SQL_HANDLE_STMT, pConn->GetHDbc(), &mHstmt);
    };
    ~InsertExecutor()
    {
        if (mHstmt)
        {
            SQLFreeHandle(SQL_HANDLE_STMT, mHstmt);
            mHstmt = nullptr;
        }
        mpConn = nullptr;
    };
    SQLHSTMT GetHStmt() const
    {
        return mHstmt;
    };
    std::string GetErrorStr() const
    {
        std::string err = GetOdbcError(SQL_HANDLE_STMT, mHstmt);
        return err.empty() ? mErrStr : err;
    };
    static bool GetInsStmt(const std::vector<BaseColumn_SharedPtr> &pCols, const char *sTableName, std::string &stmt);
    static bool GetInsStmtNoColumns(const std::vector<BaseColumn_SharedPtr> &pCols, const char *sTableName, std::string &stmt);
    bool PrepareInsStmt(const char *sSqlStmt) const;
    bool PrepareInsStmt(const std::vector<BaseColumn_SharedPtr> &pCols, const char *sTableName) const;
    bool ExecuteInsert(const ColRecords &records, bool commit = true) const;

protected:
    OdbcConn *mpConn{};
    SQLHSTMT mHstmt;
    mutable std::string mErrStr;
};


class QueryExecutor
{
public:
    explicit QueryExecutor(OdbcConn *pConn, const char *sql)
    {
        mpConn = pConn;
        if (sql) {
            mSql = sql;
        }
        SQLAllocHandle(SQL_HANDLE_STMT, pConn->GetHDbc(), &mHstmt);
    };
    explicit QueryExecutor(OdbcConn *pConn, const std::string& sql)
    {
        mpConn = pConn;
        if (!sql.empty()) {
            mSql = sql;
        }
        SQLAllocHandle(SQL_HANDLE_STMT, pConn->GetHDbc(), &mHstmt);
    };
    ~QueryExecutor()
    {
        if (mHstmt) {
            SQLFreeHandle(SQL_HANDLE_STMT, mHstmt);
            mHstmt = nullptr;
        }
        mpConn = nullptr;
        mSql.clear();
    };
    void SetSql(const char *sql)
    {
        if (sql) {
            mSql = sql;
        }
        else {
            mSql.clear();
        }
    };
    void SetSql(const std::string& sql)
    {
        mSql = sql;
    };
    const char *GetSql() const
    {
        return mSql.c_str();
    };
    const std::string& Sql() const
    {
        return mSql;
    };
    SQLHSTMT GetHStmt() const
    {
        return mHstmt;
    };
    std::string GetErrorStr() const
    {
        return GetOdbcError(SQL_HANDLE_STMT, mHstmt);
    };
    bool Execute(size_t row_array_size = 10 * 1024);
    int FetchRows(); // return actual fetched row count, -1 for error

    int FetchedColCount() const;
    std::string GetFetchedColName(int i_col) const;
    const ColRecords& GetBuffRecords() const
    {
        return *mRecordsBuff;
    }
    // return the number of lines actually into CSV
    int FetchedRowsToCsv(std::ostream &os_csv, char delimiter) const;

private:
    bool Prepare();

private:
    OdbcConn *mpConn{};
    SQLHSTMT mHstmt{};
    std::string mSql;
    std::shared_ptr<ColRecords> mRecordsBuff;
    size_t mNumRowsFetchedTotal{};
    SQLUINTEGER mNumRowsFetched{};
    std::vector<SQLUSMALLINT> mRowStatusArray;
};

HDB_END_NAMESPACE

#endif // _HDB_ODBC_H
