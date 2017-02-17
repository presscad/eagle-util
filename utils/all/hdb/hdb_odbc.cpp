#if defined(_WIN32) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS // Disable security warning message on MSVC
#endif

#include "hdb_utils.h"

USING_HDB_NAMESPACE
using namespace std;

HDB_BEGIN_NAMESPACE

////////////////////////////////////////////////////////////////////////////////////////////////////

void PrintOdbcError(SQLSMALLINT handletype, const SQLHANDLE& handle)
{
    SQLCHAR sqlstate[1024];
    SQLCHAR message[1024];
    if(SQL_SUCCESS == SQLGetDiagRec(handletype, handle, 1, sqlstate, nullptr, message, 1024, nullptr)) {
        printf("Message: %s SQLSTATE: %s\n", message, sqlstate);
    }
}

std::string GetOdbcError(SQLSMALLINT handletype, const SQLHANDLE& handle)
{
    SQLCHAR sqlstate[1024];
    SQLCHAR message[10240];
    if(SQL_SUCCESS == SQLGetDiagRec(handletype, handle, 1, sqlstate, nullptr, message, sizeof(message), nullptr)) {
        char buff[1024 * 2];
        sprintf(buff, "Message: %s SQLSTATE: %s", message, sqlstate);
        return buff;
    }
    return "";
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const TinyIntCol &col)
{
    SQLLEN *ind_vec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : nullptr;
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_UTINYINT, SQL_TINYINT,
        0, 0, (SQLPOINTER)col.GetData(), 0, ind_vec);
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, TinyIntCol &col)
{
    SQLLEN *ind_vec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : nullptr;
    return SQLBindCol(hstmt, ipar, SQL_C_UTINYINT, (SQLPOINTER)col.GetData(), 0, ind_vec);
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const SmallIntCol &col)
{
    SQLLEN *ind_vec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : nullptr;
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_SHORT, SQL_SMALLINT,
        0, 0, (SQLPOINTER)col.GetData(), 0, ind_vec);
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, SmallIntCol &col)
{
    SQLLEN *ind_vec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : nullptr;
    return SQLBindCol(hstmt, ipar, SQL_C_SHORT, (SQLPOINTER)col.GetData(), 0, ind_vec);
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const IntCol &col)
{
    SQLLEN *ind_vec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : nullptr;
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER,
        0, 0, (SQLPOINTER)col.GetData(), 0, ind_vec);
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, IntCol &col)
{
    SQLLEN *ind_vec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : nullptr;
    return SQLBindCol(hstmt, ipar, SQL_C_SLONG, (SQLPOINTER)col.GetData(), 0, ind_vec);
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const BigIntCol &col)
{
    SQLLEN *ind_vec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : nullptr;
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT,
        0, 0, (SQLPOINTER)col.GetData(), 0, ind_vec);
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, BigIntCol &col)
{
    SQLLEN *ind_vec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : nullptr;
    return SQLBindCol(hstmt, ipar, SQL_C_SBIGINT, (SQLPOINTER)col.GetData(), 0, ind_vec);
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const RealCol &col)
{
    SQLLEN *ind_vec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : nullptr;
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_FLOAT, SQL_REAL,
        0, 0, (SQLPOINTER)col.GetData(), 0, ind_vec);
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, RealCol &col)
{
    SQLLEN *ind_vec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : nullptr;
    return SQLBindCol(hstmt, ipar, SQL_C_FLOAT, (SQLPOINTER)col.GetData(), 0, ind_vec);
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const DoubleCol &col)
{
    SQLLEN *ind_vec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : nullptr;
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_DOUBLE, SQL_DOUBLE,
        0, 0, (SQLPOINTER)col.GetData(), 0, ind_vec);
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, DoubleCol &col)
{
    SQLLEN *ind_vec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : nullptr;
    return SQLBindCol(hstmt, ipar, SQL_C_DOUBLE, (SQLPOINTER)col.GetData(), 0, ind_vec);
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const FloatCol &col)
{
    SQLLEN *ind_vec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : nullptr;
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_DOUBLE, SQL_FLOAT,
        0, 0, (SQLPOINTER)col.GetData(), 0, ind_vec);
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, FloatCol &col)
{
    UnImplemented("SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, FloatCol &col)");
    return 0;
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const DateCol &col)
{
    SQLLEN *ind_vec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : nullptr;
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_TYPE_DATE, SQL_TYPE_DATE,
        0, 0, (SQLPOINTER)col.GetData(), 0, ind_vec);
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, DateCol &col)
{
    SQLLEN *ind_vec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : nullptr;
    return SQLBindCol(hstmt, ipar, SQL_C_TYPE_DATE, (SQLPOINTER)col.GetData(), 0, ind_vec);
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const TimeCol &col)
{
    SQLLEN *ind_vec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : nullptr;
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_TYPE_TIME, SQL_TYPE_TIME,
        0, 0, (SQLPOINTER)col.GetData(), 0, ind_vec);
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, TimeCol &col)
{
    SQLLEN *ind_vec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : nullptr;
    return SQLBindCol(hstmt, ipar, SQL_C_TYPE_TIME, (SQLPOINTER)col.GetData(), 0, ind_vec);
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const TimeStampCol &col)
{
    SQLLEN *ind_vec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : nullptr;
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP,
        0, 0, (SQLPOINTER)col.GetData(), 0, ind_vec);
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, TimeStampCol &col)
{
    SQLLEN *ind_vec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : nullptr;
    return SQLBindCol(hstmt, ipar, SQL_C_TYPE_TIMESTAMP, (SQLPOINTER)col.GetData(), 0, ind_vec);
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const SecondDateCol &col)
{
    SQLLEN *ind_vec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : nullptr;
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP,
        0, 0, (SQLPOINTER)col.GetData(), 0, ind_vec);
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, SecondDateCol &col)
{
    UnImplemented("SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, SecondDateCol &col)");
    return 0;
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const SmallDecimalCol &col)
{
    SQLLEN *ind_vec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : nullptr;
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_DOUBLE, SQL_DOUBLE,
        0, 0, (SQLPOINTER)col.GetData(), 0, ind_vec);
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, SmallDecimalCol &col)
{
    UnImplemented("SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, SmallDecimalCol &col)");
    return 0;
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const DecimalCol &col)
{
    SQLLEN *ind_vec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : nullptr;
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_DOUBLE, SQL_DOUBLE,
        0, 0, (SQLPOINTER)col.GetData(), 0, ind_vec);
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, DecimalCol &col)
{
    SQLLEN *ind_vec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : nullptr;
    return SQLBindCol(hstmt, ipar, SQL_C_DOUBLE, (SQLPOINTER)col.GetData(), 0, ind_vec);
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const DecimalPsCol &col)
{
    SQLULEN ColumnSize = col.GetDataAttr().a;
    SQLLEN BufferLength = col.GetDataAttr().a + 1;
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_DECIMAL,
        ColumnSize, 0, (SQLPOINTER)col.GetData(), BufferLength, (SQLLEN *)col.GetStrLenOrIndVec());
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, DecimalPsCol &col)
{
    SQLLEN BufferLength = col.GetDataAttr().a + 1;
    SQLLEN *IndVec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : nullptr;
    return SQLBindCol(hstmt, ipar, SQL_C_CHAR, (SQLPOINTER)col.GetData(), BufferLength, IndVec);
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const CharColT<SQLWCHAR, T_CHAR> &col)
{
    SQLULEN ColumnSize = col.GetDataAttr().a; // http://msdn.microsoft.com/en-us/library/ms711786.aspx
    SQLLEN BufferLength = 2 * (col.GetDataAttr().a + 1); // http://msdn.microsoft.com/en-us/library/ms710963.aspx,
                                                         // see "BufferLength Argument"
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_CHAR,
        ColumnSize, 0, (SQLPOINTER)col.GetData(), BufferLength, (SQLLEN *)col.GetStrLenOrIndVec());
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, CharColT<SQLWCHAR, T_CHAR> &col)
{
    SQLULEN ColumnSize = col.GetDataAttr().a;
    SQLLEN BufferLength = 2 * (col.GetDataAttr().a + 1);
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_CHAR,
        ColumnSize, 0, (SQLPOINTER)col.GetData(), BufferLength, (SQLLEN *)col.GetStrLenOrIndVec());
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const CharColT<SQLWCHAR, T_VARCHAR> &col)
{
    SQLULEN ColumnSize = col.GetDataAttr().a;
    SQLLEN BufferLength = 2 * (col.GetDataAttr().a + 1);
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_VARCHAR,
        ColumnSize, 0, (SQLPOINTER)col.GetData(), BufferLength, (SQLLEN *)col.GetStrLenOrIndVec());
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, CharColT<SQLWCHAR, T_VARCHAR> &col)
{
    SQLLEN BufferLength = 2 * (col.GetDataAttr().a + 1);
    SQLLEN *IndVec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : nullptr;
    return SQLBindCol(hstmt, ipar, SQL_C_WCHAR, (SQLPOINTER)col.GetData(), BufferLength, IndVec);
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const CharColT<SQLCHAR, T_CHAR> &col)
{
    SQLULEN ColumnSize = col.GetDataAttr().a;
    SQLLEN BufferLength = col.GetDataAttr().a + 1;
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR,
        ColumnSize, 0, (SQLPOINTER)col.GetData(), BufferLength, (SQLLEN *)col.GetStrLenOrIndVec());
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, CharColT<SQLCHAR, T_CHAR> &col)
{
    UnImplemented("SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, CharColT<SQLCHAR, T_CHAR> &col)");
    return 0;
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const CharColT<SQLCHAR, T_VARCHAR> &col)
{
    SQLULEN ColumnSize = col.GetDataAttr().a;
    SQLLEN BufferLength = col.GetDataAttr().a + 1;
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR,
        ColumnSize, 0, (SQLPOINTER)col.GetData(), BufferLength, (SQLLEN *)col.GetStrLenOrIndVec());
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, CharColT<SQLCHAR, T_VARCHAR> &col)
{
    SQLLEN BufferLength = col.GetDataAttr().a + 1;
    SQLLEN *IndVec = col.NullAble() ? (SQLLEN *)col.GetStrLenOrIndVec() : nullptr;
    return ::SQLBindCol(hstmt, ipar, SQL_C_CHAR, (SQLPOINTER)col.GetData(), BufferLength, IndVec);
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const CharColT<SQLWCHAR, T_NCHAR> &col)
{
    SQLULEN ColumnSize = col.GetDataAttr().a;
    SQLLEN BufferLength = 2 * (col.GetDataAttr().a + 1);
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WCHAR,
        ColumnSize, 0, (SQLPOINTER)col.GetData(), BufferLength, (SQLLEN *)col.GetStrLenOrIndVec());
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, CharColT<SQLWCHAR, T_NCHAR> &col)
{
    UnImplemented("SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, CharColT<SQLWCHAR, T_NCHAR> &col)");
    return 0;
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const CharColT<SQLWCHAR, T_NVARCHAR> &col)
{
    SQLULEN ColumnSize = col.GetDataAttr().a;
    SQLLEN BufferLength = 2 * (col.GetDataAttr().a + 1);
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR,
        ColumnSize, 0, (SQLPOINTER)col.GetData(), BufferLength, (SQLLEN *)col.GetStrLenOrIndVec());
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, CharColT<SQLWCHAR, T_NVARCHAR> &col)
{
    UnImplemented("SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, CharColT<SQLWCHAR, T_NVARCHAR> &col)");
    return 0;
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const CharColT<SQLCHAR, T_ALPHANUM> &col)
{
    SQLULEN ColumnSize = col.GetDataAttr().a;
    SQLLEN BufferLength = col.GetDataAttr().a + 1;
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR,
        ColumnSize, 0, (SQLPOINTER)col.GetData(), BufferLength, (SQLLEN *)col.GetStrLenOrIndVec());
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, CharColT<SQLCHAR, T_ALPHANUM> &col)
{
    UnImplemented("SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, CharColT<SQLCHAR, T_ALPHANUM> &col)");
    return 0;
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const LongVarColT<unsigned char, T_BLOB> &col)
{
    SQLULEN ColumnSize = SQL_MAX_LENGTH_DEFAULT;
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_LONGVARBINARY,
        ColumnSize, 0, reinterpret_cast<SQLPOINTER>(ipar), 0, (SQLLEN *)col.GetStrLenOrIndVec());
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, LongVarColT<unsigned char, T_BLOB> &col)
{
    UnImplemented("SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, LongVarColT<unsigned char, T_BLOB> &col)");
    return 0;
}

