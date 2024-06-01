#ifndef STATE_HPP
#define STATE_HPP

#include <memory>
#include <vector>

#include "entities/consumer.hpp"
#include "network/session.hpp"

class state {
    std::string worker_id_;
    std::vector<std::shared_ptr<entities::consumer>> consumers_;
    std::shared_ptr<network::session> session_;

public:
    state(std::shared_ptr<network::session> const & session) : session_(session) {}

    void user_accepted(std::string & id, std::string address, uint_least16_t port) {
        boost::json::object _accepted_message = {
            { "action", "accepted" },
            {"transaction_id", id},
            { "address", address },
            { "port", port }
        };
        session_->send(_accepted_message, id);
    }

    void user_disconnected(std::string & id) {
        std::string _transaction_id = boost::uuids::to_string(boost::uuids::random_generator()());
        boost::json::object _accepted_message = {
            {"action", "disconnected"},
            {"transaction_id", _transaction_id},
            {"user_id", id}
        };
        session_->send(_accepted_message, _transaction_id);
    }
};

#endif //STATE_HPP
