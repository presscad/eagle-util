// Excel2Txt.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "DbExcel.h"
#include "FileUtil.h"
#include <shlwapi.h>
#pragma   comment(lib, "shlwapi.lib")

// Requirements:
// 1. extract numbers from excel specified sheet and columns
// 2. output the numbers in blocks of specified size, e.g. 100
// 3. each blocks a few lines, each lines contains specified numbers of call NOs, e.g. 10
// OA system has the limiation that each time to input limited call NOs to send SMS to customers

const CString EXCEL_FILE = _T("D:\\Dev\\MyProgram\\eagle-utils\\Apps\\Excel2Txt\\Data\\ÍøÁäÓÅ»Ý-ÏÄÃô.xls");
const char    COLUMN[] = "G";
const TCHAR DELIMETER = _T(',');

const CString MANAGER_NAME = L"ÏÄÃô";
const CString MANAGER_NUMBER = _T("13901585721");

const CString SHEET_NAME = _T("Sheet1");
const BOOL  EMAIL_ADDRESS = 0;
const BOOL  ADD_MANAGER_NUMBER_AT_END_OF_EACH_BLOCK = 0;

// OUTPUT:
const int NUMS_PER_LINE = 10;
const int NUMS_PER_BLOCK = 50;

BOOL WriteToText(CStringArray &arrNum)
{
    CStringArray linesOut;
    CString line;

    if (ADD_MANAGER_NUMBER_AT_END_OF_EACH_BLOCK)
    {
        // make sure end of each block is 13901583749
        for (int i=(int)arrNum.GetSize()-1; i>=0; i--)
        {
            if (i>0 && i%(NUMS_PER_BLOCK-1) == 0)
            {
                arrNum.InsertAt(i, MANAGER_NUMBER);
            }
        }
    }

    int nGroup = 0, nIndex = 0;
    for (int i=0; i<arrNum.GetSize(); i++)
    {
        if (i>0 && (i%NUMS_PER_LINE) == 0)
        {
            //line.TrimRight(DELIMETER);
            line.Trim();
            linesOut.Add(line);
            line.Empty();
        }

        if (i%NUMS_PER_BLOCK == 0)
        {
            nGroup++;

            linesOut.Add(_T(""));
            linesOut.Add(_T("========================================"));
            CString str;
            str.Format(_T("[GROUP %d]"), nGroup);
            linesOut.Add(str);
        }

        line += arrNum[i];
        if (EMAIL_ADDRESS)
        {
            line += _T("@139.com");
        }
        line += DELIMETER;
    }

    line.Trim();
    if (!line.IsEmpty())
        linesOut.Add(line);

    CString TEXT_FILE = EXCEL_FILE;
    if (TEXT_FILE.Right(4).CompareNoCase(_T(".xls")) == 0)
    {
        TEXT_FILE = TEXT_FILE.Left(TEXT_FILE.GetLength() - 4);
    }
    TEXT_FILE += _T(".txt");
    utils::WriteAllLines(TEXT_FILE, linesOut);

    return TRUE;
}



BOOL StrBeginWith(const TCHAR *str1, const TCHAR *str2)
{
	CString s2(str2);
    return ::StrCmpN(str1, str2, s2.GetLength()) == 0;
}


int _tmain(int argc, _TCHAR* argv[])
{
    CoInitialize(NULL);

    {
        DbExcel excel;
        CString str1, str2;

        excel.Open(EXCEL_FILE);
        //excel.SelectSheet(SHEET_NAME);

        int ROW_END = excel.GetUsedRowCount();
        CStringArray arrNum;
        for (int iRow=1; iRow<=ROW_END; iRow++)
        {
            if (true == excel.GetCell(iRow, COLUMN, str1)
                //&& true == excel.GetCell(iRow, "F", str2)
                )
            {
                str1.Trim();
                str2.Trim();

                if ( /*str2 == MANAGER_NAME && */StrBeginWith(str1, _T("1")))
                {
                    arrNum.Add(str1);
                }
            }
        }
        excel.Close();

        WriteToText(arrNum);
    }
    CoUninitialize();

	return 0;
}

