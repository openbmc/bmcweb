#ifndef RESOURCE_V1
#define RESOURCE_V1

#include "NavigationReference.h"
#include "Resource_v1.h"

#include <chrono>

enum class Resource_v1_DurableNameFormat
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
enum class Resource_v1_Health
{
    OK,
    Warning,
    Critical,
};
enum class Resource_v1_IndicatorLED
{
    Lit,
    Blinking,
    Off,
};
enum class Resource_v1_LocationType
{
    Slot,
    Bay,
    Connector,
    Socket,
    Backplane,
};
enum class Resource_v1_Orientation
{
    FrontToBack,
    BackToFront,
    TopToBottom,
    BottomToTop,
    LeftToRight,
    RightToLeft,
};
enum class Resource_v1_PowerState
{
    On,
    Off,
    PoweringOn,
    PoweringOff,
};
enum class Resource_v1_RackUnits
{
    OpenU,
    EIA_310,
};
enum class Resource_v1_Reference
{
    Top,
    Bottom,
    Front,
    Rear,
    Left,
    Right,
    Middle,
};
enum class Resource_v1_ResetType
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
enum class Resource_v1_State
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
struct Resource_v1_Condition
{
    // TODO jebr manual adjustment for chrono add <std::chrono::system_clock>
    std::chrono::time_point<std::chrono::system_clock> timestamp;
    std::string messageId;
    std::string messageArgs;
    std::string message;
    Resource_v1_Health severity;
    NavigationReference_ originOfCondition;
    NavigationReference_ logEntry;
};
struct Resource_v1_ContactInfo
{
    std::string contactName;
    std::string phoneNumber;
    std::string emailAddress;
};
struct Resource_v1_Identifier
{
    std::string durableName;
    Resource_v1_DurableNameFormat durableNameFormat;
};
struct Resource_v1_Oem
{};
struct Resource_v1_Item
{
    Resource_v1_Oem oem;
};
struct Resource_v1_ItemOrCollection
{};
struct Resource_v1_Links
{
    Resource_v1_Oem oem;
};
struct Resource_v1_PostalAddress
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
struct Resource_v1_Placement
{
    std::string row;
    std::string rack;
    Resource_v1_RackUnits rackOffsetUnits;
    int64_t rackOffset;
    std::string additionalInfo;
};
struct Resource_v1_PartLocation
{
    std::string serviceLabel;
    Resource_v1_LocationType locationType;
    int64_t locationOrdinalValue;
    Resource_v1_Reference reference;
    Resource_v1_Orientation orientation;
};
struct Resource_v1_Location
{
    std::string info;
    std::string infoFormat;
    Resource_v1_Oem oem;
    Resource_v1_PostalAddress postalAddress;
    Resource_v1_Placement placement;
    Resource_v1_PartLocation partLocation;
    double longitude;
    double latitude;
    double altitudeMeters;
    Resource_v1_ContactInfo contacts;
};
struct Resource_v1_OemObject
{};
struct Resource_v1_ReferenceableMember
{
    Resource_v1_Oem oem;
    std::string memberId;
};
struct Resource_v1_Resource
{
    Resource_v1_Oem oem;
    std::string id;
    std::string description;
    std::string name;
};
struct Resource_v1_ResourceCollection
{
    std::string description;
    std::string name;
    Resource_v1_Oem oem;
};
struct Resource_v1_Status
{
    Resource_v1_State state;
    Resource_v1_Health healthRollup;
    Resource_v1_Health health;
    Resource_v1_Condition conditions;
    Resource_v1_Oem oem;
};
#endif
