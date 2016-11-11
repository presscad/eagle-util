#ifndef _GEO_UTILS_H
#define _GEO_UTILS_H

#include <vector>
#include <string>
#include <memory>
#include <tuple>

#define GEO_BEGIN_NAMESPACE namespace geo {
#define GEO_END_NAMESPACE }

GEO_BEGIN_NAMESPACE

///////////////////////////////////////////////////////////////////////////////////////////////////
// Basics

struct GeoPoint
{
    GeoPoint() : lat(0), lng(0)
    {}

    GeoPoint(double lat, double lng)
        : lat(lat), lng(lng)
    {}

    bool Empty() const
    {
        return (lat == 0) && (lng == 0);
    }

    bool operator==(const GeoPoint& src) const
    {
        return src.lat == lat && src.lng == lng;
    }

    bool operator!=(const GeoPoint& src) const
    {
        return src.lat != lat || src.lng != lng;
    }

    static GeoPoint GetMidPoint(const GeoPoint& pt1, const GeoPoint& pt2)
    {
        return GeoPoint((pt1.lat + pt2.lat) / 2, (pt1.lng + pt2.lng) / 2);
    }

    double& x()
    {
        return lng;
    }
    double& y()
    {
        return lat;
    }

    const double& x() const
    {
        return lng;
    }
    const double& y() const
    {
        return lat;
    }

    static GeoPoint FromXY(double x, double y)
    {
        return GeoPoint(y, x);
    }
    static GeoPoint FromLatLng(double lat, double lng)
    {
        return GeoPoint(lat, lng);
    }

    double lat{};
    double lng{};
};


struct FixedGeoPoint
{
    explicit FixedGeoPoint()
    {}
    explicit FixedGeoPoint(const FixedGeoPoint& src)
        : lat(src.lat), lng(src.lng)
    {}
    explicit FixedGeoPoint(const GeoPoint& src)
        : lat(Lat2FixedLat(src.lat)), lng(Lng2FixedLng(src.lng))
    {}

    static int Lat2FixedLat(double lat);
    static int Lng2FixedLng(double lng);
    static double FixedLat2Lat(int lat);
    static double FixedLng2Lng(int lng);

    bool operator==(const GeoPoint& src) const
    {
        return src.lat == lat && src.lng == lng;
    }

    bool operator!=(const GeoPoint& src) const
    {
        return src.lat != lat || src.lng != lng;
    }

    GeoPoint ToGeoPoint() const
    {
        return GeoPoint(FixedLat2Lat(lat), FixedLng2Lng(lng));
    }

    void ToGeoPoint(GeoPoint& point) const
    {
        point.lat = FixedLat2Lat(lat);
        point.lng = FixedLat2Lat(lng);
    }

    int lat{};
    int lng{};
};


struct Bound
{
    Bound()
    {}

    Bound(double minlat, double minlng, double maxlat, double maxlng)
        : minlat(minlat), minlng(minlng), maxlat(maxlat), maxlng(maxlng)
    {}

    bool Empty() const
    {
        return (0 == minlat && 0 == maxlat && 0 == minlng && 0 == maxlng);
    }

    bool Within(double lat, double lng) const
    {
        return (lat > minlat && lat < maxlat && lng > minlng && lng < maxlng);
    }

    bool Within(const GeoPoint& pt) const
    {
        return (pt.lat > minlat && pt.lat < maxlat && pt.lng > minlng && pt.lng < maxlng);
    }

    bool LatOutBound(double lat) const
    {
        return (lat < minlat || lat > maxlat);
    }

    bool LngOutBound(double lng) const
    {
        return (lng < minlng || lng > maxlng);
    }

    bool OutOfBound(const GeoPoint& point) const
    {
        return LatOutBound(point.lat) || LngOutBound(point.lng);
    }

    bool OutOfBound(double lat, double lng) const
    {
        return LatOutBound(lat) || LngOutBound(lng);
    }

    bool IntersectBound(const Bound& b2) const
    {
        auto& b1 = *this;
        return !(b1.minlng > b2.maxlng || b2.minlng > b1.maxlng || b1.minlat > b2.maxlat || b2.minlat > b1.maxlat);
    }

    void Expand(double lat, double lng)
    {
        if (lat < minlat) {
            minlat = lat;
        }
        else if (lat > maxlat) {
            maxlat = lat;
        }
        if (lng < minlng) {
            minlng = lng;
        }
        else if (lng > maxlng) {
            maxlng = lng;
        }
    }

