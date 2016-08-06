#ifndef __VEHICLERECORDS_COL_H_
#define __VEHICLERECORDS_COL_H_

#include <vector>
#include <string>
#ifdef _WIN32
#include <windows.h> // required by sqlext.h for WIN32
#include "sqlext.h"
#else
#include "/usr/sap/hdbclient/sdk/odbc/incl/sqlext.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
create column table "ITRAFFIC"."PROTO_ODBC_SINGLE"(
    "GPSDATA_ID" DECIMAL (20) null,
    "DEVID" VARCHAR (20) null,
    "STIME" TIMESTAMP null,
    "ALARMFLAG" DECIMAL (6) null,
    "STATE" DECIMAL (6) null,
    "LATITUDE" DOUBLE null,
    "LONGTITUDE" DOUBLE null,
    "SPEED" INTEGER null,
    "ORIENTATION" DOUBLE null,
    "GPSTIME" TIMESTAMP null,
    "ODOMETER" DOUBLE null,
    "OILGAUGE" DOUBLE null)
*/

#define GPSDATA_ID_LEN  20
#define DEVID_LEN       20

/*
    Class for vehicle records in column mode
*/
class VehicleRecords_Col {
public:
    VehicleRecords_Col() {
        mCount = 0;
    };
    VehicleRecords_Col(const VehicleRecords_Col& from) {
        CopyFrom(from);
    };
    virtual ~VehicleRecords_Col() {};

public:
    void Clear();
    void Reserve(int count);
    void CopyFrom(const VehicleRecords_Col& from);
    void GenerateRecords(int count);
    void ToProtoBuf(::com::sap::nic::itraffic::VehicleReports &pvr);
    void FromProtoBuf(const ::com::sap::nic::itraffic::VehicleReports &pvr);
    int  GetCount() {return mCount;};

public:
    // For double type, const DBL_MIN is used to represent DB "null"
    std::vector<SQLUBIGINT>             ARR_GPSDATA_ID;
    std::vector<char>                   ARR_DEVID; // of the size (mCount * DEVID_LEN)
    std::vector<SQLLEN>                 ARR_DEVID_LEN; // for length of ARR_DEVID
    std::vector<SQLUINTEGER>            ARR_ALARMFLAG;
    std::vector<SQLUINTEGER>            ARR_STATE;
    std::vector<SQL_TIMESTAMP_STRUCT>   ARR_STIME;
    std::vector<SQLDOUBLE>              ARR_LATITUDE;
    std::vector<SQLDOUBLE>              ARR_LONGTITUDE;
    std::vector<SQLINTEGER>             ARR_SPEED;
    std::vector<SQLDOUBLE>              ARR_ORIENTATION;
    std::vector<SQL_TIMESTAMP_STRUCT>   ARR_GPSTIME;
    std::vector<SQLDOUBLE>              ARR_ODOMETER;
    std::vector<SQLDOUBLE>              ARR_OILGAUGE;

protected:
    int mCount;
};

#ifdef __cplusplus
}
#endif
#endif // __VEHICLERECORDS_COL_H_
