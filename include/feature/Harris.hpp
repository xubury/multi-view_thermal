#ifndef _HARRIS_HPP
#define _HARRIS_HPP

#include "mve/image_io.h"
#include "mve/image_tools.h"

class Harris {
public:
    struct Derivatives {
        mve::FloatImage::Ptr Ix;
        mve::FloatImage::Ptr Iy;
        mve::FloatImage::Ptr Ixy;
    };

    struct KeyPoint {
        int x;
        int y;
        float corner_response;
        KeyPoint(int xx, int yy, float response) : x(xx), y(yy), corner_response(response) {}
    };

    using KeyPoints = std::vector<KeyPoint>;
public:
    Harris(float k, int filter_range, bool gauss);

    void SetImage(const mve::ByteImage::ConstPtr &img);

    void Process();

    KeyPoints GetMaximaPoints(float percentage, int suppression_radius);
private:
    Derivatives ComputeDerivatives();

    void ApplyGaussToDerivatives(Derivatives &d, float sigma);

    void ApplyMeanToDerivatives(Derivatives &d);

    void ComputeHarrisResponses(const Derivatives &d);
private:
    float m_k;
    int m_filter_range;
    bool m_gauss;

    mve::FloatImage::ConstPtr m_orig; // Original input image
    mve::FloatImage::Ptr m_harris_responses;
};

#endif //_HARRIS_HPP
