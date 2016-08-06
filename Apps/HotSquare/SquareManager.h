#ifndef SQUARE_SQUARE_H_
#define SQUARE_SQUARE_H_

#include "HotSquare.h"
#include "SegManager.h"
#include "TileManager.h"


////////////////////////////////////////////////////////////////////////////////////////////////////
// class SquareManager

typedef unsigned long long SQUARE_ID_T;

typedef struct {
    short       from_level;
    short       to_level;
    SEG_ID_T    seg_id;
} HEADINGS_TO_SEG_IDS_T;

typedef struct {
    // low 32 bit from lng coordinate, hi 32 bit from lat coordinate
    SQUARE_ID_T square_id;
    // Array of segment IDs for all the heading levels
    std::vector<HEADINGS_TO_SEG_IDS_T> arr_headings_seg_id;
} SQUARE_T, *P_SQUARE_T;

typedef std::map<SQUARE_ID_T, SQUARE_T> SQUARE_MAP_T;

class SquareManager {
public:
    SquareManager(double fZoomLevel = SQUARE_ZOOM_LEVEL) {
        mpSegMgr = NULL;
        mfZoomLevel = 0;
        SetZoomLevel(fZoomLevel);
    };
    ~SquareManager() {};

    double SetZoomLevel(double fZoomLevel);
    double GetZoomLevel() {
        return mfZoomLevel;
    };
    bool BuildSquareMap_Multi(SegManager &segMgr, TileManager &tileMgr, int nThreadCount);
    bool BuildSquareMap_FromCsv(SegManager &segMgr, TileManager &tileMgr, const char *filename);

    SQUARE_MAP_T &GetSquareMap() {
        return mSquareMap;
    };
    int GetSquareCount() {
        return (int)mSquareMap.size();
    };
    SQUARE_T *GetSquareById(const SQUARE_ID_T &id) {
		SQUARE_MAP_T::iterator it = mSquareMap.find(id);
        return (it == mSquareMap.end()) ? NULL : &it->second;
    };
    int CalcCsvLineCount();
    bool SaveToCsvFile(const char *filename);
    bool SaveToHanaExportFiles(const char *folder, const char *schema, const char *table);
    SEG_ID_T AssignSegment(const COORDINATE_T &coord, int nHeading); // return 0 if not found

    inline SQUARE_ID_T CoordinateToSquareId(const COORDINATE_T &coord) {
        int latId = (int)floor((1.0 - log( tan(coord.lat * M_PI/180.0) + 1.0 / cos(coord.lat * M_PI/180.0)) / M_PI) / 2.0 * mfTotalSqNum);
        int lngId = (int)floor((coord.lng + 180.0) / 360.0 * mfTotalSqNum);
        return ((unsigned long long)latId << 32) | (unsigned long long)lngId;
    };
    void CoordinateToSquareIds(const COORDINATE_T &coord, int *square_lng_id, int *square_lat_id) {
        *square_lat_id = (int)floor((1.0 - log( tan(coord.lat * M_PI/180.0) + 1.0 / cos(coord.lat * M_PI/180.0)) / M_PI) / 2.0 * (double)mfTotalSqNum);
        *square_lng_id = (int)floor((coord.lng + 180.0) / 360.0 * (double)mfTotalSqNum);
    };
    inline void SquareIdToCenterCoordinate(const SQUARE_ID_T &id, COORDINATE_T *pCoord) {
        unsigned int lngId = (int)id;
        unsigned int latId = (int)(id >> 32);
        double n = M_PI - 2.0 * M_PI * (latId + 0.5) / (double)mfTotalSqNum;
        pCoord->lat = 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
        pCoord->lng = (lngId + 0.5) / (double)mfTotalSqNum * 360.0 - 180;
    };
    inline double LngIdToLng(unsigned int lngId) {
        return lngId / (double)mfTotalSqNum * 360.0 - 180;
    };
    inline double LatIdToLat(unsigned int latId) {
        double n = M_PI - 2.0 * M_PI * latId / (double)mfTotalSqNum;
        return 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
    };
    void GetSquareSpansInMeter(double lat, double *pLngSpan, double *pLatSpan);

private:
    SegManager  *mpSegMgr;
    TileManager *mpTileMgr;
    SQUARE_MAP_T mSquareMap;

    double  mfZoomLevel;
    double  mfTotalSqNum;
};

#endif // SQUARE_SQUARE_H_
