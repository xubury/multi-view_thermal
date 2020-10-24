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
    Harris(float k, int filter_range, float sigma);

    void SetImage(const mve::ByteImage::ConstPtr &img);

    void Process();

    KeyPoints GetMaximaPoints(float percentage, int suppression_radius);
private:
    Derivatives ComputeDerivatives();

    void ApplyGaussToDerivatives(float sigma);

    void ApplyMeanToDerivatives();

    void ComputeHarrisResponses();
private:
    float m_k;
    int m_filter_range;
    float m_sigma;

    mve::FloatImage::ConstPtr m_orig; // Original input image
    mve::FloatImage::Ptr m_harris_responses;
    Derivatives m_derivatives;
};

#endif //_HARRIS_HPP
