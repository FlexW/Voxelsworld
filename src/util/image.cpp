#include "image.hpp"

#include <stb_image.h>

#include <stdexcept>

Image::Image(const std::filesystem::path &file_path)
{
  load_from_file(file_path);
}

Image::~Image()
{
  if (data_)
  {
    stbi_image_free(data_);
  }
}

int Image::width() const { return width_; }

int Image::height() const { return height_; }

int Image::channels_count() const { return channels_count_; }

unsigned char *Image::data() const { return data_; }

void Image::load_from_file(const std::filesystem::path &file_path)
{
  stbi_set_flip_vertically_on_load(true);
  data_ = stbi_load(file_path.string().c_str(),
                    &width_,
                    &height_,
                    &channels_count_,
                    0);
  if (!data_)
  {
    throw std::runtime_error("Could not load image " + file_path.string());
  }
}
