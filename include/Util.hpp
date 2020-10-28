#ifndef _UTIL_HPP
#define _UTIL_HPP

#include "mve/image_io.h"
#include "mve/scene.h"
#include "sgm_stereo.h"
#include <glm/glm.hpp>

namespace util {

glm::mat4 MveToGLMatrix(const glm::mat4 &matrix);

void GaussFilter(const mve::FloatImage::ConstPtr &img,
                 mve::FloatImage::Ptr &out,
                 int filter_range,
                 float sigma);

mve::FloatImage::Ptr ComputeIntegralImg(const mve::FloatImage::ConstPtr &img);

void MeanFilter(const mve::FloatImage::ConstPtr &img, mve::FloatImage::Ptr &out, int filter_range);

mve::TriangleMesh::Ptr GenerateMeshSMVS(mve::Scene::Ptr scene,
                                        const std::string &input_name,
                                        const std::string &dm_name,
                                        int scale,
                                        const std::string &output_name,
                                        bool triangle_mesh);

mve::TriangleMesh::Ptr GenerateMesh(mve::Scene::Ptr scene,
                                    const std::string &input_name,
                                    const std::string &dm_name,
                                    int scale,
                                    const std::string &output_name);

void ReconstructSGMDepthForView(smvs::StereoView::Ptr main_view,
                                std::vector<smvs::StereoView::Ptr> neighbors,
                                mve::Bundle::ConstPtr bundle = nullptr);

} // namespace util

#endif //_UTIL_HPP
