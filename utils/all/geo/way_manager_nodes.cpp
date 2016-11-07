
#include <algorithm>
#include <memory>
#include "way_manager.h"
#include "common/simple_matrix.hpp"
#if WAY_MANAGER_HANA_LOG == 1
#include <hana/logging.h>
#include <chrono>
#endif


namespace geo {
namespace node {

typedef long long TILE_XY;

static const double CELL_SIZE = 200; // in meters

struct NodeTile
{
    TILE_XY tile_id;
    std::vector<NodePtr> nodes;
    std::vector<NodePtr> nodes_with_neighbours;

    NodeTile() : tile_id(0)
    {
        nodes.reserve(8);
    }

    explicit NodeTile(TILE_XY tile_id) : tile_id(tile_id)
    {
        nodes.reserve(8);
    }

    void AddNode(const NodePtr& p_node)
    {
        nodes.push_back(p_node);
    }
};
typedef NodeTile* NodeTilePtr;


class NodeManager
{
public:
    typedef WayManager::NODE_SEARCH_RESULT NODE_SEARCH_RESULT;

    explicit NodeManager(const Bound& bound)
        : bound_(bound)
    {
        GRID_CELL_ZOOM_LEVEL = geo::span_to_zoom_level(CELL_SIZE,
            (bound.minlat + bound.maxlat) / 2);

        min_tile_x_ = geo::long2tilex(bound.minlng, GRID_CELL_ZOOM_LEVEL);
        min_tile_y_ = geo::lat2tiley(bound.maxlat, GRID_CELL_ZOOM_LEVEL);
        int max_tile_x = geo::long2tilex(bound.maxlng, GRID_CELL_ZOOM_LEVEL);
        int max_tile_y = geo::lat2tiley(bound.minlat, GRID_CELL_ZOOM_LEVEL);
        mat_width_ = max_tile_x - min_tile_x_ + 1;
        mat_height_ = max_tile_y - min_tile_y_ + 1;

        node_tile_mat_.SetSize(mat_height_, mat_width_);
        for (int y = 0; y < mat_height_; ++y) {
            for (int x = 0; x < mat_width_; ++x) {
                TILE_XY tile_id = geo::make_tilexy(x + min_tile_x_, y + min_tile_y_);
                node_tile_mat_(y, x).tile_id = tile_id;
            }
        }
    }

    bool LoadToTileMatrix(const std::vector<NODE>& nodes)
    {
        this->node_pool_.Reserve(nodes.size());

        for (const auto& nd : nodes) {
            if (InMaxMin(nd.geo_point.lat, nd.geo_point.lng)) {
                auto&& p_node = this->node_pool_.AllocNew(Node(nd));
                AddNode(p_node);
            }
        }

        InitNeighbourNodes();
        return true;
    }

    const std::string& GetErrorString() const
    {
        return WayManager::GetCurThreadErrStr(threads_err_mutex_, threads_err_strs_);
    }

    bool FindAdjacentNodes(const geo::GeoPoint& pos, double radius, bool has_name,
        std::vector<NODE_SEARCH_RESULT>& results) const
    {
        results.clear();

        NodeTilePtr p_tile = GetTileByPos(pos);
        if (!p_tile) {
            WayManager::SetCurThreadErrStr(threads_err_mutex_, threads_err_strs_,
                "FindAdjacentNodes: tile not found");
            return true;
        }

        for (const auto& p_node : p_tile->nodes_with_neighbours) {
            if (has_name) {
                if (p_node->nd_name_.empty()) {
                    continue;
                }
            }

            double distance = geo::distance_in_meter(pos, p_node->geo_point_);
            if (distance <= radius) {
                results.push_back(std::make_tuple(p_node, distance));
            }
        }

        // sort by distance
        std::sort(results.begin(), results.end(),
            [](const NODE_SEARCH_RESULT& i, const NODE_SEARCH_RESULT& j) {
            return std::get<1>(i) < std::get<1>(j);
        });

        return true;
    }

private:
    void AddNode(const NodePtr &p_node)
    {
        all_nodes_map_.insert(NodeMap::value_type(p_node->nd_id_, p_node));

        TILE_XY xy1 = PosToTileId(p_node->geo_point_);
        NodeTilePtr p_tile1 = GetTileById(xy1);
        if (p_tile1) {
            p_tile1->AddNode(p_node);
        }
    }

    bool InMaxMin(double lat, double lng) const
    {
        return (lat >= bound_.minlat && lat <= bound_.maxlat &&
            lng >= bound_.minlng && lng <= bound_.maxlng);
    }

