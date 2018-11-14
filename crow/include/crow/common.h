#pragma once

#include <boost/beast/http/verb.hpp>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "crow/utility.h"

namespace crow
{

inline std::string methodName(boost::beast::http::verb method)
{
    switch (method)
    {
        case boost::beast::http::verb::delete_:
            return "DELETE";
        case boost::beast::http::verb::get:
            return "GET";
        case boost::beast::http::verb::head:
            return "HEAD";
        case boost::beast::http::verb::post:
            return "POST";
        case boost::beast::http::verb::put:
            return "PUT";
        case boost::beast::http::verb::connect:
            return "CONNECT";
        case boost::beast::http::verb::options:
            return "OPTIONS";
        case boost::beast::http::verb::trace:
            return "TRACE";
        case boost::beast::http::verb::patch:
            return "PATCH";
    }
    return "invalid";
}

enum class ParamType
{
    INT,
    UINT,
    DOUBLE,
    STRING,
    PATH,

    MAX
};

struct RoutingParams
{
    std::vector<int64_t> intParams;
    std::vector<uint64_t> uintParams;
    std::vector<double> doubleParams;
    std::vector<std::string> stringParams;

    void debugPrint() const
    {
        std::cerr << "RoutingParams" << std::endl;
        for (auto i : intParams)
        {
            std::cerr << i << ", ";
        }
        std::cerr << std::endl;
        for (auto i : uintParams)
        {
            std::cerr << i << ", ";
        }
        std::cerr << std::endl;
        for (auto i : doubleParams)
        {
            std::cerr << i << ", ";
        }
        std::cerr << std::endl;
        for (auto& i : stringParams)
        {
            std::cerr << i << ", ";
        }
        std::cerr << std::endl;
    }

    template <typename T> T get(unsigned) const;
};

template <> inline int64_t RoutingParams::get<int64_t>(unsigned index) const
{
    return intParams[index];
}

template <> inline uint64_t RoutingParams::get<uint64_t>(unsigned index) const
{
    return uintParams[index];
}

template <> inline double RoutingParams::get<double>(unsigned index) const
{
    return doubleParams[index];
}

template <>
inline std::string RoutingParams::get<std::string>(unsigned index) const
{
    return stringParams[index];
}

} // namespace crow

constexpr boost::beast::http::verb operator"" _method(const char* str,
                                                      size_t /*len*/)
{
    using verb = boost::beast::http::verb;
    // clang-format off
  return
    crow::black_magic::isEquP(str, "GET", 3) ? verb::get :
    crow::black_magic::isEquP(str, "DELETE", 6) ? verb::delete_ :
    crow::black_magic::isEquP(str, "HEAD", 4) ? verb::head :
    crow::black_magic::isEquP(str, "POST", 4) ? verb::post :
    crow::black_magic::isEquP(str, "PUT", 3) ? verb::put :
    crow::black_magic::isEquP(str, "OPTIONS", 7) ? verb::options :
    crow::black_magic::isEquP(str, "CONNECT", 7) ? verb::connect :
    crow::black_magic::isEquP(str, "TRACE", 5) ? verb::trace :
    crow::black_magic::isEquP(str, "PATCH", 5) ? verb::patch :
    crow::black_magic::isEquP(str, "PURGE", 5) ? verb::purge :
    throw std::runtime_error("invalid http method");
    // clang-format on
}