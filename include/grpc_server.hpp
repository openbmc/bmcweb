/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "redfish_v1.grpc.pb.h"
#include "redfish_v1.pb.h"

#include "struct_proto_conversion.hpp"

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>

#include <http/app.hpp>
#include <http/http_request.hpp>
#include <http/http_response.hpp>

#include <memory>
#include <string>
#include <thread>

class RedfishV1_Impl final : public redfish::v1::RedfishV1::Service
{

    grpc::Status request_uri(boost::beast::http::verb method,
                             const redfish::v1::Request* request,
                             google::protobuf::Struct* response)
    {
        nlohmann::json json_req = request->message();

        boost::beast::http::request<boost::beast::http::string_body> breq;
        breq.target(request->url());
        breq.method(method);
        if (!json_req.is_null())
        {
            breq.body() = json_req.dump();
        }

        std::error_code reqEc;
        crow::Request req(breq, reqEc);
        if (reqEc)
        {
            BMCWEB_LOG_DEBUG << "Request failed to construct" << reqEc;
            return grpc::Status(grpc::StatusCode::INTERNAL,
                                "Request failed to construct");
        }

        crow::Response res;
        std::promise<void> done;
        app.handle(req, std::make_shared<bmcweb::AsyncResp>(
                            res, [&done] { done.set_value(); }));
        done.get_future().wait();

        from_json(*response, res.jsonValue);

        if (boost::beast::http::to_status_class(res.result()) ==
            boost::beast::http::status_class::successful)
        {
            return grpc::Status::OK;
        }
        else
        {
            std::ostringstream out;
            out << res.result();
            return grpc::Status(grpc::StatusCode::INTERNAL, out.str());
        }
    }

  public:
    RedfishV1_Impl(App& appIn) : app(appIn)
    {}

    inline grpc::Status Get(grpc::ServerContext* /* context */,
                            const redfish::v1::Request* request,
                            google::protobuf::Struct* response) override
    {
        return request_uri(boost::beast::http::verb::get, request, response);
    }

    inline grpc::Status Post(grpc::ServerContext* /* context */,
                             const redfish::v1::Request* request,
                             google::protobuf::Struct* response) override
    {
        return request_uri(boost::beast::http::verb::post, request, response);
    }

    inline grpc::Status Put(grpc::ServerContext* /* context */,
                            const redfish::v1::Request* request,
                            google::protobuf::Struct* response) override
    {
        return request_uri(boost::beast::http::verb::put, request, response);
    }

    inline grpc::Status Patch(grpc::ServerContext* /* context */,
                              const redfish::v1::Request* request,
                              google::protobuf::Struct* response) override
    {
        return request_uri(boost::beast::http::verb::patch, request, response);
    }

    inline grpc::Status Delete(grpc::ServerContext* /* context */,
                               const redfish::v1::Request* request,
                               google::protobuf::Struct* response) override
    {
        return request_uri(boost::beast::http::verb::delete_, request,
                           response);
    }

    App& app;
};

void RunServer(App& app)
{
    BMCWEB_LOG_INFO << "Start gRPC server...";
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    std::string server_address("0.0.0.0:50051");
    RedfishV1_Impl service(app);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    server->Wait();
}
