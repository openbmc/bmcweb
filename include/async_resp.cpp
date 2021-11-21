#include "async_resp_class_decl.hpp"

namespace bmcweb
{

AsyncResp::AsyncResp(crow::Response& response) : res(response)
{}

AsyncResp::AsyncResp(crow::Response& response, std::function<void()>&& function) :
    res(response), func(std::move(function))
{}

AsyncResp::~AsyncResp()
{
    if (func && res.result() == boost::beast::http::status::ok)
    {
        func();
    }

    res.end();
}

}