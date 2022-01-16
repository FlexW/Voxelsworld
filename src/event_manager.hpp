#pragma once

#include "event.hpp"

#include <FastDelegate.h>

#include <list>
#include <memory>
#include <queue>
#include <unordered_map>

using EventListener = fastdelegate::FastDelegate1<std::shared_ptr<Event>>;

class EventManager
{
public:
  void publish(std::shared_ptr<Event> event);

  void subscribe(const EventListener &event_listener, EventId event_id);
  void unsubscribe(const EventListener &event_listener, EventId event_id);

  void dispatch();

private:
  std::queue<std::shared_ptr<Event>>                    events_;
  std::unordered_map<EventId, std::list<EventListener>> listeners_;
};
