#include "boost/beast/core/flat_buffer.hpp"
#include "boost/beast/http/serializer.hpp"
#include "http/http_connection.hpp"
#include "http/http_response.hpp"
#include "simple_client.hpp"
#include "testapp.hpp"

#include <filesystem>
#include <fstream>
#include <thread>

#include "gtest/gtest.h"
using namespace crow;
using namespace boost::beast::http;
using namespace boost::asio;
using namespace boost::beast;
namespace
{
inline auto stringSplitter(char c)
{
    return std::views::split(c) | std::views::transform([](auto&& sub) {
               return std::string(sub.begin(), sub.end());
           });
}
inline auto split(const std::string_view& input, char c)
{
    auto vw = input | stringSplitter(c);
    return std::vector(vw.begin(), vw.end());
}
struct Router
{
    std::string root = "/tmp/";
    void validate() {}
    void handleUpgrade(
        const Request& /*unused*/,
        const std::shared_ptr<bmcweb::AsyncResp>& /*unused*/,
        boost::beast::ssl_stream<boost::asio::ip::tcp::socket>&& /*unused*/)
    {}
    void handle(Request& req,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) const
    {
        asyncResp->res.addHeader("myheader", "myvalue");
        if (req.target() == "/hello")
        {
            asyncResp->res.body() = "Hello World";
            return;
        }
        auto paths = split(req.target(), '/');

        boost::beast::http::file_body::value_type body;

        auto filetofetch = std::filesystem::path(root).c_str() + paths.back();
        asyncResp->res.openFile(filetofetch);
    }
};
struct Server
{
    using App = TestApp<Router>;
    std::thread serverThread;
    std::shared_ptr<boost::asio::io_context> io{
        std::make_shared<boost::asio::io_context>()};
    App app{io};

    Server()
    {
        serverThread = std::thread([this]() {
            app.port(8082);
            app.run();
            io->run();
        });
    }
    void exit()
    {
        io->stop();
        serverThread.join();
    }
};
class ConnectionTest : public ::testing::Test
{
    Server server;
    std::string_view path = "/tmp/temp.txt";

  protected:
    virtual void SetUp() override
    {
        std::string_view s = "sample text from file";
        std::ofstream file;
        file.open(path.data());
        file << s;
        file.close();
    }

    virtual void TearDown() override
    {
        std::filesystem::remove(path);
        server.exit();
    }
};

TEST_F(ConnectionTest, GetRootStringBody)
{
    SimpleClient client("127.0.0.1", "8082");
    auto res = client.get("/hello");

    EXPECT_EQ(res.body(), "Hello World");
    EXPECT_EQ(res["myheader"], "myvalue");

    EXPECT_EQ(strtol(res["Content-Length"].data()),
              std::string_view("Hello World").length());
}

TEST_F(ConnectionTest, GetRootFileBody)
{
    SimpleClient client("127.0.0.1", "8082");
    auto res = client.get("/temp.txt");
    EXPECT_EQ(res.body(), "sample text from file");
    EXPECT_EQ(res["myheader"], "myvalue");
    EXPECT_EQ(strtol(res["Content-Length"].data()),
              std::string_view("sample text from file").length());
}

} // Namespace
