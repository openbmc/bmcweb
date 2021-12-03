#include "async_resp.hpp"
#include "http/http_connection.hpp"
#include "http/timer_queue.hpp"

#include <boost/beast/_experimental/test/stream.hpp>

// A do nothing handler
struct MockHandler
{
    void handle(crow::Request&, std::shared_ptr<bmcweb::AsyncResp>&)
    {}

    void handleUpgrade(crow::Request&, crow::Response&,
                       boost::beast::test::stream&&)
    {}
};
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size);

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    crow::detail::TimerQueue timerQueue;

    std::shared_ptr<boost::asio::io_context> io =
        std::make_shared<boost::asio::io_context>();
    MockHandler requestHandler;

    std::function<std::string()> getDateString = []() -> std::string {
        return "<SomeDate>";
    };

    boost::beast::test::stream send{*io};
    boost::beast::test::stream receive{*io};
    send.connect(receive);
    send.write_some(boost::asio::buffer(data, size));
    send.close();
    std::shared_ptr<crow::Connection<boost::beast::test::stream, MockHandler>>
        conn = std::make_shared<
            crow::Connection<boost::beast::test::stream, MockHandler>>(
            &requestHandler, getDateString, timerQueue, std::move(receive));
    conn->start();

    io->run();

    return 0;
}
