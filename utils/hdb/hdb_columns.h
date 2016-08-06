#ifndef _HDB_COLUMNS_H
#define _HDB_COLUMNS_H

#include <vector>
#include <string>
#include <memory>

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <Windows.h> // required by sqlext.h for WIN32
#else
#  include <unistd.h>
#endif

#include <sqlext.h>
#include <ctime>
#include <cwchar>
#include <cassert>
#include <cstring>
#include "hdb_types.h"
#include "hdb_odbc.h"


HDB_BEGIN_NAMESPACE

typedef std::basic_string<unsigned short> string16;

// forward declarations of used functions
void GetCurTimestamp(SQL_TIMESTAMP_STRUCT &st);
void GetCurDate(SQL_DATE_STRUCT &date);
void GetCurTime(SQL_TIME_STRUCT &time);
hdb::string16 StrToWStr(const std::string &str);
std::string WStrToStr(const string16 &wstr);

////////////////////////////////////////////////////////////////////////////////////////////////////

class BaseColumn;
typedef std::shared_ptr<BaseColumn> BaseColumn_SharedPtr;

class BaseColumn
{
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
    virtual size_t Capacity() const = 0;
    virtual size_t GetCount() const = 0;
    virtual void SetCount(size_t count) = 0;
    const char *GetColName() const
    {
        return mColName.c_str();
    };
    void SetColName(const char *name)
    {
        mColName = (name != NULL) ? name : "";
    };
    virtual void CopyFrom(const BaseColumn &col)
    {
        mDataAttr = col.mDataAttr;
        mColName = col.mColName;
    };
    virtual void *GetData() = 0;
    virtual const void *GetData() const = 0;
    virtual void *GetStrLenOrIndVec() = 0;
    virtual const void *GetStrLenOrIndVec() const = 0;

    virtual void *GetData(size_t i) = 0;
    virtual const void *GetData(size_t i) const = 0;
    virtual SQLLEN GetDataSize(size_t i) const = 0;

    virtual void GenerateFakeData(size_t count) = 0;
    virtual SQLRETURN BindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar) const = 0;
    virtual SQLRETURN BindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar) = 0;
    virtual bool AddFromStr(const std::string &str) = 0;
    virtual bool AddFromStr(const char *str) = 0;
    virtual bool SetFromStr(size_t i, const std::string &str) = 0;
    virtual bool SetFromStr(size_t i, const string16 &wstr) = 0;
    virtual bool SetFromStr(size_t i, const char *str) = 0;
    virtual std::string GetAsStr(size_t i) const = 0;
    virtual string16 GetAsWStr(size_t i) const = 0;// for char based derived classes
    virtual void SetFromData(size_t i, const void *data) = 0;
    virtual void RemoveRow() = 0;
    virtual void RemoveAllRows() = 0;
    virtual bool Append(const BaseColumn_SharedPtr& pCol) = 0;
    virtual int  Compare(size_t i, size_t j) const = 0;
    virtual bool LessThan(size_t i, size_t j) const = 0;
    virtual bool GreaterThan(size_t i, size_t j) const = 0;

protected:
    DATA_ATTR_T mDataAttr;
    std::string mColName;
};


// Forward template class declarations
template<class T, DATA_TYPE_T data_type> class ColT;
template<class T, DATA_TYPE_T data_type> class CharColT;
template<class T, DATA_TYPE_T data_type> class LongVarColT;

typedef ColT<unsigned char, T_TINYINT> TinyIntCol;
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
#ifdef HDB_INTERNAL_NO_WCHAR
typedef CharColT<SQLCHAR, T_CHAR> CharCol;
typedef CharColT<SQLCHAR, T_VARCHAR> VarCharCol;
#else
// To support unicode, CHAR and VARCHAR are mapped to SQLWCHAR instead of SQLCHAR
typedef CharColT<SQLWCHAR, T_CHAR> CharCol;
typedef CharColT<SQLWCHAR, T_VARCHAR> VarCharCol;
#endif
typedef CharColT<SQLWCHAR, T_NCHAR > NCharCol;
typedef CharColT<SQLWCHAR, T_NVARCHAR> NVarCharCol;
typedef CharColT<SQLCHAR, T_ALPHANUM> AlphaNumCol;
typedef CharColT<SQLCHAR, T_DECIMAL_PS> DecimalPsCol; // NOTE: map chars to to decimal instead of cumbersome SQL_NUMERIC_STRUCT

typedef ColT<double, T_DECIMAL> DecimalCol;
typedef ColT<double, T_SMALLDECIMAL> SmallDecimalCol;

// T_BINARY ?
// T_VARBINARY ?

