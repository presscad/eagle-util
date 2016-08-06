// TestHanaOdbc.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "geo/geo_utils.h"




int test1()
{
    // Nanjing DeJi guangchang
    double bd_lat0 = 32.048893;
    double bd_lng0 = 118.790914;
    double lat0 = 32.0448;
    double lng0 = 118.7794;

    double lat, lng;

    geo::bd09_to_wgs84(bd_lat0, bd_lng0, lat, lng);
    double error_bd = geo::distance_in_meter(lat0, lng0, bd_lat0, bd_lng0);
    double error = geo::distance_in_meter(lat0, lng0, lat, lng);

    double bd_lat, bd_lng;
    geo::wgs84_to_bd09(lat0, lng0, bd_lat, bd_lng);
    error = geo::distance_in_meter(bd_lat0, bd_lng0, bd_lat, bd_lng);

	return 0;
}

int test2()
{
    double lat0 = 25.05888;
    double lng0 = 102.71575;
    double lat_mars, lng_mars;
    double lat_bd, lng_bd;

    printf("WGS84: loc: %lf, %lf\n", lat0, lng0);

    geo::wgs84_to_mars(lat0, lng0, lat_mars, lng_mars);
    printf("MARS: loc: %lf, %lf\n", lat_mars, lng_mars);

    geo::wgs84_to_bd09(lat0, lng0, lat_bd, lng_bd);
    printf("BD-09: loc: %lf, %lf\n", lat_bd, lng_bd);

    return 0;
}

int main()
{
    test2();
    return 0;
}
