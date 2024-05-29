#ifndef STATE_HPP
#define STATE_HPP

#include <memory>
#include <vector>

#include "entities/consumer.hpp"

class state {
    std::vector<std::shared_ptr<entities::consumer>> consumers_;
};

#endif //STATE_HPP
