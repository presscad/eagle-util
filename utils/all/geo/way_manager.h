
#ifndef _WAY_MANAGER_H_
#define _WAY_MANAGER_H_

#ifdef _WIN32
#define WAY_MANAGER_BOOST_UNORDERRED 0
#else
#define WAY_MANAGER_BOOST_UNORDERRED 1
#endif

#include <cstdio>
#include <vector>
#include <string>
#include <memory>
#include <tuple>
#include <mutex>
#include <boost/dynamic_bitset.hpp>
#if WAY_MANAGER_BOOST_UNORDERRED == 1
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#else
#include <unordered_map>
#include <unordered_set>
#endif
#include "geo_utils.h"
#include "common/simple_obj_pool.hpp"

namespace geo {

#if WAY_MANAGER_BOOST_UNORDERRED == 1
    template <typename A, typename B>
    using UNORD_MAP = boost::unordered_map<A, B>;

    template <typename T>
    using UNORD_SET = boost::unordered_set<T>;
#else
    template <typename A, typename B>
    using UNORD_MAP = std::unordered_map<A, B>;

    template <typename T>
    using UNORD_SET = std::unordered_set<T>;
#endif


typedef long long SEG_ID_T;
typedef long long WAY_ID_T;
typedef long long NODE_ID_T;
typedef long long EDGE_ID_T;


// refer to http://wiki.openstreetmap.org/wiki/Key:highway
typedef enum HIGHWAY_TYPE
{
    HIGHWAY_UNKNOWN = 0,

    // Roads
    HIGHWAY_MOTORWAY,
    HIGHWAY_TRUNK,
    HIGHWAY_PRIMARY,
    HIGHWAY_SECONDARY,
    HIGHWAY_TERTIARY,
    HIGHWAY_UNCLASSIFIED,
    HIGHWAY_RESIDENTIAL,
    HIGHWAY_SERVICE,
    HIGHWAY_RESERVE1,
    HIGHWAY_RESERVE2,
    HIGHWAY_RESERVE3,

    // Link roads
    HIGHWAY_MOTORWAY_LINK,
    HIGHWAY_TRUNK_LINK,
    HIGHWAY_PRIMARY_LINK,
    HIGHWAY_SECONDARY_LINK,
    HIGHWAY_TERTIARY_LINK,
    HIGHWAY_RESERVE4,
    HIGHWAY_RESERVE5,
    HIGHWAY_RESERVE6,

    // Special road types
    HIGHWAY_LIVING_STREET,
    HIGHWAY_PEDESTRIAN,
    HIGHWAY_TRACK,
    HIGHWAY_BUS_GUIDEWAY,
    HIGHWAY_RACEWAY,
    HIGHWAY_ROAD,
    HIGHWAY_RESERVE7,
    HIGHWAY_RESERVE8,
    HIGHWAY_RESERVE9,

    // Paths
    HIGHWAY_FOOTWAY,
    HIGHWAY_BRIDLEWAY,
    HIGHWAY_STEPS,
    HIGHWAY_PATH,
    HIGHWAY_RESERVE10,
    HIGHWAY_RESERVE11,

    // Cycleway
    HIGHWAY_CYCLEWAY,
    HIGHWAY_RESERVE12,
    HIGHWAY_RESERVE13,

    // Lifecycle
    HIGHWAY_PROPOSED,
    HIGHWAY_CONSTRUCTION,
    HIGHWAY_RESERVE14,
    HIGHWAY_RESERVE15,

    HIGHWAY_TYPE_MAX
} HIGHWAY_TYPE;

typedef enum STRUCT_TYPE
{
    STRUCT_INVALID = -1,
    STRUCT_DEFAULT = 0,
    STRUCT_BRIDGE,
    STRUCT_TUNNEL,
} STRUCT_TYPE;

extern const int HIGHWAY_REF_MAX_SPEEDS[];

class WayManager;
class Segment;

typedef struct SEGMENT SEGMENT;
typedef Segment* SegmentPtr;
typedef UNORD_MAP<SEG_ID_T, SegmentPtr> SegmentMap;

enum NODE_TYPE_ENUM
{
    NDTYPE_DEFAULT = 0,
    NDTYPE_GAS_STATION = 100,
};
typedef unsigned short NODE_TYPE;

typedef enum EXCLUSION_TYPE
{
    EXTYPE_NONE = 0,
    EXTYPE_ALWAYS = 1,
    EXTYPE_DAILY_TIME_RANGE = 2,
    EXTYPE_DATETIME_RANGE = 3,
    EXTYPE_NO_GPS = 4, // route always no GPS
    EXTYPE_MAX_VALID = 4,
} EXCLUSION_TYPE;

typedef enum TIME_TYPE
{
    TIME_TYPE_LOCAL = 0,
    TIME_TYPE_UTC = 1,
} TIME_TYPE;

typedef struct EXCLUSION_SETTING
{
    EXCLUSION_TYPE ex_type{};

    // for EXTYPE_DAILY_TIME_RANGE, do not specify the time zone, callers need to make sure it is consistent with GPS time
    // for EXTYPE_DATETIME_RANGE, use local time, only interested with the h/m/s part
    time_t time_range_from{};
    time_t time_range_to{};
} EXCLUSION_SETTING;

//DESCRIPTION, EXCLUSION_TYPE, TIME_POINT_TYPE, TIME_POINT_FROM, TIME_POINT_TO, BI_DIR, LAT1, LNG1, HEADING1, ...
typedef struct EXCLUDED_ROUTE
{
    std::string description;
    geo::EXCLUSION_TYPE exclusion_type{};
    TIME_TYPE time_type{};
    time_t time_point_from{};
    time_t time_point_to{};
    bool bi_dir{}; // if 1, the reversed roads are also excluded
    std::vector<std::tuple<GeoPoint, int>> via_points; // tuple vector of (position, heading)
} EXCLUDED_ROUTE;


typedef struct NODE
{
    explicit NODE()
        : nd_id(0), nd_type(NDTYPE_DEFAULT)
    {}

    explicit NODE(NODE_ID_T nd_id, const GeoPoint& geo_point, const std::string& nd_name)
        : nd_id(nd_id), nd_type(NDTYPE_DEFAULT), geo_point(geo_point), nd_name(nd_name)
    {}

    explicit NODE(NODE_ID_T nd_id, NODE_TYPE nd_type, const GeoPoint& geo_point, const std::string& nd_name)
        : nd_id(nd_id), nd_type(nd_type), geo_point(geo_point), nd_name(nd_name)
    {}

    NODE_ID_T   nd_id;
    NODE_TYPE   nd_type;
    GeoPoint    geo_point;
    std::string nd_name;
} NODE;

class Node
{
public:
    explicit Node(NODE_ID_T nd_id, double lat, double lng)
        : nd_id_(nd_id), geo_point_(lat, lng), nd_type_(NDTYPE_DEFAULT),
        is_way_connector_(0), is_dead_end_(0), is_weak_connected(0)
    {}

