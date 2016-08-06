#ifdef _WIN32
// Disable secuity warning message on MSVC
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <string>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <float.h>
#include "VehicleRecords.pb.h"
#include "VehicleRecords_Col.h"

#ifdef _WIN32
// Windows does not have snprintf, use _snprintf instead
#define snprintf _snprintf
#endif

using namespace std;
using namespace com::sap::nic::itraffic;

#ifndef _WIN32
// time64_t related functions for Linux
typedef long long time64_t;
extern "C" time64_t mktime64 (struct tm *t);
extern "C" struct tm *localtime64_r (const time64_t *t, struct tm *p);
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
// Local functions

static inline
long long TimestampToInt64(const SQL_TIMESTAMP_STRUCT &st) {
    struct tm stm;
    memset(&stm, 0, sizeof(struct tm));
    stm.tm_year = st.year - 1900;
    stm.tm_mon = st.month - 1;
    stm.tm_mday = st.day;
    stm.tm_hour = st.hour;
    stm.tm_min = st.minute;
    stm.tm_sec = st.second;
#ifdef _WIN32
    return _mktime64(&stm);
#else
    return mktime64(&stm);
#endif
}

static inline
void Int64ToTimestamp(long long t64, SQL_TIMESTAMP_STRUCT &st) {
    struct tm stm;
#ifdef _WIN32
    _localtime64_s(&stm, &t64);
#else
    localtime64_r(&t64, &stm);
#endif
    st.year = stm.tm_year + 1900;
    st.month = stm.tm_mon + 1;
    st.day = stm.tm_mday;
    st.hour = stm.tm_hour;
    st.minute = stm.tm_min;
    st.second = stm.tm_sec;
    st.fraction = 0;
}

