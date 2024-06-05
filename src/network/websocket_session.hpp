#ifndef NETWORK_WEBSOCKET_SESSION_HPP
#define NETWORK_WEBSOCKET_SESSION_HPP

#include <memory>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include "../state.hpp"

#include "fail.hpp"

#define WS_SERVER_NAME "Worker"

namespace network {
    class websocket_session : public std::enable_shared_from_this<websocket_session> {
        boost::beast::websocket::stream<boost::beast::tcp_stream> ws_;
        boost::beast::flat_buffer buffer_;
        std::shared_ptr<state> state_;

    public:
        std::string id_;

        // Take ownership of the socket
        explicit
        websocket_session(boost::asio::ip::tcp::socket &&socket,
                          std::shared_ptr<state> const &state) : ws_(std::move(socket)),
                                                                 state_(state),
                                                                 id_(boost::uuids::to_string(
                                                                     boost::uuids::random_generator()())) {
        }

        boost::asio::ip::tcp::endpoint remote_endpoint() {
            return ws_.next_layer().socket().remote_endpoint();
        }

        // Start the asynchronous accept operation
        template<class Body, class Allocator>
        void
        do_accept(boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator> > req) {
            // Set suggested timeout settings for the websocket
            ws_.set_option(
                boost::beast::websocket::stream_base::timeout::suggested(
                    boost::beast::role_type::server));

            // Set a decorator to change the Server of the handshake
            ws_.set_option(boost::beast::websocket::stream_base::decorator(
                [](boost::beast::websocket::response_type &res) {
                    res.set(boost::beast::http::field::server, std::string(WS_SERVER_NAME));
                }));

            // Accept the websocket handshake
            ws_.async_accept(
                req,
                boost::beast::bind_front_handler(
                    &websocket_session::on_accept,
                    shared_from_this()));
        }

    private:
        void
        on_accept(boost::beast::error_code ec) {
            if (ec)
                return fail(ec, "accept");

            state_->user_accepted(id_, this->remote_endpoint().address().to_string(), this->remote_endpoint().port());

            // Read a message
            do_read();
        }

        void
        do_read() {
            // Read a message into our buffer
            ws_.async_read(
                buffer_,
                boost::beast::bind_front_handler(
                    &websocket_session::on_read,
                    shared_from_this()));
        }

        void
        on_read(
            boost::beast::error_code ec,
            std::size_t bytes_transferred);

        void
        on_write(
            boost::beast::error_code ec,
            std::size_t bytes_transferred);
    };
};


#endif // NETWORK_WEBSOCKET_SESSION_HPP
