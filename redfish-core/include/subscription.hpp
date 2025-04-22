/*
Copyright (c) 2020 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#pragma once
#include "event_logs_object_type.hpp"
#include "event_service_store.hpp"
#include "filter_expr_parser_ast.hpp"
#include "http_client.hpp"
#include "http_response.hpp"
#include "metric_report.hpp"
#include "server_sent_event.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/url/url_view_base.hpp>

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace redfish
{

static constexpr const char* subscriptionTypeSSE = "SSE";

static constexpr const uint8_t maxNoOfSubscriptions = 20;
static constexpr const uint8_t maxNoOfSSESubscriptions = 10;
struct TestEvent
{
    std::optional<int64_t> eventGroupId;
    std::optional<std::string> eventTimestamp;
    std::optional<std::string> message;
    std::optional<std::vector<std::string>> messageArgs;
    std::optional<std::string> messageId;
    std::optional<std::string> originOfCondition;
    std::optional<std::string> resolution;
    std::optional<std::string> severity;
};

class Subscription : public std::enable_shared_from_this<Subscription>
{
  public:
    Subscription(const Subscription&) = delete;
    Subscription& operator=(const Subscription&) = delete;
    Subscription(Subscription&&) = delete;
    Subscription& operator=(Subscription&&) = delete;

    Subscription(std::shared_ptr<persistent_data::UserSubscription> userSubIn,
                 const boost::urls::url_view_base& url,
                 boost::asio::io_context& ioc);

    explicit Subscription(crow::sse_socket::Connection& connIn);

    ~Subscription() = default;

    // callback for subscription sendData
    void resHandler(const std::shared_ptr<Subscription>& /*self*/,
                    const crow::Response& res);

    void sendHeartbeatEvent();
    void scheduleNextHeartbeatEvent();
    void heartbeatParametersChanged();
    void onHbTimeout(const std::weak_ptr<Subscription>& weakSelf,
                     const boost::system::error_code& ec);

    bool sendEventToSubscriber(uint64_t eventId, std::string&& msg);

    void filterAndSendEventLogs(
        uint64_t eventId, const std::vector<EventLogObjectsType>& eventRecords);

    void filterAndSendReports(uint64_t eventId, const std::string& reportId,
                              const telemetry::TimestampReadings& var);

    void updateRetryConfig(uint32_t retryAttempts,
                           uint32_t retryTimeoutInterval);

    bool matchSseId(const crow::sse_socket::Connection& thisConn);

    // Check used to indicate what response codes are valid as part of our retry
    // policy.  2XX is considered acceptable
    static boost::system::error_code retryRespHandler(unsigned int respCode);

    std::shared_ptr<persistent_data::UserSubscription> userSub;
    std::function<void()> deleter;

  private:
    boost::urls::url host;
    std::shared_ptr<crow::ConnectionPolicy> policy;
    crow::sse_socket::Connection* sseConn = nullptr;

    boost::asio::steady_timer hbTimer;
    std::optional<crow::HttpClient> client;

  public:
    std::optional<filter_ast::LogicalAnd> filter;
};

} // namespace redfish
