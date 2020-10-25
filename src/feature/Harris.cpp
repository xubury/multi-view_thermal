#include "feature/Harris.hpp"
#include "util.hpp"

Harris::Harris(float k, int filter_range, float sigma)
    : m_k(k), m_filter_range(filter_range), m_sigma(sigma) {

}

void Harris::SetImage(const mve::ByteImage::ConstPtr &img) {
    if (img->channels() != 1 && img->channels() != 3)
        throw std::invalid_argument("Gray or color image expected");

    this->m_orig = mve::image::byte_to_float_image(img);
    if (img->channels() == 3) {
        this->m_orig = mve::image::desaturate<float>
            (this->m_orig, mve::image::DESATURATE_AVERAGE);
    }
}

void Harris::Process() {
    m_derivatives = ComputeDerivatives();
    if (m_sigma > 1e-7) {
        ApplyGaussToDerivatives(1);
    } else {
        ApplyMeanToDerivatives();
    }
    ComputeHarrisResponses();
}

Harris::Derivatives Harris::ComputeDerivatives() {
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

void Harris::ApplyGaussToDerivatives(float sigma) {
    if (m_filter_range == 0)
        return;

    util::GaussFilter(m_derivatives.Ix, m_derivatives.Ix, m_filter_range, sigma);
    util::GaussFilter(m_derivatives.Iy, m_derivatives.Iy, m_filter_range, sigma);
    util::GaussFilter(m_derivatives.Ixy, m_derivatives.Ixy, m_filter_range, sigma);
}

void Harris::ApplyMeanToDerivatives() {
    if (m_filter_range == 0)
        return;

    util::MeanFilter(m_derivatives.Ix, m_derivatives.Ix, m_filter_range);
    util::MeanFilter(m_derivatives.Ix, m_derivatives.Iy, m_filter_range);
    util::MeanFilter(m_derivatives.Ixy, m_derivatives.Ixy, m_filter_range);
}

void Harris::ComputeHarrisResponses() {
    m_harris_responses = mve::FloatImage::create(m_derivatives.Ix->width(), m_derivatives.Iy->height(), 1);
    for (int r = 0; r < m_derivatives.Iy->height(); r++) {
        for (int c = 0; c < m_derivatives.Iy->width(); c++) {
            float a11, a12, a21, a22;

            a11 = m_derivatives.Ix->at(c, r, 0) * m_derivatives.Ix->at(c, r, 0);
            a22 = m_derivatives.Iy->at(c, r, 0) * m_derivatives.Iy->at(c, r, 0);
            a21 = m_derivatives.Ix->at(c, r, 0) * m_derivatives.Iy->at(c, r, 0);
            a12 = m_derivatives.Ix->at(c, r, 0) * m_derivatives.Iy->at(c, r, 0);

            float det = a11 * a22 - a12 * a21;
            float trace = a11 + a22;

            m_harris_responses->at(c, r, 0) = std::abs(det - m_k * trace * trace);
        }
    }
}

Harris::KeyPoints Harris::GetMaximaPoints(float percentage, int suppression_radius) {
    mve::FloatImage::Ptr
        maxima_suppression_img = mve::FloatImage::create(m_harris_responses->width(), m_harris_responses->height(), 1);

    KeyPoints points;
    for (int r = 0; r < m_harris_responses->height(); ++r) {
        for (int c = 0; c < m_harris_responses->width(); ++c) {
            points.emplace_back(c, r, m_harris_responses->at(c, r, 0));
        }
    }

    std::sort(points.begin(),
              points.end(),
              [](const KeyPoint &pt1, const KeyPoint &pt2) {
                return pt1.corner_response > pt2.corner_response;
              });

    std::size_t top_points_cnt = points.size() * percentage;
    KeyPoints maxima_points;
    std::size_t i = 0;
    while (maxima_points.size() < top_points_cnt) {
        if (i == points.size()) {
            break;
        }

        int sup_rows = maxima_suppression_img->height();
        int sup_cols = maxima_suppression_img->width();

        // Check if point marked in maximaSuppression matrix
        if (maxima_suppression_img->at(points[i].x, points[i].y, 0) == 0) {
            for (int r = -suppression_radius; r <= suppression_radius; r++) {
                for (int c = -suppression_radius; c <= suppression_radius; c++) {
                    int sx = points[i].x + c;
                    int sy = points[i].y + r;

                    // bound checking
                    if (sx >= sup_cols)
                        sx = sup_cols - 1;
                    if (sx < 0)
                        sx = 0;
                    if (sy >= sup_rows)
                        sy = sup_rows - 1;
                    if (sy < 0)
                        sy = 0;

                    maxima_suppression_img->at(sx, sy, 0) = 1;
                }
            }

            // Convert back to original image coordinate system
            points[i].x += 1 + m_filter_range;
            points[i].y += 1 + m_filter_range;
            maxima_points.push_back(points[i]);
        }
        i++;
    }
    return maxima_points;
}
