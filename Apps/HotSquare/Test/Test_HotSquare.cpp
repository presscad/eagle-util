#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <iostream>
#include <fstream>
#include "HotSquare.h"
#include "SquareManager.h"

using namespace std;


typedef struct {
    long long   gpsdata_id;
    string      devid;
    string      stime;
    int         alarmflag;
    int         state;
    double      latitude;
    double      longitude;
    short       speed;
    double      orientation;
    string      gpstime;
    double      odometer;
    double      oilgauge;
} VEHICLE_RECORD;

extern SegManager gSegManager;
extern TileManager gTileManager;
extern SquareManager gSquareManager;

bool Test_CoordinateMapping()
{
    COORDINATE_T coord, coord2;
    coord.lat = 45.72275;
    coord.lng = 126.60161;
    unsigned long long id = gSquareManager.CoordinateToSquareId(coord);
    gSquareManager.SquareIdToCenterCoordinate(id, &coord2);

    int square_lng_id, square_lat_id;
    gSquareManager.CoordinateToSquareIds(coord, &square_lng_id, &square_lat_id);    // For 10X10, it shoudl be: 2380198, 997351

    return true;
}

void PrintCoord(const char *str, const COORDINATE_T &coord)
{
    printf("%s: %lf, %lf\n", str, coord.lng, coord.lat);
}

void PrintTileId(const char *str, const TILE_ID_T &tileId)
{
    printf("%s: 0x%llX, %lld\n", str, tileId, tileId);
}

void PrintSquareId(const char *str, const SQUARE_ID_T &sId)
{
    printf("%s: 0x%llX, %lld\n", str, sId, sId);
}

bool Test_TileManager()
{
    {
        COORDINATE_T coord;
        coord.lng = 126.602420;
        coord.lat = 45.720608;
        TILE_ID_T tileId = TileManager::CoordToTileId(coord);
        PrintCoord("coord", coord);
        printf("tile ID: 0x%llX\n", tileId);

        TileManager::TileIdToCenterCoord(tileId, &coord);
        PrintCoord("coord", coord);
    }
    {
        COORDINATE_T coord;
        coord.lng = 126.602420;
        coord.lat = 45.720608;
        PrintCoord("coord", coord);
        TILE_T *pTile = gTileManager.GetTileByCoord(coord);
        if (pTile == NULL)
            return false;

        COORDINATE_T coord1, coord2;
        TileManager::GetTileCoordinates(pTile->tile_id, &coord1, &coord2);
        PrintCoord("coord1", coord1);
        PrintCoord("coord2", coord2);
    }
    return true;
}

bool Test_TileManager_AssignOnePoint(double lng, double lat, int heading, const SEG_ID_T &expected_id)
{
    COORDINATE_T coord;
    coord.lng = lng;
    coord.lat = lat;

    SEG_ID_T res_id = gTileManager.AssignSegment(coord, heading);
    cout << "expected seg ID: " << expected_id << " assigned ID: " << res_id << "\n";
    return res_id == expected_id;
}

bool Test_TileManager_SampleDataAssignment()
{
    printf("\nTest_TileManager_SampleDataAssignment:\n");
    Test_TileManager_AssignOnePoint(DMS_TO_DEGREE(126,36,51,299), DMS_TO_DEGREE(45,39,46,619), 217, 0);
    Test_TileManager_AssignOnePoint(DMS_TO_DEGREE(126,39,42,803), DMS_TO_DEGREE(45,45,24,299), 272, 9774);
    Test_TileManager_AssignOnePoint(DMS_TO_DEGREE(126,40,45,433), DMS_TO_DEGREE(45,44, 3,418),  56, 2798);
    Test_TileManager_AssignOnePoint(126.70537972222222, 45.75574972222222, 19, 3980);
    Test_TileManager_AssignOnePoint(126.78225972222222, 45.77801972222222, 17, 22197);
    Test_TileManager_AssignOnePoint(126.64852972222224, 45.743409722222225, 22, 12759);
    return true;
}

