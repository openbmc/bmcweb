#ifndef PORTMETRICS_V1
#define PORTMETRICS_V1

#include "PortMetrics_v1.h"
#include "Resource_v1.h"

struct PortMetricsV1OemActions
{};
struct PortMetricsV1Actions
{
    PortMetricsV1OemActions oem;
};
struct PortMetricsV1GenZ
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
struct PortMetricsV1Networking
{
    int64_t rXFrames;
    int64_t rXUnicastFrames;
    int64_t rXMulticastFrames;
    int64_t rXBroadcastFrames;
    int64_t tXFrames;
    int64_t tXUnicastFrames;
    int64_t tXMulticastFrames;
    int64_t tXBroadcastFrames;
    int64_t rXDiscards;
    int64_t rXFrameAlignmentErrors;
    int64_t rXFCSErrors;
    int64_t rXFalseCarrierErrors;
    int64_t rXOversizeFrames;
    int64_t rXUndersizeFrames;
    int64_t tXDiscards;
    int64_t tXExcessiveCollisions;
    int64_t tXLateCollisions;
    int64_t tXMultipleCollisions;
    int64_t tXSingleCollisions;
    int64_t rXPFCFrames;
    int64_t tXPFCFrames;
    int64_t rXPauseXOFFFrames;
    int64_t rXPauseXONFrames;
    int64_t tXPauseXOFFFrames;
    int64_t tXPauseXONFrames;
    int64_t rDMARXBytes;
    int64_t rDMARXRequests;
    int64_t rDMAProtectionErrors;
    int64_t rDMAProtocolErrors;
    int64_t rDMATXBytes;
    int64_t rDMATXRequests;
    int64_t rDMATXReadRequests;
    int64_t rDMATXSendRequests;
    int64_t rDMATXWriteRequests;
};
struct PortMetricsV1Transceiver
{
    double rXInputPowerMilliWatts;
    double tXBiasCurrentMilliAmps;
    double tXOutputPowerMilliWatts;
    double supplyVoltage;
};
struct PortMetricsV1SAS
{
    int64_t invalidDwordCount;
    int64_t runningDisparityErrorCount;
    int64_t lossOfDwordSynchronizationCount;
};
struct PortMetricsV1PortMetrics
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    PortMetricsV1GenZ genZ;
    PortMetricsV1Actions actions;
    int64_t rXBytes;
    int64_t tXBytes;
    int64_t rXErrors;
    int64_t tXErrors;
    PortMetricsV1Networking networking;
    PortMetricsV1Transceiver transceivers;
    PortMetricsV1SAS SAS;
};
#endif
