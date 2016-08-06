
#include "DbBasics.h"
#include "DbSQLite.h"
#include "SQLite\sqlite3.h"

//////////////////////////////////////////////////////////////////////////////////////
// class DbSQLiteVar Implementation

DbSQLiteDataType DbSQLiteVar::GetSQLiteType() const
{
    switch(this->m_type)
    {
    case DT_Boolean:
    case DT_Int8:
    case DT_Int16:
    case DT_Int32:
    case DT_Int64:
    case DT_Uint8:
    case DT_Uint16:
    case DT_Uint32:
    case DT_Uint64:
        return SDT_INTEGER;
    case DT_Float:
    case DT_Double:
        return SDT_FLOAT;
    case DT_String:
        return SDT_TEXT;
    case DT_Blob:
        return SDT_BLOB;
    case DT_Null:
    case DT_Empty:
        return SDT_NULL;
    default:
        ASSERT(FALSE);
        return SDT_NULL;
    };
}

const DbSQLiteVar& DbSQLiteVar::operator=(const DbSQLiteVar& varSrc)
{
    (Variant&)(*this) = (Variant&)varSrc;
    return *this;
}

const DbSQLiteVar& DbSQLiteVar::operator=(long value)
{
    (Variant&)(*this) = value;
    return *this;
}

const DbSQLiteVar& DbSQLiteVar::operator=(long long value)
{
    (Variant&)(*this) = value;
    return *this;
}

const DbSQLiteVar& DbSQLiteVar::operator=(double value)
{
    (Variant&)(*this) = value;
    return *this;
}

const DbSQLiteVar& DbSQLiteVar::operator=(const char *str)
{
    (Variant&)(*this) = str;
    return *this;
}

const DbSQLiteVar& DbSQLiteVar::operator=(const WCHAR *wstr)
{
    (Variant&)(*this) = wstr;
    return *this;
}

void DbSQLiteVar::SetValueBlob(const void *blob, int bytes)
{
    CByteArray byteArr;

    if (blob != NULL && bytes>0)
    {
        byteArr.SetSize(bytes);
        memcpy(byteArr.GetData(), blob, bytes);
    }

    COleVariant varBytes(byteArr);
    (Variant&)(*this) = varBytes;
}

void DbSQLiteVar::SetValueNull()
{
    COleVariant varNull;
    varNull.ChangeType(VT_NULL);
    (Variant&)(*this) = varNull;
}

//////////////////////////////////////////////////////////////////////////////////////
// class DbSQLiteRow Implementation
DbSQLiteVar& DbSQLiteRow::GetSQLiteVarAt(int iCol) const
{
    Variant& var = this->GetAt(iCol);
    return (DbSQLiteVar&)var;
}

DbSQLiteVar& DbSQLiteRow::operator[](int iCol) const
{
    Variant& var = this->GetAt(iCol);
    return (DbSQLiteVar&)var;
}

//////////////////////////////////////////////////////////////////////////////////////
// class DbSQLiteRowArray Implementation
DbSQLiteRow& DbSQLiteRowArray::operator[](int iRow) const
{
    return (DbSQLiteRow&)this->GetRowAt(iRow);
}

BOOL DbSQLiteRowArray::SetRowAt(int iRow, const DbSQLiteRow& row)
{
    return (true == DbRange::SetRowAt(iRow, row));
}

BOOL DbSQLiteRowArray::RemoveAllRows()
{
    if (this->GetRowCount() == 0)
        return TRUE;

    this->SetRowCount(0);
    return TRUE;
}

BOOL DbSQLiteRowArray::AppendRow(const DbSQLiteRow& row)
{
    int count = this->GetRowCount();
    this->SetRowCount(count + 1);
    this->SetRowAt(count, row);
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////
// class DbSQLite Implementation

#define DATA_TO_STRUCT   VoidDataToStruct(m_data)

#define ERROR_RETURN    { \
    DATA_TO_STRUCT.wErrMsg = (const WCHAR*)sqlite3_errmsg16(DATA_TO_STRUCT.dbHandle); \
    return FALSE; }

#define ERROR_RETURN_MSG(str)    {DATA_TO_STRUCT.wErrMsg = str; return FALSE; }

struct SQLITE_DATA
{
    sqlite3*    dbHandle;
    CString     wErrMsg;

