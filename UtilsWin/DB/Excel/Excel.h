#ifndef _abaoExcel_h_

/******************************************************************************
描述: 一个好用的操作EXCEL的类: 
1,写入
2,读取
3,单元格合并
4,单元格格式操作
5,插入图片
背景: 搜集无数的VC操作EXCEL的内容, 都是最原始的方法, 一个函数下来, 数百行, 用起来
非常不方便, 看到abao++封装的类, 感觉非常好, 为了适应自己的需要增加了两个功能
读取和插入图片, 并增加了注释, 便于大家更清晰地阅读
评价: 速度不是很快, 没有ODBC那种读写速度快, 但这个功能更强大, 后面可能做数据库接口
访问EXCEL的专题
环境: Win2000+VC6
修改: 张俊勇		EMAIL:ZHJYSOFT@163.COM		QQ:44049283		QQ群:29131322
版本: 07.01.07	V2.0
取代: abao++ 2006-8-11 8:13:07 V1.0
发布: WWW.VCSOFT.ORG   致精永恒VC专题论坛 
使用: 调用前请确保已经调用了如下函数初始化com库，
if (CoInitialize(NULL)!=0) 
{ 
AfxMessageBox("初始化COM支持库失败!");	
exit(1); 
} 
******************************************************************************/

#define _abaoExcel_h_

#include "excel9.h"
#include <comdef.h>

// Samples
#if 0
    // Need to call 
       CoInitialize(NULL);
       CoUninitialize();

	//---------------------------------------------------------------
	// EXCEL 基本操作
	CExcel Excel;
	Excel.AddNewFile();							// 建一个空文件
	Excel.SetVisible(true);						// 设置可见
	Excel.CopySheet(Excel.SelectSheet(1));		// 把工作簿1复制一份
	Excel.ActiveSheet().SetName("heheok了");	// 把这个新工作簿命名
	Excel.SelectSheet("sheet1");				// 激活工作簿1
	Excel.SetCell(1,1,3.14159267,6);			// 些数据
	long t=1000;
	Excel.SetCell(1,1,t);						// 写数据
	Excel.SetCell(1,3,CString("CString"));		// 写数据
	Excel.SetCell(1,4,"C Style string");		// 写数据

	//---------------------------------------------------------------
	// 下面是单元格操作
	CRange range(Excel.GetRange("A1:B1"));		// 选单元格
	range.Merge();								// 合并
	range.Border();								// 设置边框
	range.SetHAlign(HAlignCenter);				// 水平和垂直对齐
	range.SetVAlign(VAlignTop);
	range=Excel.GetRange("B2");
	range.Border();
	range="123456";
	range=Excel.GetRange("C2");
	range=CString("CString");
	range=Excel.GetRange("D2");
	range="C Style string";
	Excel.SelectSheet("heheok了");
	Excel.SetCell(1,1,"内容都在sheet1里面呢");
	VARIANT vt = Excel.GetCell(1,1);			// 读取数据
	Excel.AddPicture(g_path.GetExePath()+"\\曲线.bmp",10,100,-1,-1);	// 插入图片
	Excel.SaveAs(CString(g_path.GetExePath()+"\\test.xls"));		// 把这个新EXCEL文件保存

#endif // #if 0

enum RangeHAlignment{HAlignDefault=1,HAlignCenter=-4108,HAlignLeft=-4131,HAlignRight=-4152};
enum RangeVAlignment{VAlignDefault=2,VAlignCenter=-4108,VAlignTop=-4160,VAlignBottom=-4107};

///////////////////////////////////////////////////////////////////////////////////////////
// EXCEL单元格操作类
class CRange
{
    Range rg;							//参数用range
public:
    CRange(Range& range);				//从一个range构造
    CRange& operator=(const WCHAR *wstr);	//填入一个CString
    CRange& operator=(const char* str);	//填入char*
    CRange& operator=(Range& range);	//赋值另一个range

    void Merge();		//合并
    //设置边框，参数还不是很清楚
    int Border(short mode=1,long BoderWidth=1,long ColorIndex=1,
        VARIANT color=COleVariant((long)DISP_E_PARAMNOTFOUND,VT_ERROR));

    //设置水平对齐方式
    int SetHAlign(RangeHAlignment mode=HAlignDefault);

    //设置竖直对齐
    int SetVAlign(RangeVAlignment mode=VAlignDefault);
};

///////////////////////////////////////////////////////////////////////////////////////////
// EXCEL操作类
class CExcel
{
public:
    CExcel();
    ~CExcel();

    //=========================================================================
    // 
    _Application	App;			// EXCEL 应用程序
    Workbooks		workbooks;		// 工作簿集合,相当于所有的EXCEL文件
    _Workbook		workbook;		// 某个工作簿,相当于某个EXCEL文件
    Worksheets		sheets;			// 工作表集合,相当于一个EXCEL文件内的所有工作表
    _Worksheet		sheet;			// 某个工作表 ,相当于一个文件的某个工作表
    Range			range;			// 单元格区域,range
    Shapes			shapes;			// 所有图片的集合
    ShapeRange		sharpRange;		// 某个图片

    //=========================================================================
    // EXCEL程序相关操作
    int SetVisible(bool visible) {App.SetVisible(visible);return 1;}//设置为可见以及隐藏
    int SaveAs(const char *FileName);		//保存到文件名
    int SaveAs(const WCHAR *WFileName);		//保存到文件名

    //=========================================================================
    // 工作簿相关操作
    _Worksheet& ActiveSheet();		//当前活动的sheet,在SelectSheet 后改变
    void CopySheet(_Worksheet &sht);			//复制一个sheet
#if (_MFC_VER  > 0x0600)
	void AddNewFile(const CStringA& ExtPath=CStringA(""));	//从一个模版构造
#endif
    void AddNewFile(const CString& WExtPath=CString(L""));//从一个模版构造
    void Close();

    //=========================================================================
    // 选择工作表
    _Worksheet& SelectSheet(const WCHAR *wSheetName);	//选择一个已知表名的表
    _Worksheet& SelectSheet(const char* SheetName)        //选择一个已知表名的表
    {
        return SelectSheet(CString(SheetName));
    };
    _Worksheet& SelectSheet(int index);				//选择一个已知索引的表

    //=========================================================================
    // 填写单元格
    void SetCell(int row,int col,const VARIANT& var);
    void SetCell(int row,int col,const WCHAR* wstr);//指定行列的单元格填入值
    void SetCell(int row,int col,const char* str);  //指定行列的单元格填入值
    void SetCell(int row,int col,long lv);			//指定行列填入long值
    void SetCell(int row,int col,double dv,int n=6);//指定行列填入浮点值，并截取为指定的小数位
    VARIANT GetCell(int row, int col);

    //=========================================================================
    // 单元格区域操作
    Range& ActiveSheetRange();								//当前的range,在使用GetRange后改变
    Range& GetRange(const CString& RangeStart, const CString& RangeEnd);    //获取range,
    Range& GetRange(const CString& RangeStr);				//获取range A1:A2模式
    void MergeRange(const CString& RangeStr);				//合并Range

    //=========================================================================
    // 图片操作
    ShapeRange& AddPicture(LPCTSTR Filename, float Left, float Top,
        float Width, float Height);		// 插入图片, 如果高度和宽度为-1,则使用图片原始大小

};

#endif
