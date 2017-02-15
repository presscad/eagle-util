
#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "geo_utils.h"
#include "common/common_utils.h"
#include <boost/geometry/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>
#include <boost/geometry/algorithms/covered_by.hpp>


namespace bg = boost::geometry;
typedef bg::model::d2::point_xy<double> Point2D;
typedef boost::geometry::model::polygon<Point2D> Polygon2D;
typedef bg::model::multi_polygon<Polygon2D> MultiPolygon2D;

typedef bg::model::point<double, 2, bg::cs::spherical_equatorial<bg::degree> > PointS;
typedef boost::geometry::model::polygon<PointS> PolygonS;
typedef bg::model::multi_polygon<PolygonS> MultiPolygonS;

GEO_BEGIN_NAMESPACE

class SimplePolygonHelper
{
public:
    explicit SimplePolygonHelper(const SimplePolygon& simple_polygon)
        : simple_polygon_(simple_polygon)
    {}

    static std::shared_ptr<SimplePolygonHelper> CheckCreateHelper(
        const SimplePolygon& simple_polygon)
    {
        if (simple_polygon.helper_ == NULL) {
            std::shared_ptr<SimplePolygonHelper> p_helper =
                std::make_shared<SimplePolygonHelper>(simple_polygon);

            std::string wkt = simple_polygon.WKT();
            bg::read_wkt(wkt, p_helper->polygon_2d_);
            bg::read_wkt(wkt, p_helper->polygon_s_);

            simple_polygon.helper_ = p_helper;
        }
        return simple_polygon.helper_;
    }

public:
    const SimplePolygon& simple_polygon_;
    Polygon2D polygon_2d_;
    PolygonS  polygon_s_;
};


class PolygonHelper
{
public:
    explicit PolygonHelper(const Polygon& polygon)
        : polygon_(polygon)
    {}

    static std::shared_ptr<PolygonHelper> CheckCreateHelper(
        const Polygon& polygon)
    {
        if (polygon.helper_ == NULL) {
            std::shared_ptr<PolygonHelper> p_helper = std::make_shared<PolygonHelper>(polygon);

            auto&& wkt = polygon.WKT();
            bg::read_wkt(wkt, p_helper->polygon_2d_);
            bg::read_wkt(wkt, p_helper->polygon_s_);

            polygon.helper_ = p_helper;
        }
        return polygon.helper_;
    }

public:
    const Polygon& polygon_;
    Polygon2D polygon_2d_;
    PolygonS  polygon_s_;
};


class MultiPolygonHelper
{
public:
    explicit MultiPolygonHelper(const MultiPolygon& multi_polygon)
        : multi_polygon_(multi_polygon)
    {}

    static std::shared_ptr<MultiPolygonHelper> CheckCreateHelper(const MultiPolygon& multi_polygon)
    {
        if (multi_polygon.helper_ == NULL) {
            std::shared_ptr<MultiPolygonHelper> p_helper =
                std::make_shared<MultiPolygonHelper>(multi_polygon);

            auto&& wkt = multi_polygon.WKT();
            bg::read_wkt(wkt, p_helper->multi_polygon_2d_);
            bg::read_wkt(wkt, p_helper->multi_polygon_s_);

            multi_polygon.helper_ = p_helper;
        }
        return multi_polygon.helper_;
    }

public:
    const MultiPolygon& multi_polygon_;
    MultiPolygon2D multi_polygon_2d_;
    MultiPolygonS  multi_polygon_s_;
};


bool point_in_polygon(const GeoPoint& point, const SimplePolygon& polygon)
{
    return point_in_polygon(point.lat, point.lng, polygon);
}

bool point_in_polygon(double lat, double lng, const SimplePolygon& polygon)
{
#define vertx(i)    (polygon.Vertex(i).lng)
#define verty(i)    (polygon.Vertex(i).lat)

    bool in = false;
    int nvert = (int)polygon.VertexNumber();
    const double& testx = lng;
    const double& testy = lat;

    int i, j;
    for (i = 0, j = nvert - 1; i < nvert; j = i++) {
        if (((verty(i) > testy) != (verty(j) > testy)) &&
            (testx < (vertx(j) - vertx(i)) * (testy - verty(i)) / (verty(j) - verty(i)) + vertx(i)))
        {
            in = !in;
        }
    }
    return in;

#undef vertx
#undef verty
}

bool point_in_polygons(const GeoPoint& point, const SimplePolygons& polygons)
{
    size_t npolygon = polygons.size();
    for (size_t i = 0; i < npolygon; ++i) {
        if (point_in_polygon(point, polygons[i])) {
            return true;
        }
    }
    return false;
}