// Refer to http://stackoverflow.com/questions/12781553/odbc-write-blob-example-oracle-c
//          https://mariadb.com/kb/en/sqlputdata/
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const LongVarColT<unsigned char, T_CLOB> &col)
{
    SQLULEN ColumnSize = SQL_MAX_LENGTH_DEFAULT;
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_LONGVARCHAR,
        ColumnSize, 0, reinterpret_cast<SQLPOINTER>(ipar), 0, (SQLLEN *)col.GetStrLenOrIndVec());
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, LongVarColT<unsigned char, T_CLOB> &col)
{
    UnImplemented("SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, LongVarColT<unsigned char, T_CLOB> &col)");
    return 0;
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const LongVarColT<SQLWCHAR, T_NCLOB> &col)
{
    SQLULEN ColumnSize = SQL_MAX_LENGTH_DEFAULT;
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_LONGVARCHAR,
        ColumnSize, 0, reinterpret_cast<SQLPOINTER>(ipar), 0, (SQLLEN *)col.GetStrLenOrIndVec());
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, LongVarColT<SQLWCHAR, T_NCLOB> &col)
{
    UnImplemented("SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, LongVarColT<SQLWCHAR, T_NCLOB> &col)");
    return 0;
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const LongVarColT<SQLCHAR, T_TEXT> &col)
{
    SQLULEN ColumnSize = SQL_MAX_LENGTH_DEFAULT;
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_LONGVARCHAR,
        ColumnSize, 0, reinterpret_cast<SQLPOINTER>(ipar), 0, (SQLLEN *)col.GetStrLenOrIndVec());
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, LongVarColT<SQLCHAR, T_TEXT> &col)
{
    UnImplemented("SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, LongVarColT<SQLCHAR, T_TEXT> &col)");
    return 0;
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const LongVarColT<unsigned char, T_ST_GEOMETRY> &col)
{
    SQLULEN ColumnSize = SQL_MAX_LENGTH_DEFAULT;
    return SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_LONGVARCHAR,
        ColumnSize, 0, reinterpret_cast<SQLPOINTER>(ipar), 0, (SQLLEN *)col.GetStrLenOrIndVec());
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, LongVarColT<unsigned char, T_ST_GEOMETRY> &col)
{
    UnImplemented("SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, LongVarColT<unsigned char, T_ST_GEOMETRY> &col)");
    return 0;
}


