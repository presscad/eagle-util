#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <stdexcept>
#include "geo_utils.h"
#include "common/common_utils.h"


#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

#define R_EARTH         6371004 // in meters
#define R_EARTH_KM      6371.004 // in km
#define EARTH_RADIUS    6378137
#define POLAR_RADIUS    6356725

#define LAT_METERS_PER_DEGREE   (111194.99646)  // R_EARTH * 2 * PI / 360

#define COORDINATE_PRECISION    1000000.0


GEO_BEGIN_NAMESPACE

int FixedGeoPoint::Lat2FixedLat(double lat)
{
    return (int)(lat * COORDINATE_PRECISION);
}

int FixedGeoPoint::Lng2FixedLng(double lng)
{
    return (int)(lng * COORDINATE_PRECISION);
}

double FixedGeoPoint::FixedLat2Lat(int lat)
{
    return lat / COORDINATE_PRECISION;
}

double FixedGeoPoint::FixedLng2Lng(int lng)
{
    return lng / COORDINATE_PRECISION;
}


// degree to rad
double rad(double degree)
{
    return degree * (M_PI / 180);
}

// rad to degree
double degree(double rad)
{
    return rad * (180 / M_PI);
}

// Get the distance on earth surface between two points
// Refer to http://blog.sina.com.cn/s/blog_65d859c30101akih.html
double distance_in_meter(double lat1, double lng1, double lat2, double lng2)
{
    double radLat1 = rad(lat1);
    double radLat2 = rad(lat2);
    double a = radLat1 - radLat2;
    double b = rad(lng1) - rad(lng2);
    double sin_a_2 = std::sin(a/2);
    double sin_b_2 = std::sin(b/2);
    double s = 2 * std::asin(std::sqrt(sin_a_2 * sin_a_2 +
        std::cos(radLat1) * std::cos(radLat2) * (sin_b_2 * sin_b_2)));
    return s * R_EARTH;
}

static double Ec(double lat)
{
    return POLAR_RADIUS + (EARTH_RADIUS - POLAR_RADIUS) * (90.0 - lat) / 90.0;
}

static double Ed(double lat)
{
    return Ec(lat) * std::cos(rad(lat));
}

// Get (lat, lng) from (lat0, lng0), with distance and angle
// Parameters: distance in meter
//             angle: from north, clock-wize, in degrees
void get_lat_lng_degree(double lat0, double lng0, double distance,
    double angle, double &lat, double &lng)
{
    get_lat_lng_rad(lat0, lng0, distance, rad(angle), lat, lng);
}

void get_lat_lng_rad(double lat0, double lng0, double distance,
    double angle_rad, double &lat, double &lng)
{
    double dx = distance * std::sin(angle_rad);
    double dy = distance * std::cos(angle_rad);

    lng = (dx / Ed(lat0) + rad(lng0)) * 180 / M_PI;
    lat = (dy / Ec(lat0) + rad(lat0)) * 180 / M_PI;
}


// Get the distance on earth surface between two points with the same lat coordinate
double distance_in_meter_same_lat(double lat, double lng1, double lng2)
{
    double r = (R_EARTH * M_PI / 180) * std::cos(rad(lat)) * (lng2 - lng1);
    return r < 0 ? -r : r;
}

// Get the distance on earth surface between two points with the same lng coordinate
double distance_in_meter_same_lng(double lat1, double lat2)
{
    double r = (R_EARTH * M_PI / 180) * (lat2 - lat1);
    return r < 0 ? -r : r;
}

double distance_in_km(double lat1, double lng1, double lat2, double lng2)
{
    double radLat1 = rad(lat1);
    double radLat2 = rad(lat2);
    double a = radLat1 - radLat2;
    double b = rad(lng1) - rad(lng2);
    double sin_a_2 = std::sin(a/2);
    double sin_b_2 = std::sin(b/2);
    double s = 2 * std::asin(std::sqrt(sin_a_2 * sin_a_2 +
        std::cos(radLat1) * std::cos(radLat2) * (sin_b_2 * sin_b_2)));
    return s * R_EARTH_KM;
}

