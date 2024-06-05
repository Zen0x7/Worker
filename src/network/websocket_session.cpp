#include "websocket_session.hpp"
#include "protocol.hpp"

void network::websocket_session::on_read(boost::beast::error_code ec, std::size_t bytes_transferred) {
            boost::ignore_unused(bytes_transferred);

            // This indicates that the websocket_session was closed
            if (ec == boost::beast::websocket::error::closed) {
                state_->user_disconnected(id_);
                return;
            }

            if (ec)
                fail(ec, "read");

            boost::system::error_code _parser_ec;

            std::string _data = boost::beast::buffers_to_string(buffer_.data());

            auto _message = boost::json::parse(_data, _parser_ec);

            if (!_parser_ec && _message.is_object()) {
                if (_message.as_object().contains("action")) {
                    if (_message.as_object().at("action").is_string()) {
                        std::string response = protocol::handle(
                                        _message.as_object(), state_, shared_from_this());

                        ws_.async_write(
                            boost::asio::buffer(response),
                            boost::beast::bind_front_handler(
                                &websocket_session::on_write,
                                shared_from_this()));
                    } else {
                        boost::json::object invalid_action_attribute_type_message = {
                            {"type", "response"},
                            {"status", 500},
                            {"error", "invalid_action_attribute_type"},
                            {"message", "attribute action in message must be a string"}
                        };
                        ws_.async_write(
                            boost::asio::buffer(boost::json::serialize(invalid_action_attribute_type_message)),
                            boost::beast::bind_front_handler(
                                &websocket_session::on_write,
                                shared_from_this()));
                    }
                } else {
                    boost::json::object missing_action_attribute_message = {
                        {"type", "response"},
                        {"status", 500},
                        {"error", "missing_action_attribute"},
                        {"message", "message must include action attribute"}
                    };
                    ws_.async_write(
                        boost::asio::buffer(boost::json::serialize(missing_action_attribute_message)),
                        boost::beast::bind_front_handler(
                            &websocket_session::on_write,
                            shared_from_this()));
                }
            } else {
                boost::json::object invalid_object_message = {
                    {"type", "response"},
                    {"status", 500},
                    {"error", "invalid_json"},
                    {"message", "message must be a valid JSON object"}
                };
                ws_.async_write(
                    boost::asio::buffer(boost::json::serialize(invalid_object_message)),
                    boost::beast::bind_front_handler(
                        &websocket_session::on_write,
                        shared_from_this()));
            }
        }

void network::websocket_session::on_write(boost::beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if (ec) {
        state_->user_disconnected(id_);
        return fail(ec, "write");
    }

    // Clear the buffer
    buffer_.consume(buffer_.size());

    // Do another read
    do_read();
}