#include "crow/query_string.h"
#include "crow/http_parser_merged.h"
#include "crow/ci_map.h"
//#include "crow/TinySHA1.hpp"
#include "crow/settings.h"
#include "crow/socket_adaptors.h"
#include "crow/json.h"
#include "crow/mustache.h"
#include "crow/logging.h"
#include "crow/dumb_timer_queue.h"
#include "crow/utility.h"
#include "crow/common.h"
#include "crow/http_request.h"
#include "crow/websocket.h"
#include "crow/parser.h"
#include "crow/http_response.h"
#include "crow/middleware.h"
#include "crow/routing.h"
#include "crow/middleware_context.h"
#include "crow/http_connection.h"
#include "crow/http_server.h"
#include "crow/app.h"

#include "color_cout_g3_sink.hpp"

#include "ssl_key_handler.hpp"
#include <iostream>
#include <string>



struct ExampleMiddleware 
{
    std::string message;

    ExampleMiddleware() 
    {
        message = "foo";
    }

    void setMessage(std::string newMsg)
    {
        message = newMsg;
    }

    struct context
    {
    };

    void before_handle(crow::request& /*req*/, crow::response& /*res*/, context& /*ctx*/)
    {
        CROW_LOG_DEBUG << " - MESSAGE: " << message;
    }

    void after_handle(crow::request& /*req*/, crow::response& /*res*/, context& /*ctx*/)
    {
        // no-op
    }
};



int main(int argc, char** argv)
{
   auto worker = g3::LogWorker::createLogWorker();
   auto handle= worker->addDefaultLogger(argv[0], "/tmp/");
   g3::initializeLogging(worker.get());
   auto log_file_name = handle->call(&g3::FileSink::fileName);
   auto sink_handle = worker->addSink(std::make_unique<crow::ColorCoutSink>(),
                                     &crow::ColorCoutSink::ReceiveLogMessage);

   LOG(DEBUG) << "Logging to " << log_file_name.get() << "\n";

    std::string ssl_pem_file("server.pem");
    ensuressl::ensure_openssl_key_present_and_valid(ssl_pem_file);
    //auto handler2 = std::make_shared<ExampleLogHandler>();
    //crow::logger::setHandler(handler2.get());
    crow::App<ExampleMiddleware> app;

    app.get_middleware<ExampleMiddleware>().setMessage("hello");

    CROW_ROUTE(app, "/")
        .name("hello")
    ([]{
        return "Hello World!";
    });

    CROW_ROUTE(app, "/about")
    ([](){
        return "About Crow example.";
    });

    // a request to /path should be forwarded to /path/
    CROW_ROUTE(app, "/path/")
    ([](){
        return "Trailing slash test case..";
    });


    // simple json response
    // To see it in action enter {ip}:18080/json
    CROW_ROUTE(app, "/json")
    ([]{
        crow::json::wvalue x;
        x["message"] = "Hello, World!";
        return x;
    });

    // To see it in action enter {ip}:18080/hello/{integer_between -2^32 and 100} and you should receive
    // {integer_between -2^31 and 100} bottles of beer!
    CROW_ROUTE(app,"/hello/<int>")
    ([](int count){
        if (count > 100)
            return crow::response(400);
        std::ostringstream os;
        os << count << " bottles of beer!";
        return crow::response(os.str());
    });

    // To see it in action submit {ip}:18080/add/1/2 and you should receive 3 (exciting, isn't it)
    CROW_ROUTE(app,"/add/<int>/<int>")
    ([](const crow::request& /*req*/, crow::response& res, int a, int b){
        std::ostringstream os;
        os << a+b;
        res.write(os.str());
        res.end();
    });

    // Compile error with message "Handler type is mismatched with URL paramters"
    //CROW_ROUTE(app,"/another/<int>")
    //([](int a, int b){
        //return crow::response(500);
    //});

    // more json example

    // To see it in action, I recommend to use the Postman Chrome extension:
    //      * Set the address to {ip}:18080/add_json
    //      * Set the method to post
    //      * Select 'raw' and then JSON
    //      * Add {"a": 1, "b": 1}
    //      * Send and you should receive 2

    // A simpler way for json example:
    //      * curl -d '{"a":1,"b":2}' {ip}:18080/add_json
    CROW_ROUTE(app, "/add_json")
        .methods("POST"_method)
    ([](const crow::request& req){
        auto x = crow::json::load(req.body);
        if (!x)
            return crow::response(400);
        int sum = x["a"].i()+x["b"].i();
        std::ostringstream os;
        os << sum;
        return crow::response{os.str()};
    });

    // Example of a request taking URL parameters
    // If you want to activate all the functions just query
    // {ip}:18080/params?foo='blabla'&pew=32&count[]=a&count[]=b
    CROW_ROUTE(app, "/params")
    ([](const crow::request& req){
        std::ostringstream os;

        // To get a simple string from the url params
        // To see it in action /params?foo='blabla'
        os << "Params: " << req.url_params << "\n\n"; 
        os << "The key 'foo' was " << (req.url_params.get("foo") == nullptr ? "not " : "") << "found.\n";

        // To get a double from the request
        // To see in action submit something like '/params?pew=42'
        if(req.url_params.get("pew") != nullptr) {
            double countD = boost::lexical_cast<double>(req.url_params.get("pew"));
            os << "The value of 'pew' is " <<  countD << '\n';
        }

        // To get a list from the request
        // You have to submit something like '/params?count[]=a&count[]=b' to have a list with two values (a and b)
        auto count = req.url_params.get_list("count");
        os << "The key 'count' contains " << count.size() << " value(s).\n";
        for(const auto& countVal : count) {
            os << " - " << countVal << '\n';
        }
        return crow::response{os.str()};
    });    

    CROW_ROUTE(app, "/large")
    ([]{
        return std::string(512*1024, ' ');
    });

    // ignore all log
    crow::logger::setLogLevel(crow::LogLevel::DEBUG);

    app.port(18080)
        .multithreaded()
        .run();
}
