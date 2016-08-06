#ifndef _abaoExcel_h_

/******************************************************************************
����: һ�����õĲ���EXCEL����: 
1,д��
2,��ȡ
3,��Ԫ��ϲ�
4,��Ԫ���ʽ����
5,����ͼƬ
����: �Ѽ�������VC����EXCEL������, ������ԭʼ�ķ���, һ����������, ������, ������
�ǳ�������, ����abao++��װ����, �о��ǳ���, Ϊ����Ӧ�Լ�����Ҫ��������������
��ȡ�Ͳ���ͼƬ, ��������ע��, ���ڴ�Ҹ��������Ķ�
����: �ٶȲ��Ǻܿ�, û��ODBC���ֶ�д�ٶȿ�, ��������ܸ�ǿ��, ������������ݿ�ӿ�
����EXCEL��ר��
����: Win2000+VC6
�޸�: �ſ���		EMAIL:ZHJYSOFT@163.COM		QQ:44049283		QQȺ:29131322
�汾: 07.01.07	V2.0
ȡ��: abao++ 2006-8-11 8:13:07 V1.0
����: WWW.VCSOFT.ORG   �¾�����VCר����̳ 
ʹ��: ����ǰ��ȷ���Ѿ����������º�����ʼ��com�⣬
if (CoInitialize(NULL)!=0) 
{ 
AfxMessageBox("��ʼ��COM֧�ֿ�ʧ��!");	
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
	// EXCEL ��������
	CExcel Excel;
	Excel.AddNewFile();							// ��һ�����ļ�
	Excel.SetVisible(true);						// ���ÿɼ�
	Excel.CopySheet(Excel.SelectSheet(1));		// �ѹ�����1����һ��
	Excel.ActiveSheet().SetName("heheok��");	// ������¹���������
	Excel.SelectSheet("sheet1");				// �������1
	Excel.SetCell(1,1,3.14159267,6);			// Щ����
	long t=1000;
	Excel.SetCell(1,1,t);						// д����
	Excel.SetCell(1,3,CString("CString"));		// д����
	Excel.SetCell(1,4,"C Style string");		// д����

	//---------------------------------------------------------------
	// �����ǵ�Ԫ�����
	CRange range(Excel.GetRange("A1:B1"));		// ѡ��Ԫ��
	range.Merge();								// �ϲ�
	range.Border();								// ���ñ߿�
	range.SetHAlign(HAlignCenter);				// ˮƽ�ʹ�ֱ����
	range.SetVAlign(VAlignTop);
	range=Excel.GetRange("B2");
	range.Border();
	range="123456";
	range=Excel.GetRange("C2");
	range=CString("CString");
	range=Excel.GetRange("D2");
	range="C Style string";
	Excel.SelectSheet("heheok��");
	Excel.SetCell(1,1,"���ݶ���sheet1������");
	VARIANT vt = Excel.GetCell(1,1);			// ��ȡ����
	Excel.AddPicture(g_path.GetExePath()+"\\����.bmp",10,100,-1,-1);	// ����ͼƬ
	Excel.SaveAs(CString(g_path.GetExePath()+"\\test.xls"));		// �������EXCEL�ļ�����

#endif // #if 0

enum RangeHAlignment{HAlignDefault=1,HAlignCenter=-4108,HAlignLeft=-4131,HAlignRight=-4152};
enum RangeVAlignment{VAlignDefault=2,VAlignCenter=-4108,VAlignTop=-4160,VAlignBottom=-4107};

///////////////////////////////////////////////////////////////////////////////////////////
// EXCEL��Ԫ�������
class CRange
{
    Range rg;							//������range
public:
    CRange(Range& range);				//��һ��range����
    CRange& operator=(const WCHAR *wstr);	//����һ��CString
    CRange& operator=(const char* str);	//����char*
    CRange& operator=(Range& range);	//��ֵ��һ��range

    void Merge();		//�ϲ�
    //���ñ߿򣬲��������Ǻ����
    int Border(short mode=1,long BoderWidth=1,long ColorIndex=1,
        VARIANT color=COleVariant((long)DISP_E_PARAMNOTFOUND,VT_ERROR));

    //����ˮƽ���뷽ʽ
    int SetHAlign(RangeHAlignment mode=HAlignDefault);

    //������ֱ����
    int SetVAlign(RangeVAlignment mode=VAlignDefault);
};

///////////////////////////////////////////////////////////////////////////////////////////
// EXCEL������
class CExcel
{
public:
    CExcel();
    ~CExcel();

    //=========================================================================
    // 
    _Application	App;			// EXCEL Ӧ�ó���
    Workbooks		workbooks;		// ����������,�൱�����е�EXCEL�ļ�
    _Workbook		workbook;		// ĳ��������,�൱��ĳ��EXCEL�ļ�
    Worksheets		sheets;			// ��������,�൱��һ��EXCEL�ļ��ڵ����й�����
    _Worksheet		sheet;			// ĳ�������� ,�൱��һ���ļ���ĳ��������
    Range			range;			// ��Ԫ������,range
    Shapes			shapes;			// ����ͼƬ�ļ���
    ShapeRange		sharpRange;		// ĳ��ͼƬ

    //=========================================================================
    // EXCEL������ز���
    int SetVisible(bool visible) {App.SetVisible(visible);return 1;}//����Ϊ�ɼ��Լ�����
    int SaveAs(const char *FileName);		//���浽�ļ���
    int SaveAs(const WCHAR *WFileName);		//���浽�ļ���

    //=========================================================================
    // ��������ز���
    _Worksheet& ActiveSheet();		//��ǰ���sheet,��SelectSheet ��ı�
    void CopySheet(_Worksheet &sht);			//����һ��sheet
#if (_MFC_VER  > 0x0600)
	void AddNewFile(const CStringA& ExtPath=CStringA(""));	//��һ��ģ�湹��
#endif
    void AddNewFile(const CString& WExtPath=CString(L""));//��һ��ģ�湹��
    void Close();

    //=========================================================================
    // ѡ������
    _Worksheet& SelectSheet(const WCHAR *wSheetName);	//ѡ��һ����֪�����ı�
    _Worksheet& SelectSheet(const char* SheetName)        //ѡ��һ����֪�����ı�
    {
        return SelectSheet(CString(SheetName));
    };
    _Worksheet& SelectSheet(int index);				//ѡ��һ����֪�����ı�

    //=========================================================================
    // ��д��Ԫ��
    void SetCell(int row,int col,const VARIANT& var);
    void SetCell(int row,int col,const WCHAR* wstr);//ָ�����еĵ�Ԫ������ֵ
    void SetCell(int row,int col,const char* str);  //ָ�����еĵ�Ԫ������ֵ
    void SetCell(int row,int col,long lv);			//ָ����������longֵ
    void SetCell(int row,int col,double dv,int n=6);//ָ���������븡��ֵ������ȡΪָ����С��λ
    VARIANT GetCell(int row, int col);

    //=========================================================================
    // ��Ԫ���������
    Range& ActiveSheetRange();								//��ǰ��range,��ʹ��GetRange��ı�
    Range& GetRange(const CString& RangeStart, const CString& RangeEnd);    //��ȡrange,
    Range& GetRange(const CString& RangeStr);				//��ȡrange A1:A2ģʽ
    void MergeRange(const CString& RangeStr);				//�ϲ�Range

    //=========================================================================
    // ͼƬ����
    ShapeRange& AddPicture(LPCTSTR Filename, float Left, float Top,
        float Width, float Height);		// ����ͼƬ, ����߶ȺͿ��Ϊ-1,��ʹ��ͼƬԭʼ��С

};

#endif
