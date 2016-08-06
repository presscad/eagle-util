#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <Windows.h>
#include <fstream>
#include <unordered_set>
#include "HotSquare.h"
#include "SquareManager.h"
#include "CsvExporter.h"

using namespace std;
using namespace stdext;


typedef struct {
    SquareManager *pSqManager;
    int nThreadId;
    SEGMENT_T *pSegStart;
    unsigned int nSegCount;
    unsigned int nFinishedCount;
    unordered_set<SQUARE_ID_T> squareIdSet;
} THREAD_DATA1;

typedef struct {
    int nThreadId;
    SquareManager *pSqManager;
    TileManager *pTileManager;
    SQUARE_ID_T *pSqStart;
    unsigned int nSqCount;
    unsigned int nFinishedCount;
    vector<SQUARE_T> *parrSquare;
} THREAD_DATA2;


static
void TimerProc_OnGenSquareIds(void *pData)
{
    if (pData) {
        vector<THREAD_DATA1> &dataArray = *(vector<THREAD_DATA1> *)pData;
        unsigned int nTotalCount = 0;
        unsigned int nTotalFinishedCount = 0;

        static CONSOLE_SCREEN_BUFFER_INFO sConsoleInfo;
        if (sConsoleInfo.dwSize.X == 0) {
            ::GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &sConsoleInfo);
        }
        ::SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), sConsoleInfo.dwCursorPosition);

        printf("\r%s: Generating square IDs: ", ElapsedTimeStr().c_str());
        for (size_t i = 0; i < dataArray.size(); i++) {
            if ((i % 4) == 0) {
                printf("\t\n\t");
            }
            printf("thread #%d - %2.2lf%%, ", dataArray[i].nThreadId, (double)dataArray[i].nFinishedCount/dataArray[i].nSegCount * 100);
            nTotalCount += dataArray[i].nSegCount;
            nTotalFinishedCount += dataArray[i].nFinishedCount;
        }
        printf("\n\tTotal - %2.2lf%% ", (double)nTotalFinishedCount/nTotalCount * 100);
    }
}

static
void TimerProc_OnGenSquareArray(void *pData)
{
    if (pData) {
        vector<THREAD_DATA2> &dataArray = *(vector<THREAD_DATA2> *)pData;
        unsigned int nTotalCount = 0; 
        unsigned int nTotalFinishedCount = 0;

        static CONSOLE_SCREEN_BUFFER_INFO sConsoleInfo;
        if (sConsoleInfo.dwSize.X == 0) {
            ::GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &sConsoleInfo);
        }
        ::SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), sConsoleInfo.dwCursorPosition);

        printf("\r%s: Squares pre-calculating: ", ElapsedTimeStr().c_str());
        for (size_t i = 0; i < dataArray.size(); i++) {
            if ((i % 4) == 0) {
                printf("\t\n\t");
            }
            printf("thread #%d - %2.2lf%%, ", dataArray[i].nThreadId, (double)dataArray[i].nFinishedCount/dataArray[i].nSqCount * 100);

            nTotalCount += dataArray[i].nSqCount;
            nTotalFinishedCount += dataArray[i].nFinishedCount;
        }
        printf("\n\tTotal - %2.2lf%% ", (double)nTotalFinishedCount/nTotalCount * 100);
    }
}

static
bool GetSegmentNeighboringSquareIds(SquareManager *This, const SEGMENT_T *pSegment, unordered_set<SQUARE_ID_T> &sqIdSet)
{
    static const double MARGIN = SEG_ASSIGN_DISTANCE_MAX * 1.1 / LAT_METERS_PER_DEGREE;
    static const double STEP = 2.0 / LAT_METERS_PER_DEGREE;

    static const double MAX_ASSIGN_DISTANCE_2 =
        (SEG_ASSIGN_DISTANCE_MAX + 7) * (SEG_ASSIGN_DISTANCE_MAX + 7);

    double lat1 = pSegment->from.lat;
    double lng1 = pSegment->from.lng;
    double lat2 = pSegment->to.lat;
    double lng2 = pSegment->to.lng;
    if (lat1 > lat2) {
        double t = lat1; lat1 = lat2; lat2 = t;
    }
    if (lng1 > lng2) {
        double t = lng1; lng1 = lng2; lng2 = t;
    }

    lat1 -= MARGIN;
    lat2 += MARGIN;
    lng1 -= MARGIN;
    lng2 += MARGIN;

    sqIdSet.clear();
    COORDINATE_T coord;
    for (coord.lat = lat1; coord.lat <= lat2; coord.lat += STEP) {
        for (coord.lng = lng1; coord.lng < lng2; coord.lng += STEP) {
            SQUARE_ID_T id = This->CoordinateToSquareId(coord);
            if (SegManager::CalcDistanceSquareMeters(coord, *pSegment) < MAX_ASSIGN_DISTANCE_2) {
                if (sqIdSet.find(id) == sqIdSet.end()) {
                        sqIdSet.insert(id);
                }
            }
        }
    }

    return !sqIdSet.empty();
}

