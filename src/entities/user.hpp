#ifndef ENTITIES_CONSUMER_HPP
#define ENTITIES_CONSUMER_HPP
#include <memory>

#include "../network/websocket_session.hpp"

namespace entities {
    class user : std::enable_shared_from_this<user> {
        std::shared_ptr<network::websocket_session> websocket_;
        std::string address_;
        int64_t port_;
    public:
        explicit user(std::string address, int64_t port, std::shared_ptr<network::websocket_session> websocket) : websocket_(std::move(websocket)), address_(std::move(address)), port_(port) {}

        void send(boost::json::object & data) {
            websocket_->send(data);
        }
    };
}

#endif // ENTITIES_CONSUMER_HPP
