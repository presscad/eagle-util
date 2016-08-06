
#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "geo_utils.h"
#include <string>
#include <fstream>
#include "common/common_utils.h"
#include "common/rapidjson_helper.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"


GEO_BEGIN_NAMESPACE

typedef GeoObj::JsonProp JsonProp;

enum JsonType {
    JSON_NULL_TYPE,
    JSON_BOOL_TYPE,
    JSON_INT_TYPE,
    JSON_REAL_TYPE,
    JSON_STR_TYPE,
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// class GeoJsonHelper

class GeoJsonHelper
{
public:
    static double ToPrecision6(double v)
    {
        return static_cast<double>(static_cast<long>(v * 1000000.0 + 0.5)) / 1000000.0;
    }

    template <typename Writer>
    static bool AppendJsonCoordinates(Writer& writer, const std::vector<GeoPoint>& points)
    {
        bool ok = writer.StartArray();
        if (ok) {
            for (auto& point : points) {
                if (ok) {
                    ok = writer.StartArray() &&
                        writer.Double(ToPrecision6(point.lng)) &&
                        writer.Double(ToPrecision6(point.lat)) &&
                        writer.EndArray();
                }
                if (!ok) break;
            }
        }
        if (ok) ok = writer.EndArray();
        return ok;
    }

    template <typename Writer>
    static bool AppendJsonProperties(Writer& writer, const std::vector<JsonProp>& props)
    {
        bool ok = writer.Key("properties") &&
            writer.StartObject();
        if (!ok) return false;

        for (auto& prop : props) {
            auto& k = std::get<0>(prop);
            auto& v = std::get<1>(prop);

            switch (std::get<2>(prop)) {
            case JSON_BOOL_TYPE:
                ok = util::WritePairBool(writer, k, (v == "1"));
                break;
            case JSON_INT_TYPE:
                ok = (v[0] == '-') ?
                    util::WritePairInt64(writer, k, static_cast<int64_t>(std::stoll(v))) :
                    util::WritePairUint64(writer, k, static_cast<uint64_t>(std::stoull(v)));
                break;
            case JSON_REAL_TYPE:
                ok = util::WritePairDouble(writer, k, std::stod(v));
                break;
            default:
                ok = util::WritePairString(writer, k, v);
                break;
            }

            if (!ok) break;
        }

        if (ok) ok = writer.EndObject();
        return ok;
    }

    template <typename Writer>
    static bool AppendJsonValue(Writer& writer, const GeoObj_Point& obj)
    {
        bool ok = writer.StartObject();
        if (ok) {
            ok = util::WritePairString(writer, "type", "Feature");
            if (ok) {
                ok = writer.Key("geometry");
                if (ok) ok = writer.StartObject();
                if (ok) {
                    ok = util::WritePairString(writer, "type", "Point") &&
                        writer.Key("coordinates");
                    if (ok) {
                        ok = writer.StartArray() &&
                            writer.Double(ToPrecision6(obj.point_.lng)) &&
                            writer.Double(ToPrecision6(obj.point_.lat)) &&
                            writer.EndArray();
                    }
                }
                if (ok) ok = writer.EndObject();
            }
            if (ok && !obj.props_.empty()) {
                ok = GeoJsonHelper::AppendJsonProperties(writer, obj.props_);
            }
        }
        if (ok) ok = writer.EndObject();
        return ok;
    }

    template <typename Writer>
    static bool AppendJsonValue(Writer& writer, const GeoObj_MultiPoint& obj)
    {
        bool ok = writer.StartObject();
        if (ok) {
            ok = util::WritePairString(writer, "type", "Feature");
            if (ok) {
                ok = writer.Key("geometry");
                if (ok) ok = writer.StartObject();
                if (ok) {
                    ok = util::WritePairString(writer, "type", "MultiPoint") &&
                        writer.Key("coordinates");
                    if (ok) {
                        ok = GeoJsonHelper::AppendJsonCoordinates(writer, obj.points_);
                    }
                }
                if (ok) ok = writer.EndObject();
            }
            if (ok && !obj.props_.empty()) {
                ok = GeoJsonHelper::AppendJsonProperties(writer, obj.props_);
            }
        }
        if (ok) ok = writer.EndObject();
        return ok;
    }

