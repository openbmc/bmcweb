#pragma once

namespace crow
{
enum class AuthMode
{
    NOAUTH, // Socket disables authentication and authorization
    AUTH,   // Socket enable authentication and authorization but not bootstrap
            // account
    BOOTSTRAP // Socket only enables bootstrap authentication and authorization
};
}