static
bool GenerateSquareIds(THREAD_DATA1 *pData1)
{
    unordered_set<SQUARE_ID_T> subSet;
    pData1->squareIdSet.clear();

    for (unsigned int i=0; i<pData1->nSegCount; i++) {
        if ((i % 500) == 0) {
            ::InterlockedExchange(&pData1->nFinishedCount, i + 1);
        }

        subSet.clear();
        GetSegmentNeighboringSquareIds(pData1->pSqManager, &pData1->pSegStart[i], subSet);

        for (unordered_set<SQUARE_ID_T>::iterator it = subSet.begin(); it != subSet.end(); it++) {
            if (pData1->squareIdSet.find(*it) == pData1->squareIdSet.end()) {
                pData1->squareIdSet.insert(*it);
            }
        }
    }
    ::InterlockedExchange(&pData1->nFinishedCount, pData1->nSegCount);

    return !pData1->squareIdSet.empty();
}


static unsigned long WINAPI ThreadFun_GenSquareIds( LPVOID lpParam ) 
{ 
    THREAD_DATA1 *pData = (THREAD_DATA1 *)lpParam;
    GenerateSquareIds(pData);
    return 0; 
}

static
bool GenerateSquareIds_Multi(SquareManager *pSqManager, const SEGMENT_T segments[], int nSegs,
    int nThreadCount, unordered_set<SQUARE_ID_T> &squareIdSet)
{
    if (segments == NULL || nThreadCount <= 0 || nSegs == 0) {
        printf("CalculateSquareIds_Multi: invalid parameter passed in\n");
        return false;
    }

    vector<THREAD_DATA1> dataArray;
    vector<DWORD> dwThreadIdArray;
    vector<HANDLE> hThreadArray;

    dataArray.resize(nThreadCount);
    dwThreadIdArray.resize(nThreadCount);
    hThreadArray.resize(nThreadCount);

    const int nAverageCount = int(nSegs / (double)nThreadCount + 0.5);
    for (int i = 0; i < nThreadCount; i++) {
        dataArray[i].pSqManager = pSqManager;
        dataArray[i].nThreadId = i;
        dataArray[i].pSegStart = (SEGMENT_T *)segments + nAverageCount * i;
        dataArray[i].nSegCount = nAverageCount;
        if (i == nThreadCount - 1) {
            dataArray[i].nSegCount = nSegs - nAverageCount * i;
        }
        dataArray[i].nFinishedCount = 0;
        hThreadArray[i] = ::CreateThread(NULL, 0, ThreadFun_GenSquareIds,
            &dataArray[i], 0, &dwThreadIdArray[i]);
    }

    // Wait until all threads are terminated.
    while (true) {
        TimerProc_OnGenSquareIds(&dataArray);

        bool bNotFinished = false;
        for (size_t i = 0; i < dataArray.size(); i++) {
            if (dataArray[i].nFinishedCount < dataArray[i].nSegCount) {
                bNotFinished = true;
                break;
            }
        }
        if (!bNotFinished) {
            break;
        }
        ::Sleep(1000);
    }
    TimerProc_OnGenSquareIds(&dataArray);
    for(int i=0; i<nThreadCount; i++) {
        ::CloseHandle(hThreadArray[i]);
    }

    // combine the result set
    squareIdSet.clear();
	const unordered_set<SQUARE_ID_T>::iterator squareIdSet_End = squareIdSet.end();

    for (int i=0; i<nThreadCount; i++) {
        unordered_set<SQUARE_ID_T> &subSet = dataArray[i].squareIdSet;
		const unordered_set<SQUARE_ID_T>::iterator subSet_End = subSet.end();
        for (std::unordered_set<SQUARE_ID_T>::iterator it = subSet.begin(); it != subSet_End; it++) {
            if (squareIdSet_End == squareIdSet.find(*it)) {
                squareIdSet.insert(*it);
            }
        }
    }

    return !squareIdSet.empty();
}

