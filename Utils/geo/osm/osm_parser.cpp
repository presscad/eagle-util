#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include "osm_parser.h"
#include <string>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include "Common/CommUtils.h"


using namespace std;

namespace osm {

bool parse_osm_xml(const std::string &filename, OsmData &osm_data, std::string &err_str)
{
    // Create an empty property tree object
    using boost::property_tree::ptree;
    ptree pt;

    // Load the XML file into the property tree. If reading fails
    // (cannot open file, parse error), an exception is thrown.
    try {
        read_xml(filename, pt);

        osm_data.node_map.clear();
        osm_data.way_map.clear();

        BOOST_FOREACH(ptree::value_type &v, pt.get_child("osm")) {
            if (v.first == "way") {
                ptree way_pt = v.second;
                boost::shared_ptr<Way> the_way(new Way);

                ptree attr_pt = way_pt.get_child("<xmlattr>");
                the_way->id = attr_pt.get<string>("id");
                the_way->version = attr_pt.get<string>("version");
                the_way->timestamp = attr_pt.get<string>("timestamp");
                the_way->uid = attr_pt.get<string>("uid");
                the_way->user = attr_pt.get<string>("user");
                the_way->changeset = attr_pt.get<string>("changeset");

                BOOST_FOREACH(ptree::value_type &v, way_pt) {
                    ptree sub_pt = v.second;
                    if (v.first == "nd") {
                        string ref = sub_pt.get_child("<xmlattr>").get<string>("ref");
                        the_way->node_refs.push_back(ref);
                    } else if (v.first == "tag") {
                        ptree attr = sub_pt.get_child("<xmlattr>");
                        string k = attr.get<string>("k");
                        string v = attr.get<string>("v");
                        the_way->tag_map.insert(TagMap::value_type(k, v));
                    }
                }

                osm_data.way_map.insert(WayMap::value_type(the_way->id, boost::shared_ptr<Way>(the_way)));
            } else if (v.first == "node") {
                ptree node_pt = v.second;
                boost::shared_ptr<Node> the_node(new Node);

                ptree attr_pt = node_pt.get_child("<xmlattr>");
                the_node->id = attr_pt.get<string>("id");
                the_node->version = attr_pt.get<string>("version");
                the_node->timestamp = attr_pt.get<string>("timestamp");
                the_node->uid = attr_pt.get<string>("uid");
                the_node->user = attr_pt.get<string>("user");
                the_node->changeset = attr_pt.get<string>("changeset");
                the_node->lat = attr_pt.get<double>("lat");
                the_node->lon = attr_pt.get<double>("lon");

                BOOST_FOREACH(ptree::value_type &v, node_pt) {
                    ptree sub_pt = v.second;
                    if (v.first == "tag") {
                        ptree attr = sub_pt.get_child("<xmlattr>");
                        string k = attr.get<string>("k");
                        string v = attr.get<string>("v");
                        the_node->tag_map.insert(TagMap::value_type(k, v));
                    }
                }

                osm_data.node_map.insert(NodeMap::value_type(the_node->id, boost::shared_ptr<Node>(the_node)));
            }
        }
    } catch(std::exception const& e) {
        err_str = "Error: cannot parse xml file: " + filename + ": " + e.what();
        return false;
    }

    return true;
}

bool osm_data_to_file(const std::string &filename, OsmData &osm_data, std::string &err_str)
{
    std::ofstream out(filename);
    if (!out.good()) {
        err_str = "Error: cannot open " + filename + " for writing!";
        return false;
    }

    // save node count + way count
    char buff[1024 * 5], tmp[512];
    snprintf(buff, sizeof(buff), "count|node:%d|way:%d\n",
        (int)osm_data.node_map.size(), (int)osm_data.way_map.size());
    out << buff;

    // save nodes
    for (NodeMap::iterator it = osm_data.node_map.begin(); it != osm_data.node_map.end(); ++it) {
        boost::shared_ptr<Node> node = it->second;
        int tag_size = (int)node->tag_map.size();

        // save attributes + count of tags
        snprintf(buff, sizeof(buff), "node|%s|%s|%s|%s|%s|%s|%llf|%llf|%d",
            node->id.c_str(), node->version.c_str(), node->timestamp.c_str(), node->uid.c_str(),
            node->user.c_str(), node->changeset.c_str(), node->lat, node->lon, tag_size);

        // save tag pairs
        for (TagMap::iterator tag_it = node->tag_map.begin(); tag_it != node->tag_map.end(); ++tag_it) {
            string v(tag_it->second);
            //StringReplace(v, ",", ";"); // conflict with csv default delimiter
            snprintf(tmp, sizeof(tmp), "|%s|%s", tag_it->first.c_str(), v.c_str());
            strncat(buff, tmp, sizeof(buff));
        }

        strncat(buff, "\n", sizeof(buff));
        out << buff;
    }

    // save ways
    for (WayMap::const_iterator it = osm_data.way_map.begin(); it != osm_data.way_map.end(); ++it) {
        boost::shared_ptr<Way> way = it->second;
        int ref_size = (int)way->node_refs.size();
        int tag_size = (int)way->tag_map.size();

        // save attributes + count of tags
        snprintf(buff, sizeof(buff), "way|%s|%s|%s|%s|%s|%s|%d|%d",
            way->id.c_str(), way->version.c_str(), way->timestamp.c_str(), way->uid.c_str(),
            way->user.c_str(), way->changeset.c_str(), ref_size, tag_size);

        // save node refs
        for (int i = 0; i < ref_size; ++i) {
            snprintf(tmp, sizeof(tmp), "|%s", way->node_refs[i].c_str());
            strncat(buff, tmp, sizeof(buff));
            if (i % 100 == 0) {
                out << buff;
                buff[0] = '\0';
            }
        }

        // save tag pairs
        for (TagMap::iterator tag_it = way->tag_map.begin(); tag_it != way->tag_map.end(); ++tag_it) {
            string v(tag_it->second);
            //StringReplace(v, ",", ";"); // conflict with csv default delimiter
            snprintf(tmp, sizeof(tmp), "|%s|%s", tag_it->first.c_str(), v.c_str());
            strncat(buff, tmp, sizeof(buff));
        }

        strncat(buff, "\n", sizeof(buff));
        out << buff;
    }

    out.close();
    return true;
}

bool osm_data_from_file(const std::string &filename, OsmData &osm_data, std::string &err_str)
{
    std::ifstream in(filename);
    if (!in.good()) {
        err_str = "Error: cannot open " + filename + " for reading!";
        return false;
    }

    // get node count and way count
    int node_count, way_count;
    string line;
    if (false == GetLine(in, line)) {
        err_str = "Error: cannot read from " + filename;
        return false;
    }
    if (2 != sscanf(line.c_str(), "count|node:%d|way:%d", &node_count, &way_count)) {
        err_str = "Error: cannot parse: " + line;
        return false;
    }

    osm_data.node_map.clear();
    osm_data.way_map.clear();
    vector<string> buffs;
    while (GetLine(in, line))
    {
        CsvLinePopulate(buffs, line, '|');
        if (buffs.size() < 8) {
            err_str = "Error: invalid line in CSV: " + line;
            return false;
        }

        if (buffs[0] == "node")
        {
            boost::shared_ptr<Node> p_node(new Node);
            p_node->id = buffs[1];
            p_node->version = buffs[2];
            p_node->timestamp = buffs[3];
            p_node->uid = buffs[4];
            p_node->user = buffs[5];
            p_node->changeset = buffs[6];
            p_node->lat = boost::lexical_cast<double>(buffs[7]);
            p_node->lon = boost::lexical_cast<double>(buffs[8]);

            // for tags
            int tag_count = boost::lexical_cast<int>(buffs[9]);
            if (tag_count * 2 + 10 != buffs.size()) {
                err_str = "Error: invalid line in CSV: " + line;
                return false;
            }

            int i = 10;
            for (int t = 0; t < tag_count; ++t, i += 2) {
                //StringReplace(buffs[i + 1], ";", ","); // "," conflict with csv default delimiter
                p_node->tag_map.insert(TagMap::value_type(buffs[i], buffs[i + 1]));
            }

            osm_data.node_map.insert(NodeMap::value_type(p_node->id, p_node));
        }
        else if (buffs[0] == "way")
        {
            boost::shared_ptr<Way> p_way(new Way);
            p_way->id = buffs[1];
            p_way->version = buffs[2];
            p_way->timestamp = buffs[3];
            p_way->uid = buffs[4];
            p_way->user = buffs[5];
            p_way->changeset = buffs[6];

            int ref_count = boost::lexical_cast<int>(buffs[7]);
            int tag_count = boost::lexical_cast<int>(buffs[8]);
            if (ref_count + tag_count * 2 + 9 != buffs.size())
            {
                err_str = "Error: invalid line in CSV: " + line;
                return false;
            }

            // for refs
            int i = 9;
            p_way->node_refs.reserve(ref_count);
            for (int t = 0; t < ref_count; ++t)
            {
                p_way->node_refs.push_back(buffs[i++]);
            }

            // for tags
            for (int t = 0; t < tag_count; ++t, i += 2)
            {
                //StringReplace(buffs[i + 1], ";", ","); // "," conflict with csv default delimiter
                p_way->tag_map.insert(TagMap::value_type(buffs[i], buffs[i + 1]));
            }

            osm_data.way_map.insert(WayMap::value_type(p_way->id, p_way));
        } else {
            err_str = "Error: invalid line in CSV: " + line;
            return false;
        }
    }

    in.close();
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

}