double distance_point_to_segment_square(double lat, double lng,
    double seg_from_lat, double seg_from_lng,
    double seg_to_lat, double seg_to_lng)
{
    //
    //                   o p (coord)
    //
    //
    //              o------------------>o
    //              a (from)   seg      b (to)
    //

    double abx = seg_to_lng - seg_from_lng;
    double aby = seg_to_lat - seg_from_lat;
    double ab2 = abx * abx + aby * aby;
    if (ab2 <= 10e-12) {
        // segment's "from" and "to" node are too close
        auto r = geo::distance_in_meter(seg_from_lat, seg_from_lng, seg_to_lat, seg_to_lng);
        return r * r;
    }

    double apx = lng - seg_from_lng;
    double apy = lat - seg_from_lat;
    double ap_ab = apx * abx + apy * aby;
    double t = ap_ab / ab2;
    if (t < 0) {
        t = 0;
    }
    else if (t > 1) {
        t = 1;
    }

    double r1 = (lng - (seg_from_lng + abx * t)) * LAT_METERS_PER_DEGREE *
        std::cos(lat * M_PI / 180.0);
    double r2 = (lat - (seg_from_lat + aby * t)) * LAT_METERS_PER_DEGREE;
    return r1*r1 + r2*r2;
}

double distance_point_to_segment(double lat, double lng,
    double seg_from_lat, double seg_from_lng,
    double seg_to_lat, double seg_to_lng)
{
    double dist_2 = distance_point_to_segment_square(lat, lng, seg_from_lat, seg_from_lng,
        seg_to_lat, seg_to_lng);
    return std::sqrt(dist_2);
}

double get_heading_in_degree(const double &from_lat, const double &from_lng,
    const double &to_lat, const double &to_lng)
{
    if (from_lat == to_lat && from_lng == to_lng) {
        // -0 so that we know something is wrong, but can still be used to calculation
        return -0.00000001;
    }

    double heading;
    double x1 = to_lng - from_lng;
    double y1 = to_lat - from_lat;
    const double x2 = 0;
    const double y2 = 1;
    double cos_value = (x1*x2 + y1*y2) / (std::sqrt(x1*x1 + y1*y1) * (std::sqrt(x2*x2 + y2*y2)));
    double delta_radian = std::acos(cos_value);
    if (x1 > 0) {
        heading = delta_radian * 180.0 / M_PI;
    }
    else {
        heading = 360.0 - delta_radian * 180.0 / M_PI;
        if (heading >= 360.0) {
            heading -= 360.0;
        }
    }
    return heading;
}

// return the angle from north, clock-wize, in degrees
double get_heading_in_degree(const GeoPoint &from, const GeoPoint &to)
{
    return get_heading_in_degree(from.lat, from.lng, to.lat, to.lng);
}

bool in_same_direction(int heading1, int heading2, int angle_tollerance)
{
    int diff = heading2 + 360 - heading1;
    if (diff >= 360) {
        diff -= 360;
    }
    return diff <= angle_tollerance || diff >= (360 - angle_tollerance);
}

// offset: in meters, e.g., 6.0
void get_offset_segment(double from_lat, double from_lng,
    double to_lat, double to_lng, double offset,
    double& offset_from_lat, double& offset_from_lng,
    double& offset_to_lat, double& offset_to_lng)
{
    double angle = get_heading_in_degree(from_lat, from_lng, to_lat, to_lng) + 90;
    if (angle < 0) {
        // error, the two points may be too close. simply return the non-offset points
        offset_from_lat = from_lat;
        offset_from_lng = from_lng;
        offset_to_lat = to_lat;
        offset_to_lng = to_lng;
        return;
    }

    if (angle >= 360) {
        angle -= 360;
    }
    get_lat_lng_degree(from_lat, from_lng, offset, angle, offset_from_lat, offset_from_lng);
    offset_to_lat = to_lat + offset_from_lat - from_lat;
    offset_to_lng = to_lng + offset_from_lng - from_lng;
}