bool point_in_polygons(double lat, double lng, const SimplePolygons& polygons)
{
    size_t npolygon = polygons.size();
    for (size_t i = 0; i < npolygon; ++i) {
        if (point_in_polygon(lat, lng, polygons[i])) {
            return true;
        }
    }
    return false;
}

bool point_in_polygon(double lat, double lng, const Polygon& poly)
{
    for (const auto& inner_poly : poly.inner_polygons) {
        if (point_in_polygon(lat, lng, inner_poly)) {
            return false;
        }
    }
    return point_in_polygon(lat, lng, poly.outer_polygon);
}

bool point_in_multi_polygon(double lat, double lng, const MultiPolygon& mpoly)
{
    for (const auto& poly : mpoly.polygons) {
        if (point_in_polygon(lat, lng, poly)) {
            return true;
        }
    }
    return false;
}


static bool get_simple_poly_text(const SimplePolygon& poly, std::string& text)
{
    text = "(";
    text.reserve(poly.VertexNumber() * 20);

    for (size_t i = 0; i < poly.VertexNumber(); ++i) {
        const GeoPoint& point = poly.Vertex(i);
        char buff[32];
        snprintf(buff, sizeof(buff) - 1, "%.6f %.6f", point.lng, point.lat);
        text += buff;
        if (i != poly.VertexNumber() - 1) {
            text += ',';
        }
    }

    if (geo::distance_in_meter(poly.Vertex(0), poly.Vertex(poly.VertexNumber() - 1)) > 1) {
        const GeoPoint& point = poly.Vertex(0);
        char buff[32];
        snprintf(buff, sizeof(buff) - 1, ",%.6f %.6f)", point.lng, point.lat);
        text += buff;
    }
    else {
        text += ')';
    }

    return true;
}

bool polygon_to_wkt(const SimplePolygon& polygon, std::string& wkt)
{
    const size_t size = polygon.vertexes.size();
    std::string text;
    if (size <= 1) {
        return false;
    }

    wkt.reserve(size * 20);
    wkt = "POLYGON(";
    get_simple_poly_text(polygon, text);
    wkt += text;
    wkt += ")";

    return true;
}

bool polygons_to_wkt(const SimplePolygons& polygons, std::string& wkt)
{
    const auto poly_count = polygons.size();
    if (poly_count == 0) {
        return false;
    }
    else if (poly_count == 1) {
        return polygon_to_wkt(polygons[0], wkt);
    }

    wkt = "MULTIPOLYGON(";

    for (size_t i = 0; i < polygons.size(); ++i) {
        const auto& poly = polygons[i];
        wkt += '(';

        std::string text;
        get_simple_poly_text(poly, text);
        text += ')';
        if (i != polygons.size() - 1) {
            text += ',';
        }

        wkt += text;
    }

    wkt += ')';
    return true;
}

bool polygon_to_wkt(const Polygon& polygon, std::string& wkt)
{
    wkt = "POLYGON(";
    std::string text;
    get_simple_poly_text(polygon.outer_polygon, text);
    wkt += text;
    // now wkt = "POLYGON((10 10,110 10,110 110,10 110)"

    if (!polygon.inner_polygons.empty()) {
        wkt += ',';
        // now wkt = "POLYGON((10 10,110 10,110 110,10 110),"

        for (const auto& inner_poly : polygon.inner_polygons) {
            get_simple_poly_text(inner_poly, text);
            wkt += text;
            // now wkt = "POLYGON((10 10,110 10,110 110,10 110),(20 20,20 30,30 30,30 20)
        }
    }

    wkt += ')';
    return true;
}

bool multi_polygon_to_wkt(const MultiPolygon& multipoly, std::string& wkt)
{
    const auto multi_count = multipoly.polygons.size();
    if (multi_count == 0) {
        wkt = "MULTIPOLYGON()";
        return false;
    }

    wkt = "MULTIPOLYGON(";
    std::string text;

    for (size_t i = 0; i < multipoly.polygons.size(); ++i) {
        const auto& poly = multipoly.polygons[i];
        wkt += '(';
        // for e.g., now wkt = "MULTIPOLYGON(("

        get_simple_poly_text(poly.outer_polygon, text);
        wkt += text;
        // now wkt = "MULTIPOLYGON(((10 10,110 10,110 110,10 110)"

        if (!poly.inner_polygons.empty()) {
            wkt += ',';
            // now wkt = "MULTIPOLYGON(((10 10,110 10,110 110,10 110),"

            for (const auto& inner_poly : poly.inner_polygons) {
                get_simple_poly_text(inner_poly, text);
                wkt += text;
                // now wkt = "MULTIPOLYGON(((10 10,110 10,110 110,10 110),(20 20,20 30,30 30,30 20)
            }
        }

        wkt += ')';
        if (i != multipoly.polygons.size() - 1) {
            wkt += ',';
        }
    }

    wkt += ')';
    return true;
}

