
#include "way_manager.h"
#include <algorithm>
#include <memory>
#include <climits>
#include <set>
#include <array>
#include "common/common_utils.h"
#if WAY_MANAGER_HANA_LOG == 1
#include <hana/logging.h>
#endif

#ifdef max
#undef max
#endif

using namespace std;

namespace geo {
namespace route_match {

struct GraphEdge;
typedef GraphEdge *GraphEdgePtr;

struct GraphNode
{
    explicit GraphNode(SegmentPtr p_seg)
        : p_seg_(p_seg)
    {}

    SegmentPtr p_seg_;
    std::vector<GraphEdgePtr> edges_;
};
typedef GraphNode *GraphNodePtr;

struct GraphEdge
{
    explicit GraphEdge(GraphNodePtr p_from_g_node, GraphNodePtr p_to_g_node, int distance)
        : p_from_g_node_(p_from_g_node), p_to_g_node_(p_to_g_node), distance_(distance)
    {}

    GraphNodePtr p_from_g_node_;
    GraphNodePtr p_to_g_node_;
    int distance_;
    // segs path between two GraphNodePtr, including the 1st and the last
    std::vector<SegmentPtr> segs_;
};

struct NodeData
{
    explicit NodeData(GraphNodePtr p_g_node, GraphNodePtr p_pre_g_node, int distance)
        : p_g_node_(p_g_node), p_pre_g_node_(p_pre_g_node), distance(distance), finished(false)
    {}

    GraphNodePtr p_g_node_;
    GraphNodePtr p_pre_g_node_;
    int  distance; // distance from source to this node
    bool finished; // indicating if in the "finished" node set
};

typedef NodeData *NodeDataPtr;

// map from GraphNodePtr to NodeDataPtr for inserted nodes
// NOTE: the class assumes GraphNode objects are linearly allocated
class InsertedNodeMap
{
public:
    InsertedNodeMap()
    {}

    void Init(int max_node, const util::SimpleObjPool<GraphNode> *p_gnode_pool)
    {
        max_node_ = max_node;
        inserted_.resize(max_node);
        p_gnode_pool_ = const_cast<util::SimpleObjPool<GraphNode> *>(p_gnode_pool);
    }

    void Clear()
    {
        max_node_ = 0;
        inserted_.clear();
    }

    NodeDataPtr& operator[](GraphNodePtr p_gnode)
    {
        auto p_gnode0 = p_gnode_pool_->ObjPtrByIndex(0);
        if (p_gnode >= p_gnode0 && p_gnode < p_gnode0 + max_node_) {
            return inserted_[p_gnode - p_gnode0];
        }
        throw std::range_error("out of bound");
    }
    NodeDataPtr operator[](GraphNodePtr p_gnode) const
    {
        auto p_gnode0 = p_gnode_pool_->ObjPtrByIndex(0);
        if (p_gnode >= p_gnode0 && p_gnode < p_gnode0 + max_node_) {
            return inserted_[p_gnode - p_gnode0];
        }
        throw std::range_error("out of bound");
    }

private:
    int max_node_{};
    util::SimpleObjPool<GraphNode> *p_gnode_pool_{};
    // as the key objects are linearly allocated, the map is optimized to use vector
    // instead of hash map
    std::vector<NodeDataPtr> inserted_;
};

struct RouteMatchingImpl;
class BinHeap
{
public:
    explicit BinHeap(int max_node, const util::SimpleObjPool<GraphNode> &gnode_pool)
        : max_node_(max_node)
    {
        node_pool_.Reserve(max_node);
        heap_.reserve(max_node);
        inserted_.Init(max_node, &gnode_pool);
    }

    bool Empty() const
    {
        return heap_.empty();
    }

    // return new created NodeData
    NodeDataPtr Insert(GraphNodePtr p_gnode, GraphNodePtr p_pre_gnode, int distance)
    {
        auto& p_node = inserted_[p_gnode];

        if (p_node == nullptr) {
            p_node = node_pool_.AllocNew(NodeData(p_gnode, p_pre_gnode, distance));
            heap_.emplace_back(p_node);

            push_heap(heap_.begin(), heap_.end(),
                [this](const NodeDataPtr i, const NodeDataPtr j) {
                return i->distance > j->distance; // greater comp function to make a min heap
            });
        }

        return p_node;
    }

    // return nullptr if was not inserted
    NodeDataPtr GetIfInserted(GraphNodePtr p_g_node) const
    {
        return inserted_[p_g_node];
    }

    NodeDataPtr DeleteMin()
    {
        NodeDataPtr min_node = heap_.front();
        pop_heap(heap_.begin(), heap_.end(),
            [this](const NodeDataPtr &i, const NodeDataPtr &j){
            return i->distance > j->distance;
        });
        heap_.pop_back();
        return min_node;
    }

    void DecreaseKey(NodeDataPtr p_node, int d)
    {
        if (p_node->distance > d) {
            p_node->distance = d;

            make_heap(heap_.begin(), heap_.end(),
                [this](const NodeDataPtr& i, const NodeDataPtr& j) {
                return i->distance > j->distance;
            });
        }
    }

    void DecreaseKeys(const vector<tuple<NodeDataPtr, int>>& pairs)
    {
        const auto pair_size = pairs.size();
        if (pair_size == 1) {
            const auto& tp = pairs.front();
            DecreaseKey(get<0>(tp), get<1>(tp));
        }
        else {
            NodeDataPtr p_node;
            int d;
            for (auto& tp : pairs) {
                tie(p_node, d) = tp;
                if (p_node->distance > d) {
                    p_node->distance = d;
                }
            }
            make_heap(heap_.begin(), heap_.end(),
                [this](const NodeDataPtr &i, const NodeDataPtr &j) {
                return i->distance > j->distance;
            });
        }
    }

    GraphNodePtr GetPreNode(GraphNodePtr p_gnode) const
    {
        auto p_node = inserted_[p_gnode];
        return (p_node == nullptr) ? nullptr : p_node->p_pre_g_node_;
    }

private:
    const int max_node_;
    util::SimpleObjPool<NodeData> node_pool_;
    vector<NodeDataPtr> heap_;
    InsertedNodeMap inserted_;

    friend struct geo::route_match::RouteMatchingImpl;
};

struct Graph
{
    util::SimpleObjPool<GraphNode> g_node_pool;
    util::SimpleObjPool<GraphEdge> g_edge_pool;
    std::vector<GraphNodePtr> all_g_nodes;
    std::vector<GraphEdgePtr> all_g_edges;

