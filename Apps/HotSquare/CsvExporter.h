#ifndef CSV_EXPORTER_H_
#define CSV_EXPORTER_H_

#include "HotSquare.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// class CsvExporter

class CsvExporter {
public:
    CsvExporter(const char *sOutFolder, const char *sSchema, const char *sTableName,
        const char *sTemplateFolder, int nRecCount);
    ~CsvExporter();
    // Generate various export files excluding the "data.csv"
    bool GenerateExportFiles();
    // Return the pathname of "data.csv"
    std::string GetDataFilePath();
    std::string GetErrorStr() {return mstrError;};

private:
    bool CreateFolders();

private:
    std::string mstrBaseFolder;
    std::string mstrTemplateFolder;
    std::string mstrDataFolder;

    std::string mstrSchema;
    std::string mstrTableName;
    int mnRecordCount;
    std::string mstrError;
};

#endif // CSV_EXPORTER_H_
