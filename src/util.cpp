#include <Image.hpp>
#include "util.hpp"
#include <algorithm>

namespace util {

mve::FloatImage::Ptr GaussFilter(const mve::FloatImage::ConstPtr &img, int filter_range, float sigma) {
    mve::FloatImage::Ptr gauss = mve::FloatImage::create(img->width() - filter_range * 2,
                                                          img->height() - filter_range * 2, 1);
    int ksize = filter_range * 2 + 1;
    std::vector<double> gauss_template(ksize * ksize);
    double sum = 0;
    for (int b = 0; b <= 2 * filter_range; b++)
    {
        int y = b - filter_range;
        for (int a = 0; a <= 2 * filter_range; a++)
        {
            int x = a - filter_range;
            double m = std::exp(-(x * x + y * y) / (2 * sigma * sigma));
            gauss_template[a + b * ksize] = m;
            sum += m;
        }
    }
    std::transform(gauss_template.begin(),
                   gauss_template.end(),
                   gauss_template.begin(),
                   [&sum](auto &c) { return c / sum; });
#pragma omp parallel for schedule(dynamic, 1)
    for (int y = filter_range; y < img->height() - filter_range; ++y) {
#pragma omp parallel for schedule(dynamic, 1)
        for (int x = filter_range; x < img->width() - filter_range; ++x) {
            double res = 0;

            for (int b = -filter_range; b <= filter_range; b++)
            {
                for (int a = -filter_range; a <= filter_range; a++)
                {
                    res += gauss_template.at(a + filter_range + (b + filter_range) * ksize)
                        * img->at(x + a, y + b, 0);
                }
            }
            gauss->at(x - filter_range, y - filter_range, 0) = res;
        }
    }
    return gauss;
}

mve::FloatImage::Ptr ComputeIntegralImg(const mve::FloatImage::ConstPtr &img) {
    mve::FloatImage::Ptr integral_img = mve::FloatImage::create(img->width() + 1, img->height() + 1, 1);

    for (int r = 0; r < img->height(); r++) {
        float sum = 0;
        for (int c = 0; c < img->width(); c++) {
            sum += img->at(c, r, 0);
            integral_img->at(c + 1, r + 1, 0) = integral_img->at(c + 1, r, 0) + sum;
        }
    }
    return integral_img;
}

mve::FloatImage::Ptr MeanFilter(const mve::FloatImage::ConstPtr &img, int filter_range) {
    mve::FloatImage::Ptr medianFilteredImg =
        mve::FloatImage::create(img->width() - 2 * filter_range, img->height() - 2 * filter_range, 1);
    mve::FloatImage::Ptr integral_img = ComputeIntegralImg(img);
    int k_size = 2 * filter_range + 1;
    float normal = 1.f  / (k_size * k_size);
#pragma omp parallel for schedule(dynamic, 1)
    for (int r = filter_range + 1; r < img->height() - filter_range + 1; r++) {
#pragma omp parallel for schedule(dynamic, 1)
        for (int c = filter_range + 1; c < img->width() - filter_range + 1 ; c++) {
            float sum = integral_img->at(c + filter_range, r + filter_range, 0)
                    + integral_img->at(c - filter_range - 1, r - filter_range - 1, 0)
                    - integral_img->at(c + filter_range, r - filter_range - 1, 0)
                    - integral_img->at(c - filter_range - 1, r + filter_range, 0);
            medianFilteredImg->at(c - filter_range - 1, r - filter_range - 1, 0) = sum * normal;
        }
    }
    return medianFilteredImg;
}

} // namespace util

