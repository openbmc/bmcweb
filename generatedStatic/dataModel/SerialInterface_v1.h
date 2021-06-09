#ifndef SERIALINTERFACE_V1
#define SERIALINTERFACE_V1

#include "Resource_v1.h"
#include "SerialInterface_v1.h"

enum class SerialInterfaceV1FlowControl
{
    None,
    Software,
    Hardware,
};
enum class SerialInterfaceV1Parity
{
    None,
    Even,
    Odd,
    Mark,
    Space,
};
enum class SerialInterfaceV1PinOut
{
    Cisco,
    Cyclades,
    Digi,
};
enum class SerialInterfaceV1SignalType
{
    Rs232,
    Rs485,
};
struct SerialInterfaceV1OemActions
{};
struct SerialInterfaceV1Actions
{
    SerialInterfaceV1OemActions oem;
};
struct SerialInterfaceV1SerialInterface
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    bool interfaceEnabled;
    SerialInterfaceV1SignalType signalType;
    std::string bitRate;
    SerialInterfaceV1Parity parity;
    std::string dataBits;
    std::string stopBits;
    SerialInterfaceV1FlowControl flowControl;
    std::string connectorType;
    SerialInterfaceV1PinOut pinOut;
    SerialInterfaceV1Actions actions;
};
#endif