typedef LongVarColT<unsigned char, T_BLOB> BlobCol;
typedef LongVarColT<unsigned char, T_CLOB> ClobCol;
typedef LongVarColT<SQLWCHAR, T_NCLOB> NClobCol;
typedef LongVarColT<SQLCHAR, T_TEXT> TextCol;

// HANA GIS related
typedef LongVarColT<unsigned char, T_ST_GEOMETRY> StGeometryCol;


static inline bool operator<(const DATE_STRUCT& d1, const DATE_STRUCT& d2)
{
    if (d1.year != d2.year) {
        return d1.year < d2.year;
    }
    if (d1.month != d2.month) {
        return d1.month < d2.month;
    }
    return d1.day < d2.day;
}
static inline bool operator>(const DATE_STRUCT& d1, const DATE_STRUCT& d2)
{
    if (d1.year != d2.year) {
        return d1.year > d2.year;
    }
    if (d1.month != d2.month) {
        return d1.month > d2.month;
    }
    return d1.day > d2.day;
}

static inline bool operator<(const TIME_STRUCT& t1, const TIME_STRUCT& t2)
{
    if (t1.hour != t2.hour) {
        return t1.hour < t2.hour;
    }
    if (t1.minute != t2.minute) {
        return t1.minute < t2.minute;
    }
    return t1.second < t2.second;
}
static inline bool operator>(const TIME_STRUCT& t1, const TIME_STRUCT& t2)
{
    if (t1.hour != t2.hour) {
        return t1.hour > t2.hour;
    }
    if (t1.minute != t2.minute) {
        return t1.minute > t2.minute;
    }
    return t1.second > t2.second;
}