void get_offset_segment(const GeoPoint &from, const GeoPoint &to, double offset,
    GeoPoint& offset_from, GeoPoint& offset_to)
{
    double angle = get_heading_in_degree(from, to) + 90;
    if (angle < 0) {
        // error, the two points may be too close. simply return the non-offset points
        offset_from = from;
        offset_to = to;
        return;
    }
    if (angle >= 360) {
        angle -= 360;
    }
    get_lat_lng_degree(from.lat, from.lng, offset, angle, offset_from.lat, offset_from.lng);
    offset_to.lat = to.lat + offset_from.lat - from.lat;
    offset_to.lng = to.lng + offset_from.lng - from.lng;
}

void get_offset_linestr_simple(const std::vector<GeoPoint>& points, double offset,
    std::vector<GeoPoint>& offset_points)
{
    if (points.size() < 2) {
        offset_points = points;
        return;
    }
    if (offset < 0.001 && offset > -0.001) {
        offset_points = points;
        return;
    }

    offset_points.clear();
    offset_points.reserve(points.size());

    const auto size = points.size();
    GeoPoint offset_from, offset_to;
    get_offset_segment(points[0], points[1], offset, offset_from, offset_to);
    offset_points.push_back(offset_from);

    for (size_t i = 1; i < size; ++i) {
        get_offset_segment(points[i - 1], points[i], offset, offset_from, offset_to);
        offset_points.push_back(offset_to);
    }
}


static void AppendResultPoints(std::vector<GeoPoint>& result_points,
    const geo::GeoPoint& point)
{
    if (result_points.empty()) {
        result_points.emplace_back(point);
    }
    else if (geo::distance_in_meter(result_points.back(), point) > 2) {
        result_points.emplace_back(point);
    }
}

static geo::GeoPoint OffsetPoint(const geo::GeoPoint& point, double offset,
    double segment_heading)
{
    if (offset > 0) {
        return geo::get_point_degree(point, offset, segment_heading + 90.0);
    }
    else if (offset == 0) {
        return point;
    }
    else {
        return geo::get_point_degree(point, -offset, segment_heading - 90.0);
    }
}

static const int SAME_HEADING_RANGE = 10;
static void SimpleCompressRoutePoints(std::vector<GeoPoint>& points,
    std::vector<bool>& one_ways, double min_len)
{
    if (points.size() <= 1) {
        return;
    }
    const int size = (int)points.size();
    std::vector<int> removals;
    removals.reserve(size);

    for (int i = 0; i < size - 1; ++i) {
        const auto& pt0 = points[i];
        const auto& pt1 = points[i + 1];
        double seg_len = geo::distance_in_meter(pt0, pt1);
        if (seg_len >= min_len) {
            continue;
        }
        int heading = (int)geo::get_heading_in_degree(pt0, pt1);
        int heading_prev = (i == 0) ?
            0 : (int)geo::get_heading_in_degree(points[i - 1], pt0);
        int heading_next = (i < size - 2) ?
            (int)geo::get_heading_in_degree(pt1, points[i + 2]) : 0;
        if (i != 0) {
            if (geo::in_same_direction(heading, heading_prev, SAME_HEADING_RANGE)) {
                removals.push_back(i);
                continue;
            }
        }
        if (i != size - 2) {
            if (geo::in_same_direction(heading, heading_next, SAME_HEADING_RANGE)) {
                removals.push_back(i + 1);
                continue;
            }
        }
    }

    if (!removals.empty()) {
        std::reverse(removals.begin(), removals.end());
        for (auto i : removals) {
            points.erase(points.begin() + i);
            one_ways.erase(one_ways.begin() + i);
        }
    }
}