static void SquareSetToArray(unordered_set<SQUARE_ID_T> &squareIdSet, vector<SQUARE_ID_T> &arrIds)
{
    arrIds.reserve(squareIdSet.size());
	for (unordered_set<SQUARE_ID_T>::iterator it = squareIdSet.begin(); it != squareIdSet.end(); it++) {
        arrIds.push_back(*it);
    }
}

static
bool GenerateSquareArray(THREAD_DATA2 *pThreadData)
{
    pThreadData->parrSquare->clear();
    pThreadData->parrSquare->reserve(pThreadData->nSqCount);

    for (unsigned int i = 0; i < pThreadData->nSqCount; i++) {
        if ((i % 20000) == 0) {
            ::InterlockedExchange(&pThreadData->nFinishedCount, i + 1);
        }

        SQUARE_T sq;
        sq.square_id = pThreadData->pSqStart[i];

        COORDINATE_T centerCoord;
        pThreadData->pSqManager->SquareIdToCenterCoordinate(sq.square_id, &centerCoord);
        SEG_ID_T seg_id_heading_levels[HEADING_LEVEL_NUM];
        // Get seg ID for each heading for the coordinate
        for (int level = HEADING_LEVEL_NUM - 1; level >= 0; level--) {
            seg_id_heading_levels[level] =
                pThreadData->pTileManager->AssignSegment(centerCoord, level * (360 / HEADING_LEVEL_NUM));
        }

        // compress seg_id_heading_levels[] into psq->arr_headings_seg_id
        for (int i1 = 0; i1 < HEADING_LEVEL_NUM;) {
            int i2 = i1;
            while(i2 < HEADING_LEVEL_NUM && seg_id_heading_levels[i2] == seg_id_heading_levels[i1]) {
                i2++;
            }
            i2--;
            if (seg_id_heading_levels[i1] != INVALID_SEG_ID) {
                HEADINGS_TO_SEG_IDS_T heads_segid;
                heads_segid.from_level = i1;
                heads_segid.to_level = i2;
                heads_segid.seg_id = seg_id_heading_levels[i1];
                sq.arr_headings_seg_id.push_back(heads_segid);
            }
            i1 = i2 + 1;
        }

#ifdef CPP11_SUPPORT
		sq.arr_headings_seg_id.shrink_to_fit();
#endif
        pThreadData->parrSquare->push_back(sq);
    }

    ::InterlockedExchange(&pThreadData->nFinishedCount, pThreadData->nSqCount);
    return !pThreadData->parrSquare->empty();
}

static unsigned long WINAPI ThreadFun_GenSquareArray( LPVOID lpParam ) 
{ 
    THREAD_DATA2 *pData = (THREAD_DATA2 *)lpParam;
    GenerateSquareArray(pData);
    return 0; 
}

static bool GenerateSquareArray_Multi(SquareManager *pSqManager, TileManager &tileMgr,
    int nThreadCount, vector<SQUARE_ID_T> &squareIdArr, SQUARE_MAP_T &sqMap)
{
    vector<THREAD_DATA2> dataArray;
    vector<DWORD> dwThreadIdArray;
    vector<HANDLE> hThreadArray;
    dataArray.resize(nThreadCount);
    dwThreadIdArray.resize(nThreadCount);
    hThreadArray.resize(nThreadCount);

    const int nAverageCount = int(squareIdArr.size() / (double)nThreadCount + 0.5);
    for (int i = 0; i < nThreadCount; i++) {
        dataArray[i].nThreadId = i;
        dataArray[i].pSqManager = pSqManager;
        dataArray[i].pTileManager = &tileMgr;
        dataArray[i].pSqStart = (SQUARE_ID_T *)&squareIdArr[0] + nAverageCount * i;
        dataArray[i].nSqCount = nAverageCount;
        if (i == nThreadCount - 1) {
            dataArray[i].nSqCount = (int)squareIdArr.size() - nAverageCount * i;
        }
        dataArray[i].nFinishedCount = 0;
        dataArray[i].parrSquare = new vector<SQUARE_T>;

        hThreadArray[i] = ::CreateThread(NULL, 0, ThreadFun_GenSquareArray,
            &dataArray[i], 0, &dwThreadIdArray[i]);
    }

    // Wait until all threads are terminated.
    while (true) {
        TimerProc_OnGenSquareArray(&dataArray);

        bool bNotFinished = false;
        for (size_t i = 0; i < dataArray.size(); i++) {
            if (dataArray[i].nFinishedCount < dataArray[i].nSqCount) {
                bNotFinished = true;
                break;
            }
        }
        if (!bNotFinished) {
            break;
        }
        ::Sleep(1000);
    }
    TimerProc_OnGenSquareArray(&dataArray);
    for(int i=0; i<nThreadCount; i++) {
        ::CloseHandle(hThreadArray[i]);
    }

    // build mSquareMap
    sqMap.clear();
    for (size_t n = 0; n < dataArray.size(); n++) {
        vector<SQUARE_T> &arrSquare = *dataArray[n].parrSquare;
        for (size_t i = 0; i < arrSquare.size(); i++) {
            sqMap.insert(SQUARE_MAP_T::value_type(arrSquare[i].square_id, arrSquare[i]));
        }

        delete dataArray[n].parrSquare;
        dataArray[n].parrSquare = NULL;
    }

    return true;
}