static inline bool operator<(const TIMESTAMP_STRUCT& t1, const TIMESTAMP_STRUCT& t2)
{
    if (t1.year != t2.year) {
        return t1.year < t2.year;
    }
    if (t1.month != t2.month) {
        return t1.month < t2.month;
    }
    if (t1.day != t2.day) {
        return t1.day < t2.day;
    }
    if (t1.hour != t2.hour) {
        return t1.hour < t2.hour;
    }
    if (t1.minute != t2.minute) {
        return t1.minute < t2.minute;
    }
    if (t1.second != t2.second) {
        return t1.second < t2.second;
    }
    return t1.fraction < t2.fraction;
}
static inline bool operator>(const TIMESTAMP_STRUCT& t1, const TIMESTAMP_STRUCT& t2)
{
    if (t1.year != t2.year) {
        return t1.year > t2.year;
    }
    if (t1.month != t2.month) {
        return t1.month > t2.month;
    }
    if (t1.day != t2.day) {
        return t1.day > t2.day;
    }
    if (t1.hour != t2.hour) {
        return t1.hour > t2.hour;
    }
    if (t1.minute != t2.minute) {
        return t1.minute > t2.minute;
    }
    if (t1.second != t2.second) {
        return t1.second > t2.second;
    }
    return t1.fraction > t2.fraction;
}



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

    virtual void Reserve(size_t count)
    {
        mDataVec.reserve(count);
        mStrLenOrIndVec.reserve(count);
    };
    virtual size_t Capacity() const
    {
        return mStrLenOrIndVec.capacity();
    }
    virtual size_t GetCount() const
    {
        return mStrLenOrIndVec.size();
    };
    virtual void SetCount(size_t count)
    {
        mDataVec.resize(count);
        size_t old_count = mStrLenOrIndVec.size();
        mStrLenOrIndVec.resize(count);
        if (count > old_count) {
            if (this->NullAble()) {
                for (size_t i = old_count; i < count; ++i) {
                    mStrLenOrIndVec[i] = SQL_NULL_DATA;
                }
            }
            else {
                for (size_t i = old_count; i < count; ++i) {
                    mStrLenOrIndVec[i] = SQL_NTS;
                }
            }
        }
    };
    virtual void *GetData()
    {
        return mDataVec.data();
    };
    virtual const void *GetData() const
    {
        return mDataVec.data();
    };
    virtual void *GetStrLenOrIndVec()
    {
        return mStrLenOrIndVec.data();
    };
    virtual const void *GetStrLenOrIndVec() const
    {
        return mStrLenOrIndVec.data();
    };

    virtual void *GetData(size_t i)
    {
        return &mDataVec[i];
    }
    virtual const void *GetData(size_t i) const
    {
        return &mDataVec[i];
    }
    virtual SQLLEN GetDataSize(size_t i) const
    {
        return sizeof(T);
    }

    virtual void GenerateFakeData(size_t count) {
        RemoveAllRows();
        Reserve(count);
        // Based on OO design, it is not recommended to use switch-case here. The recommended way is
        // to implement all the virtual function in sub-classes, which is too tedious and will generate
        // too much code.
        switch (mDataAttr.type) {
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
                long long t = (long long)((double)rand() / RAND_MAX * 99999999);
                pcol->PushBack(t);
            }
        }
        break;
        case T_REAL:
        {
            RealCol *pcol = (RealCol *)this;
            for (size_t i = 0; i < count; i++) {
                float t = ((float)rand() / RAND_MAX * 99999);
                pcol->PushBack(t);
            }
        }
        break;
        case T_DOUBLE:
        case T_FLOAT:
        {
            DoubleCol *pcol = (DoubleCol *)this;
            for (size_t i = 0; i < count; i++) {
                double t = ((double)rand() / RAND_MAX * 9999999);
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
                double t = ((double)rand() / RAND_MAX * 999999999);
                pcol->PushBack(t);
            }
        }
        break;
        case T_DECIMAL_PS:
        {
            UnImplemented("GenerateFakeData - T_DECIMAL_PS");
        }
        break;
        case T_BINARY:
        case T_VARBINARY:
        case T_BLOB:
        case T_TEXT:
            assert(false); // to implement later if necessary
            break;
        default:
            assert(false);
            return;
        };
    };

    virtual SQLRETURN BindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar) const
    {
        return SqlBindInParam(hstmt, ipar, *this);
    };
    virtual SQLRETURN BindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar)
    {
        return SQLBindOutCol(hstmt, ipar, *this);
    };

    virtual bool AddFromStr(const std::string &str)
    {
        T value;
        bool is_null = str.empty();
        mStrLenOrIndVec.push_back(is_null ? SQL_NULL_DATA : SQL_NTS);
        if (is_null) {
            memset(&value, 0, sizeof(T));
        }
        else {
            if (false == StrToValue(str, value)) {
                return false;
            }
        }
        mDataVec.push_back(value);
        return true;
    };
    virtual bool AddFromStr(const char *str)
    {
        T value;
        bool is_null = (str == nullptr) || (str[0] == '\0');
        mStrLenOrIndVec.push_back(is_null ? SQL_NULL_DATA : SQL_NTS);
        if (is_null) {
            memset(&value, 0, sizeof(T));
        }
        else {
            if (false == StrToValue(str, value)) {
                return false;
            }
        }
        mDataVec.push_back(value);
        return true;
    };
    virtual bool SetFromStr(size_t i, const std::string &str)
    {
        T value;
        bool is_null = str.empty();
        mStrLenOrIndVec[i] = is_null ? SQL_NULL_DATA : SQL_NTS;
        if (is_null) {
            memset(&value, 0, sizeof(T));
        }
        else {
            if (false == StrToValue(str, value)) {
                return false;
            }
        }
        mDataVec[i] = value;
        return true;
    };
    virtual bool SetFromStr(size_t i, const string16 &wstr) // for char based derived classes
    {
        UnImplemented(__FUNCTION__);
        return false;
    }
    virtual bool SetFromStr(size_t i, const char *str)
    {
        T value;
        bool is_null = (str == nullptr) || (str[0] == '\0');
        mStrLenOrIndVec[i] = is_null ? SQL_NULL_DATA : SQL_NTS;
        if (is_null) {
            memset(&value, 0, sizeof(T));
        }
        else {
            if (false == StrToValue(str, value)) {
                return false;
            }
        }
        mDataVec[i] = value;
        return true;
    };
    virtual std::string GetAsStr(size_t i) const
    {
        std::string result;
        if (mStrLenOrIndVec[i] != SQL_NULL_DATA) {
            result = ValueToStr(mDataVec[i]);
        }
        return result;
    }
    virtual string16 GetAsWStr(size_t i) const // for char based derived classes
    {
        UnImplemented(__FUNCTION__);
        return string16();
    }
    virtual void SetFromData(size_t i, const void *data)
    {
        const T *p_value = (const T *)data;
        mStrLenOrIndVec[i] = (p_value == NULL) ? SQL_NULL_DATA : SQL_NTS;
        if (p_value) {
            mDataVec[i] = *p_value;
        }
        else {
            memset(&mDataVec[i], 0, sizeof(T));
        }
    };
    virtual void RemoveRow()
    {
        mDataVec.pop_back();
        if (mStrLenOrIndVec.size() > 0) {
            mStrLenOrIndVec.pop_back();
        }
    }
    virtual void RemoveAllRows()
    {
        mDataVec.clear();
        mStrLenOrIndVec.clear();
    };
    virtual bool Append(const BaseColumn_SharedPtr& pCol)
    {
        const ColT<T, data_type> &col = *(const ColT<T, data_type> *)pCol.get();
        assert(mDataAttr.type == col.mDataAttr.type);
        mDataVec.insert(mDataVec.end(), col.mDataVec.begin(), col.mDataVec.end());
        mStrLenOrIndVec.insert(mStrLenOrIndVec.end(), col.mStrLenOrIndVec.begin(), col.mStrLenOrIndVec.end());
        return true;
    };
    virtual int Compare(size_t i, size_t j) const
    {
        if (mDataVec[i] < mDataVec[j]) {
            return -1;
        }
        else if (mDataVec[i] > mDataVec[j]) {
            return 1;
        }
        else {
            return 0;
        }
    };
    virtual bool LessThan(size_t i, size_t j) const
    {
        return mDataVec[i] < mDataVec[j];
    };
    virtual bool GreaterThan(size_t i, size_t j) const
    {
        return mDataVec[i] > mDataVec[j];
    };