    void Init()
    {
        dbHandle = NULL;
    };
};

static inline SQLITE_DATA& VoidDataToStruct(void *data)
{
    return *(SQLITE_DATA*)data;
}

DbSQLite::DbSQLite()
{
    m_data = new SQLITE_DATA;
    DATA_TO_STRUCT.Init();
}

DbSQLite::~DbSQLite()
{
    if (DATA_TO_STRUCT.dbHandle != NULL)
    {
        this->Close();
    }
    if (m_data != NULL)
    {
        delete (SQLITE_DATA*)m_data;
    }
}

BOOL DbSQLite::Open(const WCHAR *wfilename)
{
    SQLITE_DATA& data = DATA_TO_STRUCT;
    if (data.dbHandle != NULL)
    {
        this->Close();
    }

    if (SQLITE_OK != sqlite3_open16(wfilename, &data.dbHandle))
    {
        data.dbHandle = NULL;
        return FALSE;
    }

    m_wfilename = wfilename;
    return TRUE;
}

BOOL DbSQLite::Close()
{
    SQLITE_DATA& data = DATA_TO_STRUCT;
    if (data.dbHandle != NULL)
    {
        if (SQLITE_OK != sqlite3_close(data.dbHandle))
        {
            return FALSE;
        }
    }

    m_wfilename.Empty();
    data.Init();
    return TRUE;
}

#if (_MFC_VER > 0x0600)
void DbSQLite::GetErrMsg(CStringA &err) const
{
    CStringW werr;
    this->GetErrMsg(werr);
    err = werr;
}
#endif

void DbSQLite::GetErrMsg(CString &werr) const
{
    SQLITE_DATA& data = DATA_TO_STRUCT;

    werr = data.wErrMsg;
    if (werr.IsEmpty())
    {
        SQLITE_DATA& data = DATA_TO_STRUCT;
        data.wErrMsg = werr = (const WCHAR*)sqlite3_errmsg16(data.dbHandle);
    }
}

BOOL DbSQLite::GetDbHandle(void **pdbHandle) const
{
    SQLITE_DATA& data = DATA_TO_STRUCT;

    if (pdbHandle == NULL)
    {
        data.wErrMsg = L"Invalid buffer";
        return FALSE;
    }

    *pdbHandle = data.dbHandle;
    return TRUE;
}

BOOL DbSQLite::CreateTable(const char *SQL)
{
    SQLITE_DATA& data = DATA_TO_STRUCT;
    sqlite3_stmt* stmt;
    const char* zTail; 

    if (SQLITE_OK != sqlite3_prepare(data.dbHandle, SQL, -1, &stmt, &zTail))
        ERROR_RETURN;
    sqlite3_step(stmt);
    if (SQLITE_OK != sqlite3_finalize(stmt))
        ERROR_RETURN;
    return TRUE;
}

BOOL DbSQLite::DropTable(const char *table_name)
{
    SQLITE_DATA& data = DATA_TO_STRUCT;
    sqlite3_stmt* stmt;
    const char* zTail; 

    char SQL[1024];
	sprintf_s(SQL, "DROP TABLE %s", table_name);
    if (SQLITE_OK != sqlite3_prepare(data.dbHandle, SQL, -1, &stmt, &zTail))
        ERROR_RETURN;
    sqlite3_step(stmt);
    if (SQLITE_OK != sqlite3_finalize(stmt))
        ERROR_RETURN;
    return TRUE;
}