    void Expand(const GeoPoint& point)
    {
        Expand(point.lat, point.lng);
    }

    // format: left, bottom, right, top
    std::string ToString() const
    {
        std::string str = std::to_string(minlng);
        str += ',';
        str += std::to_string(minlat);
        str += ',';
        str += std::to_string(maxlng);
        str += ',';
        str += std::to_string(maxlat);
        return str;
    }

    double minlat{};
    double minlng{};
    double maxlat{};
    double maxlng{};
};


static inline double DMS_TO_DEGREE(int d, int m, int s, int ms)
{
    return (d + m / 60.0 + (s + ms / 1000.0) / 3600.0);
}

double rad(double degree);
double degree(double rad);

double distance_in_meter(double lat1, double lng1, double lat2, double lng2);
static inline double distance_in_meter(const GeoPoint &p1, const GeoPoint &p2)
{
    return distance_in_meter(p1.lat, p1.lng, p2.lat, p2.lng);
}
double distance_in_meter_same_lat(double lat, double lng1, double lng2);
double distance_in_meter_same_lng(double lat1, double lat2);
double distance_in_km(double lat1, double lng1, double lat2, double lng2);
static inline double distance_in_km(const GeoPoint &p1, const GeoPoint &p2)
{
    return distance_in_km(p1.lat, p1.lng, p2.lat, p2.lng);
}

double distance_point_to_segment(double lat, double lng,
    double seg_from_lat, double seg_from_lng, double seg_to_lat, double seg_to_lng);
double distance_point_to_segment_square(double lat, double lng,
    double seg_from_lat, double seg_from_lng, double seg_to_lat, double seg_to_lng);

static inline double distance_point_to_segment(const GeoPoint& point,
    const GeoPoint& seg_from, const GeoPoint& seg_to)
{
    return distance_point_to_segment(point.lat, point.lng, seg_from.lat, seg_from.lng,
        seg_to.lat, seg_to.lng);
}
static inline double distance_point_to_segment_square(const GeoPoint& point,
    const GeoPoint& seg_from, const GeoPoint& seg_to)
{
    return distance_point_to_segment_square(point.lat, point.lng, seg_from.lat, seg_from.lng,
        seg_to.lat, seg_to.lng);
}

void get_lat_lng_degree(double lat0, double lng0, double distance, double angle, double &lat, double &lng);
void get_lat_lng_rad(double lat0, double lng0, double distance, double angle_rad, double &lat, double &lng);
static inline GeoPoint get_point_degree(const GeoPoint &from, double distance, double angle)
{
    GeoPoint point;
    get_lat_lng_degree(from.lat, from.lng, distance, angle, point.lat, point.lng);
    return point;
}

double get_heading_in_degree(const GeoPoint &from, const GeoPoint &to);
double get_heading_in_degree(const double &from_lat, const double &from_lng,
    const double &to_lat, const double &to_lng);
bool in_same_direction(int heading1, int heading2, int angle_tollerance);

// offset in meters
void get_offset_segment(double from_lat, double from_lng, double to_lat, double to_lng, double offset,
    double& offset_from_lat, double& offset_from_lng, double& offset_to_lat, double& offset_to_lng);
void get_offset_segment(const GeoPoint &from, const GeoPoint &to, double offset,
    GeoPoint& offset_from, GeoPoint& offset_to);
void get_offset_linestr_simple(const std::vector<GeoPoint>& points, double offset,
    std::vector<GeoPoint>& offset_points);
// note: offset_points may have different point count from points
void get_offset_linestr(std::vector<GeoPoint>& points, std::vector<bool>& one_ways,
    double offset, std::vector<GeoPoint>& offset_points);

void get_projection_point(const GeoPoint& point, const GeoPoint& seg_from, const GeoPoint& seg_to,
    GeoPoint& projection_point);
static inline GeoPoint get_projection_point(const GeoPoint& point,
    const GeoPoint& seg_from, const GeoPoint& seg_to)
{
    GeoPoint projection_point;
    get_projection_point(point, seg_from, seg_to, projection_point);
    return projection_point;
}
double get_projection_distance_in_meter(const GeoPoint& point, const GeoPoint& seg_from,
    const GeoPoint& seg_to, bool to_seg_from);

