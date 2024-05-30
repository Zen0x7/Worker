#ifndef APP_HPP
#define APP_HPP

#include <vector>
#include <thread>

#include <boost/asio.hpp>

#include "state.hpp"
#include "cipher.hpp"
#include "network/listener.hpp"
#include "network/session.hpp"

class app {
public:
    int run() {

        std::string plaintext = "0cd8d4fc-9434-4933-a057-ea8a52a9276c";
        std::string encrypted = cipher::encrypt(plaintext);
        std::cout << "Texto encriptado: " << encrypted << std::endl;

        if (std::getenv("STATE_HOST") == nullptr)
            throw std::invalid_argument("STATE_HOST is required.");

        boost::asio::io_context _sync_ioc;
        std::string _state_host = std::getenv("STATE_HOST");
        auto _state_session = std::make_shared<network::session>(_sync_ioc);
        _state_session->run(_state_host, "8000");

        auto _sync_thread = std::thread([&_sync_ioc]() {
            _sync_ioc.run();
        });

        _sync_thread.detach();

        auto const _number_of_threads = static_cast<int>(std::thread::hardware_concurrency());
        boost::asio::io_context _ioc { _number_of_threads };
        auto const _address = boost::asio::ip::make_address("0.0.0.0");
        auto const _port = static_cast<unsigned short>(3000);
        auto const _doc_root = std::make_shared<std::string>("public");
        auto _state = std::make_shared<state>(_state_session);

        std::make_shared<network::listener>(
            _ioc,
            boost::asio::ip::tcp::endpoint{ _address, _port },
            _doc_root,
            _state
            )->run();

        boost::asio::signal_set _signals(_ioc, SIGINT, SIGTERM);
        _signals.async_wait(
            [&](boost::beast::error_code const&, int)
            {
                _ioc.stop();
            });

        std::vector<std::thread> _threads;

        _threads.reserve(_number_of_threads - 1);
        for(auto i = _number_of_threads - 1; i > 0; --i)
            _threads.emplace_back(
            [&_ioc]
            {
                _ioc.run();
            });

        _ioc.run();

        for(auto& _thread : _threads)
            _thread.join();

        _state_session->close();

        return EXIT_SUCCESS;
    }
};

#endif // APP_HPP
