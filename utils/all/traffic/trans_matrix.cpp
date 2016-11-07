#include "trans_matrix.h"
#include <algorithm>
#include <numeric>
#include <iostream>
#include <boost/graph/strong_components.hpp>
#include <boost/graph/adjacency_list.hpp>
#include "common/simple_par_algorithm.hpp"
#include "common/simple_thread_pool.hpp"
#include "common/common_utils.h"

#ifdef _WIN32
#define USE_BOOST_UNORDERED 0
#else
#define USE_BOOST_UNORDERED 1
#endif
#define USE_BOOST_MATRIX    0

#if USE_BOOST_UNORDERED == 1
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#else
#include <unordered_map>
#include <unordered_set>
#endif
#if USE_BOOST_MATRIX == 1
#include <boost/numeric/ublas/matrix.hpp>
#endif

#define LOG(level)   std::cout

#define PASSENGER_STATE_EMPTY           0
#define PASSENGER_STATE_LOADED          1
#define PASSENGER_STATE_UNSPECIFIED     2

#define TRANS_MATRIX_BEGIN_NAMESPACE namespace traffic {
#define TRANS_MATRIX_END_NAMESPACE   }

TRANS_MATRIX_BEGIN_NAMESPACE

using Transition = TransMatrixGenerator::Transition;
using Stationary = TransMatrixGenerator::Stationary;

class TransMatrixGenerator::impl
{
#if USE_BOOST_UNORDERED == 1
    template <typename K, typename V> using UNORDERED_MAP = boost::unordered_map<K, V>;
    template <typename K> using UNORDERED_SET = boost::unordered_set<K>;
#else
    template <typename K, typename V> using UNORDERED_MAP = std::unordered_map<K, V>;
    template <typename K> using UNORDERED_SET = std::unordered_set<K>;
#endif

    using NODE_ID_T = geo::NODE_ID_T;
    using EDGE_ID_T = geo::EDGE_ID_T;
    using SEG_ID_T = geo::SEG_ID_T;

    struct EdgeSegItem
    {
        geo::SEG_ID_T seg_id{};
        int seq_no{};
    };
    struct EdgeInfo
    {
        std::vector<EdgeSegItem> segs;
        geo::NODE_ID_T from_node_id{};
        geo::NODE_ID_T to_node_id{};
        bool main_component{};
    };

    typedef std::vector<geo::EDGE_ID_T> EdgePath;
    struct Trip
    {
        int start{}, end{}; // start/end index of FcdPoint[]
        EdgePath edge_path;
    };

    struct FcdDevice
    {
        std::string dev_id;
        int start{}, end{}; // start/end index of FcdPoint[]
        std::vector<Trip> trips;
    };

    struct TransFrom
    {
        geo::EDGE_ID_T from_edge{};
        double probability{};
        int edge_index{}; // sequence No of the from_edge in EdgeTransFromMap interation
    };
    struct TransTo
    {
        geo::EDGE_ID_T to_edge{};
        int count{};
        double probability{};
    };

    struct EdgeTrans
    {
        geo::EDGE_ID_T edge_id{};
        EdgeInfo edge_info;
        std::vector<TransFrom> trans_froms;
        std::vector<TransTo> trans_tos;
        double stationary_prob{}; // stationary probability
    };
    typedef UNORDERED_MAP<geo::EDGE_ID_T, EdgeTrans> EdgeMap;

    struct TransFromsTo
    {
        geo::EDGE_ID_T to_edge{};
        std::vector<TransFrom> trans_froms;
    };

public:
    explicit impl(const TransMatrixGenerator::InParams& in_params,
        const geo::WayManager& way_manager)
        : in_params_(in_params), way_manager_(way_manager)
    {}

    bool GetAdjacentOutboundSegs(const EdgeInfo& edge_info,
        std::vector<geo::SegmentPtr>& out_segs) const
    {
        if (edge_info.segs.empty()) {
            return false;
        }
        const auto& seg = edge_info.segs.back();
        auto p_seg = way_manager_.GetSegById(seg.seg_id);
        if (!p_seg) {
            return false;
        }

        out_segs.clear();
        for (const auto& p_conn_seg : p_seg->GetToNode()->ConnectedSegments()) {
            if (p_conn_seg->from_nd_ == edge_info.to_node_id) {
                out_segs.push_back(p_conn_seg);
            }
        }

        return true;
    }