// Return the intersection point of two lines defined by two points each
// Return false when there's no unique intersection
bool lines_intersection(const GeoPoint& l1a, const GeoPoint& l1b, const GeoPoint& l2a,
    const GeoPoint& l2b, GeoPoint& result);
int intersection(const GeoPoint& p1, const GeoPoint& p2, const GeoPoint& p3, const GeoPoint& p4,
    GeoPoint &intsection);
double OnSideOfLine(const GeoPoint& p, const GeoPoint& seg_from, const GeoPoint& seg_to);

int long2tilex(double lon, double z);
int lat2tiley(double lat, double z);
double tilex2long(int x, double z);
double tiley2lat(int y, double z);

double span_to_zoom_level(const double side_size, const double lat);

static inline int tilexy2tilex(long long xy)
{
    return (int)(unsigned long)(xy >> 32);
}
static inline int tilexy2tiley(long long xy)
{
    return (int)(unsigned long)xy;
}
static inline long long make_tilexy(int x, int y)
{
    return ((unsigned long long)x << 32) | (unsigned long long)y;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Coordiantes Systems Transform

// WGS84 (World Geodetic System) to GCJ-02 (Mars Geodetic System)
void wgs84_to_mars(double wgLat, double wgLon, double &mgLat, double &mgLon);
void mars_to_wgs84(double mgLat, double mgLon, double &wgLat, double &wgLon, int loop_time = 2);
static inline GeoPoint wgs84_to_mars(const GeoPoint& point)
{
    GeoPoint mars_point;
    wgs84_to_mars(point.lat, point.lng, mars_point.lat, mars_point.lng);
    return mars_point;
}
static inline GeoPoint mars_to_wgs84(const GeoPoint& mars_point, int loop_time = 2)
{
    GeoPoint point;
    mars_to_wgs84(mars_point.lat, mars_point.lng, point.lat, point.lng, loop_time);
    return point;
}

// Mars to BD-09
void bd_encrypt(double gg_lat, double gg_lon, double &bd_lat, double &bd_lon);
// BD-09 to Mars
void bd_decrypt(double bd_lat, double bd_lon, double &gg_lat, double &gg_lon);

// BD-09 to WGS84
void bd09_to_wgs84(double bd_lat, double bd_lng, double &lat, double &lng);
// WGS84 to BD-09
void wgs84_to_bd09(double lat, double lng, double &bd_lat, double &bd_lng);

bool is_inside_china(double lat, double lng);

///////////////////////////////////////////////////////////////////////////////////////////////////
// Polygon

class SimplePolygon;
typedef std::vector<SimplePolygon> SimplePolygons;
class Polygon;
class MultiPolygon;

bool point_in_polygon(double lat, double lng, const SimplePolygon& polygon);
bool point_in_polygon(const GeoPoint& point, const SimplePolygon& polygon);
bool point_in_polygons(double lat, double lng, const SimplePolygons& polygons);
bool point_in_polygons(const GeoPoint& point, const SimplePolygons& polygons);
bool point_in_polygon(double lat, double lng, const Polygon& poly);
bool point_in_multi_polygon(double lat, double lng, const MultiPolygon& mpoly);

bool polygon_covered_by(const SimplePolygon& polygon, double lat, double lng);
bool polygon_covered_by(const Polygon& polygon, double lat, double lng);
bool multi_polygon_covered_by(const MultiPolygon& mpoly, double lat, double lng);

bool polygon_to_wkt(const SimplePolygon& polygon, std::string& wkt);
bool polygons_to_wkt(const SimplePolygons& polygons, std::string& wkt);
bool polygon_to_wkt(const Polygon& polygon, std::string& wkt);
bool multi_polygon_to_wkt(const MultiPolygon& multipoly, std::string& wkt);

bool wkt_to_polygon(const std::string& wkt, SimplePolygon& polygon);
bool wkt_to_polygon(const std::string& wkt, Polygon& polygon);
bool wkt_to_multi_polygon(const std::string& wkt, MultiPolygon& multipoly);

double get_polygon_area(const SimplePolygon& polygon);
double get_polygons_area(const SimplePolygons& polygons);
double get_polygon_area(const Polygon& poly);
double get_multi_polygon_area(const MultiPolygon& mpoly);

GeoPoint get_polygon_centroid(const SimplePolygon& polygon);
bool get_polygon_centroid(const SimplePolygon& polygon, double& lat, double& lng);
bool get_polygon_centroid_simple(const SimplePolygon& polygon, double& lat, double& lng);


class SimplePolygonHelper;
class SimplePolygon
{
public:
    explicit SimplePolygon()
    {
        vertexes.reserve(64);
    }

    SimplePolygon(const SimplePolygon& src) : vertexes(src.vertexes)
    {}

    void Clear()
    {
        vertexes.clear();
        ResetInternalHelper();
    }

    bool Empty()
    {
        return vertexes.empty();
    }

    size_t VertexNumber() const
    {
        return vertexes.size();
    }

    void Reserve(size_t size)
    {
        vertexes.reserve(size);
    }

    void PushBack(const GeoPoint& pt)
    {
        vertexes.push_back(pt);
        ResetInternalHelper();
    }

    const GeoPoint& Vertex(size_t i) const
    {
        return vertexes[i];
    }

    GeoPoint& Vertex(size_t i)
    {
        return vertexes[i];
    }

    const std::vector<GeoPoint>& Vertexes() const
    {
        return vertexes;
    }

    bool Within(double lat, double lng) const
    {
        return point_in_polygon(lat, lng, *this);
    }

    bool CoveredBy(double lat, double lng) const
    {
        return polygon_covered_by(*this, lat, lng);
    }

    double Area() const
    {
        return get_polygon_area(*this);
    }

    std::string WKT() const
    {
        std::string wkt;
        polygon_to_wkt(*this, wkt);
        return wkt;
    }

    // need to be called each time polygon data is changed
    void ResetInternalHelper()
    {
        if (helper_ != nullptr) {
            helper_.reset();
        }
    }

public:
    std::vector<GeoPoint> vertexes;

private:
    mutable std::shared_ptr<SimplePolygonHelper> helper_;
    friend class SimplePolygonHelper;
};


class PolygonHelper;
class Polygon
{
public:
    void Clear()
    {
        outer_polygon.Clear();
        inner_polygons.clear();
        ResetInternalHelper();
    }

    bool Within(double lat, double lng) const
    {
        return point_in_polygon(lat, lng, *this);
    }

    bool CoveredBy(double lat, double lng) const
    {
        return polygon_covered_by(*this, lat, lng);
    }

    double Area() const
    {
        return get_polygon_area(*this);
    }

    std::string WKT() const
    {
        std::string wkt;
        polygon_to_wkt(*this, wkt);
        return wkt;
    }

    // need to be called each time polygon data is changed
    void ResetInternalHelper()
    {
        if (helper_ != NULL) {
            helper_.reset();
        }
    }

public:
    SimplePolygon  outer_polygon;
    SimplePolygons inner_polygons;

private:
    mutable std::shared_ptr<PolygonHelper> helper_;
    friend class PolygonHelper;
};


class MultiPolygonHelper;
class MultiPolygon
{
public:
    void Clear()
    {
        polygons.clear();
        ResetInternalHelper();
    }

    bool Within(double lat, double lng) const
    {
        return point_in_multi_polygon(lat, lng, *this);
    }

    bool CoveredBy(double lat, double lng) const
    {
        return multi_polygon_covered_by(*this, lat, lng);
    }

    double Area() const
    {
        return get_multi_polygon_area(*this);
    }

    std::string WKT() const
    {
        std::string wkt;
        multi_polygon_to_wkt(*this, wkt);
        return wkt;
    }

    // need to be called each time polygon data is changed
    void ResetInternalHelper()
    {
        if (helper_ != NULL) {
            helper_.reset();
        }
    }

public:
    std::vector<Polygon> polygons;

private:
    mutable std::shared_ptr<MultiPolygonHelper> helper_;
    friend class MultiPolygonHelper;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// GeoJSON utils

enum GEO_TYPE
{
    GEOTYPE_INVALID = 0,
    GEOTYPE_POINT,
    GEOTYPE_MULTIPOINT,
    GEOTYPE_LINESTRING,
    GEOTYPE_MULTILINESTRING,
    GEOTYPE_POLYGON,
    GEOTYPE_MULTIPOLYGON,
    GEOTYPE_GEOMETRYCOLLECTION,
};

class GeoJSON;
class GeoJsonHelper;

class GeoObj
{
public:
    // Json property tuple: (name, value, type)
    typedef std::tuple<std::string, std::string, int> JsonProp;

public:
    explicit GeoObj(GEO_TYPE geo_type)
        : geo_type_(geo_type)
    {}

    GEO_TYPE GetObjType() const
    {
        return geo_type_;
    }

    void AddProp(const std::string& name, const std::string& value);
    void AddProp(const std::string& name, const char* value);
    void AddProp(const std::string& name, int value);
    void AddProp(const std::string& name, unsigned int value);
    void AddProp(const std::string& name, long long value);
    void AddProp(const std::string& name, unsigned long long value);
    void AddProp(const std::string& name, float value);
    void AddProp(const std::string& name, double value);
    void AddProp(const std::string& name, bool value);

    void AddProp(const char* name, const std::string& value);
    void AddProp(const char* name, const char* value);
    void AddProp(const char* name, int value);
    void AddProp(const char* name, unsigned int value);
    void AddProp(const char* name, long long value);
    void AddProp(const char* name, unsigned long long value);
    void AddProp(const char* name, float value);
    void AddProp(const char* name, double value);
    void AddProp(const char* name, bool value);

    std::string GetPropAsStr(const std::string& name) const
    {
        for (const auto& tp : props_) {
            if (std::get<0>(tp) == name) {
                return std::get<1>(tp);
            }
        }
        return std::string();
    }

    std::string GetPropAsStr(const char* name) const
    {
        for (const auto& tp : props_) {
            if (std::get<0>(tp) == name) {
                return std::get<1>(tp);
            }
        }
        return std::string();
    }

    virtual bool IntersectWithBound(const Bound& bound) const = 0;

protected:
    GEO_TYPE geo_type_;
    std::vector<JsonProp> props_;

    friend class GeoJSON;
    friend class GeoJsonHelper;
};

typedef std::shared_ptr<GeoObj> GeoObjPtr;

class GeoObj_Point : public GeoObj
{
public:
    explicit GeoObj_Point()
        : GeoObj(GEOTYPE_POINT)
    {}
    explicit GeoObj_Point(double lat, double lng)
        : GeoObj(GEOTYPE_POINT), point_(lat, lng)
    {}
    explicit GeoObj_Point(const GeoPoint& point)
        : GeoObj(GEOTYPE_POINT), point_(point)
    {}

    void SetPoint(const GeoPoint& point)
    {
        point_ = point;
    }

    const GeoPoint& GetPoint() const
    {
        return point_;
    }

    GeoPoint& GetPoint()
    {
        return point_;
    }

    virtual bool IntersectWithBound(const Bound& bound) const
    {
        return bound.Within(point_);
    }

protected:
    GeoPoint point_;
    friend class GeoJsonHelper;
};

class GeoObj_MultiPoint : public GeoObj
{
public:
    explicit GeoObj_MultiPoint()
        : GeoObj(GEOTYPE_MULTIPOINT)
    {}
    explicit GeoObj_MultiPoint(const std::vector<GeoPoint>& points)
        : GeoObj(GEOTYPE_MULTIPOINT), points_(points)
    {}

    void AddPoint(const GeoPoint& point)
    {
        points_.push_back(point);
    }
    void AddPoint(double lat, double lng)
    {
        points_.push_back(GeoPoint(lat, lng));
    }

    virtual bool IntersectWithBound(const Bound& bound) const
    {
        for (auto& point : points_) {
            if (bound.Within(point)) {
                return true;
            }
        }
        return false;
    }

protected:
    std::vector<GeoPoint> points_;
    friend class GeoJsonHelper;
};

class GeoObj_LineString : public GeoObj
{
public:
    explicit GeoObj_LineString()
        : GeoObj(GEOTYPE_LINESTRING)
    {}
    explicit GeoObj_LineString(const std::vector<GeoPoint>& points)
        : GeoObj(GEOTYPE_LINESTRING), points_(points)
    {}

    bool FromCompressedGeometry(const std::string& compressed_geometry, int precision);

    void AddPoint(const GeoPoint& point)
    {
        points_.push_back(point);
    }
    void AddPoint(double lat, double lng)
    {
        points_.push_back(GeoPoint(lat, lng));
    }
    void AddPoint(GeoPoint&& point)
    {
        points_.emplace_back(std::forward<GeoPoint>(point));
    }

    const std::vector<GeoPoint>& GetPoints() const
    {
        return points_;
    }
    std::vector<GeoPoint>& GetPoints()
    {
        return points_;
    }

    double GetLength() const
    {
        double len = 0;
        for (size_t i = 0; i < this->points_.size() - 1; ++i) {
            double distance = distance_in_meter(points_[i], points_[i + 1]);
            len += distance;
        }
        return len;
    }

    bool FromWKT(const std::string& wkt);
    std::string ToWKT() const;

    void Clear()
    {
        points_.clear();
        GeoObj::props_.clear();
    }

    virtual bool IntersectWithBound(const Bound& bound) const
    {
        for (auto& point : points_) {
            if (bound.Within(point)) {
                return true;
            }
        }
        return false;
    }

protected:
    std::vector<GeoPoint> points_;
    friend class GeoJsonHelper;
};

class GeoObj_MultiLineString : public GeoObj
{
public:
    explicit GeoObj_MultiLineString()
        : GeoObj(GEOTYPE_MULTILINESTRING)
    {}
    explicit GeoObj_MultiLineString(const std::vector<GeoObj_LineString>& linestrings)
        : GeoObj(GEOTYPE_MULTILINESTRING), linestrings_(linestrings)
    {}

    virtual bool IntersectWithBound(const Bound& bound) const
    {
        for (auto& line : linestrings_) {
            if (line.IntersectWithBound(bound)) {
                return true;
            }
        }
        return false;
    }

protected:
    std::vector<GeoObj_LineString> linestrings_;
    friend class GeoJsonHelper;
};

class GeoObj_Polygon : public GeoObj
{
public:
    GeoObj_Polygon()
        : GeoObj(GEOTYPE_POLYGON)
    {}

    explicit GeoObj_Polygon(const std::string& wkt)
        : GeoObj(GEOTYPE_POLYGON)
    {
        FromWKT(wkt);
    }

    explicit GeoObj_Polygon(const Polygon& polygon)
        : GeoObj(GEOTYPE_POLYGON), polygon_(polygon)
    {}

    const Polygon& GetPolygon() const
    {
        return polygon_;
    }

    Polygon& GetPolygon()
    {
        return polygon_;
    }

    bool FromWKT(const std::string& wkt)
    {
        return wkt_to_polygon(wkt, polygon_);
    }

    virtual bool IntersectWithBound(const Bound& bound) const
    {
        for (auto& point : polygon_.outer_polygon.Vertexes()) {
            if (bound.Within(point)) {
                return true;
            }
        }
        return false;
    }

protected:
    Polygon polygon_;
    friend class GeoJsonHelper;
};

class GeoObj_MultiPolygon : public GeoObj
{
public:
    GeoObj_MultiPolygon()
        : GeoObj(GEOTYPE_MULTIPOLYGON)
    {}

    explicit GeoObj_MultiPolygon(const std::string& wkt)
        : GeoObj(GEOTYPE_MULTIPOLYGON)
    {
        FromWKT(wkt);
    }

    explicit GeoObj_MultiPolygon(const MultiPolygon& multi_polygon)
        : GeoObj(GEOTYPE_MULTIPOLYGON), multi_polygon_(multi_polygon)
    {}

    const MultiPolygon& GetMultiPolygon() const
    {
        return multi_polygon_;
    }

    MultiPolygon& GetMultiPolygon()
    {
        return multi_polygon_;
    }

    bool FromWKT(const std::string& wkt)
    {
        return wkt_to_multi_polygon(wkt, multi_polygon_);
    }

    virtual bool IntersectWithBound(const Bound& bound) const
    {
        for (auto& polygon : multi_polygon_.polygons) {
            for (auto& point : polygon.outer_polygon.Vertexes()) {
                if (bound.Within(point)) {
                    return true;
                }
            }
        }
        return false;
    }

protected:
    MultiPolygon multi_polygon_;
    friend class GeoJsonHelper;
};

class GeoJSON
{
public:
    explicit GeoJSON()
    {}

    void Reserve(size_t size)
    {
        geo_objs_.reserve(size);
    }

    void AddObj(const GeoObjPtr& p_obj)
    {
        geo_objs_.push_back(p_obj);
    }

    void AddObjs(const std::vector<GeoObjPtr>& objs)
    {
        for (auto& obj : objs) {
            geo_objs_.push_back(obj);
        }
    }

    std::string ToJsonString(bool pretty = false) const;
    bool ToJsonFile(const std::string& json_pathname, bool pretty = false) const;

protected:
    std::vector<GeoObjPtr> geo_objs_;

    friend class GeoJsonHelper;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

GEO_END_NAMESPACE

#endif // _GEO_UTILS_H
