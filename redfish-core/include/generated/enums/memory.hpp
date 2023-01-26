#pragma once
#include <nlohmann/json.hpp>

namespace memory
{
// clang-format off

enum class MemoryType{
    Invalid,
    DRAM,
    NVDIMM_N,
    NVDIMM_F,
    NVDIMM_P,
    IntelOptane,
};

enum class MemoryDeviceType{
    Invalid,
    DDR,
    DDR2,
    DDR3,
    DDR4,
    DDR4_SDRAM,
    DDR4E_SDRAM,
    LPDDR4_SDRAM,
    DDR3_SDRAM,
    LPDDR3_SDRAM,
    DDR2_SDRAM,
    DDR2_SDRAM_FB_DIMM,
    DDR2_SDRAM_FB_DIMM_PROBE,
    DDR_SGRAM,
    DDR_SDRAM,
    ROM,
    SDRAM,
    EDO,
    FastPageMode,
    PipelinedNibble,
    Logical,
    HBM,
    HBM2,
    HBM2E,
    HBM3,
    GDDR,
    GDDR2,
    GDDR3,
    GDDR4,
    GDDR5,
    GDDR5X,
    GDDR6,
    DDR5,
    OEM,
};

enum class BaseModuleType{
    Invalid,
    RDIMM,
    UDIMM,
    SO_DIMM,
    LRDIMM,
    Mini_RDIMM,
    Mini_UDIMM,
    SO_RDIMM_72b,
    SO_UDIMM_72b,
    SO_DIMM_16b,
    SO_DIMM_32b,
    Die,
};

enum class MemoryMedia{
    Invalid,
    DRAM,
    NAND,
    Intel3DXPoint,
    Proprietary,
};

enum class SecurityStates{
    Invalid,
    Enabled,
    Disabled,
    Unlocked,
    Locked,
    Frozen,
    Passphraselimit,
};

enum class ErrorCorrection{
    Invalid,
    NoECC,
    SingleBitECC,
    MultiBitECC,
    AddressParity,
};

enum class MemoryClassification{
    Invalid,
    Volatile,
    ByteAccessiblePersistent,
    Block,
};

enum class OperatingMemoryModes{
    Invalid,
    Volatile,
    PMEM,
    Block,
};

NLOHMANN_JSON_SERIALIZE_ENUM(MemoryType, {
    {MemoryType::Invalid, "Invalid"},
    {MemoryType::DRAM, "DRAM"},
    {MemoryType::NVDIMM_N, "NVDIMM_N"},
    {MemoryType::NVDIMM_F, "NVDIMM_F"},
    {MemoryType::NVDIMM_P, "NVDIMM_P"},
    {MemoryType::IntelOptane, "IntelOptane"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(MemoryDeviceType, {
    {MemoryDeviceType::Invalid, "Invalid"},
    {MemoryDeviceType::DDR, "DDR"},
    {MemoryDeviceType::DDR2, "DDR2"},
    {MemoryDeviceType::DDR3, "DDR3"},
    {MemoryDeviceType::DDR4, "DDR4"},
    {MemoryDeviceType::DDR4_SDRAM, "DDR4_SDRAM"},
    {MemoryDeviceType::DDR4E_SDRAM, "DDR4E_SDRAM"},
    {MemoryDeviceType::LPDDR4_SDRAM, "LPDDR4_SDRAM"},
    {MemoryDeviceType::DDR3_SDRAM, "DDR3_SDRAM"},
    {MemoryDeviceType::LPDDR3_SDRAM, "LPDDR3_SDRAM"},
    {MemoryDeviceType::DDR2_SDRAM, "DDR2_SDRAM"},
    {MemoryDeviceType::DDR2_SDRAM_FB_DIMM, "DDR2_SDRAM_FB_DIMM"},
    {MemoryDeviceType::DDR2_SDRAM_FB_DIMM_PROBE, "DDR2_SDRAM_FB_DIMM_PROBE"},
    {MemoryDeviceType::DDR_SGRAM, "DDR_SGRAM"},
    {MemoryDeviceType::DDR_SDRAM, "DDR_SDRAM"},
    {MemoryDeviceType::ROM, "ROM"},
    {MemoryDeviceType::SDRAM, "SDRAM"},
    {MemoryDeviceType::EDO, "EDO"},
    {MemoryDeviceType::FastPageMode, "FastPageMode"},
    {MemoryDeviceType::PipelinedNibble, "PipelinedNibble"},
    {MemoryDeviceType::Logical, "Logical"},
    {MemoryDeviceType::HBM, "HBM"},
    {MemoryDeviceType::HBM2, "HBM2"},
    {MemoryDeviceType::HBM2E, "HBM2E"},
    {MemoryDeviceType::HBM3, "HBM3"},
    {MemoryDeviceType::GDDR, "GDDR"},
    {MemoryDeviceType::GDDR2, "GDDR2"},
    {MemoryDeviceType::GDDR3, "GDDR3"},
    {MemoryDeviceType::GDDR4, "GDDR4"},
    {MemoryDeviceType::GDDR5, "GDDR5"},
    {MemoryDeviceType::GDDR5X, "GDDR5X"},
    {MemoryDeviceType::GDDR6, "GDDR6"},
    {MemoryDeviceType::DDR5, "DDR5"},
    {MemoryDeviceType::OEM, "OEM"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(BaseModuleType, {
    {BaseModuleType::Invalid, "Invalid"},
    {BaseModuleType::RDIMM, "RDIMM"},
    {BaseModuleType::UDIMM, "UDIMM"},
    {BaseModuleType::SO_DIMM, "SO_DIMM"},
    {BaseModuleType::LRDIMM, "LRDIMM"},
    {BaseModuleType::Mini_RDIMM, "Mini_RDIMM"},
    {BaseModuleType::Mini_UDIMM, "Mini_UDIMM"},
    {BaseModuleType::SO_RDIMM_72b, "SO_RDIMM_72b"},
    {BaseModuleType::SO_UDIMM_72b, "SO_UDIMM_72b"},
    {BaseModuleType::SO_DIMM_16b, "SO_DIMM_16b"},
    {BaseModuleType::SO_DIMM_32b, "SO_DIMM_32b"},
    {BaseModuleType::Die, "Die"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(MemoryMedia, {
    {MemoryMedia::Invalid, "Invalid"},
    {MemoryMedia::DRAM, "DRAM"},
    {MemoryMedia::NAND, "NAND"},
    {MemoryMedia::Intel3DXPoint, "Intel3DXPoint"},
    {MemoryMedia::Proprietary, "Proprietary"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(SecurityStates, {
    {SecurityStates::Invalid, "Invalid"},
    {SecurityStates::Enabled, "Enabled"},
    {SecurityStates::Disabled, "Disabled"},
    {SecurityStates::Unlocked, "Unlocked"},
    {SecurityStates::Locked, "Locked"},
    {SecurityStates::Frozen, "Frozen"},
    {SecurityStates::Passphraselimit, "Passphraselimit"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ErrorCorrection, {
    {ErrorCorrection::Invalid, "Invalid"},
    {ErrorCorrection::NoECC, "NoECC"},
    {ErrorCorrection::SingleBitECC, "SingleBitECC"},
    {ErrorCorrection::MultiBitECC, "MultiBitECC"},
    {ErrorCorrection::AddressParity, "AddressParity"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(MemoryClassification, {
    {MemoryClassification::Invalid, "Invalid"},
    {MemoryClassification::Volatile, "Volatile"},
    {MemoryClassification::ByteAccessiblePersistent, "ByteAccessiblePersistent"},
    {MemoryClassification::Block, "Block"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(OperatingMemoryModes, {
    {OperatingMemoryModes::Invalid, "Invalid"},
    {OperatingMemoryModes::Volatile, "Volatile"},
    {OperatingMemoryModes::PMEM, "PMEM"},
    {OperatingMemoryModes::Block, "Block"},
});

}
// clang-format on
