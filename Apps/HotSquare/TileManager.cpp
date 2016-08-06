#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <string>
#include <fstream>
#include <float.h>
#include <unordered_set>
#include "TileManager.h"
#include "CsvExporter.h"

using namespace std;
using namespace stdext;


#ifndef COUNT_OF
#define COUNT_OF(a) (sizeof(a)/sizeof(*a))
#endif

#define MAKE_TILE_ID(low, hi) ((TILE_ID_T)(low) | ((TILE_ID_T)(hi) << 32))

#define DECLARE_NEIGHBOR_IDS(arr_name, tile_id) \
        int _lng_ = (int)(tile_id); \
        int _lat_ = (int)((tile_id) >> 32); \
        TILE_ID_T arr_name[] = { \
            MAKE_TILE_ID(_lng_-1, _lat_-1), MAKE_TILE_ID(_lng_, _lat_-1), MAKE_TILE_ID(_lng_+1, _lat_-1), \
            MAKE_TILE_ID(_lng_-1, _lat_),                                 MAKE_TILE_ID(_lng_+1, _lat_), \
            MAKE_TILE_ID(_lng_-1, _lat_+1), MAKE_TILE_ID(_lng_, _lat_+1), MAKE_TILE_ID(_lng_+1, _lat_+1) \
        }

static inline bool InSameDirection(int heading1, int heading2) {
    int diff = (heading2 + 360 - heading1) % 360;
    return diff <= 90 || diff >= 270;
}

static inline int GetAngle(int heading1, int heading2) {
    int diff = (heading2 + 360 - heading1) % 360;
    return (diff <= 180) ? diff : 360 - diff;
}


// return number of neighboring tiles
// NOTE: buff size of neiTiles[] must be 8
static inline int GetNeighborTiles(TILE_MAP_T &tileMap, TILE_T *pThisTile, P_TILE_T neiTiles[]) {
    DECLARE_NEIGHBOR_IDS(idNeighbors, pThisTile->tile_id);
    int count = 0;
    for (int i = 0; i < COUNT_OF(idNeighbors); i++) {
		TILE_MAP_T::iterator it = tileMap.find(idNeighbors[i]);
        if (it != tileMap.end()) {
            neiTiles[count] = &it->second;
            count++;
        }
    }

    return count;
}

/*
 * If the tile ID is new, create a new tile for it and insert it to the tile map.
 * If the tile ID is already in tile map, update the related segment ID
*/
static inline void CheckUpdateTile(TILE_MAP_T &tileMap, const TILE_ID_T &tileId,
    const SEG_ID_T &segId) {

    TILE_MAP_T::iterator it = tileMap.find(tileId);
    if (it == tileMap.end()) {
        TILE_T tile;
        tile.tile_id = tileId;
        tile.segIdsSet.insert(segId);
        tileMap.insert(TILE_MAP_T::value_type(tileId, tile));
    } else {
        // The tile for the tile ID already in the map, segment ID => segment ID set
        TILE_T *pTile = &it->second;
        if (pTile->segIdsSet.find(segId) == pTile->segIdsSet.end()) {
            pTile->segIdsSet.insert(segId);
        }
    }
}

static inline void AddToSegsNoDuplicate(SegManager &mSegMgr, vector<SEGMENT_T *> &segs,
    unordered_set<SEG_ID_T> &segIdsSet) {
    for (unordered_set<SEG_ID_T>::iterator it = segIdsSet.begin(); it != segIdsSet.end(); it++) {
        const SEG_ID_T &segId = *it;
        bool found = false;
        for (int i = (int)segs.size() - 1; i >= 0; i--) {
            if (segId == segs[i]->seg_id) {
                found = true;
                break;
            }
        }
        if (!found) {
            segs.push_back((SEGMENT_T *)mSegMgr.GetSegByID(segId));
        }
    }
}