static void remove_wkt_blanks(std::string& wkt)
{
    // remove spaces
    util::StringReplace(wkt, "\t", " ");
    int replaced = 0;
    while (true) {
        // avoid resize of string
        util::StringReplace(wkt, ", ", ",|");
        util::StringReplace(wkt, " ,", "|,");
        util::StringReplace(wkt, ") ", ")|");
        util::StringReplace(wkt, " )", "|)");
        util::StringReplace(wkt, "( ", "(|");
        util::StringReplace(wkt, " (", "|(");

        int replaced_new = 0;
        for (auto c : wkt) {
            if (c == '|') ++replaced_new;
        }
        if (replaced_new == replaced) {
            break;
        }
        replaced = replaced_new;
    }
    util::StringReplace(wkt, "|", "");
}

bool wkt_to_multi_polygon(const std::string& wkt, MultiPolygon& multipoly)
{
    std::string wkt_str = wkt;
    util::MakeUpper(wkt_str);
    remove_wkt_blanks(wkt_str);

    // remove "MULTIPOLYGON(" at begin, ")" at end 
    util::StringReplace(wkt_str, "MULTIPOLYGON(", "");
    if (wkt_str.empty()) {
        return false;
    }
    wkt_str.resize(wkt_str.size() - 1);

    // parse wkt
    multipoly.Clear();
    std::vector<std::string> subs;
    util::StringReplace(wkt_str, ")),((", "))|((");
    util::ParseCsvLine(subs, wkt_str, '|');
    if (subs.empty()) {
        return false;
    }

    for (size_t i = 0; i < subs.size(); ++i) {
        auto& sub_wkt = subs[i];
        if (sub_wkt.length() < 6) {
            return false;
        }
        if (sub_wkt[0] != '(' || sub_wkt[1] != '(' ||
            sub_wkt[sub_wkt.length() - 2] != ')' || sub_wkt[sub_wkt.length() - 1] != ')') {
            return false;
        }

        Polygon poly;
        sub_wkt.insert(0, "POLYGON");
        if (false == geo::wkt_to_polygon(sub_wkt, poly)) {
            return false;
        }
        multipoly.polygons.push_back(std::move(poly));
    }

    return true;
}


bool wkt_to_polygon(const std::string& wkt, Polygon& polygon)
{
    std::string content(wkt);
    util::MakeUpper(content);
    remove_wkt_blanks(content);

    size_t i1 = content.find("POLYGON(");
    if (std::string::npos == i1) {
        return false;
    }
    i1 += sizeof("POLYGON(") - 1;
    size_t i2 = content.rfind(")");
    if (std::string::npos == i2) {
        content.resize(i2);
    }
    else {
        content = content.substr(i1, i2 - i1);
    }

    // now, it is like "(35 10,45 45,15 40,10 20,35 10),(20 30,35 35,30 20,20 30)
    util::StringReplace(content, "),(", ")|(");
    std::vector<std::string> subs;
    util::ParseCsvLine(subs, content, '|');
    if (subs.empty()) {
        return false;
    }

    polygon.Clear();
    GeoPoint pt;
    for (size_t i = 0; i < subs.size(); ++i) {
        auto& sub = subs[i];
        SimplePolygon s_poly;

        std::vector<std::string> strs;
        strs.reserve(64);
        if (sub[0] == '(') {
            sub[0] = ' ';
        }
        util::ParseCsvLine(strs, subs[i], ',');
        for (auto& str : strs) {
            if (2 != sscanf(str.c_str(), "%lf %lf", &pt.lng, &pt.lat)) {
                return false;
            }
            s_poly.vertexes.push_back(pt);
        }

        if (i == 0) {
            polygon.outer_polygon = std::move(s_poly);
        }
        else {
            polygon.inner_polygons.push_back(std::move(s_poly));
        }
    }

    return true;
}

