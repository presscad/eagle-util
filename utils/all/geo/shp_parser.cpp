
#include "shp_parser.h"
#include <string>
#include "common/common_utils.h"
#include "common/at_scope_exit.h"
#include "gdal.h"
#include "gdal_priv.h"
#include "ogr_feature.h"
#include "ogrsf_frmts.h"


using namespace std;

namespace shp {

static std::string GetLayerName(const std::string &filename)
{
    auto i = filename.rfind('/');
    if (i == std::string::npos) {
        i = filename.rfind('\\');
    }

    auto name = filename.substr(i + 1);
    return name.substr(0, name.find('.'));
}

// local threaded context
struct LocalContext
{
    ShpData* p_shp_data{};
    std::string* p_err_str{};
    double lat_offset{}, lng_offset{};

    std::vector<geo::GeoPoint> geo_points_buff;
    std::vector<OGRRawPoint> ogr_raw_points_buff;
};

static bool ConvertFeature(LocalContext& ctx, OGRLayer* poLayer, OGRFeature *poFeature,
    ShpFeature& feature)
{
    feature.fid = poFeature->GetFID();

    OGRGeometry *poGeometry = poFeature->GetGeometryRef();
    if (poGeometry == nullptr) {
        *ctx.p_err_str = "Failed calling OGRFeature::GetGeometryRef()";
        return false;
    }

    switch (wkbFlatten(poGeometry->getGeometryType())) {
    case wkbPoint:
    {
        *(int*)0 = 1; // to crash for adding new code
        break;
    }
    case wkbLineString:
    {
        std::vector<OGRRawPoint>& ogr_raw_points = ctx.ogr_raw_points_buff;
        std::vector<geo::GeoPoint>& geo_points = ctx.geo_points_buff;

        OGRLineString *poLineString = (OGRLineString *)poGeometry;
        ogr_raw_points.resize(poLineString->getNumPoints());
        poLineString->getPoints(ogr_raw_points.data());

        geo_points.clear();

        for (const auto& raw_pt : ogr_raw_points) {
            geo::GeoPoint new_pt(raw_pt.y + ctx.lat_offset, raw_pt.x + ctx.lng_offset);
            if (geo_points.empty() || geo_points.back() != new_pt) {
                geo_points.push_back(std::move(new_pt));
            }
        }
        if (geo_points.size() > 1) {
            feature.p_geo_obj = std::make_shared<geo::GeoObj_LineString>(geo_points);
        }
        break;
    }
    case wkbPolygon:
    {
        const OGRPolygon& ogr_poly = *(OGRPolygon *)poGeometry;
        char *p_wkt = nullptr;
        if (OGRERR_NONE != ogr_poly.exportToWkt(&p_wkt)) {
            *ctx.p_err_str = "Error in calling OGRPolygon::exportToWkt()";
            return false;
        }

        auto p_polygon = make_shared<geo::GeoObj_Polygon>();
        bool ok = p_polygon->FromWKT(p_wkt);
        if (!ok) {
            *ctx.p_err_str = "Error in calling GeoObj_Polygon::FromWKT, wkt = " + string(p_wkt);
            OGRFree(p_wkt);
            return false;
        }
        OGRFree(p_wkt);

        if (ctx.lat_offset != 0 && ctx.lng_offset != 0) {
            geo::Polygon& poly = p_polygon->GetPolygon();
            for (auto& point : poly.outer_polygon.vertexes) {
                point.lat += ctx.lat_offset;
                point.lng += ctx.lng_offset;
            }
            for (auto& inner_poly : poly.inner_polygons) {
                for (auto& point : inner_poly.vertexes) {
                    point.lat += ctx.lat_offset;
                    point.lng += ctx.lng_offset;
                }
            }
        }

        feature.p_geo_obj = p_polygon;
        break;
    }

    case wkbMultiPoint:
    case wkbMultiLineString:
    case wkbMultiPolygon:
    {
        *(int*)0 = 1; // to crash for adding new code
        break;
    }
    // add more case branches above if needed

    default:
        *ctx.p_err_str = "unknown geometry type: " + std::to_string(poGeometry->getGeometryType())
            + ", fix it in " + __FUNCTION__ + "()";
        return false;
    }

    feature.p_geo_obj->AddProp("FID", feature.fid);

    OGRFeatureDefn *poFDefn = poLayer->GetLayerDefn();
    for (int iField = 0; iField < poFDefn->GetFieldCount(); iField++) {
        OGRFieldDefn *poFieldDefn = poFDefn->GetFieldDefn(iField);

        auto field_name = poFieldDefn->GetNameRef();
        switch (poFieldDefn->GetType()) {
        case OFTInteger:
            feature.p_geo_obj->AddProp(field_name, poFeature->GetFieldAsInteger(iField));
            break;
        case OFTInteger64:
            feature.p_geo_obj->AddProp(field_name, poFeature->GetFieldAsInteger64(iField));
            break;
        case OFTReal:
            feature.p_geo_obj->AddProp(field_name, poFeature->GetFieldAsDouble(iField));
            break;

        // add more case branches above if needed
        default:
            feature.p_geo_obj->AddProp(field_name, poFeature->GetFieldAsString(iField));
            break;
        }
    }

    return true;
}

static bool InitLocalContext(LocalContext& ctx, ShpData &shp_data,
    double lat_offset, double lng_offset, std::string &err_str)
{
    ctx.p_shp_data = &shp_data;
    ctx.lat_offset = lat_offset;
    ctx.lng_offset = lng_offset;
    ctx.p_err_str = &err_str;

    return true;
}

bool parse_shp(const std::string &filename, double lat_offset, double lng_offset,
    ShpData &shp_data, std::string &err_str)
{
    std::string layer_name;
    OGRFeature *poFeature = nullptr;
    GDALDataset* poDS = nullptr;
    LocalContext ctx;

    err_str.clear();
    AT_SCOPE_EXIT(
        if (poFeature) OGRFeature::DestroyFeature(poFeature);
        if (poDS) GDALClose(poDS);
        if (!err_str.empty()) shp_data.Clear();
    );

    poDS = (GDALDataset*)GDALOpenEx(filename.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr);
    if (poDS == nullptr) {
        err_str = "Open failed : " + filename;
        return false;
    }

    if (false == InitLocalContext(ctx, shp_data, lat_offset, lng_offset, err_str)) {
        return false;
    }

    layer_name = GetLayerName(filename);
    OGRLayer* poLayer = poDS->GetLayerByName(layer_name.c_str());
    if (poLayer == nullptr) {
        err_str = "Cannot get the layer " + layer_name;
        return false;
    }

    shp_data.Clear();
    shp_data.pathname = filename;
    shp_data.layers.push_back(ShpLayer());
    auto& layer = shp_data.layers.front();
    layer.Clear();
    layer.layer_name = layer_name;

    poLayer->ResetReading();

    // for the fields of the layer
    while ((poFeature = poLayer->GetNextFeature()) != nullptr) {
        ShpFeature feature;
        if (false == ConvertFeature(ctx, poLayer, poFeature, feature)) {
            return false;
        }
        if (feature.IsValid()) {
            layer.features.push_back(feature);
        }

        OGRFeature::DestroyFeature(poFeature);
    }

    return true;
}

}
