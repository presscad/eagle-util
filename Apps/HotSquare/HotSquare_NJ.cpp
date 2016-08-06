// HotSquare_NJ.cpp : Defines the entry point for the console application.
//

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <iostream>
#include <fstream>
#include "hdb/hdb_utils.h"
#include "HotSquare.h"
#include "SquareManager.h"

extern SegManager gSegManager;
extern TileManager gTileManager;
extern SquareManager gSquareManager;

using namespace std;


typedef struct {
    string      vechid;
    double      lng;
    double      lat;
    double      speed;
    double      heading; // double in original DB
    string      gpstime;
    short       inload;
    short       inservice;
    int         timeslot;
    int         status;
} VEHICLE_RECORD_NJ_EXT;

static string ToDoubleStr(double f) {
    char buff[32];
    sprintf(buff, "%9.7lf", f);
    return buff;
}

static 
bool ParseRecord(string &line, VEHICLE_RECORD_NJ_EXT &r) {
    /*
      Input line example:
      "3584013563",118.79103499999999,32.010750000000002,31,194,"2011-11-29 07:59:01.0000000",0,1,7,0
    */
    vector<string> buffs;
    CsvLinePopulate(buffs, line, ',');
    if (10 != buffs.size()) {
        printf("invalid CSV line: %s\n", line.c_str());
        return false;
    }

    r.vechid = buffs[0];
    r.lng = atof(buffs[1].c_str());
    r.lat = atof(buffs[2].c_str());
    r.speed = atof(buffs[3].c_str());
    r.heading = atof(buffs[4].c_str());
    r.gpstime = buffs[5];
    r.inload = atoi(buffs[6].c_str());
    r.inservice = atoi(buffs[7].c_str());
    r.timeslot = atoi(buffs[8].c_str());
    r.status = atoi(buffs[9].c_str());

    return true;
}

static void PrintNjAssignStatus(int rec_count)
{
    static CONSOLE_SCREEN_BUFFER_INFO sConsoleInfo;
    if (sConsoleInfo.dwSize.X == 0) {
        ::GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &sConsoleInfo);
    }
    ::SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), sConsoleInfo.dwCursorPosition);
    printf("\r%s: processing #%d\n", ElapsedTimeStr().c_str(), rec_count);
}

// divide 24 hours into 144 slots
// return [0, 143]
static int GetTimeSlot(const string &gpstime)
{
    // gpstime example "2011-11-29 16:30:09.0000000"
    const char *p_time = gpstime.c_str() + 11;
    int hour, min;
    if (2 != sscanf(p_time, "%d:%d", &hour, &min)) {
        cout << ElapsedTimeStr() << ": Invalid GPSTIME: " << gpstime << endl;
        return -1;
    }
    return (hour * 60 + min) / 10;
}

static bool NanjingAssign_UsingTileManager()
{
    const int TOP = 0; //20000;
    const char *infile = "Data\\Nanjing\\1129NJ\\data.csv";
    const char *outfile = "Data\\Nanjing\\1129NJ_ASSIGNMENT.csv";

    cout << ElapsedTimeStr() << ": Enter NanjingAssign_UsingTileManager()\n";
    std::ifstream in(infile);
    if (!in.good()) {
        cout << "NanjingAssign_UsingTileManager: cannot open file " << infile << " for reading." << endl;
        return false;
    }
    remove(outfile);
    std::ofstream out(outfile);
    if (!out.good()) {
        cout << "NanjingAssign_UsingTileManager: cannot open file " << infile << " for writing." << endl;
        return false;
    }

    int count = 0;
    int no_hit_count = 0;

    std::string line;
    while (GetLine(in, line)) {
        VEHICLE_RECORD_NJ_EXT record;
        if (true == ParseRecord(line, record)) {
            if (count % 100000 == 0) {
                PrintNjAssignStatus(count);
            }
            count++;

            COORDINATE_T coord;
            coord.lng = record.lng;
            coord.lat = record.lat;

            SEG_ID_T assigned_seg_id1 = gTileManager.AssignSegment(coord, (int)(record.heading + 0.5));
            if (INVALID_SEG_ID == assigned_seg_id1) {
                no_hit_count++;
            }
            //int time_slot = GetTimeSlot(record.gpstime);

            char buff[1024 * 4];
            sprintf(buff, "%s,%s,%s,%s,%s,\"%s\",%d,%d,%d,%d,%lld\n",
                record.vechid.c_str(), ToDoubleStr(record.lng).c_str(), ToDoubleStr(record.lat).c_str(), ToDoubleStr(record.speed).c_str(),
                ToDoubleStr(record.heading).c_str(), record.gpstime.c_str(), (int)record.inload, (int)record.inservice,
                record.timeslot, record.status, assigned_seg_id1);
            out << buff;
            if (TOP > 0 && count == TOP) {
                break;
            }
        }
    }
    PrintNjAssignStatus(count);

    cout << ElapsedTimeStr() << ": Exit NanjingAssign_UsingTileManager(), total count = " << count
         << " , hit failure count = " << no_hit_count <<endl;

    return true;
}

int main_nanjing()
{
    const char SEGMENTS_CSV_PATH[] = "Data\\Nanjing\\NJ_WAY_SEGMENTS\\data.csv";
    if (false == gSegManager.LoadFromCsvFile(SEGMENTS_CSV_PATH)) {
        printf("Error: cannot read Segments CSV file: %s\n", SEGMENTS_CSV_PATH);
        return 10;
    }
    printf("%s: Found %d segments.\n", ElapsedTimeStr().c_str(), gSegManager.GetSegArrayCount());

    if (false == gTileManager.GenerateTiles(gSegManager)) {
        printf("Error: cannot generate tiles\n");
        return 20;
    }
    printf("%s: Generated %d tiles.\n", ElapsedTimeStr().c_str(), gTileManager.GetTileCount());

    NanjingAssign_UsingTileManager();

    printf("%s: Done!\n", ElapsedTimeStr().c_str());
	return 0;
}
