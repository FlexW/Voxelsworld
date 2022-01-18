#pragma once

#include "event.hpp"
#include "gui_element.hpp"
#include "math.hpp"

#include <memory>
#include <vector>

class Gui
{
public:
  Gui();
  ~Gui();

  void draw();

  void add_gui_element(std::shared_ptr<GuiElement> gui_element);

private:
  float window_width_{};
  float window_height_{};

  std::vector<std::shared_ptr<GuiElement>> gui_elements_;

  void on_window_resize_event(std::shared_ptr<Event> event);
};
