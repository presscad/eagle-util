#if defined(_WIN32) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS // Disable security warning message on MSVC
#endif

#include <string>
#include <fstream>
#include "hdb_utils.h"


#ifdef _WIN32
#define snprintf _snprintf
#endif

USING_HDB_NAMESPACE
using namespace std;


HDB_BEGIN_NAMESPACE

////////////////////////////////////////////////////////////////////////////////////////////////////
// class ColRecords

bool ColRecords::AddCol(const char *col_name, const DATA_ATTR_T &attr)
{
    return AddCol(col_name, false, attr);
}

bool ColRecords::AddCol(const char *col_name, bool col_name_case_sensitive, const DATA_ATTR_T &attr)
{
    assert(col_name != nullptr && col_name[0] != '\0');
    BaseColumn_SharedPtr pCol;

    switch(attr.type) {
    case T_TINYINT:
        pCol = std::make_shared<TinyIntCol>(col_name, attr.null_able);
        break;
    case T_SMALLINT:
        pCol = std::make_shared<SmallIntCol>(col_name, attr.null_able);
        break;
    case T_INTEGER:
        pCol = std::make_shared<IntCol>(col_name, attr.null_able);
        break;
    case T_BIGINT:
        pCol = std::make_shared<BigIntCol>(col_name, attr.null_able);
        break;
    case T_REAL:
        pCol = std::make_shared<RealCol>(col_name, attr.null_able);
        break;
    case T_DOUBLE:
        pCol = std::make_shared<DoubleCol>(col_name, attr.null_able);
        break;
    case T_FLOAT:
        pCol = std::make_shared<FloatCol>(col_name, attr.null_able);
        break;
    case T_DATE:
        pCol = std::make_shared<DateCol>(col_name, attr.null_able);
        break;
    case T_TIME:
        pCol = std::make_shared<TimeCol>(col_name, attr.null_able);
        break;
    case T_TIMESTAMP:
        pCol = std::make_shared<TimeStampCol>(col_name, attr.null_able);
        break;
    case T_SECONDDATE:
        pCol = std::make_shared<SecondDateCol>(col_name, attr.null_able);
        break;
    case T_CHAR:
    {
        auto pCharCol = std::make_shared<CharCol>(col_name, attr.a, attr.null_able);
        if (attr.a == 0) {
            pCharCol->mStrVecInUse = true;
        }
        pCol = pCharCol;
        break;
    }
    case T_NCHAR:
    {
        auto pNCharCol = std::make_shared<NCharCol>(col_name, attr.a, attr.null_able);
        if (attr.a == 0) {
            pNCharCol->mStrVecInUse = true;
        }
        pCol = pNCharCol;
        break;
    }
    case T_VARCHAR:
    {
        auto pVarCharCol = std::make_shared<VarCharCol>(col_name, attr.a, attr.null_able);
        if (attr.a == 0) {
            pVarCharCol->mStrVecInUse = true;
        }
        pCol = pVarCharCol;
        break;
    }
    case T_NVARCHAR:
    {
        auto pNVarCharCol = std::make_shared<NVarCharCol>(col_name, attr.a, attr.null_able);
        if (attr.a == 0) {
            pNVarCharCol->mStrVecInUse = true;
        }
        pCol = pNVarCharCol;
        break;
    }
    case T_ALPHANUM:
    {
        auto pAlphaNumCol = std::make_shared<AlphaNumCol>(col_name, attr.a, attr.null_able);
        if (attr.a == 0) {
            pAlphaNumCol->mStrVecInUse = true;
        }
        pCol = pAlphaNumCol;
        break;
    }
    case T_SMALLDECIMAL:
        pCol = std::make_shared<SmallDecimalCol>(col_name, attr.null_able);
        break;
    case T_DECIMAL:
        pCol = std::make_shared<DecimalCol>(col_name, attr.null_able);
        break;
    case T_DECIMAL_PS:
        pCol = std::make_shared<DecimalPsCol>(col_name, attr);
        break;
    case T_BINARY:
        UnImplemented("T_BINARY");
        break;
    case T_VARBINARY:
        UnImplemented("T_VARBINARY");
        break;
    case T_BLOB:
        pCol = std::make_shared<BlobCol>(col_name, attr.null_able);
        break;
    case T_CLOB:
        pCol = std::make_shared<ClobCol>(col_name, attr.null_able);
        break;
    case T_NCLOB:
        pCol = std::make_shared<NClobCol>(col_name, attr.null_able);
        break;
    case T_TEXT:
        // In MySQL, TEXT is CLOB. In HANA TEXT is different from CLOB.
        // No obvious difference from ODBC client side, so still map it to ClobCol
        pCol = std::make_shared<ClobCol>(col_name, attr.null_able);
        break;
    case T_ST_GEOMETRY:
        pCol = std::make_shared<StGeometryCol>(col_name, attr.null_able);
        break;
    default:
        UnImplemented("Unknown column type");
        return false;
    };

    if (col_name_case_sensitive != pCol->IsColNameCaseSensitive()) {
        pCol->SetColName(pCol->GetColName(), col_name_case_sensitive);
    }
    mPtrCols.push_back(pCol);
    return true;
}

