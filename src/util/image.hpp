#pragma once

#include <filesystem>

class Image
{
public:
  Image() = default;
  Image(const std::filesystem::path &file_path);
  ~Image();

  int width() const;
  int height() const;
  int channels_count() const;

  unsigned char *data() const;

  void load_from_file(const std::filesystem::path &file_path);

private:
  int            width_{};
  int            height_{};
  int            channels_count_{};
  unsigned char *data_{};

  Image(const Image &other) = delete;
  void operator=(const Image &other) = delete;
  Image(Image &&other)               = delete;
  void operator=(Image &&other) = delete;
};
