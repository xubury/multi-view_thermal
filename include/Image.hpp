#ifndef _IMAGE_HPP
#define _IMAGE_HPP

#include "mve/image.h"
#include "mve/view.h"

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

#endif //_IMAGE_HPP