public:
    virtual void CopyFrom(const BaseColumn &col) {
        BaseColumn::CopyFrom(col);
        mDataVec = ((ColT&)col).mDataVec;
        mStrLenOrIndVec = ((ColT&)col).mStrLenOrIndVec;
    };
    void PushBack(const T &val)
    {
        assert(mDataAttr.type != T_CHAR && mDataAttr.type != T_NCHAR &&
            mDataAttr.type != T_VARCHAR && mDataAttr.type != T_NVARCHAR);
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
    {};
    CharColT(const char *col_name, const DATA_ATTR_T& attr)
        : ColT<T, data_type>(col_name, attr)
    {};

    virtual ~CharColT() {};
    virtual void Reserve(size_t count)
    {
        ColT<T, data_type>::mDataVec.reserve(count * (ColT<T, data_type>::mDataAttr.a + 1));
        ColT<T, data_type>::mStrLenOrIndVec.reserve(count);
    };
    virtual void SetCount(size_t count)
    {
        ColT<T, data_type>::mDataVec.resize(count * (ColT<T, data_type>::mDataAttr.a + 1));
        size_t old_count = ColT<T, data_type>::mStrLenOrIndVec.size();
        ColT<T, data_type>::mStrLenOrIndVec.resize(count);
        if (count > old_count) {
            if (this->NullAble()) {
                for (size_t i = old_count; i < count; ++i) {
                    ColT<T, data_type>::mStrLenOrIndVec[i] = SQL_NULL_DATA;
                }
            }
            else {
                for (size_t i = old_count; i < count; ++i) {
                    ColT<T, data_type>::mStrLenOrIndVec[i] = SQL_NTS;
                }
            }
        }
    };
    virtual void *GetData()
    {
        return ColT<T, data_type>::mDataVec.data();
    }
    virtual const void *GetData() const
    {
        return ColT<T, data_type>::mDataVec.data();
    }
    virtual void *GetData(size_t i)
    {
        return &ColT<T, data_type>::mDataVec[(ColT<T, data_type>::mDataAttr.a + 1) * i];
    }
    virtual const void *GetData(size_t i) const
    {
        return &ColT<T, data_type>::mDataVec[(ColT<T, data_type>::mDataAttr.a + 1) * i];
    }
    virtual SQLLEN GetDataSize(size_t i) const
    {
        return sizeof(T) * ColT<T, data_type>::mDataAttr.a;
    }

    virtual SQLRETURN BindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar) const
    {
        return SqlBindInParam(hstmt, ipar, *this);
    };
    virtual SQLRETURN BindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar)
    {
        return SQLBindOutCol(hstmt, ipar, *this);
    };

    virtual bool AddFromStr(const std::string &str)
    {
        ColT<T, data_type>::mStrLenOrIndVec.push_back(SQL_NULL_DATA); // does not know what to add
        size_t len = ColT<T, data_type>::mDataVec.size();
        ColT<T, data_type>::mDataVec.resize(len + ColT<T, data_type>::mDataAttr.a + 1);
        return SetFromStr(ColT<T, data_type>::mStrLenOrIndVec.size() - 1, str);
    };
    virtual bool AddFromStr(const char *str)
    {
        ColT<T, data_type>::mStrLenOrIndVec.push_back(SQL_NULL_DATA); // does not know what to add
        size_t len = ColT<T, data_type>::mDataVec.size();
        ColT<T, data_type>::mDataVec.resize(len + ColT<T, data_type>::mDataAttr.a + 1);
        return SetFromStr(ColT<T, data_type>::mStrLenOrIndVec.size() - 1, str);
    };
    virtual bool SetFromStr(size_t i, const std::string &str)
    {
        ColT<T, data_type>::mStrLenOrIndVec[i] = (this->NullAble() && str.empty()) ? SQL_NULL_DATA : SQL_NTS;
        size_t len = (ColT<T, data_type>::mDataAttr.a + 1) * i;
        if (sizeof(T) == 2) {
            string16 wstr = StrToWStr(str);
#ifdef _WIN32
            wcsncpy_s((SQLWCHAR *)mDataVec.data() + len, mDataAttr.a + 1, (SQLWCHAR *)wstr.c_str(), mDataAttr.a);
#else
            int copy_len = (wstr.length() + 1) < ((size_t)ColT<T, data_type>::mDataAttr.a + 1) ?
                wstr.length() + 1 : ColT<T, data_type>::mDataAttr.a + 1;
            memcpy((SQLWCHAR *)ColT<T, data_type>::mDataVec.data() + len, (SQLWCHAR *)wstr.c_str(), copy_len * 2);
#endif
        }
        else if (sizeof(T) == 1) {
#ifdef _WIN32
            strncpy_s((char *)mDataVec.data() + len, mDataAttr.a + 1, str.c_str(), mDataAttr.a);
#else
            strncpy((char *)ColT<T, data_type>::mDataVec.data() + len,
                str.c_str(), ColT<T, data_type>::mDataAttr.a);
#endif
        }
        else {
            UnImplemented(NULL);
        }
        return true;
    };
    virtual bool SetFromStr(size_t i, const string16 &wstr)
    {
        ColT<T, data_type>::mStrLenOrIndVec[i] = (this->NullAble() && wstr.empty()) ? SQL_NULL_DATA : SQL_NTS;
        size_t len = (ColT<T, data_type>::mDataAttr.a + 1) * i;
        if (sizeof(T) == 2) {
#ifdef _WIN32
            wcsncpy_s((SQLWCHAR *)mDataVec.data() + len, mDataAttr.a + 1, (SQLWCHAR *)wstr.c_str(), mDataAttr.a);
#else
            int copy_len = (wstr.length() + 1) < ((size_t)ColT<T, data_type>::mDataAttr.a + 1) ?
                wstr.length() + 1 : ColT<T, data_type>::mDataAttr.a + 1;
            memcpy((SQLWCHAR *)ColT<T, data_type>::mDataVec.data() + len, (SQLWCHAR *)wstr.c_str(), copy_len * 2);
#endif
        }
        else if (sizeof(T) == 1) {
            std::string str = WStrToStr(wstr);;
#ifdef _WIN32
            strncpy_s((char *)mDataVec.data() + len, mDataAttr.a + 1, str.c_str(), mDataAttr.a);
#else
            strncpy((char *)ColT<T, data_type>::mDataVec.data() + len,
                str.c_str(), ColT<T, data_type>::mDataAttr.a);
#endif
        }
        else {
            UnImplemented(NULL);
        }
        return true;
    };
    virtual bool SetFromStr(size_t i, const char *str)
    {
        ColT<T, data_type>::mStrLenOrIndVec[i] = (this->NullAble() && (str == nullptr || str[0] == '\0')) ? SQL_NULL_DATA : SQL_NTS;
        if (str != nullptr) {
            size_t len = (ColT<T, data_type>::mDataAttr.a + 1) * i;
            if (sizeof(T) == 2) {
                string16 wstr = StrToWStr(str);
#ifdef _WIN32
                wcsncpy_s((SQLWCHAR *)mDataVec.data() + len, mDataAttr.a + 1, (SQLWCHAR *)wstr.c_str(), mDataAttr.a);
#else
                int copy_len = (wstr.length() + 1) < ((size_t)ColT<T, data_type>::mDataAttr.a + 1) ?
                    wstr.length() + 1 : ColT<T, data_type>::mDataAttr.a + 1;
                memcpy((SQLWCHAR *)ColT<T, data_type>::mDataVec.data() + len, (SQLWCHAR *)wstr.c_str(), copy_len * 2);
#endif
            }
            else if (sizeof(T) == 1) {
#ifdef _WIN32
                strncpy_s((char *)mDataVec.data() + len, mDataAttr.a + 1, str, mDataAttr.a);
#else
                strncpy((char *)ColT<T, data_type>::mDataVec.data() + len,
                    str, ColT<T, data_type>::mDataAttr.a);
#endif
            }
            else {
                UnImplemented(NULL);
            }
        }
        return true;
    };
    virtual std::string GetAsStr(size_t i) const
    {
        std::string result;
        if (ColT<T, data_type>::mStrLenOrIndVec[i] != SQL_NULL_DATA) {
            size_t len = (ColT<T, data_type>::mDataAttr.a + 1) * i;
            if (sizeof(T) == 2) {
                result = WStrToStr((SQLWCHAR *)ColT<T, data_type>::mDataVec.data() + len);
            }
            else {
                result = (char *)ColT<T, data_type>::mDataVec.data() + len;
            }
            if (result.size() > ColT<T, data_type>::mDataAttr.a) {
                result.resize(ColT<T, data_type>::mDataAttr.a);
            }
        }
        return result;
    }
    virtual string16 GetAsWStr(size_t i) const
    {
        string16 result;
        if (ColT<T, data_type>::mStrLenOrIndVec[i] != SQL_NULL_DATA) {
            size_t len = (ColT<T, data_type>::mDataAttr.a + 1) * i;
            if (sizeof(T) == 2) {
                result = (string16::value_type *)ColT<T, data_type>::mDataVec.data() + len;
            }
            else {
                result = StrToWStr((const char *)ColT<T, data_type>::mDataVec.data() + len);
            }
            if (result.size() > ColT<T, data_type>::mDataAttr.a) {
                result.resize(ColT<T, data_type>::mDataAttr.a);
            }
        }
        return result;
    }
    virtual void SetFromData(size_t i, const void *data)
    {
        const char *str = (data != nullptr) ? (const char *)data : nullptr;
        SetFromStr(i, str);
    };
    virtual void RemoveRow()
    {
        ColT<T, data_type>::mStrLenOrIndVec.pop_back();
        size_t len = ColT<T, data_type>::mDataVec.size();
        if (len > 0) {
            ColT<T, data_type>::mDataVec.resize(len - (ColT<T, data_type>::mDataAttr.a + 1));
        }
    }
    virtual void GenerateFakeData(size_t count)
    {
        this->RemoveAllRows();
        Reserve(count);
        if (sizeof(T) == 1) {
            std::string buff;
            buff.resize(ColT<T, data_type>::mDataAttr.a + 1);
            for (size_t i = 0; i < count; i++) {
                long t = (long)((double)rand() / RAND_MAX * 999999999);
#ifdef _WIN32
                _snprintf_s((char*)buff.data(), buff.size(), mDataAttr.a, "A%09ld", t);
#else
                snprintf((char*)buff.data(), ColT<T, data_type>::mDataAttr.a, "A%09ld", t);
#endif
                AddFromStr((const char *)buff.c_str());
            }
        }
        else if (sizeof(T) == 2) {
            std::wstring buff;
            buff.resize(ColT<T, data_type>::mDataAttr.a + 1);
            for (size_t i = 0; i < count; i++) {
#ifdef _WIN32
                long t = (long)((double)rand() / RAND_MAX * 999999999);
                _snwprintf_s((SQLWCHAR*)buff.data(), buff.size(), mDataAttr.a, L"N%09ld", t);
#else
                UnImplemented(NULL);
#endif
                PushBack((T*)buff.c_str());
            }
        }
        else {
            assert(false); // should not reach here!
        };
    }

    virtual int Compare(size_t i, size_t j) const
    {
        std::string str1 = this->GetAsStr(i);
        std::string str2 = this->GetAsStr(j);
        return strcmp(str1.c_str(), str2.c_str());
    }

    virtual bool LessThan(size_t i, size_t j) const
    {
        return this->GetAsStr(i) < this->GetAsStr(j);
    }

    virtual bool GreaterThan(size_t i, size_t j) const
    {
        return this->GetAsStr(i) > this->GetAsStr(j);
    }

