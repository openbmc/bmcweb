#ifndef PORTMETRICS_V1
#define PORTMETRICS_V1

#include "PortMetrics_v1.h"
#include "Resource_v1.h"

struct PortMetrics_v1_Actions
{
    PortMetrics_v1_OemActions oem;
};
struct PortMetrics_v1_GenZ
{
    int64_t packetCRCErrors;
    int64_t endToEndCRCErrors;
    int64_t rXStompedECRC;
    int64_t tXStompedECRC;
    int64_t nonCRCTransientErrors;
    int64_t lLRRecovery;
    int64_t markedECN;
    int64_t packetDeadlineDiscards;
    int64_t accessKeyViolations;
    int64_t linkNTE;
    int64_t receivedECN;
};
struct PortMetrics_v1_OemActions
{};
struct PortMetrics_v1_PortMetrics
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    PortMetrics_v1_GenZ genZ;
    PortMetrics_v1_Actions actions;
};
#endif
