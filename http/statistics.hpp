#pragma once

#include <boost/beast/http/status.hpp>
#include <boost/container/flat_map.hpp>

#include <chrono>

namespace bmcweb
{

// Duration type that uses int64_t on 32 bit systems
using int64_duration = std::chrono::duration<int64_t, std::milli>;

struct ConnectionStatistics
{
    int64_t opened = 0;
    int64_t closed = 0;
    int64_t bytesSent = 0;
    int64_t bytesReceived = 0;
    int64_t requestCount = 0;
    int64_t responseCount = 0;

    using status = boost::beast::http::status;
    // Prepopulate with existing codes to make it easier on users.
    boost::container::flat_map<status, unsigned> responseStatus{
        {status::accepted, 0},
        {status::bad_gateway, 0},
        {status::bad_request, 0},
        {status::conflict, 0},
        {status::created, 0},
        {status::forbidden, 0},
        {status::insufficient_storage, 0},
        {status::internal_server_error, 0},
        {status::method_not_allowed, 0},
        {status::no_content, 0},
        {status::not_acceptable, 0},
        {status::not_found, 0},
        {status::not_implemented, 0},
        {status::not_modified, 0},
        {status::ok, 0},
        {status::precondition_failed, 0},
        {status::service_unavailable, 0},
        {status::temporary_redirect, 0},
        {status::too_many_requests, 0},
        {status::unauthorized, 0},
        {status::unsupported_media_type, 0}};

    // Connections go through a lifecycle
    // TLS ─► Read ─► Process ─► Write
    //         ▲                   │
    //         └───── Idle ◄───────┘
    //
    // The following counters reprent the total time that any connection spent
    // in the various states.
    int64_duration tlsHandshakeTotalMs{};
    int64_duration requestLatencyTotalMs{};
    int64_duration processingLatencyTotalMs{};
    int64_duration responseWriteLatencyTotalMs{};
    int64_duration idleLatencyTotalMs{};

    void appendResponseCode(status code)
    {
        responseStatus[code]++;
    }
};

ConnectionStatistics& stats()
{
    static ConnectionStatistics stats;
    return stats;
}
} // namespace bmcweb
