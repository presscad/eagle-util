
#include "excel.h"

/************************************
  REVISION LOG ENTRY
  Revision By: abao++
  Revised on 2006-8-11 8:13:15
  Comments: ...
 ************************************/

/////////////////////////////////////////////////////////////////////////////////////////
// ��Ԫ��������
CRange::CRange(Range& range)
{
	rg=range;
}
void CRange::Merge()
{
	rg.Merge(COleVariant((short)1));
}
CRange& CRange::operator=(const WCHAR *wstr)
{
    COleVariant var;
    var.Clear();
    var.vt = VT_BSTR;
    var.bstrVal = CString(wstr).AllocSysString();
	rg.SetValue(var);
	return *this;
}

CRange& CRange::operator=(const char* str)
{
    COleVariant var;
    var.Clear();
    var.vt = VT_BSTR;
    var.bstrVal = CString(str).AllocSysString();
	rg.SetValue(var);
	return *this;
}

CRange& CRange::operator=(Range& range)
{
	rg=range;
	return *this;
}

int CRange::Border(short mode,long BoderWidth,long ColorIndex, VARIANT color)
{
	rg.BorderAround(COleVariant((short)mode),(long)BoderWidth,(long)ColorIndex,color);	
	return 1;
}
int CRange::SetHAlign(RangeHAlignment mode)
{
	rg.SetHorizontalAlignment(COleVariant((short)mode));
	return 1;
}
int CRange::SetVAlign(RangeVAlignment mode)
{
	rg.SetVerticalAlignment(COleVariant((short)mode));
	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////
// EXCEL������

//=============================================================================
// ���캯��, ����EXCEL����, ����ȡ���еĹ�����(�ļ�)
CExcel::CExcel()
{
	if(!App.CreateDispatch(_T("Excel.Application"), NULL)) 
	{ 
		::MessageBox(NULL, _T("Error"), _T("Error in creating Excel service!"), 0); 
		exit(1); 
	}
	
	workbooks.AttachDispatch(App.GetWorkbooks(),true);
	this->SetVisible(false);
}

//=============================================================================
// ��������, �رյ�ǰ�ļ�,
CExcel::~CExcel()
{
}

void CExcel::Close()
{
	//---------------------------------------------------------------
	// �ر������Դ
	COleVariant covOptional((long)DISP_E_PARAMNOTFOUND, VT_ERROR);
	workbook.Close(covOptional,covOptional,covOptional);	// �رյ�ǰ�ļ�
	workbooks.Close();										// �رջ�ȡ�Ĺ�����
	App.Quit();												// �˳������ķ���

	//---------------------------------------------------------------
	// �ͷŰ󶨵Ľӿ�
	shapes.ReleaseDispatch();					// �ͷ�ͼƬ�ӿ�
	range.ReleaseDispatch();					// �ͷŵ�Ԫ��ӿ�
	sheet.ReleaseDispatch();					// �ͷŹ�����ӿ�
	sheets.ReleaseDispatch(); 					// �ͷŹ������Ͻӿ�
	workbook.ReleaseDispatch();					// �ͷŹ������ӿ�
	workbooks.ReleaseDispatch();				// �ͷŹ��������Ͻӿ�
	App.ReleaseDispatch();						// �ͷŷ���ӿ�
}

//=============================================================================
// ���ĳ���ļ���EXCEL������, ���ָ���ļ���,��򿪸��ļ�
// �����ָ���ļ���, ���½�һ���ļ�
void CExcel::AddNewFile(const CString& WExtPath)
{
	if(WExtPath.IsEmpty())
	{
		COleVariant covOptional((long)DISP_E_PARAMNOTFOUND, VT_ERROR);
		workbook.AttachDispatch(workbooks.Add(covOptional));	
	}
	else
	{
		workbook.AttachDispatch(workbooks.Add(_variant_t(WExtPath)));			
	}
	sheets.AttachDispatch(workbook.GetWorksheets(),true);
	SelectSheet(1);			// ѡ���һ��������, �԰��صĵ�Ԫ���
}

#if (_MFC_VER > 0x0600)
void CExcel::AddNewFile(const CStringA& ExtPath)
{
    CStringW WExtPath(ExtPath);
    AddNewFile(WExtPath);
}
#endif

//=============================================================================
// ���ݹ��������, ѡ�иù�����
_Worksheet& CExcel::SelectSheet(const WCHAR *wSheetName)
{
	sheet.AttachDispatch(sheets.GetItem(_variant_t(wSheetName)),true);
	range.AttachDispatch(sheet.GetCells(),true);
	shapes.AttachDispatch( sheet.GetShapes() );		// ��ȡ��״����
	return sheet;
}

//=============================================================================
// ���ݹ���������, ѡ�иù�����
_Worksheet& CExcel::SelectSheet(int index)//ѡ��һ����֪�����ı�
{
	sheet.AttachDispatch(sheets.GetItem(_variant_t((long)index)));
	range.AttachDispatch(sheet.GetCells(),true);
	shapes.AttachDispatch(sheet.GetShapes());		// ��ȡ��״����
	return sheet;
}

//=============================================================================
// 
Range& CExcel::ActiveSheetRange()
{
	range.AttachDispatch(sheet.GetCells(),true);
	return range;
}

//=============================================================================
// ��ȡָ�����еĵ�Ԫ������
VARIANT CExcel::GetCell(int row, int col)
{
	Range rg = range.GetItem(_variant_t((long)row),_variant_t((long)col)).pdispVal;
	return rg.GetText();
}

void CExcel::SetCell(int row,int col,const VARIANT& var)
{
	range.SetItem(_variant_t((long)row),_variant_t((long)col), var);
}

//=============================================================================
// ��ָ�����еĵ�Ԫ����д���ַ���
void CExcel::SetCell(int row,int col,const WCHAR *wstr)
{
	range.SetItem(_variant_t((long)row),_variant_t((long)col),_variant_t(wstr));
}

//=============================================================================
// ��ָ�����еĵ�Ԫ����д���ַ���
void CExcel::SetCell(int row,int col, const char* str)
{
	SetCell(row,col,CString(str));
}

//=============================================================================
// ��ָ�����еĵ�Ԫ����д����������
void CExcel::SetCell(int row,int col,long lv)
{
    CString t;
	t.Format(_T("%ld"), lv);
	SetCell(row,col,t);
}

//=============================================================================
// ��ָ�����еĵ�Ԫ����д�븡��������, ���һ������Ϊ����С������λ��
void CExcel::SetCell(int row,int col,double dv,int n)
{
	CString t;
	CString format;
	format.Format(_T("%%.%dlf"),n);
	t.Format(format,dv);
	SetCell(row,col,t);
}

//=============================================================================
// �ѵ�ǰ�ļ����Ϊָ�����ļ���
int CExcel::SaveAs(const WCHAR *WFileName)
{
    // no owerwrite prompt
    this->App.SetAlertBeforeOverwriting(FALSE);
    this->App.SetDisplayAlerts(FALSE);

	COleVariant covOptional((long)DISP_E_PARAMNOTFOUND, VT_ERROR);
	this->workbook.SaveAs(_variant_t(WFileName),covOptional,covOptional,covOptional,
		covOptional,covOptional,1,covOptional,covOptional,covOptional,covOptional);
	return 1;
}

//=============================================================================
// �ѵ�ǰ�ļ����Ϊָ�����ļ���
int CExcel::SaveAs(const char *FileName)
{
	CString fileName(FileName);
    return SaveAs(fileName);
}

//=============================================================================
// �ѵ�ǰ��������Ϊָ�����Ƶ��¹�����
void CExcel::CopySheet(_Worksheet &sht)
{
	sheet.Copy(vtMissing,_variant_t(sht));
}

//=============================================================================
// ��ȡrange
Range& CExcel::GetRange(const CString& RangeStart, const CString& RangeEnd)
{
	range=sheet.GetRange(COleVariant(RangeStart), COleVariant(RangeEnd));	
	return range;
}

//=============================================================================
// ��ȡrange ����ΪA1:A2ģʽ
Range& CExcel::GetRange(const CString& RangeStr)
{
	int pos=RangeStr.Find(L':');
	if(pos>0)
	{
		CString a,b;
		a=RangeStr.Left(pos);
		b=RangeStr.Right(RangeStr.GetLength()-pos-1);
		return GetRange(a,b);
	}
	else
	{
		return GetRange(RangeStr,RangeStr);
	}
}

//=============================================================================
// �ϲ�Range
void CExcel::MergeRange(const CString& RangeStr)
{
	GetRange(RangeStr).Merge(COleVariant(long(1)));
}

//=============================================================================
// �������
_Worksheet& CExcel::ActiveSheet()
{
	sheet=workbook.GetActiveSheet();
	return sheet;
}

//=============================================================================
// ����ͼƬ
// �����еĸ߶ȺͿ��ָ����ͼƬ��,��ͼƬ���ź�ĸ߶�,�������Ϊ-1��ʹ��ԭͼƬ�Ĵ�С
ShapeRange& CExcel::AddPicture(LPCTSTR Filename, float Left, float Top, float Width, float Height)
{
	sharpRange = shapes.AddPicture(Filename,0, 1,Left,Top,Width,Height); 
	return sharpRange;
}
