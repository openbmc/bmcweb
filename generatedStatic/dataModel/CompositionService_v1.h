#ifndef COMPOSITIONSERVICE_V1
#define COMPOSITIONSERVICE_V1

#include "CompositionReservationCollection_v1.h"
#include "CompositionService_v1.h"
#include "Manifest_v1.h"
#include "ResourceBlockCollection_v1.h"
#include "Resource_v1.h"
#include "ZoneCollection_v1.h"

#include <chrono>

enum class CompositionServiceV1ComposeRequestFormat
{
    Manifest,
};
enum class CompositionServiceV1ComposeRequestType
{
    Preview,
    PreviewReserve,
    Apply,
};
struct CompositionServiceV1OemActions
{};
struct CompositionServiceV1Actions
{
    CompositionServiceV1OemActions oem;
};
struct CompositionServiceV1ComposeResponse
{
    CompositionServiceV1ComposeRequestFormat requestFormat;
    CompositionServiceV1ComposeRequestType requestType;
    ManifestV1Manifest manifest;
    std::string reservationId;
};
struct CompositionServiceV1CompositionService
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource status;
    bool serviceEnabled;
    CompositionServiceV1Actions actions;
    ResourceBlockCollectionV1ResourceBlockCollection resourceBlocks;
    ZoneCollectionV1ZoneCollection resourceZones;
    bool allowOverprovisioning;
    bool allowZoneAffinity;
    ResourceBlockCollectionV1ResourceBlockCollection activePool;
    ResourceBlockCollectionV1ResourceBlockCollection freePool;
    CompositionReservationCollectionV1CompositionReservationCollection
        compositionReservations;
    std::chrono::milliseconds reservationDuration;
};
#endif
