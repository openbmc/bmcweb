#pragma once

#include "logging.hpp"
#include "ssl_key_handler.hpp"

#include <openssl/ssl.h>

#include <boost/asio/ssl/context.hpp>

#include <memory>

namespace bmcweb
{

// Interface for SSL context factory
class ISslContextFactory
{
  public:
    virtual ~ISslContextFactory() = default;

    // Create and return SSL context
    virtual std::shared_ptr<boost::asio::ssl::context> createContext() = 0;
};

// Default SSL context factory that uses ensuressl
class DefaultSslContextFactory : public ISslContextFactory
{
  public:
    std::shared_ptr<boost::asio::ssl::context> createContext() override
    {
        return ensuressl::getSslServerContext();
    }
};

} // namespace bmcweb
