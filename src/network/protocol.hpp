//
// Created by ian on 6/5/24.
//

#ifndef NETWORK_PROTOCOL_HPP
#define NETWORK_PROTOCOL_HPP

#include <boost/json.hpp>

#include "../state.hpp"
#include "websocket_session.hpp"

class state;

namespace network {
    class protocol {
    public:
        static std::string handle(
            boost::json::object &request,
            std::shared_ptr<state> const &state,
            std::shared_ptr<websocket_session> websocket);
    };
}

#endif // NETWORK_PROTOCOL_HPP