SQLRETURN ColRecords::BindAllInColumns(SQLHSTMT hstmt) const
{
    SQLRETURN rc = SQL_SUCCESS;
    for (size_t i = 0; i < mPtrCols.size(); i++) {
        rc = mPtrCols[i]->BindInParam(hstmt, (SQLUSMALLINT)(i+1));
        if (!SQL_SUCCEEDED(rc)) {
            return rc;
        }
    }
    return rc;
}

SQLRETURN ColRecords::BindAllOutColumns(SQLHSTMT hstmt) const
{
    SQLRETURN rc = SQL_SUCCESS;
    for (size_t i = 0; i < mPtrCols.size(); i++) {
        rc = mPtrCols[i]->BindOutCol(hstmt, (SQLUSMALLINT)(i + 1));
        if (!SQL_SUCCEEDED(rc)) {
            return rc;
        }
    }
    return rc;
}

// Add one line of CSV
bool ColRecords::AddRow(const std::string &line, char delimiter)
{
    const size_t count = this->GetColCount();
    mTmpStrs.reserve(count);
    util::ParseCsvLine(mTmpStrs, line, delimiter);
    if (mTmpStrs.size() < count) {
        char tmp[1024];
        snprintf(tmp, sizeof(tmp)-1, "No enough columns. Required column #%d, actual column #%d, when parsing \"%s\"",
            (int)count, (int)mTmpStrs.size(), line.c_str());
        mErrStr = tmp;
        return false;
    }

    return AddRow(mTmpStrs);
}

bool ColRecords::AddRow(char *line, char delimiter)
{
    const size_t count = this->GetColCount();
    mTmpCstrs.reserve(count);
    util::ParseCsvLineInPlace(mTmpCstrs, line, delimiter);
    if (mTmpCstrs.size() < count) {
        char tmp[1024];
        snprintf(tmp, sizeof(tmp) - 1, "No enough columns. Required column #%d, actual column #%d, when parsing \"%s\"",
            (int)count, (int)mTmpCstrs.size(), line);
        mErrStr = tmp;
        return false;
    }

    return AddRow(mTmpCstrs);
}

bool ColRecords::AddRow(const std::vector<char *> &strs)
{
    const size_t count = this->GetColCount();
    for (size_t i = 0; i < count; i++) {
        if (false == mPtrCols[i]->AddFromStr(strs[i])) {
            // generate the error string
            string tmp = "Invalid column: \"";
            tmp.reserve(1024);
            tmp += mPtrCols[i]->GetColName();
            tmp += "\" when parsing \"";

            for (size_t k = 0; k < strs.size(); ++k) {
                tmp += strs[k];
                if (k != strs.size() - 1) {
                    tmp += ',';
                }
            }
            tmp += '\"';
            mErrStr = tmp;

            // Error happens, remove previous columns for this row
            for (size_t k = 0; k < i; ++k) {
                mPtrCols[k]->RemoveRow();
            }

            return false;
        }
    }
    mRowCount++;
    return true;
}

// Add parsed one line of CSV
bool ColRecords::AddRow(const std::vector<std::string> &strs)
{
    const size_t count = this->GetColCount();
    for (size_t i = 0; i < count; i++) {
        if (false == mPtrCols[i]->AddFromStr(strs[i])) {
            // generate the error string
            string tmp = "Invalid column: \"";
            tmp.reserve(1024);
            tmp += mPtrCols[i]->GetColName();
            tmp += "\" when parsing \"";

            for (size_t k = 0; k < strs.size(); ++k) {
                tmp += strs[i];
                if (k != strs.size() - 1) {
                    tmp += ',';
                }
            }
            tmp += '\"';
            mErrStr = tmp;

            // Error happens, remove previous columns for this row
            for (size_t k = 0; k < i; ++k) {
                mPtrCols[k]->RemoveRow();
            }

            return false;
        }
    }
    ++mRowCount;
    return true;
}

