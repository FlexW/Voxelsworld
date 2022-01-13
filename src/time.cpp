#include "time.hpp"

#ifdef WIN32
#include <Windows.h>
#else // WIN32
#include <sys/time.h>
#endif // WIN32

std::int64_t current_time_millis()
{
#ifdef WIN32
  return GetTickCount64();
#else  // WIN32
  timeval t;
  gettimeofday(&t, nullptr);
  auto ret = t.tv_sec * 1000 + t.tv_usec / 1000;

  return ret;
#endif // WIN32
}