    NodeTilePtr GetTileById(TILE_XY tile_id) const
    {
        int x = geo::tilexy2tilex(tile_id);
        int y = geo::tilexy2tiley(tile_id);
        if (x >= min_tile_x_ && x < min_tile_x_ + mat_width_ &&
            y >= min_tile_y_ && y < min_tile_y_ + mat_height_) {
            return (NodeTilePtr)&node_tile_mat_(y - min_tile_y_, x - min_tile_x_);
        }
        return nullptr;
    }

    NodeTilePtr GetTileByPos(const geo::GeoPoint& pos) const
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

    void InitNeighbourNodes()
    {
        for (int y = 0; y < mat_height_; ++y) {
            for (int x = 0; x < mat_width_; ++x) {
                NodeTile& tile = node_tile_mat_(y, x);
                std::vector<NodePtr> nodes_with_neighbours = tile.nodes;

                // get all the neighbours
                int x0 = geo::tilexy2tilex(tile.tile_id);
                int y0 = geo::tilexy2tiley(tile.tile_id);
                TILE_XY neighbours[] = {
                    geo::make_tilexy(x0-1, y0+1), geo::make_tilexy(x0, y0+1), geo::make_tilexy(x0+1, y0+1),
                    geo::make_tilexy(x0-1, y0  ),                             geo::make_tilexy(x0+1, y0  ),
                    geo::make_tilexy(x0-1, y0-1), geo::make_tilexy(x0, y0-1), geo::make_tilexy(x0+1, y0-1)
                };

                for (int i = 0; i < 8; ++i) {
                    NodeTilePtr the_tile = this->GetTileById(neighbours[i]);
                    if (the_tile && !the_tile->nodes.empty()) {
                        nodes_with_neighbours.insert(nodes_with_neighbours.end(),
                            the_tile->nodes.begin(), the_tile->nodes.end());
                    }
                }

                // remove duplicated
                for (int i = (int)nodes_with_neighbours.size() - 1; i > 0; --i) {
                    for (int j = 0; j < i; ++j) {
                        if (nodes_with_neighbours[i] == nodes_with_neighbours[j]) {
                            nodes_with_neighbours[i] = nullptr;
                            break;
                        }
                    }
                }

                // get the valid count
                int valid_count = 0;
                for (size_t i = 0; i < nodes_with_neighbours.size(); ++i) {
                    if (nodes_with_neighbours[i]) {
                        ++valid_count;
                    }
                }

                // fill in tile.nodes_with_neighbours
                tile.nodes_with_neighbours.clear();
                tile.nodes_with_neighbours.reserve(valid_count);
                for (size_t i = 0; i < nodes_with_neighbours.size(); ++i) {
                    if (nodes_with_neighbours[i]) {
                        tile.nodes_with_neighbours.push_back(nodes_with_neighbours[i]);
                    }
                }
            }
        }
    }

private:
    NodeMap all_nodes_map_;
    util::SimpleObjPool<Node> node_pool_;

    Bound bound_;
    int min_tile_x_, min_tile_y_, mat_width_, mat_height_;
    SimpleMatrix<NodeTile> node_tile_mat_;
    mutable std::mutex threads_err_mutex_;
    mutable UNORD_MAP<std::string, std::string> threads_err_strs_;

    double GRID_CELL_ZOOM_LEVEL;
    friend class WayManager;
};
} // namespace node

///////////////////////////////////////////////////////////////////////////////////////////////////
// class WayManager

bool WayManager::InitForNodeLocating(const Bound& bound, const std::vector<NODE>& nodes)
{
    p_node_manager_ = std::make_shared<node::NodeManager>(bound);

    if (false == p_node_manager_->LoadToTileMatrix(nodes)) {
        SetErrorString(p_node_manager_->GetErrorString());
        return false;
    }
    return p_node_manager_ != NULL;
}

bool WayManager::FindAdjacentNodes(const geo::GeoPoint& point, double radius, bool has_name,
    std::vector<NODE_SEARCH_RESULT>& results) const
{
    if (false == p_node_manager_->FindAdjacentNodes(point, radius, has_name, results)) {
        SetErrorString(p_node_manager_->GetErrorString());
        return false;
    }
    return true;
}

bool WayManager::FindAdjacentNodes(double lat, double lng, double radius, bool has_name,
    std::vector<NODE_SEARCH_RESULT>& results) const
{
    if (false == p_node_manager_->FindAdjacentNodes(geo::GeoPoint(lat, lng), radius,
        has_name, results)) {
        SetErrorString(p_node_manager_->GetErrorString());
        return false;
    }
    return true;
}

} // namespace geo