    void Clear()
    {
        g_node_pool.Clear();
        g_edge_pool.Clear();
        all_g_nodes.clear();
        all_g_edges.clear();
    }
};

static const size_t MAX_CANDIATES_COUNT = 3;
static const int INVALID_WEIGHT = std::numeric_limits<int>::max();
static const double MAX_SEGMENT_LEN = 200.0;

// below are used by RouteMatching
struct RouteMatchingImpl
{
public:
    struct MatchingPoint
    {
        RouteMatchingViaPoint *p_via_point{};
        std::array<SegmentPtr, MAX_CANDIATES_COUNT>   p_segs{};
        std::array<GraphNodePtr, MAX_CANDIATES_COUNT> p_gnodes{};

        void SetToNonMatched()
        {
            p_segs.fill(nullptr);
        }

        // return true if only has one matched segment
        bool IsExclusiveMatched() const
        {
            if (p_segs[0] == nullptr) {
                return false;
            }
            for (size_t i = 1; i < p_segs.size(); ++i) {
                if (p_segs[i] != nullptr) {
                    return false;
                }
            }
            return true;
        }

        bool HasMatched() const
        {
            for (const auto& p_seg : p_segs) {
                if (p_seg) {
                    return true;
                }
            }
            return false;
        }

        bool NonMatched() const
        {
            for (const auto& p_seg : p_segs) {
                if (p_seg) {
                    return false;
                }
            }
            return true;
        }
    };

    struct MatchingPointGroup
    {
        int group_index{};
        int from_index{};
        int to_index{};
    };

    struct RoutingPair
    {
        MatchingPoint *p_from_mat_pt{}, *p_to_mat_pt{};
        SegmentPtr p_seg_from{}, p_seg_to{};
        GraphNodePtr p_gnode_from{}, p_gnode_to{};
        std::vector<SegmentPtr> seg_route;
        int route_weight{ INVALID_WEIGHT }; // weighted length

        void Init(MatchingPoint* p_from_mat_pt, MatchingPoint* p_to_mat_pt,
            SegmentPtr p_seg_from, SegmentPtr p_seg_to,
            GraphNodePtr p_gnode_from, GraphNodePtr p_gnode_to)
        {
            this->p_from_mat_pt = p_from_mat_pt;
            this->p_to_mat_pt = p_to_mat_pt;
            this->p_seg_from = p_seg_from;
            this->p_seg_to = p_seg_to;
            this->p_gnode_from = p_gnode_from;
            this->p_gnode_to = p_gnode_to;

            seg_route.clear();
            route_weight = INVALID_WEIGHT;
        }
    };

private:
    double DFT_POINT_DIST_THRESHOLD; // default distance limit
    Graph g_;
    mutable std::vector<ViaPoint> temp_via_points_;
    mutable std::vector<SegmentPtr> temp_segs_;

    const WayManager &way_manager_;
    RouteMatchingParams &matching_params_;
    std::vector<MatchingPoint> matching_points_;
    std::vector<MatchingPointGroup> groups_;

public:
    explicit RouteMatchingImpl(const WayManager &way_manager, RouteMatchingParams &matching_params)
        : way_manager_(way_manager), matching_params_(matching_params)
    {
        Init();
    }

    void Init()
    {
        // input parameters from matching_params => matching_points
        MatchingPointsCopyFromParams(matching_params_, matching_points_);

        // div by 4.0 instead of 2.0, as we also check the distance to the mid-points
        // of long segments
        DFT_POINT_DIST_THRESHOLD = ((MAX_SEGMENT_LEN / 4.0) * (MAX_SEGMENT_LEN / 4.0))
            + matching_params_.radius * matching_params_.radius;
        DFT_POINT_DIST_THRESHOLD = std::sqrt(DFT_POINT_DIST_THRESHOLD)
            + 20; // added some margin
    }

    void FindMatchedRoute()
    {
        DoMultiPointsSimpleMatching();
        PointsIntoGroups();

        matching_params_.result_route.clear();
        // a guessed number based on observation, should be OK for most
        matching_params_.result_route.reserve(256);

        // find route for each group
        for (const auto &group : groups_) {
            if (group.from_index >= group.to_index) { // invalid case
                continue;
            }
            AppendResultRouteForGroup(group);
        }

        FixBeginEndPoints();
        FixOutputsInResultRoute();

        if (matching_params_.verfy_result) {
            VerifyResult(matching_params_);
        }
    }

