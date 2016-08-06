#ifdef _WIN32
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include "geo_utils.h"


using namespace std;

namespace geo {

//
// GCJ-02 to BD-09
// see http://blog.csdn.net/yorling/article/details/9175913
//
void bd_encrypt(const double gg_lat, const double gg_lon, double &bd_lat, double &bd_lon)
{
    double x = gg_lon, y = gg_lat;
    double z = sqrt(x * x + y * y) + 0.00002 * sin(y * M_PI);
    double theta = atan2(y, x) + 0.000003 * cos(x * M_PI);
    bd_lon = z * cos(theta) + 0.0065;
    bd_lat = z * sin(theta) + 0.006;
}

//
// BD-09 to GCJ-02 (Mars Geodetic System)
// see http://blog.csdn.net/yorling/article/details/9175913
//
void bd_decrypt(const double bd_lat, const double bd_lon, double &gg_lat, double &gg_lon)
{
    double x = bd_lon - 0.0065, y = bd_lat - 0.006;
    double z = sqrt(x * x + y * y) - 0.00002 * sin(y * M_PI);
    double theta = atan2(y, x) - 0.000003 * cos(x * M_PI);
    gg_lon = z * cos(theta);
    gg_lat = z * sin(theta);
}

//
// BD-09 to WGS84
// See http://blog.csdn.net/coolypf/article/details/8686588
//
void bd09_to_wgs84(const double bd_lat, const double bd_lng, double &lat, double &lng)
{
    // BD09 to Mars
    double mars_lat, mars_lng;
    bd_decrypt(bd_lat, bd_lng, mars_lat, mars_lng);

    // Mars to WGS84
    wgs84_to_mars(mars_lat, mars_lng, lat, lng);
    lat = mars_lat - (lat - mars_lat);
    lng = mars_lng - (lng - mars_lng);
}

//
// WGS84 to BD-09
// See http://blog.csdn.net/coolypf/article/details/8686588
//
void wgs84_to_bd09(const double lat, const double lng, double &bd_lat, double &bd_lng)
{
    double mars_lat, mars_lng;
    wgs84_to_mars(lat, lng, mars_lat, mars_lng);
    bd_encrypt(mars_lat, mars_lng, bd_lat, bd_lng);
}

static bool outOfChina(double lat, double lon)
{
    if (lon < 72.004 || lon > 137.8347)
        return true;
    if (lat < 0.8293 || lat > 55.8271)
        return true;
    return false;
}

static double transformLat(double x, double y)
{
    double ret = -100.0 + 2.0 * x + 3.0 * y + 0.2 * y * y + 0.1 * x * y + 0.2 * sqrt(abs(x));
    ret += (20.0 * sin(6.0 * x * M_PI) + 20.0 * sin(2.0 * x * M_PI)) * 2.0 / 3.0;
    ret += (20.0 * sin(y * M_PI) + 40.0 * sin(y / 3.0 * M_PI)) * 2.0 / 3.0;
    ret += (160.0 * sin(y / 12.0 * M_PI) + 320 * sin(y * M_PI / 30.0)) * 2.0 / 3.0;
    return ret;
}

static double transformLon(double x, double y)
{
    double ret = 300.0 + x + 2.0 * y + 0.1 * x * x + 0.1 * x * y + 0.1 * sqrt(abs(x));
    ret += (20.0 * sin(6.0 * x * M_PI) + 20.0 * sin(2.0 * x * M_PI)) * 2.0 / 3.0;
    ret += (20.0 * sin(x * M_PI) + 40.0 * sin(x / 3.0 * M_PI)) * 2.0 / 3.0;
    ret += (150.0 * sin(x / 12.0 * M_PI) + 300.0 * sin(x / 30.0 * M_PI)) * 2.0 / 3.0;
    return ret;
}

//
// World Geodetic System (WGS-84) ==> Mars Geodetic System (GCJ-02)
// see http://blog.csdn.net/yorling/article/details/9175913
//
void wgs84_to_mars(double wgLat, double wgLon, double &mgLat, double &mgLon)
{
    if (!is_inside_china(wgLat, wgLon)) {
        mgLat = wgLat;
        mgLon = wgLon;
        return;
    }

    //
    // Krasovsky 1940
    //
    // a = 6378245.0, 1/f = 298.3
    // b = a * (1 - f)
    // ee = (a^2 - b^2) / a^2;
    const double a = 6378245.0;
    const double ee = 0.00669342162296594323;

    double dLat = transformLat(wgLon - 105.0, wgLat - 35.0);
    double dLon = transformLon(wgLon - 105.0, wgLat - 35.0);
    double radLat = wgLat / 180.0 * M_PI;
    double magic = sin(radLat);
    magic = 1 - ee * magic * magic;
    double sqrtMagic = sqrt(magic);
    dLat = (dLat * 180.0) / ((a * (1 - ee)) / (magic * sqrtMagic) * M_PI);
    dLon = (dLon * 180.0) / (a / sqrtMagic * cos(radLat) * M_PI);
    mgLat = wgLat + dLat;
    mgLon = wgLon + dLon;
}

}
