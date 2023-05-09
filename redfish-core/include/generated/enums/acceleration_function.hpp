#pragma once
#include <boost/describe/enum.hpp>
#include <nlohmann/json.hpp>

namespace acceleration_function
{
// clang-format off

enum class AccelerationFunctionType{
    Invalid,
    Encryption,
    Compression,
    PacketInspection,
    PacketSwitch,
    Scheduler,
    AudioProcessing,
    VideoProcessing,
    OEM,
};

NLOHMANN_JSON_SERIALIZE_ENUM(AccelerationFunctionType, {
    {AccelerationFunctionType::Invalid, "Invalid"},
    {AccelerationFunctionType::Encryption, "Encryption"},
    {AccelerationFunctionType::Compression, "Compression"},
    {AccelerationFunctionType::PacketInspection, "PacketInspection"},
    {AccelerationFunctionType::PacketSwitch, "PacketSwitch"},
    {AccelerationFunctionType::Scheduler, "Scheduler"},
    {AccelerationFunctionType::AudioProcessing, "AudioProcessing"},
    {AccelerationFunctionType::VideoProcessing, "VideoProcessing"},
    {AccelerationFunctionType::OEM, "OEM"},
});

BOOST_DESCRIBE_ENUM(AccelerationFunctionType,

    Invalid,
    Encryption,
    Compression,
    PacketInspection,
    PacketSwitch,
    Scheduler,
    AudioProcessing,
    VideoProcessing,
    OEM,
);

}
// clang-format on
