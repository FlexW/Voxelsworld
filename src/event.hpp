#pragma once

#include <cstdint>

using EventId = std::uint64_t;

class Event
{
public:
  Event(EventId id);
  virtual ~Event() = default;

  EventId id() const;

private:
  EventId id_{};
};
