#pragma once

#include "utility.hpp"

#include <boost/beast/http/verb.hpp>

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace crow
{

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
        for (int64_t integer : intParams)
        {
            std::cerr << integer << ", ";
        }
        std::cerr << std::endl;
        for (uint64_t uInterger : uintParams)
        {
            std::cerr << uInterger << ", ";
        }
        std::cerr << std::endl;
        for (double doubleVal : doubleParams)
        {
            std::cerr << doubleVal << ", ";
        }
        std::cerr << std::endl;
        for (const std::string& str : stringParams)
        {
            std::cerr << str << ", ";
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