    static bool ResultToJson(const WayManager &way_manager,
        const RouteMatchingParams &matching_params,
        const std::string &json_pathname)
    {
#if 0
        {
            // for the testing data generation
            std::string data;
            data.reserve(matching_params.via_points.size() * 130);
            for (auto &point : matching_params.via_points) {
                char buff[128];
                snprintf(buff, sizeof(buff),
                    "params.via_points.push_back(RouteMatchingViaPoint(%.6lf, %.6lf, %d, %d, util::StrToTimeT(\"%s\")));\n",
                    point.geo_point.lat, point.geo_point.lng, point.heading, point.speed,
                    util::TimeTToStr(point.record_time).c_str());
                data += buff;
            }
            util::WriteStrToFile(json_pathname + ".data", data);
        }
#endif

        geo::GeoJSON geo_json;
        if (!SegsToJson(way_manager, matching_params.result_route, 4.0, "blue", "black",
            geo_json)) {
            return false;
        }

        auto trace_color = "green";
        std::shared_ptr<GeoObj_LineString> p_line = std::make_shared<GeoObj_LineString>();
        for (auto &via_point : matching_params.via_points) {
            p_line->AddPoint(via_point.geo_point);
        }
        p_line->AddProp("disconnected_index", matching_params.disconnected_index);
        p_line->AddProp("repeated_index", matching_params.repeated_index);
        p_line->AddProp("color", trace_color);

        geo_json.AddObj(p_line);

        int index = 0;
        for (auto &point : matching_params.via_points) {
            std::shared_ptr<GeoObj_Point> p_point = std::make_shared<GeoObj_Point>(
                point.geo_point);
            p_point->AddProp("index", index);
            p_point->AddProp("heading", point.heading);
            p_point->AddProp("speed", point.speed);
            p_point->AddProp("record_time", util::TimeTToStr(point.record_time));
            p_point->AddProp("relative_time(sec)",
                (long long)(point.record_time - matching_params.via_points[0].record_time));
            p_point->AddProp("seg_id", (point.p_seg ? point.p_seg->seg_id_ : 0));
            p_point->AddProp("broken", point.is_broken);
            p_point->AddProp("entering_no_gps", point.entering_no_gps_route);
            p_point->AddProp("color", trace_color);
            geo_json.AddObj(p_point);

            // for the direction
            if (point.heading >= 0) {
                auto p_line = std::make_shared<GeoObj_LineString>();
                p_line->AddPoint(point.geo_point);
                geo::GeoPoint to = geo::get_point_degree(point.geo_point, 12.0, point.heading);
                p_line->AddPoint(to);
                p_line->AddProp("index", index);
                p_line->AddProp("color", trace_color);
                geo_json.AddObj(p_line);
            }

            index++;
        }

        return geo_json.ToJsonFile(json_pathname);
    }

private:
    std::array<RoutingPair, MAX_CANDIATES_COUNT * MAX_CANDIATES_COUNT> routing_pairs_;
    size_t routing_pair_count_{};
    void AppendResultRouteForGroup(const MatchingPointGroup &group)
    {
        MatchingPoint &from_pt = matching_points_[group.from_index];
        MatchingPoint &to_pt = matching_points_[group.to_index];
        if (from_pt.NonMatched() || to_pt.NonMatched()) {
            return;
        }

        routing_pair_count_ = 0;
        for (const auto& p_from_seg : from_pt.p_segs) {
            if (p_from_seg != nullptr) {
                for (const auto& p_to_seg : to_pt.p_segs) {
                    if (p_to_seg != nullptr) {
                        routing_pairs_[routing_pair_count_++].Init(&from_pt, &to_pt,
                            p_from_seg, p_to_seg,nullptr, nullptr);
                    }
                }
            }
        }
        if (routing_pair_count_ == 0) {
            return;
        }

        for (size_t i = 0; i < routing_pair_count_; ++i) {
            auto &pair = routing_pairs_[i];
            BestAlignedRoute(pair, group.from_index, group.to_index, temp_via_points_);
            CalculateRouteWeight(pair);
        }
        // sort by route by weight
        if (routing_pair_count_ > 1) {
            std::sort(routing_pairs_.begin(), routing_pairs_.begin() + routing_pair_count_,
                [](const RoutingPair &i, const RoutingPair &j) {
                return i.route_weight < j.route_weight;
            });
        }

        // if no good route found, try another method to find it
        if (routing_pairs_.front().route_weight == INVALID_WEIGHT) {
            if (group.from_index + 1 < group.to_index) {
                FindRoutePointByPoint(g_, routing_pairs_, routing_pair_count_,
                    group.from_index, group.to_index);
                if (routing_pair_count_ > 1) {
                    std::sort(routing_pairs_.begin(), routing_pairs_.begin() + routing_pair_count_,
                        [](const RoutingPair &i, const RoutingPair &j) {
                        return i.route_weight < j.route_weight;
                    });
                }
            }
        }

        const auto &the_pair = routing_pairs_.front();
        auto &from_point = matching_params_.via_points[group.from_index];
        auto &to_point = matching_params_.via_points[group.to_index];
        from_point.is_broken = (the_pair.route_weight == INVALID_WEIGHT);
        if (!from_point.is_broken) {
            from_point.p_seg = the_pair.p_seg_from;
            to_point.p_seg = the_pair.p_seg_to;

            int last_size = (int)the_pair.seg_route.size();
            for (const auto &p_seg : the_pair.seg_route) {
                AppendSegRoute(matching_params_.result_route, p_seg);
            }

            // for RouteMatchingViaPoint::i_seg
            for (int i = ((last_size > 0) ? (last_size - 1) : 0);
                i < (int)matching_params_.result_route.size();
                ++i) {
                if (from_point.p_seg == matching_params_.result_route[i]) {
                    from_point.i_seg = i;
                }
                if (to_point.p_seg == matching_params_.result_route[i]) {
                    to_point.i_seg = i;
                }
            }
        }
    }

    void FixBeginEndPoints()
    {
        if (matching_params_.via_points.empty() || matching_params_.result_route.empty()) {
            return;
        }
        FixBeginPoints();
        FixEndPoints();
    }

    void FixBeginPoints()
    {
        auto seg_in_route_front = [](const vector<geo::SegmentPtr>& route, geo::SegmentPtr p_seg) {
            for (size_t i = 0; i < 4 && i < route.size(); ++i) {
                if (route[i] == p_seg) return true;
            }
            return false;
        };

        int seg_cand_count0 = 0;
        for (auto& p_seg : matching_points_.front().p_segs) {
            if (p_seg) {
                ++seg_cand_count0;
            }
        }
        if (seg_cand_count0 == 2) {
            // e.g., if the route is starting from seg2, but should start from seg1
            //      seg1     seg2
            // ----------->------------->.....
            //         o
            //      1st via point
            const auto &mat_point0 = matching_points_.front();
            SegmentPtr p_seg1 = mat_point0.p_segs[0];
            SegmentPtr p_seg2 = mat_point0.p_segs[1];
            if (p_seg1 == matching_params_.result_route.front()) {
                std::swap(p_seg1, p_seg2);
            }

            const GeoPoint& geo_point = mat_point0.p_via_point->geo_point;
            if (p_seg2 == matching_params_.result_route.front() &&
                p_seg1->DistanceSquareMeters(geo_point) < p_seg2->DistanceSquareMeters(geo_point))
            {
                if (p_seg1->GetWayIdOriented() == p_seg2->GetWayIdOriented() || p_seg1->to_nd_ == p_seg2->from_nd_) {
                    if (!seg_in_route_front(matching_params_.result_route, p_seg1)) {
                        matching_params_.result_route.insert(matching_params_.result_route.begin(), p_seg1);
                        mat_point0.p_via_point->p_seg = p_seg1;
                    }
                }
                else {
                    vector<SegmentPtr> route;
                    way_manager_.RoutingNearby(p_seg1, p_seg2, route);
                    if (route.size() == 3) {
                        matching_params_.result_route.insert(matching_params_.result_route.begin(),
                            route.begin(), route.begin() + 2);
                        mat_point0.p_via_point->p_seg = route.front();
                    }
                }
            }
        }
        else if (seg_cand_count0 == 3) {
            //   seg1/seg2     seg3
            // ------------->------------->.....
            //            o
            //      1st via point
            const auto &mat_point0 = matching_points_.front();
            SegmentPtr p_seg1 = mat_point0.p_segs[0];
            SegmentPtr p_seg2 = mat_point0.p_segs[1];
            SegmentPtr p_seg3 = mat_point0.p_segs[2];
            if (p_seg1 == matching_params_.result_route.front()) {
                std::swap(p_seg1, p_seg3);
            }
            else if (p_seg2 == matching_params_.result_route.front()) {
                std::swap(p_seg2, p_seg3);
            }

            if (p_seg3 == matching_params_.result_route.front()) {
                bool ok1 = false, ok2 = false;
                if (p_seg1->GetWayIdOriented() == p_seg3->GetWayIdOriented() || p_seg1->to_nd_ == p_seg3->from_nd_) {
                    ok1 = true;
                }
                if (p_seg2->GetWayIdOriented() == p_seg3->GetWayIdOriented() || p_seg2->to_nd_ == p_seg3->from_nd_) {
                    ok2 = true;
                }
                if (ok1 || ok2) {
                    const double dist1 = p_seg1->DistanceSquareMeters(mat_point0.p_via_point->geo_point);
                    const double dist2 = p_seg2->DistanceSquareMeters(mat_point0.p_via_point->geo_point);
                    const double dist3 = p_seg3->DistanceSquareMeters(mat_point0.p_via_point->geo_point);
                    if (ok1 && !ok2 && dist1 < dist3) {
                        if (!seg_in_route_front(matching_params_.result_route, p_seg1)) {
                            matching_params_.result_route.insert(matching_params_.result_route.begin(), p_seg1);
                            mat_point0.p_via_point->p_seg = p_seg1;
                        }
                    }
                    else if (!ok1 && ok2 && dist2 < dist3) {
                        if (!seg_in_route_front(matching_params_.result_route, p_seg2)) {
                            matching_params_.result_route.insert(matching_params_.result_route.begin(), p_seg2);
                            mat_point0.p_via_point->p_seg = p_seg2;
                        }
                    }
                    else if (ok1 && ok2 && dist1 < dist3 && dist2 < dist3) {
                        auto p_seg_new = (dist1 < dist2) ? p_seg1 : p_seg2;
                        if (!seg_in_route_front(matching_params_.result_route, p_seg_new)) {
                            matching_params_.result_route.insert(matching_params_.result_route.begin(), p_seg_new);
                            mat_point0.p_via_point->p_seg = p_seg_new;
                        }
                    }
                }
            }
        }
    }

