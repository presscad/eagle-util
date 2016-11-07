
#include <afxdisp.h>
#include "DbBasics.h"
#include "DbExcel.h"
#include "Excel/Excel.h"

#ifndef _UNICODE
#error Must be compiled with UNICODE
#endif

static
BOOL AnsiStrToWideStr(CString &wstr, const CHAR * str)
{
    BOOL result = FALSE;
    WCHAR *buffW = NULL;

    wstr.Empty();

    if( NULL != str )
    {
        int size = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
        if (size > 0)
        {
            buffW = new WCHAR[size];
            if (NULL != buffW)
            {
                if (0 < MultiByteToWideChar(CP_ACP, 0, str, -1, buffW, size))
                {
                    wstr = buffW;
                    result = TRUE;
                }
            }
        }
    }

    if (buffW) delete []buffW;
    return result;
}

static
CString AnsiStrToWideStr(const CHAR * str)
{
    CString wstr;
    if (FALSE == AnsiStrToWideStr(wstr, str))
    {
        wstr.Empty();
    }
    return wstr;
}


//////////////////////////////////////////////////////////////////////////////////////
// class DbExcel Implementation

#define DATA_TO_CEXCEL     DataToExcel(m_data)


static CExcel& DataToExcel(void *data)
{
    ASSERT(data != NULL);
    return *(CExcel *)data;
}

static int StrColToCol(const char *strCol)
{
    if (strCol == NULL || strCol[0] == '\0')
        return -1;
    if (strCol[1] == '\0')
    {
        if (strCol[0] < 'A' || strCol[0] > 'Z')
            return -1;
        else
            return strCol[0]-'A'+1;
    }
    if (strCol[2] == '\0')
    {
        ASSERT(FALSE); // TODO: ...
    }
    return -1;
}

DbExcel::DbExcel()
{
    m_data = new CExcel();
}

DbExcel::~DbExcel()
{
    if (!m_wfilename.IsEmpty())
        DATA_TO_CEXCEL.Close();
    delete (CExcel *)m_data;
}

bool DbExcel::Open(const char *filename)
{
    CString wname(filename);
#if defined(UNICODE)
    return Open(wname);
#else
    return Open(AnsiStrToWideStr(wname));
#endif
}

bool DbExcel::Open(const WCHAR *wfilename)
{
    this->m_wfilename = wfilename;
    CExcel& excel = DATA_TO_CEXCEL;
    excel.AddNewFile(m_wfilename);
    return true;
}

bool DbExcel::Close()
{
    CExcel& excel = DATA_TO_CEXCEL;
    excel.Close();
    m_wfilename.Empty();
    return true;
}

bool DbExcel::Save()
{
    if (m_wfilename.IsEmpty())
        return false;

    CExcel& excel = DATA_TO_CEXCEL;
    excel.SaveAs(m_wfilename);
    return true;
}

bool DbExcel::SaveAs(const char *filename)
{
    CString wname(filename);
#if defined(UNICODE)
    return SaveAs(wname);
#else
    return SaveAs(AnsiStrToWideStr(wname));
#endif
}

bool DbExcel::SaveAs(const WCHAR *wfilename)
{
    CExcel& excel = DATA_TO_CEXCEL;
    if (0 != excel.SaveAs(wfilename))
    {
        if (m_wfilename != wfilename)
        {
            m_wfilename = wfilename;
        }
    }
    return true;
}

bool DbExcel::SelectSheet(const char *sheet)
{
    return SelectSheet(CString(sheet));
}

bool DbExcel::SelectSheet(const WCHAR *wsheet)
{
    DATA_TO_CEXCEL.SelectSheet(wsheet);
    return true;
}

bool DbExcel::SelectSheet(int index)
{
    DATA_TO_CEXCEL.SelectSheet(index);
    return true;
}

bool DbExcel::GetCell(int iRow, int iCol, Variant& var) const
{
    if (iRow <= 0 || iCol <= 0)
        return false;

    CExcel& excel = DATA_TO_CEXCEL;
    var = excel.GetCell(iRow, iCol);
    return true;
}

