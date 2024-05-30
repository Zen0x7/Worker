#ifndef NETWORK_SESSION_HPP
#define NETWORK_SESSION_HPP

#include <memory>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/json.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "../queue.hpp"

#include "fail.hpp"
#include "../cipher.hpp"

namespace network {
    class session : public std::enable_shared_from_this<session> {
        boost::asio::ip::tcp::resolver resolver_;
        boost::beast::websocket::stream<boost::beast::tcp_stream> ws_;
        boost::beast::flat_buffer buffer_;
        std::string host_;

    public:
        // Resolver and socket require an io_context
        explicit
        session(boost::asio::io_context &ioc)
            : resolver_(boost::asio::make_strand(ioc))
              , ws_(boost::asio::make_strand(ioc)) {
        }

        // Start the asynchronous operation
        void
        run(
            std::string &host,
            char const *port) {
            // Save these for later
            host_ = host;

            // Look up the domain name
            resolver_.async_resolve(
                host,
                port,
                boost::beast::bind_front_handler(
                    &session::on_resolve,
                    shared_from_this()));
        }

        void
        on_resolve(
            boost::beast::error_code ec,
            boost::asio::ip::tcp::resolver::results_type results) {
            if (ec)
                return fail(ec, "resolve");

            // Set the timeout for the operation
            boost::beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

            // Make the connection on the IP address we get from a lookup
            boost::beast::get_lowest_layer(ws_).async_connect(
                results,
                boost::beast::bind_front_handler(
                    &session::on_connect,
                    shared_from_this()));
        }

        void
        on_connect(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type::endpoint_type ep) {
            if (ec)
                return fail(ec, "connect");

            // Turn off the timeout on the tcp_stream, because
            // the websocket stream has its own timeout system.
            boost::beast::get_lowest_layer(ws_).expires_never();

            // Set suggested timeout settings for the websocket
            ws_.set_option(
                boost::beast::websocket::stream_base::timeout::suggested(
                    boost::beast::role_type::client));

            // Set a decorator to change the User-Agent of the handshake
            ws_.set_option(boost::beast::websocket::stream_base::decorator(
                [](boost::beast::websocket::request_type &req) {
                    req.set(boost::beast::http::field::user_agent,
                            std::string(BOOST_BEAST_VERSION_STRING) +
                            " websocket-client-async");
                }));

            // Update the host_ string. This will provide the value of the
            // Host HTTP header during the WebSocket handshake.
            // See https://tools.ietf.org/html/rfc7230#section-5.4
            host_ += ':' + std::to_string(ep.port());

            // Perform the websocket handshake
            ws_.async_handshake(host_, "/",
                                boost::beast::bind_front_handler(
                                    &session::on_handshake,
                                    shared_from_this()));
        }

        void
        on_handshake(boost::beast::error_code ec) {
            if (ec)
                return fail(ec, "handshake");

            std::string _transaction_id = boost::uuids::to_string(boost::uuids::random_generator()());

            std::string _registration_token = cipher::encrypt(_transaction_id);

            boost::json::object _welcome_message = {
                {"action", "join"},
                {"transaction_id", _transaction_id},
                {"registration_token", _registration_token}
            };

            // Send the message
            ws_.async_write(
                boost::asio::buffer(boost::json::serialize(_welcome_message)),
                boost::beast::bind_front_handler(
                    &session::on_write,
                    shared_from_this()));
        }

        void send(boost::json::object & message) {
            std::string _transaction_id = boost::uuids::to_string(boost::uuids::random_generator()());
            std::string _action { message.at("action").as_string() };
            message.insert_or_assign("transaction_id", _transaction_id);

            queue::push_back(_transaction_id, _action);

            std::string _serialized = boost::json::serialize(message);
            std::cout << "Sending: " << _serialized << std::endl;

            ws_.async_write(
                boost::asio::buffer(_serialized),
                boost::beast::bind_front_handler(
                    [self = shared_from_this(), _transaction_id] (boost::beast::error_code ec, std::size_t bytes_transferred) {
                        self->on_send(ec, bytes_transferred, _transaction_id);
                    }));
        }

        void on_send(boost::beast::error_code ec, std::size_t bytes_transferred, std::string transaction_id) {
            boost::ignore_unused(bytes_transferred);

            if (ec)
                return fail(ec, "send");

            queue::change_status(transaction_id, queue::transaction::statusses::SENT);
        }

        void
        on_write(
            boost::beast::error_code ec,
            std::size_t bytes_transferred) {
            boost::ignore_unused(bytes_transferred);

            if (ec)
                return fail(ec, "write");

            ws_.async_read(
                buffer_,
                boost::beast::bind_front_handler(
                    &session::on_read,
                    shared_from_this()));
        }

        void
        on_read(
            boost::beast::error_code ec,
            std::size_t bytes_transferred) {
            boost::ignore_unused(bytes_transferred);

            if (ec)
                return fail(ec, "read");

            // The make_printable() function helps print a ConstBufferSequence
            std::cout << "Received: " << boost::beast::make_printable(buffer_.data()) << std::endl;

            buffer_.consume(bytes_transferred);

            ws_.async_read(
                buffer_,
                boost::beast::bind_front_handler(
                    &session::on_read,
                    shared_from_this()));
        }

        void
        on_close(boost::beast::error_code ec) {
            if (ec)
                return fail(ec, "close");

            // If we get here then the connection is closed gracefully

            // The make_printable() function helps print a ConstBufferSequence
            std::cout << boost::beast::make_printable(buffer_.data()) << std::endl;
        }

        void close() {
            // Close the WebSocket connection
            ws_.async_close(boost::beast::websocket::close_code::normal,
                            boost::beast::bind_front_handler(
                                &session::on_close,
                                shared_from_this()));
        }
    };
}

#endif // NETWORK_SESSION_HPP
