

#include <afxdisp.h>
#include <afxtempl.h>
#include "DbBasics.h"

#if (_MFC_VER <= 0x0600)
// DbBasics.obj : error LNK2001: unresolved external symbol "void __stdcall AfxThrowInvalidArgException(void)" (?AfxThrowInvalidArgException@@YGXXZ)
static
void __stdcall AfxThrowInvalidArgException(void)
{
}
#endif

static
char *WideStrToAnsiStr(const WCHAR * wstr)
{
    char *buff = NULL;
    if( NULL != wstr )
    {
        int size = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
        if (size > 0)
        {
            buff = new char[size];
            if (NULL != buff)
            {
                WideCharToMultiByte(CP_ACP, 0, wstr, -1, buff, size, NULL, NULL);
            }
        }
    }

	return buff;
}

static
BOOL WriteStrToFile(const char *name, const char *str)
{
    if (str == NULL)
        return FALSE;

    BOOL bRes = FALSE;
    FILE *fp = NULL;
    fopen_s(&fp, name, "wb");
    if (fp)
    {
        size_t len = strlen(str);
        fwrite((const void*)str, 1, len, fp);
        fclose(fp);
        bRes = TRUE;
    }

    return bRes;
}

//////////////////////////////////////////////////////////////////////////////////////
// class Variant Implementation

#define DATA_TO_VAR     Var_DataToOleVariant(m_data)
static COleVariant &Var_DataToOleVariant(void *data)
{
    return *(COleVariant*)data;
}

void Var_MakeString(void* data, CString& wstr)
{
    COleVariant& var = Var_DataToOleVariant(data);
    switch(var.vt)
    {
    case VT_EMPTY:
        wstr = L"Empty";
        break;
    case VT_NULL:
        wstr = L"Null";
        break;
    case VT_BOOL:
        wstr = var.boolVal ? L"true" : L"false";
        break;
    case VT_I2:
        wstr.Format(L"%d", (long)var.iVal);
        break;
    case VT_I4:
        wstr.Format(L"%ld", var.lVal);
        break;
    case VT_I8:
        wstr.Format(L"%lld", var.llVal);
        break;
    case VT_R4:
        wstr.Format(L"%f", var.fltVal);
        break;
    case VT_R8:
        wstr.Format(L"%g", var.dblVal);
        break;
    case VT_BSTR:
        wstr = var.bstrVal;
        break;
    case VT_UI1 | VT_ARRAY:
        wstr = L"{Blob}";
        break;
    default:
        ASSERT(FALSE);
        break;
    };
}

static void Var_MakeType(void* data, DataType& type)
{
    COleVariant& var = Var_DataToOleVariant(data);
    switch(var.vt)
    {
    case VT_EMPTY:  type = DT_Empty; break;
    case VT_NULL:   type = DT_Null; break;
    case VT_BOOL:   type = DT_Boolean; break;
    case VT_I2:     type = DT_Int16; break;
    case VT_I4:     type = DT_Int32; break;
    case VT_I8:     type = DT_Int64; break;
    case VT_R4:     type = DT_Float; break;
    case VT_R8:     type = DT_Double; break;
    case VT_BSTR:   type = DT_String; break;
    case VT_UI1 | VT_ARRAY: type = DT_Blob; break;
    default:
        ASSERT(FALSE);
        break;
    };
}

Variant::Variant()
{
    Init();
    Var_MakeString(m_data, m_wstr);
}

Variant::Variant(const Variant& varSrc)
{
    Init();
    *this = varSrc;
}

Variant::Variant(short i)
{
    Init();
    *this = i;
}

Variant::Variant(long i)
{
    Init();
    *this = i;
}

Variant::Variant(float f)
{
    Init();
    *this = f;
}

Variant::Variant(double lf)
{
    Init();
    *this = lf;
}

Variant::Variant(bool b)
{
    Init();
    *this = b;
}

Variant::Variant(const char* strIn)
{
    Init();
    *this = strIn;
}

Variant::Variant(const WCHAR* wstrIn)
{
    Init();
    *this = wstrIn;
}

Variant::Variant(const VARIANT& varSrc)
{
    Init();
    *this = varSrc;
}

Variant::~Variant()
{
    if (m_data)
        delete (COleVariant*)m_data;
}

const Variant& Variant::operator =(const Variant& varSrc)
{
    this->m_type  = varSrc.m_type;
    this->m_wstr  = varSrc.m_wstr;
    this->m_dirty = varSrc.m_dirty;
    Var_DataToOleVariant(this->m_data) = Var_DataToOleVariant(varSrc.m_data);
    return *this;
}

const Variant& Variant::operator =(short i)
{
    this->m_type = DT_Int16;
    this->m_dirty = false;
    DATA_TO_VAR = i;
    Var_MakeString(m_data, m_wstr);
    return *this;
}