    explicit Node(const NODE& nd)
        : nd_id_(nd.nd_id), geo_point_(nd.geo_point), nd_name_(nd.nd_name), nd_type_(nd.nd_type),
        is_way_connector_(0), is_dead_end_(0), is_weak_connected(0)
    {}

public:
    NODE_ID_T   nd_id_;
    GeoPoint    geo_point_;
    std::string nd_name_;
    NODE_TYPE   nd_type_;

public:
    const std::vector<SegmentPtr>& ConnectedSegments() const
    {
        return connected_segments_;
    }
    std::vector<SegmentPtr>& ConnectedSegments()
    {
        return connected_segments_;
    }

    bool IsWayConnector() const
    {
        return is_way_connector_ != 0;
    }

    // typically, ways' common node is callded routing node
    bool IsRoutingNode() const
    {
        // for way's dead ends, should be looked as way connectors
        return bool(is_way_connector_ || is_dead_end_);
    }

    // case 1: ---------->------------>o    one way case
    //
    // case 2: ---------->------------>o    two way case
    //         <---------<------------
    bool IsDeadEndNode() const
    {
        return is_dead_end_ != 0;
    }

    // if crossroad of two ways whose highway type are equal or higher than
    // the specified way type passed in
    bool IsCorssroad(HIGHWAY_TYPE way_type = HIGHWAY_UNKNOWN) const;
    // get the segment intersected with the segment passed in. for multiple
    // result, pick the one with highest way type
    SegmentPtr GetIntersectedSegment(const SEGMENT& seg) const;

    // weakly connected to the main component
    bool IsWeakConnected() const
    {
        return is_weak_connected;
    }

    // generate the splited segment's "to" node ID
    //
    //    sub=-1    sub=-2                        sub=-3
    //              split=-1  split=-2  split=-3
    // O<---------O<---------o<--------o<--------O<--------O
    // O--------->O--------->o-------->o-------->O-------->O
    //              split=1   split=2   split=3
    //    sub=1     sub=2                         sub=3
    static NODE_ID_T GenerateSplitSegToNodeID(WAY_ID_T way_id, int way_sub_seq, int split_seq)
    {
        if (way_sub_seq < 0) {
            way_sub_seq = -way_sub_seq;
        }
        if (split_seq < 0) {
            split_seq = -split_seq;
            --split_seq;
        }

        NODE_ID_T node_id = (NODE_ID_T)(way_id) << 24;
        uint32_t low24 = ((uint32_t)(way_sub_seq) << 8) | (uint32_t)split_seq;
        node_id |= low24;
        return node_id;
    }

