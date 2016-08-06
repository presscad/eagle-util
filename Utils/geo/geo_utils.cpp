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

// degree to rad
double rad(double degree)
{
    return degree * M_PI / 180;
}

// rad to degree
double degree(double rad)
{
    return rad * 180 / M_PI;
}

// Get the distance on earth surface between two points
// Refer to http://blog.sina.com.cn/s/blog_65d859c30101akih.html
double distance_in_meter(double lat1, double lng1, double lat2, double lng2)
{
    double radLat1 = rad(lat1);
    double radLat2 = rad(lat2);
    double a = radLat1 - radLat2;
    double b = rad(lng1) - rad(lng2);
    double sin_a_2 = sin(a/2);
    double sin_b_2 = sin(b/2);
    double s = 2 * asin(sqrt(sin_a_2 * sin_a_2 + cos(radLat1) * cos(radLat2) * (sin_b_2 * sin_b_2)));
    return s * R_EARTH;
}

static double Ec(double lat)
{
    return POLAR_RADIUS + (EARTH_RADIUS - POLAR_RADIUS) * (90 - lat) / 90;
}

static double Ed(double lat)
{
    return Ec(lat) * cos(rad(lat));
}

// Get (lat, lng) from (lat0, lng0), with distance and angle
// Parameters: distance in meter
//             angle: from north, clock-wize, in degrees
void get_lat_lng_degree(double lat0, double lng0, double distance, double angle, double &lat, double &lng)
{
    get_lat_lng_rad(lat0, lng0, distance, rad(angle), lat, lng);
}

void get_lat_lng_rad(double lat0, double lng0, double distance, double angle_rad, double &lat, double &lng)
{
    double dx = distance * sin(angle_rad);
    double dy = distance * cos(angle_rad);

    lng = (dx / Ed(lat0) + rad(lng0)) * 180 / M_PI;
    lat = (dy / Ec(lat0) + rad(lat0)) * 180 / M_PI;
}


// Get the distance on earth surface between two points with the same lat coordinate
double distance_in_meter_same_lat(double lat, double lng1, double lng2)
{
    double r = (R_EARTH * M_PI / 180) * cos(rad(lat)) * (lng2 - lng1);
    return r < 0 ? -r : r;
}

// Get the distance on earth surface between two points with the same lng coordinate
double distance_in_meter_same_lng(double lat1, double lat2)
{
    double r = (R_EARTH * M_PI / 180) * (lat2 - lat1);
    return r < 0 ? -r : r;
}

// return the angle from north, clock-wize, in degrees
double get_heading_in_degree(const COORDINATE &from, const COORDINATE &to)
{
    double heading;
    double x1 = to.lng - from.lng;
    double y1 = to.lat - from.lat;
    const double x2 = 0;
    const double y2 = 1;
    double cos_value = (x1*x2 + y1*y2) / (sqrt(x1*x1 + y1*y1) * (sqrt(x2*x2 + y2*y2)));
    double delta_radian = acos(cos_value);
    if(x1 > 0) {
        heading = delta_radian * 180.0 / M_PI;
    }else{
        heading = 360.0 - delta_radian * 180.0 / M_PI;
    }
    return heading;
}

double get_heading_in_degree(const double &from_lat, const double &from_lng,
    const double &to_lat, const double &to_lng)
{
    COORDINATE from, to;
    from.lat = from_lat;
    from.lng = from_lng;
    to.lat = to_lat;
    to.lng = to_lng;
    return get_heading_in_degree(from, to);
}

int long2tilex(double lon, double z) 
{
    return (int)(floor((lon + 180.0) / 360.0 * pow(2.0, z))); 
}

int lat2tiley(double lat, double z)
{
    return (int)(floor((1.0 - log( tan(lat * M_PI/180.0) + 1.0 / cos(lat * M_PI/180.0)) / M_PI) / 2.0 * pow(2.0, z))); 
}

double tilex2long(int x, double z) 
{
    return x / pow(2.0, z) * 360.0 - 180;
}

double tiley2lat(int y, double z) 
{
    double n = M_PI - 2.0 * M_PI * y / pow(2.0, z);
    return 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
}


static double average_span(double lat, double lng, double zoom)
{
    double lng1, lng2;
    int tile_x = long2tilex(lng, zoom);

    lng1 = tilex2long(tile_x, zoom);
    lng2 = tilex2long(tile_x + 1, zoom);

    return distance_in_meter_same_lat(lat, lng1, lng2);
}

double span_to_zoom_level(const double side_size, const double lat)
{
    const double LNG = 114.1333; // lng does not matter
    double a = 14, b = 24;
    double fa = average_span(lat, LNG, a) - side_size;
    double fb = average_span(lat, LNG, b) - side_size;

    int count = 0;
    while (abs(fa - fb) > 0.0000001) {
        double m = (a + b) / 2.0;
        double fm = average_span(lat, LNG, m) - side_size;
        if (fm * fa < 0) {
            b = m;
            fb = fm;
        } else {
            a = m;
            fa = fm;
        }

        ++count;
        if (count >= 1000) {
            break;
        }
    }

    return (a + b) / 2;
}

}
