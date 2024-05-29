#include <memory>
#include "app.hpp"

int main()
{
    auto _app = std::make_unique<app>();

    return _app->run();
}
