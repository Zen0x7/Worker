cmake_minimum_required(VERSION 3.25)
project(Worker)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(Serve
        main.cpp
        app.cpp
        app.hpp
        entities/consumer.cpp
        entities/consumer.hpp
        network/listener.cpp
        network/listener.hpp
        network/http_session.cpp
        network/http_session.hpp
        network/websocket_session.cpp
        network/websocket_session.hpp
        network/handle_request.hpp
        network/fail.hpp
        network/mime_type.hpp
        network/path_cat.hpp
        state.hpp
)
