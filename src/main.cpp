#include "application.hpp"

int main()
{
  const auto app = Application::instance();
  return app->run();
}
