
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

// local threaded context
struct LocalContext
{
    ShpData* p_shp_data{};
    ShpParseParams* p_params{};
    OGRSpatialReference wgs;

    OGRCoordinateTransformation* p_coord_trans{};
    vector<geo::GeoPoint> geo_points_buff;
    vector<OGRRawPoint> ogr_raw_points_buff;

    ~LocalContext()
    {
        if (p_coord_trans) {
            delete p_coord_trans;
            p_coord_trans = nullptr;
        }
        p_shp_data = nullptr;
        p_params = nullptr;
    }
};

static void ConvertPoint(LocalContext& ctx, geo::GeoPoint& point)
{
    if (ctx.p_params->coordinates_to_wgs84 && ctx.p_coord_trans != nullptr) {
        ctx.p_coord_trans->Transform(1, &point.lng, &point.lat);
    }

    if (ctx.p_params->lat_offset != 0) {
        point.lat += ctx.p_params->lat_offset;
    }
    if (ctx.p_params->lng_offset != 0) {
        point.lng += ctx.p_params->lng_offset;
    }
}

static bool ConvertFeature(LocalContext& ctx, OGRLayer* poLayer, OGRFeature *poFeature,
    ShpFeature& feature)
{
    feature.fid = poFeature->GetFID();

    OGRGeometry *poGeometry = poFeature->GetGeometryRef();
    if (poGeometry == nullptr) {
        ctx.p_params->err_str = "Failed calling OGRFeature::GetGeometryRef()";
        return false;
    }

    switch (wkbFlatten(poGeometry->getGeometryType())) {
    case wkbPoint:
    {
        const OGRPoint& oPoint = *(OGRPoint *)poGeometry;
        geo::GeoPoint point(oPoint.getY(), oPoint.getX());
        ConvertPoint(ctx, point);
        feature.p_geo_obj = make_shared<geo::GeoObj_Point>(point);
        break;
    }
    case wkbLineString:
    {
        vector<OGRRawPoint>& ogr_raw_points = ctx.ogr_raw_points_buff;
        vector<geo::GeoPoint>& geo_points = ctx.geo_points_buff;

        OGRLineString *poLineString = (OGRLineString *)poGeometry;
        ogr_raw_points.resize(poLineString->getNumPoints());
        poLineString->getPoints(ogr_raw_points.data());

        geo_points.clear();

        for (const auto& raw_pt : ogr_raw_points) {
            geo::GeoPoint new_pt(raw_pt.y, raw_pt.x);
            ConvertPoint(ctx, new_pt);
            if (geo_points.empty() || geo_points.back() != new_pt) {
                geo_points.push_back(move(new_pt));
            }
        }
        if (geo_points.size() > 1) {
            feature.p_geo_obj = make_shared<geo::GeoObj_LineString>(geo_points);
        }
        break;
    }
    case wkbPolygon:
    {
        const OGRPolygon& ogr_poly = *(OGRPolygon *)poGeometry;
        char *p_wkt = nullptr;
        if (OGRERR_NONE != ogr_poly.exportToWkt(&p_wkt)) {
            ctx.p_params->err_str = "Error in calling OGRPolygon::exportToWkt()";
            return false;
        }

        auto p_polygon = make_shared<geo::GeoObj_Polygon>();
        bool ok = p_polygon->FromWKT(p_wkt);
        if (!ok) {
            ctx.p_params->err_str = "Error in calling GeoObj_Polygon::FromWKT, wkt = " + string(p_wkt);
            OGRFree(p_wkt);
            return false;
        }
        OGRFree(p_wkt);
        p_wkt = nullptr;

        geo::Polygon& poly = p_polygon->GetPolygon();
        for (auto& point : poly.outer_polygon.vertexes) {
            ConvertPoint(ctx, point);
        }
        for (auto& inner_poly : poly.inner_polygons) {
            for (auto& point : inner_poly.vertexes) {
                ConvertPoint(ctx, point);
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
        ctx.p_params->err_str = "unknown geometry type: " + to_string(poGeometry->getGeometryType())
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
    feature.p_geo_obj->AddProp("_LayerName", poLayer->GetName());
    return true;
}

static bool InitLocalContext(LocalContext& ctx, ShpData &shp_data,
    ShpParseParams& parse_cfgs)
{
    ctx.p_shp_data = &shp_data;
    ctx.p_params = &parse_cfgs;

    if (ctx.p_params->coordinates_to_wgs84) {
        ctx.wgs.SetWellKnownGeogCS("WGS84");
    }

    return true;
}

static void OnNewLayer(LocalContext& ctx, OGRLayer* p_layer)
{
    if (ctx.p_coord_trans) {
        delete ctx.p_coord_trans;
        ctx.p_coord_trans = nullptr;
    }

    OGRSpatialReference *p_osrs = p_layer->GetSpatialRef();
    if (p_osrs) {
        ctx.p_coord_trans = OGRCreateCoordinateTransformation(p_osrs, &ctx.wgs);
    }
}

bool parse_shp(const std::string& filename, ShpParseParams& parse_cfgs, ShpData& shp_data)
{
    OGRFeature *poFeature = nullptr;
    GDALDataset* poDS = nullptr;
    LocalContext ctx;

    parse_cfgs.err_str.clear();
    shp_data.Clear();
    shp_data.pathname = filename;

    AT_SCOPE_EXIT(
        if (poFeature) OGRFeature::DestroyFeature(poFeature);
        if (poDS) GDALClose(poDS);
        if (!parse_cfgs.err_str.empty()) shp_data.Clear();
    );

    poDS = (GDALDataset*)GDALOpenEx(filename.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr);
    if (poDS == nullptr) {
        parse_cfgs.err_str = "Open failed : " + filename;
        return false;
    }
    if (false == InitLocalContext(ctx, shp_data, parse_cfgs)) {
        return false;
    }

    int n_layer = poDS->GetLayerCount();
    for (int i_layer = 0; i_layer < n_layer; ++i_layer) {
        OGRLayer* poLayer = poDS->GetLayer(i_layer);
        if (poLayer == nullptr) {
            parse_cfgs.err_str = "Cannot get the layer index " + to_string(i_layer);
            return false;
        }
        OnNewLayer(ctx, poLayer);
        string layer_name = poLayer->GetName();

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
            poFeature = nullptr;
        }
    }

    return true;
}

bool parse_shp(const string &filename, double lat_offset, double lng_offset,
    ShpData &shp_data, string &err_str)
{
    ShpParseParams parse_cfgs;
    parse_cfgs.lat_offset = lat_offset;
    parse_cfgs.lng_offset = lng_offset;
    bool r = parse_shp(filename, parse_cfgs, shp_data);
    if (false == r) {
        err_str = parse_cfgs.err_str;
    }
    return r;
}

}
