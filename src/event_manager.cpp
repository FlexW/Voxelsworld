#include "event_manager.hpp"

#include <algorithm>

void EventManager::publish(std::shared_ptr<Event> event)
{

  // check if a listener waits for this event
  const auto listener_map_iter = listeners_.find(event->id());
  if (listener_map_iter == listeners_.end() ||
      listener_map_iter->second.size() <= 0)
  {
    return;
  }

  // Queue the event
  events_.push(event);
}

void EventManager::subscribe(const EventListener &event_listener,
                             EventId              event_id)
{
  // create or find the listener list
  auto &listener_list = listeners_[event_id];

  // check if the listener already exists
  const auto listener_list_iter =
      std::find(listener_list.begin(), listener_list.end(), event_listener);
  if (listener_list_iter != listener_list.end())
  {
    // listener exists already
    return;
  }

  listener_list.push_back(event_listener);
}

void EventManager::unsubscribe(const EventListener &event_listener,
                               EventId              event_id)
{
  // find the listener list
  const auto listener_map_iter = listeners_.find(event_id);
  if (listener_map_iter == listeners_.end())
  {
    return;
  }

  auto &listener_list = listener_map_iter->second;

  // remove the listener if it exists
  listener_list.erase(
      std::remove(listener_list.begin(), listener_list.end(), event_listener));
}

void EventManager::dispatch()
{
  while (events_.size() > 0)
  {
    // get next event
    auto event = events_.front();
    events_.pop();

    // find the listeners for the event
    auto &listeners = listeners_[event->id()];

    // iterate through all listeners and send them the event
    for (auto iter = listeners.begin(); iter != listeners.end(); ++iter)
    {
      auto &listener = (*iter);
      listener(event);
    }
  }
}