const Variant& Variant::operator =(long i)
{
    this->m_type = DT_Int32;
    this->m_dirty = false;
    DATA_TO_VAR = i;
    Var_MakeString(m_data, m_wstr);
    return *this;
}

const Variant& Variant::operator =(float f)
{
    this->m_type = DT_Float;
    this->m_dirty = false;
    DATA_TO_VAR = f;
    Var_MakeString(m_data, m_wstr);
    return *this;
}

const Variant& Variant::operator =(double lf)
{
    this->m_type = DT_Double;
    this->m_dirty = false;
    DATA_TO_VAR = lf;
    Var_MakeString(m_data, m_wstr);
    return *this;
}

const Variant& Variant::operator =(bool b)
{
    m_type = DT_Boolean;
    this->m_dirty = false;
    DATA_TO_VAR = COleVariant((long)b, VT_BOOL);
    Var_MakeString(m_data, m_wstr);
    return *this;
}

const Variant& Variant::operator =(const char *strIn)
{
    this->m_type = DT_String;
    this->m_dirty = false;

    COleVariant &var = DATA_TO_VAR;
	var.Clear(); // Free up previous VARIANT
    var.vt = VT_BSTR;
	if (strIn == NULL)
		var.bstrVal = NULL;
	else
	{
		var.bstrVal = CString(strIn).AllocSysString();
	}

    Var_MakeString(m_data, m_wstr);
    return *this;
}

const Variant& Variant::operator =(const WCHAR *wstrIn)
{
    this->m_type = DT_String;
    this->m_dirty = false;

    COleVariant &var = DATA_TO_VAR;
	var.Clear(); // Free up previous VARIANT
    var.vt = VT_BSTR;
	if (wstrIn == NULL)
		var.bstrVal = NULL;
	else
	{
		var.bstrVal = CString(wstrIn).AllocSysString();
	}

    Var_MakeString(m_data, m_wstr);
    return *this;
}

const Variant& Variant::operator =(const VARIANT& varSrc)
{
    this->m_dirty = false;
    DATA_TO_VAR = varSrc;
    Var_MakeType(this->m_data, this->m_type);
    Var_MakeString(this->m_data, this->m_wstr);
    return *this;
}

bool Variant::operator==(const Variant& varSrc) const
{
    if (this->m_type != varSrc.m_type)
        return false;
    return (Var_DataToOleVariant(this->m_data) == Var_DataToOleVariant(varSrc.m_data)) ? true : false;
}

bool Variant::operator!=(const Variant& varSrc) const
{
    return !(*this == varSrc);
}

void Variant::SetDirty(bool dirty)
{
    m_dirty = dirty;
}

bool Variant::IsDirty() const
{
    return m_dirty;
}

short Variant::ToInt16() const
{
    return DATA_TO_VAR.iVal;
}

long Variant::ToInt32() const
{
    return DATA_TO_VAR.lVal;
}

long long Variant::ToInt64() const
{
    return DATA_TO_VAR.llVal;
}

float Variant::ToFloat() const
{
    return DATA_TO_VAR.fltVal;
}

double Variant::ToDouble() const
{
    return DATA_TO_VAR.dblVal;
}

bool Variant::ToBoolean() const
{
    return DATA_TO_VAR.boolVal ? true : false;
}

const VARIANT& Variant::ToVARIANT() const
{
    return DATA_TO_VAR;
}

const WCHAR *Variant::ToString() const
{
    return (const WCHAR *)m_wstr;
}

const void *Variant::ToBlob(int *pnum) const
{
    ASSERT(FALSE);
    if (pnum != NULL)
    {
    }

    return NULL;
}

DataType Variant::GetType() const
{
    return m_type;
}

void Variant::Init()
{
    m_data = new COleVariant();
    m_type = DT_Empty;
    m_dirty = false;
}

//////////////////////////////////////////////////////////////////////////////////////
// class DbRow Implementation

static CArray<Variant, const Variant&>& Row_DataToArray(void *data)
{
    return *(CArray<Variant, const Variant&> *)data;
}

static void Row_MakeString(void* data, CString& wstrRow)
{
    int i;
    CString wstr;
    CArray<Variant, const Variant&>& arrVars = Row_DataToArray(data);

    wstrRow.Empty();
    for (i=0; i<arrVars.GetSize(); i++)
    {
        wstr.Format(L"[%s], ", arrVars[i].ToString());
        wstrRow += wstr;
    }

    wstrRow.TrimRight(L", ");
}

DbRow::DbRow()
{
    m_data = new CArray<Variant, const Variant&>;
    m_wstr.Empty();
}

DbRow::~DbRow()
{
    if (this->m_data)
        delete (CArray<Variant, const Variant&> *)this->m_data;
}

int DbRow::GetColCount() const
{
    return (int)Row_DataToArray(m_data).GetSize();
}