public:
    void PushBack(const T *str)
    {
        if (T_NCHAR == ColT<T, data_type>::mDataAttr.type || T_NVARCHAR == ColT<T, data_type>::mDataAttr.type) {
            ColT<T, data_type>::mStrLenOrIndVec.push_back((this->NullAble() && *str == '\0') ? SQL_NULL_DATA : SQL_NTS);
            size_t len = ColT<T, data_type>::mDataVec.size();
            ColT<T, data_type>::mDataVec.resize(len + ColT<T, data_type>::mDataAttr.a + 1);
#ifdef _WIN32
            wcsncpy_s((SQLWCHAR *)mDataVec.data() + len, mDataAttr.a + 1, (SQLWCHAR *)str, mDataAttr.a);
#else
            UnImplemented(NULL);
#endif
        }
        else {
            AddFromStr((const char *)str);
        }
    };
};


template<class T, DATA_TYPE_T data_type>
class LongVarColT : public ColT<T, data_type>
{
public:
    typedef std::vector<T> LongVarT;
public:
    LongVarColT(const char *col_name, bool null_able = true)
        : ColT<T, data_type>(col_name, GenDataAttr(data_type, null_able, 0, 0))
    {};
    virtual ~LongVarColT() {};
    virtual void RemoveAllRows()
    {
        ColT<T, data_type>::RemoveAllRows();
        mLongVarVec.clear();
    };
    virtual bool Append(const BaseColumn_SharedPtr& pCol)
    {
        if (false == ColT<T, data_type>::Append(pCol)) {
            return false;
        }
        const LongVarColT<T, data_type> &col = *(const LongVarColT<T, data_type> *)pCol.get();
        mLongVarVec.insert(mLongVarVec.end(), col.mLongVarVec.begin(), col.mLongVarVec.end());
        return true;
    };
    virtual void Reserve(size_t count)
    {
        mLongVarVec.reserve(count);
        ColT<T, data_type>::mStrLenOrIndVec.reserve(count);
    };
    virtual void SetCount(size_t count)
    {
        mLongVarVec.resize(count);
        size_t old_count = ColT<T, data_type>::mStrLenOrIndVec.size();
        ColT<T, data_type>::mStrLenOrIndVec.resize(count);
        if (count > old_count) {
            if (this->NullAble()) {
                for (size_t i = old_count; i < count; ++i) {
                    ColT<T, data_type>::mStrLenOrIndVec[i] = SQL_NULL_DATA;
                }
            }
            else {
                for (size_t i = old_count; i < count; ++i) {
                    ColT<T, data_type>::mStrLenOrIndVec[i] = 0;
                }
            }
        }
    };
    virtual void *GetData()
    {
        UnImplemented(__FUNCTION__); // should not happen
        return NULL;
    }
    virtual const void *GetData() const
    {
        UnImplemented(__FUNCTION__); // should not happen
        return NULL;
    }
    virtual void *GetData(size_t i)
    {
        return mLongVarVec[i].data();
    }
    virtual const void *GetData(size_t i) const
    {
        return mLongVarVec[i].data();
    }
    virtual SQLLEN GetDataSize(size_t i) const
    {
        return sizeof(T) * mLongVarVec[i].size();
    }

