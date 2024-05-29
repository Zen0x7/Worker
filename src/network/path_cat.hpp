#ifndef NETWORK_PATH_CAT_HPP
#define NETWORK_PATH_CAT_HPP

#include <boost/beast.hpp>

namespace network {
    inline std::string
path_cat(
    boost::beast::string_view base,
    boost::beast::string_view path)
    {
        if(base.empty())
            return std::string(path);
        std::string result(base);
#ifdef BOOST_MSVC
        char constexpr path_separator = '\\';
        if(result.back() == path_separator)
            result.resize(result.size() - 1);
        result.append(path.data(), path.size());
        for(auto& c : result)
            if(c == '/')
                c = path_separator;
#else
        char constexpr path_separator = '/';
        if(result.back() == path_separator)
            result.resize(result.size() - 1);
        result.append(path.data(), path.size());
#endif
        return result;
    }
}

#endif // NETWORK_PATH_CAT_HPP
