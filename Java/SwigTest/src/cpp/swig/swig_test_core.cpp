
#include "swig_test_core.h"
#include <atomic>
#include "common/common_utils.h"
#include "common/csv_to_tuples.hpp"


#define TRACK_OBJECTS   0
#define LOG(level)      std::cout

using namespace std;

class SwigTestCore::SwigCoreImpl
{
public:
    SwigCoreImpl()
    {}

    ~SwigCoreImpl()
    {}

public:
    string err_;
};

SwigTestCore::SwigTestCore()
    : p_impl(new SwigCoreImpl())
{
}

SwigTestCore::~SwigTestCore()
{
    Reset();
}

bool SwigTestCore::Init(const SwigCoreParams & params,
    const std::string & segments_csv, const std::string & seg_edges_csv,
    const std::string & excluded_routes_csv)
{
    if (nullptr == p_impl) {
        p_impl = new SwigCoreImpl();
    }

    return true;
}

bool SwigTestCore::OnFcdData(const SwigCorePointVector& points_vector)
{
    if (points_vector.empty()) {
        return true;
    }
    if (nullptr == p_impl) {
        return false;
    }

    return true;
}

std::string SwigTestCore::GetErrorStr() const
{
    if (nullptr == p_impl) {
        return "SwigTM_TransMatGen: cannot get impl instance";
    }
    return p_impl->err_;
}

void SwigTestCore::Reset()
{
    if (p_impl) {
        delete p_impl;
        p_impl = nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// class SwigTM_FcdPoint

SwigCorePoint::SwigCorePoint()
{
#if TRACK_OBJECTS == 1
    ++swig_fcd_point_count_allocated;
#endif
}

SwigCorePoint::~SwigCorePoint()
{
#if TRACK_OBJECTS == 1
    ++swig_fcd_point_count_released;
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// class SwigTM_FcdPointVector

SwigCorePointVector::SwigCorePointVector()
{
#if TRACK_OBJECTS == 1
    ++swig_fcd_vector_count_allocated;
#endif
}

SwigCorePointVector::~SwigCorePointVector()
{
#if TRACK_OBJECTS == 1
    ++swig_fcd_vector_count_released;
#endif
}