double SquareManager::SetZoomLevel(double fZoomLevel) {
    double fOldZoom = mfZoomLevel;
    if (fZoomLevel != mfZoomLevel) {
        mfZoomLevel = fZoomLevel;
        mfTotalSqNum = pow(2, fZoomLevel);
    }
    return fOldZoom;
}

bool SquareManager::BuildSquareMap_Multi(SegManager &segMgr, TileManager &tileMgr, int nThreadCount)
{
    mpSegMgr = &segMgr;
    mpTileMgr = &tileMgr;

    printf("%s: To generate square IDs, waiting ...\n", ElapsedTimeStr().c_str());
    unordered_set<SQUARE_ID_T> squareIdSet;
    bool res = GenerateSquareIds_Multi(this, segMgr.GetSegArray(), segMgr.GetSegArrayCount(),
        nThreadCount, squareIdSet);
    if (res == false)
        return res;
    printf("\n%s: Generated square IDs count: %d\n", ElapsedTimeStr().c_str(), squareIdSet.size());

    //printf("%s: before SetToArray\n", ElapsedTimeStr().c_str());
    vector<SQUARE_ID_T> squareIdArr;
    SquareSetToArray(squareIdSet, squareIdArr);
    squareIdSet.clear();
    //printf("%s: after SetToArray\n", ElapsedTimeStr().c_str());

    GenerateSquareArray_Multi(this, tileMgr, nThreadCount, squareIdArr, mSquareMap);
    squareIdArr.clear();

    return !mSquareMap.empty();
}

// Save squares into CSV file, not saving 64-bit square IDs, but seperated lng IDs and lat IDs.
bool SquareManager::SaveToCsvFile(const char *filename)
{
    std::ofstream out(filename);
    if (!out.good()) {
        printf("Error: cannot open %s for writing!\n", filename);
        return false;
    }

	for (SQUARE_MAP_T::iterator it = mSquareMap.begin(); it != mSquareMap.end(); it++) {
        SQUARE_T *pSq = &it->second;
        for (size_t i = 0; i < pSq->arr_headings_seg_id.size(); i++) {
            char buff[512];
            // "seqare lng id, seqare lat id, heading_from, heading_to, segment id"
            sprintf(buff, "%d,%d,%d,%d,%lld\n", (int)pSq->square_id, (int)(pSq->square_id >> 32),
                (int)pSq->arr_headings_seg_id[i].from_level, (int)pSq->arr_headings_seg_id[i].to_level,
                pSq->arr_headings_seg_id[i].seg_id);
            out << buff;
        }
    }
    out.close();
    return true;
}

static void PrintCsvReadStatus(int rec_count)
{
    static CONSOLE_SCREEN_BUFFER_INFO sConsoleInfo;
    if (sConsoleInfo.dwSize.X == 0) {
        ::GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &sConsoleInfo);
    }
    ::SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), sConsoleInfo.dwCursorPosition);
    printf("\r%s: reading square count: %d", ElapsedTimeStr().c_str(), rec_count);
}

