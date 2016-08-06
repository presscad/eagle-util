#ifndef _HDB_COLUMNS_H
#define _HDB_COLUMNS_H

#include <stdlib.h>
#include <memory.h>
#include <vector>
#include <string>
#include <assert.h>
#ifdef _WIN32
#include <windows.h> // required by sqlext.h for WIN32
#else
#include <unistd.h>
#endif
#include <sqlext.h>
#include <time.h>
#include <wchar.h>
#include "hdb_types.h"
#include "hdb_odbc.h"

namespace hdb {

typedef std::basic_string<unsigned short> string16;

////////////////////////////////////////////////////////////////////////////////////////////////////
class BaseColumn {
public:
    BaseColumn(const char *col_name, const DATA_ATTR_T &attr) {
        SetColName(col_name);
        mDataAttr = attr;
    };
    virtual ~BaseColumn() {};

    const DATA_ATTR_T &GetDataAttr() const {
        return mDataAttr;
    };
    const bool NullAble() const {
        return mDataAttr.null_able;
    };
    virtual void Reserve(size_t count) = 0;
    virtual size_t GetCount() const = 0;
    const char *GetColName() const {
        return mColName.c_str();
    };
    void SetColName(const char *name) {
        mColName = (name != NULL) ? name : "";
    };
    void CopyFrom(const BaseColumn &col) {
        mDataAttr = col.mDataAttr;
        mColName = col.mColName;
    };
    virtual void *GetData() = 0;
    virtual const void *GetData() const = 0;
    virtual void *GetStrLenOrIndVec() = 0;
    virtual const void *GetStrLenOrIndVec() const = 0;
    virtual void GenerateFakeData(size_t count) = 0;
    virtual SQLRETURN BindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar) const = 0;
    virtual bool AddFromStr(const std::string &str) = 0;
    virtual void RemoveRow() = 0;
    virtual void RemoveAllRows() = 0;
    virtual bool Append(const BaseColumn *pCol) = 0;

protected:
    DATA_ATTR_T mDataAttr;
    std::string mColName;
};

// Forward template class declarations
template<class T, DATA_TYPE_T data_type> class ColT;
template<class T, DATA_TYPE_T data_type> class CharColT;

typedef ColT<char, T_TINYINT> TinyIntCol;
typedef ColT<short, T_SMALLINT> SmallIntCol;
typedef ColT<int, T_INTEGER> IntCol;
typedef ColT<SQLBIGINT, T_BIGINT> BigIntCol;
typedef ColT<float, T_REAL> RealCol;
typedef ColT<double, T_DOUBLE> DoubleCol;
typedef ColT<double, T_FLOAT> FloatCol;
typedef ColT<SQL_DATE_STRUCT, T_DATE> DateCol;
typedef ColT<SQL_TIME_STRUCT, T_TIME> TimeCol;
typedef ColT<SQL_TIMESTAMP_STRUCT, T_TIMESTAMP> TimeStampCol;
typedef ColT<SQL_TIMESTAMP_STRUCT, T_SECONDDATE> SecondDateCol;
typedef CharColT<SQLWCHAR, T_CHAR> CharCol; // To support unicode, CHAR and VARCHAR are mapped to SQLWCHAR instead of SQLCHAR
typedef CharColT<SQLWCHAR, T_NCHAR > NCharCol;
typedef CharColT<SQLWCHAR, T_VARCHAR> VarCharCol;
typedef CharColT<SQLWCHAR, T_NVARCHAR> NVarCharCol;
typedef CharColT<SQLCHAR, T_ALPHANUM> AlphaNumCol;
typedef ColT<double, T_DECIMAL> DecimalCol;
typedef ColT<double, T_SMALLDECIMAL> SmallDecimalCol;
typedef ColT<double, T_DECIMAL_PS> DecimalPsCol; // NOTE: map double to decimal may not be precise!
// T_BINARY ?
// T_VARBINARY ?
// T_BLOB ?
// T_TEXT ?

// Dummy functions for CharColT derived classes, do not use them!
static inline SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<SQLWCHAR, T_CHAR> &col) {return 0;};
static inline SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<SQLWCHAR, T_NCHAR> &col) {return 0;};
static inline SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<SQLWCHAR, T_VARCHAR> &col) {return 0;};
static inline SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<SQLWCHAR, T_NVARCHAR> &col) {return 0;};
static inline SQLRETURN SqlBindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, const ColT<SQLCHAR, T_ALPHANUM> &col) {return 0;};
static inline bool StrToValue(const std::string &s, SQLCHAR &v) {return 0;};
static inline bool StrToValue(const std::string &s, SQLWCHAR &v) {return 0;};


