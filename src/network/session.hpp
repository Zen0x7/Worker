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
#include "protocol.hpp"

class state;

namespace network {
    class session : public std::enable_shared_from_this<session> {
        boost::asio::ip::tcp::resolver resolver_;
        boost::beast::websocket::stream<boost::beast::tcp_stream> ws_;
        boost::beast::flat_buffer buffer_;
        std::string host_;

    public:
        std::shared_ptr<state> state_;
        std::string id_;

        // Resolver and socket require an io_context
        explicit
        session(boost::asio::io_context &ioc);

        // Start the asynchronous operation
        void
        run(
            std::string &host,
            char const *port);

        void
        on_resolve(
            boost::beast::error_code ec,
            boost::asio::ip::tcp::resolver::results_type results);

        void
        on_connect(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type::endpoint_type ep);

        void
        on_handshake(boost::beast::error_code ec);

        void send(boost::json::object &message, std::string &transaction_id);

        void on_send(boost::beast::error_code ec, std::size_t bytes_transferred, std::string transaction_id);

        void
        on_write(
            boost::beast::error_code ec,
            std::size_t bytes_transferred);

        void
        on_read(
            boost::beast::error_code ec,
            std::size_t bytes_transferred);

        void
        on_close(boost::beast::error_code ec);

        void close();
    };
}

#endif // NETWORK_SESSION_HPP