bool SquareManager::BuildSquareMap_FromCsv(SegManager &segMgr, TileManager &tileMgr, const char *filename)
{
    mpSegMgr = &segMgr;
    mpTileMgr = &tileMgr;

    std::ifstream infile(filename);
    if (!infile.good()) {
        printf("Canot open %s\n", filename);
        return false;
    }

    mSquareMap.clear();
    std::string line;

    SQUARE_T sq;
    sq.square_id = INVALID_SEG_ID;

    int rec_count = 0;
    while (GetLine(infile, line)) {
        rec_count++;
        if (rec_count % 100000 == 0) {
            PrintCsvReadStatus(rec_count);
        }

        unsigned int sqlngid, sqlatid;
        int from_level, to_level;
        SEG_ID_T seg_id;

        int r = sscanf(line.c_str(), "%d,%d,%d,%d,%lld",
            &sqlngid, &sqlatid, &from_level, &to_level, &seg_id);
        if (r == 5) {
            HEADINGS_TO_SEG_IDS_T htos;
            htos.from_level = from_level;
            htos.to_level = to_level;
            htos.seg_id = seg_id;

            SQUARE_ID_T square_id = (SQUARE_ID_T)sqlngid | ((SQUARE_ID_T)sqlatid << 32);

            if (sq.square_id == INVALID_SEG_ID) {
                sq.square_id = square_id;
            }

            if (square_id != sq.square_id) {
                mSquareMap.insert(SQUARE_MAP_T::value_type(sq.square_id, sq));

                sq.square_id = square_id;
                sq.arr_headings_seg_id.clear();
            }
            sq.arr_headings_seg_id.push_back(htos);

#if 0
            SQUARE_T *pSq = GetSquareById(square_id);
            if (pSq) {
                pSq->arr_headings_seg_id.push_back(htos);
            } else {
                SQUARE_T sq;
                sq.square_id = square_id;
                sq.arr_headings_seg_id.push_back(htos);
                mSquareMap.insert(SQUARE_MAP_T::value_type(square_id, sq));
            }
#endif
        } else {
            printf("Warning: incorrect line in CSV: %s\n", line.c_str());
        }
    }
    // Insert the last square
    if (sq.square_id != INVALID_SEG_ID && sq.arr_headings_seg_id.size() > 0) {
        mSquareMap.insert(SQUARE_MAP_T::value_type(sq.square_id, sq));
    }

    PrintCsvReadStatus(rec_count);

    infile.close();
    return true;
}


int SquareManager::CalcCsvLineCount()
{
    int count = 0;
	for (SQUARE_MAP_T::iterator it = mSquareMap.begin(); it != mSquareMap.end(); it++) {
        count += (int)it->second.arr_headings_seg_id.size();
    }
    return count;
}

SEG_ID_T SquareManager::AssignSegment(const COORDINATE_T &coord, int nHeading)
{
    SQUARE_ID_T sqId = SquareManager::CoordinateToSquareId(coord);
    SQUARE_T *pSq = GetSquareById(sqId);
    if (!pSq) return INVALID_SEG_ID;

    int headingLevel = SegManager::HeadingToLevel(nHeading);
    for (size_t i = 0; i < pSq->arr_headings_seg_id.size(); i++) {
        const HEADINGS_TO_SEG_IDS_T &head_segids = pSq->arr_headings_seg_id[i];
        if (headingLevel >= head_segids.from_level && headingLevel <= head_segids.to_level) {
            return head_segids.seg_id;
        }
    }
    return INVALID_SEG_ID;
}

bool SquareManager::SaveToHanaExportFiles(const char *folder, const char *schema, const char *table)
{
    const char *sTemplateFolder = "CsvTemplate\\SquareSeg";

    CsvExporter exporter(folder, schema, table, sTemplateFolder, CalcCsvLineCount());
    if (false == SaveToCsvFile(exporter.GetDataFilePath().c_str())) {
        printf("ERROR: cannot save to Square-Segment file: %s\n", exporter.GetDataFilePath().c_str());
        return false;
    }

    if (!exporter.GenerateExportFiles()) {
        printf("ERROR: %s\n", exporter.GetErrorStr().c_str());
        return false;
    }

    printf("%s: Squares saved to file %s, schema:%s, table:%s\n", ElapsedTimeStr().c_str(),
        folder, schema, table);
    return true;
}

void SquareManager::GetSquareSpansInMeter(double lat, double *pLngSpan, double *pLatSpan)
{
    COORDINATE_T coord;
    coord.lng = 125;
    coord.lat = lat; // 45.720608 is for ha-er-bin

    int square_lng_id, square_lat_id;
    this->CoordinateToSquareIds(coord, &square_lng_id, &square_lat_id);

    double lng1, lng2, lat1, lat2;
    lng1 = LngIdToLng(square_lng_id);
    lng2 = LngIdToLng(square_lng_id + 1);
    lat1 = LatIdToLat(square_lat_id);
    lat2 = LatIdToLat(square_lat_id + 1);

    if (pLngSpan) {
        //*pLngSpan = GetDistanceInMeter(lat1, lng1, lat1, lng2);
        *pLngSpan = GetDistanceSameLngInMeter(lat1, lat2);
    }
    if (pLatSpan) {
        //*pLatSpan = GetDistanceInMeter(lat1, lng1, lat2, lng1);
        *pLatSpan = GetDistanceSameLatInMeter(coord.lat, lng1, lng2);
    }
}