template<class T, DATA_TYPE_T data_type>
class ColT : public BaseColumn
{
public:
    ColT(const char *col_name = NULL, bool null_able = true)
        : BaseColumn(col_name, GenDataAttr(data_type, null_able, 0, 0))
    {};
    ColT(const char *col_name, DATA_ATTR_T attr) 
        : BaseColumn(col_name, attr)
    {
        mDataAttr.type = data_type;
    };
    virtual ~ColT() {};

    virtual void Reserve(size_t count) {
        mDataVec.reserve(count);
        mStrLenOrIndVec.reserve(count);
    };
    virtual size_t GetCount() const {
        return mStrLenOrIndVec.size();
    };
    virtual void *GetData() {
        return mDataVec.data();
    };
    virtual const void *GetData() const {
        return mDataVec.data();
    };
    virtual void *GetStrLenOrIndVec() {
        return mStrLenOrIndVec.data();
    };
    virtual const void *GetStrLenOrIndVec() const {
        return mStrLenOrIndVec.data();
    };
    virtual void GenerateFakeData(size_t count) {
        RemoveAllRows();
        Reserve(count);
        // Based on OO design, it is not recommended to use switch-case here. The recommended way is
        // to implement all the virtual function in sub-classes, which is too tedious and will generate
        // too much code.
        switch(mDataAttr.type) {
        case T_TINYINT:
            {
                TinyIntCol *pcol = (TinyIntCol *)this;
                for (size_t i = 0; i < count; i++) {
                    pcol->PushBack((char)(rand() % 128));
                }
            }
            break;
        case T_SMALLINT:
            {
                SmallIntCol *pcol = (SmallIntCol *)this;
                for (size_t i = 0; i < count; i++) {
                    pcol->PushBack((short)(rand() % 32768));
                }
            }
            break;
        case T_INTEGER:
            {
                IntCol *pcol = (IntCol *)this;
                for (size_t i = 0; i < count; i++) {
                    pcol->PushBack(rand());
                }
            }
            break;
        case T_BIGINT:
            {
                BigIntCol *pcol = (BigIntCol *)this;
                for (size_t i = 0; i < count; i++) {
			        long long t = (long long) ((double) rand() / RAND_MAX * 99999999);
                    pcol->PushBack(t);
                }
            }
            break;
        case T_REAL:
            {
                RealCol *pcol = (RealCol *)this;
                for (size_t i = 0; i < count; i++) {
			        float t = ((float) rand() / RAND_MAX * 99999);
                    pcol->PushBack(t);
                }
            }
            break;
        case T_DOUBLE:
        case T_FLOAT:
            {
                DoubleCol *pcol = (DoubleCol *)this;
                for (size_t i = 0; i < count; i++) {
			        double t = ((double) rand() / RAND_MAX * 9999999);
                    pcol->PushBack(t);
                }
            }
            break;
        case T_DATE:
            {
                DateCol *pcol = (DateCol *)this;
                for (size_t i = 0; i < count; i++) {
                    SQL_DATE_STRUCT date;
                    GetCurDate(date);
                    pcol->PushBack(date);
                }
            }
            break;
        case T_TIME:
            {
                TimeCol *pcol = (TimeCol *)this;
                for (size_t i = 0; i < count; i++) {
                    SQL_TIME_STRUCT time;
                    GetCurTime(time);
                    pcol->PushBack(time);
                }
            }
            break;
        case T_TIMESTAMP:
            {
                TimeStampCol *pcol = (TimeStampCol *)this;
                for (size_t i = 0; i < count; i++) {
                    SQL_TIMESTAMP_STRUCT st;
                    GetCurTimestamp(st);
                    pcol->PushBack(st);
                }
            }
            break;
        case T_SECONDDATE:
            assert(false);
            break;
        case T_CHAR:
        case T_NCHAR:
        case T_VARCHAR:
        case T_NVARCHAR:
        case T_ALPHANUM:
            assert(false); // should not reach here!
            break;
        case T_SMALLDECIMAL:
        case T_DECIMAL:
            {
                DoubleCol *pcol = (DoubleCol *)this;
                for (size_t i = 0; i < count; i++) {
			        double t = ((double) rand() / RAND_MAX * 999999999);
                    pcol->PushBack(t);
                }
            }
            break;
        case T_DECIMAL_PS:
            {
                UnImplemented("GenerateFakeData - T_DECIMAL_PS");
#if 0
                DoubleCol *pcol = (DoubleCol *)this;
                for (size_t i = 0; i < count; i++) {
			        double t = ((double) rand() / RAND_MAX * 9999999);
                    pcol->PushBack(t);
                }
#endif
            }
            break;
        case T_BINARY:
        case T_VARBINARY:
        case T_BLOB:
        case T_TEXT:
            // TODO: to implement later
        default:
            assert(false);
            return;
        };
    };
    virtual SQLRETURN BindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar) const {
        return SqlBindInParam(hstmt, ipar, *this);
    };
    virtual bool AddFromStr(const std::string &str) {
        T value;
        bool is_null = str.empty();
        mStrLenOrIndVec.push_back(is_null ? SQL_NULL_DATA : SQL_NTS);
        if (is_null) {
            memset(&value, 0, sizeof(T));
        } else {
            if (false == StrToValue(str, value)) {
                return false;
            }
        }
        mDataVec.push_back(value);
        return true;
    };
    virtual void RemoveRow() {
        mDataVec.pop_back();
        if (mStrLenOrIndVec.size() > 0) {
            mStrLenOrIndVec.pop_back();
        }
    }
    virtual void RemoveAllRows() {
        mDataVec.clear();
        mStrLenOrIndVec.clear();
    };
    virtual bool Append(const BaseColumn *pCol) {
        const ColT<T, data_type> &col = *(const ColT<T, data_type> *)pCol;
        assert(mDataAttr.type == col.mDataAttr.type);
        mDataVec.insert(mDataVec.end(), col.mDataVec.begin(), col.mDataVec.end());
        mStrLenOrIndVec.insert(mStrLenOrIndVec.end(), col.mStrLenOrIndVec.begin(), col.mStrLenOrIndVec.end());
        return true;
    };