    void FixEndPoints()
    {
        auto seg_in_route_back = [](const vector<geo::SegmentPtr>& route, geo::SegmentPtr p_seg) {
            for (size_t i = 0; i < 4 && i < route.size(); ++i) {
                if (route[route.size() - 1 - i] == p_seg) return true;
            }
            return false;
        };

        int seg_cand_count_n = 0;
        for (auto& p_seg : matching_points_.back().p_segs) {
            if (p_seg) {
                ++seg_cand_count_n;
            }
        }
        if (seg_cand_count_n == 2) {
            // e.g., if the route is ending with seg1, but should ending with seg2
            //                    seg1     seg2
            // ... --------->----------->------------->
            //                              o
            //                         last via point
            const auto &mat_point_n = matching_points_.back();
            SegmentPtr p_seg1 = mat_point_n.p_segs[0];
            SegmentPtr p_seg2 = mat_point_n.p_segs[1];
            if (p_seg2 == matching_params_.result_route.back()) {
                std::swap(p_seg1, p_seg2);
            }

            const GeoPoint& geo_point = mat_point_n.p_via_point->geo_point;
            if (p_seg1 == matching_params_.result_route.back() &&
                p_seg2->DistanceSquareMeters(geo_point) < p_seg1->DistanceSquareMeters(geo_point))
            {
                if (p_seg1->GetWayIdOriented() == p_seg2->GetWayIdOriented() || p_seg1->to_nd_ == p_seg2->from_nd_) {
                    if (!seg_in_route_back(matching_params_.result_route, p_seg2)) {
                        matching_params_.result_route.push_back(p_seg2);
                        mat_point_n.p_via_point->p_seg = p_seg2;
                    }
                }
                else {
                    vector<SegmentPtr> route;
                    way_manager_.RoutingNearby(p_seg1, p_seg2, route);
                    if (route.size() == 3) {
                        if (!seg_in_route_back(matching_params_.result_route, route[1])) {
                            matching_params_.result_route.push_back(route[1]);
                            mat_point_n.p_via_point->p_seg = route[1];
                        }
                        if (!seg_in_route_back(matching_params_.result_route, p_seg2)) {
                            matching_params_.result_route.push_back(p_seg2);
                            mat_point_n.p_via_point->p_seg = p_seg2;
                        }
                    }
                }
            }
        }
        else if (seg_cand_count_n == 3) {
            //                   seg3     seg1/seg2
            // ... --------->----------->------------->
            //                              o
            //                         last via point
            const auto &mat_point_n = matching_points_.back();
            SegmentPtr p_seg1 = mat_point_n.p_segs[0];
            SegmentPtr p_seg2 = mat_point_n.p_segs[1];
            SegmentPtr p_seg3 = mat_point_n.p_segs[2];
            if (p_seg1 == matching_params_.result_route.back()) {
                std::swap(p_seg1, p_seg3);
            }
            else if (p_seg2 == matching_params_.result_route.back()) {
                std::swap(p_seg2, p_seg3);
            }

            if (p_seg3 == matching_params_.result_route.back()) {
                bool ok1 = false, ok2 = false;
                if (p_seg1->GetWayIdOriented() == p_seg3->GetWayIdOriented() || p_seg1->from_nd_ == p_seg3->to_nd_) {
                    ok1 = true;
                }
                if (p_seg2->GetWayIdOriented() == p_seg3->GetWayIdOriented() || p_seg2->from_nd_ == p_seg3->to_nd_) {
                    ok2 = true;
                }
                if (ok1 || ok2) {
                    const double dist1 = p_seg1->DistanceSquareMeters(mat_point_n.p_via_point->geo_point);
                    const double dist2 = p_seg2->DistanceSquareMeters(mat_point_n.p_via_point->geo_point);
                    const double dist3 = p_seg3->DistanceSquareMeters(mat_point_n.p_via_point->geo_point);
                    if (ok1 && !ok2 && dist1 < dist3) {
                        if (!seg_in_route_back(matching_params_.result_route, p_seg1)) {
                            matching_params_.result_route.push_back(p_seg1);
                            mat_point_n.p_via_point->p_seg = p_seg1;
                        }
                    }
                    else if (!ok1 && ok2 && dist2 < dist3) {
                        if (!seg_in_route_back(matching_params_.result_route, p_seg2)) {
                            matching_params_.result_route.push_back(p_seg2);
                            mat_point_n.p_via_point->p_seg = p_seg2;
                        }
                    }
                    else if (ok1 && ok2 && dist1 < dist3 && dist2 < dist3) {
                        auto p_seg_new = (dist1 < dist2) ? p_seg1 : p_seg2;
                        if (!seg_in_route_back(matching_params_.result_route, p_seg_new)) {
                            matching_params_.result_route.push_back(p_seg_new);
                            mat_point_n.p_via_point->p_seg = p_seg_new;
                        }
                    }
                }
            }
        }
    }

