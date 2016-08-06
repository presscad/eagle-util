
#include "excel.h"

/************************************
  REVISION LOG ENTRY
  Revision By: abao++
  Revised on 2006-8-11 8:13:15
  Comments: ...
 ************************************/

/////////////////////////////////////////////////////////////////////////////////////////
// 单元格区域类
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
// EXCEL操作类

//=============================================================================
// 构造函数, 启动EXCEL服务, 并获取所有的工作簿(文件)
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
// 析构函数, 关闭当前文件,
CExcel::~CExcel()
{
}

void CExcel::Close()
{
	//---------------------------------------------------------------
	// 关闭相关资源
	COleVariant covOptional((long)DISP_E_PARAMNOTFOUND, VT_ERROR);
	workbook.Close(covOptional,covOptional,covOptional);	// 关闭当前文件
	workbooks.Close();										// 关闭获取的工作簿
	App.Quit();												// 退出创建的服务

	//---------------------------------------------------------------
	// 释放绑定的接口
	shapes.ReleaseDispatch();					// 释放图片接口
	range.ReleaseDispatch();					// 释放单元格接口
	sheet.ReleaseDispatch();					// 释放工作表接口
	sheets.ReleaseDispatch(); 					// 释放工作表集合接口
	workbook.ReleaseDispatch();					// 释放工作簿接口
	workbooks.ReleaseDispatch();				// 释放工作簿集合接口
	App.ReleaseDispatch();						// 释放服务接口
}

//=============================================================================
// 添加某个文件到EXCEL服务中, 如果指定文件名,则打开该文件
// 如果不指定文件名, 则新建一个文件
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
	SelectSheet(1);			// 选择第一个工作表, 以邦定相关的单元格等
}

#if (_MFC_VER > 0x0600)
void CExcel::AddNewFile(const CStringA& ExtPath)
{
    CStringW WExtPath(ExtPath);
    AddNewFile(WExtPath);
}
#endif

//=============================================================================
// 根据工作表表名, 选中该工作表
_Worksheet& CExcel::SelectSheet(const WCHAR *wSheetName)
{
	sheet.AttachDispatch(sheets.GetItem(_variant_t(wSheetName)),true);
	range.AttachDispatch(sheet.GetCells(),true);
	shapes.AttachDispatch( sheet.GetShapes() );		// 获取形状集合
	return sheet;
}

//=============================================================================
// 根据工作表索引, 选中该工作表
_Worksheet& CExcel::SelectSheet(int index)//选择一个已知表名的表
{
	sheet.AttachDispatch(sheets.GetItem(_variant_t((long)index)));
	range.AttachDispatch(sheet.GetCells(),true);
	shapes.AttachDispatch(sheet.GetShapes());		// 获取形状集合
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
// 读取指定行列的单元格内容
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
// 往指定行列的单元格中写入字符串
void CExcel::SetCell(int row,int col,const WCHAR *wstr)
{
	range.SetItem(_variant_t((long)row),_variant_t((long)col),_variant_t(wstr));
}

//=============================================================================
// 往指定行列的单元格中写入字符串
void CExcel::SetCell(int row,int col, const char* str)
{
	SetCell(row,col,CString(str));
}

//=============================================================================
// 往指定行列的单元格中写入整型数据
void CExcel::SetCell(int row,int col,long lv)
{
    CString t;
	t.Format(_T("%ld"), lv);
	SetCell(row,col,t);
}

//=============================================================================
// 往指定行列的单元格中写入浮点型数据, 最后一个参数为保留小数点后的位数
void CExcel::SetCell(int row,int col,double dv,int n)
{
	CString t;
	CString format;
	format.Format(_T("%%.%dlf"),n);
	t.Format(format,dv);
	SetCell(row,col,t);
}

//=============================================================================
// 把当前文件另存为指定的文件名
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
// 把当前文件另存为指定的文件名
int CExcel::SaveAs(const char *FileName)
{
	CString fileName(FileName);
    return SaveAs(fileName);
}

//=============================================================================
// 把当前工作表复制为指定名称的新工作表
void CExcel::CopySheet(_Worksheet &sht)
{
	sheet.Copy(vtMissing,_variant_t(sht));
}

//=============================================================================
// 获取range
Range& CExcel::GetRange(const CString& RangeStart, const CString& RangeEnd)
{
	range=sheet.GetRange(COleVariant(RangeStart), COleVariant(RangeEnd));	
	return range;
}

//=============================================================================
// 获取range 参数为A1:A2模式
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
// 合并Range
void CExcel::MergeRange(const CString& RangeStr)
{
	GetRange(RangeStr).Merge(COleVariant(long(1)));
}

//=============================================================================
// 激活工作簿
_Worksheet& CExcel::ActiveSheet()
{
	sheet=workbook.GetActiveSheet();
	return sheet;
}

//=============================================================================
// 插入图片
// 参数中的高度和宽度指插入图片后,此图片缩放后的高度,如果参数为-1将使用原图片的大小
ShapeRange& CExcel::AddPicture(LPCTSTR Filename, float Left, float Top, float Width, float Height)
{
	sharpRange = shapes.AddPicture(Filename,0, 1,Left,Top,Width,Height); 
	return sharpRange;
}
