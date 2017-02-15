
#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include "osm_parser.h"
#include <string>
#include <algorithm>
#include <fstream>
#include <sstream>      // std::istringstream
#include "common/common_utils.h"
#include "pugixml.hpp"


using namespace std;
using namespace util;

namespace osm {

// e.g., way1, way2, way3's node refs are identical,
//   way1.duplicate_to_id is empty, way2.duplicate_to_id and way3.duplicate_to_id are set to way1
static bool flag_duplicate_ways(WayMap& way_map, string &err_str)
{
    // way map => vector
    vector<WayPtr> ways;
    ways.reserve(way_map.size());
    for (auto& it : way_map) {
        ways.push_back(it.second);
    }

    // order by node_refs, id
    sort(ways.begin(), ways.end(), [](const WayPtr& i, const WayPtr& j) {
        if (i->node_refs.size() != j->node_refs.size()) {
            return i->node_refs.size() < j->node_refs.size();
        }

        if (i->node_refs != j->node_refs) {
            return i->node_refs < j->node_refs;
        }
        else {
            return i->id < j->id;
        }
    });

    // duplicate_to_id flag set to the 1-step same way ID (e.g., way3.duplicate_to_id set to way2)
    for (int i = 0; i < (int)ways.size() - 1; ++i) {
        if (ways[i]->node_refs == ways[i + 1]->node_refs) {
            ways[i + 1]->duplicate_to_id = ways[i]->id;
        }
    }
    // duplicate_to_id flag set to the 1st same way ID (e.g., way3.duplicate_to_id set to way1)
    for (auto& way : ways) {
        if (way->duplicate_to_id.empty()) {
            continue;
        }

        WayPtr pre_way;
        do {
            pre_way = way_map.at(way->duplicate_to_id);
        } while (!pre_way->duplicate_to_id.empty());
        if (way->duplicate_to_id != pre_way->id) {
            way->duplicate_to_id = pre_way->id;
        }
    }

    return true;
}

bool parse_osm_xml(const string &filename, OsmData &osm_data, string &err_str)
{
    ifstream in(filename.c_str());
    if (!in.good()) {
        err_str = "Error: cannot open xml file: " + filename;
        return false;
    }

    return parse_osm_xml(in, osm_data, err_str);
}

bool parse_osm_xml_from_string(const string &xml_data, OsmData &osm_data, string &err_str)
{
    istringstream iss(xml_data);
    if (!iss.good()) {
        err_str = "Error: invalid xml data string";
        return false;
    }

    return parse_osm_xml(iss, osm_data, err_str);
}

bool parse_osm_xml(istream &stream, OsmData &osm_data, string &err_str)
{
    using namespace pugi;
    xml_document xml_doc;

    auto&& load_result = xml_doc.load(stream);
    if (!load_result){
        err_str = load_result.description();
        return false;
    }

    auto&& osm = xml_doc.child("osm");
    if (osm.empty()) {
        err_str = "invalid XML";
        return false;
    }

    auto&& bounds = osm.child("bounds");
    if (!bounds.empty()) {
        osm_data.bounds.minlat = bounds.attribute("minlat").as_string();
        osm_data.bounds.minlon = bounds.attribute("minlon").as_string();
        osm_data.bounds.maxlat = bounds.attribute("maxlat").as_string();
        osm_data.bounds.maxlon = bounds.attribute("maxlon").as_string();
    }

    for (const auto& xml_nd : osm.children("node")) {
        NodePtr p_node = make_shared<Node>();
        p_node->id = xml_nd.attribute("id").as_string();
        p_node->version = xml_nd.attribute("version").as_string();
        p_node->timestamp = xml_nd.attribute("timestamp").as_string();
        p_node->uid = xml_nd.attribute("uid").as_string();
        p_node->user = xml_nd.attribute("user").as_string();
        p_node->changeset = xml_nd.attribute("changeset").as_string();
        p_node->lat = xml_nd.attribute("lat").as_double();
        p_node->lon = xml_nd.attribute("lon").as_double();

        for (const auto& tag : xml_nd.children("tag")) {
            auto&& k = tag.attribute("k").as_string();
            auto&& v = tag.attribute("v").as_string();
            p_node->tag_map.insert(TagMap::value_type(k, v));
        }

        osm_data.node_map.insert(make_pair(p_node->id, p_node));
    }

    for (const auto& xml_nd : osm.children("way")) {
        WayPtr p_way = make_shared<Way>();
        p_way->id = xml_nd.attribute("id").as_string();
        p_way->version = xml_nd.attribute("version").as_string();
        p_way->timestamp = xml_nd.attribute("timestamp").as_string();
        p_way->uid = xml_nd.attribute("uid").as_string();
        p_way->user = xml_nd.attribute("user").as_string();
        p_way->changeset = xml_nd.attribute("changeset").as_string();

        for (const auto& nd : xml_nd.children("nd")) {
            auto&& ref = nd.attribute("ref").as_string();
            if (p_way->node_refs.empty()) {
                p_way->node_refs.push_back(ref);
            }
            else if (p_way->node_refs.back() != ref) {
                p_way->node_refs.push_back(ref);
            }
        }

        for (const auto& tag : xml_nd.children("tag")) {
            auto&& k = tag.attribute("k").as_string();
            auto&& v = tag.attribute("v").as_string();
            p_way->tag_map.insert(TagMap::value_type(k, v));
        }

        osm_data.way_map.insert(make_pair(p_way->id, p_way));
    }

    for (const auto& xml_nd : osm.children("relation")) {
        RelationPtr p_relation = make_shared<Relation>();

        p_relation->id = xml_nd.attribute("id").as_string();
        p_relation->version = xml_nd.attribute("version").as_string();
        p_relation->timestamp = xml_nd.attribute("timestamp").as_string();
        p_relation->uid = xml_nd.attribute("uid").as_string();
        p_relation->user = xml_nd.attribute("user").as_string();
        p_relation->changeset = xml_nd.attribute("changeset").as_string();

        for (const auto& member : xml_nd.children("member")) {
            RelationMember mb;
            mb.type = member.attribute("type").as_string();
            mb.ref = member.attribute("ref").as_string();
            mb.role = member.attribute("role").as_string();
            p_relation->members.push_back(mb);
        }

        for (const auto& tag : xml_nd.children("tag")) {
            auto&& k = tag.attribute("k").as_string();
            auto&& v = tag.attribute("v").as_string();
            p_relation->tag_map.insert(TagMap::value_type(k, v));
        }

        osm_data.relation_map.insert(make_pair(p_relation->id, p_relation));
    }

    if (false == flag_duplicate_ways(osm_data.way_map, err_str)) {
        return false;
    }

    return true;
}

bool get_nodes_range(const NodeMap &node_map, double &lat_min, double &lat_max, double &lon_min, double &lon_max)
{
    NodeMap::const_iterator it = node_map.begin();
    if (it == node_map.end()) {
        return false;
    }

    lat_min = lat_max = it->second->lat;
    lon_min = lon_max = it->second->lon;

    for (; it != node_map.end(); ++it) {
        if (it->second->lat < lat_min) {
            lat_min = it->second->lat;
        }
        if (it->second->lat > lat_max) {
            lat_max = it->second->lat;
        }
        if (it->second->lon < lon_min) {
            lon_min = it->second->lon;
        }
        if (it->second->lon > lon_max) {
            lon_max = it->second->lon;
        }
    }
    return true;
}

NodePtr get_node_by_ref(const NodeMap &node_map, const string &node_ref)
{
    const auto it = node_map.find(node_ref);
    if (it == node_map.cend()) {
        return nullptr;
    }
    return it->second;
}

NodePtr get_node_by_ref(const NodeMap &node_map, const string& node_ref,
    string &err_str)
{
    const auto it = node_map.find(node_ref);
    if (it == node_map.cend()) {
        err_str = "Cannot find node ID " + node_ref + ". OSM data's range may be too small.";
        return nullptr;
    }
    return it->second;
}

bool get_tag_from_map(const TagMap& tag_map, const string &key, string &value)
{
    TagMap::const_iterator tag_it = tag_map.find(key);
    if (tag_it != tag_map.cend()) {
        value = tag_it->second;
        return true;
    }
    return false;
}

bool get_names_from_map(const TagMap& tag_map, char def_lang_code, vector<string>& names, string &err_str)
{
    string def_name;
    names.clear();
    bool already_have_def_name = false;

    for (auto& it : tag_map) {
        const string& k = it.first;
        const string& v = it.second;
        if (k.front() != 'n') {
            continue;
        }

        if (k.find("name:") == 0) { // str begin with
            string locale = k.c_str() + 5;
            if (locale == "eo" || // Esperanto
                locale == "vi") { // Vietnamese
                continue;
            }

            char lang_char{};
            if (true == util::LocaleToLangChar(locale, lang_char)) {
                if (lang_char == def_lang_code) {
                    already_have_def_name = true;
                }
                names.push_back(string(1, lang_char) + ':' + v);
            }
            else {
                err_str = "unknown locale string: \"" + k + "\"";
                return false;
            }
        }
        else if (k == "name") {
            def_name = v;
        }
    }

    if (!already_have_def_name && !def_name.empty()) {
        names.push_back(string(1, def_lang_code) + ':' + def_name);
    }

    sort(names.begin(), names.end());
    if (names.empty()) {
        err_str = "get_names_from_map: no name tags found";
        return false;
    }
    return true;
}

RelationPtr find_admin_boundry_relation(const OsmData &osm_data, const string& name,
    int admin_level)
{
    string adm_level;
    if (admin_level > 0) {
        adm_level = to_string(admin_level);
    }

    string value;
    for (const auto& it : osm_data.relation_map) {
        const RelationPtr& p_relation = it.second;

        // check admin level
        if (false == get_tag_from_map(p_relation->tag_map, "admin_level", value)) {
            continue;
        }
        if (admin_level > 0 && value != adm_level) {
            continue;
        }

        // check boundary
        if (false == get_tag_from_map(p_relation->tag_map, "boundary", value)) {
            continue;
        }
        if (value != "administrative") {
            continue;
        }

        // check name
        if (false == get_tag_from_map(p_relation->tag_map, "name", value)) {
            continue;
        }
        if (value.compare(0, name.length(), name) != 0) { // if value not begin with name ...
            continue;
        }
        else {
            return p_relation;
        }
    }

    return RelationPtr();
}

bool get_relation_boundry_outer_ways(const OsmData &osm_data, const Relation& relation,
    vector<WayPtr>& outer_ways, string &err_str)
{
    outer_ways.clear();
    for (const RelationMember& member : relation.members) {
        if (member.role == "outer" && member.type == "way") {
            const auto& way_id = member.ref;
            const auto it = osm_data.way_map.find(way_id);
            if (it == osm_data.way_map.cend()) {
                err_str = "Cannot find way ID " + way_id + ". OSM data's range may be too small.";
                return false;
            }
            outer_ways.push_back(it->second);
        }
    }

    if (outer_ways.empty()) {
        err_str = "Found no outer ways for relation: " + relation.id;
        return false;
    }

    return true;
}

struct LOCAL_WAY {
    WayPtr  p_way;
    NodePtr p_begin_node;
    NodePtr p_end_node;
};

typedef vector<LOCAL_WAY> LOCAL_WAY_VEC;

static void two_ways_ends_distances(const LOCAL_WAY& lway1, const LOCAL_WAY& lway2,
    double& d11, double& d12, double& d21, double& d22)
{
    const auto p_way1_nd1 = lway1.p_begin_node;
    const auto p_way1_nd2 = lway1.p_end_node;
    const auto p_way2_nd1 = lway2.p_begin_node;
    const auto p_way2_nd2 = lway2.p_end_node;

    d11 = geo::distance_in_meter(p_way1_nd1->lat, p_way1_nd1->lon, p_way2_nd1->lat, p_way2_nd1->lon);
    d12 = geo::distance_in_meter(p_way1_nd1->lat, p_way1_nd1->lon, p_way2_nd2->lat, p_way2_nd2->lon);
    d21 = geo::distance_in_meter(p_way1_nd2->lat, p_way1_nd2->lon, p_way2_nd1->lat, p_way2_nd1->lon);
    d22 = geo::distance_in_meter(p_way1_nd2->lat, p_way1_nd2->lon, p_way2_nd2->lat, p_way2_nd2->lon);
}

static void node_to_wayends_distances(const NodePtr& p_nd, const LOCAL_WAY& lway,
    double& d1, double& d2)
{
    if (lway.p_begin_node->id == p_nd->id) {
        d1 = 0.0;
    }
    else {
        const auto& p_waynd1 = lway.p_begin_node;
        d1 = geo::distance_in_meter(p_nd->lat, p_nd->lon, p_waynd1->lat, p_waynd1->lon);
    }

    if (lway.p_end_node->id == p_nd->id) {
        d2 = 0.0;
    }
    else {
        const auto& p_waynd2 = lway.p_end_node;
        d2 = geo::distance_in_meter(p_nd->lat, p_nd->lon, p_waynd2->lat, p_waynd2->lon);
    }
}

// make sure the outer ways are connected one by one in sequence
static bool sort_outer_ways(LOCAL_WAY_VEC &lways, string &err_str)
{
    if (lways.size() <= 2) {
        return true;
    }

    for (size_t i = 0; i < lways.size() - 2; ++i) {
        const LOCAL_WAY& lway1 = lways[i];
        const WayPtr& p_way1 = lway1.p_way;
        int j_min = -1;

        // get the next connected way by checking node ID
        for (size_t j = i + 1; j < lways.size(); ++j) {
            const WayPtr& p_way2 = lways[j].p_way;

            if (p_way1->node_refs[0] == p_way2->node_refs[0] ||
                p_way1->node_refs[0] == p_way2->node_refs[p_way2->node_refs.size() - 1] ||
                p_way1->node_refs[p_way1->node_refs.size() - 1] == p_way2->node_refs[0] ||
                p_way1->node_refs[p_way1->node_refs.size() - 1] == p_way2->node_refs[p_way2->node_refs.size() - 1])
            {
                j_min = (int)j;
                break;
            }
        }

        // get the next connected way by checking distance
        if (j_min == -1) {
            double d_min = 1000000000;
            for (size_t j = i + 1; j < lways.size(); ++j) {
                const LOCAL_WAY& lway2 = lways[j];
                double d11, d12, d21, d22;
                two_ways_ends_distances(lway1, lway2, d11, d12, d21, d22);

                double d_min_ij = min(min(d11, d12), min(d21, d22));
                if (d_min_ij < d_min) {
                    d_min = d_min_ij;
                    j_min = (int)j;
                }
            }
        }
        if (j_min > 0 && j_min != i + 1) {
            swap(lways[i + 1], lways[j_min]);
        }
    }
    return true;
}

static bool to_local_ways(const OsmData &osm_data, const vector<osm::WayPtr> &ways,
    LOCAL_WAY_VEC& lways, string &err_str)
{
    lways.clear();
    for (const auto& p_way : ways) {
        LOCAL_WAY lway;
        lway.p_way = p_way;
        lway.p_begin_node = get_node_by_ref(osm_data.node_map, p_way->node_refs[0], err_str);
        lway.p_end_node = get_node_by_ref(osm_data.node_map,
            p_way->node_refs[p_way->node_refs.size() - 1], err_str);

        lways.push_back(lway);
    }
    return true;
}

static bool two_way_connected(const LOCAL_WAY& way1, const LOCAL_WAY& way2)
{
    if (way1.p_begin_node->id == way2.p_begin_node->id ||
        way1.p_begin_node->id == way2.p_end_node->id ||
        way1.p_end_node->id == way2.p_begin_node->id ||
        way1.p_end_node->id == way2.p_end_node->id)
    {
        return true;
    }
    return false;
}

// merge src way group into dest way group if connected
// if src and dst are connected and merging happened, src clears and returns true
static bool check_merge_two_groups(LOCAL_WAY_VEC& src, LOCAL_WAY_VEC& dst)
{
    bool merged = false;
    if (src.size() == 1 && dst.size() == 1) {
        const LOCAL_WAY& src_way = src[0];
        const LOCAL_WAY& dst_way = dst[0];
        if (src_way.p_begin_node->id == dst_way.p_begin_node->id ||
            src_way.p_end_node->id == dst_way.p_begin_node->id)
        {
            dst.push_back(dst_way);
            dst[0] = src_way;
            merged = true;
        }
        else if (src_way.p_begin_node->id == dst_way.p_end_node->id ||
            src_way.p_end_node->id == dst_way.p_end_node->id)
        {
            dst.push_back(src_way);
            merged = true;
        }
    }
    else if (src.size() == 1 && dst.size() > 1) {
        const LOCAL_WAY& src_way = src[0];
        const LOCAL_WAY& dst_way1 = dst[0];
        const LOCAL_WAY& dst_way2 = dst[dst.size() - 1];
        if (two_way_connected(src_way, dst_way1)) {
            dst.insert(dst.begin(), src_way);
            merged = true;
        }
        else if (two_way_connected(src_way, dst_way2)) {
            dst.push_back(src_way);
            merged = true;
        }
    }
    else if (src.size() > 1 && dst.size() == 1) {
        const LOCAL_WAY& src_way1 = src[0];
        const LOCAL_WAY& src_way2 = src[src.size() - 1];
        const LOCAL_WAY& dst_way = dst[0];
        if (two_way_connected(src_way1, dst_way)) {
            src.insert(src.begin(), dst_way);
            src.swap(dst);
            merged = true;
        }
        else if (two_way_connected(src_way2, dst_way)) {
            src.push_back(dst_way);
            src.swap(dst);
            merged = true;
        }
    }
    else if (src.size() > 1 && dst.size() > 1) {
        const LOCAL_WAY& src_way1 = src[0];
        const LOCAL_WAY& src_way2 = src[src.size() - 1];
        const LOCAL_WAY& dst_way1 = dst[0];
        const LOCAL_WAY& dst_way2 = dst[dst.size() - 1];
        if (two_way_connected(src_way1, dst_way1)) {
            reverse(src.begin(), src.end());
            dst.insert(dst.begin(), src.begin(), src.end());
            merged = true;
        }
        else if (two_way_connected(src_way1, dst_way2)) {
            dst.insert(dst.end(), src.begin(), src.end());
            merged = true;
        }
        else if (two_way_connected(src_way2, dst_way1)) {
            dst.insert(dst.begin(), src.begin(), src.end());
            merged = true;
        }
        else if (two_way_connected(src_way2, dst_way2)) {
            reverse(src.begin(), src.end());
            dst.insert(dst.end(), src.begin(), src.end());
            merged = true;
        }
    }

    if (merged) {
        src.clear();
    }
    return merged;
}

// return whether the group of ways are a closed loop
// precondition: the ways in the group are connected in order
static bool is_closed_way_group(const LOCAL_WAY_VEC& group)
{
    const size_t size = group.size();
    if (size == 1) {
        const auto& way = group[0];
        if (way.p_begin_node->id == way.p_end_node->id) {
            return true;
        }
    }
    else if (size == 2) {
        const auto& way1 = group[0];
        const auto& way2 = group[1];
        if ((way1.p_begin_node == way2.p_begin_node && way1.p_end_node == way2.p_end_node) ||
            (way1.p_end_node == way2.p_end_node && way1.p_end_node == way2.p_begin_node)) {
            return true;
        }
    }
    else if (size > 2) {
        const auto& way1 = group[0];
        const auto& way2 = group[size - 1];
        if (way1.p_begin_node == way2.p_begin_node || way1.p_begin_node == way2.p_end_node ||
            way1.p_end_node == way2.p_begin_node || way1.p_end_node == way2.p_end_node) {
            return true;
        }
    }
    return false;
}

// return number of groups.
// the returned groups are to be orderred by number of ways desc.
// ways in each group are also to be connected in order
static size_t ways_to_sorted_groups(const LOCAL_WAY_VEC& lways,
    vector<LOCAL_WAY_VEC>& lway_groups)
{
    vector<LOCAL_WAY_VEC> groups;

    // firstly, each way into one group
    groups.resize(lways.size());
    for (size_t i = 0; i < lways.size(); ++i) {
        groups[i].push_back(lways[i]);
    }
    if (groups.size() <= 1) {
        return groups.size();
    }

    // merge connected ways into groups
    bool mreged;
    do {
        mreged = false;
        for (int i = 0; i < (int)groups.size() - 1; ++i) {
            for (int j = (int)groups.size() - 1; j > i; --j) {
                if (!groups[j].empty() && true == check_merge_two_groups(groups[j], groups[i])) {
                    if (!mreged) {
                        mreged = true;
                    }
                }
            }
        }
    } while (mreged);

    // copy the valid groups into result
    lway_groups.clear();
    for (auto& g : groups) {
        if (!g.empty()) {
            lway_groups.push_back(g);
        }
    }

    // put the most important groups at begining
    sort(lway_groups.begin(), lway_groups.end(),
        [](const LOCAL_WAY_VEC& i, const LOCAL_WAY_VEC& j) {
        return i.size() > j.size();
    });

    return true;
}

bool get_relation_boundry_outer_nodes(const OsmData &osm_data, const Relation& relation,
    vector<NodePtrVec>& nodes_vec, bool& boundry_complete, string &err_str)
{
    vector<osm::WayPtr> outer_ways;
    if (false == get_relation_boundry_outer_ways(osm_data, relation, outer_ways, err_str)) {
        return false;
    }

    nodes_vec.clear();
    if (outer_ways.size() == 1) {
        vector<string> node_refs;
        for (const auto& node_ref : outer_ways[0]->node_refs) {
            node_refs.push_back(node_ref);
        }
    }
    else {
        vector<LOCAL_WAY_VEC> lway_groups;
        {
            LOCAL_WAY_VEC lways;
            if (false == to_local_ways(osm_data, outer_ways, lways, err_str)) {
                return false;
            }
            ways_to_sorted_groups(lways, lway_groups);
        }

        if (!is_closed_way_group(lway_groups[0])) {
            boundry_complete = false;

            // all the imcomplete loops may belong to the same loop, merge them together
            for (size_t i = 1; i < lway_groups.size(); ++i) {
                if (!is_closed_way_group(lway_groups[i])) {
                    lway_groups[0].insert(lway_groups[0].end(), lway_groups[i].begin(), lway_groups[i].end());
                    lway_groups[i].clear();
                }
            }
            if (false == sort_outer_ways(lway_groups[0], err_str)) {
                return false;
            }
        }
        else {
            boundry_complete = true;
        }

        for (const auto& lways : lway_groups) {
            if (lways.empty()) {
                continue;
            }

            vector<string> node_refs;
#ifdef _WIN32
            unordered_map<string, int> node_set;
#else
            boost::unordered_map<string, int> node_set;
#endif
            if (lways.size() == 1) {
                for (const auto& node_ref : lways[0].p_way->node_refs) {
                    node_refs.push_back(node_ref);
                }
            }
            else {
                for (size_t i = 0; i < lways.size(); ++i) {
                    LOCAL_WAY lway1, lway2;
                    bool asc;

                    if (i == 0) {
                        lway1 = lways[i];
                        lway2 = lways[i + 1];
                        double d11, d12, d21, d22;
                        two_ways_ends_distances(lway1, lway2, d11, d12, d21, d22);

                        // get the min index
                        int k_min = -1;
                        double arr[4] = { d11, d12, d21, d22 };
                        double min_v = 100000000000.0;
                        for (int k = 0; k < 4; ++k) {
                            if (arr[k] < min_v) {
                                min_v = arr[k];
                                k_min = k;
                            }
                        }

                        asc = (k_min == 2 || k_min == 3);
                    }
                    else {
                        lway1 = lways[i];

                        double d1, d2;
                        NodePtr p_last_node = get_node_by_ref(osm_data.node_map, node_refs[node_refs.size() - 1], err_str);
                        if (NULL == p_last_node) {
                            return false;
                        }
                        node_to_wayends_distances(p_last_node, lway1, d1, d2);
                        asc = (d1 < d2);
                    }

                    if (asc) {
                        for (size_t i = 0; i < lway1.p_way->node_refs.size(); ++i) {
                            const string& node_ref = lway1.p_way->node_refs[i];
                            // avoid duplicated node
                            if (node_set.find(node_ref) == node_set.end()) {
                                node_refs.push_back(node_ref);
                                node_set.insert({ node_ref, 0 });
                            }
                        }
                    }
                    else {
                        for (int i = (int)lway1.p_way->node_refs.size() - 1; i >= 0; --i) {
                            const string& node_ref = lway1.p_way->node_refs[i];
                            // avoid duplicated node
                            if (node_set.find(node_ref) == node_set.end()) {
                                node_refs.push_back(node_ref);
                                node_set.insert({ node_ref, 0 });
                            }
                        }
                    }
                }
            }

            NodePtrVec nodes;
            nodes.reserve(node_refs.size());
            for (const auto& node_id : node_refs) {
                const auto& p_node = get_node_by_ref(osm_data.node_map, node_id, err_str);
                if (NULL == p_node) {
                    return false;
                }
                nodes.push_back(p_node);
            }

            // make sure the start and end are the same node
            if (!nodes.empty()) {
                if (nodes[0]->id != nodes[nodes.size() - 1]->id) {
                    nodes.push_back(nodes[0]);
                }
                nodes_vec.push_back(nodes);
            }
        }
    }

    return !nodes_vec.empty();
}

bool get_relation_outer_boundry(const OsmData &osm_data, const Relation& relation,
    geo::MultiPolygon& boundry, bool& boundry_complete, string &err_str)
{
    vector<NodePtrVec> nodes_vec;
    if (false == get_relation_boundry_outer_nodes(osm_data, relation, nodes_vec,
        boundry_complete, err_str)) {
        return false;
    }
    if (nodes_vec.empty()) {
        err_str = "No enough boundry nodes in Relation: " + relation.id;
        return false;
    }

    boundry.polygons.clear();
    geo::Polygon polygon;

    for (const auto& nodes : nodes_vec) {
        polygon.Clear();
        polygon.outer_polygon.Reserve(nodes.size());
        for (size_t i = 0; i < nodes.size(); ++i) {
            const Node& node = *nodes[i];
            polygon.outer_polygon.PushBack(geo::GeoPoint(node.lat, node.lon));
        }
        boundry.polygons.push_back(polygon);
    }

    return !boundry.polygons.empty();
}

bool get_relation_outer_boundry(const OsmData &osm_data, const string& relation_ref,
    geo::MultiPolygon& boundry, bool& boundry_complete, string &err_str)
{
    const auto& relation_map = osm_data.relation_map;
    auto it = relation_map.find(relation_ref);
    if (it == relation_map.end()) {
        err_str = "Not found relation ref " + relation_ref + " in relation map of OSM data";
        return false;
    }
    return get_relation_outer_boundry(osm_data, *it->second, boundry, boundry_complete, err_str);
}

string get_relation_boundry_wkt(const OsmData &osm_data, const Relation& relation,
    bool& boundry_complete, string &err_str)
{
    geo::MultiPolygon boundry;
    if (false == get_relation_outer_boundry(osm_data, relation, boundry, boundry_complete, err_str)) {
        return string();
    }

    string wkt;
    if (false == geo::multi_polygon_to_wkt(boundry, wkt)) {
        err_str = "No enough boundry information in Relation: " + relation.id;
        return string();
    }

    return wkt;
}


bool get_relation_center(const OsmData &osm_data, const Relation& relation,
    geo::GeoPoint& point, string &err_str)
{
    const RelationMember* p_member = NULL;
    for (const auto& m : relation.members) {
        if (m.role == "label") {
            p_member = &m;
            break;
        }
    }
    if (!p_member) {
        for (const auto& m : relation.members) {
            if (m.role == "admin_centre") {
                p_member = &m;
                break;
            }
        }
    }

    if (p_member) {
        if (p_member->type == "node") {
            NodePtr p_node = get_node_by_ref(osm_data.node_map, p_member->ref, err_str);
            if (NULL == p_node) {
                return false;
            }
            point.lat = p_node->lat;
            point.lng = p_node->lon;
            return true;
        }
        else if (p_member->type == "relation") {
            geo::MultiPolygon boundry;
            bool boundry_complete;
            if (false == get_relation_outer_boundry(osm_data, p_member->ref, boundry,
                boundry_complete, err_str)) {
                return false;
            }
            if (boundry_complete) {
                geo::get_polygon_centroid(boundry.polygons[0].outer_polygon, point.lat, point.lng);
            }
            else {
                geo::get_polygon_centroid_simple(boundry.polygons[0].outer_polygon, point.lat, point.lng);
            }
            return true;
        }
        err_str = "unknown type of admin_centre, relation ID: " + relation.id;
        return false;
    }

    err_str = "not found \"label\" or \"admin_centre\" for relation of ID " + relation.id;
    return false;
}

bool get_relation_subarea_members(const OsmData &osm_data, const Relation& relation,
    vector<osm::RelationMember>& subarea_members, string &err_str)
{
    subarea_members.clear();
    for (const auto& m : relation.members) {
        if (m.role == "subarea") {
            subarea_members.push_back(m);
        }
    }
    return true;
}

}
