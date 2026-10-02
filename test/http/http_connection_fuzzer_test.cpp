// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "app.hpp"
#include "async_resp.hpp"
#include "http/http_connection.hpp"
#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "http_connect_types.hpp"
#include "logging.hpp"
#include "redfish.hpp"
#include "test_stream.hpp"

#include <boost/asio/buffer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/steady_timer.hpp>

#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>
#include <utility>

struct FuzzHandler
{
    App app;
    redfish::RedfishService redfishService;
    FuzzHandler() : redfishService(app)
    {
        redfishService.validate();
    }

    template <typename Adaptor>
    void handleUpgrade(const std::shared_ptr<crow::Request>& /*req*/,
                       const std::shared_ptr<bmcweb::AsyncResp>& /*asyncResp*/,
                       Adaptor&& /*adaptor*/)
    {}

    void handle(const std::shared_ptr<crow::Request>& req,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
    {
        app.handle(req, asyncResp);
    }
};

std::string fuzzDateStr()
{
    return "FuzzTime";
}

extern "C"
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-identifier"
// This is a hack to workaround profiler data not being written on shutdown,
// because libfuzzer steals the signal handler.
int __llvm_profile_write_file(void) __attribute__((weak));
#pragma clang diagnostic pop

[[noreturn]] void flushCoverageAndExit(int /*signum*/)
{
    __llvm_profile_write_file();
    std::_Exit(0);
}

void installCoverageSignalHandlers()
{
    if (__llvm_profile_write_file != nullptr)
    {
        std::signal(SIGINT, flushCoverageAndExit);
        std::signal(SIGTERM, flushCoverageAndExit);
    }
}

int LLVMFuzzerInitialize(int* /*argc*/, char*** /*argv*/)
{
    installCoverageSignalHandlers();
    return 0;
}

void runUntilDone(boost::asio::io_context& io, crow::TestStream& clientSide)
{
    bool closed = false;
    while (true)
    {
        size_t numberRun = io.poll_one();
        if (numberRun > 0)
        {
            continue;
        }
        // If we've run out of work to do, close the connection and see if more
        // work happens before we consider this done.
        if (!closed)
        {
            clientSide.close();
            closed = true;
            continue;
        }

        break;
    }
}

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Set the logging level to error. If the code is correct, this should never
    // log anything, although we don't test that at this moment.
    crow::getBmcwebCurrentLoggingLevel() = crow::LogLevel::Error;
    boost::asio::io_context io;

    crow::TestStream serverSide(io);
    crow::TestStream clientSide(io);
    serverSide.connect(clientSide);

    if (size == 0)
    {
        return 0;
    }
    clientSide.write_some(boost::asio::buffer(data, size));

    FuzzHandler handler;
    boost::asio::steady_timer timer(io);
    std::function<std::string()> date(&fuzzDateStr);

    boost::asio::ssl::context ctx{boost::asio::ssl::context::tls};

    auto conn =
        std::make_shared<crow::Connection<crow::TestStream, FuzzHandler>>(
            &handler, crow::HttpType::BOTH, std::move(timer), date,
            boost::asio::ssl::stream<crow::TestStream>(std::move(serverSide),
                                                       ctx));
    conn->disableAuth();
    conn->start();

    runUntilDone(io, clientSide);

    return 0;
}

} // extern "C"