    bool GenTransMatrix()
    {
        // amend the adjacent edges, but without transitioned vehicles count
        std::vector<geo::SegmentPtr> out_segs;
        for (auto& it : edge_map_) {
            auto& edge_id = it.first;
            auto& edge = it.second;

            // amend using the adjacent edges
            if (true == GetAdjacentOutboundSegs(edge.edge_info, out_segs)) {
                for (const auto& p_out_seg : out_segs) {
                    auto out_edge = SegToEdgeId(p_out_seg);
                    if (out_edge == 0 || edge_id == out_edge) {
                        continue;
                    }

                    bool found = false;
                    for (size_t i = 0; i < edge.trans_tos.size(); ++i) {
                        if (edge.trans_tos[i].to_edge == out_edge) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        TransTo tt;
                        tt.to_edge = out_edge;
                        edge.trans_tos.push_back(tt);
                    }
                }
            }

            // adjust the probability
            AdjustEdgeTransitions(it.second);
            //VerifyEdgeTransitions(it.second);
        }

        return true;
    }

    void AdjustEdgeTransitions(EdgeTrans& edge) const
    {
        if (edge.trans_tos.empty()) return;

        for (auto& tt : edge.trans_tos) {
            // if no transitions found, must have at leat one
            if (tt.count == 0) {
                tt.count = 1;
            }
        }

        int sum_count = 0;
        for (const auto& tt : edge.trans_tos) {
            sum_count += tt.count;
        }
        for (auto& tt : edge.trans_tos) {
            tt.probability = double(tt.count) / sum_count;
        }

        auto p_from_last_seg = way_manager_.GetSegById(edge.edge_info.segs.back().seg_id);
        if (p_from_last_seg) {
            for (auto& tt : edge.trans_tos) {
                const auto& to_edge_info = edge_map_.at(tt.to_edge).edge_info;
                auto p_to_first_seg = way_manager_.GetSegById(to_edge_info.segs.front().seg_id);
                if (p_to_first_seg) {
                    if (180 == geo::WayManager::GetAngle(p_from_last_seg->heading_,
                        p_to_first_seg->heading_))
                    {
                        tt.probability /= 3.0; // u-turn penalty
                    }
                }
            }
        }

        // make sure sum probability is 1
        double sum_prob = 0;
        for (const auto& tt : edge.trans_tos) {
            sum_prob += tt.probability;
        }
        for (auto& tt : edge.trans_tos) {
            tt.probability /= sum_prob;
        }
    }

    static void VerifyEdgeTransitions(const EdgeTrans& edge)
    {
        const auto& trans_tos = edge.trans_tos;
        double total_prob = 0;
        for (const auto& tt : trans_tos) {
            total_prob += tt.probability;
        }
        if (total_prob < 0.99 || total_prob > 1.01) {
            LOG(DEBUG) << "invalid sum probabilities for edge " << edge.edge_id << std::endl;
        }
    }

    bool ResultsToJson(const std::string& json_pathname) const
    {
        const auto N = edge_map_.size();
        geo::GeoJSON geo_json;
        UNORDERED_SET<geo::NODE_ID_T> node_set;

        std::vector<std::string> property_names, property_values;
        property_names.push_back("edge_id");
        property_names.push_back("stationary%");
        property_names.push_back("stationary*N");
        property_names.push_back("trans-tos");
        property_names.push_back("trans-froms");
        property_values.resize(property_names.size());
        std::string tos, froms;

        for (const auto& it : edge_map_) {
            const auto& edge_id = it.first;
            const EdgeInfo& edge = it.second.edge_info;

            tos.clear();
            int k = 1;
            for (auto& t : it.second.trans_tos) {
                tos += std::to_string(k++) + ")" + std::to_string(t.to_edge) + ":";
                tos += DoubleToString(t.probability * 100) + "%(" + std::to_string(t.count) + ") ";
            }
            if (tos.back() == ' ') tos.pop_back();

            froms.clear();
            k = 1;
            for (auto& t : it.second.trans_froms) {
                froms += std::to_string(k++) + ")" + std::to_string(t.from_edge) + ":";
                froms += DoubleToString(t.probability * 100) + "% ";
            }
            if (froms.back() == ' ') froms.pop_back();

            int n = 0;
            property_values[n++] = std::to_string(edge_id);
            property_values[n++] = std::to_string(it.second.stationary_prob);
            property_values[n++] = std::to_string(it.second.stationary_prob * N);
            property_values[n++] = tos;
            property_values[n++] = froms;

            auto p_obj = EdgeToGsonLineString(edge, property_names, property_values, 12);
            if (p_obj) {
                geo_json.AddObj(p_obj);
                p_obj->AddProp("color", edge.main_component ? "blue" : "green");
            }

            node_set.insert({ edge.from_node_id, edge.to_node_id });
        }

        for (const auto& node_id : node_set) {
            auto p_node = way_manager_.GetNodeById(node_id);
            auto p_obj = std::make_shared<geo::GeoObj_Point>(p_node->geo_point_);
            if (p_obj) {
                p_obj->AddProp("node_id", p_node->nd_id_);
                geo_json.AddObj(p_obj);
            }
        }

        return geo_json.ToJsonFile(json_pathname);
    }