// Dummy functions for CharColT derived classes, do not use them!
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<SQLCHAR, T_CHAR> &col)
{
    UnImplemented(__FUNCTION__);
    return 0;
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<SQLCHAR, T_CHAR> &col)
{
    UnImplemented(__FUNCTION__);
    return 0;
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<SQLCHAR, T_VARCHAR> &col)
{
    UnImplemented(__FUNCTION__);
    return 0;
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<SQLCHAR, T_VARCHAR> &col)
{
    UnImplemented(__FUNCTION__);
    return 0;
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<SQLWCHAR, T_CHAR> &col)
{
    UnImplemented(__FUNCTION__);
    return 0;
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<SQLWCHAR, T_CHAR> &col)
{
    UnImplemented(__FUNCTION__);
    return 0;
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<SQLWCHAR, T_NCHAR> &col)
{
    UnImplemented(__FUNCTION__);
    return 0;
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<SQLWCHAR, T_NCHAR> &col)
{
    UnImplemented(__FUNCTION__);
    return 0;
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<SQLWCHAR, T_VARCHAR> &col)
{
    UnImplemented(__FUNCTION__);
    return 0;
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<SQLWCHAR, T_VARCHAR> &col)
{
    UnImplemented(__FUNCTION__);
    return 0;
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<SQLWCHAR, T_NVARCHAR> &col)
{
    UnImplemented(__FUNCTION__);
    return 0;
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<SQLWCHAR, T_NVARCHAR> &col)
{
    UnImplemented(__FUNCTION__);
    return 0;
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<SQLCHAR, T_ALPHANUM> &col)
{
    UnImplemented(__FUNCTION__);
    return 0;
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<SQLCHAR, T_ALPHANUM> &col)
{
    UnImplemented(__FUNCTION__);
    return 0;
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<SQLCHAR, T_DECIMAL_PS> &col)
{
    UnImplemented(__FUNCTION__);
    return 0;
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<SQLCHAR, T_DECIMAL_PS> &col)
{
    UnImplemented(__FUNCTION__);
    return 0;
}

