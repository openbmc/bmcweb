#ifndef SERIALINTERFACE_V1
#define SERIALINTERFACE_V1

#include "Resource_v1.h"
#include "SerialInterface_v1.h"

enum class SerialInterface_v1_FlowControl
{
    None,
    Software,
    Hardware,
};
enum class SerialInterface_v1_Parity
{
    None,
    Even,
    Odd,
    Mark,
    Space,
};
enum class SerialInterface_v1_PinOut
{
    Cisco,
    Cyclades,
    Digi,
};
enum class SerialInterface_v1_SignalType
{
    Rs232,
    Rs485,
};
struct SerialInterface_v1_Actions
{
    SerialInterface_v1_OemActions oem;
};
struct SerialInterface_v1_OemActions
{};
struct SerialInterface_v1_SerialInterface
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    bool interfaceEnabled;
    SerialInterface_v1_SignalType signalType;
    std::string bitRate;
    SerialInterface_v1_Parity parity;
    std::string dataBits;
    std::string stopBits;
    SerialInterface_v1_FlowControl flowControl;
    std::string connectorType;
    SerialInterface_v1_PinOut pinOut;
    SerialInterface_v1_Actions actions;
};
#endif
