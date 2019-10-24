#pragma once

#include "utility.h"

#include <boost/beast/http/verb.hpp>

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

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
        default:
            return "invalid";
    }
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

    template <typename T>
    T get(unsigned) const;
};

template <>
inline int64_t RoutingParams::get<int64_t>(unsigned index) const
{
    return intParams[index];
}

template <>
inline uint64_t RoutingParams::get<uint64_t>(unsigned index) const
{
    return uintParams[index];
}

template <>
inline double RoutingParams::get<double>(unsigned index) const
{
    return doubleParams[index];
}

template <>
inline std::string RoutingParams::get<std::string>(unsigned index) const
{
    return stringParams[index];
}

} // namespace crow