bool DbExcel::GetCell(int iRow, const char* col, Variant& var) const
{
    if (iRow <= 0)
        return false;
    int iCol = StrColToCol(col);
    if (iCol <= 0)
        return false;

    return GetCell(iRow, iCol, var);
}

bool DbExcel::SetCell(int iRow, int iCol, const Variant& var)
{
    if (iRow <= 0 || iCol <= 0)
        return false;

    CExcel& excel = DATA_TO_CEXCEL;
    excel.SetCell(iRow, iCol, var.ToVARIANT());
    return true;
}

bool DbExcel::SetCell(int iRow, const char* col, const Variant& var)
{
    if (iRow <= 0)
        return false;
    int iCol = StrColToCol(col);
    if (iCol <= 0)
        return false;

    return SetCell(iRow, iCol, var);
}

bool DbExcel::GetCell(int iRow, const char* col, CString& wstr) const
{
    if (iRow <= 0)
        return false;
    int iCol = StrColToCol(col);
    if (iCol <= 0)
        return false;

    CExcel& excel = DATA_TO_CEXCEL;
    VARIANT var = excel.GetCell(iRow, iCol);
    wstr = (TCHAR *)(_bstr_t)var;
    return true;
}

bool DbExcel::SetCell(int iRow, const char* col, const WCHAR* wstr)
{
    if (iRow <= 0)
        return false;
    int iCol = StrColToCol(col);
    if (iCol <= 0)
        return false;

    CExcel& excel = DATA_TO_CEXCEL;
    excel.SetCell(iRow, iCol, wstr);
    return true;
}

#if (_MFC_VER  > 0x0600)
bool DbExcel::GetCell(int iRow, const char* col, CStringA& str) const
{
    if (iRow <= 0)
        return false;
    int iCol = StrColToCol(col);
    if (iCol <= 0)
        return false;

    CExcel& excel = DATA_TO_CEXCEL;
    VARIANT var = excel.GetCell(iRow, iCol);
    str = var;
    return true;
}
#endif

bool DbExcel::SetCell(int iRow, const char* col, const char* str)
{
    if (iRow <= 0)
        return false;
    int iCol = StrColToCol(col);
    if (iCol <= 0)
        return false;

    CExcel& excel = DATA_TO_CEXCEL;
    excel.SetCell(iRow, iCol, str);
    return true;
}

bool DbExcel::GetRange(int iRowStart, int iRowEnd, const char* strColStart, const char* strColEnd, DbRange &dbRange)
{
    if (iRowStart < 1 || iRowEnd < iRowStart)
        return false;

    int iColStart = StrColToCol(strColStart);
    int iColEnd = StrColToCol(strColEnd);
    if (iColStart < 1 || iColEnd < iColStart)
        return false;

    CExcel& excel = DATA_TO_CEXCEL;
    dbRange.SetRowCount(iRowEnd - iRowStart + 1);
    dbRange.SetColumnCount(iColEnd - iColStart + 1);

    for (int iRow=iRowStart; iRow<=iRowEnd; iRow++)
    for (int iCol=iColStart; iCol<=iColEnd; iCol++)
    {
        Variant& cell = dbRange.GetAt(iRow-iRowStart, iCol-iColStart);
        if (this->GetCell(iRow, iCol, cell) == false)
            return false;
    }

    dbRange.ClearDirty();
    return true;
}

int DbExcel::GetUsedRowCount() const
{
    CExcel& excel = DATA_TO_CEXCEL;
    Range usedRange, range;
    usedRange.AttachDispatch(excel.sheet.GetUsedRange());
    range.AttachDispatch(usedRange.GetRows());
    int nRowCount = (int)range.GetCount();
    return nRowCount;
}

int DbExcel::GetUsedColCount() const
{
    CExcel& excel = DATA_TO_CEXCEL;
    Range usedRange, range;
    usedRange.AttachDispatch(excel.sheet.GetUsedRange());
    range.AttachDispatch(usedRange.GetColumns());
    int nColCount = (int)range.GetCount();
    return nColCount;
}
