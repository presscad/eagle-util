#pragma once

#ifdef _WIN32
# include <afxdisp.h>
#endif

enum DataType
{
    DT_Empty,
    DT_Null,
    DT_Int8,
    DT_Int16,
    DT_Int32,
    DT_Int64,
    DT_Uint8,
    DT_Uint16,
    DT_Uint32,
    DT_Uint64,
    DT_Float,
    DT_Double,
    DT_Boolean,
    DT_String,
    DT_Blob,
};

class Variant
{
public:
    Variant();
    Variant(const Variant& varSrc);
    Variant(short i);
    Variant(long i);
    Variant(float f);
    Variant(double f);
    Variant(bool b);
    Variant(const char *str);
    Variant(const WCHAR *wstr);
    ~Variant();

    const Variant& operator =(const Variant& varSrc);
    const Variant& operator =(short i);
    const Variant& operator =(long i);
    const Variant& operator =(float f);
    const Variant& operator =(double lf);
    const Variant& operator =(bool b);
    const Variant& operator =(const char* str);
    const Variant& operator =(const WCHAR* str);

#ifdef _WIN32
    Variant(const VARIANT& varSrc);
    const Variant& operator =(const VARIANT& varSrc);
#endif

    // operation "==" and "!=" ignore whether the two are dirty
    bool operator ==(const Variant& varSrc) const;
    bool operator !=(const Variant& varSrc) const;

    void SetDirty(bool dirty);
    bool IsDirty() const;

protected:
    void Init();

protected:
    DataType    m_type;
    CString     m_wstr; // description for the VAR for easy debug. accurate data stored in *data
    void*       m_data;
    bool        m_dirty;

public:
    DataType GetType() const;
    short ToInt16() const;
    long ToInt32() const;
    long long ToInt64() const;
    float ToFloat() const;
    double ToDouble() const;
    bool ToBoolean() const;
    const VARIANT& ToVARIANT() const;
    const WCHAR *ToString() const;
    const void *ToBlob(int *pnum = NULL) const;
};

class DbRow
{
public:
    DbRow();
    ~DbRow();

public:
    Variant& GetAt(int iCol) const; // i is zero-based index
    bool SetAt(int iCol, const Variant& var);
    int GetColCount() const;
    void SetColCount(int n);
    bool IsDirty() const;
    void ClearDirty();
    const DbRow& operator=(const DbRow& rowSrc);
    const WCHAR *ToString() const;

protected:
    CString     m_wstr;  // description of the row data, accurate data stored in *data
    void*       m_data;
};

// DbRange could be a table/sheet or sub set of a table/sheet
class DbRange
{
public:
    DbRange();
    ~DbRange();

public:
    int  GetRowCount() const;
    bool SetRowCount(int n);
    int  GetColumnCount();
    bool SetColumnCount(int n);
    Variant& GetAt(int iRow, int iCol) const; // i is zero-based index
    DbRow& GetRowAt(int iRow) const;
    bool SetRowAt(int iRow, const DbRow& row);
    void ClearDirty();
    void Dump(const WCHAR *filename);

protected:
    int         m_nRowCount;
    int         m_nColCount;
    void*       m_data;
};
