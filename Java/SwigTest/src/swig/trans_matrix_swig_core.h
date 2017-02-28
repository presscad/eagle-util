#ifndef _TRANS_MATRIX_SWIG_CORE_H
#define _TRANS_MATRIX_SWIG_CORE_H

#include <string>
#include <vector>


struct SwigTM_TransMatParams
{
    double min_lat;
    double max_lat;
    double min_lng;
    double max_lng;

    int time_point_type; // 0 - local time, 1 - GMT
    int passenger_state_filter; // 0: empty, 1 : loaded, 2 : both
    bool fcd_gps_order_by_devid{}; // indicates whether FCD_GPS is ordered by DEVID
};

struct SwigTM_FcdPoint
{
    SwigTM_FcdPoint();
    ~SwigTM_FcdPoint();

    void SetMembers(const std::string& dev_id, double lat, double lng, float speed,
        const std::string& gps_time, short heading, unsigned char passenger_state)
    {
        this->dev_id = dev_id;
        this->lat = lat;
        this->lng = lng;
        this->speed = speed;
        this->gps_time = gps_time;
        this->heading = heading;
        this->passenger_state = passenger_state;
    }

    std::string dev_id;
    double      lat;
    double      lng;
    float       speed;
    std::string gps_time;
    short       heading;
    unsigned char passenger_state;
};

struct SwigTM_FcdPointVector
{
    SwigTM_FcdPointVector();
    ~SwigTM_FcdPointVector();

    bool empty() const
    {
        return data_.empty();
    }
   
    int size() const
    {
        return (int)data_.size();
    }

    void reserve(int n)
    {
        data_.reserve(n);
    }

    void clear()
    {
        data_.clear();
    }

    void add(const SwigTM_FcdPoint& fcd_point)
    {
        data_.push_back(fcd_point);
    }

    const SwigTM_FcdPoint& get(int i) const
    {
        return data_[i];
    }

    const std::vector<SwigTM_FcdPoint>& data() const
    {
        return data_;
    }

private:
    std::vector<SwigTM_FcdPoint> data_;
};

struct SwigTM_Transition
{
    long long   from_edge_id;
    long long   to_edge_id;
    double      probability;
};

struct SwigTM_TransitionMat
{
    SwigTM_TransitionMat()
    {}
    ~SwigTM_TransitionMat()
    {
        data.clear();
    }

    void reserve(int n)
    {
        data.reserve(n);
    }
    void clear()
    {
        data.clear();
    }
    void add(const SwigTM_Transition& t)
    {
        data.push_back(t);
    }
    int size() const
    {
        return (int)data.size();
    }
    const SwigTM_Transition& get(int i) const
    {
        return data[i];
    }

private:
    std::vector<SwigTM_Transition> data;
};

struct SwigTM_Stationary
{
    long long   edge_id;
    double      probability;
};

struct SwigTM_StationaryMat
{
    SwigTM_StationaryMat()
    {}
    ~SwigTM_StationaryMat()
    {
        data.clear();
    }

    void reserve(int n)
    {
        data.reserve(n);
    }
    void clear()
    {
        data.clear();
    }
    void add(const SwigTM_Stationary& t)
    {
        data.push_back(t);
    }
    int size() const
    {
        return (int)data.size();
    }
    const SwigTM_Stationary& get(int i) const
    {
        return data[i];
    }

private:
    std::vector<SwigTM_Stationary> data;
};

class SwigTM_TransMatGen
{
public:
    SwigTM_TransMatGen();
    ~SwigTM_TransMatGen();

    bool Init(const SwigTM_TransMatParams& params, const std::string& segments_csv,
        const std::string& seg_edges_csv, const std::string& ex_routes_csv);

    // all data in one time slot (e.g., 1 hour)
    bool OnFcdData(const SwigTM_FcdPointVector& fcd_points);

    bool OnFcdDataStart();
    // data in one time slot (e.g., 1 hour) splited into multiple parts
    bool OnFcdPartialData(const SwigTM_FcdPointVector& fcd_points, int size);
    bool OnFcdDataEnd();

    bool GenerateTransMatrix(SwigTM_TransitionMat &trans_mat);
    bool GenerateStationaryMatrix(SwigTM_StationaryMat& stationary_mat);
    std::string GetErrorStr() const;
    void Reset();

    // for testing
    long long GetEdgeIdByPos(double lat, double lng, int heading);

private:
    class SwigTM_TransMatGenImpl;
    SwigTM_TransMatGenImpl *p_impl{};
};

#endif // _TRANS_MATRIX_SWIG_CORE_H