    // generate the splited segment's "from" node ID
    static NODE_ID_T GenerateSplitSegFromNodeID(WAY_ID_T way_id, int way_sub_seq, int split_seq)
    {
        if (way_sub_seq < 0) {
            way_sub_seq = -way_sub_seq;
        }
        if (split_seq < 0) {
            split_seq = -split_seq;
        }
        else {
            --split_seq;
        }

        NODE_ID_T node_id = (NODE_ID_T)(way_id) << 24;
        uint32_t low24 = ((uint32_t)(way_sub_seq) << 8) | (uint32_t)split_seq;
        node_id |= low24;
        return node_id;
    }

private:
    unsigned is_way_connector_ : 1;
    unsigned is_dead_end_ : 1;
    unsigned is_weak_connected : 1; // weak connected to the main component
    unsigned is_visited : 1; // for internal usage
    std::vector<SegmentPtr> connected_segments_; // orderred by highway type
    friend class WayManager;
};


typedef Node* NodePtr;
typedef UNORD_MAP<NODE_ID_T, NodePtr> NodeMap;

typedef enum ORIENTATION
{
    INVALID_ORIENTATION = 0,

    SOUTH_TO_NORTH = 1,
    SOUTHWEST_TO_NORTHEAST,
    WEST_TO_EAST = 3,
    NORTHWEST_TO_SOUTHEAST,
    NORTH_TO_SOUTH = 5,
    NORTHEAST_TO_SOUTHWEST,
    EAST_TO_WEST = 7,
    SOUTHEAST_TO_NORTHWEST,

    CLOCKWISE_LOOP = 9,
    COUNTER_CLOCKWISE_LOOP
} ORIENTATION;

extern const std::string ORIENTATION_STRS[];

/*
CREATE COLUMN TABLE "SEGMENTS"(
    "SEGMENT_ID" BIGINT, -- high 40 bit: WAY_ID, low 24 bit: 16 bit SUB_ID + 8 bit SPLIT_ID.
    "FROM_LAT"  DOUBLE,
    "FROM_LNG"  DOUBLE,
    "TO_LAT"    DOUBLE,
    "TO_LNG"    DOUBLE,
    "ONE_WAY"   INTEGER,
    "LENGTH"    REAL,
    "WAY_ID"    BIGINT, -- OSM way ID
    "WAY_SUB_SEQ" SMALLINT, -- For two-way segment: WAY_SUB_SEQs are in pairs (one positive, one negative), starting from 1 or -1
    "SPLIT_SEQ" SMALLINT, -- Sub seq No for split segments. 0 means no split For two-way segment, SPLIT_SEQ are in pairs
                          -- (one positive, one negative), starting from 1 or -1
    "FROM_ND"   BIGINT, -- "from" node ID, 0: no node ID
    "TO_ND"     BIGINT, -- "to" node ID, 0: no node ID
    "WAY_TYPE"  SMALLINT,
    "WAY_NAME"  VARCHAR(256),
    "STRUCT_TYPE"   SMALLINT,
    "LAYER"     SMALLINT
)*/
typedef struct SEGMENT
{
    SEG_ID_T        seg_id{ 0 };
    double          from_lat{ 0 };
    double          from_lng{ 0 };
    double          to_lat{ 0 };
    double          to_lng{ 0 };
    double          length{ 0 }; // in meters
    WAY_ID_T        way_id{ 0 }; // OSM way ID
    NODE_ID_T       from_nd{ 0 };
    NODE_ID_T       to_nd{ 0 };
    short           way_sub_seq{ 0 };
    short           split_seq{ 0 };
    HIGHWAY_TYPE    way_type{ HIGHWAY_UNKNOWN };
    STRUCT_TYPE     struct_type{ STRUCT_DEFAULT };
    short           layer{ 0 };
    bool            one_way{ false }; // 1: one way, otherwise 0

    std::string     way_name; // default name
    std::string     opt_tags; // opt tags from OSM
    std::string     highway_type_str; // highway type: motorway, trunk, primary, secondary, etc.
} SEGMENT;


class OrientedWay;
typedef OrientedWay* OrientedWayPtr;

struct Tag
{
    std::string name;
    std::string value;
};
typedef std::vector<Tag> Tags;
typedef std::shared_ptr<Tags> SharedTagsPtr;

class Segment
{
public:
    explicit Segment(const SEGMENT& SEG)
        : seg_id_(SEG.seg_id), way_id_(SEG.way_id),
        from_nd_(SEG.from_nd), to_nd_(SEG.to_nd),
        from_point_(SEG.from_lat, SEG.from_lng), to_point_(SEG.to_lat, SEG.to_lng),
        length_(geo::distance_in_meter(SEG.from_lat, SEG.from_lng, SEG.to_lat, SEG.to_lng)),
        way_name_(SEG.way_name), highway_type_str_(SEG.highway_type_str),
        way_sub_seq_(SEG.way_sub_seq), split_seq_(SEG.split_seq), one_way_(SEG.one_way),
        way_type_(SEG.way_type), struct_type_(SEG.struct_type), layer_(SEG.layer),
        excluded_flag_(0), excluded_always_(0), excluded_no_gps_(0)
    {
        // if original node ID is negative, it means it is the node generated from segment splitting
        // NOTE: need to make sure this to_node ID same as next from_node ID
        if (SEG.from_nd == 0) {
            from_nd_ = -Node::GenerateSplitSegFromNodeID(way_id_, way_sub_seq_, split_seq_);
        }
        if (SEG.to_nd == 0) {
            to_nd_ = -Node::GenerateSplitSegToNodeID(way_id_, way_sub_seq_, split_seq_);
        }
        heading_ = (int)geo::get_heading_in_degree(from_point_, to_point_);
        p_opt_tags_ = StrToTags(SEG.opt_tags);
    }

    explicit Segment(SEG_ID_T seg_id, WAY_ID_T way_id, int way_sub_seq, int split_seq,
        NODE_ID_T from_nd, NODE_ID_T to_nd, double from_lat, double from_lng,
        double to_lat, double to_lng, bool one_way, HIGHWAY_TYPE way_type,
        const std::string& highway_type_str, const std::string& way_name,
        STRUCT_TYPE struct_type, short layer, const SharedTagsPtr &p_opt_tags)
        : seg_id_(seg_id), way_id_(way_id),
        from_nd_(from_nd), to_nd_(to_nd), from_point_(from_lat, from_lng), to_point_(to_lat, to_lng),
        length_(geo::distance_in_meter(from_lat, from_lng, to_lat, to_lng)),
        way_name_(way_name), highway_type_str_(highway_type_str),
        way_sub_seq_(way_sub_seq), split_seq_(split_seq), one_way_(one_way),
        way_type_(way_type), struct_type_(struct_type), layer_(layer),
        excluded_flag_(0), excluded_always_(0), excluded_no_gps_(0), p_opt_tags_(p_opt_tags)
    {
        // if original node ID is negative, it means it is the node generated from segment splitting
        // NOTE: need to make sure this to_node ID same as next from_node ID
        if (from_nd == 0) {
            from_nd_ = -Node::GenerateSplitSegFromNodeID(way_id_, way_sub_seq_, split_seq_);
        }
        if (to_nd == 0) {
            to_nd_ = -Node::GenerateSplitSegToNodeID(way_id_, way_sub_seq_, split_seq_);
        }
        heading_ = (int)geo::get_heading_in_degree(from_point_, to_point_);
    }

    const NodePtr& GetFromNode() const
    {
        return p_from_nd_;
    }
    const NodePtr& GetToNode() const
    {
        return p_to_nd_;
    }
    NodePtr& GetFromNode()
    {
        return p_from_nd_;
    }
    NodePtr& GetToNode()
    {
        return p_to_nd_;
    }
    void SetFromNode(const NodePtr& p_node)
    {
        p_from_nd_ = p_node;
    }
    void SetToNode(const NodePtr& p_node)
    {
        p_to_nd_ = p_node;
    }

    WAY_ID_T GetWayIdOriented() const
    {
        if (seg_id_ < 0) {
            // for seg_id < 0, it is fake reversed segment
            return -way_id_;
        }
        return (one_way_ || way_sub_seq_ > 0) ? way_id_ : -way_id_;
    }

    const OrientedWayPtr& GetWayOriented() const
    {
        return p_ori_way_;
    }

    GeoPoint GetMidPoint() const
    {
        return GeoPoint((from_point_.lat + to_point_.lat) / 2, (from_point_.lng + to_point_.lng) / 2);
    }

    void GetMidPoint(GeoPoint& mid_point) const
    {
        mid_point.lat = (from_point_.lat + to_point_.lat) / 2;
        mid_point.lng = (from_point_.lng + to_point_.lng) / 2;
    }

    // return false if not found
    // p_tag_value can be nullptr if tag value is not wanted
    bool GetTagByName(const std::string& tag_name, std::string* p_tag_value) const;

