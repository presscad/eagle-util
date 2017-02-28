
#include "trans_matrix_swig_core.h"
#include <atomic>
#include "common/common_utils.h"
#include "common/csv_to_tuples.hpp"


#define TRACK_OBJECTS   0
#define LOG(level)      std::cout

using namespace std;

class SwigTM_TransMatGen::SwigTM_TransMatGenImpl
{
public:
    SwigTM_TransMatGenImpl()
    {}

    ~SwigTM_TransMatGenImpl()
    {}

public:
    string err_;
};

SwigTM_TransMatGen::SwigTM_TransMatGen()
    : p_impl(new SwigTM_TransMatGenImpl())
{
}

SwigTM_TransMatGen::~SwigTM_TransMatGen()
{
    Reset();
}

bool SwigTM_TransMatGen::Init(const SwigTM_TransMatParams & params,
    const std::string & segments_csv, const std::string & seg_edges_csv,
    const std::string & excluded_routes_csv)
{
    if (nullptr == p_impl) {
        p_impl = new SwigTM_TransMatGenImpl();
    }


    return true;
}

bool SwigTM_TransMatGen::OnFcdData(const SwigTM_FcdPointVector& points_vector)
{
    if (points_vector.empty()) {
        return true;
    }
    if (nullptr == p_impl) {
        return false;
    }

    return true;
}

bool SwigTM_TransMatGen::OnFcdDataStart()
{
    return true;
}

bool SwigTM_TransMatGen::OnFcdPartialData(const SwigTM_FcdPointVector& points_vector,
    int size)
{
    return true;
}

bool SwigTM_TransMatGen::OnFcdDataEnd()
{
    return true;
}

bool SwigTM_TransMatGen::GenerateTransMatrix(SwigTM_TransitionMat& swig_trans_mat)
{
    if (nullptr == p_impl) {
        return false;
    }

    return true;
}

bool SwigTM_TransMatGen::GenerateStationaryMatrix(SwigTM_StationaryMat& swig_stationary_mat)
{
    if (nullptr == p_impl) {
        return false;
    }

    return true;
}

std::string SwigTM_TransMatGen::GetErrorStr() const
{
    if (nullptr == p_impl) {
        return "SwigTM_TransMatGen: cannot get impl instance";
    }
    return p_impl->err_;
}

void SwigTM_TransMatGen::Reset()
{
    if (p_impl) {
        delete p_impl;
        p_impl = nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// class SwigTM_FcdPoint

SwigTM_FcdPoint::SwigTM_FcdPoint()
{
#if TRACK_OBJECTS == 1
    ++swig_fcd_point_count_allocated;
#endif
}

SwigTM_FcdPoint::~SwigTM_FcdPoint()
{
#if TRACK_OBJECTS == 1
    ++swig_fcd_point_count_released;
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// class SwigTM_FcdPointVector

SwigTM_FcdPointVector::SwigTM_FcdPointVector()
{
#if TRACK_OBJECTS == 1
    ++swig_fcd_vector_count_allocated;
#endif
}

SwigTM_FcdPointVector::~SwigTM_FcdPointVector()
{
#if TRACK_OBJECTS == 1
    ++swig_fcd_vector_count_released;
#endif
}
