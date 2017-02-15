
#include "way_manager.h"
#include <memory>
#include <set>
#include <stack>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <thread>
#include <future>
#include <cmath>
#include <tuple>
#include "geo_utils.h"
#include "common/csv_to_tuples.hpp"
#include "common/simple_matrix.hpp"
#include "common/common_utils.h"
#include "common/simple_thread_pool.hpp"
#include "common/simple_par_algorithm.hpp"
#include "common/at_scope_exit.h"
#if WAY_MANAGER_HANA_LOG == 1
#include <hana/logging.h>
#include <chrono>
#endif

#ifdef _MSC_VER
#define _THREAD __declspec( thread )
#else
#define _THREAD __thread
#endif

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

#ifdef min
#undef min
#endif

#define LAT_METERS_PER_DEGREE   (111194.99646)  // R_EARTH * 2 * PI / 360

#ifndef _countof
#define _countof(a) (sizeof(a)/sizeof(*(a)))
#endif

using namespace std;

namespace geo {

typedef geo::GeoPoint GeoPoint;

const int HIGHWAY_REF_MAX_SPEEDS[]
{
    0,   // HIGHWAY_UNKNOWN

    // Roads
    180, // HIGHWAY_MOTORWAY
    150, // HIGHWAY_TRUNK
    120, // HIGHWAY_PRIMARY
    100, // HIGHWAY_SECONDARY
    80,  // HIGHWAY_TERTIARY
    80,  // HIGHWAY_UNCLASSIFIED
    80,  // HIGHWAY_RESIDENTIAL
    80,  // HIGHWAY_SERVICE
    0,   // HIGHWAY_RESERVE1
    0,   // HIGHWAY_RESERVE2
    0,   // HIGHWAY_RESERVE3

    // Link roads
    120, // HIGHWAY_MOTORWAY_LINK
    100, // HIGHWAY_TRUNK_LINK
    100, // HIGHWAY_PRIMARY_LINK
    80,  // HIGHWAY_SECONDARY_LINK
    60,  // HIGHWAY_TERTIARY_LINK
    0,   // HIGHWAY_RESERVE4
    0,   // HIGHWAY_RESERVE5
    0,   // HIGHWAY_RESERVE6

    // Special road types
    60,  // HIGHWAY_LIVING_STREET
    60,  // HIGHWAY_PEDESTRIAN
    60,  // HIGHWAY_TRACK
    60,  // HIGHWAY_BUS_GUIDEWAY
    60,  // HIGHWAY_RACEWAY
    60,  // HIGHWAY_ROAD
    0,   // HIGHWAY_RESERVE7
    0,   // HIGHWAY_RESERVE8
    0,   // HIGHWAY_RESERVE9

    // Paths
    40,  // HIGHWAY_FOOTWAY
    40,  // HIGHWAY_BRIDLEWAY
    40,  // HIGHWAY_STEPS
    40,  // HIGHWAY_PATH
    0,   // HIGHWAY_RESERVE10
    0,   // HIGHWAY_RESERVE11

    40,  // HIGHWAY_CYCLEWAY
    0,   // HIGHWAY_RESERVE12
    0,   // HIGHWAY_RESERVE13

    // Lifecycle
    40,  // HIGHWAY_PROPOSED
    40,  // HIGHWAY_CONSTRUCTION
    0,   // HIGHWAY_RESERVE14
    0,   // HIGHWAY_RESERVE15
};

static_assert(_countof(HIGHWAY_REF_MAX_SPEEDS) == HIGHWAY_TYPE_MAX,
    "HIGHWAY_REF_MAX_SPEEDS element count must match the highway type definition");


const string ORIENTATION_STRS[] =
{
    "invalid orientation",
    "south to north",
    "southwest to northeast",
    "west to east",
    "northwest to southeast",
    "north to south",
    "northeast to southwest",
    "east to west",
    "southeast to northwest",

    "clockwise loop",
    "counter clockwise loop"
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// class Node

static inline bool IsIntersectedAngle(int angle)
{
    return (angle >= 30) && (angle <= 150);
}

bool Node::IsCorssroad(HIGHWAY_TYPE way_type) const
{
    if (this->is_way_connector_) {
        // connected_segments_[] is already orderred by way type
        const auto& p_conn_seg0 = connected_segments_[0];
        if (p_conn_seg0->way_type_ <= way_type || HIGHWAY_UNKNOWN == way_type) {
            for (const auto &p_segment : connected_segments_) {
                // surely p_segment is non-null
                if (p_segment->way_id_ != p_conn_seg0->way_id_ &&
                    (p_segment->way_type_ <= way_type || HIGHWAY_UNKNOWN == way_type))
                {
                    int angle = WayManager::GetAngle(p_segment->heading_, p_conn_seg0->heading_);
                    if (IsIntersectedAngle(angle)) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

SegmentPtr Node::GetIntersectedSegment(const SEGMENT& seg) const
{
    if (!this->is_way_connector_) {
        return nullptr;
    }

    int heading = (int)geo::get_heading_in_degree(seg.from_lat, seg.from_lng,
        seg.to_lat, seg.to_lng);

    for (const auto &p_segment : connected_segments_) {
        // surely p_segment is non-null
        if (p_segment->way_id_ == seg.way_id) {
            continue;
        }
        if (!seg.way_name.empty() && seg.way_name == p_segment->way_name_) {
            continue;
        }
        int angle = WayManager::GetAngle(p_segment->heading_, heading);
        if (IsIntersectedAngle(angle)) {
            return p_segment;
        }
    }

    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// class Segment

// Returns the square meters of distance
double Segment::CalcDistanceSquareMeters(const geo::GeoPoint &coord, const Segment &seg)
{
    //
    //                   o p (coord)
    //
    //
    //              o------------------>o
    //              a (from)   seg      b (to)
    //

    double abx = seg.to_point_.lng - seg.from_point_.lng;
    double aby = seg.to_point_.lat - seg.from_point_.lat;
    double ab2 = abx * abx + aby * aby;
    if (ab2 <= 10e-12) {
        // segment's "from" and "to" node are too close
        return geo::distance_in_meter(seg.from_point_, coord);
    }

    double apx = coord.lng - seg.from_point_.lng;
    double apy = coord.lat - seg.from_point_.lat;
    double ap_ab = apx * abx + apy * aby;
    double t = ap_ab / ab2;
    if (t < 0) {
        t = 0;
    }
    else if (t > 1) {
        t = 1;
    }

    double r1 = (coord.lng - (seg.from_point_.lng + abx * t)) * LAT_METERS_PER_DEGREE *
        cos(coord.lat * M_PI / 180.0);
    double r2 = (coord.lat - (seg.from_point_.lat + aby * t)) * LAT_METERS_PER_DEGREE;
    return r1*r1 + r2*r2;
}

double Segment::DistanceSquareMeters(const Node& node) const
{
    return CalcDistanceSquareMeters(node.geo_point_, *this);
}

double Segment::DistanceSquareMeters(const geo::GeoPoint& point) const
{
    return CalcDistanceSquareMeters(point, *this);
}

double Segment::GetDistanceInMeters(const Segment& seg)
{
    double dist_from_2 = this->DistanceSquareMeters(*seg.p_from_nd_);
    double dist_to_2 = this->DistanceSquareMeters(*seg.p_to_nd_);
    double min2 = (dist_from_2 < dist_to_2) ? dist_from_2 : dist_to_2;
    return sqrt(min2);
}

double Segment::CalcDistance(const geo::GeoPoint &coord, const Segment &seg)
{
    //
    //                   o p (coord)
    //
    //
    //              o------------------>o
    //              a (from)   seg      b (to)
    //

    double abx = seg.to_point_.lng - seg.from_point_.lng;
    double aby = seg.to_point_.lat - seg.from_point_.lat;
    double ab2 = abx * abx + aby * aby;
    if (ab2 <= 10e-12) {
        // segment's "from" and "to" node are too close
        return geo::distance_in_meter(seg.from_point_, coord);
    }

    double apx = coord.lng - seg.from_point_.lng;
    double apy = coord.lat - seg.from_point_.lat;
    double ap_ab = apx * abx + apy * aby;
    double t = ap_ab / ab2;
    if (t < 0) {
        t = 0;
    }
    else if (t > 1) {
        t = 1;
    }

    double r1 = (coord.lng - (seg.from_point_.lng + abx * t)) * LAT_METERS_PER_DEGREE *
        cos(coord.lat * M_PI / 180.0);
    double r2 = (coord.lat - (seg.from_point_.lat + aby * t)) * LAT_METERS_PER_DEGREE;
    return sqrt(r1*r1 + r2*r2);
}

void Segment::GetOffsetLinestr(const std::vector<SegmentPtr>& segs, double offset,
    std::vector<GeoPoint>& offset_points)
{
    offset_points.clear();

    if (!segs.empty()) {
        std::vector<geo::GeoPoint> points0;
        std::vector<bool> one_ways;
        points0.reserve(segs.size() + 1);
        one_ways.reserve(segs.size() + 1);

        for (auto& p_seg : segs) {
            points0.push_back(p_seg->from_point_);
            one_ways.push_back(p_seg->one_way_);
        }
        points0.push_back(segs.back()->to_point_);
        one_ways.push_back(segs.back()->one_way_);

        offset_points.reserve(segs.size() + 1);
        geo::get_offset_linestr(points0, one_ways, offset, offset_points);
    }
}

double Segment::AdjustOffset(double offset, HIGHWAY_TYPE type)
{
    // some of below entries may need future adjustments
    static const double type_adjustments[] = {
        1.0, // unknown
        1.0, // motorway
        1.03, // trunk
        1.0, // primary
        1.0, // secondary
        1.0, // tertiary
        0.73, // unclassified
        0.65, // residential
        0.3, // service
        1.0, // none
        1.0, // none
        1.0, // none

        0.7, // motorway_link
        0.85, // trunk_link
        0.9, // primary_link
        0.9, // secondary_link
        0.9, // tertiary_link
        1.0, // none
        1.0, // none
        1.0, // none

        0.75, // living_street
        1.0, // pedestrian
        0.3, // track
        1.0, // bus_guideway
        1.0, // raceway
        1.0, // road
        1.0, // none
        1.0, // none
        1.0, // none

        1.0, // footway
        1.0, // bridleway
        1.0, // steps
        1.0, // path
        1.0, // none
        1.0, // none

        1.0, // cycleway
        1.0, // none
        1.0, // none

        1.0, // proposed
        1.0, // construction
        1.0, // none
        1.0, // none
    };
    static_assert(sizeof(type_adjustments) / sizeof(type_adjustments[0]) == (int)HIGHWAY_TYPE_MAX,
        "Element count in type_adjustments[] must match HIGHWAY_TYPE enum");

    if (type >= 0 && type <= HIGHWAY_TYPE_MAX) {
        return offset * type_adjustments[type];
    }
    else {
        return offset;
    }
}

bool Segment::GetTagByName(const std::string& tag_name, std::string* p_tag_value) const
{
    if (p_opt_tags_ == nullptr || p_opt_tags_->empty()) {
        return false;
    }
    for (const auto &tag : *p_opt_tags_) {
        if (tag.name == tag_name) {
            if (p_tag_value) {
                *p_tag_value = tag.value;
            }
            return true;
        }
    }
    return false;
}

#define TAG_DELIMETER   ','
std::string Segment::TagsToStr(const Tags& tags)
{
    if (tags.empty()) {
        return std::string();
    }

    std::string result;
    const size_t tags_size = tags.size();
    std::string tag_str;
    tag_str.reserve(64);
    for (size_t i = 0; i < tags_size; ++i) {
        const auto &tag = tags[i];
        const bool has_delimeter = (std::string::npos != tag.value.find(TAG_DELIMETER))
            || (std::string::npos != tag.name.find(TAG_DELIMETER));

        if (i != 0) {
            tag_str = TAG_DELIMETER;
        }
        else {
            tag_str.clear();
        }
        if (has_delimeter) {
            tag_str += '\"';
        }

        tag_str += tag.name;
        if (!tag.value.empty()) {
            tag_str += '=';
            tag_str += tag.value;
        }

        if (has_delimeter) {
            tag_str += '\"';
        }

        result += tag_str;
    }

    return result;
}

SharedTagsPtr Segment::StrToTags(const std::string &tags_str)
{
    std::vector<std::string> tags, kvs;
    util::ParseCsvLine(tags, tags_str, TAG_DELIMETER);
    if (tags.empty()) {
        return nullptr;
    }

    SharedTagsPtr p_tags = std::make_shared<Tags>();
    kvs.reserve(2);
    p_tags->reserve(tags.size());
    Tag tag;
    for (auto &tag_str : tags) {
        kvs.clear();
        util::ParseCsvLine(kvs, tag_str, '=');
        const auto kvs_size = kvs.size();
        if (kvs_size == 0) {
            continue;
        }

        if (kvs_size >= 1) {
            tag.name = kvs.front();
        }
        if (kvs_size >= 2) {
            tag.value = kvs[1];
        }
        else {
            tag.value.clear();
        }

        if (!tag.name.empty()) {
            p_tags->push_back(tag);
        }
    }

    return p_tags;
}
#undef TAG_DELIMETER

///////////////////////////////////////////////////////////////////////////////////////////////////
// class OrientedWay

void OrientedWay::AddSegment(const SegmentPtr& p_seg)
{
    for (auto& seg : segments_) {
        if (seg->seg_id_ == p_seg->seg_id_) {
            return;
        }
    }
    segments_.push_back(p_seg);
}

void OrientedWay::DoneAddSegment(const WayManager& way_manager)
{
    // for Segment's missing members
    for (auto& p_seg : segments_) {
        p_seg->p_ori_way_ = this;
    }

    if (!segments_.empty()) {
        name_ = segments_.front()->way_name_;
    }

    // order the segments in sequence
    std::sort(segments_.begin(), segments_.end(),
        [](const SegmentPtr& i, const SegmentPtr& j) {
        return (i->way_sub_seq_ == j->way_sub_seq_) ?
            (i->split_seq_ < j->split_seq_) : (i->way_sub_seq_ < j->way_sub_seq_);
    });
    // for fake reverse segments, reverse the sequence
    if (segments_.front()->seg_id_ < 0) {
        auto size = segments_.size();
        for (size_t i = 0; i < size / 2; ++i) {
            std::swap(segments_[i], segments_[size - 1 - i]);
        }
    }

    // populate nodes_[]
    nodes_.clear();
    if (!segments_.empty()) {
        nodes_.reserve(segments_.size() + 1);
        for (auto const& p_seg : segments_) {
            nodes_.push_back(way_manager.GetNodeById(p_seg->from_nd_));
        }
        nodes_.push_back(way_manager.GetNodeById(segments_.back()->to_nd_));
    }

    // for the opposite way
    if (!one_way_ && p_opposite_way_ == nullptr) {
        auto p_op_way = way_manager.GetWayById(-way_id_);
        if (p_op_way != nullptr) {
            p_opposite_way_ = p_op_way;
        }
    }

    // for the bound box
    bbox_.minlat = bbox_.maxlat = segments_.front()->from_point_.lat;
    bbox_.minlng = bbox_.maxlng = segments_.front()->from_point_.lng;
    for (const auto& p_seg : segments_) {
        bbox_.Expand(p_seg->to_point_);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// class WayManager

bool WayManager::InitWayMap()
{
#if WAY_MANAGER_HANA_LOG == 1
    hana::Logger logger("WayManager");
    auto start0 = std::chrono::system_clock::now();
#endif

    way_map_.clear();

    // get oriented way number
    UNORD_SET<WAY_ID_T> ori_way_set;
    WAY_ID_T last_way_id = 0;
    for (auto& it : seg_map_) {
        const SegmentPtr& p_seg = it.second;
        auto signed_way_id = p_seg->GetWayIdOriented();
        if (signed_way_id != last_way_id) {
            ori_way_set.insert(signed_way_id);
            last_way_id = signed_way_id;
        }
    }
    ori_way_pool_.Reserve(ori_way_set.size());
    ori_way_set = std::move(decltype(ori_way_set)()); // destory

#if WAY_MANAGER_HANA_LOG == 1
    std::chrono::duration<double> elapsed = std::chrono::system_clock::now() - start0;
    HANA_SDK_DEBUG(logger) << "WayManager::InitWayMap() part 1: run time " << elapsed.count() << " seconds" << hana::endl;
    auto start = std::chrono::system_clock::now();
#endif

    OrientedWayPtr p_way = nullptr;
    for (auto& it : seg_map_) {
        const SegmentPtr& p_seg = it.second;

        // NOTE: for opposite oriented way, way ID is negative
        WAY_ID_T way_id = p_seg->GetWayIdOriented();
        if (p_way == nullptr || p_way->WayId() != way_id) {
            OrientedWayPtr& way = way_map_[way_id];
            if (nullptr == way) {
                way = ori_way_pool_.AllocNew(OrientedWay(way_id, p_seg->one_way_ != 0));
            }
            p_way = way;
        }
        p_way->AddSegment(p_seg);
    }

#if WAY_MANAGER_HANA_LOG == 1
    elapsed = std::chrono::system_clock::now() - start;
    HANA_SDK_DEBUG(logger) << __FUNCTION__ << " part 2: run time "
        << elapsed.count() << " seconds" << hana::endl;
    start = std::chrono::system_clock::now();
#endif

    for (auto& it : way_map_) {
        it.second->DoneAddSegment(*this);
    }

#if WAY_MANAGER_HANA_LOG == 1
    elapsed = std::chrono::system_clock::now() - start0;
    HANA_SDK_DEBUG(logger) << "WayManager::InitWayMap(): run time "
        << elapsed.count() << " seconds" << hana::endl;
#endif
    return true;
}

// for two-way case, way_sub_seq and split_seq are negative
static SEG_ID_T GenerateSegID(WAY_ID_T way_id, short way_sub_seq, short split_seq)
{
    SEG_ID_T seg_id = (way_id << 24);
    unsigned int low24 = ((unsigned int)((unsigned short)way_sub_seq) << 8) |
        (unsigned int)(unsigned char)(char)(split_seq);
    seg_id |= low24;
    return seg_id;
}

// return the revsersed segment whose from/to nodes are the to/from nodes of the p_seg
SegmentPtr WayManager::GetReversedSeg(const SegmentPtr& p_seg) const
{
    if (p_seg->one_way_) {
        return GetSegById(-p_seg->seg_id_);
    }
    else {
        SEG_ID_T rev_seg_id = GenerateSegID(p_seg->way_id_, -p_seg->way_sub_seq_,
            -p_seg->split_seq_);
        return GetSegById(rev_seg_id);
    }
    return nullptr;
}

bool WayManager::SegsToJson(const Bound& bound, double offset, const std::string& seg_color,
    const std::string& node_color, const std::string& pathname) const
{
    std::vector<SegmentPtr> segs;
    segs.reserve(1024 * 2);

    for (const auto& seg_elem : this->seg_map_) {
        const auto& p_seg = seg_elem.second;
        if (!bound.Empty() &&
            bound.OutOfBound(p_seg->from_point_) && bound.OutOfBound(p_seg->to_point_)) {
            continue;
        }
        segs.push_back(p_seg);
    }

    return SegsToJson(segs, offset, seg_color, node_color, pathname);
}

bool WayManager::SegsToJson(const std::vector<SegmentPtr>& segs,
    double offset, const std::string& seg_color, const std::string& node_color,
    const std::string& pathname) const
{
    std::set<NODE_ID_T> nodes_set;
    geo::GeoJSON geo_json;

    for (const auto& p_seg : segs) {
        std::shared_ptr<GeoObj_LineString> p_line = std::make_shared<GeoObj_LineString>();
        if (p_seg->one_way_) {
            p_line->AddPoint(p_seg->from_point_);
            p_line->AddPoint(p_seg->to_point_);
        }
        else {
            GeoPoint off_from, off_to;
            geo::get_offset_segment(p_seg->from_point_, p_seg->to_point_, offset,
                off_from, off_to);
            p_line->AddPoint(off_from);
            p_line->AddPoint(off_to);
        }
        p_line->AddProp("seg_id", p_seg->seg_id_);
        p_line->AddProp("way_id", p_seg->way_id_);
        p_line->AddProp("one_way", p_seg->one_way_);
        p_line->AddProp("length", p_seg->length_);
        p_line->AddProp("heading", p_seg->heading_);
        p_line->AddProp("way_name/type", (p_seg->way_name_.empty() ? "<null>" : p_seg->way_name_)
            + '/' + std::to_string(p_seg->way_type_));
        p_line->AddProp("from/to nodes", std::to_string(p_seg->from_nd_) + '/' +
            std::to_string(p_seg->to_nd_));
        p_line->AddProp("structure/layer", std::to_string(p_seg->struct_type_) + '/' +
            std::to_string(p_seg->layer_));

        p_line->AddProp("color", seg_color);
        geo_json.AddObj(p_line);

        NODE_ID_T nodes[2] {p_seg->from_nd_, p_seg->to_nd_};
        for (int i = 0; i < 2; ++i) {
            if (nodes_set.find(nodes[i]) == nodes_set.end()) {
                nodes_set.insert(nodes[i]);

                auto&& p_node = this->GetNodeById(nodes[i]);
                auto&& p_point = std::make_shared<geo::GeoObj_Point>(p_node->geo_point_);
                p_point->AddProp("node", p_node->nd_id_);
                p_point->AddProp("name", p_node->nd_name_);
                p_point->AddProp("coord", std::to_string(p_node->geo_point_.lat) + ',' +
                    std::to_string(p_node->geo_point_.lng));
                p_point->AddProp("way_connector", p_node->IsWayConnector());
                p_point->AddProp("connected_segs", (int)p_node->ConnectedSegments().size());
                p_point->AddProp("crossroad", p_node->IsCorssroad());
                if (p_node->IsRoutingNode()) {
                    p_point->AddProp("color", "gold");
                }
                else {
                    p_point->AddProp("color", node_color);
                }
                geo_json.AddObj(p_point);
            }
        }
    }

    return geo_json.ToJsonFile(pathname);
}

double WayManager::ProjectionLengthOnSegRoute(const GeoPoint& p1, const GeoPoint& p2,
    const std::vector<SegmentPtr>& seg_route,
    double* p_first_proj_len, double* p_last_proj_len)
{
    if (seg_route.empty()) {
        return 0;
    }
    double total_proj_length = 0;

    // for the 1st segment
    {
        const double proj_length = get_projection_distance_in_meter(p1,
            seg_route.front()->from_point_, seg_route.front()->to_point_, false);
        if (p_first_proj_len) {
            *p_first_proj_len = proj_length;
        }
        total_proj_length += proj_length;
    }

    // for the last segment
    {
        const double proj_length = get_projection_distance_in_meter(p2,
            seg_route.back()->from_point_, seg_route.back()->to_point_, true);
        if (p_last_proj_len) {
            *p_last_proj_len = proj_length;
        }
        total_proj_length += proj_length;
    }

    // exclude the first and last segments
    const int seg_count = (int)seg_route.size();
    for (int i = 1; i < seg_count - 1; ++i) {
        total_proj_length += seg_route[i]->length_;
    }

    return total_proj_length;
}

double WayManager::ProjectionLengthOnSegRoute(const GeoPoint& p1, const GeoPoint& p2,
    const std::vector<SegmentPtr>& seg_route)
{
    return ProjectionLengthOnSegRoute(p1, p2, seg_route, nullptr, nullptr);
}


static UNORD_MAP<std::string, HIGHWAY_TYPE> gHighwayToType = {
    { "motorway", HIGHWAY_MOTORWAY },
    { "trunk", HIGHWAY_TRUNK },
    { "primary", HIGHWAY_PRIMARY },
    { "secondary", HIGHWAY_SECONDARY },
    { "tertiary", HIGHWAY_TERTIARY },
    { "unclassified", HIGHWAY_UNCLASSIFIED },
    { "residential", HIGHWAY_RESIDENTIAL },
    { "service", HIGHWAY_SERVICE },

    { "motorway_link", HIGHWAY_MOTORWAY_LINK },
    { "trunk_link", HIGHWAY_TRUNK_LINK },
    { "primary_link", HIGHWAY_PRIMARY_LINK },
    { "secondary_link", HIGHWAY_SECONDARY_LINK },
    { "tertiary_link", HIGHWAY_TERTIARY_LINK },

    { "living_street", HIGHWAY_LIVING_STREET },
    { "pedestrian", HIGHWAY_PEDESTRIAN },
    { "track", HIGHWAY_TRACK },
    { "bus_guideway", HIGHWAY_BUS_GUIDEWAY },
    { "raceway", HIGHWAY_RACEWAY },
    { "road", HIGHWAY_ROAD },

    { "footway", HIGHWAY_FOOTWAY },
    { "bridleway", HIGHWAY_BRIDLEWAY },
    { "steps", HIGHWAY_STEPS },
    { "path", HIGHWAY_PATH },

    { "cycleway", HIGHWAY_CYCLEWAY },

    { "proposed", HIGHWAY_PROPOSED },
    { "construction", HIGHWAY_CONSTRUCTION },
};

HIGHWAY_TYPE WayManager::HighwayStrToType(const std::string& highway_type_str)
{
    auto it = gHighwayToType.find(highway_type_str);
    return it == gHighwayToType.cend() ? HIGHWAY_UNKNOWN : it->second;
}

const char* WayManager::HighwayTypeToStr(HIGHWAY_TYPE type)
{
    static const char *highway_type_strs[] = {
        "unknown",
        "motorway",
        "trunk",
        "primary",
        "secondary",
        "tertiary",
        "unclassified",
        "residential",
        "service",
        "",
        "",
        "",

        "motorway_link",
        "trunk_link",
        "primary_link",
        "secondary_link",
        "tertiary_link",
        "",
        "",
        "",

        "living_street",
        "pedestrian",
        "track",
        "bus_guideway",
        "raceway",
        "road",
        "",
        "",
        "",

        "footway",
        "bridleway",
        "steps",
        "path",
        "",
        "",

        "cycleway",
        "",
        "",

        "proposed",
        "construction",
        "",
        "",
    };
    static_assert(sizeof(highway_type_strs) / sizeof(highway_type_strs[0]) == (int)HIGHWAY_TYPE_MAX,
        "Element count in highway_type_strs[] must match HIGHWAY_TYPE enum");

    return highway_type_strs[type];
}

int WayManager::HighWayTypeToCategory(HIGHWAY_TYPE type)
{
    switch (type) {
    case geo::HIGHWAY_MOTORWAY:
        return 1;
    case geo::HIGHWAY_TRUNK:
    case geo::HIGHWAY_PRIMARY:
        return 2;
    default:
        return 3;
    }

    return 0;
}

// calculate the loop orientation.
// precondition: loop road
static ORIENTATION CalcLoopOrientation(const std::vector<geo::GeoPoint> &road_nodes)
{
    geo::ORIENTATION orientation = geo::INVALID_ORIENTATION;
    // refer to http://debian.fmi.uni-sofia.bg/~sergei/cgsr/docs/clockwise.htm
    if (road_nodes.size() >= 3) {
        size_t n = road_nodes.size();
        int count = 0;
        for (size_t i = 0; i < n - 1; i++) {
            size_t j = (i + 1) % n;
            size_t k = (i + 2) % n;
            double z = (road_nodes[j].lng - road_nodes[i].lng) *
                (road_nodes[k].lat - road_nodes[j].lat);
            z -= (road_nodes[j].lat - road_nodes[i].lat) * (road_nodes[k].lng - road_nodes[j].lng);
            if (z < 0) {
                count--;
            }
            else if (z > 0) {
                count++;
            }
        }
        if (count > 0) {
            orientation = geo::COUNTER_CLOCKWISE_LOOP;
        }
        else if (count < 0) {
            orientation = geo::CLOCKWISE_LOOP;
        }
        else {
            orientation = geo::INVALID_ORIENTATION;
        }
    }
    return orientation;
}

ORIENTATION WayManager::CalcOrientation(const std::vector<geo::GeoPoint> &path,
    double begin_end_threshold)
{
    size_t point_count = path.size();
    if (point_count <= 1) {
        return geo::INVALID_ORIENTATION;
    }

    geo::ORIENTATION orientation = geo::INVALID_ORIENTATION;
    const auto &point1 = path.front();
    const auto &point2 = path.back();
    double distance = geo::distance_in_meter(point1, point2);
    if (distance > begin_end_threshold) {
        double heading = geo::get_heading_in_degree(point1, point2);
        orientation = geo::WayManager::HeadingToORIENTATION(int(heading + 0.5));
    }
    else {
        // if the 1st point and last point are too close, consider it as a loop
        orientation = CalcLoopOrientation(path);
    }
    return orientation;
}

// return 0, if east to west, or south to north, or clock-wise
// return 1, if west to east, or north to south, or counter clock-wise
// return -1, error
int WayManager::CalcDirection01(const std::vector<geo::GeoPoint> &path,
    double begin_end_threshold)
{
    size_t point_count = path.size();
    if (point_count <= 1) {
        return -1;
    }

    const auto &point1 = path.front();
    const auto &point2 = path.back();
    double distance = geo::distance_in_meter(point1, point2);
    if (distance > begin_end_threshold) {
        double heading = geo::get_heading_in_degree(point1, point2);
        return (heading > 45 && heading <= 235) ? 1 : 0;
    }
    else {
        // if the 1st point and last point are too close, consider it as a loop
        auto orientation = CalcLoopOrientation(path);
        switch (orientation) {
        case CLOCKWISE_LOOP: return 0;
        case COUNTER_CLOCKWISE_LOOP: return 1;
        default: break;
        }
    }
    return -1;
}

ORIENTATION WayManager::HeadingToORIENTATION(int heading)
{
    ORIENTATION orientation;
    const int N_DIR = 8;
    switch ((int)((heading % 360)/ (360.0 / (N_DIR * 2))))
    {
    case 15:
    case 0:
        orientation = geo::SOUTH_TO_NORTH;
        break;
    case 1:
    case 2:
        orientation = geo::SOUTHWEST_TO_NORTHEAST;
        break;
    case 3:
    case 4:
        orientation = geo::WEST_TO_EAST;
        break;
    case 5:
    case 6:
        orientation = geo::NORTHWEST_TO_SOUTHEAST;
        break;
    case 7:
    case 8:
        orientation = geo::NORTH_TO_SOUTH;
        break;
    case 9:
    case 10:
        orientation = geo::NORTHEAST_TO_SOUTHWEST;
        break;
    case 11:
    case 12:
        orientation = geo::EAST_TO_WEST;
        break;
    case 13:
    case 14:
    default:
        orientation = geo::SOUTHEAST_TO_NORTHWEST;
        break;
    };
    return orientation;
}

void WayManager::FlagWeakConnectivity(util::SimpleObjPool<Node>& node_pool)
{
    const int n = (int)node_pool.Size();
    if (n == 0) return;

    // map each component's node ID to (component ID, node pool index)
    UNORD_MAP<NODE_ID_T, std::tuple<int, int>> node_to_component_id;
    node_to_component_id.reserve(n);
    std::vector<size_t> component_sizes; // sizes of all components
    std::vector<NodePtr> component(n); // component for weak connection

    int component_id = 0;
    for (int i = 0; i < n; ++i) {
        Node& seed_node = node_pool[i];
        if (node_to_component_id.find(seed_node.nd_id_) != node_to_component_id.end()) {
            continue;
        }

        // reset Node::is_visited
        for (int i = 0; i < n; ++i) {
            node_pool[i].is_visited = 0;
        }
        seed_node.is_visited = 1;
        size_t component_size = 0;
        component[component_size++] = &seed_node;

        for (size_t i = 0; i < component_size; ++i) {
            auto& p_node = component[i];

            for (auto& p_seg : p_node->connected_segments_) {
                if (!p_seg->p_from_nd_->is_visited) {
                    p_seg->p_from_nd_->is_visited = 1;
                    component[component_size++] = p_seg->p_from_nd_;
                }
                else if (!p_seg->p_to_nd_->is_visited) {
                    p_seg->p_to_nd_->is_visited = 1;
                    component[component_size++] = p_seg->p_to_nd_;
                }
            }
        }

        if ((int)component_size >= n / 2) {
            // this must be the main component. set flag for each node
            for (size_t i = 0; i < component_size; ++i) {
                component[i]->is_weak_connected = 1;
            }
            return;
        }
        else if (component_size > 0) {
            for (size_t k = 0; k < component_size; ++k) {
                node_to_component_id[component[k]->nd_id_] = std::make_tuple(component_id, i);
            }
            component_sizes.push_back(component_size);

            component_id++;
        }
    }

    if (component_sizes.empty()) {
        // should never happen
        throw std::runtime_error("graph weak connectivity main component not found");
    }

    // get the main component ID
    component_id = 0;
    size_t size = component_sizes[0];
    for (size_t i = 1; i < component_sizes.size(); ++i) {
        if (size < component_sizes[i]) {
            component_id = (int)i;
            size = component_sizes[i];
        }
    }

    // set flag for each node
    for (const auto& it : node_to_component_id) {
        int comp_id, i_pool;
        std::tie(comp_id, i_pool) = it.second;
        if (comp_id == component_id) {
            node_pool[i_pool].is_weak_connected = 1;
        }
    }
}

void WayManager::FlagNodeInternals(util::SimpleObjPool<Node>& node_pool)
{
    // for all Nodes' internal members
    const int size = (int)node_pool.Size();
    for (int i = 0; i < size; ++i) {
        auto& node = node_pool[i];
        const std::vector<SegmentPtr>& conn_segs = node.connected_segments_;

        // init is_way_connector_
        if (node.nd_id_ > 0 && conn_segs.size() >= 2) {
            auto& p_conn_seg0 = conn_segs.front();

            // to init all Node::is_way_connector_
            for (auto& p_conn_seg : conn_segs) {
                if (p_conn_seg->way_id_ != p_conn_seg0->way_id_) {
                    node.is_way_connector_ = 1;
                    break;
                }
            }
        }

        // init is_dead_end_
        if (conn_segs.size() == 1) {
            node.is_dead_end_ = 1;
        }
        else if (conn_segs.size() == 2) {
            if (WayManager::GetAngle(conn_segs.front()->heading_, conn_segs.back()->heading_) == 180) {
                node.is_dead_end_ = 1;
            }
        }
    }
}

void WayManager::DoneAddNodeMap(util::SimpleObjPool<Node>& node_pool)
{
#if WAY_MANAGER_HANA_LOG == 1
    hana::Logger logger("WayManager");
    auto start = std::chrono::system_clock::now();
    AT_SCOPE_EXIT(std::chrono::duration<double> elapsed = std::chrono::system_clock::now() - start;
    HANA_SDK_DEBUG(logger) << __FUNCTION__ << ": run time " << elapsed.count() << " seconds" << hana::endl;);
#endif

    int n_nodes = (int)node_pool.Size();
    int task_count = std::min((int)std::thread::hardware_concurrency(), (int)8);
    if (task_count < 2) task_count = 2;
    std::vector<std::tuple<int, int>> blocks;
    if (n_nodes > 1024) {
        blocks = util::SplitIntoSubsBlocks(task_count, n_nodes);
    }
    else {
        blocks.push_back(std::make_tuple(0, n_nodes - 1));
    }
    auto work_to_do = [&node_pool, &blocks](size_t i_block) {
        int start, end;
        std::tie(start, end) = blocks[i_block];

        NodePtr p_node = node_pool.ObjPtrByIndex(start);
        for (int i = start; i <= end; ++i, ++p_node) {
            // sort the connected segments by highway type
            if (p_node->nd_id_ > 0) {
                std::sort(p_node->connected_segments_.begin(), p_node->connected_segments_.end(),
                    [](const SegmentPtr& i, const SegmentPtr& j) {
                    return i->way_type_ < j->way_type_;
                });
            }
        }
    };

    if (blocks.size() > 1) {
        std::vector<std::shared_ptr<std::thread>> threads(blocks.size());
        for (size_t i_block = 0; i_block < blocks.size(); ++i_block) {
            threads[i_block] = std::make_shared<std::thread>(work_to_do, i_block);
        }
        for (auto& t : threads) {
            t->join();
        }
    }
    else if (blocks.size() == 1) {
        work_to_do(0);
    }
    else {
        // should never happen
        throw std::runtime_error("WayManager::DoneAddNodeMap: empty sub blocks");
    }

    if (n_nodes > 1024) {
        auto f_node_internals = std::async(std::launch::async, [&node_pool]() {
            FlagNodeInternals(node_pool);
        });

        FlagWeakConnectivity(node_pool);
        f_node_internals.get();
    }
    else {
        FlagNodeInternals(node_pool);
        FlagWeakConnectivity(node_pool);
    }
}


// static
const std::string& WayManager::GetCurThreadErrStr(std::mutex& threads_err_mutex,
    UNORD_MAP<std::string, std::string>& threads_err_strs)
{
    std::string thread_id;
    std::stringstream ss;
    ss << std::this_thread::get_id();
    ss >> thread_id;

    std::lock_guard<std::mutex> guard(threads_err_mutex);
    return threads_err_strs[thread_id];
}

// static
void WayManager::SetCurThreadErrStr(std::mutex& threads_err_mutex,
    UNORD_MAP<std::string, std::string>& threads_err_strs,
    const std::string& str)
{
    std::string thread_id;
    std::stringstream ss;
    ss << std::this_thread::get_id();
    ss >> thread_id;

    std::lock_guard<std::mutex> guard(threads_err_mutex);
    threads_err_strs[thread_id] = str;
}

//Parser lines to segments
static string ParserLinesToSegs(const std::vector<char *>& lines, vector<SEGMENT>& segs,
    const string& in_segments_csv)
{
    //#SEG_ID, FROM_LAT, FROM_LNG, TO_LAT, TO_LNG, ONE_WAY, LENGTH, WAY_ID, WAY_SUB_SEQ, SPLIT_SEQ,
    // FROM_ND, TO_ND, WAY_TYPE, WAY_NAME, STRUCT_TYPE, LAYER, OPT_TAGS
    static const std::string STR_ONE("1");
    SEGMENT seg;
    std::vector<char *> subs;
    for (auto& p_line : lines) {
        if (!p_line || p_line[0] == '\0') {
            continue;
        }
        if (p_line[0] == '#') { // ignore comment line
            continue;
        }

        util::ParseCsvLineInPlace(subs, p_line, ',');
        if (subs.size() < 13) {
#ifdef _WIN32
            printf("WARNING: ignore invalid line: \"%s\" in file %s\n", p_line,
                in_segments_csv.c_str());
#endif
            continue;
        }
        seg.seg_id = std::atoll(subs[0]);
        if (0 == seg.seg_id) {
            return "Error in parsing segment ID: \"" + string(subs[0]) + '\"';
        }
        seg.from_lat = atof(subs[1]);
        seg.to_lat = atof(subs[3]);
        seg.from_lng = atof(subs[2]);
        seg.to_lng = atof(subs[4]);

        seg.one_way = (subs[5] == STR_ONE);
        seg.length = atof(subs[6]);
        seg.way_id = std::atoll(subs[7]);
        seg.way_sub_seq = atoi(subs[8]);
        seg.split_seq = atoi(subs[9]);
        seg.from_nd = std::atoll(subs[10]);
        seg.to_nd = std::atoll(subs[11]);
        seg.way_type = static_cast<geo::HIGHWAY_TYPE>(atoi(subs[12]));
        if (subs.size() >= 14) {
            seg.way_name = subs[13];
        }
        if (subs.size() >= 15) {
            seg.struct_type = static_cast<STRUCT_TYPE>(atoi(subs[14]));
        }
        if (subs.size() >= 16) {
            seg.layer = static_cast<short>(atoi(subs[15]));
        }
        if (subs.size() >= 17) {
            seg.opt_tags = subs[16];
        }

        segs.push_back(seg);
    }
    return string();
}

// multi-threaded version of LoadSegmentsCsvs()
// have checked in_segments_csvs and the string contains '*'
static bool LoadSegmentsCsvs_Multi(const string& in_segments_csvs, vector<SEGMENT> &segs,
    string& err)
{
#if WAY_MANAGER_HANA_LOG == 1
    hana::Logger logger("WayManager");
    auto start = std::chrono::system_clock::now();
    AT_SCOPE_EXIT(std::chrono::duration<double> elapsed = std::chrono::system_clock::now() - start;
    HANA_SDK_DEBUG(logger) << "LoadSegmentsCsv_Multi(): run time "
        << elapsed.count() << " seconds" << hana::endl;);
#endif
    vector<string> files;
    std::string in_csvs = in_segments_csvs;
    util::StringReplace(in_csvs, "/", "\\");
    size_t n_pos = in_csvs.rfind("\\");
    size_t end_index = (n_pos == std::string::npos) ? 0 : n_pos + 1;
    std::string base_dir = in_csvs.substr(0, end_index);
    if (false == util::FindFiles(in_csvs, files)){
        err = "Error: cannot find file: " + in_segments_csvs;
        return false;
    }
    size_t concurrency = (files.size() > std::thread::hardware_concurrency()) ?
        std::thread::hardware_concurrency() : files.size();
    if (concurrency == 0) concurrency = 2;

    struct block_data {
        vector<SEGMENT> block_segs;
        string block_err;
    };
    vector<block_data> seg_blocks(concurrency);
    std::vector<std::shared_ptr<std::thread> > threads(concurrency);
    for (size_t i = 0; i < concurrency; ++i) {
        threads[i] = std::make_shared<std::thread>(
            [&files, i, &seg_blocks, base_dir, concurrency]() {
            size_t j = i;
            block_data& bd = seg_blocks[i];
            std::string part_file;
            do { 
                if (!util::ReadAllFromFile(base_dir + files[j], part_file)) {
                    bd.block_err = "Error in opening " + base_dir + files[j];
                    return ;
                }
                bd.block_segs.reserve(part_file.size() / 110 * ((files.size() - 1) / concurrency + 1));
                std::vector<char *> lines;
                util::ParseCsvLineInPlace(lines, &part_file[0], '\n', true); // true means leave quotes alone
                bd.block_err = ParserLinesToSegs(lines, bd.block_segs, base_dir + files[j]);
                if (!bd.block_err.empty()) {
                    return;
                }

                j += concurrency;
            } while (j < files.size());
        });
    }
    // join all threads
    for (auto& p_thread : threads) {
        p_thread->join();
    }
    // total seg cuont
    size_t seg_count = 0;
    for (const auto& seg_block : seg_blocks) {
        seg_count += seg_block.block_segs.size();
        if (!seg_block.block_err.empty()) {
            err = seg_block.block_err;
            return false;
        }
    }
    // merge all segs
    segs = move(seg_blocks[0].block_segs);
    segs.reserve(seg_count);
    for (size_t i = 1; i < seg_blocks.size(); ++i) {
        if (!seg_blocks[i].block_segs.empty()) {
            segs.insert(segs.end(), std::make_move_iterator(seg_blocks[i].block_segs.begin()),
                std::make_move_iterator(seg_blocks[i].block_segs.end()));
        }
    }
    std::sort(segs.begin(), segs.end(), [](const SEGMENT& i, const SEGMENT& j) {
        if (i.way_id == j.way_id) {
            return i.seg_id < j.seg_id;
        }
        return i.way_id < j.way_id;
    });
    return true;
}

// multi-threaded version of LoadSegmentsCsv()
static bool LoadSegmentsCsv_Multi(const string& in_segments_csv, vector<SEGMENT> &segs,
    string& err)
{
#if WAY_MANAGER_HANA_LOG == 1
    hana::Logger logger("WayManager");
    auto start = std::chrono::system_clock::now();
    AT_SCOPE_EXIT(std::chrono::duration<double> elapsed = std::chrono::system_clock::now() - start;
    HANA_SDK_DEBUG(logger) << "LoadSegmentsCsv_Multi(): run time "
        << elapsed.count() << " seconds" << hana::endl;);
#endif
    std::string file_data;
    if (!util::ReadAllFromFile(in_segments_csv, file_data)) {
        err = "Error in opening file: " + in_segments_csv;
        return false;
    }

    std::vector<char *> blocks;
    auto concurrency = std::thread::hardware_concurrency();
    if (concurrency == 0) concurrency = 2;
    if (false == util::SplitBigData(&file_data[0], file_data.size(), '\n', concurrency, blocks)) {
        err = "No rows found in segments file: " + in_segments_csv;
        return false;
    }

    struct block_data {
        vector<SEGMENT> block_segs;
        string block_err;
    };
    vector<block_data> seg_blocks(blocks.size());
    std::vector<std::shared_ptr<std::thread> > threads(blocks.size());
    for (size_t i_block = 0; i_block < blocks.size(); ++i_block) {
        threads[i_block] = std::make_shared<std::thread>(
            [&file_data, i_block, &seg_blocks, &blocks, &in_segments_csv]() {
            char *p_block = blocks[i_block];
            block_data& bd = seg_blocks[i_block];
            bd.block_segs.reserve(file_data.size() / 110 / blocks.size()); // rough estimate

            std::vector<char *> lines;
            util::ParseCsvLineInPlace(lines, p_block, '\n', true); // true means leave quotes alone
            if (lines.empty()) {
                return;
            }
            bd.block_err = ParserLinesToSegs(lines, bd.block_segs, in_segments_csv);
            if (!bd.block_err.empty()) {
                return;
            }
        });
    }
    // join all threads
    for (auto& p_thread : threads) {
        p_thread->join();
    }

    // total seg cuont
    size_t seg_count = 0;
    for (const auto& seg_block : seg_blocks) {
        seg_count += seg_block.block_segs.size();
        if (!seg_block.block_err.empty()) {
            err = seg_block.block_err;
            return false;
        }
    }

    // merge all segs
    segs = move(seg_blocks[0].block_segs);
    segs.reserve(seg_count);
    for (size_t i = 1; i < seg_blocks.size(); ++i) {
        if (!seg_blocks[i].block_segs.empty()) {
            segs.insert(segs.end(), seg_blocks[i].block_segs.begin(), seg_blocks[i].block_segs.end());
        }
    }
    return true;
}

// static
bool WayManager::LoadSegmentsFromCsv(const string& in_segments_csv,
    vector<SEGMENT> &segs, string& err)
{
    if (in_segments_csv.find('*') != std::string::npos) {
        return LoadSegmentsCsvs_Multi(in_segments_csv, segs, err);
    }
    return LoadSegmentsCsv_Multi(in_segments_csv, segs, err);
}

bool WayManager::LoadSegments(const std::string &segs_csv, bool reverse_seg/*= false*/)
{
#if WAY_MANAGER_HANA_LOG == 1
    hana::Logger logger("WayManager");
    auto start = std::chrono::system_clock::now();
    AT_SCOPE_EXIT(std::chrono::duration<double> elapsed = std::chrono::system_clock::now() - start;
    HANA_SDK_DEBUG(logger) << "WayManager::LoadSegments(csv_file): run time "
        << elapsed.count() << " seconds" << hana::endl;);
#endif

    vector<SEGMENT> segs;
    std::string err;
    if (segs_csv.find('*') != std::string::npos) {
        if (false == LoadSegmentsCsvs_Multi(segs_csv, segs, err)) {
            this->SetErrorString(err);
            return false;
        }
    }
    else {
        if (false == LoadSegmentsCsv_Multi(segs_csv, segs, err)) {
            this->SetErrorString(err);
            return false;
        }
    }

    // segs[] are preferred to be orderred by "WAY_ID", "WAY_SUB_SEQ", "SPLIT_SEQ";
    util::ParSort(segs.begin(), segs.end(), [](const SEGMENT& i, const SEGMENT& j) {
        if (i.way_id != j.way_id) {
            return i.way_id < j.way_id;
        }
        if (i.way_sub_seq != j.way_sub_seq) {
            return i.way_sub_seq < j.way_sub_seq;
        }
        return i.split_seq < j.split_seq;
    }, 4);

    return LoadSegments(segs, reverse_seg);
}

// init all Node::connected_segments_ and all Segment::p_from_nd_, p_to_nd_
bool WayManager::InitNodeConnectedSegs()
{
#if WAY_MANAGER_HANA_LOG == 1
    hana::Logger logger("WayManager");
    auto start = std::chrono::system_clock::now();
    AT_SCOPE_EXIT(std::chrono::duration<double> elapsed = std::chrono::system_clock::now() - start;
    HANA_SDK_DEBUG(logger) << "WayManager::InitNodeConnectedSegs(): run time "
        << elapsed.count() << " seconds" << hana::endl;);
#endif

    for (const auto& entry : seg_map_) {
        const SegmentPtr& p_seg = entry.second;
        if (0 == p_seg->from_nd_|| 0 == p_seg->to_nd_) {
            SetErrorString("Error: WayManager: invalid from/to node: "
                + std::to_string(p_seg->from_nd_) + "/" + std::to_string(p_seg->to_nd_));
            return false;
        }

        if (nullptr == p_seg->p_from_nd_) {
            p_seg->p_from_nd_ = node_map_[p_seg->from_nd_];
            if (nullptr == p_seg->p_from_nd_) {
                SetErrorString("Error: WayManager: cannot get node info for "
                    + std::to_string(p_seg->from_nd_));
                return false;
            }
        }
        if (nullptr == p_seg->p_to_nd_) {
            p_seg->p_to_nd_ = node_map_[p_seg->to_nd_];
            if (nullptr == p_seg->p_to_nd_) {
                SetErrorString("Error: WayManager: cannot get node info for "
                    + std::to_string(p_seg->to_nd_));
                return false;
            }
        }

        // populate Node::connected_segments_, make sure no duplicated segments
        auto it = std::find_if(p_seg->p_from_nd_->connected_segments_.begin(),
            p_seg->p_from_nd_->connected_segments_.end(),
            [&p_seg](const SegmentPtr& s) {return p_seg->seg_id_ == s->seg_id_; });
        if (it == p_seg->p_from_nd_->connected_segments_.end()) {
            p_seg->p_from_nd_->connected_segments_.push_back(p_seg);
        }

        it = std::find_if(p_seg->p_to_nd_->connected_segments_.begin(),
            p_seg->p_to_nd_->connected_segments_.end(),
            [&p_seg](const SegmentPtr& s) {return p_seg->seg_id_ == s->seg_id_; });
        if (it == p_seg->p_to_nd_->connected_segments_.end()) {
            p_seg->p_to_nd_->connected_segments_.push_back(p_seg);
        }
    }

    return true;
}

static inline bool GetSegData(const SEGMENT& seg, WAY_ID_T& seg_way_id,
    geo::GeoPoint &from, geo::GeoPoint &to)
{
    if (seg.way_sub_seq < 0) {
        return false;
    }
    seg_way_id = seg.way_id;
    from.lat = seg.from_lat;
    from.lng = seg.from_lng;
    to.lat = seg.to_lat;
    to.lng = seg.to_lng;
    return true;
}

static inline bool GetSegData(const SEGMENT* p_seg, WAY_ID_T& seg_way_id,
    geo::GeoPoint &from, geo::GeoPoint &to)
{
    if (p_seg->way_sub_seq < 0) {
        return false;
    }
    seg_way_id = p_seg->way_id;
    from.lat = p_seg->from_lat;
    from.lng = p_seg->from_lng;
    to.lat = p_seg->to_lat;
    to.lng = p_seg->to_lng;
    return true;
}

struct WAY_BOUND
{
    WAY_ID_T way_id;
    geo::Bound bound;
};

template <typename SEG>
void GetWayBoundMap(const std::vector<SEG> &segs, std::vector<WAY_BOUND>& way_bounds)
{
#if WAY_MANAGER_HANA_LOG == 1
    hana::Logger logger("WayManager");
    auto start = util::GetTimeInMs64();
    AT_SCOPE_EXIT(auto elapsed = util::GetTimeInMs64() - start;
    HANA_SDK_DEBUG(logger) << "GetWayBoundMap(): run time "
        << elapsed << " ms" << hana::endl;);
#endif
    way_bounds.clear();
    way_bounds.reserve(segs.size() / 10);

    WAY_BOUND *p_bound, *p_last_bound = nullptr;
    WAY_ID_T seg_way_id, last_way_id = 0;
    geo::GeoPoint from, to;
    for (auto& seg : segs) {
        if (false == GetSegData(seg, seg_way_id, from, to)) {
            continue;
        }

        if (seg_way_id == last_way_id) {
            p_bound = p_last_bound;

            if (p_bound) {
                p_bound->bound.Expand(from.lat, from.lng);
                p_bound->bound.Expand(to.lat, to.lng);
            }
        }
        else {
            last_way_id = seg_way_id;
            WAY_BOUND way_bound = { seg_way_id };
            way_bounds.emplace_back(way_bound);
            p_bound = p_last_bound = &way_bounds.back();

            p_bound->bound.minlat = p_bound->bound.maxlat = from.lat;
            p_bound->bound.minlng = p_bound->bound.maxlng = from.lng;
            p_bound->bound.Expand(to.lat, to.lng);
        }
    }

    {
        // make sure is in order
        bool in_order = true;
        const size_t size = way_bounds.size();
        for (size_t i = 0; i < size - 1; ++i) {
            if (way_bounds[i].way_id > way_bounds[i + 1].way_id) {
                in_order = false;
                break;
            }
        }
        if (!in_order) {
            util::ParSort(way_bounds.begin(), way_bounds.end(),
                [](const WAY_BOUND &i, const WAY_BOUND &j){
                return i.way_id < j.way_id;
            });
        }
    }
};

// way_bounds should already been sorted by way ID, asc
static inline geo::Bound& GetWayBoundById(std::vector<WAY_BOUND>& way_bounds, WAY_ID_T way_id)
{
    WAY_BOUND way_bound = {way_id};
    auto it = std::lower_bound(way_bounds.begin(), way_bounds.end(), way_bound,
        [](const WAY_BOUND& i, const WAY_BOUND& j) {return i.way_id < j.way_id;});
    return it->bound;
}

// accept SEGMENT& and SEGMENT* and act as pointer
struct SEGMENT_PTR
{
    explicit SEGMENT_PTR(const SEGMENT& seg) : p_seg_(&seg)
    {}
    explicit SEGMENT_PTR(const SEGMENT* p_seg) : p_seg_(p_seg)
    {}

    const SEGMENT* operator->() const
    {
        return p_seg_;
    };

    const SEGMENT *p_seg_;
};

template <typename SEGMENT_T>
bool WayManager_LoadSegmentsT(WayManager &way_manager,
    const std::vector<SEGMENT_T> &segs, bool reversed_seg)
{
    const int num_rows = (int)segs.size();
    if (num_rows == 0) {
        way_manager.SetErrorString("Error: WayManager: too few segments to load");
        return false;
    }

    way_manager.node_map_.clear();
    way_manager.seg_map_.clear();
    way_manager.seg_map_.reserve(num_rows / 2);
    way_manager.node_map_.reserve(num_rows / 2);

    way_manager.node_pool_.Reserve(num_rows * 3 / 4);
    way_manager.seg_pool_.Reserve(num_rows);

#if WAY_MANAGER_HANA_LOG == 1
    hana::Logger logger("WayManager");
    auto start0 = std::chrono::system_clock::now();
#endif

    // if part of the way is in bound, make sure all the segments of the way ID are in bound
    std::vector<WAY_BOUND> way_bounds;
    GetWayBoundMap(segs, way_bounds);
    geo::WAY_ID_T last_inbound_way_id = 0, last_outbound_way_id = 0;

    NODE_ID_T last_from_id = 0, last_to_id = 0;
    std::string last_opt_tags_str;
    SharedTagsPtr p_opt_tags;

    for (auto const& one_seg : segs) {
        SEGMENT_PTR seg(one_seg);

        // boundary check
        bool in_bound;
        if (seg->way_id == last_inbound_way_id) {
            in_bound = true;
        }
        else if (seg->way_id == last_outbound_way_id) {
            in_bound = false;
        }
        else {
            geo::Bound& way_bound = GetWayBoundById(way_bounds, seg->way_id);
            if (way_bound.IntersectBound(way_manager.bound_)) {
                in_bound = true;
                last_inbound_way_id = seg->way_id;
            }
            else {
                in_bound = false;
                last_outbound_way_id = seg->way_id;
            }
        }
        if (!in_bound) {
            continue;
        }

        if (last_opt_tags_str != seg->opt_tags) {
            p_opt_tags = Segment::StrToTags(seg->opt_tags);
            last_opt_tags_str = seg->opt_tags;
        }

        // add the segment to segment map
        auto p_seg = way_manager.seg_pool_.AllocNew(Segment(seg->seg_id, seg->way_id,
            seg->way_sub_seq, seg->split_seq,
            seg->from_nd, seg->to_nd, seg->from_lat, seg->from_lng, seg->to_lat, seg->to_lng,
            seg->one_way, seg->way_type, seg->highway_type_str, seg->way_name,
            seg->struct_type, seg->layer, p_opt_tags));
        way_manager.seg_map_.insert({ seg->seg_id, p_seg });

        // add the nodes to node map
        if (last_to_id != p_seg->from_nd_) { // simply save time, no need to add the same node
            NodePtr& p_nd = way_manager.node_map_[p_seg->from_nd_];
            if (nullptr == p_nd) {
                p_nd = way_manager.node_pool_.AllocNew(
                    Node(p_seg->from_nd_, seg->from_lat, seg->from_lng));
            }
        }
        if (last_from_id != p_seg->to_nd_) { // simply save time, no need to add the same node
            NodePtr& p_nd = way_manager.node_map_[p_seg->to_nd_];
            if (nullptr == p_nd) {
                p_nd = way_manager.node_pool_.AllocNew(
                    Node(p_seg->to_nd_, seg->to_lat, seg->to_lng));
            }
        }

        last_from_id = p_seg->from_nd_;
        last_to_id = p_seg->to_nd_;
    }

#if WAY_MANAGER_HANA_LOG == 1
    std::chrono::duration<double> elapsed = std::chrono::system_clock::now() - start0;
    HANA_SDK_DEBUG(logger) << "WayManager::LoadSegments(segs) part 1: run time "
        << elapsed.count() << " seconds" << hana::endl;
#endif

    way_manager.InitNodeConnectedSegs();

    // for generation of fake reversed segments
    if (reversed_seg) {
        way_manager.GenerateReversedSegsIntoSegMap();

        // clear and re-generate all nodes' connectd segments
        for (auto& e : way_manager.node_map_) {
            e.second->ConnectedSegments().clear();
        }
        way_manager.InitNodeConnectedSegs();
    }

    way_manager.DoneAddNodeMap(way_manager.node_pool_);

    if (false == way_manager.InitWayMap()) {
        return false;
    }

    if (way_manager.seg_map_.empty()) {
        way_manager.SetErrorString("Error: too few segments or invalid bound");
        return false;
    }
    return true;
}

bool WayManager::LoadSegments(const std::vector<SEGMENT> &segs, bool reversed_seg)
{
    return WayManager_LoadSegmentsT(*this, segs, reversed_seg);
}

bool WayManager::LoadSegments(const std::vector<SEGMENT*> &p_segs, bool reversed_seg)
{
    return WayManager_LoadSegmentsT(*this, p_segs, reversed_seg);
}

// NOTE: the function adds offsets to the two oriented ways from any two-way segments
// so that they are not overlapped
std::shared_ptr<GeoObj_LineString> WayManager::SegmentToGsonLineString(const Segment& segment,
    double two_way_offset) // in meters
{
    std::shared_ptr<GeoObj_LineString> p_line_string = std::make_shared<GeoObj_LineString>();

    if (segment.one_way_) {
        p_line_string->AddPoint(segment.from_point_);
        p_line_string->AddPoint(segment.to_point_);
    }
    else {
        // for segment from two-way, add offset so that it is not overlapped
        // with its opposite segment
        GeoPoint offset_from, offset_to;
        geo::get_offset_segment(segment.from_point_, segment.to_point_, two_way_offset,
            offset_from, offset_to);
        p_line_string->AddPoint(offset_from);
        p_line_string->AddPoint(offset_to);
    }

    return p_line_string;
}

namespace seg {

typedef long long TILE_XY;

static const double CELL_SIZE = 200; // in meters


static inline bool InSameDirection(int heading1, int heading2, int angle_tollerance)
{
    int diff = heading2 + 360 - heading1;
    if (diff >= 360) {
        diff -= 360;
    }
    return diff <= angle_tollerance || diff >= (360 - angle_tollerance);
}

// Returns the square meters of distance
static double CalcDistanceSquareMeters(const geo::GeoPoint &coord, const Segment &seg)
{
    return geo::distance_point_to_segment_square(coord, seg.from_point_, seg.to_point_);
}

struct Tile
{
public:
    TILE_XY tile_id;
    std::vector<SegmentPtr> segments;
    std::vector<SegmentPtr> segments_with_neighbours;

public:
    Tile() : tile_id(0)
    {
        segments.reserve(16);
    }

    explicit Tile(TILE_XY tile_id) : tile_id(tile_id)
    {
        segments.reserve(16);
    }

    void AddSegment(const SegmentPtr& p_seg)
    {
        if (p_seg->length_ >= 0.1) {
            segments.push_back(p_seg);
        }
    }
};
typedef Tile* TilePtr;

// NOTE: this segment manager implementation merge two-way segment as only one. It merges them into
// one if two are found.
class SegmentManager
{
public:
    explicit SegmentManager(const SegmentMap& all_segs_map, const Bound& bound, int match_priority)
        : all_segs_map_(all_segs_map), bound_(bound), match_priority_(match_priority),
        local_utc_diff_(8 * 3600) // timezone default China
    {
        GRID_CELL_ZOOM_LEVEL = geo::span_to_zoom_level(CELL_SIZE, (bound.minlat + bound.maxlat) / 2);

        min_tile_x_ = geo::long2tilex(bound.minlng, GRID_CELL_ZOOM_LEVEL);
        min_tile_y_ = geo::lat2tiley(bound.maxlat, GRID_CELL_ZOOM_LEVEL);
        int max_tile_x = geo::long2tilex(bound.maxlng, GRID_CELL_ZOOM_LEVEL);
        int max_tile_y = geo::lat2tiley(bound.minlat, GRID_CELL_ZOOM_LEVEL);
        mat_width_ = max_tile_x - min_tile_x_ + 1;
        mat_height_ = max_tile_y - min_tile_y_ + 1;

        tile_mat_.SetSize(mat_height_, mat_width_);
        for (int y = 0; y < mat_height_; ++y) {
            for (int x = 0; x < mat_width_; ++x) {
                TILE_XY tile_id = geo::make_tilexy(x + min_tile_x_, y + min_tile_y_);
                tile_mat_(y, x).tile_id = tile_id;
            }
        }
    }

    bool LoadToTileMatrix()
    {
#if WAY_MANAGER_HANA_LOG == 1
        hana::Logger logger("WayManager");
        auto start0 = std::chrono::system_clock::now();
#endif

        for (auto& entry : all_segs_map_) {
            const SegmentPtr& p_seg = entry.second;
            this->AddSegmentForTiles(p_seg);
        }


#if WAY_MANAGER_HANA_LOG == 1
        std::chrono::duration<double> elapsed = std::chrono::system_clock::now() - start0;
        HANA_SDK_DEBUG(logger) << "LoadToTileMatrix() part 1: run time "
            << elapsed.count() << " seconds" << hana::endl;
#endif

        InitNeighbourSegs();
        return true;
    }

    const std::string& GetErrorString() const
    {
        return WayManager::GetCurThreadErrStr(threads_err_mutex_, threads_err_strs_);
    }

    SegmentPtr AssignSegment(const geo::GeoPoint& point, const SegAssignParams& params,
        SegAssignResults *p_results) const
    {
        if (p_results) {
            p_results->clear();
        }
        TilePtr p_tile = GetTileByPos(point);
        if (!p_tile) {
            WayManager::SetCurThreadErrStr(threads_err_mutex_, threads_err_strs_,
                "AssignSegment: coordinate's tile not in range");
            return nullptr;
        }

        const std::vector<SegmentPtr>& arrSegs = p_tile->segments_with_neighbours;
        const size_t MAX = 512 * 6;
        if (arrSegs.size() > MAX) {
            WayManager::SetCurThreadErrStr(threads_err_mutex_, threads_err_strs_,
                "AssignSegment: BUFFER SIZE TOO SMALL!!!, SEGMENTS NUMBER IS " +
                std::to_string(arrSegs.size()));
            return nullptr;
        }

        double aDistances[MAX];
        int aCandidatesIndexes[MAX];
        int candidateCount = 0;
        const double radius2 = params.radius * params.radius;

        const int segs_count = (int)arrSegs.size();
        for (int i = 0; i < segs_count; i++) {
            const SegmentPtr& pSeg = arrSegs[i];

            if (params.ignore_reverse_segs && pSeg->seg_id_ < 0) {
                continue;
            }
            if (params.excluded_seg_count != 0) {
                bool found_ignored = false;
                for (int i = 0; i < params.excluded_seg_count; ++i) {
                    if (params.excluded_seg_ids[i] == pSeg->seg_id_) {
                        found_ignored = true;
                        break;
                    }
                }
                if (found_ignored) {
                    continue;
                }
            }

            if (params.excluded_type_count != 0) {
                bool hit = false;
                for (int i = 0; i < params.excluded_type_count; ++i) {
                    if (params.excluded_types[i] == pSeg->way_type_) {
                        hit = true;
                        break;
                    }
                }
                if (hit) {
                    continue;
                }
            }

            if (params.included_layer_count != 0) {
                bool included = false;
                for (int i = 0; i < params.included_layer_count; ++i) {
                    if (params.included_layers[i] == pSeg->layer_) {
                        included = true;
                        break;
                    }
                }
                if (!included) {
                    continue;
                }
            }

            // check way name if provided
            if (params.way_name && params.way_name[0] != '\0') {
                if (pSeg->way_name_ != params.way_name) {
                    continue;
                }
            }

            // If not the same direction, ignore
            if (params.heading >= 0) {
                if (!InSameDirection(pSeg->heading_, params.heading, params.angle_tollerance)) {
                    continue;
                }
            }

            if (params.no_road_link) {
                if (pSeg->way_type_ >= HIGHWAY_MOTORWAY_LINK &&
                    pSeg->way_type_ <= HIGHWAY_SECONDARY_LINK) {
                    continue;
                }
            }
            if (params.no_bridge && pSeg->struct_type_ == STRUCT_BRIDGE) {
                continue;
            }
            if (params.no_tunnel && pSeg->struct_type_ == STRUCT_TUNNEL) {
                continue;
            }

            if (params.check_no_gps_route && pSeg->excluded_no_gps_) {
                continue;
            }

            // seg is excluded. e.g., some tunnels may be closed in the middle night
            if (params.dev_data_time && pSeg->excluded_flag_) {
                if (pSeg->excluded_always_) {
                    continue;
                }
                if (this->IsSegmentExcluded(pSeg, params.dev_data_time,
                    params.dev_data_local_time)) {
                    continue;
                }
            }

            double distance2 = CalcDistanceSquareMeters(point, *pSeg);
            if (distance2 < radius2) {
                aDistances[i] = distance2;
                aCandidatesIndexes[candidateCount++] = i;
            }
        }

        if (candidateCount == 0) {
            return nullptr;
        }
        else if (candidateCount == 1) {
            if (p_results) {
                p_results->resize(1);
                SegAssignRes& res = p_results->front();
                res.p_seg = arrSegs[aCandidatesIndexes[0]];
                res.distance = (float)aDistances[0];
                if (params.heading >= 0) {
                    res.heading_distance = WayManager::GetAngle(
                        params.heading, arrSegs[aCandidatesIndexes[0]]->heading_);
                }
                else {
                    res.heading_distance = 0;
                }
                res.score = (float)GetMatchingScore(res.heading_distance,
                    aDistances[aCandidatesIndexes[0]]);
                res.exclusive = true;
            }
            return arrSegs[aCandidatesIndexes[0]];
        }
        else {
            int match_priority = (params.match_priority == -1) ? match_priority_ : params.match_priority;
            if (params.heading < 0) {
                match_priority = WayManager::MATCH_PRI_DISTANCE;
            }

            if (p_results == nullptr) {
                int i_min = 0;
                if (match_priority == WayManager::MATCH_PRI_DISTANCE) { // distance priority
                    // find the nearest
                    double distance_min = aDistances[aCandidatesIndexes[0]];
                    for (int i = 1; i < candidateCount; ++i) {
                        if (aDistances[aCandidatesIndexes[i]] < distance_min) {
                            i_min = i;
                            distance_min = aDistances[aCandidatesIndexes[i]];
                        }
                    }
                }
                else if (match_priority == WayManager::MATCH_PRI_ANGLE) { // angle priority
                    // find the one in same direction
                    int angle_min = WayManager::GetAngle(params.heading,
                        arrSegs[aCandidatesIndexes[0]]->heading_);
                    for (int i = 1; i < candidateCount; ++i) {
                        int angle = WayManager::GetAngle(params.heading,
                            arrSegs[aCandidatesIndexes[i]]->heading_);
                        if (angle < angle_min) {
                            i_min = i;
                            angle_min = angle;
                        }
                    }
                }
                else { // consider both angle and distance
                    int i_max = 0;
                    double value_max = GetMatchingScore(WayManager::GetAngle(params.heading,
                        arrSegs[aCandidatesIndexes[0]]->heading_),
                        aDistances[aCandidatesIndexes[0]]);
                    for (int i = 1; i < candidateCount; ++i) {
                        double value = GetMatchingScore(WayManager::GetAngle(params.heading,
                            arrSegs[aCandidatesIndexes[i]]->heading_),
                            aDistances[aCandidatesIndexes[i]]);
                        if (value > value_max) {
                            i_max = i;
                            value_max = value;
                        }
                    }
                    return arrSegs[aCandidatesIndexes[i_max]];
                }

                return arrSegs[aCandidatesIndexes[i_min]];
            }
            else {
                p_results->resize(candidateCount);
                int i = 0;
                for (auto & res : *p_results) {
                    res.p_seg = arrSegs[aCandidatesIndexes[i]];
                    res.distance = (float)std::sqrt(aDistances[aCandidatesIndexes[i]]);
                    if (params.heading >= 0) {
                        res.heading_distance = WayManager::GetAngle(params.heading,
                            arrSegs[aCandidatesIndexes[i]]->heading_);
                    }
                    else {
                        res.heading_distance = 0;
                    }
                    res.score = (float)GetMatchingScore(res.heading_distance,
                        aDistances[aCandidatesIndexes[i]]);

                    ++i;
                }

                if (params.heading < 0) {
                    match_priority = WayManager::MATCH_PRI_DISTANCE;
                }
                if (match_priority == WayManager::MATCH_PRI_DISTANCE) {
                    std::sort(p_results->begin(), p_results->end(),
                        [](const SegAssignRes& i, const SegAssignRes &j) {
                        return i.distance < j.distance;
                    });
                }
                else if (match_priority == WayManager::MATCH_PRI_ANGLE) {
                    std::sort(p_results->begin(), p_results->end(),
                        [](const SegAssignRes& i, const SegAssignRes &j) {
                        return i.heading_distance < j.heading_distance;
                    });
                }
                else {
                    std::sort(p_results->begin(), p_results->end(),
                        [](const SegAssignRes& i, const SegAssignRes &j) {
                        return i.score > j.score;
                    });
                }
                FlagExclusiveAssignedSeg(point, *p_results);

                // no need to keep too many results
                if (p_results->size() > 4) {
                    p_results->resize(4);
                }

                // normalize the scores
                if (!p_results->empty()) {
                    float max_score = 0;
                    for (const auto& res : *p_results) {
                        if (max_score < res.score) {
                            max_score = res.score;
                        }
                    }
                    if (max_score == 0) {
                        for (auto& res : *p_results) {
                            res.score = 1.0f;
                        }
                    }
                    else {
                        for (auto& res : *p_results) {
                            res.score /= max_score;
                        }
                    }
                }

                return p_results->front().p_seg;
            }
        }
    }

private:
    void FlagExclusiveAssignedSeg(const geo::GeoPoint& point,
        SegAssignResults &assign_results) const
    {
        const size_t res_size = assign_results.size();
        if (res_size == 0) {
            return;
        }
        else if (res_size == 1) {
            if (!assign_results.front().exclusive) {
                assign_results.front().exclusive = true;
            }
            return;
        }

        // if each seg belongs to different oriented way, just return
        size_t oriented_way_id_count = 1;
        for (size_t i = 1; i < res_size; ++i) {
            if (assign_results[i - 1].p_seg->GetWayIdOriented() !=
                assign_results[i].p_seg->GetWayIdOriented()) {
                ++oriented_way_id_count;
            }
        }
        if (oriented_way_id_count == res_size) {
            return;
        }

        struct ResRef
        {
            SegAssignRes *p_res;
            SEG_ID_T      temp_edge_id;
        };
        std::vector<ResRef> res_refs;
        res_refs.reserve(res_size);
        for (auto &res : assign_results) {
            res_refs.push_back({ &res, 0 });
        }

        // order by way_id (oriented), sub_way_id, split_id
        std::sort(res_refs.begin(), res_refs.end(), [](const ResRef &i, const ResRef &j) {
            if (i.p_res->p_seg->GetWayIdOriented() != j.p_res->p_seg->GetWayIdOriented()) {
                return i.p_res->p_seg->GetWayIdOriented() < j.p_res->p_seg->GetWayIdOriented();
            }
            if (i.p_res->p_seg->way_sub_seq_ != j.p_res->p_seg->way_sub_seq_) {
                return i.p_res->p_seg->way_sub_seq_ < j.p_res->p_seg->way_sub_seq_;
            }
            return i.p_res->p_seg->split_seq_ < j.p_res->p_seg->split_seq_;
        });

        // for below case, if seg1, seg2, seg3 are of the same way ID, but node between seg2
        // and seg3 is way connector, seg3 should be kept. among seg1 and seg2, the one with
        // higher score should be kept.
        //
        //                            A
        //                            |
        //       -------->------------>---------------->
        //         seg1     seg2            seg3
        for (size_t i = 0, j; i < res_size - 1; i = j) {
            j = i + 1;
            while (j < res_size &&
                res_refs[j].p_res->p_seg->GetWayIdOriented() ==
                res_refs[i].p_res->p_seg->GetWayIdOriented()) {
                ++j;
            }
            size_t i1 = i, i2 = j - 1;
            res_refs[i1].temp_edge_id = res_refs[i1].p_res->p_seg->seg_id_;

            if (i1 != i2) { // [i1, i2] are of the same way
                for (size_t k = i1 + 1; k <= i2; ++k) {
                    SegmentPtr &p_seg_prev = res_refs[k - 1].p_res->p_seg;
                    SegmentPtr &p_seg = res_refs[k].p_res->p_seg;

                    res_refs[k].temp_edge_id = res_refs[k - 1].temp_edge_id;
                    if (p_seg_prev->to_nd_ == p_seg->from_nd_) { // two segments connected
                        if (p_seg->GetFromNode()->IsWayConnector()) {
                            res_refs[k].temp_edge_id = p_seg->seg_id_;
                        }
                    }
                    else {
                        if (p_seg_prev->GetToNode()->IsWayConnector() ||
                            p_seg->GetFromNode()->IsWayConnector()) {
                            res_refs[k].temp_edge_id = p_seg->seg_id_;
                        }
                    }
                }
            }
        }

        const int SAME_WAY_ANGLE_TOLERANCE = 25;
        {
            // order by score, desc
            std::sort(res_refs.begin(), res_refs.end(), [](const ResRef &i, const ResRef &j) {
                return i.p_res->score > j.p_res->score;
            });

            // remove duplicated temp_edge_id, firstly flag them
            int remove_count = 0;
            for (int i = 0; i < (int)res_size - 1; ++i) {
                auto &ref_i = res_refs[i];
                if (ref_i.p_res) {
                    for (int j = (int)res_size - 1; j > i; --j) {
                        auto &ref_j = res_refs[j];
                        if (ref_j.p_res) {
                            if (ref_i.temp_edge_id == ref_j.temp_edge_id &&
                                WayManager::GetAngle(ref_i.p_res->p_seg->heading_,
                                    ref_j.p_res->p_seg->heading_) < SAME_WAY_ANGLE_TOLERANCE) {
                                ref_j.p_res->p_seg = nullptr; // removal flag in assign_results[*]
                                ref_j.p_res = nullptr;
                                ++remove_count;
                            }
                        }
                    }
                }
            }
            // do the actual removal
            if (remove_count != 0) {
                std::stable_partition(assign_results.begin(), assign_results.end(),
                    [](const SegAssignRes &res) {
                    return res.p_seg != nullptr;
                });
                assign_results.resize(assign_results.size() - remove_count);
            }

            if (assign_results.size() == 1) {
                assign_results.front().exclusive = true;
            }
        }
    }

public:
    static double GetMatchingScore(int angle, double distance2)
    {
        // [0, 16) => [8, 16), linearly
        if (distance2 < 16.0) {
            distance2 = 8.0 + distance2 / 2;
        }

        // [0, 10) => [5, 10), linearly
        double angle_lf;
        if (angle < 10) {
            angle_lf = 5.0 + angle / 2.0;
        }
        else {
            angle_lf = angle;
        }

        double d = 1.0 / (1.0 + std::sqrt(distance2) / 10.0);
        double theta = 1.0 / (1.0 + angle_lf * (M_PI / 45.0));
        return 2 * d + theta;
    }

    bool SetExclusionSegs(std::vector<SegmentPtr> &segs, const EXCLUSION_SETTING &setting)
    {
        auto p_setting = std::make_shared<EXCLUSION_SETTING>(setting);
        if (p_setting->ex_type == EXTYPE_DAILY_TIME_RANGE) {
            auto shift_to_y2k = [](time_t localtime) {
                util::TIMESTAMP_STRUCT ts;
                util::TimeToTimestamp(localtime, ts);
                ts.year = 2000;
                ts.month = 1;
                ts.day = 1;
                return util::TimestampToTime(ts);
            };

            p_setting->time_range_from = shift_to_y2k(p_setting->time_range_from);
            p_setting->time_range_to = shift_to_y2k(p_setting->time_range_to);
        }

        for (auto &p_seg : segs) {
            p_seg->excluded_flag_ = 1;
            p_seg->excluded_always_ = (setting.ex_type == EXTYPE_ALWAYS) ? 1 : 0;
            p_seg->excluded_no_gps_ = (setting.ex_type == EXTYPE_NO_GPS) ? 1 : 0;

            excluded_seg_map_[p_seg->seg_id_] = p_setting;
        }
        return true;
    }

    // is_localtime: true if dev_data_time is local time
    bool IsSegmentExcluded(const SegmentPtr &p_seg, time_t dev_data_time, bool is_localtime) const
    {
        if (!p_seg->excluded_flag_) {
            return false;
        }
        if (p_seg->excluded_always_) {
            return true;
        }

        auto it = excluded_seg_map_.find(p_seg->seg_id_);
        if (it == excluded_seg_map_.cend()) {
            return false;
        }
        auto &ex_setting = *it->second;
        switch (ex_setting.ex_type) {
        case EXTYPE_NONE:
            return false;
        case EXTYPE_ALWAYS:
            return true;
        case EXTYPE_DAILY_TIME_RANGE:
        {
            static _THREAD time_t s_current_day_begin{}, s_current_day_p2k_diff{};
            time_t local_dev_time = is_localtime ?
                dev_data_time : (dev_data_time + local_utc_diff_);
            time_t local_dev_time_p2k;

            // on Linux, TimeToTimestamp and TimestampToTime involve time-consuming time zone
            // related system call
            if (local_dev_time < s_current_day_begin ||
                local_dev_time >= s_current_day_begin + 24 * 3600) {
                util::TIMESTAMP_STRUCT ts;
                util::TimeToTimestamp(local_dev_time, ts);
                ts.hour = ts.minute = ts.second = 0;
                ts.fraction = 0;
                // ts is now the begining of current day
                s_current_day_begin = util::TimestampToTime(ts);

                ts.year = 2000;
                ts.month = 1;
                ts.day = 1;
                // ts is now the begining of 2000-1-1
                s_current_day_p2k_diff = s_current_day_begin - util::TimestampToTime(ts);
            }
            local_dev_time_p2k = local_dev_time - s_current_day_p2k_diff;

            return (local_dev_time_p2k >= ex_setting.time_range_from) &&
                (local_dev_time_p2k <= ex_setting.time_range_to);
        }
        case EXTYPE_DATETIME_RANGE:
        {
            // for this case, does not care about time zone, but caller needs to make sure
            // device data time and time range in the setting are in the same time zone
            time_t local_dev_time = is_localtime ?
                dev_data_time : (dev_data_time + local_utc_diff_);
            return (local_dev_time >= ex_setting.time_range_from) &&
                (local_dev_time <= ex_setting.time_range_to);
        }
        case EXTYPE_NO_GPS:
        {
            // excluded from segment assignment, but can still be used as part of the routing
            // result callers can check Segment::excluded_no_gps_ flag for fine control before
            // call this method IsSegmentExcluded
            return false;
        }
        default:
            break;
        }

        return false;
    }

    bool FindAdjacentSegments(const geo::GeoPoint& pos, double radius, bool has_name,
        std::vector<std::tuple<SegmentPtr, double> >& segs) const
    {
        segs.clear();

        TilePtr p_tile = GetTileByPos(pos);
        if (!p_tile) {
            return true;
        }

        for (const auto& p_seg : p_tile->segments_with_neighbours) {
            if (has_name) {
                if (p_seg->way_name_.empty()) {
                    continue;
                }
            }

            double distance = std::sqrt(CalcDistanceSquareMeters(pos, *p_seg));
            if (distance <= radius) {
                segs.push_back(std::make_tuple(p_seg, distance));
            }
        }

        // sort by distance
        if (!segs.empty()) {
            std::sort(segs.begin(), segs.end(),
                [](const std::tuple<SegmentPtr, double>& i,
                    const std::tuple<SegmentPtr, double>& j) {
                return std::get<1>(i) < std::get<1>(j);
            });
        }

        return true;
    }

    bool FindAdjacentNodes(const geo::GeoPoint& pos, double radius,
        std::vector<std::tuple<NodePtr, double> >& nodes) const
    {
        std::vector<std::tuple<SegmentPtr, double> > segs;
        if (false == FindAdjacentSegments(pos, radius, false, segs)) {
            return false;
        }

        std::vector<NodePtr> tmp_nodes;
        tmp_nodes.reserve(segs.size() * 2);

        for (const auto& p_seg : segs) {
            tmp_nodes.push_back(std::get<0>(p_seg)->GetFromNode());
            tmp_nodes.push_back(std::get<0>(p_seg)->GetToNode());
        }

        nodes.clear();
        if (tmp_nodes.empty()) {
            return true;
        }

        // remove the duplicated and distant
        std::sort(tmp_nodes.begin(), tmp_nodes.end(),
            [](const NodePtr& i, const NodePtr& j) {
            return i->nd_id_ < j->nd_id_;
        });

        nodes.push_back(std::make_tuple(tmp_nodes.front(),
            geo::distance_in_meter(pos, tmp_nodes.front()->geo_point_)));
        for (size_t i = 1; i < tmp_nodes.size(); ++i) {
            const auto& p_node = tmp_nodes[i];
            if (tmp_nodes[i]->nd_id_ != std::get<0>(nodes.back())->nd_id_) {
                double distance = geo::distance_in_meter(pos, p_node->geo_point_);
                if (distance <= radius) {
                    nodes.push_back(std::make_tuple(tmp_nodes.front(), distance));
                }
            }
        }

        // sort by distance
        std::sort(nodes.begin(), nodes.end(),
            [](const std::tuple<NodePtr, double>& i, const std::tuple<NodePtr, double>& j) {
            return std::get<1>(i) < std::get<1>(j);
        });

        return true;
    }

private:
    static int GetMinAngleBiDir(const SegmentPtr& p_seg, int heading, int opposite_heading)
    {
        // NOTE: for the opposite angle calcuation, all the result are added with 1 degree.
        // This is because like below, some way are modelled in GIS as two ways, each is a one-way
        //    ---------------> way_id1
        //    <--------------- way_id2
        // We'd prefer the original segment in GIS are selected with high priority insead of the
        // opposite segment.

        int angle_min1 = WayManager::GetAngle(heading, p_seg->heading_);

        // for these link roads, even special car are not allowed to drive on it?
        if (p_seg->way_type_ >= HIGHWAY_MOTORWAY_LINK &&
            p_seg->way_type_ <= HIGHWAY_TERTIARY_LINK) {
            return angle_min1;
        }

        int angle_min2 = WayManager::GetAngle(opposite_heading, p_seg->heading_) + 1;
        return (angle_min1 <= angle_min2) ? angle_min1 : angle_min2;
    }

    void AddSegmentForTiles(const SegmentPtr &p_seg)
    {
        TILE_XY xy1 = PosToTileId(p_seg->from_point_);
        auto&& p_tile1 = GetTileById(xy1);
        if (p_tile1) {
            p_tile1->AddSegment(p_seg);
        }

        TILE_XY xy2 = PosToTileId(p_seg->to_point_);
        if (xy1 != xy2) {
            auto&& p_tile2 = GetTileById(xy2);
            if (p_tile2) {
                p_tile2->AddSegment(p_seg);
            }

            // add the segment to the related tile if the middle point is in another tile
            geo::GeoPoint mid((p_seg->from_point_.lat + p_seg->to_point_.lat) / 2,
                (p_seg->from_point_.lng + p_seg->to_point_.lng) / 2);
            TILE_XY xy0 = PosToTileId(mid);
            if (xy0 != xy1 && xy0 != xy2) {
                auto p_tile0 = GetTileById(xy0);
                if (p_tile0) {
                    p_tile0->AddSegment(p_seg);
                }
            }
        }
    }

    bool InBound(double lat, double lng) const
    {
        return (lat >= bound_.minlat && lat <= bound_.maxlat &&
            lng >= bound_.minlng && lng <= bound_.maxlng);
    }

    TilePtr GetTileById(TILE_XY tile_id) const
    {
        int x = geo::tilexy2tilex(tile_id);
        int y = geo::tilexy2tiley(tile_id);
        if (x >= min_tile_x_ && x < min_tile_x_ + mat_width_ &&
            y >= min_tile_y_ && y < min_tile_y_ + mat_height_) {
            return (TilePtr)&tile_mat_(y - min_tile_y_, x - min_tile_x_);
        }
        return nullptr;
    }

    TilePtr GetTileByPos(const geo::GeoPoint& pos) const
    {
        TILE_XY tile_id = PosToTileId(pos);
        return GetTileById(tile_id);
    }

    TILE_XY PosToTileId(const geo::GeoPoint& pos) const
    {
        int x = geo::long2tilex(pos.lng, GRID_CELL_ZOOM_LEVEL);
        int y = geo::lat2tiley(pos.lat, GRID_CELL_ZOOM_LEVEL);
        return geo::make_tilexy(x, y);
    }

#if 0
    // two-way segment is put to table twice, here is to remove 1
    void RemoveDupSegs()
    {
        for (int y = 0; y < mat_height_; ++y) {
            for (int x = 0; x < mat_width_; ++x) {
                TilePtr& p_tile = tile_mat_(y, x);

                // check remove dup segments for the tile
                for (int i = (int)p_tile->segments.size() - 1; i > 0; --i) {
                    SegmentPtr& p_seg = p_tile->segments[i];
                    for (int j = 0; j < i; ++j) {
                        if (p_seg->from_nd_ == p_tile->segments[j]->to_nd_ &&
                            p_seg->to_nd_ == p_tile->segments[j]->from_nd_) {
                            // also remove from this->all_segs_map_
                            SegmentMap::iterator it = all_segs_map_.find(p_seg->seg_id_);
                            if (it != all_segs_map_.end()) {
                                all_segs_map_.erase(it);
                            }

                            p_tile->segments.erase(p_tile->segments.begin() + i);
                            break;
                        }
                    }
                }
            }
        }
    }
#endif

    void InitNeighbourSegsOneTile(int x, int y)
    {
        const TilePtr& p_tile = &tile_mat_(y, x);
        std::vector<SegmentPtr> segments_with_neighbours = p_tile->segments;

        // get all the neighbours
        int x0 = geo::tilexy2tilex(p_tile->tile_id);
        int y0 = geo::tilexy2tiley(p_tile->tile_id);
        TILE_XY neighbours[] = {
            geo::make_tilexy(x0 - 1, y0 + 1), geo::make_tilexy(x0, y0 + 1), geo::make_tilexy(x0 + 1, y0 + 1),
            geo::make_tilexy(x0 - 1, y0),                                   geo::make_tilexy(x0 + 1, y0),
            geo::make_tilexy(x0 - 1, y0 - 1), geo::make_tilexy(x0, y0 - 1), geo::make_tilexy(x0 + 1, y0 - 1)
        };

        for (int i = 0; i < 8; ++i) {
            TilePtr the_tile = this->GetTileById(neighbours[i]);
            if (the_tile && !the_tile->segments.empty()) {
                segments_with_neighbours.insert(segments_with_neighbours.end(),
                    the_tile->segments.begin(), the_tile->segments.end());
            }
        }

        // flag duplicated
        int effective_count = (int)segments_with_neighbours.size();
        if (effective_count >= 100) { // arbitrary boundry between n^2 vs n log(n)
            std::sort(segments_with_neighbours.begin(), segments_with_neighbours.end());

            auto size = segments_with_neighbours.size();
            for (size_t i = 0; i < size - 1; ++i) {
                auto& s1 = segments_with_neighbours[i];
                const auto& s2 = segments_with_neighbours[i + 1];
                if (s1 == s2) {
                    s1 = nullptr;
                    --effective_count;
                }
            }
        }
        else {
            for (int i = (int)segments_with_neighbours.size() - 1; i > 0; --i) {
                auto& seg_i = segments_with_neighbours[i];
                for (int j = 0; j < i; ++j) {
                    if (seg_i == segments_with_neighbours[j]) {
                        seg_i = nullptr;
                        --effective_count;
                        break;
                    }
                }
            }
        }


        // fill in p_tile->segments_with_neighbours[]
        if ((int)segments_with_neighbours.size() == effective_count) { // no duplicated segments
            p_tile->segments_with_neighbours.swap(segments_with_neighbours);
        }
        else {
            p_tile->segments_with_neighbours.clear();
            p_tile->segments_with_neighbours.reserve(effective_count);
            for (const auto& p_seg : segments_with_neighbours) {
                if (p_seg) {
                    p_tile->segments_with_neighbours.push_back(p_seg);
                }
            }
        }
    }

    void InitNeighbourSegs()
    {
#if WAY_MANAGER_HANA_LOG == 1
        hana::Logger logger("WayManager");
        auto start = std::chrono::system_clock::now();
        AT_SCOPE_EXIT(std::chrono::duration<double> elapsed = std::chrono::system_clock::now() - start;
        HANA_SDK_DEBUG(logger) << "InitNeighbourSegs(): run time "
            << elapsed.count() << " seconds" << hana::endl;);
#endif

        std::vector<std::tuple<int, int> > tile_tuples;
        util::SimpleDataQueue<size_t> indices;
        tile_tuples.reserve(mat_height_ * mat_width_);

        size_t i = 0;
        for (int y = 0; y < mat_height_; ++y) {
            for (int x = 0; x < mat_width_; ++x) {
                tile_tuples.push_back(std::make_tuple(x, y));
                indices.Add(i++);
            }
        }

        unsigned task_count = std::thread::hardware_concurrency();
        if (task_count == 0) {
            task_count = 2;
        }
        util::CreateSimpleThreadPool("WayManageer_InitNeigh", task_count,
            [&indices, this, &tile_tuples]() {
            while (true) {
                size_t index;
                if (false == indices.Get(index)) {
                    break;
                }

                int x, y;
                std::tie(x, y) = tile_tuples[index];
                this->InitNeighbourSegsOneTile(x, y);
            }
        }).JoinAll();
    }

private:
    const SegmentMap& all_segs_map_;
    const Bound bound_;
    const int match_priority_;
    int min_tile_x_, min_tile_y_, mat_width_, mat_height_;
    SimpleMatrix<Tile> tile_mat_;
    mutable std::mutex threads_err_mutex_;
    mutable UNORD_MAP<std::string, std::string> threads_err_strs_;

    time_t local_utc_diff_; // = local time - utc time
    UNORD_MAP<SEG_ID_T, std::shared_ptr<EXCLUSION_SETTING>> excluded_seg_map_;

    double GRID_CELL_ZOOM_LEVEL;
    friend class geo::WayManager;
};
} // namespace seg


///////////////////////////////////////////////////////////////////////////////////////////////////
// class WayManager

bool WayManager::InitSegServices(const Bound& bound, MATCH_PRI match_priority/* = 0*/)
{
    if (match_priority < 0 || match_priority > 2) {
        SetErrorString("WayManager.InitSegServices: invalid parameter for matching priority: "
            + std::to_string(match_priority));
        return false;
    }

    p_seg_manager_ = std::make_shared<seg::SegmentManager>(seg_map_, bound, match_priority);
    if (false == p_seg_manager_->LoadToTileMatrix()) {
        SetErrorString(p_seg_manager_->GetErrorString());
        return false;
    }

    p_seg_manager_->excluded_seg_map_.clear();
    return true;
}

SegmentPtr WayManager::AssignSegment(const geo::GeoPoint& point, const SegAssignParams& params,
    SegAssignResults *p_results /*= nullptr*/) const
{
    return p_seg_manager_->AssignSegment(point, params, p_results);
}

SegmentPtr WayManager::AssignSegment(double lat, double lng, int heading, double radius,
    int angle_tollerance, const char *way_name /* = nullptr */,
    bool no_road_link /* = false */, bool ignore_reverse_segs/* = false*/,
    const SEG_ID_T *excluded_seg_ids /*= nullptr*/, int excluded_seg_count /*= 0*/) const
{
    SegAssignParams params;
    params.heading = heading;
    params.radius = radius;
    params.angle_tollerance = angle_tollerance;
    params.way_name = const_cast<char *>(way_name);
    params.no_road_link = no_road_link;
    params.ignore_reverse_segs = ignore_reverse_segs;
    params.excluded_seg_ids = const_cast<SEG_ID_T*>(excluded_seg_ids);
    params.excluded_seg_count = excluded_seg_count;

    return p_seg_manager_->AssignSegment(geo::GeoPoint(lat, lng), params, nullptr);
}

SegmentPtr WayManager::AssignSegment(const geo::GeoPoint& point, int heading, double radius,
    int angle_tollerance, const char *way_name /*= nullptr*/,
    bool no_road_link /*= false*/, bool ignore_reverse_segs/* = false*/,
    const SEG_ID_T *excluded_seg_ids /*= nullptr*/, int excluded_seg_count /*= 0*/) const
{
    SegAssignParams params;
    params.heading = heading;
    params.radius = radius;
    params.angle_tollerance = angle_tollerance;
    params.way_name = const_cast<char *>(way_name);
    params.no_road_link = no_road_link;
    params.ignore_reverse_segs = ignore_reverse_segs;
    params.excluded_seg_ids = const_cast<SEG_ID_T*>(excluded_seg_ids);
    params.excluded_seg_count = excluded_seg_count;

    return p_seg_manager_->AssignSegment(point, params, nullptr);
}

bool WayManager::FindAdjacentSegments(const geo::GeoPoint& point, double radius, bool has_name,
    std::vector<std::tuple<SegmentPtr, double> >& segs) const
{
    return p_seg_manager_->FindAdjacentSegments(point, radius, has_name, segs);
}

bool WayManager::FindAdjacentSegments(double lat, double lng, double radius, bool has_name,
    std::vector<std::tuple<SegmentPtr, double> >& segs) const
{
    return p_seg_manager_->FindAdjacentSegments(geo::GeoPoint(lat, lng), radius, has_name, segs);
}

static bool SimpleCheckReverseAble(const SegmentPtr& p_seg)
{
    // only one-way needs the fake reverse segment
    if (!p_seg->one_way_) {
        return false;
    }
    // can special vehicles drive in tunnel? should be no
    if (p_seg->struct_type_ == STRUCT_TUNNEL) {
        return false;
    }
    // can special vehicles drive the way above ground and under ground? guess no
    if (p_seg->layer_ != 0) {
        return false;
    }
    // can special vehicles drive on road links? I guess not
    if (p_seg->way_type_ >= HIGHWAY_MOTORWAY_LINK && p_seg->way_type_ <= HIGHWAY_TERTIARY_LINK) {
        return false;
    }
    // this is part of a roundabout
    if (p_seg->GetTagByName("roundabout", nullptr) == true) {
        return false;
    }

    return true;
}

// precondition: seg_map_ already contains all the segments excluding fake reversed segments
bool WayManager::GenerateReversedSegsIntoSegMap()
{
#if WAY_MANAGER_HANA_LOG == 1
    hana::Logger logger("WayManager");
    auto start = std::chrono::system_clock::now();
    AT_SCOPE_EXIT(std::chrono::duration<double> elapsed = std::chrono::system_clock::now() - start;
    HANA_SDK_DEBUG(logger) << "WayManager::GenerateReversedSegsIntoSegMap(): run time "
        << elapsed.count() << " seconds" << hana::endl;);
#endif

    seg::SegmentManager seg_manager(seg_map_, bound_, 0); // does not care match_priority]
    if (false == seg_manager.LoadToTileMatrix()) {
        SetErrorString(seg_manager.GetErrorString());
        return false;
    }

    UNORD_SET<WAY_ID_T> non_rev_ways;
    for (const auto& elem : seg_map_) {
        const SegmentPtr& p_seg = elem.second;
        if (!SimpleCheckReverseAble(p_seg)) {
            continue;
        }
        if (non_rev_ways.find(p_seg->way_id_) != non_rev_ways.end()) {
            continue;
        }

        // do not generate fake reversed segmemnts for any segments connected with one-way tunnel
        bool found = false;
        for (int i = 0; i < 2 && !found; ++i) {
            const auto& p_node = (i == 0) ? p_seg->p_from_nd_ : p_seg->p_to_nd_;
            for (const auto& p_conn_seg : p_node->connected_segments_) {
                if (p_conn_seg->way_id_ == p_seg->way_id_) {
                    continue;
                }
                if (p_conn_seg->one_way_ && p_conn_seg->struct_type_ == STRUCT_TUNNEL) {
                    found = true;
                    break;
                }
            }
        }
        if (found) {
            non_rev_ways.insert(p_seg->way_id_);
            continue;
        }

        // for one-way, check if there is an opposite one-way nearby.
        {
            // <----------<----------<----------
            // ---------->---------->---------->
            //             this seg

            bool ok_to_generate = true;
            const GeoPoint&& mid_pt = GeoPoint::GetMidPoint(p_seg->from_point_, p_seg->to_point_);
            std::vector<std::tuple<SegmentPtr, double> > segs;
            if (!p_seg->way_name_.empty()) {
                seg_manager.FindAdjacentSegments(mid_pt, 60.0, true, segs);
            }
            else {
                // segments with and without names are all returned
                seg_manager.FindAdjacentSegments(mid_pt, 60.0, false, segs);
            }

            if (!segs.empty()) {
                SegmentPtr p_near_seg;
                for (auto& t : segs) {
                    std::tie(p_near_seg, std::ignore) = t;

                    if (!p_near_seg->one_way_) {
                        continue;
                    }
                    // basic checks of direction, way name ...
                    if (WayManager::GetAngle(p_near_seg->heading_, p_seg->heading_) <= 170) {
                        continue;
                    }
                    // if have way name, should have the same way name. if not, should have
                    // the same way type
                    if (!p_seg->way_name_.empty()) {
                        if (p_near_seg->way_name_ != p_seg->way_name_) {
                            continue;
                        }
                    }
                    else {
                        if (p_near_seg->way_type_ != p_seg->way_type_) {
                            continue;
                        }
                    }
                    if (p_near_seg->struct_type_ != p_seg->struct_type_ ||
                        p_near_seg->layer_ != p_seg->layer_) {
                        continue;
                    }

                    // verify if the opposiste segment is valid (for countries driving on right,
                    // should be on the left)
                    //
                    //                <-------------------
                    //                         A
                    //                         |
                    //   ------------->------------------>---------------->
                    //                      this seg
                    const GeoPoint&& mid_pt_near_seg =
                        GeoPoint::GetMidPoint(p_near_seg->from_point_, p_near_seg->to_point_);
                    int heading_mid_to_mid =
                        (int)geo::get_heading_in_degree(mid_pt, mid_pt_near_seg);
                    int angle = (heading_mid_to_mid - p_seg->heading_ + 720) % 360;
                    bool valid = false;
                    if (this->drive_on_right_) {
                        valid = (angle >= 190 && angle <= 350);
                    }
                    else {
                        valid = (angle >= 10 && angle <= 170);
                    }
                    if (valid) {
                        // if found valid opposite segment, no need to generate the fake one
                        ok_to_generate = false;
                        break;
                    }
                }
            }

            if (!ok_to_generate) {
                non_rev_ways.insert(p_seg->way_id_);
                continue;
            }
        }
    }

    std::vector<SegmentPtr> rev_seg_candidates;
    rev_seg_candidates.reserve(seg_map_.size() / 4);
    for (const auto& elem : seg_map_) {
        const SegmentPtr& p_seg = elem.second;
        if (!SimpleCheckReverseAble(p_seg)) {
            continue;
        }
        if (non_rev_ways.find(p_seg->way_id_) != non_rev_ways.end()) {
            continue;
        }

        // got a chance to set the node IDs
        if (p_seg->from_nd_ != 0 && nullptr == p_seg->p_from_nd_) {
            p_seg->p_from_nd_ = node_map_[p_seg->from_nd_];
        }
        if (p_seg->to_nd_ != 0 && nullptr == p_seg->p_to_nd_) {
            p_seg->p_to_nd_ = node_map_[p_seg->to_nd_];
        }

        // ok, now, let's generate the fake one
        // since the fake reversed segment will be added, treat it as two-way
        p_seg->one_way_ = false;

        rev_seg_candidates.push_back(p_seg);
    }

    this->seg_pool_rev_.Reserve(rev_seg_candidates.size());
    for (auto& p_seg : rev_seg_candidates) {
        Segment rev_seg(-p_seg->seg_id_, p_seg->way_id_, p_seg->way_sub_seq_,
            p_seg->split_seq_, p_seg->to_nd_, p_seg->from_nd_,
            p_seg->to_point_.lat, p_seg->to_point_.lng,
            p_seg->from_point_.lat, p_seg->from_point_.lng, p_seg->one_way_,
            p_seg->way_type_, p_seg->highway_type_str_, p_seg->way_name_,
            p_seg->struct_type_, p_seg->layer_, p_seg->p_opt_tags_);
        auto&& p_rev_seg = this->seg_pool_rev_.AllocNew(move(rev_seg));

        if (nullptr == p_rev_seg->p_from_nd_) {
            p_rev_seg->SetFromNode(p_seg->p_to_nd_);
        }
        if (nullptr == p_rev_seg->p_to_nd_) {
            p_rev_seg->SetToNode(p_seg->p_from_nd_);
        }

        // add the segment to segment map
        seg_map_.insert({ p_rev_seg->seg_id_, p_rev_seg });
    }

    return true;
}

bool WayManager::SetExclusionSegs(std::vector<SegmentPtr> &segs, const EXCLUSION_SETTING &setting,
    bool sync_for_routing)
{
    bool ok = p_seg_manager_->SetExclusionSegs(segs, setting);
    if (sync_for_routing) {
        SyncExclusionSegsToRouting();
    }
    return ok;
}

bool WayManager::IsSegmentExcluded(const SegmentPtr &p_seg, time_t dev_data_time,
    bool is_localtime) const
{
    return p_seg_manager_->IsSegmentExcluded(p_seg, dev_data_time, is_localtime);
}

bool WayManager::SetNoGpsTunnelRoute(const std::vector<SegmentPtr> &tunnel_route)
{
    // it is observed that the tunnel entry can still have GPS reports
    std::vector<SegmentPtr> segs;
    double len = 0;
    size_t i = 0;
    for (auto p_seg : tunnel_route) {
        ++i;
        len += p_seg->length_;
        if (len > 50) break; // some distance into tunnel, still have GPS report
    }
    if (i < tunnel_route.size()) {
        segs.insert(segs.begin(), tunnel_route.begin() + i, tunnel_route.end());

        EXCLUSION_SETTING setting;
        setting.ex_type = EXTYPE_NO_GPS;
        return SetExclusionSegs(segs, setting, true);
    }
    return true;
}

bool WayManager::LoadExclusionRoutesFromCsv(const std::string &pathname,
    std::vector<EXCLUDED_ROUTE> &ex_routes, std::string& err)
{
    using ex_route_tuple = std::tuple<string, int, int, string, string, int,
        double, double ,int, double, double, int, double, double, int, double, double, int>;
    vector<ex_route_tuple> table;
    if (false == util::CsvToTuples(pathname, ',', table, err)) {
        return false;
    }

    const size_t num_rows = table.size();
    ex_routes.clear();
    ex_routes.reserve(table.size());

    string description, time_point_from, time_point_to;
    int exclusion_type, time_point_type, bi_dir;
    double lat1, lng1, lat2, lng2, lat3, lng3, lat4, lng4;
    int heading1, heading2, heading3, heading4;
    for (size_t i = 0; i < num_rows; ++i) {
        geo::EXCLUDED_ROUTE r;
        tie(description, exclusion_type, time_point_type, time_point_from, time_point_to, bi_dir,
            lat1, lng1, heading1, lat2, lng2, heading2, lat3, lng3, heading3,
            lat4, lng4, heading4) = table[i];

        r.exclusion_type = (geo::EXCLUSION_TYPE)exclusion_type;
        if (r.exclusion_type < 0 || r.exclusion_type > EXTYPE_MAX_VALID) {
            err = "Invalid EXCLUSION_TYPE " + to_string(r.exclusion_type);
            continue;
        }
        if (r.exclusion_type == EXTYPE_NONE) {
            continue;
        }

        r.description = description;
        r.time_type = (geo::TIME_TYPE)time_point_type;
        if (r.exclusion_type != EXTYPE_ALWAYS) {
            r.time_point_from = util::StrToTimeT(time_point_from);
            r.time_point_to = util::StrToTimeT(time_point_to);
        }
        r.bi_dir = (bi_dir != 0);

        r.via_points.clear();
        r.via_points.reserve(4);
        auto add_via_point = [&r, i](double lat, double lng, int heading) {
            if (lat != 0 && lng != 0 && heading >= 0 && heading < 360) {
                GeoPoint point(lat, lng);
                r.via_points.push_back(std::make_tuple(point, heading));
            }
        };

        add_via_point(lat1, lng1, heading1);
        add_via_point(lat2, lng2, heading2);
        add_via_point(lat3, lng3, heading3);
        add_via_point(lat4, lng4, heading4);

        ex_routes.emplace_back(r);
    }

    return true;
}

static bool ExcludedRouteToSetting(const EXCLUDED_ROUTE &ex_route, EXCLUSION_SETTING &ex_setting,
    std::string &err)
{
    ex_setting.ex_type = ex_route.exclusion_type;

    if (ex_route.exclusion_type == EXTYPE_NONE || ex_route.exclusion_type == EXTYPE_ALWAYS) {
        return true;
    }
    else if (ex_route.exclusion_type == EXTYPE_DAILY_TIME_RANGE) {
        if (TIME_TYPE_LOCAL != ex_route.time_type) {
            err = std::string(__FUNCTION__) + "[" + ex_route.description +
                "] For EXTYPE_DAILY_TIME_RANGE, must use local time.";
            return false;
        }
        else {
            ex_setting.time_range_from = ex_route.time_point_from;
            ex_setting.time_range_to = ex_route.time_point_to;
            return true;
        }
    }
    else if (ex_route.exclusion_type == EXTYPE_DATETIME_RANGE) {
        ex_setting.time_range_from = ex_route.time_point_from;
        ex_setting.time_range_to = ex_route.time_point_to;
        return true;
    }
    else if (ex_route.exclusion_type == EXTYPE_NO_GPS) {
        return true;
    }
    else {
        err = std::string(__FUNCTION__) + "[" + ex_route.description +
            +"] Invalid EXCLUSION_TYPE " + std::to_string((int)ex_route.exclusion_type);
        return false;
    }
}

static bool ExcludedRoutesToSegs(const geo::WayManager &way_manager,
    const std::vector<EXCLUDED_ROUTE> &ex_routes,
    std::vector<std::vector<SegmentPtr>> &seg_routes,
    std::vector<std::vector<SegmentPtr>> &seg_routes_rev,
    std::string &err)
{
    err.clear();
    seg_routes.clear();
    seg_routes_rev.clear();
    if (ex_routes.empty()) {
        return true;
    }
    seg_routes.resize(ex_routes.size());
    seg_routes_rev.resize(ex_routes.size());

    ViaRouteParams params;
    params.radius = 40;
    params.angle_tollerance = 35;
    params.check_no_gps_route = false;

    ViaRouteResult result;
    for (size_t index = 0; index < ex_routes.size(); ++index) {
        auto &r = ex_routes[index];
        auto via_points_out_of_bound = [&way_manager](const ViaRouteParams &params) {
            for (auto &via : params.via_points) {
                if (way_manager.GetBoundries().OutOfBound(via.geo_point)) {
                    return true;
                }
            }
            return false;
        };

        params.via_points.clear();
        geo::GeoPoint point;
        int heading;
        for (auto &t : r.via_points) {
            std::tie(point, heading) = t;
            params.via_points.emplace_back(point, heading);
        }

        result.Clear();
        if (true == way_manager.ViaRoute(params, result)) {
            seg_routes[index] = result.seg_route;
        }
        else if (!via_points_out_of_bound(params)) {
            auto point0 = std::get<0>(r.via_points[0]);
            err = std::string(__FUNCTION__) + "[" + r.description + "] routing failed from ("
                + std::to_string(point0.lat) + ", " + std::to_string(point0.lng) + ") ..."
                + " route seq = " + std::to_string(index + 1);
        }

        if (r.bi_dir) {
            params.via_points.clear();
            for (int i = (int)r.via_points.size() - 1; i >= 0; --i) {
                std::tie(point, heading) = r.via_points[i];
                params.via_points.emplace_back(point, heading); // reverse the order
                params.via_points.back().heading =
                    (params.via_points.back().heading + 180) % 360; // opposite direction
            }
            result.Clear();
            if (true == way_manager.ViaRoute(params, result)) {
                seg_routes_rev[index] = result.seg_route;
            }
            else if (!via_points_out_of_bound(params)) {
                auto point0 = std::get<0>(r.via_points[0]);
                err = std::string(__FUNCTION__) + "[" +
                    r.description + "] reverse routing failed from ("
                    + std::to_string(point0.lat) + ", " + std::to_string(point0.lng) + ") ..."
                    + " route seq = " + std::to_string(index + 1);
            }
        }
    }

    return true;
}

bool WayManager::SetExclusionRoutes(const std::vector<EXCLUDED_ROUTE> &ex_routes,
    bool sync_for_routing)
{
    std::vector<std::vector<SegmentPtr>> seg_routes;
    std::vector<std::vector<SegmentPtr>> seg_routes_rev;
    std::string err;
    ExcludedRoutesToSegs(*this, ex_routes, seg_routes, seg_routes_rev, err);
    if (seg_routes.empty()) {
        if (!err.empty()) {
            this->SetErrorString(err);
        }
        else {
            this->SetErrorString("WayManager::SetExclusionRoutes has errors");
        }
        return false;
    }

    int index = 0;
    for (const auto &r : ex_routes) {
        EXCLUSION_SETTING ex_setting;
        if (false == ExcludedRouteToSetting(r, ex_setting, err)) {
            this->SetErrorString(err);
            return false;
        }

        if (ex_setting.ex_type == EXTYPE_NO_GPS) {
            // NO GPS route is a little special, better to use specific API
            if (!seg_routes[index].empty()) {
                this->SetNoGpsTunnelRoute(seg_routes[index]);
            }
            if (!seg_routes_rev[index].empty()) {
                this->SetNoGpsTunnelRoute(seg_routes_rev[index]);
            }
        }
        else if (ex_setting.ex_type != EXTYPE_NONE) {
            if (!seg_routes[index].empty()) {
                this->SetExclusionSegs(seg_routes[index], ex_setting, false);
            }
            if (!seg_routes_rev[index].empty()) {
                this->SetExclusionSegs(seg_routes_rev[index], ex_setting, false);
            }
        }

        ++index;
    }

    if (sync_for_routing) {
        SyncExclusionSegsToRouting();
    }
    return true;
}


static bool ExcludedRoutesToJson(const geo::WayManager &way_manager,
    const std::vector<EXCLUDED_ROUTE> &ex_routes,
    const std::vector<std::vector<SegmentPtr>> &seg_routes,
    const std::vector<std::vector<SegmentPtr>> &seg_routes_rev,
    geo::EXCLUSION_TYPE exclusion_type, const std::string& json_pathname,
    std::string &err)
{
    geo::GeoJSON geo_json;
    auto get_linestr = [](const EXCLUDED_ROUTE &r, bool is_reverse,
        const std::vector<SegmentPtr>& route) {
         auto p_linestr = std::make_shared<geo::GeoObj_LineString>();
        p_linestr->AddPoint(route.front()->from_point_);

        for (auto &p_seg : route) {
            p_linestr->AddPoint(p_seg->to_point_);
        }

        if (p_linestr) {
            p_linestr->AddProp("exclusion_type", (int)r.exclusion_type);
            p_linestr->AddProp("description", r.description);
            p_linestr->AddProp("is_reverse", is_reverse);
            p_linestr->AddProp("color", "blue");
        }

        return p_linestr;
    };

    for (size_t index = 0; index < ex_routes.size(); ++index) {
        auto &r = ex_routes[index];
        if (r.exclusion_type != exclusion_type) {
            continue;
        }
        if (!seg_routes[index].empty()) {
            auto p_linestr = get_linestr(r, false, seg_routes[index]);
            if (p_linestr) {
                geo_json.AddObj(p_linestr);
            }
        }
        if (!seg_routes_rev[index].empty()) {
            auto p_linestr = get_linestr(r, true, seg_routes_rev[index]);
            if (p_linestr) {
                geo_json.AddObj(p_linestr);
            }
        }
    }

    return geo_json.ToJsonFile(json_pathname);
}

bool WayManager::ExcludedRoutesToJsons(const std::vector<EXCLUDED_ROUTE> &ex_routes,
    const std::string &path_prefix) const
{
    std::string err;
    std::vector<std::vector<SegmentPtr>> seg_routes, seg_routes_rev;
    bool ok = ExcludedRoutesToSegs(*this, ex_routes, seg_routes, seg_routes_rev, err);
    if (!ok) {
        this->SetErrorString(err);
        return false;
    }

    for (int i = (int)EXTYPE_ALWAYS; i <= (int)EXTYPE_MAX_VALID; ++i) {
        std::string pathname = path_prefix + "-" + std::to_string(i) + ".json";
        if (false == ExcludedRoutesToJson(*this, ex_routes, seg_routes, seg_routes_rev,
            EXCLUSION_TYPE(i), pathname, err)) {
            this->SetErrorString(err);
            return false;
        }
    }
    return true;
}

}