    template <typename Writer>
    static bool AppendJsonValue(Writer& writer, const GeoObj_LineString& obj)
    {
        bool ok = writer.StartObject();
        if (ok) {
            ok = util::WritePairString(writer, "type", "Feature");
            if (ok) {
                ok = writer.Key("geometry");
                if (ok) ok = writer.StartObject();
                if (ok) {
                    ok = util::WritePairString(writer, "type", "LineString") &&
                        writer.Key("coordinates");
                    if (ok) {
                        ok = GeoJsonHelper::AppendJsonCoordinates(writer, obj.points_);
                    }
                }
                if (ok) ok = writer.EndObject();
            }
            if (ok && !obj.props_.empty()) {
                ok = GeoJsonHelper::AppendJsonProperties(writer, obj.props_);
            }
        }
        if (ok) ok = writer.EndObject();
        return ok;
    }

    template <typename Writer>
    static bool AppendJsonValue(Writer& writer, const GeoObj_MultiLineString& obj)
    {
        bool ok = writer.StartObject();
        if (ok) {
            ok = util::WritePairString(writer, "type", "Feature");
            if (ok) {
                ok = writer.Key("geometry");
                if (ok) ok = writer.StartObject();
                if (ok) {
                    ok = util::WritePairString(writer, "type", "MultiLineString");
                    if (ok) ok = writer.Key("coordinates");
                    if (ok) {
                        ok = writer.StartArray();
                        if (ok) {
                            for (auto& linestring : obj.linestrings_) {
                                ok = GeoJsonHelper::AppendJsonCoordinates(writer, linestring.points_);
                                if (!ok) break;
                            }
                        }
                        if (ok) ok = writer.EndArray();
                    }
                }
                if (ok) ok = writer.EndObject();
            }
            if (ok && !obj.props_.empty()) {
                ok = GeoJsonHelper::AppendJsonProperties(writer, obj.props_);
            }
        }
        if (ok) ok = writer.EndObject();
        return ok;
    }

    template <typename Writer>
    static bool AppendJsonValue(Writer& writer, const GeoObj_Polygon& obj)
    {
        bool ok = writer.StartObject();
        if (ok) {
            ok = util::WritePairString(writer, "type", "Feature");
            if (ok) {
                ok = writer.Key("geometry");
                if (ok) ok = writer.StartObject();
                if (ok) {
                    ok = util::WritePairString(writer, "type", "Polygon");
                    if (ok) ok = writer.Key("coordinates");
                    if (ok) {
                        ok = writer.StartArray();
                        if (ok) {
                            ok = GeoJsonHelper::AppendJsonCoordinates(writer,
                                obj.polygon_.outer_polygon.vertexes);
                        }
                        if (ok) {
                            for (auto& inner_poly : obj.polygon_.inner_polygons) {
                                ok = GeoJsonHelper::AppendJsonCoordinates(writer, inner_poly.vertexes);
                                if (!ok) break;
                            }
                        }
                        if (ok) ok = writer.EndArray();
                    }
                }
                if (ok) ok = writer.EndObject();
            }
            if (ok && !obj.props_.empty()) {
                ok = GeoJsonHelper::AppendJsonProperties(writer, obj.props_);
            }
        }
        if (ok) ok = writer.EndObject();
        return ok;
    }

