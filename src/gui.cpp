#include "gui.hpp"
#include "application.hpp"

#include <FastDelegate.h>

using namespace fastdelegate;

Gui::Gui()
{
  const auto app           = Application::instance();
  const auto event_manager = app->event_manager();

  event_manager->subscribe(MakeDelegate(this, &Gui::on_window_resize_event),
                           WindowResizeEvent::id);

  window_width_  = app->window_width();
  window_height_ = app->window_height();
}

Gui::~Gui()
{
  const auto app           = Application::instance();
  const auto event_manager = app->event_manager();

  event_manager->unsubscribe(MakeDelegate(this, &Gui::on_window_resize_event),
                             WindowResizeEvent::id);
}

void Gui::draw()
{
  for (auto &gui_element : gui_elements_)
  {
    gui_element->draw(window_width_, window_height_);
  }
}

void Gui::add_gui_element(std::shared_ptr<GuiElement> gui_element)
{
  gui_elements_.push_back(gui_element);
}

void Gui::on_window_resize_event(std::shared_ptr<Event> event)
{
  const auto window_resize_event =
      dynamic_cast<WindowResizeEvent *>(event.get());
  assert(window_resize_event);
  if (!window_resize_event)
  {
    return;
  }

  window_width_  = window_resize_event->width();
  window_height_ = window_resize_event->height();
}