bool Test_SquareManager_AssignOnePoint(double lng, double lat, int heading, const SEG_ID_T &expected_id)
{
    COORDINATE_T coord;
    coord.lng = lng;
    coord.lat = lat;

    int level = SegManager::HeadingToLevel(heading);

    int square_lng_id, square_lat_id;
    gSquareManager.CoordinateToSquareIds(coord, &square_lng_id, &square_lat_id);
    cout << "square_lng_id: " << square_lng_id << " square_lat_id: " << square_lat_id << "\t";

    SEG_ID_T res_id_1 = gTileManager.AssignSegment(coord, heading);
    SEG_ID_T res_id_2 = gSquareManager.AssignSegment(coord, heading);
    cout << "expected seg ID: " << expected_id << " assigned ID 1: " << res_id_1 << " assigned ID 2: " << res_id_2 << "\n";
    return res_id_2 == expected_id;
    return true;
}

bool Test_SquareManager_SampleDataAssignment()
{
    printf("\nTest_SquareManager_SampleDataAssignment:\n");
    Test_SquareManager_AssignOnePoint(DMS_TO_DEGREE(126,36,51,299), DMS_TO_DEGREE(45,39,46,619), 217, 0);
    Test_SquareManager_AssignOnePoint(DMS_TO_DEGREE(126,39,42,803), DMS_TO_DEGREE(45,45,24,299), 272, 9774);
    Test_SquareManager_AssignOnePoint(DMS_TO_DEGREE(126,40,45,433), DMS_TO_DEGREE(45,44, 3,418),  56, 2798);
    Test_SquareManager_AssignOnePoint(126.65652, 45.78949, 271, 16832);

    return true;
}

bool Test_SegManager_Distance()
{
    COORDINATE_T coord1, coord2;
    coord1.lng = 126.6119100;
    coord1.lat = 45.7396050;
    coord2.lng = 126.6357800;
    coord2.lat = 45.7371900;

    const SEGMENT_T *pSeg1 = gSegManager.GetSegByID(2270); // should be of minimal distance?
    const SEGMENT_T *pSeg2 = gSegManager.GetSegByID(22549);
    double distance1 = gSegManager.CalcDistanceSquareMeters(coord1, *pSeg1);
    double distance2 = gSegManager.CalcDistanceSquareMeters(coord2, *pSeg2);
    return distance1 <= distance2;
}

void Test_GetTileSize()
{
    COORDINATE_T coord;
    coord.lng = 126.5288783855;
    coord.lat = 45.8017592384;
    TILE_ID_T tid = TileManager::CoordToTileId(coord);
    double north, south, east, west;
    TileManager::GetBoundingBox(tid, north, south, east, west);

    double span_lat = GetDistanceInMeter(north, east, south, east);
    double span_lng = GetDistanceInMeter(north, east, north, west);

    double span_lat2 = GetDistanceSameLngInMeter(north, south);
    double span_lng2 = GetDistanceSameLatInMeter(north, east, west);
}

static 
bool ParseRecord(string &line, VEHICLE_RECORD &r) {
    std::vector<std::string> row;
    CsvLinePopulate(row, line, ',');

    if(row.size() < 12){
        return false;
    }

    r.gpsdata_id = atol(row[0].c_str());
    r.devid = row[1];
    r.stime = row[2];
    r.alarmflag = atoi(row[3].c_str());
    r.state = atoi(row[4].c_str());
    r.latitude = atof(row[5].c_str());
    r.longitude = atof(row[6].c_str());
    r.speed = atoi(row[7].c_str());
    r.orientation = atof(row[8].c_str());
    r.gpstime = row[9];
    r.odometer = 0;
    r.oilgauge = 0;

    return true;
}

static string DoubleToStr(double f) {
    char buff[32];
    sprintf(buff, "%9.7lf", f);
    return buff;
}