void get_offset_linestr(std::vector<GeoPoint>& points, std::vector<bool>& one_ways,
    double offset, std::vector<GeoPoint>& offset_points)
{
    SimpleCompressRoutePoints(points, one_ways, 5);
    if (points.size() <= 1 || (offset < 0.001 && offset > -0.001)) {
        offset_points = points;
        return;
    }
    if (one_ways.size() != points.size()) {
        throw std::runtime_error("points number does not match one_way flags number");
    }

    geo::GeoPoint point1, point2;
    offset_points.clear();
    const int size = (int)points.size();
    offset_points.reserve(size);

    // offset for the 1st point
    if (one_ways.front()) {
        AppendResultPoints(offset_points, points.front());
    }
    else {
        auto heading = geo::get_heading_in_degree(points.front(), points[1]);
        auto point = OffsetPoint(points.front(), offset, heading);
        AppendResultPoints(offset_points, point);
    }

    for (int i = 1; i < size - 1; ++i) {
        const GeoPoint& pre_pt = points[i - 1], pt = points[i], next_pt = points[i + 1];
        auto heading1 = geo::get_heading_in_degree(pre_pt, pt);
        auto heading2 = geo::get_heading_in_degree(pt, next_pt);

        if (one_ways[i - 1] && one_ways[i]) {
            AppendResultPoints(offset_points, pt);
        }
        else if (geo::in_same_direction((int)heading1, (int)heading2, SAME_HEADING_RANGE)) {
            // same direction case
            geo::get_offset_segment(pt, next_pt, one_ways[i] ? 0 : offset, point1, point2);
            AppendResultPoints(offset_points, point1);
        }
        else if (std::abs((((int)heading1 - (int)heading2 + 360) % 360) - 180) < SAME_HEADING_RANGE) {
            // u-turn case
            point1 = OffsetPoint(pt, (one_ways[i - 1] ? 0 : offset), heading1);
            AppendResultPoints(offset_points, point1);

            point2 = OffsetPoint(pt, (one_ways[i] ? 0 : offset), heading2);
            AppendResultPoints(offset_points, point2);
        }
        else {
            geo::GeoPoint s1_from, s1_to, s2_from, s2_to;
            geo::get_offset_segment(pre_pt, pt, one_ways[i - 1] ? 0 : offset, s1_from, s1_to);
            geo::get_offset_segment(pt, next_pt, one_ways[i] ? 0 : offset, s2_from, s2_to);

            if (true == geo::lines_intersection(s1_from, s1_to, s2_from, s2_to, point1)) {
                // if have the intersection point
                AppendResultPoints(offset_points, point1);
            }
            else {
                AppendResultPoints(offset_points, s1_to);
                AppendResultPoints(offset_points, s2_from);
            }
        }
    }

    // offset for the last point
    if (one_ways.back()) {
        AppendResultPoints(offset_points, points.back());
    }
    else {
        auto heading = geo::get_heading_in_degree(points[size - 2], points.back());
        auto point = OffsetPoint(points.back(), offset, heading);
        AppendResultPoints(offset_points, point);
    }
}

// Get the projection point on the segment
void get_projection_point(const GeoPoint& point,
    const GeoPoint& seg_from, const GeoPoint& seg_to,
    GeoPoint& projection_point)
{
    double abx = seg_to.lng - seg_from.lng;
    double aby = seg_to.lat - seg_from.lat;
    double ab2 = abx * abx + aby * aby;
    if (ab2 <= 10e-12) {
        // segment's "from" and "to" node are too close
        projection_point = seg_from;
        return;
    }

    double apx = point.lng - seg_from.lng;
    double apy = point.lat - seg_from.lat;
    double ap_ab = apx * abx + apy * aby;
    double t = ap_ab / ab2;
    if (t <= 0) {
        projection_point = seg_from;
    }
    else if (t >= 1) {
        projection_point = seg_to;
    }
    else {
        projection_point.lng = seg_from.lng + abx * t;
        projection_point.lat = seg_from.lat + aby * t;
    }
}

// to_seg_from - true : the distance between the projection point and seg_from point,
//   otherwise between the projection point and seg_to point
double get_projection_distance_in_meter(const GeoPoint& point,
    const GeoPoint& seg_from, const GeoPoint& seg_to,
    bool to_seg_from)
{
    double abx = seg_to.lng - seg_from.lng;
    double aby = seg_to.lat - seg_from.lat;
    double ab2 = abx * abx + aby * aby;
    if (ab2 <= 10e-12) {
        // segment's "from" and "to" node are too close
        return geo::distance_in_meter(seg_from, point);
    }

    double apx = point.lng - seg_from.lng;
    double apy = point.lat - seg_from.lat;
    double ap_ab = apx * abx + apy * aby;
    double t = ap_ab / ab2;
    if (t <= 0) {
        return to_seg_from ?
            0 : distance_in_meter(seg_from.lat, seg_from.lng, seg_to.lat, seg_to.lng);
    }
    else if (t >= 1) {
        return to_seg_from ?
            distance_in_meter(seg_from.lat, seg_from.lng, seg_to.lat, seg_to.lng) : 0;
    }
    else {
        double lng = seg_from.lng + abx * t;
        double lat = seg_from.lat + aby * t;
        return to_seg_from ? distance_in_meter(lat, lng, seg_from.lat, seg_from.lng) :
            distance_in_meter(lat, lng, seg_to.lat, seg_to.lng);
    }
}



