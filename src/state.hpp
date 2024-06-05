#ifndef STATE_HPP
#define STATE_HPP

#include <memory>
#include <vector>

#include <boost/json.hpp>

#include "entities/user.hpp"

namespace network {
    class session;
}

class state {
    std::string worker_id_;
    std::unordered_map<std::string, std::shared_ptr<entities::user> > users_;
    network::session * session_;

public:
    state() {}

    void set_session(network::session * session);

    void user_accepted(std::string & id, std::string address, uint_least16_t port, std::shared_ptr<network::websocket_session> session);

    void user_disconnected(std::string & id);

    void user_broadcast(boost::json::object & message);

    void distribute(boost::json::object & message);
};

#endif //STATE_HPP