static inline
void GetCurTimestamp(SQL_TIMESTAMP_STRUCT &st) {
    time_t t;
    time(&t);
    struct tm stm;
#ifdef _WIN32
    localtime_s(&stm, &t);
#else
    localtime_r(&t, &stm);
#endif

    st.year = stm.tm_year + 1900;
    st.month = stm.tm_mon + 1;
    st.day = stm.tm_mday;
    st.hour = stm.tm_hour;
    st.minute = stm.tm_min;
    st.second = stm.tm_sec;
    st.fraction = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// class VehicleRecord

void VehicleRecords_Col::Clear() {
    this->mCount = 0;
    this->ARR_GPSDATA_ID.clear();
    this->ARR_DEVID.clear();
    this->ARR_DEVID_LEN.clear();
    this->ARR_ALARMFLAG.clear();
    this->ARR_STATE.clear();
    this->ARR_STIME.clear();
    this->ARR_LATITUDE.clear();
    this->ARR_LONGTITUDE.clear();
    this->ARR_SPEED.clear();
    this->ARR_ORIENTATION.clear();
    this->ARR_GPSTIME.clear();
    this->ARR_ODOMETER.clear();
    this->ARR_OILGAUGE.clear();
}

void VehicleRecords_Col::Reserve(int count) {
    this->ARR_GPSDATA_ID.reserve(count);
    this->ARR_DEVID.reserve(count * DEVID_LEN);
    this->ARR_DEVID_LEN.reserve(count);
    this->ARR_ALARMFLAG.reserve(count);
    this->ARR_STATE.reserve(count);
    this->ARR_STIME.reserve(count);
    this->ARR_LATITUDE.reserve(count);
    this->ARR_LONGTITUDE.reserve(count);
    this->ARR_SPEED.reserve(count);
    this->ARR_ORIENTATION.reserve(count);
    this->ARR_GPSTIME.reserve(count);
    this->ARR_ODOMETER.reserve(count);
    this->ARR_OILGAUGE.reserve(count);
}

void VehicleRecords_Col::CopyFrom(const VehicleRecords_Col& from) {
    assert(this != &from);
    this->mCount = from.mCount;

    ARR_GPSDATA_ID  = from.ARR_GPSDATA_ID;
    ARR_DEVID       = from.ARR_DEVID;
    ARR_DEVID_LEN   = from.ARR_DEVID_LEN;
    ARR_ALARMFLAG   = from.ARR_ALARMFLAG;
    ARR_STATE       = from.ARR_STATE;
    ARR_STIME       = from.ARR_STIME;
    ARR_LATITUDE    = from.ARR_LATITUDE;
    ARR_LONGTITUDE  = from.ARR_LONGTITUDE;
    ARR_SPEED       = from.ARR_SPEED;
    ARR_ORIENTATION = from.ARR_ORIENTATION;
    ARR_GPSTIME     = from.ARR_GPSTIME;
    ARR_ODOMETER    = from.ARR_ODOMETER;
    ARR_OILGAUGE    = from.ARR_OILGAUGE;
}

// Generate random records
void VehicleRecords_Col::GenerateRecords(int count) {
    Clear();
    this->mCount = count;
    Reserve(count);

    for (int i=0; i<count; i++) {
        {
            unsigned long long t = (unsigned long long)((double)rand()/RAND_MAX * 99999999);
            this->ARR_GPSDATA_ID.push_back(t);
        }
        {
            long t = (long)((double)rand()/RAND_MAX * 999999999);
            int size = i * DEVID_LEN;
            ARR_DEVID.resize(size + DEVID_LEN);
            snprintf(ARR_DEVID.data() + size, DEVID_LEN, "01%09ld", t);
            ARR_DEVID_LEN.push_back(SQL_NTS);
        }
        {
            SQL_TIMESTAMP_STRUCT st;
            GetCurTimestamp(st);
            st.minute = rand() % 60;
            st.second = rand() % 60;
            this->ARR_STIME.push_back(st);
        }
        {
            int t = (int)((double)rand()/RAND_MAX * 51200);
            this->ARR_ALARMFLAG.push_back(t);
        }
        {
            const static int STATES[] = {0, 1, 256, 513, 768, 17152, 15753};
            int index = rand() % (sizeof(STATES)/sizeof(int));
            this->ARR_STATE.push_back(STATES[index]);
        }
        {
            double t1 = (double)rand()/RAND_MAX * 10 + 40 + rand()/RAND_MAX;
            double t2 = (double)rand()/RAND_MAX * 10 + 120 + rand()/RAND_MAX;
            this->ARR_LATITUDE.push_back(t1);
            this->ARR_LONGTITUDE.push_back(t2);
        }
        {
            int t = (int)((double)rand()/RAND_MAX * 1000 - 500);
            if (t < 0) t=0;
            this->ARR_SPEED.push_back(t);
        }
        {
            int t = (int)((double)rand()/RAND_MAX * 360 - 180);
            this->ARR_ORIENTATION.push_back(t);
        }
        {
            SQL_TIMESTAMP_STRUCT st;
            GetCurTimestamp(st);
            st.minute = rand() % 60;
            st.second = rand() % 60;
            this->ARR_GPSTIME.push_back(st);
        }
        {
            // ODOMETER and OILGAUGE are all NULLs, what to insert?
            this->ARR_ODOMETER.push_back(DBL_MIN);
            this->ARR_OILGAUGE.push_back(DBL_MIN);
        }
    }
}

void VehicleRecords_Col::ToProtoBuf(VehicleReports &pvr) {
    pvr.Clear();
    pvr.set_report_numer(mCount);

    for (int i=0; i<mCount; i++) {
        Report *pReport = pvr.add_report();
        pReport->set_gpsdata_id(ARR_GPSDATA_ID[i]);
        // for devid
        {
            char devid[DEVID_LEN + 1];
            devid[DEVID_LEN] = '\0';
            if (ARR_DEVID_LEN[i] == SQL_NTS) {
                strncpy(devid, ARR_DEVID.data() + i * DEVID_LEN, sizeof(devid));
            } else {
                memcpy(devid, ARR_DEVID.data() + i * DEVID_LEN, DEVID_LEN);
            }
            pReport->set_devid(devid);
        }
        pReport->set_stime(TimestampToInt64(ARR_STIME[i]));
        pReport->set_alarmflag(ARR_ALARMFLAG[i]);
        pReport->set_state(ARR_STATE[i]);
        pReport->set_latitude(ARR_LATITUDE[i]);
        pReport->set_longtitude(ARR_LONGTITUDE[i]);
        pReport->set_speed(ARR_SPEED[i]);
        pReport->set_orientation(ARR_ORIENTATION[i]);
        pReport->set_gpstime(TimestampToInt64(ARR_GPSTIME[i]));
        if (ARR_ODOMETER[i] != DBL_MIN) {
            pReport->set_odometer(ARR_ODOMETER[i]);
        }
        if (ARR_OILGAUGE[i] != DBL_MIN) {
            pReport->set_oilgauge(ARR_OILGAUGE[i]);
        }
    }
}

void VehicleRecords_Col::FromProtoBuf(const VehicleReports &pvr) {
    Clear();
    mCount = pvr.report_size();
    Reserve(mCount);

    for (int i=0; i<mCount; i++) {
        const Report &report_i = pvr.report(i);

        ARR_GPSDATA_ID.push_back(report_i.gpsdata_id());
        {
            int size = i * DEVID_LEN;
            ARR_DEVID.resize(size + DEVID_LEN);
            strncpy(ARR_DEVID.data() + size, report_i.devid().c_str(), DEVID_LEN);
            ARR_DEVID_LEN.push_back(SQL_NTS);
        }
        {
            SQL_TIMESTAMP_STRUCT st;
            Int64ToTimestamp(report_i.stime(), st);
            ARR_STIME.push_back(st);
        }
        ARR_ALARMFLAG.push_back(report_i.alarmflag());
        ARR_STATE.push_back(report_i.state());
        {
            ARR_LATITUDE.push_back(report_i.latitude());
            ARR_LONGTITUDE.push_back(report_i.longtitude());
        }
        ARR_SPEED.push_back(report_i.speed());
        ARR_ORIENTATION.push_back(report_i.orientation());
        {
            SQL_TIMESTAMP_STRUCT st;
            Int64ToTimestamp(report_i.gpstime(), st);
            ARR_GPSTIME.push_back(st);
        }
        {
            if (report_i.has_odometer()) {
                ARR_ODOMETER.push_back(report_i.odometer());
            } else {
                ARR_ODOMETER.push_back(DBL_MIN); // DBL_MIN is used for "null"
            }
        }
        {
            if (report_i.has_oilgauge()) {
                ARR_OILGAUGE.push_back(report_i.oilgauge());
            } else {
                ARR_OILGAUGE.push_back(DBL_MIN); // DBL_MIN is used for "null"
            }
        }
    }
}
