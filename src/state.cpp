//
// Created by ian on 6/5/24.
//
#include "state.hpp"
#include "network/session.hpp"

void state::set_session(network::session * session) {
    session_ = session;
}

void state::user_accepted(std::string &id, std::string address, uint_least16_t port, std::shared_ptr<network::websocket_session> session) {
    boost::json::object _accepted_message = {
        { "action", "accepted" },
        {"transaction_id", id},
        { "address", address },
        { "port", port }
    };
    session_->send(_accepted_message, id);
    users_.insert( {id, std::make_shared<entities::user>(address, port, session) } );
}

void state::user_disconnected(std::string &id) {
    std::string _transaction_id = boost::uuids::to_string(boost::uuids::random_generator()());
    boost::json::object _accepted_message = {
        {"action", "disconnected"},
        {"transaction_id", _transaction_id},
        {"user_id", id}
    };
    session_->send(_accepted_message, _transaction_id);
    users_.erase(id);
}

void state::user_broadcast(boost::json::object &message) {
    std::string _transaction_id = boost::uuids::to_string(boost::uuids::random_generator()());
    boost::json::object _broadcast_message = {
        {"action", "broadcast"},
        {"transaction_id", _transaction_id},
        {"channel", "*"},
        {"message", message}
    };
    session_->send(_broadcast_message, _transaction_id);
}

void state::distribute(boost::json::object &message) {
    std::string _transaction_id = boost::uuids::to_string(boost::uuids::random_generator()());

    boost::json::object _broadcast_message = {
        {"action", "message"},
        {"transaction_id", _transaction_id},
        {"channel", "*"},
        {"message", message}
    };

    for (auto & user : users_) {
        user.second->send(_broadcast_message);
    }

}