// Make sure this tile has 8 neighboring tiles, even though some of them are empty.
// This is because in this way, if a point falls on one of the neighboring tiles (e.g., E2),
// it can still find "This" tile to get the segment.
//  E1   E2   E3
//  E4   This E5
//  E6   E7   E8
static inline void CheckAddEmptyNeighborTiles(TILE_MAP_T &tileMap, TILE_ID_T tileId) {
    DECLARE_NEIGHBOR_IDS(nbTileIdArray, tileId);
    for (int i = 0; i < COUNT_OF(nbTileIdArray); i++) {
		TILE_MAP_T::iterator it = tileMap.find(nbTileIdArray[i]);
        if (it == tileMap.end()) {
            // Simply inser an empty tile for the neighbor
            TILE_T tile;
            tile.tile_id = nbTileIdArray[i];
            tileMap.insert(TILE_MAP_T::value_type(nbTileIdArray[i], tile));
        }
    }
}

// This function only update pTile->segsWithNeighbors[]
static inline void UpdateTileForNeighborSegs(SegManager &mSegMgr, TILE_MAP_T &mTileMap, TILE_T *pTile) {
    pTile->segsWithNeighbors.clear();
    pTile->segsWithNeighbors.reserve(pTile->segIdsSet.size() * 3);

    // copy the segments into pTile->segsWithNeighbors
	for (unordered_set<SEG_ID_T>::iterator it = pTile->segIdsSet.begin(); it != pTile->segIdsSet.end(); it++) {
        //pTile->segsWithNeighbors.push_back(*it);
        pTile->segsWithNeighbors.push_back((SEGMENT_T *)mSegMgr.GetSegByID(*it));
    }

    // copy the neighboring segments into pTile->segsWithNeighbors
    P_TILE_T nbTiles[8];
    int count = GetNeighborTiles(mTileMap, pTile, nbTiles);
    for (int i = 0; i < count; i++) {
        AddToSegsNoDuplicate(mSegMgr, pTile->segsWithNeighbors, nbTiles[i]->segIdsSet);
    }

#ifdef CPP11_SUPPORT
    pTile->segsWithNeighbors.shrink_to_fit();
#endif
}

bool TileManager::GenerateTiles(SegManager &segMgr)
{
    mpSegMgr = &segMgr;
    if (mpSegMgr->GetSegArrayCount() == 0)
        return false;

    SEGMENT_T *allSegments = mpSegMgr->GetSegArray();
    const int nSegCount = mpSegMgr->GetSegArrayCount();

    for (int i = 0; i < nSegCount; i++) {
        const SEGMENT_T &seg = allSegments[i];
        TILE_ID_T tileId1 = CoordToTileId(seg.from);
        TILE_ID_T tileId2 = CoordToTileId(seg.to);
        if (tileId1 == tileId2) {
            CheckUpdateTile(mTileMap, tileId1, seg.seg_id);
        } else {
            CheckUpdateTile(mTileMap, tileId1, seg.seg_id);
            CheckUpdateTile(mTileMap, tileId2, seg.seg_id);
        }
    }

    {
        vector<TILE_ID_T> arrTileIds;
        arrTileIds.reserve(mTileMap.size());
		for (TILE_MAP_T::iterator it = mTileMap.begin(); it != mTileMap.end(); it++) {
            arrTileIds.push_back(it->first);
        }
        for (int i = (int)arrTileIds.size() - 1; i >= 0; i--) {
            CheckAddEmptyNeighborTiles(mTileMap, arrTileIds[i]);
        }
    }

	for (TILE_MAP_T::iterator it = mTileMap.begin(); it != mTileMap.end(); it++) {
        UpdateTileForNeighborSegs(segMgr, mTileMap, &it->second);
    }

    return !mTileMap.empty();
}

bool TileManager::SaveToCsvFile(const char *filename)
{
    std::ofstream out(filename);
    if (!out.good())
        return false;

    for (TILE_MAP_T::iterator it = mTileMap.begin(); it != mTileMap.end(); it++) {
        COORDINATE_T coord1, coord2;
        GetTileCoordinates(it->second.tile_id, &coord1, &coord2);

        char buff[1024];
        sprintf(buff, "0x%llX,%lf,%lf,%lf,%lf\n", it->second.tile_id,
            coord1.lat, coord1.lng, coord2.lat, coord2.lng);
        out << buff;
    }
    out.close();
    return true;
}

