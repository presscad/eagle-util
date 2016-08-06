#ifndef SEG_MANAGER_H_
#define SEG_MANAGER_H_

#include <map>
#include <vector>
#include "HotSquare.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// class SegManager

/*
CREATE ROW TABLE "HEB_OSM"."WAY_SEGMENTS"  (
    "ID" BIGINT CS_FIXED DEFAULT 0 NOT NULL,
    "FROM_LAT" DOUBLE CS_DOUBLE NOT NULL, "FROM_LNG" DOUBLE CS_DOUBLE NOT NULL,
    "TO_LAT" DOUBLE CS_DOUBLE NOT NULL, "TO_LNG" DOUBLE CS_DOUBLE NOT NULL,
    "WAY_ID" BIGINT CS_FIXED NOT NULL,
    "ONE_WAY" TINYINT CS_INT DEFAULT 0 NOT NULL,
    "LENGTH" DOUBLE CS_DOUBLE DEFAULT 0 NOT NULL,
    "WEIGHT" DOUBLE CS_DOUBLE DEFAULT 0 ) 
*/

typedef struct {
    double lat;
    double lng;
} COORDINATE_T;

typedef long long SEG_ID_T;
#define INVALID_SEG_ID  ((SEG_ID_T)-1)

typedef struct {
    // The following are raw data members
    SEG_ID_T seg_id;
    COORDINATE_T from;
    COORDINATE_T to;
    unsigned long long way_id;
    short int one_way;
    double length;
    double weight;

    // Below are dynamically generated
    int heading_int; // [0, 359]
} SEGMENT_T;

class SegManager {
public:
    SegManager();
    ~SegManager();

    SEGMENT_T *GetSegArray();
    int GetSegArrayCount() const;
    bool LoadFromCsvFile(const char *path);
    const SEGMENT_T *GetSegByID(const SEG_ID_T &segId) {
        SEG_ID_MAP::iterator it = mSegIdMap.find(segId);
        return (it == mSegIdMap.end()) ? NULL : &mAllSegs[it->second];
    };

    static int HeadingToLevel(double heading) {
        return int((heading + 360.0)/ (360.0/HEADING_LEVEL_NUM) + 0.5) % HEADING_LEVEL_NUM;
    };
    static double GetHeading(const COORDINATE_T &coFrom, const COORDINATE_T &coTo);
    static int GetHeadingLevel(const COORDINATE_T &coFrom, const COORDINATE_T &coTo) {
        return HeadingToLevel(GetHeading(coFrom, coTo));
    }
    static double CalcDistance(const COORDINATE_T &coord, const SEGMENT_T &seg);
    static double CalcDistanceSquareMeters(const COORDINATE_T &coord, const SEGMENT_T &seg);

private:
    typedef std::map<SEG_ID_T, int> SEG_ID_MAP;
    SEG_ID_MAP mSegIdMap; // map seg ID to the index of mAllSegs
    std::vector<SEGMENT_T> mAllSegs;
};


#endif // SEG_MANAGER_H_
