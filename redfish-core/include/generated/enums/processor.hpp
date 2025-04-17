// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

namespace processor
{
// clang-format off

enum class ProcessorType{
    Invalid,
    CPU,
    GPU,
    FPGA,
    DSP,
    Accelerator,
    Core,
    Thread,
    Partition,
    OEM,
};

enum class ProcessorArchitecture{
    Invalid,
    x86,
    IA64,
    ARM,
    MIPS,
    Power,
    RISCV,
    OEM,
};

enum class InstructionSet{
    Invalid,
    x86,
    x8664,
    IA64,
    ARMA32,
    ARMA64,
    MIPS32,
    MIPS64,
    PowerISA,
    RV32,
    RV64,
    OEM,
};

enum class ProcessorMemoryType{
    Invalid,
    Cache,
    L1Cache,
    L2Cache,
    L3Cache,
    L4Cache,
    L5Cache,
    L6Cache,
    L7Cache,
    HBM1,
    HBM2,
    HBM2E,
    HBM3,
    SGRAM,
    GDDR,
    GDDR2,
    GDDR3,
    GDDR4,
    GDDR5,
    GDDR5X,
    GDDR6,
    DDR,
    DDR2,
    DDR3,
    DDR4,
    DDR5,
    SDRAM,
    SRAM,
    Flash,
    OEM,
};

enum class FpgaType{
    Invalid,
    Integrated,
    Discrete,
};

enum class SystemInterfaceType{
    Invalid,
    QPI,
    UPI,
    PCIe,
    Ethernet,
    AMBA,
    CCIX,
    CXL,
    OEM,
};

enum class TurboState{
    Invalid,
    Enabled,
    Disabled,
};

enum class BaseSpeedPriorityState{
    Invalid,
    Enabled,
    Disabled,
};

enum class ThrottleCause{
    Invalid,
    PowerLimit,
    ThermalLimit,
    ClockLimit,
    ManagementDetectedFault,
    Unknown,
    OEM,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ProcessorType, {
    {ProcessorType::Invalid, "Invalid"},
    {ProcessorType::CPU, "CPU"},
    {ProcessorType::GPU, "GPU"},
    {ProcessorType::FPGA, "FPGA"},
    {ProcessorType::DSP, "DSP"},
    {ProcessorType::Accelerator, "Accelerator"},
    {ProcessorType::Core, "Core"},
    {ProcessorType::Thread, "Thread"},
    {ProcessorType::Partition, "Partition"},
    {ProcessorType::OEM, "OEM"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ProcessorArchitecture, {
    {ProcessorArchitecture::Invalid, "Invalid"},
    {ProcessorArchitecture::x86, "x86"},
    {ProcessorArchitecture::IA64, "IA-64"},
    {ProcessorArchitecture::ARM, "ARM"},
    {ProcessorArchitecture::MIPS, "MIPS"},
    {ProcessorArchitecture::Power, "Power"},
    {ProcessorArchitecture::RISCV, "RISC-V"},
    {ProcessorArchitecture::OEM, "OEM"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(InstructionSet, {
    {InstructionSet::Invalid, "Invalid"},
    {InstructionSet::x86, "x86"},
    {InstructionSet::x8664, "x86-64"},
    {InstructionSet::IA64, "IA-64"},
    {InstructionSet::ARMA32, "ARM-A32"},
    {InstructionSet::ARMA64, "ARM-A64"},
    {InstructionSet::MIPS32, "MIPS32"},
    {InstructionSet::MIPS64, "MIPS64"},
    {InstructionSet::PowerISA, "PowerISA"},
    {InstructionSet::RV32, "RV32"},
    {InstructionSet::RV64, "RV64"},
    {InstructionSet::OEM, "OEM"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ProcessorMemoryType, {
    {ProcessorMemoryType::Invalid, "Invalid"},
    {ProcessorMemoryType::Cache, "Cache"},
    {ProcessorMemoryType::L1Cache, "L1Cache"},
    {ProcessorMemoryType::L2Cache, "L2Cache"},
    {ProcessorMemoryType::L3Cache, "L3Cache"},
    {ProcessorMemoryType::L4Cache, "L4Cache"},
    {ProcessorMemoryType::L5Cache, "L5Cache"},
    {ProcessorMemoryType::L6Cache, "L6Cache"},
    {ProcessorMemoryType::L7Cache, "L7Cache"},
    {ProcessorMemoryType::HBM1, "HBM1"},
    {ProcessorMemoryType::HBM2, "HBM2"},
    {ProcessorMemoryType::HBM2E, "HBM2E"},
    {ProcessorMemoryType::HBM3, "HBM3"},
    {ProcessorMemoryType::SGRAM, "SGRAM"},
    {ProcessorMemoryType::GDDR, "GDDR"},
    {ProcessorMemoryType::GDDR2, "GDDR2"},
    {ProcessorMemoryType::GDDR3, "GDDR3"},
    {ProcessorMemoryType::GDDR4, "GDDR4"},
    {ProcessorMemoryType::GDDR5, "GDDR5"},
    {ProcessorMemoryType::GDDR5X, "GDDR5X"},
    {ProcessorMemoryType::GDDR6, "GDDR6"},
    {ProcessorMemoryType::DDR, "DDR"},
    {ProcessorMemoryType::DDR2, "DDR2"},
    {ProcessorMemoryType::DDR3, "DDR3"},
    {ProcessorMemoryType::DDR4, "DDR4"},
    {ProcessorMemoryType::DDR5, "DDR5"},
    {ProcessorMemoryType::SDRAM, "SDRAM"},
    {ProcessorMemoryType::SRAM, "SRAM"},
    {ProcessorMemoryType::Flash, "Flash"},
    {ProcessorMemoryType::OEM, "OEM"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(FpgaType, {
    {FpgaType::Invalid, "Invalid"},
    {FpgaType::Integrated, "Integrated"},
    {FpgaType::Discrete, "Discrete"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(SystemInterfaceType, {
    {SystemInterfaceType::Invalid, "Invalid"},
    {SystemInterfaceType::QPI, "QPI"},
    {SystemInterfaceType::UPI, "UPI"},
    {SystemInterfaceType::PCIe, "PCIe"},
    {SystemInterfaceType::Ethernet, "Ethernet"},
    {SystemInterfaceType::AMBA, "AMBA"},
    {SystemInterfaceType::CCIX, "CCIX"},
    {SystemInterfaceType::CXL, "CXL"},
    {SystemInterfaceType::OEM, "OEM"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(TurboState, {
    {TurboState::Invalid, "Invalid"},
    {TurboState::Enabled, "Enabled"},
    {TurboState::Disabled, "Disabled"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(BaseSpeedPriorityState, {
    {BaseSpeedPriorityState::Invalid, "Invalid"},
    {BaseSpeedPriorityState::Enabled, "Enabled"},
    {BaseSpeedPriorityState::Disabled, "Disabled"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ThrottleCause, {
    {ThrottleCause::Invalid, "Invalid"},
    {ThrottleCause::PowerLimit, "PowerLimit"},
    {ThrottleCause::ThermalLimit, "ThermalLimit"},
    {ThrottleCause::ClockLimit, "ClockLimit"},
    {ThrottleCause::ManagementDetectedFault, "ManagementDetectedFault"},
    {ThrottleCause::Unknown, "Unknown"},
    {ThrottleCause::OEM, "OEM"},
});

}
// clang-format on