    // for the missing RouteMatchingViaPoint::i_seg, etc.
    void FixOutputsInResultRoute()
    {
        const int result_seg_count = (int)matching_params_.result_route.size();
        const int point_size = (int)matching_params_.via_points.size();

        for (int i_point = 0; i_point < point_size; ++i_point) {
            auto &via_point = matching_params_.via_points[i_point];
            if (via_point.i_seg >= 0) {
                continue; // no need
            }
            auto &mat_point = matching_points_[i_point];

            if ((via_point.p_seg == nullptr) && mat_point.HasMatched()) {
                for (int i = 0; i < result_seg_count; ++i) {
                    for (auto& p_candiate_seg : mat_point.p_segs) {
                        if (p_candiate_seg && p_candiate_seg == matching_params_.result_route[i]) {
                            via_point.p_seg = p_candiate_seg;
                            via_point.i_seg = i;
                            break;
                        }
                    }
                }
            }
        }

        for (auto &via_point : matching_params_.via_points) {
            // try last time for RouteMatchingViaPoint::i_seg
            if (via_point.i_seg < 0 && via_point.p_seg != nullptr) {
                for (int i = 0; i < result_seg_count; ++i) {
                    if (via_point.p_seg == matching_params_.result_route[i]) {
                        via_point.i_seg = i;
                        break;
                    }
                }
            }
        }

        // for is_broken flag
        bool is_broken = matching_params_.via_points.front().is_broken =
            (matching_params_.via_points.front().i_seg < 0);
        const int point_count = (int)matching_params_.via_points.size();
        for (int i = 1; i < point_count; ++i) {
            auto &via_point = matching_params_.via_points[i];
            if (via_point.is_broken) { // if already set as broken
                is_broken = true;
            }
            else if (via_point.i_seg < 0) { // not set as broken, just copy the flag from the previous one
                via_point.is_broken = is_broken;
            }
        }

        // for entering_no_gps_route flag
        for (int i = 1; i < point_count - 1; ++i) {
            auto &via_point1 = matching_params_.via_points[i];
            if (via_point1.p_seg == nullptr || via_point1.is_broken) {
                continue;
            }
            int i2 = i + 1;
            for (; i2 < point_count; ++i2) {
                if (via_point1.p_seg) {
                    break;
                }
            }
            if (i2 >= point_count) {
                break;
            }
            auto &via_point2 = matching_params_.via_points[i2];
            i = i2 - 1;

            for (int k = via_point1.i_seg + 1; k < via_point2.i_seg; ++k) {
                const SegmentPtr &p_seg = matching_params_.result_route[k];
                if (p_seg->excluded_no_gps_) {
                    via_point1.entering_no_gps_route = true;
                    break;
                }
            }
        }
    }

    static void AppendSegRoute(std::vector<SegmentPtr> &seg_route, const SegmentPtr &p_seg)
    {
        if (seg_route.empty()) {
            seg_route.push_back(p_seg);
        }
        else if (seg_route.back() != p_seg) {
            seg_route.push_back(p_seg);
        }
    }

    // find the shortest route aligned with the points trace
    bool BestAlignedRoute(RoutingPair &pair, int from_index, int to_index,
        std::vector<ViaPoint> &trace) const
    {
        bool ok = false;

        if (to_index - from_index > 1) {
            double direct_dist = geo::distance_in_meter(
                matching_params_.via_points.front().geo_point,
                matching_params_.via_points.back().geo_point);
            if (direct_dist < 1500) {
                double distance; // for similarity distance
                trace.clear();
                if (trace.capacity() < size_t(to_index - from_index + 1)) {
                    trace.reserve((to_index - from_index + 1) * 2);
                }
                for (int i = from_index; i <= to_index; ++i) {
                    const auto& via_pt = matching_params_.via_points[i];
                    trace.push_back(ViaPoint(via_pt.geo_point, via_pt.heading));
                }
                ok = way_manager_.SimilarRoutingNearby(pair.p_seg_from, pair.p_seg_to,
                    trace, pair.seg_route, distance);
                if (ok && matching_params_.via_points[from_index].record_time) {
                    if (way_manager_.AreSegmentsExcluded(pair.seg_route,
                        matching_params_.via_points[from_index].record_time,
                        matching_params_.is_localtime))
                    {
                        pair.seg_route.clear();
                        return false;
                    }
                }
            }
        }

        if (!ok) {
            const auto &from_point = matching_points_[from_index];
            ok = way_manager_.ShortestPath(pair.p_seg_from, pair.p_seg_to, pair.seg_route,
                false, from_point.p_via_point->record_time);
            if (!ok) {
                pair.seg_route.clear();
                return false;
            }
        }

        // check if all the via points are close to the route
        bool aligned = IsRouteAlignedWithPointsTrace(from_index, to_index, pair.seg_route);
        if (!aligned) {
            pair.seg_route.clear();
            return false;
        }

        return !pair.seg_route.empty();
    }

    bool IsRouteAlignedWithPointsTrace(int from_index, int to_index,
        const vector<SegmentPtr> &seg_route) const
    {
        if (seg_route.empty()) {
            return false;
        }

        double DISTANCE_LIMIT = DFT_POINT_DIST_THRESHOLD;
        if (matching_params_.distance_limit > 0 &&
            matching_params_.distance_limit < DFT_POINT_DIST_THRESHOLD) {
            DISTANCE_LIMIT = matching_params_.distance_limit;
        }

        // check if all the via points are close to the route
        for (int i = from_index; i <= to_index; ++i) {
            const auto &via_point = matching_params_.via_points[i];
            double point_to_route = PointToRouteDist(via_point.geo_point, seg_route);
            if (point_to_route > DISTANCE_LIMIT) {
                return false;
            }
        }
        return true;
    }

    // calculate the weight (length considering way type)
    // the weight includes the 1st segment, excluding the last one
    int CalculateRouteWeight(const std::vector<SegmentPtr> &seg_route) const
    {
        const int seg_count = (int)seg_route.size();
        int route_weight = 0;
        for (int i = 0; i < seg_count - 1; ++i) {
            auto &p_seg = seg_route[i];
            route_weight += WayManager::DistanceToWeight(p_seg->length_, p_seg->way_type_);
        }
        return route_weight;
    }

    void CalculateRouteWeight(RoutingPair &pair) const
    {
        if (pair.seg_route.empty()) {
            pair.route_weight = INVALID_WEIGHT;
            return;
        }

        const int seg_count = (int)pair.seg_route.size();
        pair.route_weight = 0;

        // for 1st segment
        {
            auto p_seg = pair.seg_route.front();
            double proj_len = geo::get_projection_distance_in_meter(pair.p_from_mat_pt->p_via_point->geo_point,
                p_seg->from_point_, p_seg->to_point_, false);
            pair.route_weight += WayManager::DistanceToWeight(proj_len, p_seg->way_type_);
        }

        // for last segment
        {
            auto p_seg = pair.seg_route.back();
            double proj_len = geo::get_projection_distance_in_meter(pair.p_to_mat_pt->p_via_point->geo_point,
                p_seg->from_point_, p_seg->to_point_, true);
            pair.route_weight += WayManager::DistanceToWeight(proj_len, p_seg->way_type_);
        }

        // by default, exclude 1st and last
        for (int i = 1; i < seg_count - 1; ++i) {
            auto &p_seg = pair.seg_route[i];
            pair.route_weight += WayManager::DistanceToWeight(p_seg->length_, p_seg->way_type_);
        }
    }

