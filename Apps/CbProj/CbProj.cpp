// CbProj.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "CbProj.h"
#include "Ini/SimpleIni.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// key word to replace in template file
#define PROJECTNAME _T("[ProjectName]")
#define SOURCEFILES _T("[SourceFiles]")


// The one and only application object
CWinApp theApp;

using namespace std;

utils::CSimpleIni g_cfgMgr;
std::string g_strPrjName;
std::string g_strTemplateFile;
std::string g_strSourceFile;
std::string g_strPathPrefix;

int GetParameters()
{
   int nRetCode = 0;

   if (nRetCode == 0)
   {
       g_strPrjName = g_cfgMgr.GetString("Parameters", "ProjectName", NULL).c_str();
       if (g_strPrjName.empty())
       {
           _tprintf(_T("Error: ProjectName is not specified. Update CbProj.ini to set ProjectName parameter.\n"));
           nRetCode = 10;
       }
   }

   if (nRetCode == 0)
   {
       g_strTemplateFile = g_cfgMgr.GetString("Parameters", "Template", NULL);
       if (g_strTemplateFile.empty())
       {
           _tprintf(_T("Error: Template is not specified. Update CbProj.ini to set Project Template file name.\n"));
           nRetCode = 11;
       }
   }

   if (nRetCode == 0)
   {
       g_strSourceFile = g_cfgMgr.GetString("Parameters", "Source", NULL);
       if (g_strSourceFile.empty())
       {
           _tprintf(_T("Error: Source is not specified. Update CbProj.ini to set Source file name.\n"));
           nRetCode = 12;
       }
   }

   if (nRetCode == 0)
   {
       g_strPathPrefix = g_cfgMgr.GetString("Parameters", "PathPrefix", NULL);
       //g_strPathPrefix.Trim();
   }

   return nRetCode;
}

CStringA ReadTemplate()
{
    CStringA all;
    CFile file;

    if (!file.Open(CString(g_strTemplateFile.c_str()), CFile::modeRead))
    {
        _tprintf(_T("Invalid project templateFile file: %s\n"), g_strTemplateFile);
        return all;
    }

    CFileStatus rStatus;
    file.GetStatus(rStatus);
    all.GetBuffer(int(rStatus.m_size) + 1);

    file.Read((void *)(const char *)all, int(rStatus.m_size));
    ((char *)(const char *)all)[rStatus.m_size] = 0;
    all.ReleaseBuffer();
    file.Close();

    all.Replace("\r\n", "\n");
    all.Replace("\r", "\n");

    return all;
}

CStringA ParseSourceFiles()
{
    int i;
    CStringA srcFilesFormatted;
    CStringArray rawLines;

    if (FALSE == ReadAllLines(g_strSourceFile, rawLines))
    {
        _tprintf(_T("Invalid source list file: %s\n"), g_strSourceFile);
        return srcFilesFormatted;
    }

    for (i=(int)rawLines.GetCount()-1; i>=0; i--)
    {
        CString &line = rawLines[i];
        line.Trim();
        line.Replace(_T('\t'), _T(' '));
        line.Replace(_T('/'), _T('\\'));
        if (line.Right(1) == _T("\\"))
        {
            line.TrimRight(_T('\\'));
            line.TrimRight();
        }

        if (line.IsEmpty())
            rawLines.RemoveAt(i);
    }

/* example:
		<Unit filename="..\..\..\..\external\icu4c\i18n\utmscale.c">
			<Option compilerVar="CC" />
		</Unit>
		<Unit filename="..\..\..\..\external\icu4c\i18n\utrans.cpp" />
*/
    for (i=0; i<rawLines.GetCount(); i++)
    {
        // e.g. rawLines[i] = "ucal.cpp        ucol_bld.cpp ucol_cnt.cpp"
        // parse rawLines[i] into array of single string
        CArray<CStringA> arrFile;
        SplitStringA(CStringA(rawLines[i]), ' ', arrFile);

        for (int k=0; k<(int)arrFile.GetCount(); k++)
        {
            CStringA line("\t\t<Unit filename=\"");
            line += g_strPathPrefix.c_str();
            line += arrFile[k];
            line += "\"";

            if (arrFile[k].Right(2) == ".c" || arrFile[k].Right(2) == ".C")
            {
                line += ">\n\t\t\t<Option compilerVar=\"CC\" />\n\t\t</Unit>\n";
            }
            else
            {
                line += " />\n";
            }

            srcFilesFormatted += line;
        }
    }

    return srcFilesFormatted;
}

int GenerateCbProj()
{
    CStringA strProjA = ReadTemplate();
    if (strProjA.IsEmpty())
        return 21;

    // replace PROJECTNAME with g_strPrjName
    CStringA strPrjName_Template(PROJECTNAME);
    CStringA strPrjNameA(g_strPrjName.c_str());
    strProjA.Replace(strPrjName_Template, strPrjNameA);

    // Parse soure file list
    CStringA strSrcFiles = ParseSourceFiles();
    if (strSrcFiles.IsEmpty())
        return 22;
    CStringA strSrc_Template(SOURCEFILES);
    strProjA.Replace(strSrc_Template, strSrcFiles);

    // write project
    CFile file;
    CString fileOut = CString(g_strPrjName.c_str()) + _T(".cbp");
    if (!file.Open(fileOut, CFile::modeCreate | CFile::modeWrite))
    {
        _tprintf(_T("Cannot open file for writing: %s\n"), fileOut);
        return 25;
    }
    file.Write((const char *)strProjA, strProjA.GetLength());
    file.Close();

    _tprintf(_T("%s is created successfully.\n"), fileOut);
    return 0;
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
   int nRetCode = 0;

   // initialize MFC and print and error on failure
   if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
   {
      _tprintf(_T("Fatal Error: MFC initialization failed\n"));
      nRetCode = 1;
   }
   else
   {
       _tprintf(_T("CbProj - Create Code::Blocks project, by yingdai@motorola.com\n\n"));
   }

   if (nRetCode == 0)
   {
       nRetCode = GetParameters();
   }

   if (nRetCode == 0)
   {
       GenerateCbProj();
   }

   return nRetCode;
}
