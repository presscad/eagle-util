#ifndef _SWIG_TEST_CORE_H
#define _SWIG_TEST_CORE_H

#include <string>
#include <vector>


struct SwigCoreParams
{
    double min_lat;
    double max_lat;
    double min_lng;
    double max_lng;

    int time_point_type; // 0 - local time, 1 - GMT
    int passenger_state_filter; // 0: empty, 1 : loaded, 2 : both
    bool fcd_gps_order_by_devid{}; // indicates whether FCD_GPS is ordered by DEVID
};

struct SwigCorePoint
{
    SwigCorePoint();
    ~SwigCorePoint();

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

struct SwigCorePointVector
{
    SwigCorePointVector();
    ~SwigCorePointVector();

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

    void add(const SwigCorePoint& fcd_point)
    {
        data_.push_back(fcd_point);
    }

    const SwigCorePoint& get(int i) const
    {
        return data_[i];
    }

    const std::vector<SwigCorePoint>& data() const
    {
        return data_;
    }

private:
    std::vector<SwigCorePoint> data_;
};

class SwigTestCore
{
public:
    SwigTestCore();
    ~SwigTestCore();

    bool Init(const SwigCoreParams& params, const std::string& segments_csv,
        const std::string& seg_edges_csv, const std::string& ex_routes_csv);

    // all data in one time slot (e.g., 1 hour)
    bool OnFcdData(const SwigCorePointVector& fcd_points);

    std::string GetErrorStr() const;
    void Reset();

private:
    class SwigCoreImpl;
    SwigCoreImpl *p_impl{};
};

#endif // _SWIG_TEST_CORE_H
