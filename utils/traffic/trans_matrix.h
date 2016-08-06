#ifndef _TRAFFIC_TRANS_MATRIX_H_
#define _TRAFFIC_TRANS_MATRIX_H_

#include <vector>
#include <string>
#include "geo/way_manager.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// external API

namespace traffic {

class TransMatrixGenerator
{
public:
    struct InParams
    {
        geo::Bound bound;
        int time_point_type{};         // 0: local, 1: utc
        int passenger_state_filter{};  // 0: empty, 1 : loaded, 2 : both
        bool fcd_gps_order_by_devid{}; // indicates whether FCD_GPS is ordered by DEVID
    };

    struct SegEdgeRelation
    {
        geo::SEG_ID_T seg_id;
        geo::EDGE_ID_T edge_id;
        int seq_no;
    };

    struct FcdPoint
    {
        std::string   dev_id;
        geo::GeoPoint geo_point;
        float         speed{};
        time_t        gps_time{};
        short         heading{};
        uint8_t       passenger_state{};
    };

    struct Transition
    {
        geo::EDGE_ID_T from_edge_id;
        geo::EDGE_ID_T to_edge_id;
        double probability;
    };

    struct Stationary
    {
        geo::EDGE_ID_T edge_id;
        double probability;
    };

public:
    explicit TransMatrixGenerator(const InParams& in_params, const geo::WayManager& way_manager);
    ~TransMatrixGenerator();

    bool SetSegEdgeRelations(const std::vector<SegEdgeRelation>& relations);
    bool OnFcdData(std::vector<FcdPoint>&& fcd_points, std::string& err);

    std::vector<Transition> GenerateTransMatrix(std::string& err);
    std::vector<Stationary> GenerateStationaryMatrix(std::string& err);
    bool ResultsToJson(const std::string& json_pathname) const;

private:
    class impl;
    impl* p_impl_;
};

}

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif // _TRAFFIC_TRANS_MATRIX_H_
