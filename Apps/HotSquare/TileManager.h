#ifndef TILE_MANAGER_H_
#define TILE_MANAGER_H_

#include <unordered_set>
#include "SegManager.h"


////////////////////////////////////////////////////////////////////////////////////////////////////
// class TileManager

typedef unsigned long long TILE_ID_T;

typedef struct {
    // low 32 bit from lng coordinate, hi 32 bit from lat coordinate
    TILE_ID_T tile_id;
    // hash set for all intersected segments
    std::unordered_set<SEG_ID_T> segIdsSet;
    // array of segments containing segments from extra 8 neighbor 
    std::vector<SEGMENT_T *> segsWithNeighbors;
} TILE_T, *P_TILE_T;

typedef std::map<TILE_ID_T, TILE_T> TILE_MAP_T;

class TileManager {
public:
    TileManager() {
        mpSegMgr = NULL;
    };
    ~TileManager() {};
    int GetTileCount() const {
        return (int)mTileMap.size();
    };
    TILE_T *GetTileById(const TILE_ID_T &tileId) {
		TILE_MAP_T::iterator it = mTileMap.find(tileId);
        return it == mTileMap.end() ? NULL : &it->second;
    };
    TILE_T *GetTileByCoord(const COORDINATE_T &coord) {
        return GetTileById(CoordToTileId(coord));
    };
    TILE_MAP_T &GetTileMap() {
        return mTileMap;
    };
    bool GenerateTiles(SegManager &segMgr);
    bool SaveToCsvFile(const char *filename);
    bool SaveSegTilesToCsv(const char *filename);
    bool SaveToHanaExportFiles(const char *folder);
    SEG_ID_T AssignSegment(const COORDINATE_T &coord, int nHeading); // return 0 if not found

    static void GetTileCoordinates(const TILE_ID_T &tileId, COORDINATE_T *pCoord1, COORDINATE_T *pCoord2);
    static void GetBoundingBox(const TILE_ID_T &tileId, double &north, double &south, double &east, double &west);

    static inline TILE_ID_T CoordToTileId(const COORDINATE_T &coord) {
        // Refer to http://wiki.openstreetmap.org/wiki/Slippy_map_tilenames, C/C++ section
        int latId = (int)floor((1.0 - log( tan(coord.lat * M_PI/180.0) + 1.0 / cos(coord.lat * M_PI/180.0)) / M_PI) / 2.0 * (double)TOTAL_TILE_NUM);
        int lngId = (int)floor((coord.lng + 180.0) / 360.0 * (double)TOTAL_TILE_NUM);
        return ((unsigned long long)latId << 32) | (unsigned long long)lngId;
    };
    static inline void TileIdToCenterCoord(const TILE_ID_T &tileId, COORDINATE_T *pCoord) {
        // Refer to http://wiki.openstreetmap.org/wiki/Slippy_map_tilenames, C/C++ section
        pCoord->lat = LatIdToLat((int)(tileId >> 32));
        pCoord->lng = LngIdToLng((int)tileId);
    };
    static inline double LngIdToLng(unsigned int lngId) {
        return lngId / (double)TOTAL_TILE_NUM * 360.0 - 180;
    };
    static inline double LatIdToLat(unsigned int latId) {
        double n = M_PI - 2.0 * M_PI * latId / (double)TOTAL_TILE_NUM;
        return 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
    };

private:
    SegManager *mpSegMgr;
    TILE_MAP_T mTileMap;
};

#endif // TILE_MANAGER_H_
