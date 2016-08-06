#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <string>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include "HotSquare.h"
#include "CsvExporter.h"

using namespace std;

static bool ReadAll(const char *path, string &data)
{
    std::ifstream in(path);
    string line;

    data.clear();
    while(GetLine(in, line)) {
        data += line + '\n';
    }
    return true;
}

static bool WriteAll(const char *path, const string &data)
{
    std::ofstream out(path);
    if (!out.good()) return false;
    out << data;
    return true;
}

static int GetFileSize(const char *filename)
{
    struct stat st;
    if (0 == stat(filename, &st)) {
        return st.st_size;
    } else {
        return -1;
    }
}

CsvExporter::CsvExporter(const char *sOutFolder, const char *sSchema, const char *sTableName,
    const char *sTemplateFolder, int nRecCount)
{
    mstrSchema = sSchema;
    mstrTableName = sTableName;
    mnRecordCount = nRecCount;

    mstrBaseFolder = sOutFolder;
    StringReplace(mstrBaseFolder, "/", "\\");
    if (!mstrBaseFolder.empty() && mstrBaseFolder[mstrBaseFolder.size()-1] != '\\') {
        mstrBaseFolder += '\\';
    }
    mstrTemplateFolder = sTemplateFolder;
    if (!mstrTemplateFolder.empty() && mstrTemplateFolder[mstrTemplateFolder.size()-1] != '\\') {
        mstrTemplateFolder += '\\';
    }
    mstrDataFolder = mstrBaseFolder + "index\\" + mstrSchema + "\\" + mstrTableName.substr(0, 2) +
        '\\' + mstrTableName + '\\';
}

CsvExporter::~CsvExporter()
{
}

string CsvExporter::GetDataFilePath()
{
    CreateFolders();
    return mstrDataFolder + "data.csv";
}

bool CsvExporter::GenerateExportFiles()
{
    if (!CreateFolders()) {
        return false;
    }

    {
        string create_sql;
        ReadAll((mstrTemplateFolder + "create.sql").c_str(), create_sql);
        if (create_sql.empty()) {
            mstrError = "File create.sql is not found in CSV template.";
            return false;
        }
        StringReplace(create_sql, "[SCHEMA]", mstrSchema);
        StringReplace(create_sql, "[TABLE]", mstrTableName);
        if (false == WriteAll((mstrDataFolder + "create.sql").c_str(), create_sql)) {
            mstrError = "Cannot write to create.sql.";
            return false;
        }
    }
    {
        string data_ctl;
        ReadAll((mstrTemplateFolder + "data.ctl").c_str(), data_ctl);
        if (data_ctl.empty()) {
            mstrError = "File data.ctl is not found in CSV template.";
            return false;
        }
        StringReplace(data_ctl, "[SCHEMA]", mstrSchema);
        StringReplace(data_ctl, "[TABLE]", mstrTableName);
        if (false == WriteAll((mstrDataFolder + "data.ctl").c_str(), data_ctl)) {
            mstrError = "Cannot write to data.ctl.";
            return false;
        }
    }
    {
        string data_info;
        char buff[64];
        ReadAll((mstrTemplateFolder + "data.info").c_str(), data_info);
        if (data_info.empty()) {
            mstrError = "File data.info is not found in CSV template.";
            return false;
        }

        int size = GetFileSize(GetDataFilePath().c_str());
        if (size <= 0) {
             mstrError = "Incorrect size of file ";
             mstrError += GetDataFilePath();
             return false;
        }
        sprintf(buff, "%d", size);
        StringReplace(data_info, "[FILE_SIZE]", buff);

        sprintf(buff, "%d", mnRecordCount);
        StringReplace(data_info, "[RECORD_COUNT]", buff);
        if (false == WriteAll((mstrDataFolder + "data.info").c_str(), data_info)) {
            mstrError = "Cannot write to data.info.";
            return false;
        }
    }
    {
        string table_xml;
        ReadAll((mstrTemplateFolder + "table.xml").c_str(), table_xml);
        if (table_xml.empty()) {
            mstrError = "File table.xml is not found in CSV template.";
            return false;
        }
        StringReplace(table_xml, "[SCHEMA]", mstrSchema);
        StringReplace(table_xml, "[TABLE]", mstrTableName);
        if (false == WriteAll((mstrDataFolder + "table.xml").c_str(), table_xml)) {
            mstrError = "Cannot write to table.xml.";
            return false;
        }
    }
    return true;
}

bool CsvExporter::CreateFolders()
{
    if (!CreateDirNested(mstrDataFolder.c_str())) {
        mstrError = "Cannot create sub-folder for CSV data files.";
        return false;
    }
    return true;
}