    void VerifyResult(RouteMatchingParams &params) const
    {
        int count = (int)params.result_route.size();
        if (!params.verfy_result || count == 0) return;

        // verify if disconnected
        params.disconnected_index = -1;
        for (int i = 0; i < count - 1; ++i) {
            const auto &p_seg1 = params.result_route[i];
            const auto &p_seg2 = params.result_route[i + 1];
            if (p_seg1->to_nd_ != p_seg2->from_nd_) {
                params.disconnected_index = i;
                break;
            }
        }

        params.repeated_index = -1;
        if (count <= 128) {
            for (int i = 0; i < count - 1; ++i) {
                const auto &p_seg1 = params.result_route[i];
                for (int j = i + 1; j < count; ++j) {
                    const auto &p_seg2 = params.result_route[j];
                    if (p_seg1 == p_seg2) {
                        params.repeated_index = i;
                        break;
                    }
                }
                if (params.repeated_index >= 0) {
                    break;
                }
            }
        }
        else {
            temp_segs_ = params.result_route;
            std::sort(temp_segs_.begin(), temp_segs_.end());
            SegmentPtr p_seg{};
            for (int i = 0; i < count - 1; ++i) {
                if (temp_segs_[i] == temp_segs_[i + 1]) {
                    p_seg = temp_segs_[i];
                    break;
                }
            }
            if (p_seg) {
                for (int i = 0; i < count; ++i) {
                    if (params.result_route[i] == p_seg) {
                        params.repeated_index = i;
                        break;
                    }
                }
            }
        }
    }

private:
    // complex way to find the route, check all the passing points
    bool FindRoutePointByPoint(Graph& g,
        std::array<RoutingPair, MAX_CANDIATES_COUNT * MAX_CANDIATES_COUNT> &pairs,
        size_t& pair_count,
        int from_index, int to_index) const
    {
        BuildGraph(from_index, to_index, g);
        if (g.all_g_nodes.empty()) {
            return false;
        }

        // re-populate pairs[] with graph nodes information
        const MatchingPoint &from = matching_points_[from_index];
        const MatchingPoint &to = matching_points_[to_index];

        pair_count = 0;
        for (size_t i_from = 0; i_from < from.p_gnodes.size(); ++i_from) {
            auto& p_from_gnode = from.p_gnodes[i_from];
            if (p_from_gnode) {
                for (size_t i_to = 0; i_to < to.p_gnodes.size(); ++i_to) {
                    auto& p_to_gnode = to.p_gnodes[i_to];
                    if (p_to_gnode) {
                        pairs[pair_count++].Init(const_cast<MatchingPoint *>(&from),
                            const_cast<MatchingPoint *>(&to),
                            from.p_segs[i_from], to.p_segs[i_to],
                            p_from_gnode, p_to_gnode);
                    }
                }
            }
        }

        for (size_t i = 0; i < pair_count; ++i) {
            auto &pair = pairs[i];
            RunDijkstra(g, pair.p_gnode_from, pair.p_gnode_to, pair.seg_route);
            CalculateRouteWeight(pair);
            if (pair.route_weight != INVALID_WEIGHT) {
                if (false == IsRouteAlignedWithPointsTrace(from_index, to_index, pair.seg_route)) {
                    pair.route_weight = INVALID_WEIGHT;
                    pair.seg_route.clear();
                }
            }
        }

        return true;
    }

    bool RunDijkstra(const Graph &g, const GraphNodePtr g_node1, const GraphNodePtr g_node2,
        vector<GraphNodePtr> &g_node_path) const
    {
        const int max_node_count = (int)g.all_g_nodes.size();

        BinHeap fwd_heap(max_node_count, g.g_node_pool);
        fwd_heap.Insert(g_node1, 0, 0);

        bool success = false;
        vector<tuple<NodeDataPtr, int>> pairs;;

        while (!fwd_heap.Empty()) {
            NodeDataPtr p_min_node = fwd_heap.DeleteMin();
            NodeData& min_node = *p_min_node;

            min_node.finished = true;
            if (min_node.p_g_node_ == g_node2) {
                success = true;
                break;
            }

            pairs.clear();
            for (auto &p_g_edge : min_node.p_g_node_->edges_) {
                const auto& p_to_g_node = p_g_edge->p_to_g_node_;

                NodeDataPtr p_to_node_data = fwd_heap.GetIfInserted(p_to_g_node);
                if (p_to_node_data != nullptr && p_to_node_data->finished) {
                    continue;
                }

                // make sure the "to" node is already in working queue
                // do not do insertion if was inserted before
                auto e_weight = p_g_edge->distance_;
                if (p_to_node_data == nullptr) {
                    p_to_node_data = fwd_heap.Insert(p_to_g_node, min_node.p_g_node_,
                        min_node.distance + e_weight);
                }

                // get all the keys to be decreased later
                if (min_node.distance + e_weight < p_to_node_data->distance) {
                    pairs.emplace_back(make_tuple(p_to_node_data, min_node.distance + e_weight));
                    // also update the parent node
                    p_to_node_data->p_pre_g_node_ = min_node.p_g_node_;
                }
            }

            if (!pairs.empty()) {
                fwd_heap.DecreaseKeys(pairs);
            }
        }

        if (success) {
            // get the path
            g_node_path.clear();
            GraphNodePtr p_gn = g_node2;
            while (p_gn != nullptr) {
                g_node_path.emplace_back(p_gn);
                p_gn = fwd_heap.GetPreNode(p_gn);
            }

            // reverse the order
            const size_t gn_path_size = g_node_path.size();
            for (size_t i = 0; i < gn_path_size / 2; ++i) {
                std::swap(g_node_path[i], g_node_path[gn_path_size - 1 - i]);
            }
        }

        return success;
    }