static inline bool Equal(double f1, double f2)
{
    double diff = f1 - f2;
    return (diff < 0.00000001) && (diff > -0.00000001);
}

static inline bool Equal(const GeoPoint &p1, const GeoPoint &p2)
{
    return (Equal(p1.lng, p2.lng) && Equal(p1.lat, p2.lat));
}

static inline bool operator>(const GeoPoint &p1, const GeoPoint &p2)
{
    return (p1.lng > p2.lng || (Equal(p1.lng, p2.lng) && p1.lat > p2.lat));
}

// outer product
static inline double operator^(const GeoPoint &p1, const GeoPoint &p2) {
    return (p1.lng * p2.lat - p1.lat * p2.lng);
}

// return: 6: completely overlap, 5: 1 end overlap, 4: partially overlap
//         3: intersects at endpoint, 2: intersects on line, 1: orthogonal, 0: no intersects
//        -1: parameter error
int intersection(const GeoPoint& _p1, const GeoPoint& _p2,
    const GeoPoint& _p3, const GeoPoint& _p4,
    GeoPoint &intsection)
{
    GeoPoint p1(_p1), p2(_p2), p3(_p3), p4(_p4);
    if (Equal(p1, p2) || Equal(p3, p4)) {
        return -1;
    }
    if (p1 > p2) {
        std::swap(p1, p2);
    }
    if (p3 > p4) {
        std::swap(p3, p4);
    }
    if (p1 == p3 && p2 == p4) {
        return 6;
    }

    GeoPoint v1(p2.lat - p1.lat, p2.lng - p1.lng), v2(p4.lat - p3.lat, p4.lng - p3.lng);
    double Corss = v1 ^ v2;
    if (p1 == p3) {
        intsection = p1;
        return (Equal(Corss, 0) ? 5 : 3);
    }
    if (p2 == p4) {
        intsection = p2;
        return (Equal(Corss, 0) ? 5 : 3);
    }
    if (p1 == p4) {
        intsection = p1;
        return 3;
    }
    if (p2 == p3) {
        intsection = p2;
        return 3;
    }

    if (p1 > p3) {
        std::swap(p1, p3);
        std::swap(p2, p4);
        std::swap(v1, v2);
        Corss = v1 ^ v2;
    }

    if (Equal(Corss, 0)) {
        GeoPoint vs(p3.lat - p1.lat, p3.lng - p1.lng);
        if (Equal(v1 ^ vs, 0)) {
            if (p2 > p3) {
                intsection = p3;
                return 4;
            }
        }
        return 0;
    }

    double ymax1 = p1.lat, ymin1 = p2.lat, ymax2 = p3.lat, ymin2 = p4.lat;
    if (ymax1 < ymin1) {
        std::swap(ymax1, ymin1);
    }
    if (ymax2 < ymin2) {
        std::swap(ymax2, ymin2);
    }

    if (p1.lng > p4.lng || p2.lng < p3.lng || ymax1 < ymin2 || ymin1 > ymax2) {
        return 0;
    }

    GeoPoint vs1(p1.lat - p3.lat, p1.lng - p3.lng), vs2(p2.lat - p3.lat, p2.lng - p3.lng);
    GeoPoint vt1(p3.lat - p1.lat, p3.lng - p1.lng), vt2(p4.lat - p1.lat, p4.lng - p1.lng);
    double s1v2, s2v2, t1v1, t2v1;
    if (Equal(s1v2 = vs1 ^ v2, 0) && p4 > p1 && p1 > p3) {
        intsection = p1;
        return 2;
    }
    if (Equal(s2v2 = vs2 ^ v2, 0) && p4 > p2 && p2 > p3) {
        intsection = p2;
        return 2;
    }
    if (Equal(t1v1 = vt1 ^ v1, 0) && p2 > p3 && p3 > p1) {
        intsection = p3;
        return 2;
    }
    if (Equal(t2v1 = vt2 ^ v1, 0) && p2 > p4 && p4 > p1) {
        intsection = p4;
        return 2;
    }

    if (s1v2 * s2v2 > 0 || t1v1 * t2v1 > 0) {
        return 0;
    }

    double ConA = p1.lng * v1.lat - p1.lat * v1.lng;
    double ConB = p3.lng * v2.lat - p3.lat * v2.lng;

    intsection.lng = (ConB * v1.lng - ConA * v2.lng) / Corss;
    intsection.lat = (ConB * v1.lat - ConA * v2.lat) / Corss;
    return 1;
}

