#ifndef _IMAGE_HPP
#define _IMAGE_HPP

#include "mve/image.h"
#include "mve/view.h"
#include "mve/scene.h"
#include "sfm/bundler_common.h"

#define THUMBNAIL_SIZE 50
#define RAND_SEED_MATCHING 0
#define MAX_IMAGE_SIZE 6000000
#define ORIGINAL_IMAGE_NAME "original"

template<class T>
typename mve::Image<T>::Ptr limit_image_size(typename mve::Image<T>::Ptr img, int max_pixels);

mve::ImageBase::Ptr limit_image_size(mve::ImageBase::Ptr image, int max_pixels);

bool has_jpeg_extension(std::string const &filename);

std::string remove_file_extension(std::string const &filename);

mve::ByteImage::Ptr load_8bit_image(std::string const &fname, std::string *exif);

mve::RawImage::Ptr load_16bit_image(std::string const &fname);

mve::FloatImage::Ptr load_float_image(std::string const &fname);

mve::ImageBase::Ptr load_any_image(std::string const &fname, std::string *exif);

void add_exif_to_view(mve::View::Ptr view, std::string const &exif);

std::string make_image_name(int id);

template<typename T>
void find_min_max_percentile(typename mve::Image<T>::ConstPtr image, T *vmin, T *vmax);

mve::ByteImage::Ptr create_thumbnail(mve::ImageBase::ConstPtr img);

void features_and_matching (mve::Scene::Ptr scene,
                       sfm::bundler::ViewportList* viewports,
                       sfm::bundler::PairwiseMatching* pairwise_matching);

#endif //_IMAGE_HPP