BOOL DbSQLite::Insert(const char *table_name, const char *column_names, const DbSQLiteRowArray &rows)
{
    if (table_name==NULL || table_name[0]=='\0')
        ERROR_RETURN_MSG("Invalid table name");
    if (column_names==NULL || column_names[0]=='\0')
        ERROR_RETURN_MSG("Invalid column names");
    if (rows.GetRowCount() == 0)
        ERROR_RETURN_MSG("Invalid rows count");

    SQLITE_DATA& data = DATA_TO_STRUCT;
    sqlite3_stmt* stmt;
    const char* zTail;
    int column_num = 1;

	char sql[1024];
	CString values("?");
    for (int i=(int)strlen(column_names)-1; i>=0; i--)
    {
        if (column_names[i] == ',')
        {
            values += ",?";
            column_num++;
        }
    }
    sprintf_s(sql, "INSERT INTO %s (%s) VALUES(%s);", table_name, column_names, values);
	#pragma TODO(Remember to fix values conversion)

    // SQL example: "INSERT INTO players (name, age) VALUES(?,?);"
    if (SQLITE_OK != sqlite3_prepare(data.dbHandle, sql, -1, &stmt, &zTail))
        ERROR_RETURN;

    for (int iRow=0; iRow<rows.GetRowCount(); iRow++)
    {
        int res;
        DbSQLiteRow& row = rows[iRow];
        for (int iCol=0; iCol<column_num && iCol<row.GetColCount(); iCol++)
        {
            int num;
            void *bytes;
            DbSQLiteVar& var = row[iCol];
            DbSQLiteDataType type = var.GetSQLiteType();

            switch(type)
            {
            case SDT_INTEGER:
                if (var.GetType() == DT_Int64 || var.GetType() == DT_Uint64)
                {
                    res = sqlite3_bind_int64(stmt, iCol+1, var.ToInt64());
                }
                else
                {
                    res = sqlite3_bind_int(stmt, iCol+1, var.ToInt32());
                }
                break;
            case SDT_FLOAT:
                res = sqlite3_bind_double(stmt, iCol+1, var.ToDouble());
                break;
            case SDT_TEXT:
                res = sqlite3_bind_text16(stmt, iCol+1, var.ToString(), -1, SQLITE_TRANSIENT);
                break;
            case SDT_BLOB:
                bytes = (void *)var.ToBlob(&num);
                res = sqlite3_bind_blob(stmt, iCol, bytes, num, SQLITE_TRANSIENT);
                break;
            case SDT_NULL:
                res = sqlite3_bind_null(stmt, iCol+1);
                break;
            default:
                break;
            }
            if (SQLITE_OK != res)
            {
                sqlite3_finalize(stmt);
                ERROR_RETURN;
            }

        }
        res = sqlite3_step(stmt);
        if (SQLITE_DONE != res)
        {
            sqlite3_finalize(stmt);
            ERROR_RETURN;
        }

        if (iRow < rows.GetRowCount()-1)
        {
            sqlite3_reset(stmt);
        }
    }
    sqlite3_finalize(stmt);

    return TRUE;
}

BOOL DbSQLite::Query(const char *SQL, DbSQLiteRowArray &rows)
{
    if (SQL==NULL || SQL[0]=='\0')
        ERROR_RETURN_MSG("Invalid SQL statement");

    SQLITE_DATA& data = DATA_TO_STRUCT;
    sqlite3_stmt* stmt;
    const char* zTail;
    int res;
    DbSQLiteRow row;

    if (SQLITE_OK != sqlite3_prepare(data.dbHandle, SQL, -1, &stmt, &zTail))
        ERROR_RETURN;

    rows.RemoveAllRows();

    res = sqlite3_step(stmt);
    while (res == SQLITE_ROW)
    {
        int col_count = sqlite3_data_count(stmt);
        row.SetColCount(col_count);

        for (int iCol=0; iCol<col_count; iCol++)
        {
            int iType = sqlite3_column_type(stmt, iCol);
            sqlite_int64 v_int64;
            double v_double;
            const void *v_blob;
            int v_blob_byes;
            const WCHAR *v_text;

            switch(iType)
            {
            case SQLITE_INTEGER:
                v_int64 = sqlite3_column_int64(stmt, iCol);
                row[iCol] = v_int64;
                break;
            case SQLITE_FLOAT:
                v_double = sqlite3_column_double(stmt, iCol);
                row[iCol] = v_double;
                break;
            case SQLITE_BLOB:
                v_blob = sqlite3_column_blob(stmt, iCol);
                v_blob_byes = sqlite3_column_bytes(stmt, iCol);
                row[iCol].SetValueBlob(v_blob, v_blob_byes);
                break;
            case SQLITE3_TEXT:
                v_text = (const WCHAR *)sqlite3_column_text16(stmt, iCol);
                row[iCol] = v_text;
                break;
            case SQLITE_NULL:
                row[iCol].SetValueNull();
                break;
            default:
                sqlite3_finalize(stmt);
                ERROR_RETURN_MSG("Invalid data in rows passed in");
            }
        }

        rows.AppendRow(row);
        res = sqlite3_step(stmt);
    }

    sqlite3_finalize(stmt);
    return TRUE;
}