    void FreeUnused()
    {
        points_pool_ = decltype(points_pool_)();
        fcd_devices_ = decltype(fcd_devices_)();
    }

    /*
    B12.setdiag(0) # Here, B12 the transition matrix
    A=B12.tocsc()
    x0=rand(1,A.shape[1],density=1)
    #====================================================

    x0=csc_matrix(x0/x0.sum(1))

    eps=1e-6
    imax=1e6
    i=1
    delta=1

    while (delta>=eps and i<=imax ):
        x1=x0*A
        delta=linalg.norm((x1-x0).toarray())
        x0=x1
        i=i+1

    x1=x1.toarray()
    x1=x1/sum(x1)
    */
#if USE_BOOST_MATRIX == 0
    bool GenStationaryMatrix()
    {
        PreStationaryCalc(edge_map_);
        UpdateTransFroms(main_edges_);

        auto trans_froms_to_vec = BuildTransFromsTo(main_edges_);
        const size_t n_edge = trans_froms_to_vec.size();
        std::vector<double> x0(n_edge, 1.0 / n_edge), x1(n_edge);

        const double eps = 1e-7;
        const int imax = 1000000;
        int i = 0;
        double delta = 1;

        while (delta >= eps && i <= imax) {
            // x1 = x0 * A
            int i_x1 = 0;
            for (const auto& froms_to : trans_froms_to_vec) {
                double m = 0;
                for (const TransFrom& trans_from : froms_to.trans_froms) {
                    m += trans_from.probability * x0[trans_from.edge_index];
                }
                x1[i_x1++] = m;
            }

            // x1-x0 norm
            delta = 0;
            for (size_t k = 0; k < n_edge; ++k) {
                double diff = x1[k] - x0[k];
                delta += diff * diff;
            }
            delta = std::sqrt(delta);

            x0 = x1;
            i++;
        }

        double sum = std::accumulate(x1.begin(), x1.end(), 0.0);
        for (auto& v : x1) {
            v /= sum;
        }

        // populate stationary_prob in map
        i = 0;
        for (const auto& froms_to : trans_froms_to_vec) {
            edge_map_[froms_to.to_edge].stationary_prob = x1[i++];
        }
        return true;
    }
#else
    bool GenStationaryMatrix()
    {
        using namespace boost::numeric::ublas;
        PreStationaryCalc(edge_map_);
        UpdateTransFroms(main_edges_);

        // edge ID mapped to index
        UNORDERED_MAP<geo::EDGE_ID_T, int> edge_to_index;
        edge_to_index.reserve(main_edges_.size());
        int index = 0;
        for (const auto& e : main_edges_) {
            edge_to_index[e.edge_id] = index++;
        }
        const size_t n = edge_to_index.size();

        // build matrix A
        matrix<double> A(n, n, 0.0);
        for (const auto& e : main_edges_) {
            int i = edge_to_index.at(e.edge_id);
            for (const auto& trans_to : e.trans_tos) {
                int j = edge_to_index.at(trans_to.to_edge);
                A(i, j) = trans_to.probability;
            }
        }

        // populate x0
        vector<double> x0(n, 1.0 / n), x1(n);

        const double eps = 1e-7;
        const int imax = 1000000;
        double delta = 1;

        int i;
        for (i = 0; delta >= eps && i <= imax; ++i) {
            x1 = prod(x0, A);
            delta = norm_2(x1 - x0);
            x0 = x1;
        }

        LOG(DEBUG) << __FUNCTION__ << ": delta = " << delta << ", i = " << i
            << ", exiting loop" << std::endl;

        double sum = 0;
        for (size_t i = 0; i < n; ++i) {
            sum += x1(i);
        }
        for (size_t i = 0; i < n; ++i) {
            x1(i) /= sum;
        }

        // populate stationary_prob in map
        i = 0;
        for (auto& edge : main_edges_) {
            edge_map_[edge.edge_id].stationary_prob = x1[i++];
        }

        return true;
    }
#endif

