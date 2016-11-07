
#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "way_manager.h"
#include <algorithm>
#include <memory>
#include <cmath>
#include <climits>
#include <chrono>
#include <atomic>
#include <stdexcept>
#include "common/common_utils.h"
#include "common/at_scope_exit.h"
#if WAY_MANAGER_HANA_LOG == 1
#include <hana/logging.h>
#endif

#ifdef max
#undef max
#endif

// compiling switch for using bidirection dijkstra or not
#define BI_DIR_DIJKSTRA     0
#if (BI_DIR_DIJKSTRA == 1)
#define CALL_DIJKSTRA DijkstraBiDir
#else
#define CALL_DIJKSTRA Dijkstra
#endif

// compiling switch for some data members for easy checking of variables
// should be disabled in product release version
#define EASY_NODE_DEBUG         0

// compiling switch for generation intermediate JSON files assisting debug
// should be disabled in product release version
#define ROUTING_STEPS_TO_JSON   0

// for Dijkstra statistics
#define DIJKSTRA_STATISTICS     1
#ifndef _WIN32
#undef DIJKSTRA_STATISTICS
#define DIJKSTRA_STATISTICS     0
#endif


using namespace std;

namespace geo {
namespace route {

typedef int ROUTING_NODE_INDEX;
typedef UNORD_MAP<NODE_ID_T, ROUTING_NODE_INDEX> RoutingNodeMap;

// routing_node => RN
//
//        RN1  seg1     seg2      seg3   RN2                       RN3
//         O--------->--------->--------->O---->------>----->------>O
//         <---------<---------<----------
//             seg4     seg5      seg6
// RN1 to RN2 is a connection. RN2 to RN1 is another connection.

typedef int CONN_INDEX;
typedef int CONN2_INDEX;
typedef int CONN4_INDEX;
typedef int CONN6_INDEX;

struct Connection
{
    ROUTING_NODE_INDEX i_from_rn_;
    ROUTING_NODE_INDEX i_to_rn_;
#if (EASY_NODE_DEBUG == 1)
    NodePtr p_from_nd_;
    NodePtr p_to_nd_;
#endif
    WAY_ID_T conn_way_id_; // signed way ID
    int weight_; // modified weight considering length, way type, etc.
    vector<SegmentPtr> segs_;
    bool excluded_flag_; // true if any of the segs' flag is set
};

// From RN1 via RN2, to RN3, is TwoStepConnection
struct TwoStepConnection
{
    ROUTING_NODE_INDEX i_from_rn_;
    ROUTING_NODE_INDEX i_mid_rn_;
    ROUTING_NODE_INDEX i_to_rn_; // to simplify, not including connections back to itself
    WAY_ID_T conn_way_id1_; // signed way ID
    WAY_ID_T conn_way_id2_; // signed way ID
    int weight_;
};

struct FourStepConnection
{
    ROUTING_NODE_INDEX i_from_rn_;
    ROUTING_NODE_INDEX i_to_rn_;
    ROUTING_NODE_INDEX mid_rns_[3]; // nodes in the middle
    WAY_ID_T conn_way_ids_[4]; // N signed way IDs
    int weight_;
    uint64_t hash_three_;
};

struct SixStepConnection
{
    ROUTING_NODE_INDEX i_from_rn_;
    ROUTING_NODE_INDEX i_to_rn_;
    ROUTING_NODE_INDEX mid_rns_[5]; // nodes in the middle
    int weight_;
    WAY_ID_T conn_way_ids_[6]; // N signed way IDs
    uint64_t hash_five_;
};

class RoutingNode
{
public:
    explicit RoutingNode(const NodePtr& p_node)
        : p_node_(p_node)
    {}

    NODE_ID_T NodeId() const
    {
        return p_node_->nd_id_;
    }

public:
    NodePtr             p_node_;
    vector<CONN_INDEX>  conn_froms_;
    vector<CONN_INDEX>  conn_tos_;
    vector<CONN2_INDEX> two_step_conn_tos_;
    vector<CONN4_INDEX> four_step_conn_tos_;
    vector<CONN6_INDEX> six_step_conn_tos_;
    vector<WAY_ID_T>    out_oriented_ways_; // ways starting from this node
};

class RouteManager;

namespace dijkstra {

typedef int NODE_DATA_INDEX;

static const NODE_DATA_INDEX INVALID_NODE_INDEX = 0;

struct NodeData
{
    NodeData()
        : i_rn(0), i_pre_rn(0), distance(0), finished(false), heap_index(0)
    {}

    explicit NodeData(ROUTING_NODE_INDEX rn, ROUTING_NODE_INDEX pre_rn, int dist)
        : i_rn(rn), i_pre_rn(pre_rn), distance(dist), finished(false), heap_index(0)
    {}

    ROUTING_NODE_INDEX i_rn;
    ROUTING_NODE_INDEX i_pre_rn;
    int distance;
    bool finished; // indicating if in the "finished" node set
    int heap_index; // index of the node in min bin heap

#if (EASY_NODE_DEBUG == 1)
    RoutingNode* p_rn;
    RoutingNode* p_pre_rn;
#endif
};

// as the key is in range [1, max_node], the map is optimized to use vector
// instead of hash map
template<typename KEY, typename VALUE>
class DirectAccessNodeMap
{
public:
    void Reserve(int max_node)
    {
        vector_.resize(max_node + 1);
    }

    void Clear()
    {
        vector_.clear();
    }

    VALUE& operator[](KEY i_rn)
    {
        return vector_[i_rn];
    }

    VALUE operator[](KEY i_rn) const
    {
        return vector_[i_rn];
    }

private:
    std::vector<VALUE> vector_;
};

class HeapData
{
public:
    explicit HeapData(NODE_DATA_INDEX pool_index, int distance)
        : pool_index_(pool_index), distance_(distance)
    {}

    NODE_DATA_INDEX pool_index_; // index in pool
    // distance from source to this node, identical to NodeData::distance
    // the reason to maintain duplicate distance is for speed performance
    int distance_;
};


class BinHeap // min-heap
{
public:
    explicit BinHeap(const util::SimpleObjPool<RoutingNode>& routing_node_pool, int max_node)
        : max_node_(max_node), routing_node_pool_(routing_node_pool)
    {
        pool_.Reserve(max_node);
        // allocate a dummy one whose NODE_DATA_INDEX is 0. 0 is regarded as
        // invalid NODE_DATA_INDEX
        pool_.AllocNew();

        heap_.reserve(max_node);
        inserted_.Reserve(max_node);
    }

    bool Empty() const
    {
        return heap_.empty();
    }

    // return new created NodeData
    NODE_DATA_INDEX Insert(ROUTING_NODE_INDEX i_rn, ROUTING_NODE_INDEX i_pre_rn, int distance)
    {
        NODE_DATA_INDEX& i_node = inserted_[i_rn];

        if (i_node == 0) {
            i_node = pool_.AllocNewIndex(NodeData(i_rn, i_pre_rn, distance));
#if (EASY_NODE_DEBUG == 1)
            pool_[i_node].p_rn = const_cast<RoutingNode*>(routing_node_pool_.ObjPtrByIndex(i_rn));
            pool_[i_node].p_pre_rn = const_cast<RoutingNode*>(
                routing_node_pool_.ObjPtrByIndex(i_pre_rn));
#endif

            heap_.emplace_back(i_node, distance);
            int new_heap_index = (int)heap_.size() - 1;
            pool_[i_node].heap_index = new_heap_index;
            HeapBubbleUp(new_heap_index);
        }

        return i_node;
    }

    // return INVALID_NODE_INDEX if was not inserted
    NODE_DATA_INDEX GetIfInserted(ROUTING_NODE_INDEX i_rn) const
    {
        auto i_node_data = inserted_[i_rn];
        return (i_node_data == 0) ? INVALID_NODE_INDEX : i_node_data;
    }

    // precondition: heap_[] non-empty
    HeapData DeleteMin()
    {
        HeapData min_node = heap_.front();

        HeapSwap(heap_.front(), heap_.back());
        heap_.pop_back();
        HeapBubbleDown(0);

        return min_node;
    }

    void DecreaseKey(const HeapData& heap_node)
    {
        auto& distance = pool_[heap_node.pool_index_].distance;
        if (distance > heap_node.distance_) {
            distance = heap_node.distance_;

            const int& heap_index = pool_[heap_node.pool_index_].heap_index;
            heap_[heap_index].distance_ = heap_node.distance_;

            HeapBubbleUp(heap_index);
        }
    }

    void DecreaseKeys(const vector<HeapData>& heap_nodes)
    {
        for (const auto& nd : heap_nodes) {
            DecreaseKey(nd);
        }
    }

    void RemoveAll()
    {
        heap_.clear();
    }

    void Clear()
    {
        pool_.Clear();
        heap_.clear();
        inserted_.Clear();
        search_steps_ = 0;
    }

    ROUTING_NODE_INDEX GetPreNode(ROUTING_NODE_INDEX i_rn) const
    {
        auto i_node_data = inserted_[i_rn];
        return (i_node_data == INVALID_NODE_INDEX) ? 0 : pool_[i_node_data].i_pre_rn;
    }

    util::SimpleObjPool<NodeData>& NodePool()
    {
        return pool_;
    }

    int MaxNodeCount() const
    {
        return max_node_;
    }

private:
    void HeapSwap(HeapData& i, HeapData& j)
    {
        if (&i != &j) {
            std::swap(pool_[i.pool_index_].heap_index, pool_[j.pool_index_].heap_index);
            std::swap(i, j);
        }
    }

    void HeapBubbleUp(int index)
    {
        while (index > 0) {
            int parent = (index - 1) / 2;
            if (heap_[parent].distance_ <= heap_[index].distance_) {
                // stop when element is not less than it's parent
                break;
            }
            else {
                HeapSwap(heap_[index], heap_[parent]);
                index = parent;
            }
        }
    }

    // Heapify
    void HeapBubbleDown(int index)
    {
        const int count = (int)heap_.size();
        const int last_parent = (count - 2) / 2;
        while (index <= last_parent) {
            int left = 2 * index + 1;
            int right = left + 1;
            int min = index;

            if (left < count && heap_[left].distance_ <= heap_[min].distance_) {
                min = left;
            }
            if (right < count && heap_[right].distance_ <= heap_[min].distance_) {
                min = right;
            }

            // stop if index element is not less than both children
            if (min == index) {
                break;
            }
            HeapSwap(heap_[index], heap_[min]);
            index = min;
        }
    }

private:
    const int max_node_;
    const util::SimpleObjPool<RoutingNode>& routing_node_pool_;
    util::SimpleObjPool<NodeData> pool_;
    vector<HeapData> heap_;
    DirectAccessNodeMap<ROUTING_NODE_INDEX, NODE_DATA_INDEX> inserted_;
    int search_steps_{};

    friend class geo::route::RouteManager;
};

}


struct route_info_tuple
{
    vector<SegmentPtr> route;
    double      length;
    uint64_t    hash;
    double      distance; // Hausdorff distance to the given trace
};

static bool operator==(const route_info_tuple& r1, const route_info_tuple& r2)
{
    if (r1.route.size() != r2.route.size()) {
        return false;
    }
    for (size_t i = 0; i < r1.route.size(); ++i) {
        if (r1.route[i]->seg_id_ != r2.route[i]->seg_id_) {
            return false;
        }
    }
    return true;
}

class RouteManager
{
public:
    explicit RouteManager(const WayManager& way_manager)
        : way_manager_(way_manager)
    {}

    bool InitForRouting(bool shortest_mode)
    {
#if WAY_MANAGER_HANA_LOG == 1
        hana::Logger logger("WayManager");
        auto start = chrono::system_clock::now();
#endif

        shortest_mode_ = shortest_mode;

        routing_node_pool_.Clear();
        routing_node_pool_.Reserve(way_manager_.node_map_.size() / 4);
        // dummy one as index zero is used as invalid index
        routing_node_pool_.AllocNewIndex(RoutingNode(nullptr));

        // build RoutingNodeMap object, pre-allocated all the routing node
        routing_node_map_.clear();
        routing_node_map_.reserve(way_manager_.node_map_.size() / 4); // rough estimate
        for (auto const it : way_manager_.node_map_) {
            const NodePtr& p_node = it.second;
            if (p_node->IsRoutingNode()) {
                routing_node_map_[p_node->nd_id_] = routing_node_pool_.AllocNewIndex(
                    RoutingNode(p_node));
            }
        }

        // rough estimates for the capacities of the pools
        conn_pool_.Reserve(routing_node_map_.size() * 5 / 2);
        conn2_pool_.Reserve(routing_node_map_.size() * 5);
        conn4_pool_.Reserve(routing_node_map_.size() * 15);
        conn6_pool_.Reserve(routing_node_map_.size() * 50);
        conn_pool_.AllocNewIndex(); // dummy one as index zero is used as invalid index
        conn2_pool_.AllocNewIndex();
        conn4_pool_.AllocNewIndex();
        conn6_pool_.AllocNewIndex();

        InitConnsOneStep();
        InitRoutingNodesOutWays();
        InitConnsTwoSteps();
        InitConnsFourSteps();
        InitConnsSixSteps();

#if WAY_MANAGER_HANA_LOG == 1
        chrono::duration<double> elapsed = chrono::system_clock::now() - start;
        HANA_SDK_DEBUG(logger) << __FUNCTION__ << ": run time "
            << elapsed.count() << " seconds" << hana::endl;
#endif
        return !routing_node_map_.empty();
    }

    void SyncExclusionSegsToRouting()
    {
        // recalculate all the edges' excluded_flag_
        const int size = (int)conn_pool_.Size();
        for (int i = 0; i < size; ++i) {
            auto p_conn = conn_pool_.ObjPtrByIndex(i);

            p_conn->excluded_flag_ = false;
            for (const auto& p_seg : p_conn->segs_) {
                if (p_seg->excluded_flag_) {
                    p_conn->excluded_flag_ = true;
                    break;
                }
            }
        }
    }

