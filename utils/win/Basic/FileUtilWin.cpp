#include <afx.h>         // MFC core and standard components
#include <string>
#include <fstream>
#include <direct.h>
#include <io.h>
#include <shlobj.h>
#include "FileUtil.h"
#include "StrUtil.h"

#if defined(_MSC_VER) && defined(_DEBUG)
#define new DEBUG_NEW
#endif


namespace utils {

bool FileExists(const char* pathname)
{
    DWORD att = ::GetFileAttributesA(pathname);
    if (att == (DWORD)-1)
        return FALSE;
    if(att & FILE_ATTRIBUTE_DIRECTORY)
        return false;
    else
        return true;
}

bool RecursiveCreateDirectory(const char *directory)
{
    int res = SHCreateDirectoryExA(NULL, directory, NULL);
    return (ERROR_SUCCESS == res);
}

// Checks the existence of a Windows directory.
bool DirectoryExists(const char* directory)
{
    DWORD att = ::GetFileAttributesA(directory);
    if (att == (DWORD)-1)
        return false;
    return (att & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

std::string GetCurModulePathname()
{
    std::string name;
    char buff[MAX_PATH];
    DWORD res = ::GetModuleFileNameA(NULL, buff, MAX_PATH);
    if (res != 0)
        name = buff;
    return name;
}

// "c:\folder\file.txt" => "c:\folder\"
std::string GetFilePath(const char* pathname)
{
    std::string path;
    if (pathname)
        path = pathname;
    size_t k = path.rfind('\\');
    if (k == std::string::npos)
    {
        path.clear();
    }
    else
    {
        path = path.substr(0, k+1);
    }
    return path;
}

// "c:\folder\file.txt" => "file.txt"
std::string GetFileName(const char* pathname)
{
    std::string name;
    if (pathname)
        name = pathname;
    size_t k = name.rfind('\\');
    if (k != std::string::npos)
    {
        name = name.substr(k+1);
    }
    return name;
}

// "c:\folder\file.txt" => "file"
std::string GetFileTitle(const char* pathname)
{
    std::string title, name;
    name = GetFileName(pathname);
    if (name.empty())
        return name;

    size_t k = name.rfind('.');
    if (k != std::string::npos)
    {
        title = name.substr(0, k);
    }
    else
    {
        title = name;
    }

    return title;
}



bool WriteAllLines(const TCHAR * name, const CStringArray &linesArray)
{
   bool bRes = false;

   CStdioFile file;
   if (TRUE == file.Open(name, CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive))
   {
      for (int i=0; i<linesArray.GetSize(); i++)
      {
		  std::string line;
#ifdef _UNICODE
         WideStrToAnsiStr(line, linesArray[i]);
#else
         line = linesArray[i];
#endif
         line += "\n";

		 file.Write((const void *)line.c_str(), (UINT)line.length() * sizeof(line[0]));
      }
      file.Close();

      bRes = true;
   }

   return bRes;
}

bool GetLine(std::ifstream &fs, std::string &line) {
    line.clear();
    do{
        if(std::getline(fs, line)) {
            if(fs.good() && line.empty()){
                continue;
            }
            return true;
        } else {
            return false;
        }
    } while(true);
    return false;
}

bool ReadAllFromFile(const char *path, std::string &data)
{
    std::ifstream in(path);
    std::string line;

    data.clear();
    while(GetLine(in, line)) {
        data += line + '\n';
    }
    return true;
}

bool WriteStrToFile(const char *path, const std::string &data)
{
    std::ofstream out(path);
    if (!out.good()) return false;
    out << data;
    return true;
}

bool CreateDirNested(const char *pDir)
{
    int iRet = 0;
    int iLen;
    char* pszDir;

    if(NULL == pDir) {
        return false;
    }
    if (_access(pDir, 0) == 0) {
        return true;
    }

    pszDir = _strdup(pDir);
    iLen = (int)strlen(pszDir);

    for (int i = 0;i < iLen; i++) {
        if (pszDir[i] == '\\' || pszDir[i] == '/') { 
            pszDir[i] = '\0';

            // create the file if it does not exist
            iRet = _access(pszDir, 0);
            if (iRet != 0) {
                iRet = _mkdir(pszDir);
                if (iRet != 0) {
                    return false;
                }
            }
            pszDir[i] = '/'; // Replace '\' with '/' to support Linux
        }
    }
    
    if (_access(pszDir, 0) != 0) {
        iRet = _mkdir(pszDir);
    }
    free(pszDir);
    return iRet == 0;
}

#if 0
void CsvLinePopulate(vector<string> &record, const char *line, char delimiter)
{
    int linepos = 0;
    bool inquotes = false;
    char c;
    int linemax = (int)strlen(line);
    string curstring;
    record.clear();

    while(line[linepos]!=0 && linepos < linemax)
    {
        c = line[linepos];

        if (!inquotes && curstring.length()==0 && c=='"')
        {
            //beginquotechar
            inquotes=true;
        }
        else if (inquotes && c=='"')
        {
            //quotechar
            if ( (linepos+1 <linemax) && (line[linepos+1]=='"') ) 
            {
                //encountered 2 double quotes in a row (resolves to 1 double quote)
                curstring.push_back(c);
                linepos++;
            }
            else
            {
                //endquotechar
                inquotes=false; 
            }
        }
        else if (!inquotes && c==delimiter)
        {
            //end of field
            record.push_back( curstring );
            curstring="";
        }
        else if (!inquotes && (c=='\r' || c=='\n') )
        {
            record.push_back( curstring );
            return;
        }
        else
        {
            curstring.push_back(c);
        }
        linepos++;
    }
    record.push_back( curstring );
    return;
}
#else

void CsvLinePopulate(std::vector<std::string> &record, const char *line, char delimiter)
{
    int linepos = 0;
    bool inquotes = false;
    char c;
    int linemax = (int)strlen(line);

    char curstring[1024];
    int cur_cur = 0;
    int rec_num = 0, rec_size = (int)record.size();

    while(linepos < linemax)
    {
        c = line[linepos];

        if (!inquotes && cur_cur==0 && c=='"')
        {
            //beginquotechar
            inquotes=true;
        }
        else if (inquotes && c=='"')
        {
            //quotechar
            if ( (linepos+1 <linemax) && (line[linepos+1]=='"') ) 
            {
                //encountered 2 double quotes in a row (resolves to 1 double quote)
                curstring[cur_cur++] = c;
                linepos++;
            }
            else
            {
                //endquotechar
                inquotes=false; 
            }
        }
        else if (!inquotes && c==delimiter)
        {
            //end of field
            curstring[cur_cur] = '\0';
            if (rec_num >= rec_size) {
                record.push_back( curstring );
                rec_size++;
            } else {
                record[rec_num] = curstring;
            }
            rec_num++;

            cur_cur = 0;
        }
        else if (!inquotes && (c=='\r' || c=='\n') )
        {
            break;
        }
        else
        {
            curstring[cur_cur++] = c;
        }
        linepos++;
    }

    curstring[cur_cur] = '\0';
    if (rec_num >= rec_size) {
        record.push_back( curstring );
        rec_size++;
    } else {
        record[rec_num] = curstring;
    }
    rec_num++;

    if (rec_size > rec_num) {
        record.resize(rec_num);
    }
    return;
}
#endif

}