    // assumption: g_node1 != g_node2
    bool RunDijkstra(const Graph &g, const GraphNodePtr g_node1, const GraphNodePtr g_node2,
        vector<SegmentPtr> &seg_route) const
    {
        vector<GraphNodePtr> gnode_path;
        seg_route.clear();
        bool ok = RunDijkstra(g, g_node1, g_node2, gnode_path);
        if (!ok) return false;

        int gnode_count = (int)gnode_path.size();
        for (int i = 0; i < gnode_count - 1; ++i) {
            const GraphNodePtr &g_nd1 = gnode_path[i];
            const GraphNodePtr &g_nd2 = gnode_path[i + 1];

            for (auto &e : g_nd1->edges_) {
                if (e->p_to_g_node_ == g_nd2) {
                    for (auto &p_seg : e->segs_) {
                        AppendSegRoute(seg_route, p_seg);
                    }
                    break;
                }
            }
        }

        return !seg_route.empty();
    }

    // note: method is declared as "const", but we still changed matching_points_[i].p_gnode1
    //       matching_points_[i].p_gnode2, ...
    void BuildGraph(int from_index, int to_index, Graph &g) const
    {
        g.Clear();

        size_t max_node = 0;
        for (int i = from_index; i <= to_index; ++i) {
            for (auto& p_seg : matching_points_[i].p_segs) {
                if (p_seg) {
                    ++max_node;
                }
            }
        }
        if (max_node == 0) {
            return;
        }

        g.g_node_pool.Reserve(max_node);
        g.all_g_nodes.reserve(max_node);

        size_t edge_reserve_size = (to_index - from_index + 1) *
            (MAX_CANDIATES_COUNT * MAX_CANDIATES_COUNT);
        g.g_edge_pool.Reserve(edge_reserve_size);
        g.all_g_edges.reserve(edge_reserve_size);

        // build g_nodes
        for (int i = from_index; i <= to_index; ++i) {
            auto &mat_point = matching_points_[i];
            for (size_t j = 0; j < mat_point.p_segs.size(); ++j) {
                auto& p_seg = mat_point.p_segs[j];
                if (p_seg) {
                    auto p_new_g_node = g.g_node_pool.AllocNew(GraphNode(p_seg));
                    g.all_g_nodes.push_back(p_new_g_node);
                    (GraphNodePtr&)mat_point.p_gnodes[j] = p_new_g_node;
                }
            }
        }

        // build g_edges
        std::vector<SegmentPtr> &seg_route = temp_segs_;
        seg_route.reserve(128);
        for (int i = from_index, i2; i < to_index; i = i2) {
            auto new_g_edge = [this, &g, &seg_route](size_t i_candidate1, size_t i_candidate2,
                const MatchingPoint &mat_point1, const MatchingPoint &mat_point2)
            {
                bool ok = way_manager_.ShortestPath(
                    mat_point1.p_segs[i_candidate1], mat_point2.p_segs[i_candidate2], seg_route,
                    false, mat_point1.p_via_point->record_time);
                if (ok) {
                    GraphNodePtr p_from_g_node = mat_point1.p_gnodes[i_candidate1];
                    GraphNodePtr p_to_g_node = mat_point2.p_gnodes[i_candidate2];;
                    if (!p_from_g_node || !p_to_g_node) {
                        return false;
                    }

                    int distance = CalculateRouteWeight(seg_route);
                    auto p_new_g_edge = g.g_edge_pool.AllocNew(
                        GraphEdge(p_from_g_node, p_to_g_node, distance));
                    p_new_g_edge->segs_ = seg_route;
                    g.all_g_edges.push_back(p_new_g_edge);
                    p_from_g_node->edges_.push_back(p_new_g_edge);
                }
                return ok;
            };

            // make sure point2 has at least one matched segment
            i2 = i + 1;
            while (i2 < to_index && matching_points_[i2].NonMatched()) {
                ++i2;
            }
            if (i2 > to_index) {
                break;
            }
            auto &mat_point1 = matching_points_[i];
            auto &mat_point2 = matching_points_[i2];
            int ok_count = 0;

            for (size_t i_candidate1 = 0;
                i_candidate1 < mat_point1.p_segs.size();
                ++i_candidate1)
            {
                if (!mat_point1.p_segs[i_candidate1]) continue;
                for (size_t i_candidate2 = 0;
                    i_candidate2 < mat_point2.p_segs.size();
                    ++i_candidate2)
                {
                    if (!mat_point2.p_segs[i_candidate2]) continue;
                    bool ok = new_g_edge(i_candidate1, i_candidate2, mat_point1, mat_point2);
                    if (ok) ++ok_count;
                }
            }

            if (ok_count == 0) { // try skip
                if (i < to_index - 1) {
                    // make sure point3 has at least one matched segment
                    ++i2;
                    while (i2 < to_index && matching_points_[i2].NonMatched()) {
                        ++i2;
                    }
                    if (i2 > to_index) {
                        break;
                    }
                    auto &mat_point3 = matching_points_[i2];

                    for (size_t i_candidate1 = 0;
                        i_candidate1 < mat_point1.p_segs.size();
                        ++i_candidate1)
                    {
                        if (!mat_point1.p_segs[i_candidate1]) continue;
                        for (size_t i_candidate3 = 0;
                            i_candidate3 < mat_point3.p_segs.size();
                            ++i_candidate3)
                        {
                            if (!mat_point3.p_segs[i_candidate3]) continue;
                            bool ok = new_g_edge(i_candidate1, i_candidate3,
                                mat_point1, mat_point3);
                            if (ok) ++ok_count;
                        }
                    }

                    ++i;
                }
            }

            // still no route found, give up
            if (ok_count == 0) {
                g.all_g_nodes.clear();
                g.all_g_edges.clear();
                return;
            }
        }
    }

    // return the min distance from point to route
    double PointToRouteDist(const geo::GeoPoint& geo_point,
        const std::vector<SegmentPtr> &seg_route) const
    {
        double min_dist = std::numeric_limits<double>::max();
        for (auto &p_seg : seg_route) {
            double dist = geo::distance_in_meter(p_seg->from_point_, geo_point);
            if (dist < MAX_SEGMENT_LEN * 2) {
                dist = geo::distance_point_to_segment(geo_point, p_seg->from_point_,
                    p_seg->to_point_);
            }
            if (dist < min_dist) {
                min_dist = dist;
            }
        }
        return min_dist;
    }

    static void MatchingPointsCopyFromParams(const RouteMatchingParams &params,
        std::vector<MatchingPoint> &matching_points)
    {
        matching_points.clear();
        matching_points.resize(params.via_points.size());
        size_t i = 0;
        for (const auto & via_pt : params.via_points) {
            auto &mat_pt = matching_points[i++];
            mat_pt.p_via_point = const_cast<RouteMatchingViaPoint *>(&via_pt);
        }
    }