    template <typename Writer>
    static bool AppendJsonValue(Writer& writer, const GeoObj& obj)
    {
        switch (obj.GetObjType()) {
        case GEOTYPE_POINT:
            return AppendJsonValue(writer, static_cast<const GeoObj_Point&>(obj));
        case GEOTYPE_MULTIPOINT:
            return AppendJsonValue(writer, static_cast<const GeoObj_MultiPoint&>(obj));
        case GEOTYPE_LINESTRING:
            return AppendJsonValue(writer, static_cast<const GeoObj_LineString&>(obj));
        case GEOTYPE_MULTILINESTRING:
            return AppendJsonValue(writer, static_cast<const GeoObj_MultiLineString&>(obj));
        case GEOTYPE_POLYGON:
            return AppendJsonValue(writer, static_cast<const GeoObj_Polygon&>(obj));
        case GEOTYPE_MULTIPOLYGON:
            // TODO:
        case GEOTYPE_GEOMETRYCOLLECTION:
            // TODO:
            break;
        default:
            break;
        }

        bool ok = writer.StartObject();
        if (ok) {
            ok = writer.Key("TODO") &&
                writer.Key("to impletment") &&
                writer.EndObject();
        }
        return ok;
    }

    template <typename Writer>
    static bool ConstructJson(Writer& writer, const GeoJSON& geo_json)
    {
        bool ok = writer.StartObject();
        if (ok) {
            ok = writer.Key("type") &&
                writer.String("FeatureCollection");
        }
        if (ok) {
            ok = writer.Key("features") &&
                writer.StartArray();
            if (ok) {
                for (auto& p_obj : geo_json.geo_objs_) {
                    ok = GeoJsonHelper::AppendJsonValue(writer, *p_obj);
                    if (!ok) break;
                }
            }
            if (ok) ok = writer.EndArray();
        }
        if (ok) ok = writer.EndObject();
        return ok;
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// class GeoObj

void GeoObj::AddProp(const std::string& name, const std::string& value)
{
    props_.push_back(std::make_tuple(name, value, JSON_STR_TYPE));
}

void GeoObj::AddProp(const std::string& name, const char* value)
{
    props_.push_back(std::make_tuple(name, std::string(value), JSON_STR_TYPE));
}

void GeoObj::AddProp(const std::string& name, int value)
{
    props_.push_back(std::make_tuple(name, std::to_string(value), JSON_INT_TYPE));
}

void GeoObj::AddProp(const std::string& name, unsigned int value)
{
    props_.push_back(std::make_tuple(name, std::to_string(value), JSON_INT_TYPE));
}

void GeoObj::AddProp(const std::string& name, long long value)
{
    props_.push_back(std::make_tuple(name, std::to_string(value), JSON_INT_TYPE));
}

void GeoObj::AddProp(const std::string& name, unsigned long long value)
{
    props_.push_back(std::make_tuple(name, std::to_string(value), JSON_INT_TYPE));
}

void GeoObj::AddProp(const std::string& name, float value)
{
    props_.push_back(std::make_tuple(name, std::to_string(value), JSON_REAL_TYPE));
}

void GeoObj::AddProp(const std::string& name, double value)
{
    props_.push_back(std::make_tuple(name, std::to_string(value), JSON_REAL_TYPE));
}

void GeoObj::AddProp(const std::string& name, bool value)
{
    props_.push_back(std::make_tuple(name, std::to_string(value), JSON_BOOL_TYPE));
}

void GeoObj::AddProp(const char* name, const std::string& value)
{
    props_.push_back(std::make_tuple(name, value, JSON_STR_TYPE));
}

void GeoObj::AddProp(const char* name, const char* value)
{
    props_.push_back(std::make_tuple(name, std::string(value), JSON_STR_TYPE));
}

void GeoObj::AddProp(const char* name, int value)
{
    props_.push_back(std::make_tuple(name, std::to_string(value), JSON_INT_TYPE));
}

void GeoObj::AddProp(const char* name, unsigned int value)
{
    props_.push_back(std::make_tuple(name, std::to_string(value), JSON_INT_TYPE));
}

void GeoObj::AddProp(const char* name, long long value)
{
    props_.push_back(std::make_tuple(name, std::to_string(value), JSON_INT_TYPE));
}

void GeoObj::AddProp(const char* name, unsigned long long value)
{
    props_.push_back(std::make_tuple(name, std::to_string(value), JSON_INT_TYPE));
}

void GeoObj::AddProp(const char* name, float value)
{
    props_.push_back(std::make_tuple(name, std::to_string(value), JSON_REAL_TYPE));
}

void GeoObj::AddProp(const char* name, double value)
{
    props_.push_back(std::make_tuple(name, std::to_string(value), JSON_REAL_TYPE));
}

void GeoObj::AddProp(const char* name, bool value)
{
    props_.push_back(std::make_tuple(name, std::to_string(value), JSON_BOOL_TYPE));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// class GeoObj_LineString

// decode compressed route geometry.
// see https://github.com/DennisSchiefer/Project-OSRM-Web/blob/develop/WebContent/routing/OSRM.RoutingGeometry.js
bool GeoObj_LineString::FromCompressedGeometry(const std::string& encoded, int i_precision)
{
    const double precision = ::pow(10, -i_precision);
    int len = (int)encoded.length();
    int index = 0;
    int lat = 0, lng = 0;
    this->points_.clear();

    while (index < len) {
        int b, shift = 0, result = 0;
        do {
            b = encoded[index++] - 63;
            result |= (b & 0x1f) << shift;
            shift += 5;
        } while (b >= 0x20);
        int dlat = ((result & 1) ? ~(result >> 1) : (result >> 1));
        lat += dlat;

        shift = 0;
        result = 0;
        do {
            b = encoded[index++] - 63;
            result |= (b & 0x1f) << shift;
            shift += 5;
        } while (b >= 0x20);
        int dlng = ((result & 1) ? ~(result >> 1) : (result >> 1));
        lng += dlng;

        this->points_.push_back(GeoPoint(lat * precision, lng * precision));
    }
    return true;
}

bool GeoObj_LineString::FromWKT(const std::string& wkt)
{
    const std::string prefix("LINESTRING(");
    auto i1 = wkt.find(prefix);
    if (11 == std::string::npos) {
        return false;
    }
    i1 += prefix.length();
    auto i2 = wkt.find(")", i1);
    if (12 == std::string::npos) {
        return false;
    }
    auto content = wkt.substr(i1, i2 - i1);
    util::StringReplace(content, "\\,", ", ");

    this->points_.clear();
    std::vector<char*> subs;
    subs.reserve(64);
    util::ParseCsvLineInPlace(subs, &content[0], ',');

    for (auto& sub : subs) {
        double lat, lng;
        if (2 != sscanf(sub, "%lf %lf", &lng, &lat)) {
            return false;
        }
        this->points_.emplace_back(geo::GeoPoint(lat, lng));
    }
    return true;
}

std::string GeoObj_LineString::ToWKT() const
{
    std::string wkt("LINESTRING(");
    wkt.reserve(11 + 22 * this->points_.size() + 2);
    for (size_t i = 0; i < this->points_.size(); ++i) {
        char buff[128];
        snprintf(buff, sizeof(buff), "%.6f %.6f", points_[i].lng, points_[i].lat);
        wkt += buff;
        if (i < this->points_.size() - 1) {
            wkt += ',';
        }
    }

    wkt += ')';
    return wkt;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// class GeoJSON

std::string GeoJSON::ToJsonString(bool pretty/* = false*/) const
{
    bool ok;
    rapidjson::StringBuffer buffer;
    if (pretty) {
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        ok = GeoJsonHelper::ConstructJson(writer, *this);
    }
    else {
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        ok = GeoJsonHelper::ConstructJson(writer, *this);
    }
    return ok ? buffer.GetString() : std::string();
}

bool GeoJSON::ToJsonFile(const std::string& json_pathname, bool pretty/* = false*/) const
{
    bool ok;
    rapidjson::StringBuffer buffer;

    if (pretty) {
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        ok = GeoJsonHelper::ConstructJson(writer, *this);
    }
    else {
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        ok = GeoJsonHelper::ConstructJson(writer, *this);
    }
    if (!ok) return false;

    std::ofstream out(json_pathname.c_str());
    if (!out.good()) {
        return false;
    }
    out << buffer.GetString();
    return true;
}

GEO_END_NAMESPACE