public:
    void CopyFrom(ColT<T, data_type> &col) {
        BaseColumn::CopyFrom(col);
        mDataVec = col.mDataVec;
        mStrLenOrIndVec = col.mStrLenOrIndVec;
    };
    void PushBack(const T &val) {
        assert(mDataAttr.type != T_CHAR && mDataAttr.type != T_NCHAR && mDataAttr.type != T_VARCHAR && mDataAttr.type != T_NVARCHAR);
        mDataVec.push_back(val);
        mStrLenOrIndVec.push_back(SQL_NTS);
    };

protected:
    std::vector<T> mDataVec;
    std::vector<SQLLEN> mStrLenOrIndVec;
};

template<class T, DATA_TYPE_T data_type>
class CharColT : public ColT<T, data_type>
{
public:
    CharColT(const char *col_name, int n, bool null_able = true)
        : ColT<T, data_type>(col_name, GenDataAttr(data_type, null_able, n, 0))
    {
    };
    virtual ~CharColT() {};
    virtual void Reserve(size_t count) {
    	ColT<T, data_type>::mDataVec.reserve(count * (ColT<T, data_type>::mDataAttr.a + 1));
    	ColT<T, data_type>::mStrLenOrIndVec.reserve(count);
    };
    virtual SQLRETURN BindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar) const {
        return SqlBindInParam(hstmt, ipar, *this);
    };
    virtual bool AddFromStr(const std::string &str) {
    	ColT<T, data_type>::mStrLenOrIndVec.push_back((NullAble() && str.empty()) ? SQL_NULL_DATA : SQL_NTS);
        size_t len = ColT<T, data_type>::mDataVec.size();
        ColT<T, data_type>::mDataVec.resize(len + ColT<T, data_type>::mDataAttr.a + 1);
        if (sizeof(T) == 2) {
            string16 wstr = StrToWStr(str);
#ifdef _WIN32
            wcsncpy_s((SQLWCHAR *)mDataVec.data() + len, mDataAttr.a + 1, (SQLWCHAR *)wstr.c_str(), mDataAttr.a);
#else
            int copy_len = (wstr.length() + 1) < ((size_t)ColT<T, data_type>::mDataAttr.a + 1) ?
            		wstr.length() + 1 : ColT<T, data_type>::mDataAttr.a + 1;
            memcpy((SQLWCHAR *)ColT<T, data_type>::mDataVec.data() + len, (SQLWCHAR *)wstr.c_str(), copy_len * 2);
#endif
        } else {
#ifdef _WIN32
            strncpy_s((char *)mDataVec.data() + len, mDataAttr.a + 1, str.c_str(), mDataAttr.a);
#else
            strncpy((char *)ColT<T, data_type>::mDataVec.data() + len,
            		str.c_str(), ColT<T, data_type>::mDataAttr.a);
#endif
        }
        return true;
    };
    virtual void RemoveRow() {
      	ColT<T, data_type>::mStrLenOrIndVec.pop_back();
        size_t len = ColT<T, data_type>::mDataVec.size();
        if (len > 0) {
            ColT<T, data_type>::mDataVec.resize(len - (ColT<T, data_type>::mDataAttr.a + 1));
        }
    }
    virtual void GenerateFakeData(size_t count) {
        RemoveAllRows();
        Reserve(count);
        if (sizeof(T) == 1) {
            std::string buff;
            buff.resize(ColT<T, data_type>::mDataAttr.a + 1);
            for (size_t i = 0; i < count; i++) {
                long t = (long) ((double) rand() / RAND_MAX * 999999999);
#ifdef _WIN32
                _snprintf_s((char*)buff.data(), buff.size(), mDataAttr.a, "A%09ld", t);
#else
                snprintf((char*)buff.data(), ColT<T, data_type>::mDataAttr.a, "A%09ld", t);
#endif
                AddFromStr((const char *)buff.c_str());
            }
        } else if (sizeof(T) == 2) {
            std::wstring buff;
            buff.resize(ColT<T, data_type>::mDataAttr.a + 1);
            for (size_t i = 0; i < count; i++) {
#ifdef _WIN32
                long t = (long) ((double) rand() / RAND_MAX * 999999999);
                _snwprintf_s((SQLWCHAR*)buff.data(), buff.size(), mDataAttr.a, L"N%09ld", t);
#else
                UnImplemented(NULL);
#endif
                PushBack((T*)buff.c_str());
            }
        } else {
            assert(false); // should not reach here!
        };
    };

