#include "protocol.hpp"

std::string network::protocol::handle(boost::json::object &request,
                                      std::shared_ptr<state> const &state,
                                      std::shared_ptr<websocket_session> websocket) {
    std::string _action{request.at("action").as_string()};

    boost::json::object missing_required_attributes_message = {
        {"type", "response"},
        {"status", 500},
        {"error", "required_attributes"},
    };

    boost::json::object invalid_values_message = {
        {"type", "response"},
        {"status", 500},
        {"error", "invalid_values"},
    };

    boost::json::object success_message = {
        {"type", "response"},
        {"status", 200},
    };

    boost::json::object unauthorized_message = {
        {"type", "response"},
        {"status", 401},
        {"error", "unauthorized"},
    };

    boost::json::object not_found_message = {
        {"type", "response"},
        {"status", 404},
        {"error", "not_found"},
    };

    if (_action == "broadcast") {
        if (request.contains("message")) {
            if (request.at("message").is_object()) {
                boost::json::object _message = request.at("message").as_object();
                std::string _worker_id = websocket->id_;

                state->user_broadcast(_message);

                return serialize(success_message);
            }
            return serialize(invalid_values_message);
        }
    }

    return serialize(not_found_message);
}