    bool RoutingNearby(const SegmentPtr& p_seg1, const SegmentPtr& p_seg2,
        vector<SegmentPtr>& result_route, bool exclude_reverse_segs /*= false*/,
        time_t time_point /*= 0*/, bool is_localtime /*= false*/,
        bool points_reversed /*= false*/) const
    {
        // same segment ID
        if (p_seg1 == p_seg2 || p_seg1->seg_id_ == p_seg2->seg_id_) {
            if (!points_reversed) {
                result_route.resize(1);
                result_route[0] = p_seg1;
                return true;
            }
        }

        //          seg1 = seg2    ---------o----------o---------->
        //                                point2    point1
        const bool same_seg_and_points_reversed =
            (p_seg1 == p_seg2 || p_seg1->seg_id_ == p_seg2->seg_id_) && points_reversed;
        const WAY_ID_T signed_seg1_way_id = p_seg1->GetWayIdOriented();
        const WAY_ID_T signed_seg2_way_id = p_seg2->GetWayIdOriented();

        // same way ID and in same direction
        if (signed_seg1_way_id == signed_seg2_way_id) {
            if (!same_seg_and_points_reversed) {
                result_route.clear();
                bool result = RoutingSameOrientedWay(p_seg1, p_seg2, result_route);
                if (result) {
                    if (time_point == 0) {
                        return result;
                    }
                    else {
                        if (!this->HasExcludedSegs(result_route, time_point, is_localtime)) {
                            return result;
                        }
                    }
                }
            }
            // if failed, fall back to below more complicated methods
        }

        const OrientedWayPtr& p_way1 = p_seg1->GetWayOriented();
        if (!p_way1) {
            return false;
        }
        vector<SegmentPtr>::const_iterator it_seg1 = p_way1->FindSegment(p_seg1->seg_id_);
        if (it_seg1 == p_way1->Segments().end()) {
            return false;
        }

        vector<route_info_tuple> candidate_routes;
        candidate_routes.reserve(16);

        const RoutingNode* p_routing_node1 = nullptr;
        for (auto it_way1_seg = it_seg1; it_way1_seg != p_way1->Segments().end(); ++it_way1_seg) {
            const NodePtr& p_node = (*it_way1_seg)->GetToNode();
            if (p_node->IsRoutingNode()) {
                auto&& it = routing_node_map_.find(p_node->nd_id_);
                if (it != routing_node_map_.end()) {
                    p_routing_node1 = routing_node_pool_.ObjPtrByIndex(it->second);
                    break;
                }
            }
        }
        if (p_routing_node1 == nullptr) {
            return false;
        }
        bool no_three_step_routes;

        AppendAllOneStepRoutes(p_seg1, p_seg2, p_routing_node1, signed_seg2_way_id,
            candidate_routes);
        AppendAllTwoStepRoutes(p_seg1, p_seg2, p_routing_node1, signed_seg2_way_id,
            candidate_routes);
        AppendAllThreeStepRoutes(p_seg1, p_seg2, p_routing_node1, signed_seg2_way_id,
            candidate_routes);
        no_three_step_routes = candidate_routes.empty();
        AppendAllFourStepRoutes(p_seg1, p_seg2, p_routing_node1, signed_seg2_way_id,
            candidate_routes);
        // only search over-5-step routes if no very short routes found
        if (no_three_step_routes) {
            AppendAllFiveStepRoutes(p_seg1, p_seg2, p_routing_node1, signed_seg2_way_id,
                candidate_routes);
            AppendAllSixStepRoutes(p_seg1, p_seg2, p_routing_node1, signed_seg2_way_id,
                candidate_routes);
            AppendAllSevenStepRoutes(p_seg1, p_seg2, p_routing_node1, signed_seg2_way_id,
                candidate_routes);
        }

        if (candidate_routes.empty()) {
            return false;
        }
        if (same_seg_and_points_reversed) {
            // if point1 and point2 are reversed and assigned the same segment, remove the
            // the wrong route from candidates.
            auto it = std::partition(candidate_routes.begin(), candidate_routes.end(),
                [p_seg1](const route_info_tuple& r) {
                return !(r.route.size() == 1 && r.route.front() == p_seg1);
            });
            if (it != candidate_routes.end()) {
                candidate_routes.resize(it - candidate_routes.begin());
            }
            if (candidate_routes.empty()) {
                return false;
            }
        }

        if (exclude_reverse_segs) {
            // sort by length asc. if same length, prefer route without reverse segs
            std::sort(candidate_routes.begin(), candidate_routes.end(),
                [this](const route_info_tuple& i, const route_info_tuple& j) {
                if (i.length == j.length) {
                    bool has_rev_i = HasReverseSegs(i.route);
                    bool has_rev_j = HasReverseSegs(j.route);
                    return has_rev_i > has_rev_j;
                }
                return i.length < j.length;
            });

            for (auto& candidate_route : candidate_routes) {
                bool found_reverse = HasReverseSegs(candidate_route.route);
                if (!found_reverse) {
                    result_route = candidate_route.route;
                    return true;
                }
            }
        }
        else {
            if (time_point == 0) {
                std::nth_element(candidate_routes.begin(),
                    candidate_routes.begin(), candidate_routes.end(),
                    [](const route_info_tuple& i, const route_info_tuple& j) {
                    return i.length < j.length;
                });
                result_route = candidate_routes.front().route;
                return true;
            }
            else {
                std::nth_element(candidate_routes.begin(),
                    candidate_routes.begin(), candidate_routes.end(),
                    [this, time_point, is_localtime](const route_info_tuple& i,
                        const route_info_tuple& j) {
                    auto has_i = this->HasExcludedSegs(i.route, time_point, is_localtime);
                    auto has_j = this->HasExcludedSegs(j.route, time_point, is_localtime);
                    if (has_i == has_j) {
                        return i.length < j.length;
                    }
                    return has_i > has_j;
                });
                auto has_excluded = this->HasExcludedSegs(candidate_routes.front().route,
                    time_point, is_localtime);
                if (!has_excluded) {
                    result_route = candidate_routes.front().route;
                    return true;
                }
            }
        }

        return false;
    }

    template<typename POINT>
    bool SimilarRoutingNearbyImpl(const SegmentPtr& p_seg1, const SegmentPtr& p_seg2,
        const vector<POINT>& trace, vector<SegmentPtr>& result_route,
        double& distance, bool exclude_reverse_segs) const
    {
        distance = -1.0;
        if (trace.size() < 2) {
            return this->RoutingNearby(p_seg1, p_seg2, result_route,
                exclude_reverse_segs, 0, false, false);
        }

        // same segment ID
        if (p_seg1 == p_seg2 || p_seg1->seg_id_ == p_seg2->seg_id_) {
            result_route.resize(1);
            result_route[0] = p_seg1;
            return true;
        }

        const OrientedWayPtr& p_way1 = p_seg1->GetWayOriented();
        if (!p_way1) {
            return false;
        }
        vector<SegmentPtr>::const_iterator it_seg1 = p_way1->FindSegment(p_seg1->seg_id_);
        if (it_seg1 == p_way1->Segments().end()) {
            return false;
        }

        const OrientedWayPtr& p_way2 = p_seg2->GetWayOriented();
        if (!p_way2) {
            return false;
        }
        if (p_way2->WayId() == p_way1->WayId()) { // same way ID and in same direction
            vector<SegmentPtr>::const_iterator it_seg2 = p_way2->FindSegment(p_seg2->seg_id_);
            if (it_seg2 == p_way2->Segments().end()) {
                return false;
            }
            if (it_seg1 <= it_seg2) {
                result_route.clear();
                bool ok = RoutingSameOrientedWay(p_seg1, p_seg2, result_route);
                if (ok && exclude_reverse_segs && HasReverseSegs(result_route)) {
                    ok = false;
                }
                if (ok) {
                    distance = Distance(result_route, trace);
                }
                return distance < 1000000.0;
            }
        }

        vector<route_info_tuple> candidate_routes;
        candidate_routes.reserve(16);
        const RoutingNode* p_routing_node1 = nullptr;
        for (auto it_way1_seg = it_seg1; it_way1_seg != p_way1->Segments().end(); ++it_way1_seg) {
            const NodePtr& p_node = (*it_way1_seg)->GetToNode();
            if (p_node->IsRoutingNode()) {
                auto&& it = routing_node_map_.find(p_node->nd_id_);
                if (it != routing_node_map_.end()) {
                    p_routing_node1 = routing_node_pool_.ObjPtrByIndex(it->second);
                    break;
                }
            }
        }
        if (p_routing_node1 == nullptr) {
            return false;
        }

        const WAY_ID_T signed_seg2_way_id = p_seg2->GetWayIdOriented();
        AppendAllOneStepRoutes(p_seg1, p_seg2, p_routing_node1, signed_seg2_way_id,
            candidate_routes);
        AppendAllTwoStepRoutes(p_seg1, p_seg2, p_routing_node1, signed_seg2_way_id,
            candidate_routes);
        AppendAllThreeStepRoutes(p_seg1, p_seg2, p_routing_node1, signed_seg2_way_id,
            candidate_routes);
        AppendAllFourStepRoutes(p_seg1, p_seg2, p_routing_node1, signed_seg2_way_id,
            candidate_routes);
        AppendAllFiveStepRoutes(p_seg1, p_seg2, p_routing_node1, signed_seg2_way_id,
            candidate_routes);
        AppendAllSixStepRoutes(p_seg1, p_seg2, p_routing_node1, signed_seg2_way_id,
            candidate_routes);
        AppendAllSevenStepRoutes(p_seg1, p_seg2, p_routing_node1, signed_seg2_way_id,
            candidate_routes);
        if (candidate_routes.empty()) {
            return false;
        }

        // remove routes with reverse segs
        if (exclude_reverse_segs) {
            RemoveRouteWithReverseSegs(candidate_routes);
        }
        RemoveDuplicatedRoutes(candidate_routes);

        if (candidate_routes.empty()) {
            return false;
        }
        else if (candidate_routes.size() == 1) {
            result_route = move(candidate_routes.front().route);
            distance = this->Distance(result_route, trace);
            return distance < 1000000.0;
        }
        else {
            // sort by similarity to the trace
            for (auto& route_info : candidate_routes) {
                route_info.distance = Distance(route_info.route, trace);
            }
            nth_element(candidate_routes.begin(), candidate_routes.begin(), candidate_routes.end(),
                [](const route_info_tuple& i, const route_info_tuple& j)
            {
                auto diff = i.distance - j.distance;
                if (diff < 0) diff = -diff;
                if (diff < 0.00003) {
                    return i.length < j.length;
                }
                return i.distance < j.distance;
            });

            result_route = move(candidate_routes.front().route);
            distance = candidate_routes.front().distance;
            return distance < 1000000.0;
        }

        return false;
    }

    static double hAB(const vector<GeoPoint>& A, const vector<GeoPoint>& B)
    {
        const int NA = (int)A.size();
        const int NB = (int)B.size();

        double maxA2 = 0;
        for (int i = 0; i < NA; ++i) {
            const GeoPoint& a = A[i];
            double distMin2 = std::numeric_limits<double>::max();;
            for (int j = NB - 1; j >= 0; --j) {
                const GeoPoint& b = B[j];
                double s1 = b.lat - a.lat;
                double s2 = b.lng - a.lng;
                double dist2 = s1 * s1 + s2 * s2;
                if (dist2 < distMin2) {
                    distMin2 = dist2;
                }
            }
            if (maxA2 < distMin2) {
                maxA2 = distMin2;
            }
        }
        if (maxA2 > 1000000.0) {
            return std::numeric_limits<double>::max();
        }
        return ::sqrt(maxA2);
    }

    static double Distance(const vector<SegmentPtr>& route, const vector<GeoPoint>& trace)
    {
        vector<GeoPoint> A;
        const vector<GeoPoint>& B = trace;
        A.reserve(route.size() + 1);
        for (const auto& p_seg : route) {
            A.push_back(p_seg->from_point_);
        }
        A.push_back(route.back()->to_point_);

        double hab = hAB(A, B);
        double hba = hAB(B, A);
        return hab > hba ? hab : hba; // max
    }

    // Optimized the distance calculation using heading.
    // Simple hausdorff distance is not well-suited for special route like with Z shape.
    // Frechet distance is better considering the order of the trace, but with poor performance
    // and more implementation complexity. A practical compromise solution is to compute the
    // hausdorff distance considering GPS heading info. For most of the cases, the distance is
    // much more similar to Frechet distance, but much faster and easier to implement.
    //                   s2       s1
    //              <----------<---------
    //              | <--o P2
    //          s3  |             <--o P1
    //              V----------->----------->
    //                  s4         s5
    // E.g., the for the above GPS point P1, the hausdorff from A (P1, P2, ...) to
    // B (s1, s2, s3, ...) is greater than the simple hausdoff distance
    static double Distance(const vector<SegmentPtr>& route, const vector<ViaPoint>& trace)
    {
        const int ANGLE_TOLERANCE = 40;
        auto dist_point_to_segs_path = [&route, ANGLE_TOLERANCE](const ViaPoint& p) {
            double min_dist = std::numeric_limits<double>::max();
            for (auto& p_seg : route) {
                if (p_seg->length_ < 0.1) continue;
                if (p.heading != -1) {
                    if (WayManager::GetAngle(p_seg->heading_, p.heading) > ANGLE_TOLERANCE) {
                        continue;
                    }
                }
                double dist = p_seg->CalcDistanceSquareMeters(p.geo_point, *p_seg);
                if (dist < min_dist) {
                    min_dist = dist;
                }
            }
            if (min_dist > 1000000.0) {
                return std::numeric_limits<double>::max();
            }
            return std::sqrt(min_dist);
        };

        double max_dist = -1.0;
        for (auto& via_point : trace) {
            auto dist = dist_point_to_segs_path(via_point);
            if (dist > max_dist) {
                max_dist = dist;
            }
        }

        return max_dist;
    }

    static void RemoveRouteWithReverseSegs(vector<route_info_tuple>& candidate_routes)
    {
        int removed_count = 0;
        for (auto& route_info : candidate_routes) {
            if (HasReverseSegs(route_info.route)) {
                route_info.route.clear();
                ++removed_count;
            }
        }
        if (removed_count > 0) {
            RemoveFlagedCandiates(candidate_routes, removed_count);
        }
    }

    static void RemoveDuplicatedRoutes(vector<route_info_tuple>& candidate_routes)
    {
        if (candidate_routes.size() <= 1) {
            return;
        }

        // for route hash
        for (auto& route_info : candidate_routes) {
            route_info.hash = 0;
            for (const auto& p_seg : route_info.route) {
                route_info.hash += (uint64_t)(p_seg->seg_id_);
            }
        }

        // remove same route in candiates
        int removal_count = 0;
        for (int i = (int)candidate_routes.size() - 1; i > 0; --i) {
            auto& route_info_i = candidate_routes[i];
            for (int j = 0; j < i; ++j) {
                if (route_info_i.hash != candidate_routes[j].hash) {
                    continue;
                }
                if (route_info_i.route.size() != candidate_routes[j].route.size()) {
                    continue;
                }
                if (route_info_i == candidate_routes[j]) {
                    route_info_i.route.clear();
                    ++removal_count;
                    break;
                }
            }
        }
        if (removal_count > 0) {
            RemoveFlagedCandiates(candidate_routes, removal_count);
        }
    }

    static void RemoveFlagedCandiates(vector<route_info_tuple>& candidate_routes,
        int removal_count)
    {
        vector<route_info_tuple> candidate_routes_new;
        candidate_routes_new.reserve(candidate_routes.size() - removal_count);
        for (auto& route_info : candidate_routes) {
            if (!route_info.route.empty()) {
                candidate_routes_new.push_back(move(route_info));
            }
        }
        candidate_routes = move(candidate_routes_new);
    }

    void GetRoutingSummary(int& total_request, int& success_request)
    {
#if DIJKSTRA_STATISTICS == 1
        total_request = this->dijkstra_calls_num_;
        success_request = this->dijkstra_calls_success_num_;
#else
        total_request = 0;
        success_request = 0;
#endif
    }

