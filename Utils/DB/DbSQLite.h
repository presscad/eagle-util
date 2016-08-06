#pragma once

#include "DbBasics.h"

enum DbSQLiteDataType
{
    SDT_INTEGER = 1,    // SQLITE_INTEGER 
    SDT_FLOAT = 2,      // SQLITE_FLOAT
    SDT_TEXT = 3,       // SQLITE3_TEXT
    SDT_BLOB = 4,       // SQLITE_BLOB
    SDT_NULL = 5,       // SQLITE_NULL
};

class DbSQLiteVar : public Variant
{
public:
    DbSQLiteDataType GetSQLiteType() const;

    const DbSQLiteVar& operator=(const DbSQLiteVar& varSrc);
    const DbSQLiteVar& operator=(long value);
    const DbSQLiteVar& operator=(long long value);
    const DbSQLiteVar& operator=(double value);
    const DbSQLiteVar& operator=(const char *str);
    const DbSQLiteVar& operator=(const WCHAR *wstr);
    void SetValueBlob(const void *blob, int bytes);
    void SetValueNull();
};

class DbSQLiteRow : public DbRow
{
public:
    DbSQLiteVar& GetSQLiteVarAt(int iCol) const; // iCol: 0-based index
    DbSQLiteVar& operator[](int iCol) const;
};

class DbSQLiteRowArray : public DbRange
{
public:
    DbSQLiteRow& operator[](int iRow) const;
    BOOL SetRowAt(int iRow, const DbSQLiteRow& row);
    BOOL RemoveAllRows();
    BOOL AppendRow(const DbSQLiteRow& row);
};

class DbSQLite
{
public:
    DbSQLite();
    ~DbSQLite();

public:
    BOOL Open(const WCHAR *wsFilename);
    BOOL Close();
#if (_MFC_VER > 0x0600)
    void GetErrMsg(CStringA &err) const;
#endif
    void GetErrMsg(CString &werr) const;
    BOOL GetDbHandle(void **pdbHandle) const; // get db handle which can be mapped to type sqlite3*

    // Create table, SQL example: "CREATE TABLE players (ID INTEGER PRIMARY KEY, name TEXT, age INTERER);"
    BOOL CreateTable(const char *SQL);
    // Drop table
    BOOL DropTable(const char *table_name);

    // tableName, e.g.: "players", column_names, e.g.: "name, age"
    BOOL Insert(const char *table_name, const char *column_names, const DbSQLiteRowArray &rows);
    // SQL e.g.: "SELECT ID, name, age FROM players ORDER BY age;", results stored in rows
    BOOL Query(const char *SQL, DbSQLiteRowArray &rows);

protected:
    CString     m_wfilename;
    void*       m_data;
};