    // main component edges vector => std::vector<TransFromsTo>
    std::vector<TransFromsTo> BuildTransFromsTo(std::vector<EdgeTrans>& main_edges) const
    {
        std::vector<TransFromsTo> froms_to_vec;
        froms_to_vec.reserve(main_edges.size());

        for (auto& edge : main_edges) {
            froms_to_vec.push_back(TransFromsTo());
            TransFromsTo& froms_to = froms_to_vec.back();
            froms_to.to_edge = edge.edge_id;
            froms_to.trans_froms = edge.trans_froms;
        }

        // optimize the order for CPU cache
        std::sort(froms_to_vec.begin(), froms_to_vec.end(),
            [this](const TransFromsTo& i, const TransFromsTo& j) {
            auto i_seg = this->way_manager_.GetSegById(i.to_edge);
            auto j_seg = this->way_manager_.GetSegById(j.to_edge);
            return i_seg->from_point_.lng < j_seg->from_point_.lng;
        });

        // for froms_to.edge_index
        UNORDERED_MAP<EDGE_ID_T, int> edge_id_to_index;
        edge_id_to_index.reserve(froms_to_vec.size());
        int index = 0;
        for (auto& froms_to : froms_to_vec) {
            edge_id_to_index[froms_to.to_edge] = index++;
        }
        for (auto& froms_to : froms_to_vec) {
            for (auto& from : froms_to.trans_froms) {
                from.edge_index = edge_id_to_index.at(from.from_edge);
            }
        }

        return froms_to_vec;
    }

    void OnFcdDataReady()
    {
        LoadFcdData();
        UpdateTransitionCounts();
        FreeUnused();
    }