bool Test_Data5000()
{
    const int TOP = 0;
    const char *infile = "Data\\TEST_DATA_5000.data.csv";
    const char *outfile = "Data\\TEST_DATA_5000.data.out.csv";

    cout << "Enter Test_Data5000()\n";
    std::ifstream in(infile);
    if (!in.good()) {
        cout << "Test_Data5000: cannot open file " << infile << " for reading." << endl;
        return false;
    }
    remove(outfile);
    std::ofstream out(outfile);
    if (!out.good()) {
        cout << "Test_Data5000: cannot open file " << infile << " for writing." << endl;
        return false;
    }

    int count = 0;
    int diff_count = 0;
    int no_hit_count = 0;
    int rev1_count = 0;
    int rev2_count = 0;

    std::string line;
    while (GetLine(in, line)) {
        VEHICLE_RECORD record;
        if (true == ParseRecord(line, record)) {
            count++;

            COORDINATE_T coord;
            coord.lng = record.longitude;
            coord.lat = record.latitude;

            int assigned_seg_id1_rev_flag = 0;
            SEG_ID_T assigned_seg_id1 = gTileManager.AssignSegment(coord, (int)(record.orientation + 0.5));
            if (INVALID_SEG_ID == assigned_seg_id1) {
                assigned_seg_id1 = gTileManager.AssignSegment(coord, (int)(record.orientation + 180 + 0.5));
                if (assigned_seg_id1 != INVALID_SEG_ID) {
                    assigned_seg_id1_rev_flag = 1;
                    rev1_count++;
                }
            }

            int assigned_seg_id2_rev_flag = 0;
            SEG_ID_T assigned_seg_id2 = gSquareManager.AssignSegment(coord, (int)(record.orientation + 0.5));
            if (INVALID_SEG_ID == assigned_seg_id2) {
                assigned_seg_id2 = gSquareManager.AssignSegment(coord, (int)(record.orientation + 180 + 0.5));
                if (assigned_seg_id2 != INVALID_SEG_ID) {
                    assigned_seg_id2_rev_flag = 1;
                    rev2_count++;
                }
            }

            out << record.gpsdata_id << ',' << record.devid << ',' << '"'
                << record.stime << '"' << ','
                << record.alarmflag << ',' << record.state << ','
                << DoubleToStr(record.latitude) << ',' << DoubleToStr(record.longitude) << ','
                << DoubleToStr(record.speed) << ',' << DoubleToStr(record.orientation) << ','
                << '"' << record.gpstime << '"' << ','
                << record.odometer << ',' << record.oilgauge << ','
                << assigned_seg_id1 << ',' << assigned_seg_id1_rev_flag;

            // Comment out the lines below for generation of 5000 records for accuracy verification
            out << ',' << assigned_seg_id2 << ',' << assigned_seg_id2_rev_flag;

            out << endl;

            if (assigned_seg_id2 == INVALID_SEG_ID) {
                no_hit_count++;
            }
            if (assigned_seg_id1 != assigned_seg_id2 || assigned_seg_id1_rev_flag != assigned_seg_id2_rev_flag) {
                diff_count++;
            }

            if (TOP > 0 && count == TOP) {
                break;
            }
        }
    }

    cout << "Exit Test_Data5000(), total count = " << count
         << ", diff count = " << diff_count << " , hit failure count = " << no_hit_count
         << ", reverse 1 = " << rev1_count << ", reverse 2 = " << rev2_count <<endl;

    return true;
}


bool Test_Main()
{
/*
    if (false == Test_CoordinateMapping()) {
        printf("Test_CoordinateMapping failed!\n");
    }
    if (false == Test_TileManager()) {
        printf("Test_TileManager failed!\n");
    }
    Test_GetTileSize();
    Test_SegManager_Distance();
    Test_TileManager_SampleDataAssignment();
    Test_SquareManager_SampleDataAssignment();
*/
    Test_Data5000();

    return true;
}