bool wkt_to_polygon(const std::string& wkt, SimplePolygon& polygon)
{
    // WKT simple polygon example: "POLYGON((118.846151 31.9825822,118.851213 31.980864, ...))"

    std::string content;
    size_t i1 = wkt.find("POLYGON((");
    if (std::string::npos == i1) {
        return false;
    }
    i1 += sizeof("POLYGON((") - 1;
    size_t i2 = wkt.rfind("))");
    if (std::string::npos == i2) {
        content = wkt.substr(i1);
    }
    else {
        content = wkt.substr(i1, i2 - i1);
    }
    util::StringReplace(content, "\\,", ", ");

    polygon.Clear();
    std::vector<std::string> subs;
    subs.reserve(64);
    util::ParseCsvLine(subs, content, ',');
    for (size_t i = 0; i < subs.size(); ++i) {
        GeoPoint pt;
        if (2 != sscanf(subs[i].c_str(), "%lf %lf", &pt.lng, &pt.lat)) {
            return false;
        }
        polygon.vertexes.push_back(pt);
    }
    return true;
}

double get_polygon_area(const SimplePolygon& polygon)
{
    const auto p_helper = SimplePolygonHelper::CheckCreateHelper(polygon);
    bg::strategy::area::huiller<PointS> sphere_earth(6371.004);
    double area = bg::area(p_helper->polygon_s_, sphere_earth);
    return area;
}

double get_polygons_area(const SimplePolygons& polygons)
{
    double area = 0;
    for (const auto& p : polygons) {
        area += get_polygon_area(p);
    }
    return area;
}

double get_polygon_area(const Polygon& poly)
{
    double area = get_polygon_area(poly.outer_polygon);
    for (const auto& p : poly.inner_polygons) {
        area -= get_polygon_area(p);
    }
    return area;
}

double get_multi_polygon_area(const MultiPolygon& mpoly)
{
    double area = 0;
    for (const auto& p : mpoly.polygons) {
        area += get_polygon_area(p);
    }
    return area;
}

// simply get the centroid: average of the vertexes' coordinates
bool get_polygon_centroid_simple(const SimplePolygon& polygon, double& lat, double& lng)
{
    lat = lng = 0;
    if (polygon.VertexNumber() == 0) {
        return false;
    }

    for (const auto& v : polygon.vertexes) {
        lat += v.lat;
        lng += v.lng;
    }

    lat /= polygon.VertexNumber();
    lng /= polygon.VertexNumber();
    return true;
}

GeoPoint get_polygon_centroid(const SimplePolygon& polygon)
{
    GeoPoint centroid(0, 0);
    double signedArea = 0.0;
    double x0; // Current vertex X
    double y0; // Current vertex Y
    double x1; // Next vertex X
    double y1; // Next vertex Y
    double a;  // Partial signed area

    // For all vertices except last
    int i = 0;
    const int n_vertex = (int)polygon.VertexNumber();
    for (i = 0; i < n_vertex - 1; ++i) {
        x0 = polygon.vertexes[i].lng;
        y0 = polygon.vertexes[i].lat;
        x1 = polygon.vertexes[i + 1].lng;
        y1 = polygon.vertexes[i + 1].lat;
        a = x0*y1 - x1*y0;
        signedArea += a;
        centroid.lng += (x0 + x1)*a;
        centroid.lat += (y0 + y1)*a;
    }

    // Do last vertex
    x0 = polygon.vertexes[i].lng;
    y0 = polygon.vertexes[i].lat;
    x1 = polygon.vertexes[0].lng;
    y1 = polygon.vertexes[0].lat;
    a = x0*y1 - x1*y0;
    signedArea += a;
    centroid.lng += (x0 + x1)*a;
    centroid.lat += (y0 + y1)*a;

    signedArea *= 0.5;
    centroid.lng /= (6.0*signedArea);
    centroid.lat /= (6.0*signedArea);

    return centroid;
}

bool get_polygon_centroid(const SimplePolygon& polygon, double& lat, double& lng)
{
    GeoPoint pt = get_polygon_centroid(polygon);
    lat = pt.lat;
    lng = pt.lng;
    return true;
}


bool polygon_covered_by(const SimplePolygon& polygon, double lat, double lng)
{
    const auto p_helper = SimplePolygonHelper::CheckCreateHelper(polygon);
    Point2D pt(lat, lng);
    return bg::covered_by(pt, p_helper->polygon_2d_);
}

bool polygon_covered_by(const Polygon& polygon, double lat, double lng)
{
    const auto p_helper = PolygonHelper::CheckCreateHelper(polygon);
    Point2D pt(lat, lng);
    return bg::covered_by(pt, p_helper->polygon_2d_);
}

bool multi_polygon_covered_by(const MultiPolygon& mpoly, double lat, double lng)
{
    const auto p_helper = MultiPolygonHelper::CheckCreateHelper(mpoly);
    Point2D pt(lng, lat);
    return bg::covered_by(pt, p_helper->multi_polygon_2d_);
}

GEO_END_NAMESPACE
