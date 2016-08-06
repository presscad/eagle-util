// HotSquare.cpp : Defines the entry point for the console application.
//

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "stdafx.h"
#include "HotSquare.h"
#include "SquareManager.h"

using namespace std;

extern bool Test_Main();

// Global Settings
int SEG_ASSIGN_DISTANCE_MAX = 120;
double SQUARE_ZOOM_LEVEL = 21.4142821;   // Zoom level 21.4142821 (HA ER BIN) is for 10M x 10M square


SegManager gSegManager;
TileManager gTileManager;
SquareManager gSquareManager;


static
bool read_configs_from_ini(const char *config_file) {
    dictionary *dict = iniparser_load(config_file);
    if (NULL == dict) {
        printf("ERROR: cannot open config file: %s\n", config_file);
        return false;
    }

    if (false == read_ini_int(dict, "ASSIGNMENT:SEG_ASSIGN_DISTANCE_MAX", SEG_ASSIGN_DISTANCE_MAX)) {
        iniparser_freedict(dict);
        return false;
    }

    string str;
    if (false == read_ini_string(dict, "ASSIGNMENT:SQUARE_ZOOM_LEVEL", str)) {
        iniparser_freedict(dict);
        return false;
    }
    SQUARE_ZOOM_LEVEL = atof(str.c_str());

    iniparser_freedict(dict);
    return true;
}

bool CheckSettings()
{
    if (HEADING_LEVEL_NUM < 1 || HEADING_LEVEL_NUM > 360 || (360 % HEADING_LEVEL_NUM) != 0) {
        printf("Error: invalid HEADING_LEVEL_NUM: %d\n", HEADING_LEVEL_NUM);
        return false;
    }

    if (!read_configs_from_ini(CONFIG_FILE)) {
        return false;
    }

    return true;
}

int main_kunming()
{
    gSquareManager.SetZoomLevel(SQUARE_ZOOM_LEVEL);
    printf("%s: Square Zoom Level: %.10lf\n", ElapsedTimeStr().c_str(), SQUARE_ZOOM_LEVEL);

    const char *SEGMENTS_CSV_PATH = "Data\\KM_OSM\\WAY_SEGMENTS.csv";
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
    //gTileManager.SaveToHanaExportFiles("Data\\Tiles-Z17");
    //printf("%s: Tiles for zoom level %d saved to file Data\\Tiles-Z17\\\n", ElapsedTimeStr().c_str(), TILE_ZOOM_LEVEL);

    gSquareManager.BuildSquareMap_Multi(gSegManager, gTileManager, 8);
    printf("\n%s: Generated %d squares, %d records.\n", ElapsedTimeStr().c_str(), gSquareManager.GetSquareCount(), gSquareManager.CalcCsvLineCount());

    const char *square_table = "KM_SQUARES";
    gSquareManager.SaveToHanaExportFiles((std::string("Data\\") + square_table).c_str(), "KM_TAXI", square_table);

    printf("%s: Done!\n", ElapsedTimeStr().c_str());
    return 0;
}

const double LNG_HEB = 45.720608;
const double LNG_KM = DMS_TO_DEGREE(25, 2, 30, 0);

int main()
{
    if (false == CheckSettings()) {
        return 1;
    }

    //extern int main_nanjing(); return main_nanjing();
    extern int main_nanjing(); return main_kunming();

    gSquareManager.SetZoomLevel(SQUARE_ZOOM_LEVEL);
    double fLngSpan, fLatSpan;
    double fLng = LNG_KM;
    gSquareManager.GetSquareSpansInMeter(fLng, &fLngSpan, &fLatSpan);
    printf("%s: For Longitude %f, Square Zoom Level: %.10lf (%lf M x %lf M)\n", ElapsedTimeStr().c_str(),
        fLng, gSquareManager.GetZoomLevel(), fLngSpan, fLatSpan);

    const char *SEGMENTS_CSV_PATH = "Data\\WAY_SEGMENTS\\heb_data.csv";
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
    //gTileManager.SaveToHanaExportFiles("Data\\Tiles-Z17");
    //printf("%s: Tiles for zoom level %d saved to file Data\\Tiles-Z17\\\n", ElapsedTimeStr().c_str(), TILE_ZOOM_LEVEL);

#if 0
    gSquareManager.BuildSquareMap_Multi(gSegManager, gTileManager, 8);
    printf("\n%s: Generated %d squares, %d records.\n", ElapsedTimeStr().c_str(), gSquareManager.GetSquareCount(), gSquareManager.CalcCsvLineCount());
#else
    gSquareManager.BuildSquareMap_FromCsv(gSegManager, gTileManager, "Data\\HEB_SQUARES\\index\\HEB_OSM\\HE\\HEB_SQUARES\\data.csv");
    printf("\n%s: Got %d squares, %d records from CSV file.\n", ElapsedTimeStr().c_str(), gSquareManager.GetSquareCount(), gSquareManager.CalcCsvLineCount());
#endif

    Test_Main();

    const char *square_table = "CQ_SQUARES";
    //gSquareManager.SaveToHanaExportFiles((std::string("Data\\") + square_table).c_str(), "ITRAFFIC_TEST", square_table);

    printf("%s: Done!\n", ElapsedTimeStr().c_str());
	return 0;
}

