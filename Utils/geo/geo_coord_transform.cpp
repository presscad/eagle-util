
#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdlib>
#include "geo_utils.h"


using namespace std;

GEO_BEGIN_NAMESPACE

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif
static const double X_PI = M_PI * 3000.0 / 180.0;

//
// GCJ-02 to BD-09
// see http://blog.csdn.net/coolypf/article/details/8569813
//

void bd_encrypt(double gg_lat, double gg_lon, double &bd_lat, double &bd_lon)
{
    double x = gg_lon, y = gg_lat;
    double z = sqrt(x * x + y * y) + 0.00002 * sin(y * X_PI);
    double theta = atan2(y, x) + 0.000003 * cos(x * X_PI);
    bd_lon = z * cos(theta) + 0.0065;
    bd_lat = z * sin(theta) + 0.006;
}

//
// BD-09 to GCJ-02 (Mars Geodetic System)
// see http://blog.csdn.net/coolypf/article/details/8569813
//
void bd_decrypt(double bd_lat, double bd_lon, double &gg_lat, double &gg_lon)
{
    double x = bd_lon - 0.0065, y = bd_lat - 0.006;
    double z = sqrt(x * x + y * y) - 0.00002 * sin(y * X_PI);
    double theta = atan2(y, x) - 0.000003 * cos(x * X_PI);
    gg_lon = z * cos(theta);
    gg_lat = z * sin(theta);
}

//
// BD-09 to WGS84
// See http://blog.csdn.net/coolypf/article/details/8686588
//
void bd09_to_wgs84(double bd_lat, double bd_lng, double &lat, double &lng)
{
    // BD09 to Mars
    double mars_lat, mars_lng;
    bd_decrypt(bd_lat, bd_lng, mars_lat, mars_lng);

    // Mars to WGS84
    mars_to_wgs84(mars_lat, mars_lng, lat, lng);
}

//
// WGS84 to BD-09
// See http://blog.csdn.net/coolypf/article/details/8686588
//
void wgs84_to_bd09(double lat, double lng, double &bd_lat, double &bd_lng)
{
    double mars_lat, mars_lng;
    wgs84_to_mars(lat, lng, mars_lat, mars_lng);
    bd_encrypt(mars_lat, mars_lng, bd_lat, bd_lng);
}

#if 0
static bool outOfChina(double lat, double lon)
{
    if (lon < 72.004 || lon > 137.8347)
        return true;
    if (lat < 0.8293 || lat > 55.8271)
        return true;
    return false;
}
#endif

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
#if 0
    if (!is_inside_china(wgLat, wgLon)) {
        mgLat = wgLat;
        mgLon = wgLon;
        return;
    }
#endif
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

void mars_to_wgs84(double mgLat, double mgLon, double &wgLat, double &wgLon, int loop_time)
{
    double lat_new, lng_new;

    wgLat = mgLat;
    wgLon = mgLon;
    for (int i = 0; i < loop_time; ++i)
    {
        geo::wgs84_to_mars(wgLat, wgLon, lat_new, lng_new);
        wgLat = mgLat - (lat_new - wgLat);
        wgLon = mgLon - (lng_new - wgLon);
    }

    // no need to verify, after a couple of rounds, it is already very accurate
#if 0
    geo::wgs84_to_mars(wgLat, wgLon, lat_new, lng_new);
    double distance = geo::distance_in_meter(lat_new, lng_new, mgLat, mgLon);
#endif
}

GEO_END_NAMESPACE