    SEGMENT ToSEGMENT() const
    {
        SEGMENT segment;
        segment.seg_id = this->seg_id_;
        segment.way_id = this->way_id_;
        segment.way_sub_seq = this->way_sub_seq_;
        segment.split_seq = this->split_seq_;
        segment.from_nd = this->from_nd_;
        segment.to_nd = this->to_nd_;
        segment.from_lat = this->from_point_.lat;
        segment.from_lng = this->from_point_.lng;
        segment.to_lat = this->to_point_.lat;
        segment.to_lng = this->to_point_.lng;
        segment.one_way = this->one_way_;
        segment.length = this->length_;
        segment.way_type = this->way_type_;
        segment.way_name = this->way_name_;
        segment.struct_type = this->struct_type_;
        segment.layer = this->layer_;
        segment.opt_tags = this->TagsToStr(p_opt_tags_);

        segment.highway_type_str = this->highway_type_str_;
        return segment;
    }

    double DistanceSquareMeters(const Node& node) const;
    double DistanceSquareMeters(const geo::GeoPoint& point) const;
    double GetDistanceInMeters(const Segment& seg);

    static double CalcDistanceSquareMeters(const geo::GeoPoint &coord, const Segment &seg);
    static double CalcDistance(const geo::GeoPoint &coord, const Segment &seg);

    static void GetOffsetLinestr(const std::vector<SegmentPtr>& segments, double offset,
        std::vector<GeoPoint>& offset_points);
    static double AdjustOffset(double offset, HIGHWAY_TYPE type);
    static SEG_ID_T GenerateSegID(WAY_ID_T way_id, int way_sub_seq, int split_seq)
    {
        SEG_ID_T seg_id = (SEG_ID_T)(way_id) << 24;
        unsigned int low24 = ((unsigned int)((unsigned short)way_sub_seq) << 8) |
            (unsigned int)(unsigned char)(char)split_seq;
        seg_id |= low24;
        return seg_id;
    }
    static std::string TagsToStr(const SharedTagsPtr &tags);
    static SharedTagsPtr StrToTags(const std::string &tags_str);

public:
    SEG_ID_T seg_id_;
    WAY_ID_T way_id_;
    NODE_ID_T from_nd_, to_nd_;
    GeoPoint from_point_, to_point_;
    double length_; // in meters
    std::string way_name_;
    std::string highway_type_str_; // generated fields
    int way_sub_seq_ : 16;
    int split_seq_ : 8;
    bool one_way_;
    HIGHWAY_TYPE way_type_;
    STRUCT_TYPE struct_type_;
    int heading_ : 12; // generated fields
    int layer_ : 8;
    unsigned excluded_flag_ : 1;
    unsigned excluded_always_ : 1;
    unsigned excluded_no_gps_ : 1;

private:
    NodePtr p_from_nd_{}, p_to_nd_{};
    OrientedWayPtr p_ori_way_{};
    SharedTagsPtr p_opt_tags_{};

    friend class WayManager;
    friend class OrientedWay;
};


// for opposite oriented way, way ID is negative
typedef UNORD_MAP<WAY_ID_T, OrientedWayPtr> OrientedWayMap;

class OrientedWay
{
public:
    explicit OrientedWay(WAY_ID_T way_id, bool one_way)
        : way_id_(way_id), one_way_(one_way), p_opposite_way_{nullptr}
    {}

    void AddSegment(const SegmentPtr& p_seg);
    void DoneAddSegment(const WayManager& way_manager);

    bool IsOppositeWay() const
    {
        return way_id_ < 0;
    }

    HIGHWAY_TYPE HighwayType() const
    {
        return segments_.front()->way_type_;
    }

    // get the way ID of the oriented way, could be negative
    WAY_ID_T WayId() const
    {
        return way_id_;
    }

    const std::string& WayName() const
    {
        return segments_.front()->way_name_;
    }

    const bool OneWay() const
    {
        return one_way_;
    }

    const std::vector<SegmentPtr>& Segments() const
    {
        return segments_;
    }

    const std::vector<NodePtr>& Nodes() const
    {
        return nodes_;
    }

    OrientedWayPtr GetOppositeWay() const
    {
        return one_way_ ? nullptr : p_opposite_way_;
    }

    const geo::Bound& BoundBox() const
    {
        return bbox_;
    }

    static std::vector<SegmentPtr>::const_iterator FindSegment(
        std::vector<SegmentPtr>::const_iterator it_from, std::vector<SegmentPtr>::const_iterator it_to,
        SEG_ID_T seg_id)
    {
        for (std::vector<SegmentPtr>::const_iterator it = it_from; it != it_to; ++it) {
            if ((*it)->seg_id_ == seg_id) {
                return it;
            }
        }
        return it_to;
    }

    std::vector<SegmentPtr>::const_iterator FindSegment(SEG_ID_T seg_id) const
    {
        return FindSegment(segments_.begin(), segments_.end(), seg_id);
    }

    static std::vector<SegmentPtr>::const_iterator FindSegmentByToNode(
        std::vector<SegmentPtr>::const_iterator it_from, std::vector<SegmentPtr>::const_iterator it_to,
        NODE_ID_T node_id)
    {
        for (std::vector<SegmentPtr>::const_iterator it = it_from; it != it_to; ++it) {
            if ((*it)->to_nd_ == node_id) {
                return it;
            }
        }
        return it_to;
    }

private:
    WAY_ID_T way_id_; // for opposite oriented way, way ID is negative
    std::string name_; // way name in the default language
    bool one_way_;
    std::vector<SegmentPtr> segments_;
    std::vector<NodePtr> nodes_;
    geo::Bound bbox_;

private:
    OrientedWayPtr p_opposite_way_; // valid only for two way

    friend class WayManager;
};

namespace seg {
    class SegmentManager;
}
namespace node {
    class NodeManager;
}
namespace route {
    class RouteManager;
}

struct SegAssignParams
{
    int heading{ -1 };  // [0, 359] for normal case. -1 if ignore heading.
    double radius{}; // > 0, in meters
    int angle_tollerance{};

    time_t dev_data_time{}; // if non-zero, do not assign to excluded segs (e.g., closed tunnel in the middle night)
    bool dev_data_local_time{}; // is dev_data_time local time or utc time