bool ColRecords::AddColsFromCreateSql(const char *create_sql)
{
    PARSED_TABLE_T parsed_table;
    std::string err;
    if (!ParseTableFromSql(create_sql, parsed_table, err)) {
        if (!err.empty()) {
            mErrStr = err;
        } else {
            mErrStr = std::string("Error in parsing: ") + create_sql;
        }
        return false;
    }
    size_t col_count = parsed_table.col_names.size();
    if (col_count == 0) {
        mErrStr = std::string("Error in parsing: ") + create_sql;
        return false;
    }

    ClearAllCols();
    for (size_t i = 0; i < col_count; i++) {
        if (false == this->AddCol(parsed_table.col_names[i].c_str(),
            parsed_table.col_names_case_sensitive[i],
            parsed_table.col_attrs[i]))
        {
            return false;
        }
    }
    return true;
}

bool ColRecords::AddColsFromRecords(const ColRecords &src_records)
{
    size_t col_count = src_records.GetColCount();
    if (col_count == 0) {
        mErrStr = __FUNCTION__ + string(": source records have no column");
        return false;
    }

    ClearAllCols();
    for (size_t i = 0; i < col_count; i++) {
        const auto &src_col = *src_records.GetColumn(i);
        if (false == this->AddCol(src_col.GetColName(), src_col.IsColNameCaseSensitive(),
            src_col.GetDataAttr()))
        {
            return false;
        }
    }
    return true;
}

// this version of read_line handles mixed dos/unix '\n'
static bool GetLine(std::istream& is, std::string& line)
{
    line.clear();

    std::istream::sentry se(is, true);
    std::streambuf* sb = is.rdbuf();

    for (;;) {
        int c = sb->sbumpc();
        switch (c) {
        case '\n': {
            return true;
        }
        case '\r':
            if (sb->sgetc() == '\n') {
                sb->sbumpc();
            }
            return true;
        case EOF:
            // Also handle the case when the last line has no line ending
            if (line.empty()) {
                is.setstate(std::ios::eofbit);
            }
            return false; // because of EOF
        default:
            line += (char)c;
        }
    }
    return true;
}

// return the number of lines actually added
int ColRecords::AddRows(std::istream &is_csv, int num, char delimiter)
{
    if (!!is_csv.good()) {
        return 0;
    }

    std::string line;
    int total = 0;
    for (int i = 0; i < num; i++) {
        if (!GetLine(is_csv, line)) {
            break;
        }
#if 0
        // Special handling: replace 0x0 with 0x20 (space)
        for (int k = (int)line.size() - 1; k >=0; k--) {
            if (line[k] == '\0') {
                line[k] = 0x20;
            }
        }
#endif
        if (AddRow(line, delimiter)) {
            total++;
        } else {
            printf("%s\n", this->GetErrStr());
        }
    }
    mRowCount = total;
    return total;
}

// return the number of lines actually into CSV
// TODO: optimize to use batch write for better performance
int ColRecords::RowsToCsv(std::ostream &os_csv, int start_row, int row_num, char delimiter) const
{
    if (!os_csv.good()) {
        return 0;
    }
    const int row_count = (int)this->GetRowCount();
    int end_row = start_row + row_num;
    if (end_row > row_count) {
        end_row = row_count;
    }

    const size_t BUFF_SIZE = 10 * 1024;
    std::string lines, str;
    lines.reserve(BUFF_SIZE + 512);
    int total = 0;

    for (int i = start_row; i < end_row; ++i) {
        int i_col = 0;
        for (auto &p_col : mPtrCols) {
            if (i_col != 0) {
                lines += delimiter;
            }
            p_col->GetAsStr(i, str);
            lines += str;
            ++i_col;
        }
        lines += '\n';

        if (lines.size() > BUFF_SIZE) {
            os_csv << lines;
            lines.clear();
        }
        ++total;
    }

    if (!lines.empty()) {
        os_csv << lines;
    }
    return total;
}

// pre-condition: column structurs of *this and records must be identical
// return the number of lines actually added
int ColRecords::AddRows(const ColRecords &records)
{
    if (this->GetColCount() > records.GetColCount()) {
        return 0;
    }
    for (size_t i = 0; i < GetColCount(); i++) {
        this->GetColumn(i)->Append(records.GetColumn(i));
    }
    mRowCount += records.GetRowCount();
    return (int)records.GetRowCount();
}

void ColRecords::Swap(ColRecords &records)
{
    std::swap(mRowCount, records.mRowCount);
    std::swap(mPtrCols, records.mPtrCols);
    std::swap(mErrStr, records.mErrStr);
}

void ColRecords::GenerateFakeData(size_t row_count)
{
    size_t col_count = this->GetColCount();
    for (size_t col = 0; col < col_count; col++) {
        mPtrCols[col]->GenerateFakeData(row_count);
    }
    mRowCount = row_count;
}

HDB_END_NAMESPACE
