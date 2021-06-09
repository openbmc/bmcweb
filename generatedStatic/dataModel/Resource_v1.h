#ifndef RESOURCE_V1
#define RESOURCE_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

#include <chrono>

enum class ResourceV1DurableNameFormat
{
    NAA,
    iQN,
    FC_WWN,
    UUID,
    EUI,
    NQN,
    NSID,
    NGUID,
};
enum class ResourceV1Health
{
    OK,
    Warning,
    Critical,
};
enum class ResourceV1IndicatorLED
{
    Lit,
    Blinking,
    Off,
};
enum class ResourceV1LocationType
{
    Slot,
    Bay,
    Connector,
    Socket,
    Backplane,
};
enum class ResourceV1Orientation
{
    FrontToBack,
    BackToFront,
    TopToBottom,
    BottomToTop,
    LeftToRight,
    RightToLeft,
};
enum class ResourceV1PowerState
{
    On,
    Off,
    PoweringOn,
    PoweringOff,
};
enum class ResourceV1RackUnits
{
    OpenU,
    EIA_310,
};
enum class ResourceV1Reference
{
    Top,
    Bottom,
    Front,
    Rear,
    Left,
    Right,
    Middle,
};
enum class ResourceV1ResetType
{
    On,
    ForceOff,
    GracefulShutdown,
    GracefulRestart,
    ForceRestart,
    Nmi,
    ForceOn,
    PushPowerButton,
    PowerCycle,
};
enum class ResourceV1State
{
    Enabled,
    Disabled,
    StandbyOffline,
    StandbySpare,
    InTest,
    Starting,
    Absent,
    UnavailableOffline,
    Deferring,
    Quiesced,
    Updating,
    Qualified,
};
struct ResourceV1Condition
{
    std::chrono::time_point<std::chrono::system_clock> timestamp;
    std::string messageId;
    std::string messageArgs;
    std::string message;
    ResourceV1Health severity;
    NavigationReferenceRedfish originOfCondition;
    NavigationReferenceRedfish logEntry;
};
struct ResourceV1ContactInfo
{
    std::string contactName;
    std::string phoneNumber;
    std::string emailAddress;
};
struct ResourceV1Identifier
{
    std::string durableName;
    ResourceV1DurableNameFormat durableNameFormat;
};
struct ResourceV1Oem
{};
struct ResourceV1Item
{
    ResourceV1Oem oem;
};
struct ResourceV1ItemOrCollection
{};
struct ResourceV1Links
{
    ResourceV1Oem oem;
};
struct ResourceV1PostalAddress
{
    std::string country;
    std::string territory;
    std::string district;
    std::string city;
    std::string division;
    std::string neighborhood;
    std::string leadingStreetDirection;
    std::string street;
    std::string trailingStreetSuffix;
    std::string streetSuffix;
    int64_t houseNumber;
    std::string houseNumberSuffix;
    std::string landmark;
    std::string location;
    std::string floor;
    std::string name;
    std::string postalCode;
    std::string building;
    std::string unit;
    std::string room;
    std::string seat;
    std::string placeType;
    std::string community;
    std::string pOBox;
    std::string additionalCode;
    std::string road;
    std::string roadSection;
    std::string roadBranch;
    std::string roadSubBranch;
    std::string roadPreModifier;
    std::string roadPostModifier;
    std::string gPSCoords;
    std::string additionalInfo;
};
struct ResourceV1Placement
{
    std::string row;
    std::string rack;
    ResourceV1RackUnits rackOffsetUnits;
    int64_t rackOffset;
    std::string additionalInfo;
};
struct ResourceV1PartLocation
{
    std::string serviceLabel;
    ResourceV1LocationType locationType;
    int64_t locationOrdinalValue;
    ResourceV1Reference reference;
    ResourceV1Orientation orientation;
};
struct ResourceV1Location
{
    std::string info;
    std::string infoFormat;
    ResourceV1Oem oem;
    ResourceV1PostalAddress postalAddress;
    ResourceV1Placement placement;
    ResourceV1PartLocation partLocation;
    double longitude;
    double latitude;
    double altitudeMeters;
    ResourceV1ContactInfo contacts;
};
struct ResourceV1OemObject
{};
struct ResourceV1ReferenceableMember
{
    ResourceV1Oem oem;
    std::string memberId;
};
struct ResourceV1Resource
{
    ResourceV1Oem oem;
    std::string id;
    std::string description;
    std::string name;
};
struct ResourceV1ResourceCollection
{
    std::string description;
    std::string name;
    ResourceV1Oem oem;
};
struct ResourceV1Status
{
    ResourceV1State state;
    ResourceV1Health healthRollup;
    ResourceV1Health health;
    ResourceV1Condition conditions;
    ResourceV1Oem oem;
};
#endif