    char *way_name{}; // if specified, only assign to way with the name
    bool no_road_link{};
    bool no_bridge{};
    bool no_tunnel{};
    bool check_no_gps_route{ true };

    // when LoadSegments() with reverse_seg enabled, set ignore_reverse_segs to true to ignore the
    // reversed segments during assignment
    bool ignore_reverse_segs{};
    SEG_ID_T *excluded_seg_ids{};
    int excluded_seg_count{};
    HIGHWAY_TYPE *excluded_types{};
    int excluded_type_count{};
    short *included_layers{};
    int included_layer_count{};
};

struct SegAssignRes
{
    SegmentPtr  p_seg{};
    float       score{};
    float       distance{};
    short       heading_distance{};
    bool        exclusive{};
};
typedef std::vector<SegAssignRes> SegAssignResults;


struct ViaPoint
{
    explicit ViaPoint()
    {}
    explicit ViaPoint(const geo::GeoPoint& point, int heading)
        : geo_point(point), heading(heading)
    {}
    explicit ViaPoint(double lat, double lng, int heading)
        : geo_point(lat, lng), heading(heading)
    {}

    geo::GeoPoint geo_point;
    int heading{-1};  // [0, 359]
};

// below are for WayManager::ViaRoute function
struct ViaRouteParams
{
    std::vector<ViaPoint> via_points;

    // for via points map matching
    double radius{}; // > 0, in meters
    int angle_tollerance{};

    time_t dev_data_time{}; // if non-zero, do not assign to excluded segs (e.g., closed tunnel in the middle night)
    bool dev_data_local_time{}; // is dev_data_time local time or utc time
    bool check_no_gps_route{ true };

    bool calc_used_time{};
};

struct ViaRouteResult
{
    std::vector<SegmentPtr> seg_route;
    std::string err; // error string if returned false
    double used_time{}; // time in seconds
    int search_steps{};

    void Clear()
    {
        seg_route.clear();
        err.clear();
        used_time = .0;
    }
};


// below are for Points-Route matching
struct RouteMatchingViaPoint
{
    explicit RouteMatchingViaPoint()
    {}
    explicit RouteMatchingViaPoint(double lat, double lng, int heading, int speed, time_t tm)
        : geo_point(lat, lng), heading(heading), speed(speed), record_time(tm)
    {}

    // inputs
    GeoPoint    geo_point;
    int         heading{};  // normal range: [0, 359], if -1, does not check heading
    int         speed{};    // as a reference. if -1.0, ignore the speed
    time_t      record_time{}; // for GPS record, it is typically GPS time
    int         index{};    // can be set as the index of the points array/vector for backtrace

    // as ouput, the result segment
    // as input, used as exclusively matched segment
    SegmentPtr  p_seg{};
    // outputs
    int         i_seg{ -1 }; // index of segment the RouteMatchingParams::result_route
    bool        is_broken{}; // indicate if route is broken at this point
    bool        entering_no_gps_route{}; // indicate if going to enter no GPS route like tunnel
};

struct RouteMatchingParams;
void RouteMatchingInternalCleanup(RouteMatchingParams &params);

struct RouteMatchingParams
{
    std::vector<RouteMatchingViaPoint> via_points; // input/output
    bool is_localtime{ true }; // time info in via_points[]

    // for via points map matching
    double  radius{ 40.0 };         // > 0, in meters, input
    int     angle_tollerance{ 40 }; // input
    bool    check_no_gps_route{ true };
    bool    verfy_result{};         // whether to verfy the result and set the flags
    double  distance_limit{ 0.0 };  // each point in the trace should be in the range. if 0, the methond
                                    // automatically set a limit

    // output, can be broken, broken point indicated in RouteMatchingViaPoint
    std::vector<SegmentPtr> result_route;
    std::string             err; // error string if returned false
    int disconnected_index{};   // if verfy_result is true, check and set it to the index of first
                                // disconnected segment in result_route[] if found
    int repeated_index{};       // if verfy_result is true, check and set it to the index of first
                                // repeated segment in result_route[] if found

    // internal use only
    void *p_impl{};

    ~RouteMatchingParams()
    {
        if (p_impl) {
            RouteMatchingInternalCleanup(*this);
            p_impl = nullptr;
        }
    }
};


class WayManager
{
public:
    explicit WayManager()
        : drive_on_right_(true)
    {}

    explicit WayManager(const Bound& bound, bool drive_on_right = true, time_t local_utc_diff = 8 * 3600)
        : bound_(bound), drive_on_right_(drive_on_right)
    {}

    void SetBoundries(const Bound& bound)
    {
        bound_ = bound;
    }

    const Bound& GetBoundries() const
    {
        return bound_;
    }

    const std::string& GetErrorString() const
    {
        return GetCurThreadErrStr(threads_err_mutex_, threads_err_strs_);
    }
    const char *GetErrorCString() const
    {
        return GetErrorString().c_str();
    }

private:
    template <typename SEGMENT_T>
    friend bool WayManager_LoadSegmentsT(WayManager &way_manager,
        const std::vector<SEGMENT_T> &segs, bool reversed_seg);

public:
    // paramteer reverse_seg - true: automatically generated fake reversed segments which are useful for
    //   special vehicles (e.g., bus) assignment and routing
    bool LoadSegments(const std::string &segs_csv, bool reversed_seg = false);
    bool LoadSegments(const std::vector<SEGMENT> &segs, bool reversed_seg = false);
    bool LoadSegments(const std::vector<SEGMENT*> &p_segs, bool reversed_seg = false);
    static bool LoadSegmentsFromCsv(const std::string& in_segments_csv, std::vector<SEGMENT> &segs,
        std::string& err);

public:
    size_t GetSegCount() const
    {
        return seg_map_.size();
    }

    const SegmentPtr GetSegById(SEG_ID_T seg_id) const
    {
        SegmentMap::const_iterator it = seg_map_.find(seg_id);
        return it == seg_map_.end() ? nullptr : it->second;
    }

    const std::vector<Segment>& GetAllSegs() const
    {
        return seg_pool_.AllObjs();
    }

    // return the revsersed segment whose from/to nodes are the to/from nodes of the p_seg
    SegmentPtr GetReversedSeg(const SegmentPtr& p_seg) const;

