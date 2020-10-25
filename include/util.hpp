#ifndef _UTIL_HPP
#define _UTIL_HPP

#include "mve/image_io.h"

namespace util {

void GaussFilter(const mve::FloatImage::ConstPtr &img,
                 mve::FloatImage::Ptr &out,
                 int filter_range,
                 float sigma);

mve::FloatImage::Ptr ComputeIntegralImg(const mve::FloatImage::ConstPtr &img);

void MeanFilter(const mve::FloatImage::ConstPtr &img, mve::FloatImage::Ptr &out, int filter_range);
} // namespace util

#endif //_UTIL_HPP
