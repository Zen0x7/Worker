#include "../state.hpp"
#include "websocket_session.hpp"
#include "protocol.hpp"

network::websocket_session::websocket_session(boost::asio::ip::tcp::socket &&socket,
                                              std::shared_ptr<state> const &state) : ws_(std::move(socket)),
                                                                                     state_(state),
                                                                                     id_(boost::uuids::to_string(
                                                                                         boost::uuids::random_generator()())) {
}

void network::websocket_session::on_accept(boost::beast::error_code ec) {
    if (ec)
        return fail(ec, "accept");

    state_->user_accepted(
        id_,
        this->remote_endpoint().address().to_string(),
        this->remote_endpoint().port(),
        shared_from_this());

    // Read a message
    do_read();
}

void network::websocket_session::on_read(boost::beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    // This indicates that the websocket_session was closed
    if (ec == boost::beast::websocket::error::closed || ec == boost::asio::error::operation_aborted) {
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
                auto response = protocol::handle(
                    _message.as_object(), state_, shared_from_this());

                this->send(response);
            } else {
                boost::json::object invalid_action_attribute_type_message = {
                    {"type", "response"},
                    {"status", 500},
                    {"error", "invalid_action_attribute_type"},
                    {"message", "attribute action in message must be a string"}
                };
                this->send(invalid_action_attribute_type_message);
            }
        } else {
            boost::json::object missing_action_attribute_message = {
                {"type", "response"},
                {"status", 500},
                {"error", "missing_action_attribute"},
                {"message", "message must include action attribute"}
            };
            this->send(missing_action_attribute_message);
        }
    } else {
        boost::json::object invalid_object_message = {
            {"type", "response"},
            {"status", 500},
            {"error", "invalid_json"},
            {"message", "message must be a valid JSON object"}
        };

        this->send(invalid_object_message);
    }

    buffer_.consume(buffer_.size());

    ws_.async_read(
        buffer_,
        boost::beast::bind_front_handler(
            &websocket_session::on_read,
            shared_from_this()));
}

void network::websocket_session::on_write(boost::beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if (ec) {
        state_->user_disconnected(id_);
        return fail(ec, "write");
    }

    // Handle the error, if any
    if (ec)
        return fail(ec, "write");

    // Remove the string from the queue
    queue_.erase(queue_.begin());

    // Send the next message if any
    if (!queue_.empty())
        ws_.async_write(
            boost::asio::buffer(*queue_.front()),
            boost::beast::bind_front_handler(
                &websocket_session::on_write,
                shared_from_this()));
}