    NodePtr GetNodeById(NODE_ID_T node_id) const
    {
        NodeMap::const_iterator it = node_map_.find(node_id);
        return it == node_map_.end() ? nullptr : it->second;
    }

    // To find the opposite way, set way_id to negative
    OrientedWayPtr GetWayById(WAY_ID_T signed_way_id) const
    {
        OrientedWayMap::const_iterator it = way_map_.find(signed_way_id);
        return it == way_map_.end() ? nullptr : it->second;
    }

    const OrientedWayMap& GetWayMap() const
    {
        return way_map_;
    }

    const std::vector<OrientedWay>& GetAllWays() const
    {
        return ori_way_pool_.AllObjs();
    }

    static ORIENTATION CalcOrientation(const std::vector<geo::GeoPoint> &path, double begin_end_threshold = 5.0);
    static int CalcDirection01(const std::vector<geo::GeoPoint> &path, double begin_end_threshold = 5.0);
    static ORIENTATION HeadingToORIENTATION(int heading);
    static HIGHWAY_TYPE HighwayStrToType(const std::string& highway_type_str);
    static const char* HighwayTypeToStr(HIGHWAY_TYPE type);
    // Default mapping of OSM highway type to category (1/2/3)
    static int HighWayTypeToCategory(HIGHWAY_TYPE type);
    static std::shared_ptr<GeoObj_LineString> SegmentToGsonLineString(const Segment& segment,
        double two_way_offset);
    // always return value in range [0, 180]
    // preconditon: heading1 and heading2 are in range [0, 359]
    static int GetAngle(int heading1, int heading2)
    {
        int angle = (heading1 > heading2) ? heading1 - heading2 : heading2 - heading1;
        return (angle > 180) ? 360 - angle : angle;
    }

    bool InBoundary(double lat, double lng) const
    {
        return (lat >= bound_.minlat && lat <= bound_.maxlat && lng >= bound_.minlng && lng <= bound_.maxlng);
    }

    bool LatOutBoundary(double lat) const
    {
        return (lat < bound_.minlat || lat > bound_.maxlat);
    }

    bool LngOutBoundary(double lng) const
    {
        return (lng < bound_.minlng || lng > bound_.maxlng);
    }

    bool SegsToJson(const Bound& bound, double offset, const std::string& seg_color,
        const std::string& node_color, const std::string& pathname) const;
    bool SegsToJson(const std::vector<SegmentPtr>& segs, double offset,
        const std::string& seg_color, const std::string& node_color,
        const std::string& pathname) const;

    bool DriveOnRight() const
    {
        return drive_on_right_;
    }

private:
    bool InitWayMap();
    bool InitNodeConnectedSegs();
    bool GenerateReversedSegsIntoSegMap();
    static void DoneAddNodeMap(util::SimpleObjPool<Node>& node_pool);
    static void FlagNodeInternals(util::SimpleObjPool<Node>& node_pool);
    static void FlagWeakConnectivity(util::SimpleObjPool<Node>& node_pool);

    static const std::string& GetCurThreadErrStr(std::mutex& threads_err_mutex,
        UNORD_MAP<std::string, std::string>& threads_err_strs);
    static void SetCurThreadErrStr(std::mutex& threads_err_mutex,
        UNORD_MAP<std::string, std::string>& threads_err_strs,
        const std::string& str);
    void SetErrorString(const std::string& str) const
    {
        SetCurThreadErrStr(threads_err_mutex_, threads_err_strs_, str);
    }

private:
    Bound           bound_;
    NodeMap         node_map_;
    SegmentMap      seg_map_;
    OrientedWayMap  way_map_;
    bool            drive_on_right_ = {true};

    mutable std::mutex threads_err_mutex_;
    mutable UNORD_MAP<std::string, std::string> threads_err_strs_;

    util::SimpleObjPool<Node> node_pool_;
    util::SimpleObjPool<Segment> seg_pool_, seg_pool_rev_;
    util::SimpleObjPool<OrientedWay> ori_way_pool_;

public:
    // routing related all put to below

    // initialization for short distance routing
    // parameter shortest_mode - true: optimized for shortest route mode
    bool InitForRouting(bool shortest_mode = true);

    // find the route to the nearby segment
    // param ignore_reversed_segs: when LoadSegments() with reversed_seg enabled, set ignore_reversed_segs to true
    //    to ignore the reverse segments during routing
    // param time_point: if non-zero, also check the excluded segs (e.g., closed tunnel in the middle night)
    // param points_reversed: point1 is ahead of point2, only effective when seg_id1 = seg_id2
    //          seg1 = seg2    ---------o----------o---------->
    //                                point2    point1
    bool RoutingNearby(const SegmentPtr& p_seg1, const SegmentPtr& p_seg2, std::vector<SegmentPtr>& route,
        bool exclude_reversed_segs = false, time_t time_point = 0, bool is_localtime = false,
        bool points_reversed = false) const;
    bool RoutingNearby(SEG_ID_T seg_id1, SEG_ID_T seg_id2, std::vector<SegmentPtr>& route,
        bool exclude_reversed_segs = false, time_t time_point = 0, bool is_localtime = false,
        bool points_reversed = false) const
    {
        const SegmentPtr p_seg1 = GetSegById(seg_id1);
        if (p_seg1 == nullptr) {
            return false;
        }
        if (seg_id1 == seg_id2) {
            return RoutingNearby(p_seg1, p_seg1, route, exclude_reversed_segs, time_point,
                is_localtime, points_reversed);
        }
        else {
            const SegmentPtr p_seg2 = GetSegById(seg_id2);
            if (p_seg2 == nullptr) {
                return false;
            }
            return RoutingNearby(p_seg1, p_seg2, route, exclude_reversed_segs, time_point,
                is_localtime, points_reversed);
        }
    }

