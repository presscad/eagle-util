#ifndef _OSM_PARSER_H
#define _OSM_PARSER_H

#include <string>
#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>

namespace osm {

typedef std::map<std::string, std::string>  TagMap;

typedef struct {
    // <node id="29055831" version="21" timestamp="2013-08-01T07:05:36Z" uid="386131" user="zhongguo" changeset="7097408" lat="31.706849" lon="119.0231171">
    //    <tag k="barrier" v="toll_booth"/>
    // </node>
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

typedef struct {
    // <way id="118932337" version="1" timestamp="2013-08-01T07:05:36Z" uid="253683" user="sinopitt" changeset="8533092"> ...
    std::string     id;
    std::string     version;
    std::string     timestamp;
    std::string     uid;
    std::string     user;
    std::string     changeset;
    std::vector<std::string>    node_refs;
    TagMap          tag_map;
} Way;

typedef std::map<std::string, boost::shared_ptr<Node>>  NodeMap;
typedef std::map<std::string, boost::shared_ptr<Way>>   WayMap;

typedef struct {
    NodeMap     node_map;
    WayMap      way_map;
} OsmData;

bool parse_osm_xml(const std::string &filename, OsmData &osm_data, std::string &err_str);
bool osm_data_from_file(const std::string &filename, OsmData &osm_data, std::string &err_str);
bool osm_data_to_file(const std::string &filename, OsmData &osm_data, std::string &err_str);
bool get_nodes_range(const NodeMap &node_map, double &lat_min, double &lat_max, double &lon_min, double &lon_max);

}

#endif // _OSM_PARSER_H
