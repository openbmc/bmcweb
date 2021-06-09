#ifndef IOSTATISTICS_V1
#define IOSTATISTICS_V1

struct IOStatistics_v1_IOStatistics
{
    int64_t readIORequests;
    int64_t readHitIORequests;
    int64_t readIOKiBytes;
    std::string readIORequestTime;
    int64_t writeIORequests;
    int64_t writeHitIORequests;
    int64_t writeIOKiBytes;
    std::string writeIORequestTime;
    int64_t nonIORequests;
    std::string nonIORequestTime;
};
#endif
