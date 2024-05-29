#ifndef STATE_HPP
#define STATE_HPP

#include <memory>
#include <vector>

#include "entities/consumer.hpp"
#include "network/session.hpp"

class state {
    std::vector<std::shared_ptr<entities::consumer>> consumers_;
    std::shared_ptr<network::session> session_;

public:
    state(std::shared_ptr<network::session> const & session) : session_(session) {}

    void user_accepted() {
        boost::json::object _accepted_message = {
            {"action", "user_accepted"}
        };
        session_->send(_accepted_message);
    }
};

#endif //STATE_HPP