    virtual SQLRETURN BindInParam(SQLHSTMT hstmt, SQLUSMALLINT ipar) const
    {
        return SqlBindInParam(hstmt, ipar, *this);
    };
    virtual SQLRETURN BindOutCol(SQLHSTMT hstmt, SQLUSMALLINT ipar)
    {
        return SQLBindOutCol(hstmt, ipar, *this);
    };

    virtual bool AddFromStr(const std::string &str)
    {
        ColT<T, data_type>::mStrLenOrIndVec.push_back(0); // will be updated in SetFromStr
        mLongVarVec.push_back(LongVarT()); // will be updated in SetFromStr
        return SetFromStr(mLongVarVec.size() - 1, str);
    };
    virtual bool SetFromStr(size_t i, const std::string &str)
    {
        ColT<T, data_type>::mStrLenOrIndVec[i] = (this->NullAble() && str.empty()) ? SQL_NULL_DATA
            : (SQLLEN)SQL_LEN_DATA_AT_EXEC((SQLLEN)(str.size() * sizeof(T)));
        if (!str.empty()) {
            LongVarT& lvar = mLongVarVec[i];
            lvar.resize(str.size());
            if (sizeof(T) == 2) {
                string16 wstr = StrToWStr(str);
                memcpy(lvar.data(), wstr.c_str(), wstr.size() * 2);
            }
            else if (sizeof(T) == 1) {
                memcpy(lvar.data(), str.c_str(), str.size());
            }
        }
        else {
            if (!mLongVarVec[i].empty()) {
                mLongVarVec[i].clear();
            }
        }
        return true;
    };
    virtual bool AddFromStr(const char *str)
    {
        ColT<T, data_type>::mStrLenOrIndVec.push_back(0); // will be updated in SetFromStr
        mLongVarVec.push_back(LongVarT()); // will be updated in SetFromStr
        return SetFromStr(mLongVarVec.size() - 1, str);
    };
    virtual bool SetFromStr(size_t i, const char *str)
    {
        bool is_null = (str == nullptr) || (str[0] == '\0');
        size_t str_size = is_null ? 0 : strlen(str);
        ColT<T, data_type>::mStrLenOrIndVec[i] = (this->NullAble() && is_null) ? SQL_NULL_DATA
            : (SQLLEN)SQL_LEN_DATA_AT_EXEC((SQLLEN)(str_size * sizeof(T)));
        if (!is_null) {
            LongVarT& lvar = mLongVarVec[i];
            lvar.resize(str_size);
            if (sizeof(T) == 2) {
                string16 wstr = StrToWStr(str);
                memcpy(lvar.data(), wstr.c_str(), wstr.size() * 2);
            }
            else if (sizeof(T) == 1) {
                memcpy(lvar.data(), str, str_size);
            }
        }
        else {
            if (!mLongVarVec[i].empty()) {
                mLongVarVec[i].clear();
            }
        }
        return true;
    };
    virtual std::string GetAsStr(size_t i) const
    {
        if ((SQL_NULL_DATA == ColT<T, data_type>::mStrLenOrIndVec[i]) || mLongVarVec[i].empty()) {
            return std::string();
        }
        else {
            const LongVarT& lvar = mLongVarVec[i];
            if (sizeof(T) == 1) {
                std::string str;
                str.resize(lvar.size());
                memcpy((void *)str.c_str(), lvar.data(), lvar.size());
                return str;
            }
            else if (sizeof(T) == 2) {
                string16 wstr;
                wstr.resize(lvar.size());
                memcpy((void *)wstr.c_str(), lvar.data(), lvar.size() * 2);
                return WStrToStr(wstr);
            }
        }
        return std::string();
    }
    virtual void SetFromData(size_t i, const void *data)
    {
        const char *str = (data != nullptr) ? (const char *)data : nullptr;
        SetFromStr(i, str);
    };
    virtual void RemoveRow()
    {
        ColT<T, data_type>::mStrLenOrIndVec.pop_back();
        mLongVarVec.pop_back();
    }
    virtual void GenerateFakeData(size_t count)
    {
        UnImplemented(__FUNCTION__);
    }
    virtual int Compare(size_t i, size_t j) const
    {
        UnImplemented(__FUNCTION__);
        return 0;
    }
    virtual bool LessThan(size_t i, size_t j) const
    {
        UnImplemented(__FUNCTION__);
        return false;
    }
    virtual bool GreaterThan(size_t i, size_t j) const
    {
        UnImplemented(__FUNCTION__);
        return false;
    }

protected:
    std::vector<LongVarT> mLongVarVec;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

class ColRecords
{
public:
    ColRecords() : mRowCount(0)
    {};
    virtual ~ColRecords()
    {
        ClearAllCols();
    };

public:
    void ClearAllCols()
    {
        mRowCount = 0;
        mPtrCols.clear();
        mErrStr.clear();
    };
    void ClearAllRows()
    {
        mRowCount = 0;
        size_t col_count = mPtrCols.size();
        for (size_t i = 0; i < col_count; i++) {
            mPtrCols[i]->RemoveAllRows();
        }
        mErrStr.clear();
    };
    void Reserve(size_t count)
    {
        size_t col_count = mPtrCols.size();
        for (size_t i = 0; i < col_count; i++) {
            mPtrCols[i]->Reserve(count);
        }
    };
    void SetRowCount(size_t count)
    {
        size_t col_count = mPtrCols.size();
        for (size_t i = 0; i < col_count; i++) {
            mPtrCols[i]->SetCount(count);
        }
        mRowCount = count;
    };
    const char *GetErrStr() const
    {
        return mErrStr.c_str();
    };
    size_t GetRowCount() const
    {
        return mRowCount;
    };
    size_t GetColCount() const
    {
        return mPtrCols.size();
    };
    BaseColumn_SharedPtr GetColumn(size_t index)
    {
        return mPtrCols[index];
    };
    const BaseColumn_SharedPtr GetColumn(size_t index) const
    {
        return mPtrCols[index];
    };
    const std::vector<BaseColumn_SharedPtr>& GetColumns() const
    {
        return mPtrCols;
    };
    std::vector<BaseColumn_SharedPtr>& GetColumns()
    {
        return mPtrCols;
    };
    bool AddCol(const char *col_name, DATA_TYPE_T type, bool null_able = true)
    {
        return AddCol(col_name, GenDataAttr(type, null_able, 0, 0));
    };
    bool AddCol(const char *col_name, const DATA_ATTR_T &attr);
    bool AddColFixedChar(const char *col_name, DATA_TYPE_T type, unsigned char num, bool null_able = false)
    {
        assert(type == T_CHAR || type == T_NCHAR);
        return AddCol(col_name, GenDataAttr(type, null_able, num, 0));
    };
    bool AddColDecimalPs(const char *col_name, unsigned char p, unsigned char s, bool null_able = false)
    {
        return AddCol(col_name, GenDataAttr(T_DECIMAL_PS, null_able, p, s));
    };