    //                              A s1
    //           p_seg              |  s2
    //   O------>----->------>----->O---->----->
    //                              |
    //                              V s3
    // node_id: if node ID is 0, the API will automatically check the most closest "routing node"
    // output: [s1, s2, s3]
    bool GetAdjacentOutboundSegs(const SegmentPtr& p_seg, NODE_ID_T node_id,
        std::vector<SegmentPtr>& out_segs) const
    {
        NodePtr p_node = nullptr; 
        if (node_id == 0) {
            ROUTING_NODE_INDEX i_rn = GetSrcRoutingNode(p_seg);
            if (i_rn) {
                p_node = routing_node_pool_[i_rn].p_node_;
                node_id = p_node->nd_id_;
            }
        }
        else {
            p_node = way_manager_.GetNodeById(node_id);
        }
        if (node_id == 0 || p_node == nullptr) {
            return false;
        }

        out_segs.clear();
        for (const auto& p_conn_seg : p_node->ConnectedSegments()) {
            if (p_conn_seg->from_nd_ == node_id) {
                out_segs.push_back(p_conn_seg);
            }
        }

        return true;
    }

    //    p_seg0       p_seg
    //   O------>----->------>----->O---->----->O
    // return: p_seg0
    SegmentPtr GetLeadSeg(const SegmentPtr& p_seg) const
    {
        if (p_seg->GetFromNode()->IsRoutingNode()) { // lucky
            return p_seg;
        }

        auto& segs = p_seg->GetWayOriented()->Segments();
        const int segs_size = (int)segs.size();
        int i1;

        for (i1 = 0; i1 < segs_size; ++i1) {
            if (segs[i1] == p_seg) {
                break;
            }
        }
        if (i1 >= segs_size) { // not found
            return nullptr;
        }

        for (int i = i1 - 1; i >= 0; --i) {
            if (segs[i]->GetFromNode()->IsRoutingNode()) {
                return segs[i];
            }
        }

        return segs.front();
    }

private:
    static bool HasReverseSegs(const vector<SegmentPtr>& route)
    {
        bool found_reverse = false;
        for (const auto& p_seg : route) {
            if (p_seg->seg_id_ < 0) {
                found_reverse = true;
                break;
            }
        }
        return found_reverse;
    }