    bool SimilarRoutingNearby(const SegmentPtr& p_seg1, const SegmentPtr& p_seg2,
        const std::vector<GeoPoint>& trace, std::vector<SegmentPtr>& route, double &distance,
        bool exclude_reversed_segs = false) const;
    bool SimilarRoutingNearby(SEG_ID_T seg_id1, SEG_ID_T seg_id2,
        const std::vector<GeoPoint>& trace, std::vector<SegmentPtr>& route, double &distance,
        bool exclude_reversed_segs = false) const
    {
        const SegmentPtr p_seg_id1 = GetSegById(seg_id1);
        if (p_seg_id1 == nullptr) {
            return false;
        }
        if (seg_id1 == seg_id2) {
            return SimilarRoutingNearby(p_seg_id1, p_seg_id1, trace, route, distance, exclude_reversed_segs);
        }
        else {
            const SegmentPtr p_seg_id2 = GetSegById(seg_id2);
            if (p_seg_id2 == nullptr) {
                return false;
            }
            return SimilarRoutingNearby(p_seg_id1, p_seg_id2, trace, route, distance, exclude_reversed_segs);
        }
    }

    bool SimilarRoutingNearby(const SegmentPtr& p_seg1, const SegmentPtr& p_seg2,
        const std::vector<ViaPoint>& trace, std::vector<SegmentPtr>& route, double &distance,
        bool exclude_reversed_segs = false) const;
    bool SimilarRoutingNearby(SEG_ID_T seg_id1, SEG_ID_T seg_id2,
        const std::vector<ViaPoint>& trace, std::vector<SegmentPtr>& route, double &distance,
        bool exclude_reversed_segs = false) const
    {
        const SegmentPtr p_seg_id1 = GetSegById(seg_id1);
        if (p_seg_id1 == nullptr) {
            return false;
        }
        if (seg_id1 == seg_id2) {
            return SimilarRoutingNearby(p_seg_id1, p_seg_id1, trace, route, distance, exclude_reversed_segs);
        }
        else {
            const SegmentPtr p_seg_id2 = GetSegById(seg_id2);
            if (p_seg_id2 == nullptr) {
                return false;
            }
            return SimilarRoutingNearby(p_seg_id1, p_seg_id2, trace, route, distance, exclude_reversed_segs);
        }
    }

    // shorted path. firstly try RoutingNearby, then Dijkstra algorithm, returns the route in segments
    bool ShortestPath(const SegmentPtr& p_seg1, const SegmentPtr& p_seg2, std::vector<SegmentPtr>& route,
        bool exclude_reversed_segs = false, time_t time_point = 0, bool is_localtime = false,
        bool points_reversed = false) const
    {
        bool ok = RoutingNearby(p_seg1, p_seg2, route, exclude_reversed_segs, time_point,
            is_localtime, points_reversed);
        if (!ok) {
            if (p_seg1->GetDistanceInMeters(*p_seg2) > 200) {
                ok = DijkstraShortestPath(p_seg1, p_seg2, route, nullptr, time_point, is_localtime,
                    points_reversed);
            }
        }
        return ok;
    }
    bool ShortestPath(SEG_ID_T seg_id1, SEG_ID_T seg_id2, std::vector<SegmentPtr>& route,
        bool exclude_reversed_segs = false, time_t time_point = 0, bool is_localtime = false,
        bool points_reversed = false) const
    {
        const SegmentPtr p_seg1 = GetSegById(seg_id1);
        if (p_seg1 == nullptr) {
            return false;
        }
        if (seg_id1 == seg_id2) {
            return this->ShortestPath(p_seg1, p_seg1, route, exclude_reversed_segs,
                time_point, is_localtime, points_reversed);
        }
        else {
            const SegmentPtr p_seg2 = GetSegById(seg_id2);
            if (p_seg2 == nullptr) {
                return false;
            }
            return this->ShortestPath(p_seg1, p_seg2, route, exclude_reversed_segs,
                time_point, is_localtime, points_reversed);
        }
    }

    // shorted path by Dijkstra algorithm, returns the route in segments
    bool DijkstraShortestPath(const SegmentPtr& p_seg1, const SegmentPtr& p_seg2,
        std::vector<SegmentPtr>& route, int *seach_steps = nullptr,
        time_t time_point = 0, bool is_localtime = false,
        bool points_reversed = false) const;
    bool DijkstraShortestPath(SEG_ID_T seg_id1, SEG_ID_T seg_id2, std::vector<SegmentPtr>& route,
        int *seach_steps = nullptr, time_t time_point = 0, bool is_localtime = false,
        bool points_reversed = false) const
    {
        const SegmentPtr p_seg1 = GetSegById(seg_id1);
        if (p_seg1 == nullptr) {
            return false;
        }
        if (seg_id1 == seg_id2) {
            return this->DijkstraShortestPath(p_seg1, p_seg1, route, seach_steps,
                time_point, is_localtime, points_reversed);
        }
        else {
            const SegmentPtr p_seg2 = GetSegById(seg_id2);
            if (p_seg2 == nullptr) {
                return false;
            }
            return this->DijkstraShortestPath(p_seg1, p_seg2, route, seach_steps,
                time_point, is_localtime, points_reversed);
        }
    }

    // shorted path by Dijkstra algorithm, returns the route in edges
    bool DijkstraShortestEdgePath(const SegmentPtr& p_seg1, const SegmentPtr& p_seg2,
        std::vector<EDGE_ID_T>& route) const;
    bool DijkstraShortestEdgePath(SEG_ID_T seg_id1, SEG_ID_T seg_id2, std::vector<EDGE_ID_T>& route) const
    {
        const SegmentPtr p_seg1 = GetSegById(seg_id1);
        if (p_seg1 == nullptr) {
            return false;
        }
        if (seg_id1 == seg_id2) {
            return this->DijkstraShortestEdgePath(p_seg1, p_seg1, route);
        }
        else {
            const SegmentPtr p_seg2 = GetSegById(seg_id2);
            if (p_seg2 == nullptr) {
                return false;
            }
            return this->DijkstraShortestEdgePath(p_seg1, p_seg2, route);
        }
    }

    // precondition: InitSegServices
    bool ViaRoute(const ViaRouteParams &params, ViaRouteResult &result) const;

    // precondition: InitSegServices
    // the difference between ViaRoute and RouteMatching: ViaRoute uses static segment
    //   assignment function, RouteMatching uses complex dynamic segment assignment algorithm.
    //   cost of RouteMatching is bigger than ViaRoute
    bool RouteMatching(RouteMatchingParams &params) const;
    bool RouteMatchingResultToJson(const RouteMatchingParams &params,
        const std::string &pathname) const;