    // point-segment matching, can have multiple results for each point
    void DoMultiPointsSimpleMatching()
    {
        SegAssignParams assign_params;
        SegAssignResults assign_results;
        assign_params.radius = matching_params_.radius;
        assign_params.angle_tollerance = matching_params_.angle_tollerance;
        assign_params.check_no_gps_route = matching_params_.check_no_gps_route;

        for (auto &mat_pt : matching_points_) {
            mat_pt.p_segs.fill(nullptr);

            if (mat_pt.p_via_point->p_seg) {
                // if already has exclusive matched segment
                mat_pt.p_segs[0] = mat_pt.p_via_point->p_seg;
            }
            else {
                // if assignments are not provided, do it here
                assign_params.heading = mat_pt.p_via_point->heading;
                assign_params.dev_data_time = mat_pt.p_via_point->record_time;

                if (nullptr != way_manager_.AssignSegment(mat_pt.p_via_point->geo_point,
                    assign_params, &assign_results)) {
                    size_t res_size = assign_results.size();
                    for (size_t i = 0; i < mat_pt.p_segs.size() && i < res_size; ++i) {
                        mat_pt.p_segs[i] = assign_results[i].p_seg;
                    }
                }
            }
        }

        // remove some suspicious points' simple matching results
        const int point_count = (int)matching_params_.via_points.size();
        for (int i = 1; i < point_count - 1; ++i) {
            if (matching_points_[i].NonMatched()) {
                continue;
            }

            const auto &prev = matching_params_.via_points[i - 1];
            const auto &cur = matching_params_.via_points[i];
            const auto &next = matching_params_.via_points[i + 1];
            if (prev.heading < 0 || cur.heading < 0 || next.heading < 0) {
                continue;
            }

            // if the current heading is not consistent with  the previous and next headings
            if (WayManager::GetAngle(prev.heading, next.heading) < 40) {
                if ((prev.record_time && next.record_time &&
                    (next.record_time - prev.record_time <= 180))
                    ||
                    (prev.record_time == 0) || (next.record_time == 0)) {
                    int avg_angle = ((prev.heading + next.heading) / 2) % 360;
                    if (WayManager::GetAngle(avg_angle, cur.heading) > 80) {
                        // ok, this point's heading is suspicious, ignore it
                        matching_points_[i].SetToNonMatched();
                    }
                }
            }
        }
    }

    // each group tends to begin and end with points of exclusive segment
    void PointsIntoGroups()
    {
        groups_.clear();
        const int points_size = (int)matching_points_.size();

        MatchingPointGroup group;
        group.group_index = 0;
        group.from_index = 0;

        for (int i = 1; i < points_size - 1; ++i) {
            if (matching_points_[i].IsExclusiveMatched()) {
                group.to_index = i;
                groups_.push_back(group);

                // for the next group
                group.group_index = groups_.back().group_index + 1;
                group.from_index = i;
            }
        }

        group.to_index = points_size - 1;
        groups_.push_back(group);

        // make sure no non-matched points at begin/end
        for (auto &group : groups_) {
            for (int i = group.from_index; i < group.to_index; ++i) {
                if (matching_points_[i].HasMatched()) {
                    break;
                }
                ++group.from_index;
            }
            for (int i = group.to_index; i > group.from_index; --i) {
                if (matching_points_[i].HasMatched()) {
                    break;
                }
                --group.to_index;
            }
        }
    }

    static bool SegsToJson(const WayManager &way_manager, const std::vector<SegmentPtr>& segs,
        double offset, const std::string& seg_color, const std::string& node_color,
        geo::GeoJSON &geo_json)
    {
        std::set<NODE_ID_T> nodes_set;

        int index = 0;
        for (const auto& p_seg : segs) {
            std::shared_ptr<GeoObj_LineString> p_line = std::make_shared<GeoObj_LineString>();
            if (p_seg->one_way_) {
                p_line->AddPoint(p_seg->from_point_);
                p_line->AddPoint(p_seg->to_point_);
            }
            else {
                GeoPoint off_from, off_to;
                geo::get_offset_segment(p_seg->from_point_, p_seg->to_point_,
                    offset, off_from, off_to);
                p_line->AddPoint(off_from);
                p_line->AddPoint(off_to);
            }
            p_line->AddProp("seg_id", p_seg->seg_id_);
            p_line->AddProp("index", index++);
            p_line->AddProp("way_id", p_seg->way_id_);
            p_line->AddProp("one_way", (int)p_seg->one_way_);
            p_line->AddProp("length", p_seg->length_);
            p_line->AddProp("heading", p_seg->heading_);
            p_line->AddProp("way_name/type",
                (p_seg->way_name_.empty() ? "<null>" : p_seg->way_name_)
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

                    auto p_node = way_manager.GetNodeById(nodes[i]);
                    auto p_point = std::make_shared<geo::GeoObj_Point>(p_node->geo_point_);
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

        return true;
    }
};

class RouteMatcher
{
public:
    explicit RouteMatcher(const WayManager& way_manager)
        : way_manager_(way_manager)
    {}

public:
    bool RouteMatching(RouteMatchingParams &params) const
    {
        if (params.via_points.empty()) {
            params.result_route.clear();
            params.err = std::string(__FUNCTION__) + ": no input via points";
            return false;
        }

        params.result_route.clear();
        RouteMatchingImpl *p_impl;
        if (params.p_impl == nullptr) {
            p_impl = new RouteMatchingImpl(way_manager_, params);
            params.p_impl = (void *)p_impl;
        }
        else {
            p_impl = static_cast<RouteMatchingImpl *>(params.p_impl);
            p_impl->Init();
        }
        p_impl->FindMatchedRoute();

        if (params.result_route.empty()) {
            params.err = std::string(__FUNCTION__) + ": error in route matching from points trace";
            return false;
        }
        return true;
    }

    bool RouteMatchingResultToJson(const RouteMatchingParams &params,
        const std::string &pathname) const
    {
        return RouteMatchingImpl::ResultToJson(way_manager_, params, pathname);
    }

private:
    const WayManager& way_manager_;
    friend class geo::WayManager;
};

} // end of namespace route_match

///////////////////////////////////////////////////////////////////////////////////////////////////
// class WayManager

using namespace route;
using namespace geo::route_match;

bool WayManager::RouteMatching(RouteMatchingParams &params) const
{
    RouteMatcher matcher(*this);
    return matcher.RouteMatching(params);
}

bool WayManager::RouteMatchingResultToJson(const RouteMatchingParams &params,
    const std::string &pathname) const
{
    RouteMatcher matcher(*this);
    return matcher.RouteMatchingResultToJson(params, pathname);
}

void RouteMatchingInternalCleanup(RouteMatchingParams &params)
{
    if (params.p_impl) {
        RouteMatchingImpl *p_rm_impl = (RouteMatchingImpl *)params.p_impl;
        delete p_rm_impl;
        params.p_impl = nullptr;
    }
}

} // end of namespace
