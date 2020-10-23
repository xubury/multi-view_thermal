#include "feature/Harris.hpp"
#include "util.hpp"

Harris::Harris(float k, int filter_range, bool gauss)
        : m_k(k), m_filter_range(filter_range), m_gauss(gauss) {

}

void Harris::SetImage(const mve::ByteImage::ConstPtr& img) {
    if (img->channels() != 1 && img->channels() != 3)
        throw std::invalid_argument("Gray or color image expected");

    this->m_orig = mve::image::byte_to_float_image(img);
    if (img->channels() == 3) {
        this->m_orig = mve::image::desaturate<float>
                (this->m_orig, mve::image::DESATURATE_AVERAGE);
    }
}

void Harris::Process() {
    Derivatives d = ComputeDerivatives();
    if (m_gauss) {
        ApplyGaussToDerivatives(d, m_filter_range, 1);
    } else {
        ApplyMeanToDerivatives(d, m_filter_range);
    }
}

Derivatives Harris::ComputeDerivatives() {
    mve::FloatImage::Ptr sobel_vertical = mve::FloatImage::create(m_orig->width(), m_orig->height() - 2, 1);
    for (int y = 1; y < sobel_vertical->height() - 1; ++y) {
        for (int x = 0; x < sobel_vertical->width(); ++x) {
            float a1 = m_orig->at(x, y - 1, 0);
            float a2 = m_orig->at(x, y, 0);
            float a3 = m_orig->at(x, y + 1, 0);
            sobel_vertical->at(x, y - 1, 0) = a1 + a2 + a2 + a3;
        }
    }
    mve::FloatImage::Ptr sobel_horizontal = mve::FloatImage::create(m_orig->width() - 2, m_orig->height(), 1);
    for (int y = 0; y < sobel_horizontal->height(); ++y) {
        for (int x = 1; x < sobel_horizontal->width() - 1; ++x) {
            float a1 = m_orig->at(x - 1, y, 0);
            float a2 = m_orig->at(x, y, 0);
            float a3 = m_orig->at(x + 1, y, 0);
            sobel_horizontal->at(x - 1, y, 0) = a1 + a2 + a2 + a3;
        }
    }
    // Apply Sobel filter to compute 1st derivatives
    mve::FloatImage::Ptr Ix = mve::FloatImage::create(m_orig->width() - 2, m_orig->height() - 2, 1);
    mve::FloatImage::Ptr Iy = mve::FloatImage::create(m_orig->width() - 2, m_orig->height() - 2, 1);
    mve::FloatImage::Ptr Ixy = mve::FloatImage::create(m_orig->width() - 2, m_orig->height() - 2, 1);

    for (int y = 0; y < m_orig->height() - 2; y++) {
        for (int x = 0; x < m_orig->width() - 2; x++) {
            Ix->at(x, y, 0) = sobel_horizontal->at(x, y, 0) - sobel_horizontal->at(x, y + 2, 0);
            Iy->at(x, y, 0) = -sobel_vertical->at(x, y, 0) + sobel_vertical->at(x + 2, y, 0);
            Ixy->at(x, y, 0) = Ix->at(x, y, 0) * Iy->at(x, y, 0);
        }
    }
    Derivatives d;
    d.Ix = Ix;
    d.Iy = Iy;
    d.Ixy = Ixy;
    return d;
}

void Harris::ApplyGaussToDerivatives(Derivatives &d, int filter_range, float sigma) {
    if (filter_range == 0)
        return;

    d.Ix = util::GaussFilter(d.Ix, filter_range, sigma);
    d.Iy = util::GaussFilter(d.Iy, filter_range, sigma);
    d.Ixy = util::GaussFilter(d.Ixy, filter_range, sigma);
}

void Harris::ApplyMeanToDerivatives(Derivatives &d, int filter_range) {
    if (filter_range == 0)
        return;

    d.Ix = util::MeanFilter(d.Ix, filter_range);
    d.Iy = util::MeanFilter(d.Ix, filter_range);
    d.Ixy = util::MeanFilter(d.Ixy, filter_range);
}
