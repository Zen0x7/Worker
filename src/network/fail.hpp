#ifndef NETWORK_FAIL_HPP
#define NETWORK_FAIL_HPP

#include <iostream>

#include <boost/beast.hpp>

namespace network {
    inline void
fail(boost::beast::error_code ec, char const* what)
    {
        std::cerr << what << ": " << ec.message() << "\n";
    }
}


#endif // NETWORK_FAIL_HPP
