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
    STRING,
    PATH,

    MAX
};

} // namespace crow
