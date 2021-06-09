#ifndef COMPOSITIONRESERVATION_V1
#define COMPOSITIONRESERVATION_V1

#include "CompositionReservation_v1.h"
#include "Manifest_v1.h"
#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

#include <chrono>

struct CompositionReservationV1OemActions
{};
struct CompositionReservationV1Actions
{
    CompositionReservationV1OemActions oem;
};
struct CompositionReservationV1CompositionReservation
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::chrono::time_point<std::chrono::system_clock> reservationTime;
    std::string client;
    NavigationReferenceRedfish reservedResourceBlocks;
    ManifestV1Manifest manifest;
    CompositionReservationV1Actions actions;
};
#endif