    //                              A s1
    //           p_seg              |  s2
    //   O------>----->------>----->O---->----->
    //                              |
    //                              V s3
    // node_id: if node ID is 0, the API will automatically check the most closest "routing node"
    // output: [s1, s2, s3]
    bool GetAdjacentOutboundSegs(const SegmentPtr& p_seg, NODE_ID_T node_id,
        std::vector<SegmentPtr>& out_segs) const;
    bool GetAdjacentOutboundSegs(SEG_ID_T seg_id, NODE_ID_T node_id,
        std::vector<SegmentPtr>& out_segs) const
    {
        const SegmentPtr p_seg = GetSegById(seg_id);
        if (p_seg == nullptr) {
            return false;
        }
        return GetAdjacentOutboundSegs(p_seg, node_id, out_segs);
    }

    //    p_seg0       p_seg
    //   O------>----->------>----->O---->----->O
    // return: p_seg0
    SegmentPtr GetLeadSeg(const SegmentPtr& p_seg) const;
    SegmentPtr GetLeadSeg(SEG_ID_T seg_id) const
    {
        const SegmentPtr p_seg = GetSegById(seg_id);
        return (p_seg != nullptr) ? GetLeadSeg(p_seg) : nullptr;
    }

    // Calculate the segment route's length, not including the 1st and last segments
    static inline double GetRouteLength(const std::vector<SegmentPtr>& route)
    {
        size_t size = route.size();
        if (size < 2) {
            return 0;
        }

        double length = 0;
        for (size_t i = 1; i < size - 1; ++i) {
            length += route[i]->length_;
        }
        return length;
    }

    // convert the distance (in meters) to internal weight used by routing
    static int DistanceToWeight(double distance, HIGHWAY_TYPE type);

    // for debug purpose only, dose not function in release code
    void GetRoutingSummary(int& total_request, int& success_request);

private:
    friend class route::RouteManager;
    std::shared_ptr<route::RouteManager> p_route_manager_;

public:
    enum MATCH_PRI {
        MATCH_PRI_DEFAULT = 0,
        MATCH_PRI_DISTANCE = 1,
        MATCH_PRI_ANGLE = 2
    };
    // segment related services (assignment, etc.) related all put to below
    // param match_priority: used by segment assignment matching method:
    bool InitSegServices(const Bound& bound, MATCH_PRI match_priority = MATCH_PRI_DEFAULT);

    bool SetExclusionSegs(std::vector<SegmentPtr> &segs, const EXCLUSION_SETTING &setting, bool sync_for_routing);
    bool IsSegmentExcluded(const SegmentPtr &p_seg, time_t dev_data_time, bool is_localtime) const;
    bool AreSegmentsExcluded(const std::vector<SegmentPtr> &segs, time_t dev_data_time, bool is_localtime) const
    {
        for (auto &p_seg : segs) {
            if (IsSegmentExcluded(p_seg, dev_data_time, is_localtime)) {
                return true;
            }
        }
        return false;
    }
    bool SetNoGpsTunnelRoute(const std::vector<SegmentPtr> &tunnel_route);

    static bool LoadExclusionRoutesFromCsv(const std::string &pathname,
        std::vector<EXCLUDED_ROUTE> &execluded_routes, std::string& err);
    // precondtion: routing service is ready
    bool SetExclusionRoutes(const std::vector<EXCLUDED_ROUTE> &execluded_routes, bool sync_for_routing);
    bool SetExclusionRoutesFromCsv(const std::string &pathname, bool sync_for_routing)
    {
        std::vector<EXCLUDED_ROUTE> execluded_routes;
        std::string err;
        bool ok = LoadExclusionRoutesFromCsv(pathname, execluded_routes, err);
        if (ok) {
            ok = SetExclusionRoutes(execluded_routes, sync_for_routing);
        }
        else {
            SetErrorString(err);
        }
        return ok;
    }
    bool ExcludedRoutesToJsons(const std::vector<EXCLUDED_ROUTE> &ex_routes, const std::string &pathname_prefix) const;
    void SyncExclusionSegsToRouting();

    SegmentPtr AssignSegment(double lat, double lng, int heading, double radius, int angle_tollerance,
        const char *way_name = nullptr, bool no_road_link = false, bool ignore_reversed_segs = false,
        const SEG_ID_T *excluded_seg_ids = nullptr, int excluded_seg_count = 0) const;
    SegmentPtr AssignSegment(const geo::GeoPoint& point, int heading, double radius, int angle_tollerance,
        const char *way_name = nullptr, bool no_road_link = false, bool ignore_reversed_segs = false,
        const SEG_ID_T *excluded_seg_ids = nullptr, int excluded_seg_count = 0) const;
    SegmentPtr AssignSegment(const geo::GeoPoint& point, const SegAssignParams& params,
        SegAssignResults *p_results = nullptr) const;

    // NOTE: this API is a byproduct of segment assignment. Limitation: max search radius limited
    // Param has_name - true: only segments with names are returned, false: does not check names
    bool FindAdjacentSegments(double lat, double lng, double radius, bool has_name,
        std::vector<std::tuple<SegmentPtr, double> >& segs) const;
    bool FindAdjacentSegments(const geo::GeoPoint& point, double radius, bool has_name,
        std::vector<std::tuple<SegmentPtr, double> >& segs) const;

private:
    friend class seg::SegmentManager;
    std::shared_ptr<seg::SegmentManager> p_seg_manager_;

public:
    // node locating related all put to below
    typedef std::tuple<NodePtr, double> NODE_SEARCH_RESULT;
    bool InitForNodeLocating(const Bound& bound, const std::vector<NODE>& nodes);
    bool FindAdjacentNodes(const geo::GeoPoint& point, double radius, bool has_name,
        std::vector<NODE_SEARCH_RESULT>& results) const;
    bool FindAdjacentNodes(double lat, double lng, double radius, bool has_name,
        std::vector<NODE_SEARCH_RESULT>& results) const;
private:
    friend class node::NodeManager;
    std::shared_ptr<node::NodeManager> p_node_manager_;
};

}

#endif // _WAY_MANAGER_H_