void DbRow::SetColCount(int n)
{
    Row_DataToArray(m_data).SetSize(n);
    Row_MakeString(this->m_data, this->m_wstr);
}

Variant& DbRow::GetAt(int i) const
{
    return Row_DataToArray(m_data).GetAt(i);
}

bool DbRow::SetAt(int i, const Variant& varNew)
{
    Variant& varOld = this->GetAt(i);
    if (varOld != varNew)
    {
        Row_DataToArray(m_data).SetAt(i, (Variant &)varNew);
        Row_MakeString(this->m_data, this->m_wstr);
    }
    return TRUE;
}

bool DbRow::IsDirty() const
{
    for (int i=GetColCount()-1; i>=0; i--)
    {
        Variant& var = (Variant&)this->GetAt(i);
        if (var.IsDirty())
            return true;
    }
    return false;
}

void DbRow::ClearDirty()
{
    for (int i=GetColCount()-1; i>=0; i--)
    {
        Variant& var = (Variant&)this->GetAt(i);
        if (var.IsDirty())
            var.SetDirty(false);
    }
}

const DbRow& DbRow::operator=(const DbRow& rowSrc)
{
    if (this == &rowSrc)
        return *this;

    this->SetColCount(rowSrc.GetColCount());
    for (int i=0; i<this->GetColCount(); i++)
    {
        this->SetAt(i, rowSrc.GetAt(i));
    }

    return *this;
}

const WCHAR* DbRow::ToString() const
{
    return this->m_wstr;
}

//////////////////////////////////////////////////////////////////////////////////////
// class DbRange Implementation
#define MAX_COL_COUNT   10000

static CArray<DbRow, const DbRow&>& Table_DataToArray(void *data)
{
    return *(CArray<DbRow, const DbRow&> *)data;
}

DbRange::DbRange()
: m_nRowCount(0), m_nColCount(0)
{
    m_data = new CArray<DbRow, const DbRow&>;
}

DbRange::~DbRange()
{
    if (this->m_data)
        delete (CArray<DbRow, const DbRow&> *)this->m_data;
}

int DbRange::GetRowCount() const
{
    return (int)Table_DataToArray(m_data).GetSize();
}

bool DbRange::SetRowCount(int n)
{
    if (n < 0)
        return false;

    CArray<DbRow, const DbRow&>& rows = Table_DataToArray(m_data);
    int nLastRowCount = m_nRowCount;
    m_nRowCount = n;
    rows.SetSize(m_nRowCount);

    if (m_nRowCount > nLastRowCount)
    {
        for (int nRow=nLastRowCount; nRow<m_nRowCount; nRow++)
        {
            if (m_nColCount != rows[nRow].GetColCount())
            {
                rows[nRow].SetColCount(m_nColCount);
            }
        }
    }

    return true;
}

int DbRange::GetColumnCount()
{
    ASSERT(m_nColCount>=0 && m_nColCount<=MAX_COL_COUNT);
    return m_nColCount;
}

bool DbRange::SetColumnCount(int n)
{
    if (n<0 || n>MAX_COL_COUNT)
        return false;

    m_nColCount = n;
    CArray<DbRow, const DbRow&>& rows = Table_DataToArray(m_data);
    for (int i=0; i<rows.GetSize(); i++)
    {
        rows[i].SetColCount(m_nColCount);
    }
    return true;
}

Variant& DbRange::GetAt(int iRow, int iCol) const
{
    CArray<DbRow, const DbRow&>& rows = Table_DataToArray(m_data);
    return rows[iRow].GetAt(iCol);
}

DbRow& DbRange::GetRowAt(int iRow) const
{
    CArray<DbRow, const DbRow&>& rows = Table_DataToArray(m_data);
    return rows.GetAt(iRow);
}

bool DbRange::SetRowAt(int iRow, const DbRow& row)
{
    CArray<DbRow, const DbRow&>& rows = Table_DataToArray(m_data);
    if (iRow >= rows.GetSize())
        return false;
    rows.SetAt(iRow, row);
    return true;
}

void DbRange::ClearDirty()
{
    CArray<DbRow, const DbRow&>& rows = Table_DataToArray(m_data);
    for (int iRow=0; iRow<rows.GetSize(); iRow++)
    {
        rows.GetAt(iRow).ClearDirty();
    }
}

void DbRange::Dump(const WCHAR *wfilename)
{
    CString str, strAll;

    CArray<DbRow, const DbRow&>& rows = Table_DataToArray(m_data);
    for (int iRow=0; iRow<rows.GetSize(); iRow++)
    {
        str = rows.GetAt(iRow).ToString();
        strAll += str;
        strAll += _T("\n");
    }

	char *filename = WideStrToAnsiStr(wfilename);
	char *content = WideStrToAnsiStr(strAll);

	if (filename && content)
	{
		WriteStrToFile(filename, content);
	}
	if (filename) delete[] filename;
	if (content) delete[] content;
}
