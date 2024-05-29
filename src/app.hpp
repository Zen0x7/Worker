#ifndef APP_HPP
#define APP_HPP

#include <vector>
#include <thread>

#include <boost/asio.hpp>

#include "state.hpp"
#include "network/listener.hpp"

class app {
public:
    int run() {
        auto const _number_of_threads = static_cast<int>(std::thread::hardware_concurrency());
        boost::asio::io_context _ioc { _number_of_threads };
        auto const _address = boost::asio::ip::make_address("0.0.0.0");
        auto const _port = static_cast<unsigned short>(3000);
        auto const _doc_root = std::make_shared<std::string>("public");
        auto _state = std::make_shared<state>();

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

        return EXIT_SUCCESS;
    }
};

#endif // APP_HPP
