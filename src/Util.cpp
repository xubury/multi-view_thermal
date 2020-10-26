#include <Image.hpp>
#include "Util.hpp"
#include <algorithm>

namespace util {

glm::mat4 MveToGLMatrix(const glm::mat4 &matrix) {
    glm::mat4 out;
    out = glm::transpose(matrix);
    out[1] = -out[1];
    out[2] = -out[2];
    return out;
}

void GaussFilter(const mve::FloatImage::ConstPtr &img,
                 mve::FloatImage::Ptr &out,
                 int filter_range,
                 float sigma) {
    assert(img->channels() == 1);

    out = mve::FloatImage::create(img->width() - filter_range * 2, img->height() - filter_range * 2, 1);

    int k_size = filter_range * 2 + 1;
    std::vector<double> gauss_template(k_size);
    double sum = 0;
    for (int a = 0; a < k_size; a++) {
        int x = a - filter_range;
        double m = std::exp((-1.0f * x * x) / (2.f * sigma * sigma));
        gauss_template[a] = m;
        sum += m;
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
            for (int a = -filter_range; a <= filter_range; a++) {
                res += gauss_template.at(a + filter_range) * img->at(x, y + a, 0);
            }
            out->at(x - filter_range, y - filter_range, 0) = res;
        }
    }
#pragma omp parallel for schedule(dynamic, 1)
    for (int y = filter_range; y < img->height() - filter_range; ++y) {
#pragma omp parallel for schedule(dynamic, 1)
        for (int x = filter_range; x < img->width() - filter_range; ++x) {
            double res = 0;
            for (int a = -filter_range; a <= filter_range; a++) {
                res += gauss_template.at(a + filter_range) * out->at(x - filter_range + a, y - filter_range, 0);
            }
            out->at(x - filter_range, y - filter_range, 0) = res;
        }
    }
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

void MeanFilter(const mve::FloatImage::ConstPtr &img, mve::FloatImage::Ptr &out, int filter_range) {
    out = mve::FloatImage::create(img->width() - 2 * filter_range, img->height() - 2 * filter_range, 1);
    mve::FloatImage::Ptr integral_img = ComputeIntegralImg(img);
    int k_size = 2 * filter_range + 1;
    float normal = 1.f / (k_size * k_size);
#pragma omp parallel for schedule(dynamic, 1)
    for (int r = filter_range + 1; r < img->height() - filter_range + 1; r++) {
#pragma omp parallel for schedule(dynamic, 1)
        for (int c = filter_range + 1; c < img->width() - filter_range + 1; c++) {
            float sum = integral_img->at(c + filter_range, r + filter_range, 0)
                + integral_img->at(c - filter_range - 1, r - filter_range - 1, 0)
                - integral_img->at(c + filter_range, r - filter_range - 1, 0)
                - integral_img->at(c - filter_range - 1, r + filter_range, 0);
            out->at(c - filter_range - 1, r - filter_range - 1, 0) = sum * normal;
        }
    }
}

} // namespace util

