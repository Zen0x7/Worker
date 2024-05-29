#ifndef NETWORK_LISTENER_HPP
#define NETWORK_LISTENER_HPP

#include <memory>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include "../state.hpp"

#include "fail.hpp"
#include "http_session.hpp"

namespace network {
 class listener  : public std::enable_shared_from_this<listener>
{
    boost::asio::io_context& ioc_;
    boost::asio::ip::tcp::acceptor acceptor_;
    std::shared_ptr<std::string const> doc_root_;
    std::shared_ptr<state> state_;
public:
    listener(
        boost::asio::io_context& ioc,
        boost::asio::ip::tcp::endpoint endpoint,
        std::shared_ptr<std::string const> const& doc_root,
        std::shared_ptr<state> const & state)
        : ioc_(ioc)
        , acceptor_(boost::asio::make_strand(ioc))
        , doc_root_(doc_root)
        , state_(state)
    {
        boost::beast::error_code ec;

        // Open the acceptor
        acceptor_.open(endpoint.protocol(), ec);
        if(ec)
        {
            fail(ec, "open");
            return;
        }

        // Allow address reuse
        acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
        if(ec)
        {
            fail(ec, "set_option");
            return;
        }

        // Bind to the server address
        acceptor_.bind(endpoint, ec);
        if(ec)
        {
            fail(ec, "bind");
            return;
        }

        // Start listening for connections
        acceptor_.listen(
            boost::asio::socket_base::max_listen_connections, ec);
        if(ec)
        {
            fail(ec, "listen");
            return;
        }
    }

    // Start accepting incoming connections
    void
    run()
    {
        // We need to be executing within a strand to perform async operations
        // on the I/O objects in this session. Although not strictly necessary
        // for single-threaded contexts, this example code is written to be
        // thread-safe by default.
        boost::asio::dispatch(
            acceptor_.get_executor(),
            boost::beast::bind_front_handler(
                &listener::do_accept,
                this->shared_from_this()));
    }

private:
    void
    do_accept()
    {
        // The new connection gets its own strand
        acceptor_.async_accept(
            boost::asio::make_strand(ioc_),
            boost::beast::bind_front_handler(
                &listener::on_accept,
                shared_from_this()));
    }

    void
    on_accept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket)
    {
        if(ec)
        {
            fail(ec, "accept");
        }
        else
        {
            // Create the http session and run it
            std::make_shared<http_session>(
                std::move(socket),
                doc_root_,
                state_)->run();
        }

        // Accept another connection
        do_accept();
    }
};
}

#endif // NETWORK_LISTENER_HPP
