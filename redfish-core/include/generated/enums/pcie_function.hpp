#pragma once
#include <nlohmann/json.hpp>

namespace pcie_function
{
// clang-format off

enum class DeviceClass{
    Invalid,
    UnclassifiedDevice,
    MassStorageController,
    NetworkController,
    DisplayController,
    MultimediaController,
    MemoryController,
    Bridge,
    CommunicationController,
    GenericSystemPeripheral,
    InputDeviceController,
    DockingStation,
    Processor,
    SerialBusController,
    WirelessController,
    IntelligentController,
    SatelliteCommunicationsController,
    EncryptionController,
    SignalProcessingController,
    ProcessingAccelerators,
    NonEssentialInstrumentation,
    Coprocessor,
    UnassignedClass,
    Other,
};

enum class FunctionType{
    Invalid,
    Physical,
    Virtual,
};

enum class FunctionProtocol{
    Invalid,
    PCIe,
    CXL,
};

NLOHMANN_JSON_SERIALIZE_ENUM(DeviceClass, {
    {DeviceClass::Invalid, "Invalid"},
    {DeviceClass::UnclassifiedDevice, "UnclassifiedDevice"},
    {DeviceClass::MassStorageController, "MassStorageController"},
    {DeviceClass::NetworkController, "NetworkController"},
    {DeviceClass::DisplayController, "DisplayController"},
    {DeviceClass::MultimediaController, "MultimediaController"},
    {DeviceClass::MemoryController, "MemoryController"},
    {DeviceClass::Bridge, "Bridge"},
    {DeviceClass::CommunicationController, "CommunicationController"},
    {DeviceClass::GenericSystemPeripheral, "GenericSystemPeripheral"},
    {DeviceClass::InputDeviceController, "InputDeviceController"},
    {DeviceClass::DockingStation, "DockingStation"},
    {DeviceClass::Processor, "Processor"},
    {DeviceClass::SerialBusController, "SerialBusController"},
    {DeviceClass::WirelessController, "WirelessController"},
    {DeviceClass::IntelligentController, "IntelligentController"},
    {DeviceClass::SatelliteCommunicationsController, "SatelliteCommunicationsController"},
    {DeviceClass::EncryptionController, "EncryptionController"},
    {DeviceClass::SignalProcessingController, "SignalProcessingController"},
    {DeviceClass::ProcessingAccelerators, "ProcessingAccelerators"},
    {DeviceClass::NonEssentialInstrumentation, "NonEssentialInstrumentation"},
    {DeviceClass::Coprocessor, "Coprocessor"},
    {DeviceClass::UnassignedClass, "UnassignedClass"},
    {DeviceClass::Other, "Other"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(FunctionType, {
    {FunctionType::Invalid, "Invalid"},
    {FunctionType::Physical, "Physical"},
    {FunctionType::Virtual, "Virtual"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(FunctionProtocol, {
    {FunctionProtocol::Invalid, "Invalid"},
    {FunctionProtocol::PCIe, "PCIe"},
    {FunctionProtocol::CXL, "CXL"},
});

}
// clang-format on
