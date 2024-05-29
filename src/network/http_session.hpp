#ifndef NETWORK_HTTP_SESSION_HPP
#define NETWORK_HTTP_SESSION_HPP

#include <memory>
#include <queue>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include "../state.hpp"

#include "websocket_session.hpp"
#include "handle_request.hpp"
#include "fail.hpp"

namespace network {
    class http_session : public std::enable_shared_from_this<http_session>
{
    boost::beast::tcp_stream stream_;
    boost::beast::flat_buffer buffer_;
    std::shared_ptr<std::string const> doc_root_;
    std::shared_ptr<state> state_;

    static constexpr std::size_t queue_limit = 8; // max responses
    std::queue<boost::beast::http::message_generator> response_queue_;

    // The parser is stored in an optional container so we can
    // construct it from scratch it at the beginning of each new message.
    boost::optional<boost::beast::http::request_parser<boost::beast::http::string_body>> parser_;

public:
    // Take ownership of the socket
    http_session(
        boost::asio::ip::tcp::socket&& socket,
        std::shared_ptr<std::string const> const& doc_root,
        std::shared_ptr<state> const & state)
        : stream_(std::move(socket))
        , doc_root_(doc_root)
        , state_(state)
    {
        static_assert(queue_limit > 0,
                      "queue limit must be positive");
    }

    // Start the session
    void
    run()
    {
        // We need to be executing within a strand to perform async operations
        // on the I/O objects in this session. Although not strictly necessary
        // for single-threaded contexts, this example code is written to be
        // thread-safe by default.
        boost::asio::dispatch(
            stream_.get_executor(),
            boost::beast::bind_front_handler(
                &http_session::do_read,
                this->shared_from_this()));
    }

private:
    void
    do_read()
    {
        // Construct a new parser for each message
        parser_.emplace();

        // Apply a reasonable limit to the allowed size
        // of the body in bytes to prevent abuse.
        parser_->body_limit(10000);

        // Set the timeout.
        stream_.expires_after(std::chrono::seconds(30));

        // Read a request using the parser-oriented interface
        boost::beast::http::async_read(
            stream_,
            buffer_,
            *parser_,
            boost::beast::bind_front_handler(
                &http_session::on_read,
                shared_from_this()));
    }

    void
    on_read(boost::beast::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        // This means they closed the connection
        if(ec == boost::beast::http::error::end_of_stream)
            return do_close();

        if(ec)
            return fail(ec, "read");

        // See if it is a WebSocket Upgrade
        if(boost::beast::websocket::is_upgrade(parser_->get()))
        {
            // Create a websocket session, transferring ownership
            // of both the socket and the HTTP request.
            std::make_shared<websocket_session>(
                stream_.release_socket(),
                state_)->do_accept(parser_->release());
            return;
        }

        // Send the response
        queue_write(handle_request(*doc_root_, parser_->release(), state_));

        // If we aren't at the queue limit, try to pipeline another request
        if (response_queue_.size() < queue_limit)
            do_read();
    }

    void
    queue_write(boost::beast::http::message_generator response)
    {
        // Allocate and store the work
        response_queue_.push(std::move(response));

        // If there was no previous work, start the write loop
        if (response_queue_.size() == 1)
            do_write();
    }

    // Called to start/continue the write-loop. Should not be called when
    // write_loop is already active.
    void
    do_write()
    {
        if(! response_queue_.empty())
        {
            bool keep_alive = response_queue_.front().keep_alive();

            boost::beast::async_write(
                stream_,
                std::move(response_queue_.front()),
                boost::beast::bind_front_handler(
                    &http_session::on_write,
                    shared_from_this(),
                    keep_alive));
        }
    }

    void
    on_write(
        bool keep_alive,
        boost::beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return fail(ec, "write");

        if(! keep_alive)
        {
            // This means we should close the connection, usually because
            // the response indicated the "Connection: close" semantic.
            return do_close();
        }

        // Resume the read if it has been paused
        if(response_queue_.size() == queue_limit)
            do_read();

        response_queue_.pop();

        do_write();
    }

    void
    do_close()
    {
        // Send a TCP shutdown
        boost::beast::error_code ec;
        stream_.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);

        // At this point the connection is closed gracefully
    }
};
}

#endif // NETWORK_HTTP_SESSION_HPP