// for LongVarColT derived classes, do not use them!
SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<unsigned char, T_BLOB> &col)
{
    UnImplemented(__FUNCTION__);
    return 0;
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<unsigned char, T_BLOB> &col)
{
    UnImplemented(__FUNCTION__);
    return 0;
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<unsigned char, T_CLOB> &col)
{
    UnImplemented(__FUNCTION__);
    return 0;
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<unsigned char, T_CLOB> &col)
{
    UnImplemented(__FUNCTION__);
    return 0;
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<SQLWCHAR, T_NCLOB> &col)
{
    UnImplemented(__FUNCTION__);
    return 0;
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<SQLWCHAR, T_NCLOB> &col)
{
    UnImplemented(__FUNCTION__);
    return 0;
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<SQLCHAR, T_TEXT> &col)
{
    UnImplemented(__FUNCTION__);
    return 0;
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<SQLCHAR, T_TEXT> &col)
{
    UnImplemented(__FUNCTION__);
    return 0;
}

SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<unsigned char, T_ST_GEOMETRY> &col)
{
    UnImplemented(__FUNCTION__);
    return 0;
}
SQLRETURN SQLBindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar, ColT<unsigned char, T_ST_GEOMETRY> &col)
{
    UnImplemented(__FUNCTION__);
    return 0;
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

void OdbcConn::DisConnect()
{
    if (mHdbc) {
        if (mConnected) {
            SQLDisconnect(mHdbc);
            mConnected = false;
        }
        SQLFreeHandle(SQL_HANDLE_DBC, mHdbc);
        mHdbc = nullptr;
    }
    if (mHenv) {
        SQLFreeHandle(SQL_HANDLE_ENV, mHenv);
        mHenv = nullptr;
    }

    mConnected = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class InsertExecutor

bool InsertExecutor::GetInsStmt(const std::vector<BaseColumn_SharedPtr> &pCols,
                                const char *table_name, std::string &stmt)
{
    string ins_into = "INSERT INTO ";
    string values = "VALUES (";
    ins_into.reserve(1000);
    values.reserve(500);

    ins_into += table_name;
    ins_into += " (";

    size_t size = pCols.size();
    for (size_t i = 0; i < size; i++) {
        if (pCols[i]->IsColNameCaseSensitive()) {
            ins_into += '\"';
            ins_into += pCols[i]->GetColName();
            ins_into += '\"';
        }
        else {
            ins_into += pCols[i]->GetColName();
        }

        if (i == size - 1) {
            ins_into += ") ";
            values += "?)";
        } else {
            ins_into += ", ";
            values += "?,";
        }
    }
    ins_into += values;
    stmt = ins_into;
    return true;
}

bool InsertExecutor::GetInsStmtNoColumns(const std::vector<BaseColumn_SharedPtr> &pCols,
    const char *table_name, std::string &stmt)
{
    string ins_into = "INSERT INTO ";
    string values = " VALUES (";
    ins_into.reserve(1000);
    values.reserve(500);

    ins_into += table_name;
    const size_t size = pCols.size();

    for (size_t i = 0; i < size; i++) {
        if (i == size - 1) {
            values += "?)";
        } else {
            values += "?,";
        }
    }
    ins_into += values;
    stmt = ins_into;
    return true;
}

bool InsertExecutor::PrepareInsStmt(const char *sSqlStmt) const
{
    SQLRETURN rc;
#ifndef FAKE_DB_CONN
    rc = ::SQLPrepare(mHstmt, (SQLCHAR *)sSqlStmt, SQL_NTS);
#endif
    return SQL_SUCCEEDED(rc);
}

bool InsertExecutor::PrepareInsStmt(const std::vector<BaseColumn_SharedPtr> &pCols,
                                    const char *table_name) const
{
    std::string ins_into;
    GetInsStmtNoColumns(pCols, table_name, ins_into);
    SQLRETURN rc;
#ifndef FAKE_DB_CONN
    rc = ::SQLPrepare(mHstmt, (SQLCHAR *)ins_into.c_str(), SQL_NTS);
#endif
    return SQL_SUCCEEDED(rc);
}

bool InsertExecutor::ExecuteInsert(const ColRecords &records, bool commit/* = true*/) const
{
    SQLULEN ParamsProcessed = 0;
    SQLRETURN rc;

    const SQLUSMALLINT col_count = (SQLUSMALLINT)records.GetColCount();
    if (col_count == 0) {
        mErrStr = "no columns in input data";
        return false;
    }
    const SQLUSMALLINT row_count = (SQLUSMALLINT)records.GetRowCount();
#if 0
    if (row_count > 1) {
        // check if there is LOB columns
        for (size_t col = 0; col < col_count; ++col) {
            DATA_TYPE_T type = records.GetColumn(col)->GetDataAttr().type;
            if (type == T_BLOB || type == T_CLOB || type == T_NCLOB || type == T_TEXT)
            {
                mErrStr = "LOB types batch insert is not supported by driver";
                return false;
            }
        }
    }
#endif

    rc = SQLSetStmtAttr(mHstmt, SQL_ATTR_PARAM_BIND_TYPE, (SQLPOINTER)SQL_PARAM_BIND_BY_COLUMN, 0);
    if (SQL_SUCCEEDED(rc)) {
        rc = SQLSetStmtAttr(mHstmt, SQL_ATTR_PARAMSET_SIZE, reinterpret_cast<SQLPOINTER>(row_count), 0);
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

    std::vector<size_t> col_i(col_count, 0); // for index of each LOB column
    while (rc == SQL_NEED_DATA) {
        // Background: http://msdn.microsoft.com/en-us/library/ms716238(v=vs.85).aspx
        // Example: http://msdn.microsoft.com/en-us/library/ms713824(v=vs.85).aspx

        SQLPOINTER pToken;
        rc = SQLParamData(mHstmt, &pToken);
        if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO) {
            break;
        }

        size_t ipar = (size_t)pToken;
        if (ipar < 1 || ipar > col_count) {
            // should never happen as the token was specified in SQLBindParameter
            mErrStr = "invalid token returned from SQLParamData";
            return SQL_SUCCEEDED(rc);
        }

        if (rc == SQL_NEED_DATA) {
            BaseColumn_SharedPtr p_column = records.GetColumn(ipar - 1);
            size_t &row = col_i[ipar - 1];
            rc = SQLPutData(mHstmt, p_column->GetData(row), p_column->GetDataSize(row));
            row++;
            if (!SQL_SUCCEEDED(rc)) {
                break;
            }
            else {
                rc = SQL_NEED_DATA; // to satisfy the loop condition
            }
        }
    }

    if (SQL_SUCCEEDED(rc) && commit) {
        rc = SQLEndTran(SQL_HANDLE_DBC, mpConn->GetHDbc(), SQL_COMMIT);
    }
    return SQL_SUCCEEDED(rc);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class QueryExecutor

static DATA_ATTR_T OdbcTypeToDataAttr(SQLSMALLINT dataType, SQLULEN columnSize, SQLSMALLINT decimalDigits, bool bullable)
{
    DATA_ATTR_T attr;
    attr.a = (int)columnSize;
    attr.s = (int)decimalDigits;
    attr.null_able = bullable;

    switch(dataType)
    {
    case SQL_CHAR:
        attr.type = T_CHAR;
        break;
    case SQL_VARCHAR:
        attr.type = T_VARCHAR;
        break;
    case SQL_WCHAR:
        attr.type = T_NCHAR;
        break;
    case SQL_WVARCHAR:
        attr.type = T_NVARCHAR;
        break;
    case SQL_DECIMAL:
    case SQL_NUMERIC:
        attr.type = T_DECIMAL_PS;
        if (attr.p == 0 && attr.s == 0) {
            attr.type = T_DECIMAL;
        }
        break;
    case SQL_SMALLINT:
        attr.type = T_SMALLINT;
        break;
    case SQL_INTEGER:
        attr.type = T_INTEGER;
        break;
    case SQL_TINYINT:
        attr.type = T_TINYINT;
        break;
    case SQL_BIGINT:
        attr.type = T_BIGINT;
        break;
    case SQL_REAL:
        attr.type = T_REAL;
        break;
    case SQL_FLOAT:
        attr.type = T_FLOAT;
        break;
    case SQL_DOUBLE:
        attr.type = T_DOUBLE;
        break;
    case SQL_TYPE_DATE:
        attr.type = T_DATE;
        break;
    case SQL_TYPE_TIME:
        attr.type = T_TIME;
        break;
    case SQL_TYPE_TIMESTAMP:
        attr.type = T_TIMESTAMP;
        break;
    case SQL_UNKNOWN_TYPE:
        attr.type = T_UNKNOWN;
        break;
    default:
        hdb::UnImplemented(("OdbcTypeToDataAttr: unimplemented type: " + std::to_string(dataType)).c_str());
        attr.type = T_UNKNOWN;
        break;
    }

    return attr;
}

bool QueryExecutor::Prepare()
{
    SQLHSTMT hstmt = GetHStmt();
    SQLRETURN rc;

    rc = SQLPrepare(hstmt, (SQLCHAR *)mSql.c_str(), (SQLINTEGER)mSql.length());
    if (SQL_SUCCEEDED(rc)) {
        mRecordsBuff = std::make_shared<ColRecords>();

        SQLUSMALLINT    ColumnNumber = 1;
        SQLCHAR         ColumnName[512];
        SQLSMALLINT     NameLength;
        SQLSMALLINT     DataType;
        SQLULEN         ColumnSize;
        SQLSMALLINT     DecimalDigits;
        SQLSMALLINT     Nullable;

        // build columns in records
        while (SQL_SUCCEEDED(rc)) {
            rc = SQLDescribeCol(hstmt, ColumnNumber, ColumnName, sizeof(ColumnName), &NameLength,
                &DataType, &ColumnSize, &DecimalDigits, &Nullable);
            if (SQL_SUCCEEDED(rc) && ColumnSize > 0) {
                ColumnName[sizeof(ColumnName) - 1] = '\0';
                DATA_ATTR_T attr = OdbcTypeToDataAttr(DataType, ColumnSize, DecimalDigits, Nullable != 0);
                if (attr.type == T_UNKNOWN) {
                    std::cout << "Warning: SQLDescribeCol returned unknown column type!" << std::endl;
                    attr.type = T_CHAR;
                }
#if 0
                std::cout << "ColumnNumber = " << ColumnNumber << ", ColumnName = " << ColumnName
                    << ", DataType = " << DataType << ", ColumnSize = " << ColumnSize << ", DecimalDigits = "
                    << DecimalDigits << std::endl;
#endif
                mRecordsBuff->AddCol((const char *)ColumnName, attr);
            }
            else {
                break;
            }
            ColumnNumber++;
        }

        rc = SQL_SUCCESS;
    }

    return SQL_SUCCEEDED(rc);
}

// Refer to http://msdn.microsoft.com/en-us/library/windows/desktop/ms713541(v=vs.85).aspx
bool QueryExecutor::Execute(size_t row_array_size)
{
    mNumRowsFetched = 0;
    mNumRowsFetchedTotal = 0;

    if (false == Prepare()) {
        return false;
    }

    SQLHSTMT hstmt = GetHStmt();
    SQLRETURN rc;

    mRowStatusArray.resize(row_array_size);
    memset(&mRowStatusArray[0], 0, mRowStatusArray.size() * sizeof(mRowStatusArray[0]));

    rc = SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_BIND_TYPE, SQL_BIND_BY_COLUMN, 0);
    if (SQL_SUCCEEDED(rc)) {
        SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)row_array_size, 0);
        SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_STATUS_PTR, (SQLPOINTER)&mRowStatusArray[0], 0);
        SQLSetStmtAttr(hstmt, SQL_ATTR_ROWS_FETCHED_PTR, &mNumRowsFetched, 0);

        mRecordsBuff->SetRowCount(row_array_size);
        rc = mRecordsBuff->BindAllOutColumns(hstmt);
    }

    if (SQL_SUCCEEDED(rc)) {
        rc = SQLExecDirect(hstmt, (SQLCHAR *)mSql.c_str(), SQL_NTS);
    }
    return SQL_SUCCEEDED(rc);
}

int QueryExecutor::FetchRows()
{
    SQLRETURN rc;
    SQLHSTMT hstmt = GetHStmt();

    rc = SQLFetchScroll(hstmt, SQL_FETCH_NEXT, 0);
    if (rc == SQL_NO_DATA) {
        mRecordsBuff->SetRowCount(0);
    }
    else if (!SQL_SUCCEEDED(rc)) {
        mRecordsBuff->SetRowCount(0);
        return -1;
    }

    mRecordsBuff->SetRowCount(mNumRowsFetched);
    mNumRowsFetchedTotal += mNumRowsFetched;
    return (int)mRecordsBuff->GetRowCount();
}

int QueryExecutor::FetchedColCount() const
{
    return (int)mRecordsBuff->GetColCount();;
}

std::string QueryExecutor::GetFetchedColName(int i_col) const
{
    return mRecordsBuff->GetColumn(i_col)->GetColName();
}

int QueryExecutor::FetchedRowsToCsv(std::ostream &os_csv, char delimiter) const
{
    return mRecordsBuff->RowsToCsv(os_csv, 0, (int)mNumRowsFetched, delimiter);
}

HDB_END_NAMESPACE
