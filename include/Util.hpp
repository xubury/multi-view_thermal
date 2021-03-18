#ifndef _UTIL_HPP
#define _UTIL_HPP

#include "mve/image_io.h"
#include "mve/scene.h"
#include "sgm_stereo.h"
#include <glm/glm.hpp>
#include <set>

namespace Util {

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
                                        const std::string &output_name,
                                        bool triangle_mesh);

mve::TriangleMesh::Ptr GenerateMesh(mve::Scene::Ptr scene,
                                    const std::string &input_name,
                                    const std::string &dm_name,
                                    int scale,
                                    const std::string &output_name);

void reconstructSGMDepthForView(const smvs::SGMStereo::Options &opt,
                                const std::string &outputName,
                                smvs::StereoView::Ptr main_view,
                                std::vector<smvs::StereoView::Ptr> neighbors,
                                mve::Bundle::ConstPtr bundle = nullptr);


void resizeViews(mve::Scene::ViewList &views, const std::set<int> &list, const std::string &output_name, int scale);

void resizeView(mve::View::Ptr view, const std::string &output_name, int scale);

} // namespace Util

#endif //_UTIL_HPP
