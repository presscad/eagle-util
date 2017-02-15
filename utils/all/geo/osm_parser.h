
#ifndef _OSM_PARSER_H
#define _OSM_PARSER_H

#include <string>
#include <vector>
#include <fstream>
#include <memory>
#ifdef _WIN32
#include <unordered_map>
#else
#include <boost/unordered_map.hpp>
#endif
#include "geo_utils.h"

//#pragma optimize("", off)

namespace osm {

typedef struct Bounds
{
    std::string minlat;
    std::string minlon;
    std::string maxlat;
    std::string maxlon;

    void Clear()
    {
        *this = Bounds();
    }
} Bounds;

#ifdef _WIN32
typedef std::unordered_map<std::string, std::string>  TagMap;
#else
typedef boost::unordered_map<std::string, std::string>  TagMap;
#endif

// E.g.,
// <node id="29055831" version="21" timestamp="2013-08-01T07:05:36Z" uid="386131" user="zhongguo" changeset="7097408" lat="31.706849" lon="119.0231171">
//    <tag k="barrier" v="toll_booth"/>
// </node>
typedef struct Node
{
    std::string     id;
    std::string     version;
    std::string     timestamp;
    std::string     uid;
    std::string     user;
    std::string     changeset;
    double          lat;
    double          lon;
    TagMap          tag_map;
} Node;

typedef std::shared_ptr<Node> NodePtr;
typedef std::vector<NodePtr> NodePtrVec;


// E.g.,
// <way id="118932337" version="1" timestamp="2013-08-01T07:05:36Z" uid="253683" user="sinopitt" changeset="8533092"> ...
typedef struct Way
{
    std::string     id;
    std::string     version;
    std::string     timestamp;
    std::string     uid;
    std::string     user;
    std::string     changeset;
    std::vector<std::string>    node_refs;
    TagMap          tag_map;

    // Wwo OSM ways with different IDs but same node refs ...
    std::string     duplicate_to_id;
} Way;

typedef std::shared_ptr<Way> WayPtr;
typedef std::vector<WayPtr> WayPtrVec;


typedef struct RelationMember
{
    std::string     type;
    std::string     ref;
    std::string     role;
} RelationMember;

typedef struct Relation
{
    std::string     id;
    std::string     version;
    std::string     timestamp;
    std::string     uid;
    std::string     user;
    std::string     changeset;
    std::vector<RelationMember> members;
    TagMap          tag_map;
} Relation;

typedef std::shared_ptr<Relation> RelationPtr;
typedef std::vector<RelationPtr> RelationPtrVec;

#ifdef _WIN32
typedef std::unordered_map<std::string, NodePtr> NodeMap;
typedef std::unordered_map<std::string, WayPtr> WayMap;
typedef std::unordered_map<std::string, RelationPtr> RelationMap;
#else
typedef boost::unordered_map<std::string, NodePtr> NodeMap;
typedef boost::unordered_map<std::string, WayPtr> WayMap;
typedef boost::unordered_map<std::string, RelationPtr> RelationMap;
#endif

typedef struct OsmData
{
    Bounds      bounds;
    NodeMap     node_map;
    WayMap      way_map;
    RelationMap relation_map;

    void Clear()
    {
        bounds.Clear();
        node_map.clear();
        way_map.clear();
        relation_map.clear();
    }

    bool Empty() const
    {
        return node_map.empty() && way_map.empty();
    }
} OsmData;


bool parse_osm_xml(const std::string &filename, OsmData &osm_data, std::string &err_str);
bool parse_osm_xml(std::istream &stream, OsmData &osm_data, std::string &err_str);
bool parse_osm_xml_from_string(const std::string &xml_data, OsmData &osm_data, std::string &err_str);

bool get_nodes_range(const NodeMap &node_map, double &lat_min, double &lat_max,
    double &lon_min, double &lon_max);
NodePtr get_node_by_ref(const NodeMap &node_map, const std::string& node_ref);
NodePtr get_node_by_ref(const NodeMap &node_map, const std::string& node_ref,
    std::string& err_str);

bool get_tag_from_map(const TagMap& tag_map, const std::string &key, std::string &value);
// def_lang_code: default language
// names_t: entry example "E:some name in English"
bool get_names_from_map(const TagMap& tag_map, char def_lang_code,
    std::vector<std::string>& names_t, std::string &err_str);

RelationPtr find_admin_boundry_relation(const OsmData &osm_data, const std::string& name,
    int admin_level);
bool get_relation_boundry_outer_nodes(const OsmData &osm_data, const Relation& relation,
    std::vector<NodePtrVec>& nodes_vec, bool& boundry_complete, std::string &err_str);
bool get_relation_boundry_outer_ways(const OsmData &osm_data, const Relation& relation,
    std::vector<WayPtr>& ways, std::string &err_str);
std::string get_relation_boundry_wkt(const OsmData &osm_data, const Relation& relation,
    bool& boundry_complete, std::string &err_str);
bool get_relation_outer_boundry(const OsmData &osm_data, const Relation& relation,
    geo::MultiPolygon& boundry, bool& boundry_complete, std::string &err_str);
bool get_relation_outer_boundry(const OsmData &osm_data, const std::string& relation_ref,
    geo::MultiPolygon& boundry, bool& boundry_complete, std::string &err_str);
bool get_relation_center(const OsmData &osm_data, const Relation& relation,
    geo::GeoPoint& point, std::string &err_str);
bool get_relation_subarea_members(const OsmData &osm_data, const Relation& relation,
    std::vector<osm::RelationMember>& subarea_members, std::string &err_str);

}

#endif // _OSM_PARSER_H
