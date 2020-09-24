#include "Image.hpp"
#include "mve/image_tools.h"
#include "mve/image_io.h"
#include "mve/image_exif.h"
#include "util/file_system.h"

/* ---------------------------------------------------------------- */

template <class T>
typename mve::Image<T>::Ptr
limit_image_size (typename mve::Image<T>::Ptr img, int max_pixels)
{
    while (img->get_pixel_amount() > max_pixels)
        img = mve::image::rescale_half_size<T>(img);
    return img;
}

/* ---------------------------------------------------------------- */

mve::ImageBase::Ptr
limit_image_size (mve::ImageBase::Ptr image, int max_pixels)
{
    switch (image->get_type())
    {
        case mve::IMAGE_TYPE_FLOAT:
            return limit_image_size<float>(std::dynamic_pointer_cast
                                                   <mve::FloatImage>(image), max_pixels);
        case mve::IMAGE_TYPE_UINT8:
            return limit_image_size<uint8_t>(std::dynamic_pointer_cast
                                                     <mve::ByteImage>(image), max_pixels);
        case mve::IMAGE_TYPE_UINT16:
            return limit_image_size<uint16_t>(std::dynamic_pointer_cast
                                                      <mve::RawImage>(image), max_pixels);
        default:
            break;
    }
    return mve::ImageBase::Ptr();
}

/* ---------------------------------------------------------------- */

bool
has_jpeg_extension (std::string const& filename)
{
    std::string lcfname(util::string::lowercase(filename));
    return util::string::right(lcfname, 4) == ".jpg"
           || util::string::right(lcfname, 5) == ".jpeg";
}

std::string
remove_file_extension (std::string const& filename)
{
    std::size_t pos = filename.find_last_of('.');
    if (pos != std::string::npos)
        return filename.substr(0, pos);
    return filename;
}
/* ---------------------------------------------------------------- */

mve::ByteImage::Ptr
load_8bit_image (std::string const& fname, std::string* exif)
{
    std::string lcfname(util::string::lowercase(fname));
    std::string ext4 = util::string::right(lcfname, 4);
    std::string ext5 = util::string::right(lcfname, 5);
    try
    {
        if (ext4 == ".jpg" || ext5 == ".jpeg")
            return mve::image::load_jpg_file(fname, exif);
        else if (ext4 == ".png" ||  ext4 == ".ppm"
                 || ext4 == ".tif" || ext5 == ".tiff")
            return mve::image::load_file(fname);
    }
    catch (...)
    { }

    return mve::ByteImage::Ptr();
}

/* ---------------------------------------------------------------- */

mve::RawImage::Ptr
load_16bit_image (std::string const& fname)
{
    std::string lcfname(util::string::lowercase(fname));
    std::string ext4 = util::string::right(lcfname, 4);
    std::string ext5 = util::string::right(lcfname, 5);
    try
    {
        if (ext4 == ".tif" || ext5 == ".tiff")
            return mve::image::load_tiff_16_file(fname);
        else if (ext4 == ".ppm")
            return mve::image::load_ppm_16_file(fname);
    }
    catch (...)
    { }

    return mve::RawImage::Ptr();
}

/* ---------------------------------------------------------------- */

mve::FloatImage::Ptr
load_float_image (std::string const& fname)
{
    std::string lcfname(util::string::lowercase(fname));
    std::string ext4 = util::string::right(lcfname, 4);
    std::string ext5 = util::string::right(lcfname, 5);
    try
    {
        if (ext4 == ".tif" || ext5 == ".tiff")
            return mve::image::load_tiff_float_file(fname);
        else if (ext4 == ".pfm")
            return mve::image::load_pfm_file(fname);
    }
    catch (...)
    { }

    return mve::FloatImage::Ptr();
}

/* ---------------------------------------------------------------- */

mve::ImageBase::Ptr
load_any_image (std::string const& fname, std::string* exif)
{
    mve::ByteImage::Ptr img_8 = load_8bit_image(fname, exif);
    if (img_8 != nullptr)
        return img_8;

    mve::RawImage::Ptr img_16 = load_16bit_image(fname);
    if (img_16 != nullptr)
        return img_16;

    mve::FloatImage::Ptr img_float = load_float_image(fname);
    if (img_float != nullptr)
        return img_float;

#pragma omp critical
    std::cerr << "Skipping file " << util::fs::basename(fname)
              << ", cannot load image." << std::endl;
    return mve::ImageBase::Ptr();
}

/* ---------------------------------------------------------------- */

void
add_exif_to_view (mve::View::Ptr view, std::string const& exif)
{
    if (exif.empty())
        return;

    mve::ByteImage::Ptr exif_image = mve::ByteImage::create(exif.size(), 1, 1);
    std::copy(exif.begin(), exif.end(), exif_image->begin());
    view->set_blob(exif_image, "exif");
}
/* ---------------------------------------------------------------- */

std::string
make_image_name (int id)
{
    return "view_" + util::string::get_filled(id, 4) + ".mve";
}