    // Re-create column structures
    bool AddColsFromCreateSql(const char *create_sql);
    bool AddColsFromRecords(const ColRecords &records);

    SQLRETURN BindAllInColumns(SQLHSTMT hstmt) const;
    SQLRETURN BindAllOutColumns(SQLHSTMT hstmt) const;

    bool AddRow(const std::string &line, char delimiter = ','); // one line of CSV
    bool AddRow(char *line, char delimiter = ','); // one line of CSV
    bool AddRow(const std::vector<std::string> &strs); // elements in one line of CSV
    bool AddRow(const std::vector<char *> &strs); // elements in one line of CSV
    int AddRows(std::istream &is_csv, int num, char delimiter = ',');
    int AddRows(const ColRecords &records);
    void Swap(ColRecords &records);
    void GenerateFakeData(size_t row_count);

    int RowsToCsv(std::ostream &os_csv, int start_row, int row_num, char delimiter = ',') const;

protected:
    size_t mRowCount;
    std::vector<BaseColumn_SharedPtr> mPtrCols;
    std::string mErrStr;

private:
    // internal temp buffers, to avoid duplicated memory allocations
    std::vector<std::string> mTmpStrs;
    std::vector<char *> mTmpCstrs;
};

HDB_END_NAMESPACE

#endif // _HDB_COLUMNS_H