    // weather to have excluded segs (e.g., closed tunnels in the middle night)
    bool HasExcludedSegs(const vector<SegmentPtr>& route, time_t time_point,
        bool is_localtime) const
    {
        for (const auto& p_seg : route) {
            if (p_seg->excluded_flag_) {
                if (p_seg->excluded_always_) {
                    return true;
                }
                else {
                    if (way_manager_.IsSegmentExcluded(p_seg, time_point, is_localtime)) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    static int DistanceToWeight(double distance, HIGHWAY_TYPE type)
    {
        const static int SPEED_PROFILE[]
        {
            10,   // HIGHWAY_UNKNOWN

            90,  // HIGHWAY_MOTORWAY
            85,  // HIGHWAY_TRUNK
            65,  // HIGHWAY_PRIMARY
            55,  // HIGHWAY_SECONDARY
            40,  // HIGHWAY_TERTIARY
            25,  // HIGHWAY_UNCLASSIFIED
            25,  // HIGHWAY_RESIDENTIAL
            15,  // HIGHWAY_SERVICE
            10,  // HIGHWAY_RESERVE1
            10,  // HIGHWAY_RESERVE2
            10,  // HIGHWAY_RESERVE3

            // Link roads
            45,  // HIGHWAY_MOTORWAY_LINK
            40,  // HIGHWAY_TRUNK_LINK
            30,  // HIGHWAY_PRIMARY_LINK
            25,  // HIGHWAY_SECONDARY_LINK
            20,  // HIGHWAY_TERTIARY_LINK
            10,  // HIGHWAY_RESERVE4
            10,  // HIGHWAY_RESERVE5
            10,  // HIGHWAY_RESERVE6

            // Special road types
            10,  // HIGHWAY_LIVING_STREET
            10,  // HIGHWAY_PEDESTRIAN
            10,  // HIGHWAY_TRACK
            10,  // HIGHWAY_BUS_GUIDEWAY
            50,  // HIGHWAY_RACEWAY
            10,  // HIGHWAY_ROAD
            10,  // HIGHWAY_RESERVE7
            10,  // HIGHWAY_RESERVE8
            10,  // HIGHWAY_RESERVE9

            // Paths
            10,  // HIGHWAY_FOOTWAY
            10,  // HIGHWAY_BRIDLEWAY
            10,  // HIGHWAY_STEPS
            10,  // HIGHWAY_PATH
            10,  // HIGHWAY_RESERVE10
            10,  // HIGHWAY_RESERVE11

            40,  // HIGHWAY_CYCLEWAY
            10,  // HIGHWAY_RESERVE12
            10,  // HIGHWAY_RESERVE13

            // Lifecycle
            10,  // HIGHWAY_PROPOSED
            10,  // HIGHWAY_CONSTRUCTION
            10,  // HIGHWAY_RESERVE14
            10,  // HIGHWAY_RESERVE15
        };
        static_assert(_countof(SPEED_PROFILE) == HIGHWAY_TYPE_MAX,
            "SPEED_PROFILE element count must match the highway type definition");

        // scale speeds to get better average driving times
        const double speed_reduction = 0.8;
        double speed = SPEED_PROFILE[type];
        if (speed > 0) {
            speed = speed * speed_reduction + 11;
        }

        double weight = (distance * 10.) / (speed / 3.6);
        int i_weight = int(weight + .5);
        return (i_weight <= 0) ? 1 : i_weight;
    }

    bool InitConnsOneStep()
    {
#if WAY_MANAGER_HANA_LOG == 1
        hana::Logger logger("WayManager");
        auto start = chrono::system_clock::now();
#endif

        for (auto const& it : routing_node_map_) {
            const ROUTING_NODE_INDEX& i_routing_node = it.second;
            const NodePtr& p_node = routing_node_pool_[i_routing_node].p_node_;

            for (auto& p_segment1 : p_node->ConnectedSegments()) {
                if (p_node->nd_id_ != p_segment1->from_nd_) { // need only the "from" node
                    continue;
                }
                const OrientedWayPtr& p_way = p_segment1->GetWayOriented();
                if (!p_way) {
                    continue; // should never happen
                }
                auto it_1 = p_way->FindSegment(p_segment1->seg_id_);
                if (it_1 == p_way->Segments().cend()) {
                    continue; // should never happen
                }

                double distance = 0;
                int way_connector_count = 0;

                for (auto it = it_1; it != p_way->Segments().cend(); ++it) {
                    const NodePtr& p_to_nd = (*it)->GetToNode();
                    distance += (*it)->length_;

                    if (p_to_nd && p_to_nd->IsRoutingNode()) {
                        bool new_conn = false;
                        if (p_to_nd->IsDeadEndNode()) {
                            new_conn = true;
                            //          RN  seg1           p_seg
                            // --------->O--------->  ... ------->O
                            //                                    ^
                            //                                    dead end
                        }
                        else {
                            //          RN  seg1           p_seg      seg2
                            // --------->O--------->  ... ------->O--------->
                            //                                    ^
                            //                                    way connector

                            for (const auto& p_segment2 : p_to_nd->ConnectedSegments()) {
                                // only connected to "from" node, can have the same way ID
                                if (p_segment2 != nullptr
                                    && p_segment2->from_nd_ == p_to_nd->nd_id_
                                    && p_node->nd_id_ != p_to_nd->nd_id_) {

                                    new_conn = true;
                                    break;
                                }
                            }
                        }

                        if (new_conn) {
                            CONN_INDEX i_conn = conn_pool_.AllocNewIndex();
                            Connection* p_conn = conn_pool_.ObjPtrByIndex(i_conn);
                            p_conn->conn_way_id_ = p_way->WayId();
                            p_conn->i_from_rn_ = i_routing_node;
                            p_conn->i_to_rn_ = routing_node_map_.at(p_to_nd->nd_id_);
#if (EASY_NODE_DEBUG == 1)
                            p_conn->p_from_nd_ = routing_node_pool_[p_conn->i_from_rn_].p_node_;
                            p_conn->p_to_nd_ = routing_node_pool_[p_conn->i_to_rn_].p_node_;
#endif
                            p_conn->segs_.clear();
                            this->RoutingSameOrientedWay(p_way, p_node, p_to_nd, p_conn->segs_);
                            if (p_conn->segs_.front()->seg_id_ < 0) {
                                // for simulated reverse way, e.g., bus specific way,
                                // a little bit not preferred
                                p_conn->weight_ = DistanceToWeight(distance * 1.05,
                                    p_way->HighwayType());
                            }
                            else {
                                p_conn->weight_ = DistanceToWeight(distance, p_way->HighwayType());
                            }

                            p_conn->excluded_flag_ = false;
                            for (const auto& p_seg : p_conn->segs_) {
                                if (p_seg->excluded_flag_) {
                                    p_conn->excluded_flag_ = true;
                                    break;
                                }
                            }

                            routing_node_pool_[p_conn->i_to_rn_].conn_froms_.push_back(i_conn);
                            routing_node_pool_[i_routing_node].conn_tos_.push_back(i_conn);
                            ++way_connector_count;
                        }
                    }

                    // no skip of connector
                    if (way_connector_count > 0) {
                        break;
                    }
                }
            }
        }

#if WAY_MANAGER_HANA_LOG == 1
        chrono::duration<double> elapsed = chrono::system_clock::now() - start;
        HANA_SDK_DEBUG(logger) << __FUNCTION__ << ": run time "
            << elapsed.count() << " seconds" << hana::endl;
#endif
        return true;
    }

    // init out_oriented_ways_
    bool InitRoutingNodesOutWays()
    {
#if WAY_MANAGER_HANA_LOG == 1
        hana::Logger logger("WayManager");
        auto start = chrono::system_clock::now();
#endif

        for (const auto& it : routing_node_map_) {
            const auto p_routing_node = routing_node_pool_.ObjPtrByIndex(it.second);
            auto& out_ways = p_routing_node->out_oriented_ways_;

            out_ways.clear();
            for (auto& p_conn_seg : p_routing_node->p_node_->ConnectedSegments()) {
                if (p_conn_seg->from_nd_ != p_routing_node->p_node_->nd_id_) {
                    continue;
                }

                if (std::find(out_ways.begin(), out_ways.end(), p_conn_seg->way_id_)
                    == out_ways.end()) {
                    out_ways.push_back(p_conn_seg->way_id_);
                }
            }
        }

#if WAY_MANAGER_HANA_LOG == 1
        chrono::duration<double> elapsed = chrono::system_clock::now() - start;
        HANA_SDK_DEBUG(logger) << __FUNCTION__ << ": run time "
            << elapsed.count() << " seconds" << hana::endl;
#endif
        return true;
    }

    bool InitConnsTwoSteps()
    {
#if WAY_MANAGER_HANA_LOG == 1
        hana::Logger logger("WayManager");
        auto start = chrono::system_clock::now();
#endif

        // init two_step_conn_tos_ in each routing node
        for (auto& it1 : routing_node_map_) {
            const auto i_routing_node1 = it1.second;
            const auto p_routing_node1 = routing_node_pool_.ObjPtrByIndex(i_routing_node1);
            p_routing_node1->two_step_conn_tos_.reserve(p_routing_node1->conn_tos_.size() * 3);

            for (auto i_nd1_conn2 : p_routing_node1->conn_tos_) {
                const auto p_routing_node2 = routing_node_pool_.ObjPtrByIndex(
                    conn_pool_[i_nd1_conn2].i_to_rn_);
                const auto& conn_way_id1 = conn_pool_[i_nd1_conn2].conn_way_id_;

                for (auto i_nd2_conn3 : p_routing_node2->conn_tos_) {
                    const auto i_routing_node3 = conn_pool_[i_nd2_conn3].i_to_rn_;
                    const auto& conn_way_id2 = conn_pool_[i_nd2_conn3].conn_way_id_;

                    // if back from original node, invalid
                    if (i_routing_node1 == i_routing_node3) {
                        continue;
                    }

                    CONN2_INDEX i_conn2 = conn2_pool_.AllocNewIndex();
                    TwoStepConnection* p_conn2 = conn2_pool_.ObjPtrByIndex(i_conn2);
                    p_conn2->i_from_rn_ = i_routing_node1;
                    p_conn2->i_mid_rn_ = conn_pool_[i_nd1_conn2].i_to_rn_;
                    p_conn2->i_to_rn_ = i_routing_node3;
                    p_conn2->weight_ = conn_pool_[i_nd1_conn2].weight_
                        + conn_pool_[i_nd2_conn3].weight_;
                    p_conn2->conn_way_id1_ = conn_way_id1;
                    p_conn2->conn_way_id2_ = conn_way_id2;

                    p_routing_node1->two_step_conn_tos_.push_back(i_conn2);
                }
            }

            // if multiple routes to the same destionation found, keep the shortest
            sort(p_routing_node1->two_step_conn_tos_.begin(),
                p_routing_node1->two_step_conn_tos_.end(),
                [this](const CONN2_INDEX& i, const CONN2_INDEX& j) {
                return conn2_pool_[i].weight_ < conn2_pool_[j].weight_;
            });

            if (shortest_mode_) {
                // same destination, through different point, only keep the shortest?
                int removal_count = 0;
                for (int i = (int)p_routing_node1->two_step_conn_tos_.size() - 1; i > 0; i--) {
                    auto p_conn2_i = conn2_pool_.ObjPtrByIndex(
                        p_routing_node1->two_step_conn_tos_[i]);
                    const NODE_ID_T& ith_to_node_id =
                        routing_node_pool_[p_conn2_i->i_to_rn_].p_node_->nd_id_;
                    for (int k = 0; k < i; ++k) {
                        auto i_conn2_k = p_routing_node1->two_step_conn_tos_[k];
                        if (ith_to_node_id == routing_node_pool_[
                            conn2_pool_[i_conn2_k].i_to_rn_].p_node_->nd_id_) {
                            p_routing_node1->two_step_conn_tos_[i] = 0;
                            ++removal_count;
                            break;
                        }
                    }
                }
                if (removal_count > 0) {
                    decltype(p_routing_node1->two_step_conn_tos_) two_step_conn_tos_new;
                    two_step_conn_tos_new.reserve(
                        p_routing_node1->two_step_conn_tos_.size() - removal_count);
                    for (auto& i_conn_to : p_routing_node1->two_step_conn_tos_) {
                        if (i_conn_to != 0) {
                            two_step_conn_tos_new.push_back(i_conn_to);
                        }
                    }
                    p_routing_node1->two_step_conn_tos_ = move(two_step_conn_tos_new);
                }
            }
        }

#if WAY_MANAGER_HANA_LOG == 1
        chrono::duration<double> elapsed = chrono::system_clock::now() - start;
        HANA_SDK_DEBUG(logger) << __FUNCTION__ << ": run time "
            << elapsed.count() << " seconds" << hana::endl;
#endif
        return true;
    }

    void InitSingleConnFourSteps(ROUTING_NODE_INDEX i_rn1)
    {
        RoutingNode* p_rn1 = routing_node_pool_.ObjPtrByIndex(i_rn1);
        auto& four_step_conn_tos = p_rn1->four_step_conn_tos_;
        four_step_conn_tos.reserve(p_rn1->two_step_conn_tos_.size() * 3);

        //        node1      node2     node3      node4      node5
        // -------->O--------->O-------->O---------->O--------->O---------->----------

        for (auto i_conn1to3 : p_rn1->two_step_conn_tos_) {
            const TwoStepConnection& conn1to3 = conn2_pool_[i_conn1to3];
            const ROUTING_NODE_INDEX i_rn2 = conn1to3.i_mid_rn_;
            const ROUTING_NODE_INDEX i_rn3 = conn1to3.i_to_rn_;

            const auto p_routing_node3 = routing_node_pool_.ObjPtrByIndex(i_rn3);
            for (auto i_conn3to5 : p_routing_node3->two_step_conn_tos_) {
                const TwoStepConnection& conn3to5 = conn2_pool_[i_conn3to5];
                const ROUTING_NODE_INDEX i_rn4 = conn3to5.i_mid_rn_;
                const ROUTING_NODE_INDEX i_rn5 = conn3to5.i_to_rn_;

                if (i_rn4 != i_rn1 && i_rn5 != i_rn1 && i_rn4 != i_rn2 && i_rn5 != i_rn2) {
                    CONN4_INDEX i_conn4 = conn4_pool_.AllocNewIndex();
                    FourStepConnection* p_conn4 = conn4_pool_.ObjPtrByIndex(i_conn4);
                    p_conn4->i_from_rn_ = i_rn1;
                    p_conn4->i_to_rn_ = i_rn5;

                    p_conn4->mid_rns_[0] = i_rn2;
                    p_conn4->mid_rns_[1] = i_rn3;
                    p_conn4->mid_rns_[2] = i_rn4;

                    p_conn4->conn_way_ids_[0] = conn1to3.conn_way_id1_;
                    p_conn4->conn_way_ids_[1] = conn1to3.conn_way_id2_;
                    p_conn4->conn_way_ids_[2] = conn3to5.conn_way_id1_;
                    p_conn4->conn_way_ids_[3] = conn3to5.conn_way_id2_;
                    p_conn4->weight_ = conn1to3.weight_ + conn3to5.weight_;

                    four_step_conn_tos.push_back(i_conn4);
                }
            }
        }

        for (auto& i_conn4 : four_step_conn_tos) {
            auto four_step_conn_to = conn4_pool_.ObjPtrByIndex(i_conn4);
            four_step_conn_to->hash_three_ = HashFirstThree(four_step_conn_to);
        }

        // sort by distance, group by each route's first 4 nodes (totally 5 nodes)
        sort(four_step_conn_tos.begin(), four_step_conn_tos.end(),
            [this](const CONN4_INDEX& i, const CONN4_INDEX& j) {
            auto& ci = conn4_pool_[i];
            auto& cj = conn4_pool_[j];
            if (ci.hash_three_ == cj.hash_three_) {
                return ci.weight_ < cj.weight_;
            }
            return ci.hash_three_ < cj.hash_three_;
        });

#if 0 // too slow, to remove the block?
        // if multiple routes to the same destionation found, only keep the shortest?
        int removal_count = 0;
        for (int i = (int)four_step_conn_tos.size() - 1; i > 0; i--) {
            const NODE_ID_T& ith_to_nd_id = four_step_conn_tos[i]->p_to_nd_->nd_id_;
            for (int k = 0; k < i; ++k) {
                if (ith_to_nd_id == four_step_conn_tos[k]->p_to_nd_->nd_id_) {
                    four_step_conn_tos[i].reset();
                    ++removal_count;
                    break;
                }
            }
        }
        if (removal_count > 0) {
            vector<unique_ptr<FourStepConnection>> four_step_conn_tos_new;
            four_step_conn_tos_new.reserve(four_step_conn_tos.size() - removal_count);
            for (auto& p_conn_to : four_step_conn_tos) {
                if (p_conn_to != nullptr) {
                    four_step_conn_tos_new.push_back(move(p_conn_to));
                }
            }
            four_step_conn_tos = move(four_step_conn_tos_new);
        }
#endif
    }

    uint64_t HashFirstThree(const FourStepConnection* p_conn4) const
    {
        uint64_t hash = (uint64_t)routing_node_pool_[p_conn4->mid_rns_[0]].p_node_->nd_id_
            + (uint64_t)routing_node_pool_[p_conn4->mid_rns_[1]].p_node_->nd_id_
            + (uint64_t)routing_node_pool_[p_conn4->mid_rns_[2]].p_node_->nd_id_
            + (uint64_t)p_conn4->conn_way_ids_[0]
            + (uint64_t)p_conn4->conn_way_ids_[1]
            + (uint64_t)p_conn4->conn_way_ids_[2];
        return hash;
    }

    uint64_t HashFirstFive(const SixStepConnection* p_conn6) const
    {
        uint64_t hash = (uint64_t)routing_node_pool_[p_conn6->mid_rns_[0]].p_node_->nd_id_
            + (uint64_t)routing_node_pool_[p_conn6->mid_rns_[1]].p_node_->nd_id_
            + (uint64_t)routing_node_pool_[p_conn6->mid_rns_[2]].p_node_->nd_id_
            + (uint64_t)routing_node_pool_[p_conn6->mid_rns_[3]].p_node_->nd_id_
            + (uint64_t)routing_node_pool_[p_conn6->mid_rns_[4]].p_node_->nd_id_
            + (uint64_t)p_conn6->conn_way_ids_[0]
            + (uint64_t)p_conn6->conn_way_ids_[1]
            + (uint64_t)p_conn6->conn_way_ids_[2]
            + (uint64_t)p_conn6->conn_way_ids_[3]
            + (uint64_t)p_conn6->conn_way_ids_[4];
        return hash;
    }

    bool InitConnsFourSteps()
    {
#if WAY_MANAGER_HANA_LOG == 1
        hana::Logger logger("WayManager");
        auto start = chrono::system_clock::now();
#endif
        // init four_step_conn_tos_ in each routing node
        for (auto& it : routing_node_map_) {
            auto& i_routing_node1 = it.second;
            InitSingleConnFourSteps(i_routing_node1);
        }

#if WAY_MANAGER_HANA_LOG == 1
        chrono::duration<double> elapsed = chrono::system_clock::now() - start;
        HANA_SDK_DEBUG(logger) << __FUNCTION__ << ": run time "
            << elapsed.count() << " seconds" << hana::endl;
#endif
        return true;
    }

    void InitSingleConnSixSteps(ROUTING_NODE_INDEX i_rn1)
    {
        RoutingNode *p_routing_node1 = routing_node_pool_.ObjPtrByIndex(i_rn1);
        auto& six_step_conn_tos = p_routing_node1->six_step_conn_tos_;
        six_step_conn_tos.reserve(p_routing_node1->four_step_conn_tos_.size() * 4);

        //      node1      node2     node3      node4      node5        node6       node7
        // ------>O--------->O-------->O---------->O--------->O---------->O---------->O---------->

        for (auto i_conn1to5 : p_routing_node1->four_step_conn_tos_) {
            const FourStepConnection& conn1to5 = conn4_pool_[i_conn1to5];
            const ROUTING_NODE_INDEX& i_rn2 = conn1to5.mid_rns_[0];
            const ROUTING_NODE_INDEX& i_rn3 = conn1to5.mid_rns_[1];
            const ROUTING_NODE_INDEX& i_rn4 = conn1to5.mid_rns_[2];
            const ROUTING_NODE_INDEX& i_rn5 = conn1to5.i_to_rn_;
            if (i_rn1 == i_rn5) {
                continue;
            }

            const auto p_routing_node5 = routing_node_pool_.ObjPtrByIndex(i_rn5);
            for (auto i_conn5to7 : p_routing_node5->two_step_conn_tos_) {
                const TwoStepConnection& conn5to7 = conn2_pool_[i_conn5to7];
                const ROUTING_NODE_INDEX& i_rn6 = conn5to7.i_mid_rn_;
                const ROUTING_NODE_INDEX& i_rn7 = conn5to7.i_to_rn_;

                if ((i_rn6 != i_rn1 && i_rn6 != i_rn2 && i_rn6 != i_rn3 && i_rn6 != i_rn4)
                    &&
                    (i_rn7 != i_rn1 && i_rn7 != i_rn2 && i_rn7 != i_rn3 && i_rn7 != i_rn4))
                {
                    CONN6_INDEX i_conn6 = conn6_pool_.AllocNewIndex();
                    SixStepConnection* p_conn6 = conn6_pool_.ObjPtrByIndex(i_conn6);
                    p_conn6->i_from_rn_ = i_rn1;
                    p_conn6->mid_rns_[0] = i_rn2;
                    p_conn6->mid_rns_[1] = i_rn3;
                    p_conn6->mid_rns_[2] = i_rn4;
                    p_conn6->mid_rns_[3] = i_rn5;
                    p_conn6->mid_rns_[4] = i_rn6;
                    p_conn6->i_to_rn_ = i_rn7;

                    p_conn6->conn_way_ids_[0] = conn1to5.conn_way_ids_[0];
                    p_conn6->conn_way_ids_[1] = conn1to5.conn_way_ids_[1];
                    p_conn6->conn_way_ids_[2] = conn1to5.conn_way_ids_[2];
                    p_conn6->conn_way_ids_[3] = conn1to5.conn_way_ids_[3];
                    p_conn6->conn_way_ids_[4] = conn5to7.conn_way_id1_;
                    p_conn6->conn_way_ids_[5] = conn5to7.conn_way_id2_;

                    p_conn6->weight_ = conn1to5.weight_ + conn5to7.weight_;

                    six_step_conn_tos.push_back(i_conn6);
                }
            }
        }

        for (auto& i_conn6 : six_step_conn_tos) {
            auto p_conn6 = conn6_pool_.ObjPtrByIndex(i_conn6);
            p_conn6->hash_five_ = HashFirstFive(p_conn6);
        }

        // sort, group by each route's first 6 nodes (totally 7 nodes)
        sort(six_step_conn_tos.begin(), six_step_conn_tos.end(),
            [this](const CONN6_INDEX& i, const CONN6_INDEX& j)
        {
            const auto& ci = conn6_pool_[i];
            const auto& cj = conn6_pool_[j];
            if (ci.hash_five_ == cj.hash_five_) {
                return ci.weight_ < cj.weight_;
            }
            return ci.hash_five_ < cj.hash_five_;
        });
    }

    bool InitConnsSixSteps()
    {
#if WAY_MANAGER_HANA_LOG == 1
        hana::Logger logger("WayManager");
        auto start = chrono::system_clock::now();
#endif
        for (auto& it1 : routing_node_map_) {
            auto& i_routing_node1 = it1.second;
            InitSingleConnSixSteps(i_routing_node1);
        }

#if WAY_MANAGER_HANA_LOG == 1
        chrono::duration<double> elapsed = chrono::system_clock::now() - start;
        HANA_SDK_DEBUG(logger) << __FUNCTION__ << ": run time "
            << elapsed.count() << " seconds" << hana::endl;
#endif
        return true;
    }

    // same way ID and same direction, from segment to segment
    bool RoutingSameOrientedWay(const Segment* p_seg1, const Segment* p_seg2,
        vector<SegmentPtr>& route) const
    {
        if (!p_seg1 || !p_seg2) {
            return false;
        }
        const OrientedWayPtr& p_way = p_seg1->GetWayOriented();
        if (!p_way) {
            return false;
        }
        auto it_1 = p_way->FindSegment(p_seg1->seg_id_);
        if (it_1 == p_way->Segments().end()) {
            return false;
        }
        auto it_2 = OrientedWay::FindSegment(it_1, p_way->Segments().end(), p_seg2->seg_id_);
        if (it_2 == p_way->Segments().end()) {
            return false;
        }

        route.insert(route.end(), it_1, it_2 + 1);
        return true;
    }

    // same way ID and same direction, from segment to node
    bool RoutingSameOrientedWay(const Segment* p_seg1, const Node* p_node2,
        vector<SegmentPtr>& route) const
    {
        if (!p_seg1 || !p_node2) {
            return false;
        }
        const OrientedWayPtr& p_way = p_seg1->GetWayOriented();
        if (!p_way) {
            return false;
        }
        auto it_1 = p_way->FindSegment(p_seg1->seg_id_);
        if (it_1 == p_way->Segments().end()) {
            return false;
        }
        auto it_2 = OrientedWay::FindSegmentByToNode(it_1, p_way->Segments().end(),
            p_node2->nd_id_);
        if (it_2 == p_way->Segments().end()) {
            return false;
        }

        route.insert(route.end(), it_1, it_2 + 1);
        return true;
    }

    // same way ID and same direction, from node to segment
    bool RoutingSameOrientedWay(const Node* p_node1, const Segment* p_seg2,
        vector<SegmentPtr>& route) const
    {
        if (!p_node1 || !p_seg2) {
            return false;
        }
        const auto& p_way = p_seg2->GetWayOriented();
        if (!p_way) {
            return false;
        }

        // find the iterator to "from" segment
        auto it_1 = p_way->Segments().begin();
        while (it_1 != p_way->Segments().end() &&
            (*it_1)->GetFromNode()->nd_id_ != p_node1->nd_id_) {
            ++it_1;
        }
        if (it_1 == p_way->Segments().end()) {
            return false;
        }

        // find the iterator to "to" segment
        auto it_2 = OrientedWay::FindSegment(it_1, p_way->Segments().end(), p_seg2->seg_id_);
        if (it_2 == p_way->Segments().end()) {
            return false;
        }

        route.insert(route.end(), it_1, it_2 + 1);
        return true;
    }

    // same way ID and same direction, from node to node
    bool RoutingSameOrientedWay(const OrientedWayPtr &p_way,
        const Node* p_node1, const Node* p_node2,
        vector<SegmentPtr>& route) const
    {
        if (!p_way || !p_node1 || !p_node2) {
            return false;
        }

        // find the iterator to "from" segment
        auto it_1 = p_way->Segments().begin();
        while (it_1 != p_way->Segments().end() &&
            (*it_1)->GetFromNode()->nd_id_ != p_node1->nd_id_) {
            ++it_1;
        }
        if (it_1 == p_way->Segments().end()) {
            return false;
        }

        return RoutingSameOrientedWay(*it_1, p_node2, route);
    }

    // same way ID and same direction, from node to node
    bool RoutingSameOrientedWay(WAY_ID_T oriented_way_id,
        const Node* p_node1, const Node* p_node2,
        vector<SegmentPtr>& route) const
    {
        OrientedWayPtr p_way = way_manager_.GetWayById(oriented_way_id);
        if (!p_way) {
            return false;
        }
        return RoutingSameOrientedWay(p_way, p_node1, p_node2, route);
    }

    // find the nearest routing node and find the indirectly connected way
    //              seg1            RN1         seg2            RN2
    //             ------>---------->O------->-------->--------->O---->
    //                   way1                   way2
    int AppendAllOneStepRoutes(const SegmentPtr& p_seg1, const SegmentPtr& p_seg2,
        const RoutingNode* p_routing_node1, const WAY_ID_T& signed_seg2_way_id,
        vector<route_info_tuple>& candidate_routes) const
    {
        int routes_added = 0;

        for (const auto& i_conn : p_routing_node1->conn_tos_) {
            auto& conn_to = conn_pool_[i_conn];
            if (conn_to.conn_way_id_ == signed_seg2_way_id) {
                route_info_tuple route_info;
                auto& p_conn_from_node = routing_node_pool_[conn_to.i_from_rn_].p_node_;

                if (false == RoutingSameOrientedWay(p_seg1, p_conn_from_node, route_info.route)) {
                    continue;
                }
                if (false == RoutingSameOrientedWay(p_conn_from_node, p_seg2, route_info.route)) {
                    continue;
                }
                route_info.length = WayManager::GetRouteLength(route_info.route);
                candidate_routes.push_back(move(route_info));
                ++routes_added;
            }
        }
        return routes_added;
    }


    // the case: check two steps (across two routing nodes)
    //              seg1            RN1                         RN2         seg2        RN3
    //             ------>---------->O------->-------->--------->O-------->------->----->O---->
    //                  way1                    way_m                       way2
    int AppendAllTwoStepRoutes(const SegmentPtr& p_seg1, const SegmentPtr& p_seg2,
        const RoutingNode* p_routing_node1, const WAY_ID_T& signed_seg2_way_id,
        vector<route_info_tuple>& candidate_routes) const
    {
        int routes_added = 0;
        for (const auto& i_conn2 : p_routing_node1->two_step_conn_tos_) {
            auto two_step_conn = conn2_pool_.ObjPtrByIndex(i_conn2);
            if (signed_seg2_way_id == two_step_conn->conn_way_id2_) {
                route_info_tuple route_info;
                auto& p_conn2_from_nd = routing_node_pool_[two_step_conn->i_from_rn_].p_node_;
                auto& p_conn2_mid_nd = routing_node_pool_[two_step_conn->i_mid_rn_].p_node_;

                if (false == RoutingSameOrientedWay(p_seg1, p_conn2_from_nd, route_info.route)) {
                    continue;
                }
                if (false == RoutingSameOrientedWay(two_step_conn->conn_way_id1_, p_conn2_from_nd,
                    p_conn2_mid_nd, route_info.route)) {
                    continue;
                }
                if (false == RoutingSameOrientedWay(p_conn2_mid_nd, p_seg2, route_info.route)) {
                    continue;
                }

                route_info.length = WayManager::GetRouteLength(route_info.route);
                candidate_routes.push_back(move(route_info));
                ++routes_added;
            }
        }

        // if not found the seg's way on the last connection, try outing ways
        if (routes_added == 0) {
            for (const auto& i_conn : p_routing_node1->conn_tos_) {
                auto conn_to = conn_pool_.ObjPtrByIndex(i_conn);
                auto const& to_nd_out_ways = RoutingNodeOutWays(conn_to->i_to_rn_);
                if (to_nd_out_ways.empty()) {
                    continue;
                }

                auto&& it_way_id = find(to_nd_out_ways.begin(), to_nd_out_ways.end(),
                    signed_seg2_way_id);
                if (it_way_id != to_nd_out_ways.end()) {
                    route_info_tuple route_info;
                    auto& p_conn_from_node = routing_node_pool_[conn_to->i_from_rn_].p_node_;
                    auto& p_conn_to_node = routing_node_pool_[conn_to->i_to_rn_].p_node_;

                    if (false == RoutingSameOrientedWay(p_seg1, p_conn_from_node,
                        route_info.route)) {
                        continue;
                    }
                    if (false == RoutingSameOrientedWay(conn_to->conn_way_id_, p_conn_from_node,
                        p_conn_to_node, route_info.route)) {
                        continue;
                    }
                    if (false == RoutingSameOrientedWay(p_conn_to_node, p_seg2,
                        route_info.route)) {
                        continue;
                    }

                    route_info.length = WayManager::GetRouteLength(route_info.route);
                    candidate_routes.push_back(move(route_info));
                    ++routes_added;
                }
            }
        }

        return routes_added;
    }


    // the case: check three-four steps (via 3 or 4 routing nodes)
    //  seg1           from_nd          mid_nds[0]           mid_nds[1]    mid_nds[2]  seg2  to_nd
    //  ------>---------->O----->--------->O-------->------->----->O-------->---->O------>------>O--->----->
    //        way1        conn_way_ids[0]     conn_way_ids[1]      conn_way_ids[2]       way2
    int AppendAllThreeStepRoutes(const SegmentPtr& p_seg1, const SegmentPtr& p_seg2,
        const RoutingNode* p_routing_node1, const WAY_ID_T& signed_seg2_way_id,
        vector<route_info_tuple>& candidate_routes) const
    {
        const auto& four_step_conns = p_routing_node1->four_step_conn_tos_;
        int routes_added = 0;

        for (size_t i = 0; i < four_step_conns.size(); ++i) {
            const auto four_step_conn = conn4_pool_.ObjPtrByIndex(four_step_conns[i]);

            // check the 1st three nodes are same as the previous one
            if (i != 0) {
                if (four_step_conn->hash_three_ ==
                    conn4_pool_[four_step_conns[i - 1]].hash_three_) {
                    continue;
                }
            }

            if (signed_seg2_way_id == four_step_conn->conn_way_ids_[2]) {
                route_info_tuple route_info;
                auto& conn4_from_rn = routing_node_pool_[four_step_conn->i_from_rn_];
                auto& conn4_mid_rn0 = routing_node_pool_[four_step_conn->mid_rns_[0]];
                auto& conn4_mid_rn1 = routing_node_pool_[four_step_conn->mid_rns_[1]];

                if (false == RoutingSameOrientedWay(p_seg1, conn4_from_rn.p_node_,
                    route_info.route)) {
                    continue;
                }
                if (false == RoutingSameOrientedWay(four_step_conn->conn_way_ids_[0],
                    conn4_from_rn.p_node_, conn4_mid_rn0.p_node_, route_info.route)) {
                    continue;
                }
                if (false == RoutingSameOrientedWay(four_step_conn->conn_way_ids_[1],
                    conn4_mid_rn0.p_node_, conn4_mid_rn1.p_node_, route_info.route)) {
                    continue;
                }
                if (false == RoutingSameOrientedWay(conn4_mid_rn1.p_node_, p_seg2,
                    route_info.route)) {
                    continue;
                }

                route_info.length = WayManager::GetRouteLength(route_info.route);
                candidate_routes.push_back(move(route_info));
                ++routes_added;
            }
        }

        // if not found the seg's way on the last connection, try outing ways
        if (routes_added == 0) {
            for (const auto i_conn2 : p_routing_node1->two_step_conn_tos_) {
                auto two_step_conn = conn2_pool_.ObjPtrByIndex(i_conn2);
                const auto& to_nd_out_ways = RoutingNodeOutWays(two_step_conn->i_to_rn_);
                if (to_nd_out_ways.empty()) {
                    continue;
                }

                auto&& it_way_id = std::find(to_nd_out_ways.begin(), to_nd_out_ways.end(),
                    signed_seg2_way_id);
                if (it_way_id != to_nd_out_ways.end()) {
                    route_info_tuple route_info;
                    auto& p_conn2_from_nd = routing_node_pool_[two_step_conn->i_from_rn_].p_node_;
                    auto& p_conn2_mid_nd = routing_node_pool_[two_step_conn->i_mid_rn_].p_node_;
                    auto& p_conn2_to_nd = routing_node_pool_[two_step_conn->i_to_rn_].p_node_;

                    if (false == RoutingSameOrientedWay(p_seg1, p_conn2_from_nd,
                        route_info.route)) {
                        continue;
                    }
                    if (false == RoutingSameOrientedWay(two_step_conn->conn_way_id1_,
                        p_conn2_from_nd,
                        p_conn2_mid_nd, route_info.route)) {
                        continue;
                    }
                    if (false == RoutingSameOrientedWay(two_step_conn->conn_way_id1_,
                        p_conn2_mid_nd,
                        p_conn2_to_nd, route_info.route)) {
                        continue;
                    }
                    if (false == RoutingSameOrientedWay(p_conn2_to_nd, p_seg2, route_info.route)) {
                        continue;
                    }

                    route_info.length = WayManager::GetRouteLength(route_info.route);
                    candidate_routes.push_back(move(route_info));
                    ++routes_added;
                }
            }
        }

        return routes_added;
    }

    int AppendAllFourStepRoutes(const SegmentPtr& p_seg1, const SegmentPtr& p_seg2,
        const RoutingNode* p_routing_node1, const WAY_ID_T& signed_seg2_way_id,
        vector<route_info_tuple>& candidate_routes) const
    {
        const auto& four_step_conns = p_routing_node1->four_step_conn_tos_;
        int routes_added = 0;

        for (size_t i = 0; i < four_step_conns.size(); ++i) {
            const auto four_step_conn = conn4_pool_.ObjPtrByIndex(four_step_conns[i]);

            if (signed_seg2_way_id == four_step_conn->conn_way_ids_[3]) {
                route_info_tuple route_info;
                auto& conn4_from_rn = routing_node_pool_[four_step_conn->i_from_rn_];
                auto& conn4_mid_rn0 = routing_node_pool_[four_step_conn->mid_rns_[0]];
                auto& conn4_mid_rn1 = routing_node_pool_[four_step_conn->mid_rns_[1]];
                auto& conn4_mid_rn2 = routing_node_pool_[four_step_conn->mid_rns_[2]];

                if (false == RoutingSameOrientedWay(p_seg1, conn4_from_rn.p_node_,
                    route_info.route)) {
                    continue;
                }
                if (false == RoutingSameOrientedWay(four_step_conn->conn_way_ids_[0],
                    conn4_from_rn.p_node_, conn4_mid_rn0.p_node_, route_info.route)) {
                    continue;
                }
                if (false == RoutingSameOrientedWay(four_step_conn->conn_way_ids_[1],
                    conn4_mid_rn0.p_node_, conn4_mid_rn1.p_node_, route_info.route)) {
                    continue;
                }
                if (false == RoutingSameOrientedWay(four_step_conn->conn_way_ids_[2],
                    conn4_mid_rn1.p_node_, conn4_mid_rn2.p_node_, route_info.route)) {
                    continue;
                }
                if (false == RoutingSameOrientedWay(conn4_mid_rn2.p_node_, p_seg2,
                    route_info.route)) {
                    continue;
                }

                route_info.length = WayManager::GetRouteLength(route_info.route);
                candidate_routes.push_back(move(route_info));
                ++routes_added;
            }
        }

        // if not found the seg's way on the last connection, try outing ways
        if (routes_added == 0) {
            for (size_t i = 0; i < four_step_conns.size(); ++i) {
                const auto four_step_conn = conn4_pool_.ObjPtrByIndex(four_step_conns[i]);

                // check the 1st three nodes are same as the previous one
                if (i != 0) {
                    if (four_step_conn->hash_three_ ==
                        conn4_pool_[four_step_conns[i - 1]].hash_three_) {
                        continue;
                    }
                }

                //    (p_from_nd_) (mid_rns_[0])       (mid_rns_[2])
                //          RN1        RN2       RN3       RN4                  p_to_nd_
                //  -------->O---------->O-------->O-------->O-------------------->O
                //    seg1    way_ids_[0]         way_ids_[2]          seg2

                const RoutingNode* p_routing_node4 = routing_node_pool_.ObjPtrByIndex(
                    four_step_conn->mid_rns_[2]);
                auto&& it_way_id = find(p_routing_node4->out_oriented_ways_.begin(),
                    p_routing_node4->out_oriented_ways_.end(), signed_seg2_way_id);

                if (it_way_id != p_routing_node4->out_oriented_ways_.end()) {
                    route_info_tuple route_info;
                    auto& conn4_from_rn = routing_node_pool_[four_step_conn->i_from_rn_];
                    auto& conn4_mid_rn0 = routing_node_pool_[four_step_conn->mid_rns_[0]];
                    auto& conn4_mid_rn1 = routing_node_pool_[four_step_conn->mid_rns_[1]];
                    auto& conn4_mid_rn2 = routing_node_pool_[four_step_conn->mid_rns_[2]];

                    if (false == RoutingSameOrientedWay(p_seg1, conn4_from_rn.p_node_,
                        route_info.route)) {
                        continue;
                    }
                    if (false == RoutingSameOrientedWay(four_step_conn->conn_way_ids_[0],
                        conn4_from_rn.p_node_, conn4_mid_rn0.p_node_, route_info.route)) {
                        continue;
                    }
                    if (false == RoutingSameOrientedWay(four_step_conn->conn_way_ids_[1],
                        conn4_mid_rn0.p_node_, conn4_mid_rn1.p_node_, route_info.route)) {
                        continue;
                    }
                    if (false == RoutingSameOrientedWay(four_step_conn->conn_way_ids_[2],
                        conn4_mid_rn1.p_node_, conn4_mid_rn2.p_node_, route_info.route)) {
                        continue;
                    }
                    if (false == RoutingSameOrientedWay(conn4_mid_rn2.p_node_, p_seg2,
                        route_info.route)) {
                        continue;
                    }

                    route_info.length = WayManager::GetRouteLength(route_info.route);
                    candidate_routes.push_back(move(route_info));
                    ++routes_added;
                }
            }
        }

        return routes_added;
    }

    int AppendAllFiveStepRoutes(const SegmentPtr& p_seg1, const SegmentPtr& p_seg2,
        const RoutingNode* p_routing_node1, const WAY_ID_T& signed_seg2_way_id,
        vector<route_info_tuple>& candidate_routes) const
    {
        const auto& six_step_conns = p_routing_node1->six_step_conn_tos_;
        int routes_added = 0;

        for (size_t i = 0; i < six_step_conns.size(); ++i) {
            const auto six_step_conn = conn6_pool_.ObjPtrByIndex(six_step_conns[i]);

            // check the 1st six nodes are same as the previous one
            if (i != 0) {
                if (six_step_conn->hash_five_ == conn6_pool_[six_step_conns[i - 1]].hash_five_) {
                    continue;
                }
            }

            if (signed_seg2_way_id == six_step_conn->conn_way_ids_[4]) {
                route_info_tuple route_info;
                auto& conn6_from_rn = routing_node_pool_[six_step_conn->i_from_rn_];
                auto& conn6_mid_rn0 = routing_node_pool_[six_step_conn->mid_rns_[0]];
                auto& conn6_mid_rn1 = routing_node_pool_[six_step_conn->mid_rns_[1]];
                auto& conn6_mid_rn2 = routing_node_pool_[six_step_conn->mid_rns_[2]];
                auto& conn6_mid_rn3 = routing_node_pool_[six_step_conn->mid_rns_[3]];

                if (false == RoutingSameOrientedWay(p_seg1, conn6_from_rn.p_node_,
                    route_info.route)) {
                    continue;
                }
                if (false == RoutingSameOrientedWay(six_step_conn->conn_way_ids_[0],
                    conn6_from_rn.p_node_, conn6_mid_rn0.p_node_, route_info.route)) {
                    continue;
                }
                if (false == RoutingSameOrientedWay(six_step_conn->conn_way_ids_[1],
                    conn6_mid_rn0.p_node_, conn6_mid_rn1.p_node_, route_info.route)) {
                    continue;
                }
                if (false == RoutingSameOrientedWay(six_step_conn->conn_way_ids_[2],
                    conn6_mid_rn1.p_node_, conn6_mid_rn2.p_node_, route_info.route)) {
                    continue;
                }
                if (false == RoutingSameOrientedWay(six_step_conn->conn_way_ids_[3],
                    conn6_mid_rn2.p_node_, conn6_mid_rn3.p_node_, route_info.route)) {
                    continue;
                }
                if (false == RoutingSameOrientedWay(conn6_mid_rn3.p_node_, p_seg2,
                    route_info.route)) {
                    continue;
                }

                route_info.length = WayManager::GetRouteLength(route_info.route);
                candidate_routes.push_back(move(route_info));
                ++routes_added;
            }
        }

        // if not found the seg's way on the last connection, try outing ways
        if (routes_added == 0) {
            for (const auto i_conn4 : p_routing_node1->four_step_conn_tos_) {
                auto four_step_conn = conn4_pool_.ObjPtrByIndex(i_conn4);
                const auto& to_nd_out_ways = RoutingNodeOutWays(four_step_conn->i_to_rn_);
                if (to_nd_out_ways.empty()) {
                    continue;
                }

                auto&& it_way_id = find(to_nd_out_ways.begin(), to_nd_out_ways.end(), signed_seg2_way_id);
                if (it_way_id != to_nd_out_ways.end()) {
                    route_info_tuple route_info;
                    auto& conn4_from_rn = routing_node_pool_[four_step_conn->i_from_rn_];
                    auto& conn4_to_rn = routing_node_pool_[four_step_conn->i_to_rn_];
                    auto& conn4_mid_rn0 = routing_node_pool_[four_step_conn->mid_rns_[0]];
                    auto& conn4_mid_rn1 = routing_node_pool_[four_step_conn->mid_rns_[1]];
                    auto& conn4_mid_rn2 = routing_node_pool_[four_step_conn->mid_rns_[2]];

                    if (false == RoutingSameOrientedWay(p_seg1, conn4_from_rn.p_node_,
                        route_info.route)) {
                        continue;
                    }
                    if (false == RoutingSameOrientedWay(four_step_conn->conn_way_ids_[0],
                        conn4_from_rn.p_node_, conn4_mid_rn0.p_node_, route_info.route)) {
                        continue;
                    }
                    if (false == RoutingSameOrientedWay(four_step_conn->conn_way_ids_[1],
                        conn4_mid_rn0.p_node_, conn4_mid_rn1.p_node_, route_info.route)) {
                        continue;
                    }
                    if (false == RoutingSameOrientedWay(four_step_conn->conn_way_ids_[2],
                        conn4_mid_rn1.p_node_, conn4_mid_rn2.p_node_, route_info.route)) {
                        continue;
                    }
                    if (false == RoutingSameOrientedWay(four_step_conn->conn_way_ids_[3],
                        conn4_mid_rn2.p_node_, conn4_to_rn.p_node_, route_info.route)) {
                        continue;
                    }
                    if (false == RoutingSameOrientedWay(conn4_to_rn.p_node_, p_seg2,
                        route_info.route)) {
                        continue;
                    }

                    route_info.length = WayManager::GetRouteLength(route_info.route);
                    candidate_routes.push_back(move(route_info));
                    ++routes_added;
                }
            }
        }

        return routes_added;
    }

    int AppendAllSixStepRoutes(const SegmentPtr& p_seg1, const SegmentPtr& p_seg2,
        const RoutingNode* p_routing_node1, const WAY_ID_T& signed_seg2_way_id,
        vector<route_info_tuple>& candidate_routes) const
    {
        const auto& six_step_conn_tos = p_routing_node1->six_step_conn_tos_;
        int routes_added = 0;

        for (size_t i = 0; i < six_step_conn_tos.size(); ++i) {
            const auto six_step_conn = conn6_pool_.ObjPtrByIndex(six_step_conn_tos[i]);

            if (signed_seg2_way_id == six_step_conn->conn_way_ids_[5]) {
                route_info_tuple route_info;
                route_info.route.reserve(64);

                auto& conn6_from_rn = routing_node_pool_[six_step_conn->i_from_rn_];
                auto& conn6_mid_rn0 = routing_node_pool_[six_step_conn->mid_rns_[0]];
                auto& conn6_mid_rn1 = routing_node_pool_[six_step_conn->mid_rns_[1]];
                auto& conn6_mid_rn2 = routing_node_pool_[six_step_conn->mid_rns_[2]];
                auto& conn6_mid_rn3 = routing_node_pool_[six_step_conn->mid_rns_[3]];
                auto& conn6_mid_rn4 = routing_node_pool_[six_step_conn->mid_rns_[4]];

                if (false == RoutingSameOrientedWay(p_seg1, conn6_from_rn.p_node_,
                    route_info.route)) {
                    continue;
                }
                if (false == RoutingSameOrientedWay(six_step_conn->conn_way_ids_[0],
                    conn6_from_rn.p_node_, conn6_mid_rn0.p_node_, route_info.route)) {
                    continue;
                }
                if (false == RoutingSameOrientedWay(six_step_conn->conn_way_ids_[1],
                    conn6_mid_rn0.p_node_, conn6_mid_rn1.p_node_, route_info.route)) {
                    continue;
                }
                if (false == RoutingSameOrientedWay(six_step_conn->conn_way_ids_[2],
                    conn6_mid_rn1.p_node_, conn6_mid_rn2.p_node_, route_info.route)) {
                    continue;
                }
                if (false == RoutingSameOrientedWay(six_step_conn->conn_way_ids_[3],
                    conn6_mid_rn2.p_node_, conn6_mid_rn3.p_node_, route_info.route)) {
                    continue;
                }
                if (false == RoutingSameOrientedWay(six_step_conn->conn_way_ids_[4],
                    conn6_mid_rn3.p_node_, conn6_mid_rn4.p_node_, route_info.route)) {
                    continue;
                }
                if (false == RoutingSameOrientedWay(conn6_mid_rn4.p_node_, p_seg2,
                    route_info.route)) {
                    continue;
                }

                route_info.length = WayManager::GetRouteLength(route_info.route);
                candidate_routes.push_back(move(route_info));
                ++routes_added;
            }
        }

        return routes_added;
    }

    int AppendAllSevenStepRoutes(const SegmentPtr& p_seg1, const SegmentPtr& p_seg2,
        const RoutingNode* p_routing_node1, const WAY_ID_T& signed_seg2_way_id,
        vector<route_info_tuple>& candidate_routes) const
    {
        int routes_added = 0;

        for (size_t i = 0; i < p_routing_node1->six_step_conn_tos_.size(); ++i) {
            const auto six_step_conn = conn6_pool_.ObjPtrByIndex(
                p_routing_node1->six_step_conn_tos_[i]);
            const auto& to_nd_out_ways = RoutingNodeOutWays(six_step_conn->i_to_rn_);
            if (to_nd_out_ways.empty()) {
                continue;
            }

            auto&& it_way_id = find(to_nd_out_ways.begin(), to_nd_out_ways.end(),
                signed_seg2_way_id);
            if (it_way_id != to_nd_out_ways.end()) {
                route_info_tuple route_info;
                auto& conn6_from_rn = routing_node_pool_[six_step_conn->i_from_rn_];
                auto& conn6_to_rn = routing_node_pool_[six_step_conn->i_to_rn_];
                auto& conn6_mid_rn0 = routing_node_pool_[six_step_conn->mid_rns_[0]];
                auto& conn6_mid_rn1 = routing_node_pool_[six_step_conn->mid_rns_[1]];
                auto& conn6_mid_rn2 = routing_node_pool_[six_step_conn->mid_rns_[2]];
                auto& conn6_mid_rn3 = routing_node_pool_[six_step_conn->mid_rns_[3]];
                auto& conn6_mid_rn4 = routing_node_pool_[six_step_conn->mid_rns_[4]];

                if (false == RoutingSameOrientedWay(p_seg1, conn6_from_rn.p_node_,
                    route_info.route)) {
                    continue;
                }
                if (false == RoutingSameOrientedWay(six_step_conn->conn_way_ids_[0],
                    conn6_from_rn.p_node_, conn6_mid_rn0.p_node_, route_info.route)) {
                    continue;
                }
                if (false == RoutingSameOrientedWay(six_step_conn->conn_way_ids_[1],
                    conn6_mid_rn0.p_node_, conn6_mid_rn1.p_node_, route_info.route)) {
                    continue;
                }
                if (false == RoutingSameOrientedWay(six_step_conn->conn_way_ids_[2],
                    conn6_mid_rn1.p_node_, conn6_mid_rn2.p_node_, route_info.route)) {
                    continue;
                }
                if (false == RoutingSameOrientedWay(six_step_conn->conn_way_ids_[3],
                    conn6_mid_rn2.p_node_, conn6_mid_rn3.p_node_, route_info.route)) {
                    continue;
                }
                if (false == RoutingSameOrientedWay(six_step_conn->conn_way_ids_[4],
                    conn6_mid_rn3.p_node_, conn6_mid_rn4.p_node_, route_info.route)) {
                    continue;
                }
                if (false == RoutingSameOrientedWay(six_step_conn->conn_way_ids_[5],
                    conn6_mid_rn4.p_node_, conn6_to_rn.p_node_, route_info.route)) {
                    continue;
                }
                if (false == RoutingSameOrientedWay(conn6_to_rn.p_node_, p_seg2,
                    route_info.route)) {
                    continue;
                }

                route_info.length = WayManager::GetRouteLength(route_info.route);
                candidate_routes.push_back(move(route_info));
                ++routes_added;
            }
        }

        return routes_added;
    }

private:
    const vector<WAY_ID_T>& RoutingNodeOutWays(ROUTING_NODE_INDEX i_rn) const
    {
        return routing_node_pool_[i_rn].out_oriented_ways_;
    }

public:
    bool DijkstraShortestPath(const SegmentPtr& p_seg1, const SegmentPtr& p_seg2,
        vector<SegmentPtr>& route, int *seach_steps, time_t time_point, bool is_localtime,
        bool points_reversed) const
    {
        //                              (source)        (destination)
        //               seg1              RN1              RN2             seg2
        //    --------->--------->--------->O----> ... ----->O------>------>----->---->
        //
        route.clear();
        if (p_seg1 == p_seg2 || p_seg1->seg_id_ == p_seg2->seg_id_) {
            if (seach_steps) {
                *seach_steps = 0;
            }
            bool ok = RoutingNearby(p_seg1, p_seg2, route, false, time_point, is_localtime,
                points_reversed);
            if (ok) return true;
        }
        else if (p_seg1->GetWayOriented() == p_seg2->GetWayOriented()) {
            auto seg_pos_in_way = [](const SegmentPtr p_seg) {
                auto& segs = p_seg->GetWayOriented()->Segments();
                for (size_t i = 0; i < segs.size(); ++i) {
                    if (segs[i] == p_seg) {
                        return i;
                    }
                }
                return segs.size(); // shall never reach here
            };
            if (seg_pos_in_way(p_seg1) < seg_pos_in_way(p_seg2)) {
                // same way, seg2 is ahead of seg1
                if (seach_steps) {
                    *seach_steps = 0;
                }
                return RoutingNearby(p_seg1, p_seg2, route, false, time_point,
                    is_localtime, false);
            }
        }

        ROUTING_NODE_INDEX i_rn1 = GetSrcRoutingNode(p_seg1);
        if (0 == i_rn1) {
            return false;
        }
        ROUTING_NODE_INDEX i_rn2 = GetDstRoutingNode(p_seg2);
        if (0 == i_rn2) {
            return false;
        }
        if (routing_node_pool_[i_rn1].p_node_->IsWeakConnected() !=
            routing_node_pool_[i_rn2].p_node_->IsWeakConnected()) {
            return false;
        }

        auto routing_nodes_path = CALL_DIJKSTRA(i_rn1, i_rn2, seach_steps, time_point,
            is_localtime);
        if (routing_nodes_path.empty()) {
            return false;
        }

        // estimated capacity
        route.reserve((routing_nodes_path.size() + 2) * 6);

        // from seg1 to first routing node
        RoutingSameOrientedWay(p_seg1, routing_node_pool_[i_rn1].p_node_, route);

        // first routing node to the last routing node
        for (size_t i = 0; i < routing_nodes_path.size() - 1; ++i) {
            const auto& p_n1 = routing_nodes_path[i];
            const auto& p_n2 = routing_nodes_path[i + 1];
            bool found = false;
            for (const auto& i_conn : p_n1->conn_tos_) {
                auto p_conn = conn_pool_.ObjPtrByIndex(i_conn);
                auto& p_conn_from_node = routing_node_pool_[p_conn->i_from_rn_].p_node_;
                auto& p_conn_to_node = routing_node_pool_[p_conn->i_to_rn_].p_node_;

                if (p_conn_to_node == p_n2->p_node_) {
                    RoutingSameOrientedWay(p_conn->conn_way_id_, p_conn_from_node, p_conn_to_node,
                        route);
                    found = true;
                    break;
                }
            }
            if (!found) {
                route.clear();
                return false;
            }
        }

        // the last routing node to seg2
        RoutingSameOrientedWay(routing_node_pool_[i_rn2].p_node_, p_seg2, route);
        return true;
    }

    bool DijkstraShortestEdgePath(const SegmentPtr& p_seg1, const SegmentPtr& p_seg2,
        vector<EDGE_ID_T>& route) const
    {
        //                              (source)        (destination)
        //               seg1              RN1              RN2             seg2
        //    --------->--------->--------->O----> ... ----->O------>------>----->---->
        //
        route.clear();

        if (p_seg1 == p_seg2 || p_seg1->seg_id_ == p_seg2->seg_id_) {
            auto edge1 = SegmentToEdge(p_seg1);
            route.push_back(edge1);
            return true;
        }
        else if (p_seg1->GetWayOriented() == p_seg2->GetWayOriented()) {
            auto seg_pos_in_way = [](const SegmentPtr p_seg) {
                auto& segs = p_seg->GetWayOriented()->Segments();
                for (size_t i = 0; i < segs.size(); ++i) {
                    if (segs[i] == p_seg) {
                        return i;
                    }
                }
                return (size_t)-1; // shall never reach here
            };

            auto i_seg1 = seg_pos_in_way(p_seg1);
            auto i_seg2 = seg_pos_in_way(p_seg2);
            if (i_seg1 < i_seg2 && i_seg2 != (size_t)-1) { // same way, seg2 is ahead of seg1
                vector<SegmentPtr> seg_route;
                route.reserve(i_seg2 - i_seg1 + 1);
                if (false == RoutingNearby(p_seg1, p_seg2, seg_route, false, 0, false, false)) {
                    return false;
                }
                route.push_back(SegmentToEdge(p_seg1));
                for (size_t i = 1; i < seg_route.size(); ++i) {
                    auto edge_id = SegmentToEdge(p_seg1);
                    if (edge_id != route.back()) {
                        route.push_back(edge_id);
                    }
                }
                return true;
            }
        }

        ROUTING_NODE_INDEX i_rn1 = GetSrcRoutingNode(p_seg1);
        if (0 == i_rn1) {
            return false;
        }
        ROUTING_NODE_INDEX i_rn2 = GetDstRoutingNode(p_seg2);
        if (0 == i_rn2) {
            return false;
        }
        if (routing_node_pool_[i_rn1].p_node_->IsWeakConnected() !=
            routing_node_pool_[i_rn2].p_node_->IsWeakConnected()) {
            return false;
        }

        auto routing_nodes_path = CALL_DIJKSTRA(i_rn1, i_rn2, nullptr, 0, false);
        if (routing_nodes_path.empty()) {
            return false;
        }

        // estimated capacity
        route.reserve(routing_nodes_path.size() + 2);

        // 1st edge
        auto edge1 = SegmentToEdge(p_seg1);
        if (edge1 == 0) {
            return false;
        }
        route.push_back(edge1);

        // first routing node to the last routing node
        for (size_t i = 0; i < routing_nodes_path.size() - 1; ++i) {
            const auto& p_n1 = routing_nodes_path[i];
            const auto& p_n2 = routing_nodes_path[i + 1];

            EDGE_ID_T edge_id = 0;
            for (const auto& i_conn : p_n1->conn_tos_) {
                auto p_conn = conn_pool_.ObjPtrByIndex(i_conn);
                auto& p_conn_from_node = routing_node_pool_[p_conn->i_from_rn_].p_node_;
                auto& p_conn_to_node = routing_node_pool_[p_conn->i_to_rn_].p_node_;

                if (p_conn_to_node == p_n2->p_node_) {
                    for (auto& conn_seg : p_conn_from_node->ConnectedSegments()) {
                        if (conn_seg->way_id_ == p_conn->conn_way_id_ &&
                            conn_seg->from_nd_ == p_conn_from_node->nd_id_) {
                            edge_id = conn_seg->seg_id_;
                            break;
                        }
                    }
                    if (edge_id) break;
                }
            }
            if (!edge_id) {
                route.clear();
                return false;
            }
            route.push_back(edge_id);
        }

        // last edge
        auto edge2 = SegmentToEdge(p_seg2);
        if (edge2 == 0) {
            route.clear();
            return false;
        }
        route.push_back(edge2);

        return true;
    }

    bool ViaRoute(const ViaRouteParams& params, ViaRouteResult& result)
    {
        result.Clear();

        chrono::system_clock::time_point start;
        if (params.calc_used_time) {
            start = chrono::system_clock::now();
        }
        AT_SCOPE_EXIT(if (params.calc_used_time) {
            chrono::duration<double> elapsed = chrono::system_clock::now() - start;
            result.used_time = elapsed.count();
        });

        if (params.via_points.size() < 2) {
            result.err = "too few points";
            return false;
        }
        vector<SegmentPtr> via_segs;
        via_segs.reserve(params.via_points.size());

        // segment assignment
        SegAssignParams assign_params;
        assign_params.radius = params.radius;
        assign_params.angle_tollerance = params.angle_tollerance;
        assign_params.check_no_gps_route = params.check_no_gps_route;
        assign_params.dev_data_time = params.dev_data_time;
        assign_params.dev_data_local_time = params.dev_data_local_time;

        for (auto& point : params.via_points) {
            assign_params.heading = point.heading;
            auto p_seg = way_manager_.AssignSegment(point.geo_point, assign_params);
            if (p_seg == nullptr) {
                char err_buff[256];
                snprintf(err_buff, sizeof(err_buff),
                    "point assignment failed. Point: (%.6f, %.6f), heading = %d",
                    point.geo_point.lat, point.geo_point.lng, point.heading);
                result.err = err_buff;
                return false;
            }
            via_segs.push_back(p_seg);
        }

        result.search_steps = 0;
        vector<SegmentPtr> route_section;
        for (size_t i = 0; i < via_segs.size() - 1; ++i) {
            const auto& p_seg1 = via_segs[i];
            const auto& p_seg2 = via_segs[i + 1];
            double direct_dist;
            bool points_reversed = false;

            if (p_seg1 == p_seg2) {
                auto proj_point1 = geo::get_projection_point(params.via_points[i].geo_point,
                    p_seg1->from_point_, p_seg1->to_point_);
                auto proj_point2 = geo::get_projection_point(params.via_points[i + 1].geo_point,
                    p_seg1->from_point_, p_seg1->to_point_);
                double dist_proj1 = geo::distance_in_meter(p_seg1->from_point_, proj_point1);
                double dist_proj2 = geo::distance_in_meter(p_seg1->from_point_, proj_point2);
                direct_dist = std::abs(dist_proj1 - dist_proj2);

                if (dist_proj1 > dist_proj2 + 10.0) { // 10 meters are the exprence GPS distance error
                    points_reversed = true;
                }
            }
            else {
                direct_dist = geo::distance_in_meter(params.via_points[i].geo_point,
                    params.via_points[i + 1].geo_point);
            }

            bool ok = false;
            int steps = 0;
            // experence distance threshold, with engough margins. typical the range is more than 2500
            if (direct_dist < 1200) {
                ok = this->RoutingNearby(p_seg1, p_seg2, route_section, false,
                    params.dev_data_time, params.dev_data_local_time, points_reversed);
                if (!ok) {
                    ok = this->DijkstraShortestPath(p_seg1, p_seg2, route_section, &steps,
                        params.dev_data_time, params.dev_data_local_time, points_reversed);
                    result.search_steps += steps;
                }
            }
            else {
                ok = this->DijkstraShortestPath(p_seg1, p_seg2, route_section, &steps,
                    params.dev_data_time, params.dev_data_local_time, points_reversed);
                result.search_steps += steps;
            }
            if (!ok) {
                char err_buff[256];
                snprintf(err_buff, sizeof(err_buff),
                    "failed to find route from (%.6lf, %.6lf), segment %lld to (%.6lf, %.6lf), segment %lld",
                    params.via_points[i].geo_point.lat, params.via_points[i].geo_point.lng,
                    p_seg1->seg_id_,
                    params.via_points[i + 1].geo_point.lat, params.via_points[i + 1].geo_point.lng,
                    p_seg2->seg_id_);
                result.err = err_buff;
                return false;
            }

            result.seg_route.insert(result.seg_route.end(),
                (i == 0) ? route_section.begin() : route_section.begin() + 1,
                route_section.end());
        }

        return true;
    }

    // returns the nodes including source and destination nodes
    vector<RoutingNode*> Dijkstra(ROUTING_NODE_INDEX i_rn1, ROUTING_NODE_INDEX i_rn2,
        int *seach_steps, time_t time_point, bool is_localtime) const
    {
#if DIJKSTRA_STATISTICS == 1
        auto t_start = util::GetTimeInMs64();
#endif
        using namespace dijkstra;
        vector<RoutingNode*> result;
        int max_node_count = (int)routing_node_map_.size();

        BinHeap fwd_heap(routing_node_pool_, max_node_count);
        auto& node_pool = fwd_heap.NodePool();

        fwd_heap.Insert(i_rn1, 0, 0);

        bool success = false;
        vector<HeapData> pairs;;

        fwd_heap.search_steps_ = 0;
        while (!fwd_heap.Empty()) {
            HeapData min_heap_node = fwd_heap.DeleteMin();
            NodeData& min_node = node_pool[min_heap_node.pool_index_];

            ++fwd_heap.search_steps_;

            min_node.finished = true;
            if (min_node.i_rn == i_rn2) {
                success = true;
                break;
            }

            pairs.clear();
            for (auto i_conn : routing_node_pool_[min_node.i_rn].conn_tos_) {
                const auto& edge = conn_pool_[i_conn];
                const auto& i_to_rn = edge.i_to_rn_;

                // if the some segs are excluded, e.g., closed tunnel in the midnight
                if (time_point != 0 && edge.excluded_flag_) {
                    bool is_excluded = false;
                    for (const auto &p_seg : edge.segs_) {
                        if (p_seg->excluded_flag_) {
                            if (p_seg->excluded_always_ ||
                                this->way_manager_.IsSegmentExcluded(p_seg, time_point,
                                    is_localtime)) {
                                is_excluded = true;
                                break;
                            }
                        }
                    }
                    if (is_excluded) {
                        continue;
                    }
                }

                NODE_DATA_INDEX i_to_node_data = fwd_heap.GetIfInserted(i_to_rn);
                if (i_to_node_data != 0 && node_pool[i_to_node_data].finished) {
                    continue;
                }

                // make sure the "to" node is already in working queue
                // do not do insertion if was inserted before
                auto e_weight = edge.weight_;
                if (i_to_node_data == 0) {
                    i_to_node_data = fwd_heap.Insert(i_to_rn, min_node.i_rn,
                        min_node.distance + e_weight);
                }

                // get all the keys to be decreased later
                if (min_node.distance + e_weight < node_pool[i_to_node_data].distance) {
                    pairs.emplace_back(i_to_node_data, min_node.distance + e_weight);
                    // also update the parent node
                    node_pool[i_to_node_data].i_pre_rn = min_node.i_rn;
                }
            }

            if (!pairs.empty()) {
                fwd_heap.DecreaseKeys(pairs);
            }
        }
#if (ROUTING_STEPS_TO_JSON == 1)
        VisitedToJson(fwd_heap, 5,
            "data/dijkstra_step_" + std::to_string(fwd_heap.search_steps_) + ".json");
#endif

        if (success) {
            // get the path
            std::vector<ROUTING_NODE_INDEX> rn_path;
            ROUTING_NODE_INDEX i_rn = i_rn2;
            while (i_rn != 0) {
                rn_path.emplace_back(i_rn);
                i_rn = fwd_heap.GetPreNode(i_rn);
            }

            // reverse the order
            for (size_t i = 0; i < rn_path.size() / 2; ++i) {
                std::swap(rn_path[i], rn_path[rn_path.size() - 1 - i]);
            }

            result.reserve(rn_path.size());
            for (auto i_rn : rn_path) {
                result.push_back(const_cast<RoutingNode*>(routing_node_pool_.ObjPtrByIndex(i_rn)));
            }
        }

        if (seach_steps) {
            *seach_steps = fwd_heap.search_steps_;
        }

#if DIJKSTRA_STATISTICS == 1
        auto t_end = util::GetTimeInMs64();
        ++dijkstra_calls_num_;
        dijkstra_total_time_ += t_end - t_start;
        if (success) {
            ++dijkstra_calls_success_num_;
            dijkstra_success_time_ += t_end - t_start;
        }
#endif

        return result;
    }

#if DIJKSTRA_STATISTICS == 1
    mutable std::atomic_int dijkstra_calls_num_, dijkstra_calls_success_num_;
    mutable std::atomic_llong dijkstra_total_time_, dijkstra_success_time_;
#endif

    // bi-directional version
    // returns the nodes including source and destination nodes
    vector<RoutingNode*> DijkstraBiDir(ROUTING_NODE_INDEX i_rn1, ROUTING_NODE_INDEX i_rn2,
        int *seach_steps, time_t time_point, bool is_localtime) const
    {
        using namespace dijkstra;
        vector<RoutingNode*> result;
        const int max_node_count = (int)routing_node_map_.size();

        // forward
        BinHeap fwd_heap(routing_node_pool_, max_node_count);
        fwd_heap.Insert(i_rn1, 0, 0);

        // backward
        BinHeap rev_heap(routing_node_pool_, max_node_count);
        rev_heap.Insert(i_rn2, 0, 0);

        ROUTING_NODE_INDEX mid_rn = 0;
        int upper_bound = INT_MAX;
        vector<HeapData> pairs; // declared here to avoid repeating memory allocations

#if (ROUTING_STEPS_TO_JSON == 1)
        int steps = 0;
#endif
        while (!fwd_heap.Empty() || !rev_heap.Empty()) {
            if (!fwd_heap.Empty()) {
                ++fwd_heap.search_steps_;
                StepForward(fwd_heap, rev_heap, mid_rn, upper_bound, pairs,
                    time_point, is_localtime);
            }
            if (!rev_heap.Empty()) {
                ++rev_heap.search_steps_;
                StepBackward(rev_heap, fwd_heap, mid_rn, upper_bound, pairs,
                    time_point, is_localtime);
            }

#if (ROUTING_STEPS_TO_JSON == 1)
            ++steps;
#endif
        }
#if (ROUTING_STEPS_TO_JSON == 1)
        VisitedToJson(fwd_heap, 5, "data/bidir_fwd_step_" + std::to_string(steps) + ".json");
        VisitedToJson(rev_heap, 5, "data/bidir_rev_step_" + std::to_string(steps) + ".json");
#endif
        if (seach_steps) {
            *seach_steps = fwd_heap.search_steps_ + rev_heap.search_steps_;
        }

        if (INT_MAX != upper_bound) {
            // get the path part 1 - from source to middle
            std::vector<ROUTING_NODE_INDEX> rn_path;
            rn_path.reserve(16);
            ROUTING_NODE_INDEX i_rn = mid_rn;
            while (i_rn != 0) {
                rn_path.emplace_back(i_rn);
                i_rn = fwd_heap.GetPreNode(i_rn);
            }

            // reverse the order
            for (size_t i = 0; i < rn_path.size() / 2; ++i) {
                std::swap(rn_path[i], rn_path[rn_path.size() - 1 - i]);
            }

            // then the part 2 - from middle to target
            rn_path.reserve(rn_path.size() * 3); // 3 instead of 2 to more reservations
            i_rn = rev_heap.GetPreNode(mid_rn);
            while (i_rn != 0) {
                rn_path.emplace_back(i_rn);
                i_rn = rev_heap.GetPreNode(i_rn);
            }

            // convert to RoutingNode pointers
            result.reserve(rn_path.size());
            for (auto i_rn : rn_path) {
                result.push_back(const_cast<RoutingNode*>(routing_node_pool_.ObjPtrByIndex(i_rn)));
            }
        }

        return result;
    }

    // debug function
    bool VisitedToJson(dijkstra::BinHeap& bin_heap, double offset,
        const std::string& pathname) const
    {
        using namespace dijkstra;

        geo::GeoJSON geo_json;
        UNORD_SET<CONN_INDEX> conns;

        geo_json.Reserve(bin_heap.MaxNodeCount());
        conns.reserve(bin_heap.MaxNodeCount() * 4);

        // populate conns set
        for (int i = 1; i <= bin_heap.MaxNodeCount(); ++i) {
            NODE_DATA_INDEX i_node_data = bin_heap.inserted_[i];
            if (INVALID_NODE_INDEX == i_node_data) {
                continue;
            }
            const auto& node_data = bin_heap.pool_[i_node_data];
            const auto& routing_node = routing_node_pool_[node_data.i_rn];

            for (auto& i_conn : routing_node.conn_froms_) {
                conns.insert(i_conn);
            }
            for (auto& i_conn : routing_node.conn_tos_) {
                conns.insert(i_conn);
            }
        }

        // all related edges => geo_json
        vector<SegmentPtr> edge_route;
        for (auto& i_conn : conns) {
            auto& edge = conn_pool_[i_conn];

            edge_route.clear();
            if (false == RoutingSameOrientedWay(edge.conn_way_id_,
                routing_node_pool_[edge.i_from_rn_].p_node_,
                routing_node_pool_[edge.i_to_rn_].p_node_, edge_route)) {
                return false;
            }
            auto p_line = std::make_shared<GeoObj_LineString>();
            double length = 0;

            for (auto p_seg : edge_route) {
                if (p_seg->one_way_) {
                    if (p_seg == edge_route.front()) {
                        p_line->AddPoint(p_seg->from_point_);
                    }
                    p_line->AddPoint(p_seg->to_point_);
                }
                else {
                    GeoPoint off_from, off_to;
                    geo::get_offset_segment(p_seg->from_point_, p_seg->to_point_, offset,
                        off_from, off_to);
                    if (p_seg == edge_route.front()) {
                        p_line->AddPoint(off_from);
                    }
                    p_line->AddPoint(off_to);
                }
                length += p_seg->length_;
            }
            p_line->AddProp("edge_id", edge_route.front()->seg_id_);
            p_line->AddProp("length", length);
            p_line->AddProp("weight", edge.weight_);

            string color = "green";
            auto i_from_node_data = bin_heap.GetIfInserted(edge.i_from_rn_);
            auto i_to_node_data = bin_heap.GetIfInserted(edge.i_to_rn_);
            if (i_from_node_data != INVALID_NODE_INDEX && i_to_node_data != INVALID_NODE_INDEX) {
                if (bin_heap.pool_[i_from_node_data].finished &&
                    bin_heap.pool_[i_to_node_data].finished) {
                    color = "orange";
                }
            }
            p_line->AddProp("color", color);
            geo_json.AddObj(move(p_line));
        }

        // all visited nodes => geo_json
        for (int i = 1; i <= bin_heap.MaxNodeCount(); ++i) {
            NODE_DATA_INDEX i_node_data = bin_heap.inserted_[i];
            if (INVALID_NODE_INDEX == i_node_data) {
                continue;
            }
            const auto& node_data = bin_heap.pool_[i_node_data];
            const auto& routing_node = routing_node_pool_[node_data.i_rn];

            auto p_node = std::make_shared<GeoObj_Point>(routing_node.p_node_->geo_point_);
            p_node->AddProp("node_id", routing_node.p_node_->nd_id_);
            p_node->AddProp("distance", node_data.distance);
            p_node->AddProp("color", node_data.finished ? "red" : "blue");
            geo_json.AddObj(move(p_node));
        }

        return geo_json.ToJsonFile(pathname);
    }

private:
    void StepForward(dijkstra::BinHeap& fwd_heap, dijkstra::BinHeap& rev_heap,
        ROUTING_NODE_INDEX& mid_rn, int& upper_bound,
        vector<dijkstra::HeapData>& pairs,
        time_t time_point, bool is_localtime) const
    {
        using namespace dijkstra;

        auto& fwd_pool = fwd_heap.NodePool();
        HeapData min_heap_node = fwd_heap.DeleteMin();
        NodeData& min_node = fwd_pool[min_heap_node.pool_index_];

        // check if overlapped with reversed heap
        auto i_min_node_in_rev = rev_heap.GetIfInserted(min_node.i_rn);
        if (i_min_node_in_rev != 0) {
            auto& rev_pool = rev_heap.NodePool();
            int new_distance = min_node.distance + rev_pool[i_min_node_in_rev].distance;
            if (new_distance < upper_bound) {
                mid_rn = min_node.i_rn;
                upper_bound = new_distance;
            }
        }

        if (min_node.distance > upper_bound) {
            fwd_heap.RemoveAll();
            return;
        }

        min_node.finished = true;

        pairs.clear();
        for (auto i_conn : routing_node_pool_[min_node.i_rn].conn_tos_) {
            const auto& edge = conn_pool_[i_conn];
            const auto& i_to_rn = edge.i_to_rn_;

            // if the some segs are excluded, e.g., closed tunnel in the midnight
            if (time_point != 0 && edge.excluded_flag_) {
                bool is_excluded = false;
                for (const auto &p_seg : edge.segs_) {
                    if (p_seg->excluded_flag_) {
                        if (p_seg->excluded_always_ ||
                            this->way_manager_.IsSegmentExcluded(p_seg, time_point,
                                is_localtime)) {
                            is_excluded = true;
                            break;
                        }
                    }
                }
                if (is_excluded) {
                    continue;
                }
            }

            NODE_DATA_INDEX i_to_node_data = fwd_heap.GetIfInserted(i_to_rn);
            if (i_to_node_data != 0 && fwd_pool[i_to_node_data].finished) {
                continue;
            }

            // make sure the "to" node is already in working queue
            // below operation will not do insertion if was inserted before
            auto e_weight = edge.weight_;
            if (i_to_node_data == 0) {
                i_to_node_data = fwd_heap.Insert(i_to_rn, min_node.i_rn,
                    min_node.distance + e_weight);
            }

            // get all the keys to be decreased later
            if (min_node.distance + e_weight < fwd_pool[i_to_node_data].distance) {
                pairs.emplace_back(i_to_node_data, min_node.distance + e_weight);
                fwd_pool[i_to_node_data].i_pre_rn = min_node.i_rn; // also update the parent node
            }
        }

        if (!pairs.empty()) {
            fwd_heap.DecreaseKeys(pairs);
        }
    }

    void StepBackward(dijkstra::BinHeap& rev_heap, dijkstra::BinHeap& fwd_heap,
        ROUTING_NODE_INDEX& mid_rn, int& upper_bound,
        vector<dijkstra::HeapData>& pairs,
        time_t time_point, bool is_localtime) const
    {
        using namespace dijkstra;

        auto& rev_pool = rev_heap.NodePool();
        HeapData min_heap_node = rev_heap.DeleteMin();
        NodeData& min_node = rev_pool[min_heap_node.pool_index_];

        // check if overlapped with q1
        auto i_min_node_in_fwd = fwd_heap.GetIfInserted(min_node.i_rn);
        if (i_min_node_in_fwd != 0) {
            auto& fwd_pool = fwd_heap.NodePool();
            int new_distance = min_node.distance + fwd_pool[i_min_node_in_fwd].distance;
            if (new_distance < upper_bound) {
                mid_rn = min_node.i_rn;
                upper_bound = new_distance;
            }
        }

        if (min_node.distance > upper_bound) {
            rev_heap.RemoveAll();
            return;
        }
        min_node.finished = true;

        pairs.clear();
        for (auto i_conn : routing_node_pool_[min_node.i_rn].conn_froms_) {
            const auto& edge = conn_pool_[i_conn];
            const auto& i_from_rn = edge.i_from_rn_;

            // if the some segs are excluded, e.g., closed tunnel in the midnight
            if (time_point != 0 && edge.excluded_flag_) {
                bool is_excluded = false;
                for (const auto &p_seg : edge.segs_) {
                    if (p_seg->excluded_flag_) {
                        if (p_seg->excluded_always_ ||
                            this->way_manager_.IsSegmentExcluded(p_seg, time_point,
                                is_localtime)) {
                            is_excluded = true;
                            break;
                        }
                    }
                }
                if (is_excluded) {
                    continue;
                }
            }

            NODE_DATA_INDEX i_from_node_data = rev_heap.GetIfInserted(i_from_rn);
            if (i_from_node_data != 0 && rev_pool[i_from_node_data].finished) {
                continue;
            }

            // make sure the "from" node is already in working queue
            // below operation will not do insertion if was inserted before
            auto weight = edge.weight_;
            if (i_from_node_data == 0) {
                i_from_node_data = rev_heap.Insert(i_from_rn, min_node.i_rn,
                    min_node.distance + weight);
            }

            // get all the keys to be decreased later
            if (min_node.distance + weight < rev_pool[i_from_node_data].distance) {
                pairs.emplace_back(i_from_node_data, min_node.distance + weight);
                rev_pool[i_from_node_data].i_pre_rn = min_node.i_rn; // also update the parent node
            }
        }

        if (!pairs.empty()) {
            rev_heap.DecreaseKeys(pairs);
        }
    }

    ROUTING_NODE_INDEX GetSrcRoutingNode(const SegmentPtr& p_seg) const
    {
        auto& segs = p_seg->GetWayOriented()->Segments();
        const auto segs_size = segs.size();
        size_t i1;

        for (i1 = 0; i1 < segs_size; ++i1) {
            if (segs[i1] == p_seg) {
                break;
            }
        }
        if (i1 >= segs_size) {
            for (i1 = 0; i1 < segs_size; ++i1) {
                if (segs[i1]->seg_id_ == p_seg->seg_id_) {
                    break;
                }
            }
            if (i1 >= segs_size) {
                return 0;
            }
        }

        for (size_t i = i1; i < segs_size; ++i) {
            auto& p_to_nd = segs[i]->GetToNode();
            if (p_to_nd->IsRoutingNode()) {
                return routing_node_map_.at(p_to_nd->nd_id_);
            }
        }

        return 0;
    }

    ROUTING_NODE_INDEX GetDstRoutingNode(const SegmentPtr& p_seg) const
    {
        auto& segs = p_seg->GetWayOriented()->Segments();
        const int segs_size = (int)segs.size();
        int i2;
        for (i2 = segs_size - 1; i2 >= 0; --i2) {
            if (segs[i2] == p_seg) {
                break;
            }
        }
        if (i2 < 0) {
            for (i2 = segs_size - 1; i2 >= 0; --i2) {
                if (segs[i2]->seg_id_ == p_seg->seg_id_) {
                    break;
                }
            }
            if (i2 < 0) {
                return 0;
            }
        }

        for (int i = i2; i >= 0; --i) {
            auto& p_from_nd = segs[i]->GetFromNode();
            if (p_from_nd->IsRoutingNode()) {
                return routing_node_map_.at(p_from_nd->nd_id_);
            }
        }

        return 0;
    }

    // assumption: edge ID is same as 1st segment ID on the edge
    EDGE_ID_T SegmentToEdge(const SegmentPtr& p_seg) const
    {
        auto& segs = p_seg->GetWayOriented()->Segments();
        const size_t segs_size = segs.size();

        size_t i1;
        for (i1 = 0; i1 < segs_size; ++i1) {
            if (segs[i1] == p_seg) {
                break;
            }
        }
        if (i1 >= segs_size) {
            for (i1 = 0; i1 < segs_size; ++i1) {
                if (segs[i1]->seg_id_ == p_seg->seg_id_) {
                    break;
                }
            }
            if (i1 >= segs_size) {
                return 0;
            }
        }

        for (int i = (int)i1; i >= 0; --i) {
            auto& p_from_nd = segs[i]->GetFromNode();
            if (p_from_nd->IsRoutingNode()) {
                // edge ID is same as 1st segment ID on the edge
                return segs[i]->seg_id_;
            }
        }

        return 0;
    }

private:
    const WayManager& way_manager_;
    RoutingNodeMap routing_node_map_;
    bool shortest_mode_{};

    util::SimpleObjPool<RoutingNode>        routing_node_pool_;
    util::SimpleObjPool<Connection>         conn_pool_;
    util::SimpleObjPool<TwoStepConnection>  conn2_pool_;
    util::SimpleObjPool<FourStepConnection> conn4_pool_;
    util::SimpleObjPool<SixStepConnection>  conn6_pool_;

    friend class geo::WayManager;
};

} // end of namespace route

///////////////////////////////////////////////////////////////////////////////////////////////////
// class WayManager

using namespace route;

bool WayManager::InitForRouting(bool shortest_mode /*= true*/)
{
    p_route_manager_ = make_shared<RouteManager>(*this);
    return p_route_manager_->InitForRouting(shortest_mode);
}

void WayManager::SyncExclusionSegsToRouting()
{
    if (p_route_manager_) {
        p_route_manager_->SyncExclusionSegsToRouting();
    }
}

bool WayManager::RoutingNearby(const SegmentPtr& p_seg1, const SegmentPtr& p_seg2,
    vector<SegmentPtr>& result_route, bool exclude_reversed_segs /*= false*/,
    time_t time_point /*= 0*/, bool is_localtime /*= false*/,
    bool points_reversed /*= false*/) const
{
    return p_route_manager_->RoutingNearby(p_seg1, p_seg2, result_route,
        exclude_reversed_segs, time_point, is_localtime, points_reversed);
}

bool WayManager::SimilarRoutingNearby(const SegmentPtr& p_seg1, const SegmentPtr& p_seg2,
    const vector<GeoPoint>& trace, vector<SegmentPtr>& result_route, double& distance,
    bool exclude_reverse_segs /*= false*/) const
{
    return p_route_manager_->SimilarRoutingNearbyImpl(p_seg1, p_seg2, trace, result_route,
        distance, exclude_reverse_segs);
}

bool WayManager::SimilarRoutingNearby(const SegmentPtr& p_seg1, const SegmentPtr& p_seg2,
    const vector<ViaPoint>& trace, vector<SegmentPtr>& result_route, double& distance,
    bool exclude_reverse_segs /*= false*/) const
{
    return p_route_manager_->SimilarRoutingNearbyImpl(p_seg1, p_seg2, trace, result_route,
        distance, exclude_reverse_segs);
}

bool WayManager::DijkstraShortestPath(const SegmentPtr& p_seg1, const SegmentPtr& p_seg2,
    vector<SegmentPtr>& result_route, int *search_steps/*= nullptr*/,
    time_t time_point/* = 0*/, bool is_localtime/*= false*/,
    bool points_reversed /*= false*/) const
{
    return p_route_manager_->DijkstraShortestPath(p_seg1, p_seg2, result_route, search_steps,
        time_point, is_localtime, points_reversed);
}

bool WayManager::DijkstraShortestEdgePath(const SegmentPtr& p_seg1, const SegmentPtr& p_seg2,
    vector<EDGE_ID_T>& result_route) const
{
    return p_route_manager_->DijkstraShortestEdgePath(p_seg1, p_seg2, result_route);
}

bool WayManager::ViaRoute(const ViaRouteParams& params, ViaRouteResult& result) const
{
    return p_route_manager_->ViaRoute(params, result);
}

int WayManager::DistanceToWeight(double distance, HIGHWAY_TYPE type)
{
    return RouteManager::DistanceToWeight(distance, type);
}

void WayManager::GetRoutingSummary(int& total_request, int& success_request)
{
    p_route_manager_->GetRoutingSummary(total_request, success_request);
}

bool WayManager::GetAdjacentOutboundSegs(const SegmentPtr& p_seg, NODE_ID_T node_id,
    std::vector<SegmentPtr>& out_segs) const
{
    return p_route_manager_->GetAdjacentOutboundSegs(p_seg, node_id, out_segs);
}

SegmentPtr WayManager::GetLeadSeg(const SegmentPtr& p_seg) const
{
    return p_route_manager_->GetLeadSeg(p_seg);
}

} // end of namespace
