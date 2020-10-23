#ifndef _HARRIS_HPP
#define _HARRIS_HPP

#include "mve/image_io.h"
#include "mve/image_tools.h"

struct Derivatives {
    mve::FloatImage::Ptr Ix;
    mve::FloatImage::Ptr Iy;
    mve::FloatImage::Ptr Ixy;
};

class Harris {
public:
    Harris(float k, int filter_range, bool gauss);

    void SetImage(const mve::ByteImage::ConstPtr& img);

    void Process();

    Derivatives ComputeDerivatives();

    static void ApplyGaussToDerivatives(Derivatives &d, int filter_range, float sigma);

    static void ApplyMeanToDerivatives(Derivatives &d, int filter_range);
private:
    float m_k;
    int m_filter_range;
    bool m_gauss;

    mve::FloatImage::ConstPtr m_orig; // Original input image
};


#endif //_HARRIS_HPP