void TileManager::GetBoundingBox(const TILE_ID_T &tileId,
    double &north, double &south, double &east, double &west)
{
    north = LatIdToLat((unsigned int)(tileId >> 32));
    south = LatIdToLat((unsigned int)(tileId >> 32) + 1);
    west = LngIdToLng((unsigned int)tileId);
    east = LngIdToLng((unsigned int)tileId + 1);
}

void TileManager::GetTileCoordinates(const TILE_ID_T &tileId, COORDINATE_T *pCoord1, COORDINATE_T *pCoord2)
{
    if (pCoord1 == NULL && pCoord2 == NULL)
        return;

    double north, south, east, west;
    GetBoundingBox(tileId, north, south, east, west);

    if (pCoord1) {
        pCoord1->lat = north;
        pCoord1->lng = west;
    }
    if (pCoord2) {
        pCoord2->lat = south;
        pCoord2->lng = east;
    }
}

SEG_ID_T TileManager::AssignSegment(const COORDINATE_T &coord, int nHeading)
{
    TILE_ID_T tileId = TileManager::CoordToTileId(coord);
	TILE_MAP_T::iterator it = mTileMap.find(tileId);
    if (it == mTileMap.end())
        return INVALID_SEG_ID;

    TILE_T *pTile = &it->second;
    std::vector<SEGMENT_T *> &arrSegs = pTile->segsWithNeighbors;

    double distanceMin = DBL_MAX;
    int minIndex = -1;

    const int MAX = 512 * 4;
    if (arrSegs.size() > MAX) {
        printf("BUFFER SIZE TOO SMALL!!!, SEGMENTS NUMBER IS %d\n", (int)arrSegs.size());
        *(int*)0 = 1; // to crash
    }
	bool aIsSameDir[MAX];
    double aDistances[MAX];

    for (size_t i = 0; i < arrSegs.size(); i++) {
        const SEGMENT_T *pSeg = arrSegs[i];

        // If not the same direction, ignore
        aIsSameDir[i] = InSameDirection(pSeg->heading_int, nHeading);
        if (aIsSameDir[i]) {
            // get the min distance
            double distance = SegManager::CalcDistanceSquareMeters(coord, *pSeg);
            aDistances[i] = distance;
            if (distance < distanceMin) {
                minIndex = (int)i;
                distanceMin = distance;
            }
        }
    }

    if (distanceMin > SEG_ASSIGN_DISTANCE_MAX * SEG_ASSIGN_DISTANCE_MAX) {
        return INVALID_SEG_ID;
    }

    static const double ERROR_2 = 1*1;
    int angleMin = 180;
    for (size_t i = 0; i < arrSegs.size(); i++) {
        if (aIsSameDir[i] &&
            (distanceMin - aDistances[i] < ERROR_2) && (distanceMin - aDistances[i] > -ERROR_2))
        {
            int angle = GetAngle(arrSegs[i]->heading_int, nHeading);
            if (angle < angleMin) {
                minIndex = (int)i;
                angleMin = angle;
            }
        }
    }

    return minIndex < 0 ? INVALID_SEG_ID : arrSegs[minIndex]->seg_id;
}

bool TileManager::SaveToHanaExportFiles(const char *folder)
{
    const char *sTemplateFolder = "CsvTemplate\\SegsTiles";

    CsvExporter exporter(folder, "I078212", "WAY_SEGMENT_17", sTemplateFolder, mpSegMgr->GetSegArrayCount());
    if (false == SaveSegTilesToCsv(exporter.GetDataFilePath().c_str())) {
        printf("ERROR: cannot save to segment-tile file: %s\n", exporter.GetDataFilePath().c_str());
        return false;
    }

    if (!exporter.GenerateExportFiles()) {
        printf("ERROR: %s\n", exporter.GetErrorStr().c_str());
        return false;
    }
    return true;
}

bool TileManager::SaveSegTilesToCsv(const char *filename)
{
    std::ofstream out(filename);
    if (!out.good()) {
        printf("Error: cannot open %s to write.", filename);
        return false;
    }

    // TODO: how to save?

    out.close();
    return true;
}