    void UpdateTransitionCounts()
    {
        if (fcd_devices_.size() < 20) {
            geo::RouteMatchingParams match_params;
            match_params.is_localtime = (in_params_.time_point_type == 0);

            for (auto& dev_trace : fcd_devices_) {
                if (true == SplitTraceIntoTrips(dev_trace)) {
                    DoTraceMatching(match_params, dev_trace);
                }
            }
        }
        else {
            util::SimpleDataQueue<size_t> indices;
            for (size_t i = 0; i < fcd_devices_.size(); ++i) {
                indices.Add(i);
            }

            unsigned task_count = std::thread::hardware_concurrency();
            if (task_count == 0) {
                task_count = 1;
            }
            util::CreateSimpleThreadPool("GenTransMat", task_count, [&indices, this]() {
                while (true) {
                    geo::RouteMatchingParams match_params;
                    match_params.is_localtime = (in_params_.time_point_type == 0);

                    size_t dev_index;
                    if (false == indices.Get(dev_index)) {
                        break;
                    }

                    auto& dev_trace = fcd_devices_[dev_index];
                    if (true == SplitTraceIntoTrips(dev_trace)) {
                        DoTraceMatching(match_params, dev_trace);
                    }
                }
            }).JoinAll();
        }

        // populate edge_map_
        for (const auto& dev_trace : fcd_devices_) {
            for (const Trip& trip : dev_trace.trips) {
                if (trip.edge_path.size() <= 1) continue;
                for (size_t i = 0; i < trip.edge_path.size() - 1; ++i) {
                    auto& from_edge = trip.edge_path[i];
                    auto& to_edge = trip.edge_path[i + 1];

                    // check if the two edges are connected
                    auto it_from_edge = edge_map_.find(from_edge);
                    auto it_to_edge = edge_map_.find(to_edge);
                    if (it_from_edge == edge_map_.cend() || it_to_edge == edge_map_.cend()) {
                        break;
                    }
                    if (it_from_edge->second.edge_info.to_node_id != it_to_edge->second.edge_info.from_node_id) {
                        break;
                    }

                    // update entry in edge_map_
                    auto &edge = edge_map_[from_edge];
                    edge.edge_id = from_edge;
                    bool found = false;
                    for (size_t j = 0; j < edge.trans_tos.size(); ++j) {
                        if (edge.trans_tos[j].to_edge == to_edge) {
                            edge.trans_tos[j].count++;
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        edge.trans_tos.emplace_back(TransTo());
                        edge.trans_tos.back().to_edge = to_edge;
                        edge.trans_tos.back().count++;
                    }
                }
            }
        }

    }

    void LoadFcdData()
    {
        const size_t total_points_size = points_pool_.size();
        if (total_points_size == 0) {
            return;
        }

        if (!in_params_.fcd_gps_order_by_devid) {
            // sort by dev_id
            util::ParSort(points_pool_.begin(), points_pool_.end(),
                [](const TransMatrixGenerator::FcdPoint& i, const TransMatrixGenerator::FcdPoint& j) {
                return i.dev_id < j.dev_id;
            });
        }

        // pupulate fcd_devices_[]
        fcd_devices_.clear();
        fcd_devices_.reserve(10000);

        FcdDevice dev;
        dev.dev_id = points_pool_[0].dev_id;
        dev.start = 0;
        for (size_t i = 1; i < total_points_size; ++i) {
            if (points_pool_[i].dev_id != points_pool_[i - 1].dev_id) {
                dev.end = (int)i - 1;
                fcd_devices_.push_back(dev);

                dev.dev_id = points_pool_[i].dev_id;
                dev.start = (int)i;
            }
        }
        // for last device
        if (fcd_devices_.back().dev_id != points_pool_.back().dev_id) {
            dev.end = (int)total_points_size - 1;
            fcd_devices_.push_back(dev);
        }
    }

    bool SplitTraceIntoTrips(FcdDevice& dev_trace)
    {
        if (dev_trace.end - dev_trace.start < 1) {
            return false;
        }

        // sort by time
        std::sort(points_pool_.begin() + dev_trace.start, points_pool_.begin() + dev_trace.end + 1,
            [](const TransMatrixGenerator::FcdPoint& i, const TransMatrixGenerator::FcdPoint& j) {
            return i.gps_time < j.gps_time;
        });

        // generate trips
        Trip trip;
        auto CheckAppendTrip = [&]() {
            if (trip.end <= trip.start) return;
            switch (in_params_.passenger_state_filter) {
            case PASSENGER_STATE_UNSPECIFIED:
                dev_trace.trips.push_back(trip);
                break;
            case PASSENGER_STATE_EMPTY:
            case PASSENGER_STATE_LOADED:
                if (in_params_.passenger_state_filter == points_pool_[trip.start].passenger_state) {
                    dev_trace.trips.push_back(trip);
                }
                break;
            default:
                break;
            }
        };

        trip.start = dev_trace.start;
        for (int i = dev_trace.start + 1; i <= dev_trace.end; ++i) {
            auto& prev = points_pool_[i - 1];
            auto& point = points_pool_[i];
            if (prev.passenger_state != point.passenger_state) {
                trip.end = i - 1;
                CheckAppendTrip();

                // for next trip
                trip.start = i;
            }
        }
        // the last trip
        trip.end = dev_trace.end;
        CheckAppendTrip();

        return !dev_trace.trips.empty();
    }

    void DoTraceMatching(geo::RouteMatchingParams& match_params, FcdDevice& dev_trace) const
    {
        auto SegToEdge = [this](geo::SEG_ID_T seg_id) -> geo::EDGE_ID_T {
            auto it = seg_to_edge_.find(seg_id);
            if (it == seg_to_edge_.cend()) {
                return 0;
            }
            else {
                return it->second;
            }
        };

        for (auto& trip : dev_trace.trips) {
            geo::RouteMatchingInternalCleanup(match_params);
            match_params.is_localtime = (in_params_.time_point_type == 0);

            match_params.via_points.clear();
            for (auto i = trip.start; i <= trip.end; ++i) {
                const auto& r = points_pool_[i];
                match_params.via_points.emplace_back(r.geo_point.lat, r.geo_point.lng,
                    (int)r.heading, (int)(r.speed + .5), r.gps_time);
            }

            bool ok = way_manager_.RouteMatching(match_params);
            if (ok && !match_params.result_route.empty()) {
                trip.edge_path.clear();
                for (const auto& p_seg : match_params.result_route) {
                    auto edge_id = SegToEdge(p_seg->seg_id_);
                    if (edge_id != 0) {
                        if (trip.edge_path.empty() || (trip.edge_path.back() != edge_id)) {
                            trip.edge_path.push_back(edge_id);
                        }
                    }
                }
            }
        }
    }

    static inline std::string DoubleToString(double v)
    {
        char buff[128];
        snprintf(buff, sizeof(buff) - 1, "%.4f", v);
        return buff;
    }

    // NOTE: the function adds offsets to the two oriented ways from any two-way segments
    //       so that they are not overlapped
    std::shared_ptr<geo::GeoObj_LineString> EdgeToGsonLineString(
        const EdgeInfo& edge,
        const std::vector<std::string>& property_names,
        const std::vector<std::string>& property_values,
        double two_way_offset) const
    {
        auto line_string = std::make_shared<geo::GeoObj_LineString>();
        std::vector<geo::SegmentPtr> segs;
        segs.reserve(edge.segs.size());
        for (const auto& seg : edge.segs) {
            segs.push_back(way_manager_.GetSegById(seg.seg_id));
        }

        if (segs.front()->one_way_) {
            for (size_t i = 0; i < edge.segs.size(); ++i) {
                if (i == 0) {
                    line_string->AddPoint(segs.front()->from_point_);
                    line_string->AddPoint(segs.front()->to_point_);
                }
                else {
                    line_string->AddPoint(segs[i]->to_point_);
                }
            }
        }
        else {
            // for segment from two-way, add offset so that it is not overlapped with
            // its opposite segment 
            for (size_t i = 0; i < segs.size(); ++i) {
                if (segs[i]->length_ == 0) {
                    line_string->AddPoint(segs[i]->to_point_);
                }
                else if (i == 0) {
                    geo::GeoPoint offset_from, offset_to;
                    geo::get_offset_segment(segs.front()->from_point_,
                        segs.front()->to_point_, two_way_offset,
                        offset_from, offset_to);
                    line_string->AddPoint(offset_from);
                    line_string->AddPoint(offset_to);
                }
                else {
                    // for segment from two-way, add offset so that it is not overlapped with
                    // its opposite segment
                    geo::GeoPoint offset_from, offset_to;
                    geo::get_offset_segment(segs[i]->from_point_,
                        segs[i]->to_point_, two_way_offset,
                        offset_from, offset_to);
                    line_string->AddPoint(offset_to);
                }
            }
        }

        for (size_t i = 0; i < property_names.size(); ++i) {
            if (!property_names[i].empty() && !property_values[i].empty()) {
                line_string->AddProp(property_names[i], property_values[i]);
            }
        }

        return line_string;
    }

    UNORDERED_SET<NODE_ID_T> GetMainComponentNodesSet(const EdgeMap& edge_trans_map) const
    {
        struct GNode {
            NODE_ID_T node_id;
            bool main_component;
        };

        UNORDERED_SET<NODE_ID_T> node_id_set;
        node_id_set.reserve(edge_map_.size());
        for (const auto& it : edge_map_) {
            node_id_set.insert(it.second.edge_info.from_node_id);
            node_id_set.insert(it.second.edge_info.to_node_id);
        }
        const auto N = node_id_set.size();

        std::vector<GNode> g_nodes;
        g_nodes.reserve(N);
        for (auto& node_id : node_id_set) {
            g_nodes.push_back({ node_id, false });
        }

        UNORDERED_MAP<NODE_ID_T, int> node_id_to_index;
        node_id_to_index.reserve(N);
        int index = 0;
        for (const auto& g_node : g_nodes) {
            node_id_to_index[g_node.node_id] = index++;
        }

        typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS> Graph;
        Graph G(N);
        for (const auto& it : edge_map_) {
            auto& e = it.second;
            boost::add_edge(node_id_to_index.at(e.edge_info.from_node_id),
                node_id_to_index.at(e.edge_info.to_node_id), G);
        }

        std::vector<int> components(N);
        int components_num = strong_components(G, boost::make_iterator_property_map(
            components.begin(), boost::get(boost::vertex_index, G), components[0]));
        UNORDERED_MAP<int, int> component_counts;
        component_counts.reserve(components_num);
        for (auto& i_component : components) {
            component_counts[i_component]++;
        }

        // get the index of main component
        int i_max_component = -1;
        int max_count = 0;
        for (auto& it : component_counts) {
            if (max_count < it.second) {
                max_count = it.second;
                i_max_component = it.first;
            }
        }

        // flag the main component
        size_t main_component_count = 0;
        for (size_t i_gnode = 0; i_gnode < N; ++i_gnode) {
            int i_component = components[i_gnode];
            if (i_component == i_max_component) {
                g_nodes[i_gnode].main_component = true;
                main_component_count++;
            }
        }

        UNORDERED_SET<NODE_ID_T> main_component_nodes;
        main_component_nodes.reserve(main_component_count);
        for (auto& g_node : g_nodes) {
            if (g_node.main_component) {
                main_component_nodes.insert(g_node.node_id);
            }
        }

        return main_component_nodes;
    }

    void PreStationaryCalc(EdgeMap& edge_trans_map)
    {
        auto&& main_component_set = GetMainComponentNodesSet(edge_trans_map);
        auto is_main_component_node = [&main_component_set](NODE_ID_T node_id) {
            auto it = main_component_set.find(node_id);
            return it != main_component_set.end();
        };

        size_t main_count = 0;
        for (auto& it : edge_trans_map) {
            auto& edge = it.second;
            if (is_main_component_node(edge.edge_info.from_node_id) &&
                is_main_component_node(edge.edge_info.to_node_id))
            {
                edge.edge_info.main_component = true;
                ++main_count;
            }
            else {
                edge.edge_info.main_component = false;
            }
        }

        // populate main_edges_[]
        main_edges_.clear();
        main_edges_.reserve(main_count);
        for (auto& it : edge_trans_map) {
            auto& edge = it.second;
            if (edge.edge_info.main_component) {
                main_edges_.push_back(edge);
            }
        }
        for (auto& edge : main_edges_) {
            for (int i = (int)edge.trans_tos.size() - 1; i >= 0; --i) {
                const auto& to_edge_id = edge.trans_tos[i].to_edge;
                // make sure all the "to" edges are also in main component
                auto it = edge_map_.find(to_edge_id);
                if (it == edge_map_.end() || !it->second.edge_info.main_component) {
                    edge.trans_tos.erase(edge.trans_tos.begin() + i);
                }
            }
        }
    }

    // build the transition-from info in each entry, using transition-to info
    void UpdateTransFroms(std::vector<EdgeTrans>& main_edges) const
    {
        std::sort(main_edges.begin(), main_edges.end(), [](const EdgeTrans& i, const EdgeTrans& j) {
            return i.edge_id < j.edge_id;
        });

        // populate edge.trans_from in each map entry
        EdgeTrans comp_val;
        TransFrom t_from;
        t_from.edge_index = -1;
        for (const auto& edge : main_edges) {
            t_from.from_edge = edge.edge_id;
            t_from.edge_index++;

            for (const auto& trans_to : edge.trans_tos) {
                comp_val.edge_id = trans_to.to_edge;
                auto it = std::lower_bound(main_edges.begin(), main_edges.end(), comp_val,
                    [](const EdgeTrans& i, const EdgeTrans& j) {
                    return i.edge_id < j.edge_id;
                });

                if (it != main_edges.end()) {
                    t_from.probability = trans_to.probability;
                    it->trans_froms.push_back(t_from);
                }
            }
        }
    }

    EDGE_ID_T SegToEdgeId(const geo::SegmentPtr p_seg) const
    {
        auto it = seg_to_edge_.find(p_seg->seg_id_);
        return (it == seg_to_edge_.cend()) ? 0 : it->second;
    }

    bool SetSegEdgeRelations(const std::vector<SegEdgeRelation>& relations)
    {
        size_t num_rows = relations.size();
        seg_to_edge_.clear();
        edge_map_.clear();

        for (size_t i = 0; i < num_rows; ++i) {
            const SegEdgeRelation& r = relations[i];

            if (r.seg_id == 0 || r.edge_id == 0) continue;
            if (way_manager_.GetSegById(r.seg_id) == nullptr) continue;
            if (way_manager_.GetSegById(r.edge_id) == nullptr) continue;

            seg_to_edge_[r.seg_id] = r.edge_id;
            auto &edge = edge_map_[r.edge_id];
            edge.edge_id = r.edge_id;
            edge.edge_info.segs.push_back(EdgeSegItem());
            edge.edge_info.segs.back().seg_id = r.seg_id;
            edge.edge_info.segs.back().seq_no = r.seq_no;
        }

        // pupulate edge_map_ edge info related data
        for (auto& it : edge_map_) {
            auto& edge = it.second;
            std::sort(edge.edge_info.segs.begin(), edge.edge_info.segs.end(),
                [](const EdgeSegItem& i, const EdgeSegItem& j) {
                return i.seq_no < j.seq_no;
            });

            if (!edge.edge_info.segs.empty()) {
                auto from_seg = way_manager_.GetSegById(edge.edge_info.segs.front().seg_id);
                auto to_seg = way_manager_.GetSegById(edge.edge_info.segs.back().seg_id);
                if (from_seg) {
                    edge.edge_info.from_node_id = from_seg->from_nd_;;
                }
                if (to_seg) {
                    edge.edge_info.to_node_id = to_seg->to_nd_;
                }
            }
        }

        return !seg_to_edge_.empty();
    }

public:
    const TransMatrixGenerator::InParams in_params_;
    const geo::WayManager& way_manager_;
    std::vector<TransMatrixGenerator::FcdPoint> points_pool_;
    std::vector<FcdDevice> fcd_devices_;
    UNORDERED_MAP<geo::SEG_ID_T, geo::EDGE_ID_T> seg_to_edge_;
    EdgeMap edge_map_;
    std::vector<EdgeTrans> main_edges_; // main component edges
};

TransMatrixGenerator::TransMatrixGenerator(const InParams& in_params,
    const geo::WayManager& way_manager)
    : p_impl_(new impl(in_params, way_manager))
{}

TransMatrixGenerator::~TransMatrixGenerator()
{
    if (p_impl_) {
        delete p_impl_;
        p_impl_ = nullptr;
    }
}

bool TransMatrixGenerator::SetSegEdgeRelations(const std::vector<SegEdgeRelation>& relations)
{
    return p_impl_->SetSegEdgeRelations(relations);
}

bool TransMatrixGenerator::OnFcdData(std::vector<FcdPoint>&& fcd_points, std::string& err)
{
    p_impl_->points_pool_ = std::forward<std::vector<FcdPoint>>(fcd_points);
    p_impl_->OnFcdDataReady();
    return true;
}

std::vector<Transition> TransMatrixGenerator::GenerateTransMatrix(std::string& err)
{
    std::vector<Transition> transitions;
    bool ok = p_impl_->GenTransMatrix();
    if (!ok) {
        return transitions;
    }

    size_t row_count = 0;
    for (const auto& it : p_impl_->edge_map_) {
        row_count += it.second.trans_tos.size();
    }
    if (row_count == 0) {
        err = __FUNCTION__ + std::string(": empty transition matrix");
        return transitions;
    }

    transitions.reserve(row_count);
    size_t row = 0;
    for (const auto& it : p_impl_->edge_map_) {
        for (auto& trans_tos : it.second.trans_tos) {
            transitions.push_back(Transition());
            auto& trans = transitions.back();
            trans.from_edge_id = it.first;
            trans.to_edge_id = trans_tos.to_edge;
            trans.probability = trans_tos.probability;

            ++row;
        }
    }

    return transitions;
}

std::vector<Stationary> TransMatrixGenerator::GenerateStationaryMatrix(std::string& err)
{
    std::vector<Stationary> stationaries;
    bool ok = p_impl_->GenStationaryMatrix();
    if (!ok) {
        return stationaries;
    }

    size_t row_count = p_impl_->edge_map_.size();
    if (row_count == 0) {
        err = __FUNCTION__ + std::string(": empty stationary matrix");
        return stationaries;
    }

    stationaries.reserve(row_count);
    size_t row = 0;
    for (const auto& it : p_impl_->edge_map_) {
        stationaries.push_back(Stationary());
        auto& s = stationaries.back();
        s.edge_id = it.first;
        s.probability = it.second.stationary_prob;

        ++row;
    }

    return stationaries;
}

bool TransMatrixGenerator::ResultsToJson(const std::string& json_pathname) const
{
    return p_impl_->ResultsToJson(json_pathname);
}

TRANS_MATRIX_END_NAMESPACE