// > 0, to the left
// = 0, on the line
// < 0, on the right
double OnSideOfLine(const GeoPoint& p, const GeoPoint& p1, const GeoPoint& p2)
{
    double t = (p1.lat - p2.lat) * p.lng + (p2.lng - p1.lng) * p.lat + p1.lng * p2.lat - p2.lng * p1.lat;
    return t;
}


// The coefficients (a,b) of a line of equation y = a.x + b,
// or the constant x for vertical lines
struct LineEquCoeff
{
    union {
        double a{};
        double x;
    };
    double b{};
    bool x_valid{};
};

// Find the coefficients (a,b) of a line of equation y = a.x + b,
// or the constant x for vertical lines
// Return false if there's no equation possible
static bool LineEquation(const geo::GeoPoint& pt1, const geo::GeoPoint& pt2,
    LineEquCoeff& coeff)
{
    const double epsilon = 10e-8;
    if (std::abs(pt1.lng - pt2.lng) > epsilon) {
        coeff.x_valid = false;
        coeff.a = (pt2.lat - pt1.lat) / (pt2.lng - pt1.lng);
        coeff.b = pt1.lat - coeff.a * pt1.lng;
        return true;
    }

    if (std::abs(pt1.lat - pt2.lat) > epsilon) {
        coeff.x_valid = true;
        coeff.x = pt1.lng;
        return true;
    }

    return false;
}

// Return the intersection point of two lines defined by two points each
// Return false when there's no unique intersection
bool lines_intersection(const geo::GeoPoint& l1a, const geo::GeoPoint& l1b,
    const geo::GeoPoint& l2a, const geo::GeoPoint& l2b, geo::GeoPoint& result)
{
    LineEquCoeff line1, line2;
    bool ok = LineEquation(l1a, l1b, line1);
    if (!ok) return false;
    ok = LineEquation(l2a, l2b, line2);
    if (!ok) return false;

    if (line1.x_valid) {
        if (line2.x_valid) {
            return false;
        }
        result.x() = line1.x;
        result.y() = line2.a * line1.x + line2.b;
        return true;
    }
    if (line2.x_valid) {
        result.x() = line2.x;
        result.y() = line1.a * line2.x + line1.b;
        return true;
    }

    if (line1.a == line2.a) {
        return false;
    }

    result.x() = (line2.b - line1.b) / (line1.a - line2.a);
    result.y() = line1.a * result.x() + line1.b;
    return true;
};


int long2tilex(double lon, double z)
{
    return (int)(std::floor((lon + 180.0) / 360.0 * std::pow(2.0, z)));
}

int lat2tiley(double lat, double z)
{
    return (int)(std::floor(
        (1.0 - std::log(std::tan(lat * M_PI/180.0) + 1.0 / std::cos(lat * M_PI/180.0)) / M_PI)
        / 2.0 * std::pow(2.0, z)));
}

double tilex2long(int x, double z) 
{
    return x / std::pow(2.0, z) * 360.0 - 180;
}

double tiley2lat(int y, double z) 
{
    double n = M_PI - 2.0 * M_PI * y / std::pow(2.0, z);
    return 180.0 / M_PI * std::atan(0.5 * (std::exp(n) - std::exp(-n)));
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

GEO_END_NAMESPACE