public:
    void PushBack(const T *str) {
        if (T_NCHAR == ColT<T, data_type>::mDataAttr.type || T_NVARCHAR == ColT<T, data_type>::mDataAttr.type) {
        	ColT<T, data_type>::mStrLenOrIndVec.push_back((NullAble() && *str == '\0') ? SQL_NULL_DATA : SQL_NTS);
            size_t len = ColT<T, data_type>::mDataVec.size();
            ColT<T, data_type>::mDataVec.resize(len +  ColT<T, data_type>::mDataAttr.a + 1);
#ifdef _WIN32
            wcsncpy_s((SQLWCHAR *)mDataVec.data() + len, mDataAttr.a + 1, (SQLWCHAR *)str, mDataAttr.a);
#else
            UnImplemented(NULL);
#endif
        } else {
            AddFromStr((const char *)str);
        }
    };
};

////////////////////////////////////////////////////////////////////////////////////////////////////

class ColRecords {
public:
    ColRecords() : mRowCount(0) {
    };
    virtual ~ColRecords() {
        ClearAllCols();
    };

public:
    void ClearAllCols() {
        mRowCount = 0;
        for (size_t i = 0; i < mPtrCols.size(); i++) {
            delete mPtrCols[i];
        }
        mPtrCols.clear();
        mErrStr.clear();
    };
    void ClearAllRows() {
        mRowCount = 0;
        for (size_t i = 0; i < mPtrCols.size(); i++) {
            mPtrCols[i]->RemoveAllRows();
        }
        mErrStr.clear();
    };
    void Reserve(size_t count) {
        for (size_t i = 0; i < mPtrCols.size(); i++) {
            mPtrCols[i]->Reserve(count);
        }
    };
    const char *GetErrStr() const {
        return mErrStr.c_str();
    };
    size_t GetRowCount() const {
        return mRowCount;
    };
    size_t GetColCount() const {
        return mPtrCols.size();
    };
    BaseColumn *GetColumn(size_t index) {
        return mPtrCols[index];
    };
    const BaseColumn *GetColumn(size_t index) const {
        return mPtrCols[index];
    };
    std::vector<BaseColumn *> &GetColumns() {
        return mPtrCols;
    };
    bool AddCol(const char *col_name, DATA_TYPE_T type, bool null_able = true) {
        return AddCol(col_name, GenDataAttr(type, null_able, 0, 0));
    };
    bool AddCol(const char *col_name, const DATA_ATTR_T &attr);
    bool AddColFixedChar(const char *col_name, DATA_TYPE_T type, unsigned char num, bool null_able = false) {
        assert(type == T_CHAR || type == T_NCHAR);
        return AddCol(col_name, GenDataAttr(type, null_able, num, 0));
    };
    bool AddColDecimalPs(const char *col_name, unsigned char p, unsigned char s, bool null_able = false) {
        return AddCol(col_name, GenDataAttr(T_DECIMAL_PS, null_able, p, s));
    };
    bool AddColsFromCreateSql(const char *create_sql);

    SQLRETURN BindAllInColumns(SQLHSTMT hstmt) const;
    bool AddRow(const std::string &line, char delimiter = ','); // one line of CSV
    int AddRows(std::ifstream &is_csv, int num, char delimiter = ',');
    int AddRows(const ColRecords &records);
    void GenerateFakeData(size_t row_count);

protected:
    size_t mRowCount;
    std::vector<BaseColumn *> mPtrCols;
    std::string mErrStr;
};


}// namespace hdb

#endif // _HDB_COLUMNS_H
