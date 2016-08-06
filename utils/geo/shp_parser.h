#ifndef _SHP_PARSER_H
#define _SHP_PARSER_H

#include <string>
#include <vector>
#include <memory>
#include "geo_utils.h"


namespace shp {


struct ShpFeature
{
    long long      fid{-1};     // feature ID
    geo::GeoObjPtr p_geo_obj;

    bool IsValid() const
    {
        return (fid >= 0) && (p_geo_obj != nullptr);
    }

    std::string GetPropAsStr(const std::string& prop_name) const
    {
        return p_geo_obj->GetPropAsStr(prop_name);
    }

    std::string GetPropAsStr(const char* prop_name) const
    {
        return p_geo_obj->GetPropAsStr(prop_name);
    }
};

struct ShpLayer
{
    std::string             layer_name;
    std::vector<ShpFeature> features;

    void Clear()
    {
        layer_name.clear();
        features.clear();
    }
};

struct ShpData
{
    std::string             pathname;
    std::vector<ShpLayer>   layers;

    void Clear()
    {
        pathname.clear();
        for (auto& l : layers) {
            l.Clear();
        }
    }
};

bool parse_shp(const std::string &filename, double lat_offset, double lng_offset,
    ShpData &shp_data, std::string &err_str);

}

#endif // _SHP_PARSER_H
