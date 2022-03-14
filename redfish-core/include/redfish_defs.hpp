#pragma once
#include <nlohmann/json.hpp>

// clang-format off
enum class AccelerationFunction_AccelerationFunctionType{
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

NLOHMANN_JSON_SERIALIZE_ENUM(AccelerationFunction_AccelerationFunctionType, {
    {AccelerationFunction_AccelerationFunctionType::Invalid, "Invalid"},
    {AccelerationFunction_AccelerationFunctionType::Encryption, "Encryption"},
    {AccelerationFunction_AccelerationFunctionType::Compression, "Compression"},
    {AccelerationFunction_AccelerationFunctionType::PacketInspection, "PacketInspection"},
    {AccelerationFunction_AccelerationFunctionType::PacketSwitch, "PacketSwitch"},
    {AccelerationFunction_AccelerationFunctionType::Scheduler, "Scheduler"},
    {AccelerationFunction_AccelerationFunctionType::AudioProcessing, "AudioProcessing"},
    {AccelerationFunction_AccelerationFunctionType::VideoProcessing, "VideoProcessing"},
    {AccelerationFunction_AccelerationFunctionType::OEM, "OEM"},
});

enum class AccountService_OAuth2Mode{
    Invalid,
    Discovery,
    Offline,
};

NLOHMANN_JSON_SERIALIZE_ENUM(AccountService_OAuth2Mode, {
    {AccountService_OAuth2Mode::Invalid, "Invalid"},
    {AccountService_OAuth2Mode::Discovery, "Discovery"},
    {AccountService_OAuth2Mode::Offline, "Offline"},
});

enum class AccountService_AccountProviderTypes{
    Invalid,
    RedfishService,
    ActiveDirectoryService,
    LDAPService,
    OEM,
    TACACSplus,
    OAuth2,
};

NLOHMANN_JSON_SERIALIZE_ENUM(AccountService_AccountProviderTypes, {
    {AccountService_AccountProviderTypes::Invalid, "Invalid"},
    {AccountService_AccountProviderTypes::RedfishService, "RedfishService"},
    {AccountService_AccountProviderTypes::ActiveDirectoryService, "ActiveDirectoryService"},
    {AccountService_AccountProviderTypes::LDAPService, "LDAPService"},
    {AccountService_AccountProviderTypes::OEM, "OEM"},
    {AccountService_AccountProviderTypes::TACACSplus, "TACACSplus"},
    {AccountService_AccountProviderTypes::OAuth2, "OAuth2"},
});

enum class AccountService_AuthenticationTypes{
    Invalid,
    Token,
    KerberosKeytab,
    UsernameAndPassword,
    OEM,
};

NLOHMANN_JSON_SERIALIZE_ENUM(AccountService_AuthenticationTypes, {
    {AccountService_AuthenticationTypes::Invalid, "Invalid"},
    {AccountService_AuthenticationTypes::Token, "Token"},
    {AccountService_AuthenticationTypes::KerberosKeytab, "KerberosKeytab"},
    {AccountService_AuthenticationTypes::UsernameAndPassword, "UsernameAndPassword"},
    {AccountService_AuthenticationTypes::OEM, "OEM"},
});

enum class AccountService_LocalAccountAuth{
    Invalid,
    Enabled,
    Disabled,
    Fallback,
    LocalFirst,
};

NLOHMANN_JSON_SERIALIZE_ENUM(AccountService_LocalAccountAuth, {
    {AccountService_LocalAccountAuth::Invalid, "Invalid"},
    {AccountService_LocalAccountAuth::Enabled, "Enabled"},
    {AccountService_LocalAccountAuth::Disabled, "Disabled"},
    {AccountService_LocalAccountAuth::Fallback, "Fallback"},
    {AccountService_LocalAccountAuth::LocalFirst, "LocalFirst"},
});

enum class AccountService_TACACSplusPasswordExchangeProtocol{
    Invalid,
    ASCII,
    PAP,
    CHAP,
    MSCHAPv1,
    MSCHAPv2,
};

NLOHMANN_JSON_SERIALIZE_ENUM(AccountService_TACACSplusPasswordExchangeProtocol, {
    {AccountService_TACACSplusPasswordExchangeProtocol::Invalid, "Invalid"},
    {AccountService_TACACSplusPasswordExchangeProtocol::ASCII, "ASCII"},
    {AccountService_TACACSplusPasswordExchangeProtocol::PAP, "PAP"},
    {AccountService_TACACSplusPasswordExchangeProtocol::CHAP, "CHAP"},
    {AccountService_TACACSplusPasswordExchangeProtocol::MSCHAPv1, "MSCHAPv1"},
    {AccountService_TACACSplusPasswordExchangeProtocol::MSCHAPv2, "MSCHAPv2"},
});

enum class ActionInfo_ParameterTypes{
    Invalid,
    Boolean,
    Number,
    NumberArray,
    String,
    StringArray,
    Object,
    ObjectArray,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ActionInfo_ParameterTypes, {
    {ActionInfo_ParameterTypes::Invalid, "Invalid"},
    {ActionInfo_ParameterTypes::Boolean, "Boolean"},
    {ActionInfo_ParameterTypes::Number, "Number"},
    {ActionInfo_ParameterTypes::NumberArray, "NumberArray"},
    {ActionInfo_ParameterTypes::String, "String"},
    {ActionInfo_ParameterTypes::StringArray, "StringArray"},
    {ActionInfo_ParameterTypes::Object, "Object"},
    {ActionInfo_ParameterTypes::ObjectArray, "ObjectArray"},
});

enum class AggregationSource_SNMPAuthenticationProtocols{
    Invalid,
    None,
    CommunityString,
    HMAC_MD5,
    HMAC_SHA96,
    HMAC128_SHA224,
    HMAC192_SHA256,
    HMAC256_SHA384,
    HMAC384_SHA512,
};

NLOHMANN_JSON_SERIALIZE_ENUM(AggregationSource_SNMPAuthenticationProtocols, {
    {AggregationSource_SNMPAuthenticationProtocols::Invalid, "Invalid"},
    {AggregationSource_SNMPAuthenticationProtocols::None, "None"},
    {AggregationSource_SNMPAuthenticationProtocols::CommunityString, "CommunityString"},
    {AggregationSource_SNMPAuthenticationProtocols::HMAC_MD5, "HMAC_MD5"},
    {AggregationSource_SNMPAuthenticationProtocols::HMAC_SHA96, "HMAC_SHA96"},
    {AggregationSource_SNMPAuthenticationProtocols::HMAC128_SHA224, "HMAC128_SHA224"},
    {AggregationSource_SNMPAuthenticationProtocols::HMAC192_SHA256, "HMAC192_SHA256"},
    {AggregationSource_SNMPAuthenticationProtocols::HMAC256_SHA384, "HMAC256_SHA384"},
    {AggregationSource_SNMPAuthenticationProtocols::HMAC384_SHA512, "HMAC384_SHA512"},
});

enum class AggregationSource_SNMPEncryptionProtocols{
    Invalid,
    None,
    CBC_DES,
    CFB128_AES128,
};

NLOHMANN_JSON_SERIALIZE_ENUM(AggregationSource_SNMPEncryptionProtocols, {
    {AggregationSource_SNMPEncryptionProtocols::Invalid, "Invalid"},
    {AggregationSource_SNMPEncryptionProtocols::None, "None"},
    {AggregationSource_SNMPEncryptionProtocols::CBC_DES, "CBC_DES"},
    {AggregationSource_SNMPEncryptionProtocols::CFB128_AES128, "CFB128_AES128"},
});

enum class AggregationSource_AggregationType{
    Invalid,
    NotificationsOnly,
    Full,
};

NLOHMANN_JSON_SERIALIZE_ENUM(AggregationSource_AggregationType, {
    {AggregationSource_AggregationType::Invalid, "Invalid"},
    {AggregationSource_AggregationType::NotificationsOnly, "NotificationsOnly"},
    {AggregationSource_AggregationType::Full, "Full"},
});

enum class AllowDeny_AllowType{
    Invalid,
    Allow,
    Deny,
};

NLOHMANN_JSON_SERIALIZE_ENUM(AllowDeny_AllowType, {
    {AllowDeny_AllowType::Invalid, "Invalid"},
    {AllowDeny_AllowType::Allow, "Allow"},
    {AllowDeny_AllowType::Deny, "Deny"},
});

enum class AllowDeny_DataDirection{
    Invalid,
    Ingress,
    Egress,
};

NLOHMANN_JSON_SERIALIZE_ENUM(AllowDeny_DataDirection, {
    {AllowDeny_DataDirection::Invalid, "Invalid"},
    {AllowDeny_DataDirection::Ingress, "Ingress"},
    {AllowDeny_DataDirection::Egress, "Egress"},
});

enum class AllowDeny_IPAddressType{
    Invalid,
    IPv4,
    IPv6,
};

NLOHMANN_JSON_SERIALIZE_ENUM(AllowDeny_IPAddressType, {
    {AllowDeny_IPAddressType::Invalid, "Invalid"},
    {AllowDeny_IPAddressType::IPv4, "IPv4"},
    {AllowDeny_IPAddressType::IPv6, "IPv6"},
});

enum class AttributeRegistry_AttributeType{
    Invalid,
    Enumeration,
    String,
    Integer,
    Boolean,
    Password,
};

NLOHMANN_JSON_SERIALIZE_ENUM(AttributeRegistry_AttributeType, {
    {AttributeRegistry_AttributeType::Invalid, "Invalid"},
    {AttributeRegistry_AttributeType::Enumeration, "Enumeration"},
    {AttributeRegistry_AttributeType::String, "String"},
    {AttributeRegistry_AttributeType::Integer, "Integer"},
    {AttributeRegistry_AttributeType::Boolean, "Boolean"},
    {AttributeRegistry_AttributeType::Password, "Password"},
});

enum class AttributeRegistry_DependencyType{
    Invalid,
    Map,
};

NLOHMANN_JSON_SERIALIZE_ENUM(AttributeRegistry_DependencyType, {
    {AttributeRegistry_DependencyType::Invalid, "Invalid"},
    {AttributeRegistry_DependencyType::Map, "Map"},
});

enum class AttributeRegistry_MapFromCondition{
    Invalid,
    EQU,
    NEQ,
    GTR,
    GEQ,
    LSS,
    LEQ,
};

NLOHMANN_JSON_SERIALIZE_ENUM(AttributeRegistry_MapFromCondition, {
    {AttributeRegistry_MapFromCondition::Invalid, "Invalid"},
    {AttributeRegistry_MapFromCondition::EQU, "EQU"},
    {AttributeRegistry_MapFromCondition::NEQ, "NEQ"},
    {AttributeRegistry_MapFromCondition::GTR, "GTR"},
    {AttributeRegistry_MapFromCondition::GEQ, "GEQ"},
    {AttributeRegistry_MapFromCondition::LSS, "LSS"},
    {AttributeRegistry_MapFromCondition::LEQ, "LEQ"},
});

enum class AttributeRegistry_MapFromProperty{
    Invalid,
    CurrentValue,
    DefaultValue,
    ReadOnly,
    WriteOnly,
    GrayOut,
    Hidden,
    LowerBound,
    UpperBound,
    MinLength,
    MaxLength,
    ScalarIncrement,
};

NLOHMANN_JSON_SERIALIZE_ENUM(AttributeRegistry_MapFromProperty, {
    {AttributeRegistry_MapFromProperty::Invalid, "Invalid"},
    {AttributeRegistry_MapFromProperty::CurrentValue, "CurrentValue"},
    {AttributeRegistry_MapFromProperty::DefaultValue, "DefaultValue"},
    {AttributeRegistry_MapFromProperty::ReadOnly, "ReadOnly"},
    {AttributeRegistry_MapFromProperty::WriteOnly, "WriteOnly"},
    {AttributeRegistry_MapFromProperty::GrayOut, "GrayOut"},
    {AttributeRegistry_MapFromProperty::Hidden, "Hidden"},
    {AttributeRegistry_MapFromProperty::LowerBound, "LowerBound"},
    {AttributeRegistry_MapFromProperty::UpperBound, "UpperBound"},
    {AttributeRegistry_MapFromProperty::MinLength, "MinLength"},
    {AttributeRegistry_MapFromProperty::MaxLength, "MaxLength"},
    {AttributeRegistry_MapFromProperty::ScalarIncrement, "ScalarIncrement"},
});

enum class AttributeRegistry_MapTerms{
    Invalid,
    AND,
    OR,
};

NLOHMANN_JSON_SERIALIZE_ENUM(AttributeRegistry_MapTerms, {
    {AttributeRegistry_MapTerms::Invalid, "Invalid"},
    {AttributeRegistry_MapTerms::AND, "AND"},
    {AttributeRegistry_MapTerms::OR, "OR"},
});

enum class AttributeRegistry_MapToProperty{
    Invalid,
    CurrentValue,
    DefaultValue,
    ReadOnly,
    WriteOnly,
    GrayOut,
    Hidden,
    Immutable,
    HelpText,
    WarningText,
    DisplayName,
    DisplayOrder,
    LowerBound,
    UpperBound,
    MinLength,
    MaxLength,
    ScalarIncrement,
    ValueExpression,
};

NLOHMANN_JSON_SERIALIZE_ENUM(AttributeRegistry_MapToProperty, {
    {AttributeRegistry_MapToProperty::Invalid, "Invalid"},
    {AttributeRegistry_MapToProperty::CurrentValue, "CurrentValue"},
    {AttributeRegistry_MapToProperty::DefaultValue, "DefaultValue"},
    {AttributeRegistry_MapToProperty::ReadOnly, "ReadOnly"},
    {AttributeRegistry_MapToProperty::WriteOnly, "WriteOnly"},
    {AttributeRegistry_MapToProperty::GrayOut, "GrayOut"},
    {AttributeRegistry_MapToProperty::Hidden, "Hidden"},
    {AttributeRegistry_MapToProperty::Immutable, "Immutable"},
    {AttributeRegistry_MapToProperty::HelpText, "HelpText"},
    {AttributeRegistry_MapToProperty::WarningText, "WarningText"},
    {AttributeRegistry_MapToProperty::DisplayName, "DisplayName"},
    {AttributeRegistry_MapToProperty::DisplayOrder, "DisplayOrder"},
    {AttributeRegistry_MapToProperty::LowerBound, "LowerBound"},
    {AttributeRegistry_MapToProperty::UpperBound, "UpperBound"},
    {AttributeRegistry_MapToProperty::MinLength, "MinLength"},
    {AttributeRegistry_MapToProperty::MaxLength, "MaxLength"},
    {AttributeRegistry_MapToProperty::ScalarIncrement, "ScalarIncrement"},
    {AttributeRegistry_MapToProperty::ValueExpression, "ValueExpression"},
});

enum class Battery_ChargeState{
    Invalid,
    Idle,
    Charging,
    Discharging,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Battery_ChargeState, {
    {Battery_ChargeState::Invalid, "Invalid"},
    {Battery_ChargeState::Idle, "Idle"},
    {Battery_ChargeState::Charging, "Charging"},
    {Battery_ChargeState::Discharging, "Discharging"},
});

enum class Cable_CableClass{
    Invalid,
    Power,
    Network,
    Storage,
    Fan,
    PCIe,
    USB,
    Video,
    Fabric,
    Serial,
    General,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Cable_CableClass, {
    {Cable_CableClass::Invalid, "Invalid"},
    {Cable_CableClass::Power, "Power"},
    {Cable_CableClass::Network, "Network"},
    {Cable_CableClass::Storage, "Storage"},
    {Cable_CableClass::Fan, "Fan"},
    {Cable_CableClass::PCIe, "PCIe"},
    {Cable_CableClass::USB, "USB"},
    {Cable_CableClass::Video, "Video"},
    {Cable_CableClass::Fabric, "Fabric"},
    {Cable_CableClass::Serial, "Serial"},
    {Cable_CableClass::General, "General"},
});

enum class Cable_CableStatus{
    Invalid,
    Normal,
    Degraded,
    Failed,
    Testing,
    Disabled,
    SetByService,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Cable_CableStatus, {
    {Cable_CableStatus::Invalid, "Invalid"},
    {Cable_CableStatus::Normal, "Normal"},
    {Cable_CableStatus::Degraded, "Degraded"},
    {Cable_CableStatus::Failed, "Failed"},
    {Cable_CableStatus::Testing, "Testing"},
    {Cable_CableStatus::Disabled, "Disabled"},
    {Cable_CableStatus::SetByService, "SetByService"},
});

enum class Cable_ConnectorType{
    Invalid,
    ACPower,
    DB9,
    DCPower,
    DisplayPort,
    HDMI,
    ICI,
    IPASS,
    PCIe,
    Proprietary,
    RJ45,
    SATA,
    SCSI,
    SlimSAS,
    SFP,
    SFPPlus,
    USBA,
    USBC,
    QSFP,
    CDFP,
    OSFP,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Cable_ConnectorType, {
    {Cable_ConnectorType::Invalid, "Invalid"},
    {Cable_ConnectorType::ACPower, "ACPower"},
    {Cable_ConnectorType::DB9, "DB9"},
    {Cable_ConnectorType::DCPower, "DCPower"},
    {Cable_ConnectorType::DisplayPort, "DisplayPort"},
    {Cable_ConnectorType::HDMI, "HDMI"},
    {Cable_ConnectorType::ICI, "ICI"},
    {Cable_ConnectorType::IPASS, "IPASS"},
    {Cable_ConnectorType::PCIe, "PCIe"},
    {Cable_ConnectorType::Proprietary, "Proprietary"},
    {Cable_ConnectorType::RJ45, "RJ45"},
    {Cable_ConnectorType::SATA, "SATA"},
    {Cable_ConnectorType::SCSI, "SCSI"},
    {Cable_ConnectorType::SlimSAS, "SlimSAS"},
    {Cable_ConnectorType::SFP, "SFP"},
    {Cable_ConnectorType::SFPPlus, "SFPPlus"},
    {Cable_ConnectorType::USBA, "USBA"},
    {Cable_ConnectorType::USBC, "USBC"},
    {Cable_ConnectorType::QSFP, "QSFP"},
    {Cable_ConnectorType::CDFP, "CDFP"},
    {Cable_ConnectorType::OSFP, "OSFP"},
});

enum class Certificate_CertificateUsageType{
    Invalid,
    User,
    Web,
    SSH,
    Device,
    Platform,
    BIOS,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Certificate_CertificateUsageType, {
    {Certificate_CertificateUsageType::Invalid, "Invalid"},
    {Certificate_CertificateUsageType::User, "User"},
    {Certificate_CertificateUsageType::Web, "Web"},
    {Certificate_CertificateUsageType::SSH, "SSH"},
    {Certificate_CertificateUsageType::Device, "Device"},
    {Certificate_CertificateUsageType::Platform, "Platform"},
    {Certificate_CertificateUsageType::BIOS, "BIOS"},
});

enum class Certificate_CertificateType{
    Invalid,
    PEM,
    PEMchain,
    PKCS7,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Certificate_CertificateType, {
    {Certificate_CertificateType::Invalid, "Invalid"},
    {Certificate_CertificateType::PEM, "PEM"},
    {Certificate_CertificateType::PEMchain, "PEMchain"},
    {Certificate_CertificateType::PKCS7, "PKCS7"},
});

enum class Certificate_KeyUsage{
    Invalid,
    DigitalSignature,
    NonRepudiation,
    KeyEncipherment,
    DataEncipherment,
    KeyAgreement,
    KeyCertSign,
    CRLSigning,
    EncipherOnly,
    DecipherOnly,
    ServerAuthentication,
    ClientAuthentication,
    CodeSigning,
    EmailProtection,
    Timestamping,
    OCSPSigning,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Certificate_KeyUsage, {
    {Certificate_KeyUsage::Invalid, "Invalid"},
    {Certificate_KeyUsage::DigitalSignature, "DigitalSignature"},
    {Certificate_KeyUsage::NonRepudiation, "NonRepudiation"},
    {Certificate_KeyUsage::KeyEncipherment, "KeyEncipherment"},
    {Certificate_KeyUsage::DataEncipherment, "DataEncipherment"},
    {Certificate_KeyUsage::KeyAgreement, "KeyAgreement"},
    {Certificate_KeyUsage::KeyCertSign, "KeyCertSign"},
    {Certificate_KeyUsage::CRLSigning, "CRLSigning"},
    {Certificate_KeyUsage::EncipherOnly, "EncipherOnly"},
    {Certificate_KeyUsage::DecipherOnly, "DecipherOnly"},
    {Certificate_KeyUsage::ServerAuthentication, "ServerAuthentication"},
    {Certificate_KeyUsage::ClientAuthentication, "ClientAuthentication"},
    {Certificate_KeyUsage::CodeSigning, "CodeSigning"},
    {Certificate_KeyUsage::EmailProtection, "EmailProtection"},
    {Certificate_KeyUsage::Timestamping, "Timestamping"},
    {Certificate_KeyUsage::OCSPSigning, "OCSPSigning"},
});

enum class Chassis_ChassisType{
    Invalid,
    Rack,
    Blade,
    Enclosure,
    StandAlone,
    RackMount,
    Card,
    Cartridge,
    Row,
    Pod,
    Expansion,
    Sidecar,
    Zone,
    Sled,
    Shelf,
    Drawer,
    Module,
    Component,
    IPBasedDrive,
    RackGroup,
    StorageEnclosure,
    Other,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Chassis_ChassisType, {
    {Chassis_ChassisType::Invalid, "Invalid"},
    {Chassis_ChassisType::Rack, "Rack"},
    {Chassis_ChassisType::Blade, "Blade"},
    {Chassis_ChassisType::Enclosure, "Enclosure"},
    {Chassis_ChassisType::StandAlone, "StandAlone"},
    {Chassis_ChassisType::RackMount, "RackMount"},
    {Chassis_ChassisType::Card, "Card"},
    {Chassis_ChassisType::Cartridge, "Cartridge"},
    {Chassis_ChassisType::Row, "Row"},
    {Chassis_ChassisType::Pod, "Pod"},
    {Chassis_ChassisType::Expansion, "Expansion"},
    {Chassis_ChassisType::Sidecar, "Sidecar"},
    {Chassis_ChassisType::Zone, "Zone"},
    {Chassis_ChassisType::Sled, "Sled"},
    {Chassis_ChassisType::Shelf, "Shelf"},
    {Chassis_ChassisType::Drawer, "Drawer"},
    {Chassis_ChassisType::Module, "Module"},
    {Chassis_ChassisType::Component, "Component"},
    {Chassis_ChassisType::IPBasedDrive, "IPBasedDrive"},
    {Chassis_ChassisType::RackGroup, "RackGroup"},
    {Chassis_ChassisType::StorageEnclosure, "StorageEnclosure"},
    {Chassis_ChassisType::Other, "Other"},
});

enum class Chassis_IndicatorLED{
    Invalid,
    Unknown,
    Lit,
    Blinking,
    Off,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Chassis_IndicatorLED, {
    {Chassis_IndicatorLED::Invalid, "Invalid"},
    {Chassis_IndicatorLED::Unknown, "Unknown"},
    {Chassis_IndicatorLED::Lit, "Lit"},
    {Chassis_IndicatorLED::Blinking, "Blinking"},
    {Chassis_IndicatorLED::Off, "Off"},
});

enum class Chassis_PowerState{
    Invalid,
    On,
    Off,
    PoweringOn,
    PoweringOff,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Chassis_PowerState, {
    {Chassis_PowerState::Invalid, "Invalid"},
    {Chassis_PowerState::On, "On"},
    {Chassis_PowerState::Off, "Off"},
    {Chassis_PowerState::PoweringOn, "PoweringOn"},
    {Chassis_PowerState::PoweringOff, "PoweringOff"},
});

enum class Chassis_IntrusionSensor{
    Invalid,
    Normal,
    HardwareIntrusion,
    TamperingDetected,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Chassis_IntrusionSensor, {
    {Chassis_IntrusionSensor::Invalid, "Invalid"},
    {Chassis_IntrusionSensor::Normal, "Normal"},
    {Chassis_IntrusionSensor::HardwareIntrusion, "HardwareIntrusion"},
    {Chassis_IntrusionSensor::TamperingDetected, "TamperingDetected"},
});

enum class Chassis_IntrusionSensorReArm{
    Invalid,
    Manual,
    Automatic,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Chassis_IntrusionSensorReArm, {
    {Chassis_IntrusionSensorReArm::Invalid, "Invalid"},
    {Chassis_IntrusionSensorReArm::Manual, "Manual"},
    {Chassis_IntrusionSensorReArm::Automatic, "Automatic"},
});

enum class Chassis_EnvironmentalClass{
    Invalid,
    A1,
    A2,
    A3,
    A4,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Chassis_EnvironmentalClass, {
    {Chassis_EnvironmentalClass::Invalid, "Invalid"},
    {Chassis_EnvironmentalClass::A1, "A1"},
    {Chassis_EnvironmentalClass::A2, "A2"},
    {Chassis_EnvironmentalClass::A3, "A3"},
    {Chassis_EnvironmentalClass::A4, "A4"},
});

enum class Circuit_CircuitType{
    Invalid,
    Mains,
    Branch,
    Subfeed,
    Feeder,
    Bus,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Circuit_CircuitType, {
    {Circuit_CircuitType::Invalid, "Invalid"},
    {Circuit_CircuitType::Mains, "Mains"},
    {Circuit_CircuitType::Branch, "Branch"},
    {Circuit_CircuitType::Subfeed, "Subfeed"},
    {Circuit_CircuitType::Feeder, "Feeder"},
    {Circuit_CircuitType::Bus, "Bus"},
});

enum class Circuit_VoltageType{
    Invalid,
    AC,
    DC,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Circuit_VoltageType, {
    {Circuit_VoltageType::Invalid, "Invalid"},
    {Circuit_VoltageType::AC, "AC"},
    {Circuit_VoltageType::DC, "DC"},
});

enum class Circuit_BreakerStates{
    Invalid,
    Normal,
    Tripped,
    Off,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Circuit_BreakerStates, {
    {Circuit_BreakerStates::Invalid, "Invalid"},
    {Circuit_BreakerStates::Normal, "Normal"},
    {Circuit_BreakerStates::Tripped, "Tripped"},
    {Circuit_BreakerStates::Off, "Off"},
});

enum class Circuit_NominalVoltageType{
    Invalid,
    AC100To240V,
    AC100To277V,
    AC120V,
    AC200To240V,
    AC200To277V,
    AC208V,
    AC230V,
    AC240V,
    AC240AndDC380V,
    AC277V,
    AC277AndDC380V,
    AC400V,
    AC480V,
    DC48V,
    DC240V,
    DC380V,
    DCNeg48V,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Circuit_NominalVoltageType, {
    {Circuit_NominalVoltageType::Invalid, "Invalid"},
    {Circuit_NominalVoltageType::AC100To240V, "AC100To240V"},
    {Circuit_NominalVoltageType::AC100To277V, "AC100To277V"},
    {Circuit_NominalVoltageType::AC120V, "AC120V"},
    {Circuit_NominalVoltageType::AC200To240V, "AC200To240V"},
    {Circuit_NominalVoltageType::AC200To277V, "AC200To277V"},
    {Circuit_NominalVoltageType::AC208V, "AC208V"},
    {Circuit_NominalVoltageType::AC230V, "AC230V"},
    {Circuit_NominalVoltageType::AC240V, "AC240V"},
    {Circuit_NominalVoltageType::AC240AndDC380V, "AC240AndDC380V"},
    {Circuit_NominalVoltageType::AC277V, "AC277V"},
    {Circuit_NominalVoltageType::AC277AndDC380V, "AC277AndDC380V"},
    {Circuit_NominalVoltageType::AC400V, "AC400V"},
    {Circuit_NominalVoltageType::AC480V, "AC480V"},
    {Circuit_NominalVoltageType::DC48V, "DC48V"},
    {Circuit_NominalVoltageType::DC240V, "DC240V"},
    {Circuit_NominalVoltageType::DC380V, "DC380V"},
    {Circuit_NominalVoltageType::DCNeg48V, "DCNeg48V"},
});

enum class Circuit_PhaseWiringType{
    Invalid,
    OnePhase3Wire,
    TwoPhase3Wire,
    OneOrTwoPhase3Wire,
    TwoPhase4Wire,
    ThreePhase4Wire,
    ThreePhase5Wire,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Circuit_PhaseWiringType, {
    {Circuit_PhaseWiringType::Invalid, "Invalid"},
    {Circuit_PhaseWiringType::OnePhase3Wire, "OnePhase3Wire"},
    {Circuit_PhaseWiringType::TwoPhase3Wire, "TwoPhase3Wire"},
    {Circuit_PhaseWiringType::OneOrTwoPhase3Wire, "OneOrTwoPhase3Wire"},
    {Circuit_PhaseWiringType::TwoPhase4Wire, "TwoPhase4Wire"},
    {Circuit_PhaseWiringType::ThreePhase4Wire, "ThreePhase4Wire"},
    {Circuit_PhaseWiringType::ThreePhase5Wire, "ThreePhase5Wire"},
});

enum class Circuit_PlugType{
    Invalid,
    NEMA_5_15P,
    NEMA_L5_15P,
    NEMA_5_20P,
    NEMA_L5_20P,
    NEMA_L5_30P,
    NEMA_6_15P,
    NEMA_L6_15P,
    NEMA_6_20P,
    NEMA_L6_20P,
    NEMA_L6_30P,
    NEMA_L14_20P,
    NEMA_L14_30P,
    NEMA_L15_20P,
    NEMA_L15_30P,
    NEMA_L21_20P,
    NEMA_L21_30P,
    NEMA_L22_20P,
    NEMA_L22_30P,
    California_CS8265,
    California_CS8365,
    IEC_60320_C14,
    IEC_60320_C20,
    IEC_60309_316P6,
    IEC_60309_332P6,
    IEC_60309_363P6,
    IEC_60309_516P6,
    IEC_60309_532P6,
    IEC_60309_563P6,
    IEC_60309_460P9,
    IEC_60309_560P9,
    Field_208V_3P4W_60A,
    Field_400V_3P5W_32A,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Circuit_PlugType, {
    {Circuit_PlugType::Invalid, "Invalid"},
    {Circuit_PlugType::NEMA_5_15P, "NEMA_5_15P"},
    {Circuit_PlugType::NEMA_L5_15P, "NEMA_L5_15P"},
    {Circuit_PlugType::NEMA_5_20P, "NEMA_5_20P"},
    {Circuit_PlugType::NEMA_L5_20P, "NEMA_L5_20P"},
    {Circuit_PlugType::NEMA_L5_30P, "NEMA_L5_30P"},
    {Circuit_PlugType::NEMA_6_15P, "NEMA_6_15P"},
    {Circuit_PlugType::NEMA_L6_15P, "NEMA_L6_15P"},
    {Circuit_PlugType::NEMA_6_20P, "NEMA_6_20P"},
    {Circuit_PlugType::NEMA_L6_20P, "NEMA_L6_20P"},
    {Circuit_PlugType::NEMA_L6_30P, "NEMA_L6_30P"},
    {Circuit_PlugType::NEMA_L14_20P, "NEMA_L14_20P"},
    {Circuit_PlugType::NEMA_L14_30P, "NEMA_L14_30P"},
    {Circuit_PlugType::NEMA_L15_20P, "NEMA_L15_20P"},
    {Circuit_PlugType::NEMA_L15_30P, "NEMA_L15_30P"},
    {Circuit_PlugType::NEMA_L21_20P, "NEMA_L21_20P"},
    {Circuit_PlugType::NEMA_L21_30P, "NEMA_L21_30P"},
    {Circuit_PlugType::NEMA_L22_20P, "NEMA_L22_20P"},
    {Circuit_PlugType::NEMA_L22_30P, "NEMA_L22_30P"},
    {Circuit_PlugType::California_CS8265, "California_CS8265"},
    {Circuit_PlugType::California_CS8365, "California_CS8365"},
    {Circuit_PlugType::IEC_60320_C14, "IEC_60320_C14"},
    {Circuit_PlugType::IEC_60320_C20, "IEC_60320_C20"},
    {Circuit_PlugType::IEC_60309_316P6, "IEC_60309_316P6"},
    {Circuit_PlugType::IEC_60309_332P6, "IEC_60309_332P6"},
    {Circuit_PlugType::IEC_60309_363P6, "IEC_60309_363P6"},
    {Circuit_PlugType::IEC_60309_516P6, "IEC_60309_516P6"},
    {Circuit_PlugType::IEC_60309_532P6, "IEC_60309_532P6"},
    {Circuit_PlugType::IEC_60309_563P6, "IEC_60309_563P6"},
    {Circuit_PlugType::IEC_60309_460P9, "IEC_60309_460P9"},
    {Circuit_PlugType::IEC_60309_560P9, "IEC_60309_560P9"},
    {Circuit_PlugType::Field_208V_3P4W_60A, "Field_208V_3P4W_60A"},
    {Circuit_PlugType::Field_400V_3P5W_32A, "Field_400V_3P5W_32A"},
});

enum class Circuit_PowerRestorePolicyTypes{
    Invalid,
    AlwaysOn,
    AlwaysOff,
    LastState,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Circuit_PowerRestorePolicyTypes, {
    {Circuit_PowerRestorePolicyTypes::Invalid, "Invalid"},
    {Circuit_PowerRestorePolicyTypes::AlwaysOn, "AlwaysOn"},
    {Circuit_PowerRestorePolicyTypes::AlwaysOff, "AlwaysOff"},
    {Circuit_PowerRestorePolicyTypes::LastState, "LastState"},
});

enum class Circuit_PowerState{
    Invalid,
    On,
    Off,
    PowerCycle,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Circuit_PowerState, {
    {Circuit_PowerState::Invalid, "Invalid"},
    {Circuit_PowerState::On, "On"},
    {Circuit_PowerState::Off, "Off"},
    {Circuit_PowerState::PowerCycle, "PowerCycle"},
});

enum class ComponentIntegrity_ComponentIntegrityType{
    Invalid,
    SPDM,
    TPM,
    OEM,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ComponentIntegrity_ComponentIntegrityType, {
    {ComponentIntegrity_ComponentIntegrityType::Invalid, "Invalid"},
    {ComponentIntegrity_ComponentIntegrityType::SPDM, "SPDM"},
    {ComponentIntegrity_ComponentIntegrityType::TPM, "TPM"},
    {ComponentIntegrity_ComponentIntegrityType::OEM, "OEM"},
});

enum class ComponentIntegrity_DMTFmeasurementTypes{
    Invalid,
    ImmutableROM,
    MutableFirmware,
    HardwareConfiguration,
    FirmwareConfiguration,
    MutableFirmwareVersion,
    MutableFirmwareSecurityVersionNumber,
    MeasurementManifest,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ComponentIntegrity_DMTFmeasurementTypes, {
    {ComponentIntegrity_DMTFmeasurementTypes::Invalid, "Invalid"},
    {ComponentIntegrity_DMTFmeasurementTypes::ImmutableROM, "ImmutableROM"},
    {ComponentIntegrity_DMTFmeasurementTypes::MutableFirmware, "MutableFirmware"},
    {ComponentIntegrity_DMTFmeasurementTypes::HardwareConfiguration, "HardwareConfiguration"},
    {ComponentIntegrity_DMTFmeasurementTypes::FirmwareConfiguration, "FirmwareConfiguration"},
    {ComponentIntegrity_DMTFmeasurementTypes::MutableFirmwareVersion, "MutableFirmwareVersion"},
    {ComponentIntegrity_DMTFmeasurementTypes::MutableFirmwareSecurityVersionNumber, "MutableFirmwareSecurityVersionNumber"},
    {ComponentIntegrity_DMTFmeasurementTypes::MeasurementManifest, "MeasurementManifest"},
});

enum class ComponentIntegrity_MeasurementSpecification{
    Invalid,
    DMTF,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ComponentIntegrity_MeasurementSpecification, {
    {ComponentIntegrity_MeasurementSpecification::Invalid, "Invalid"},
    {ComponentIntegrity_MeasurementSpecification::DMTF, "DMTF"},
});

enum class ComponentIntegrity_SPDMmeasurementSummaryType{
    Invalid,
    TCB,
    All,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ComponentIntegrity_SPDMmeasurementSummaryType, {
    {ComponentIntegrity_SPDMmeasurementSummaryType::Invalid, "Invalid"},
    {ComponentIntegrity_SPDMmeasurementSummaryType::TCB, "TCB"},
    {ComponentIntegrity_SPDMmeasurementSummaryType::All, "All"},
});

enum class ComponentIntegrity_SecureSessionType{
    Invalid,
    Plain,
    EncryptedAuthenticated,
    AuthenticatedOnly,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ComponentIntegrity_SecureSessionType, {
    {ComponentIntegrity_SecureSessionType::Invalid, "Invalid"},
    {ComponentIntegrity_SecureSessionType::Plain, "Plain"},
    {ComponentIntegrity_SecureSessionType::EncryptedAuthenticated, "EncryptedAuthenticated"},
    {ComponentIntegrity_SecureSessionType::AuthenticatedOnly, "AuthenticatedOnly"},
});

enum class ComponentIntegrity_VerificationStatus{
    Invalid,
    Success,
    Failed,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ComponentIntegrity_VerificationStatus, {
    {ComponentIntegrity_VerificationStatus::Invalid, "Invalid"},
    {ComponentIntegrity_VerificationStatus::Success, "Success"},
    {ComponentIntegrity_VerificationStatus::Failed, "Failed"},
});

enum class CompositionService_ComposeRequestFormat{
    Invalid,
    Manifest,
};

NLOHMANN_JSON_SERIALIZE_ENUM(CompositionService_ComposeRequestFormat, {
    {CompositionService_ComposeRequestFormat::Invalid, "Invalid"},
    {CompositionService_ComposeRequestFormat::Manifest, "Manifest"},
});

enum class CompositionService_ComposeRequestType{
    Invalid,
    Preview,
    PreviewReserve,
    Apply,
};

NLOHMANN_JSON_SERIALIZE_ENUM(CompositionService_ComposeRequestType, {
    {CompositionService_ComposeRequestType::Invalid, "Invalid"},
    {CompositionService_ComposeRequestType::Preview, "Preview"},
    {CompositionService_ComposeRequestType::PreviewReserve, "PreviewReserve"},
    {CompositionService_ComposeRequestType::Apply, "Apply"},
});

enum class ComputerSystem_BootSourceOverrideEnabled{
    Invalid,
    Disabled,
    Once,
    Continuous,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ComputerSystem_BootSourceOverrideEnabled, {
    {ComputerSystem_BootSourceOverrideEnabled::Invalid, "Invalid"},
    {ComputerSystem_BootSourceOverrideEnabled::Disabled, "Disabled"},
    {ComputerSystem_BootSourceOverrideEnabled::Once, "Once"},
    {ComputerSystem_BootSourceOverrideEnabled::Continuous, "Continuous"},
});

enum class ComputerSystem_IndicatorLED{
    Invalid,
    Unknown,
    Lit,
    Blinking,
    Off,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ComputerSystem_IndicatorLED, {
    {ComputerSystem_IndicatorLED::Invalid, "Invalid"},
    {ComputerSystem_IndicatorLED::Unknown, "Unknown"},
    {ComputerSystem_IndicatorLED::Lit, "Lit"},
    {ComputerSystem_IndicatorLED::Blinking, "Blinking"},
    {ComputerSystem_IndicatorLED::Off, "Off"},
});

enum class ComputerSystem_PowerState{
    Invalid,
    On,
    Off,
    PoweringOn,
    PoweringOff,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ComputerSystem_PowerState, {
    {ComputerSystem_PowerState::Invalid, "Invalid"},
    {ComputerSystem_PowerState::On, "On"},
    {ComputerSystem_PowerState::Off, "Off"},
    {ComputerSystem_PowerState::PoweringOn, "PoweringOn"},
    {ComputerSystem_PowerState::PoweringOff, "PoweringOff"},
});

enum class ComputerSystem_SystemType{
    Invalid,
    Physical,
    Virtual,
    OS,
    PhysicallyPartitioned,
    VirtuallyPartitioned,
    Composed,
    DPU,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ComputerSystem_SystemType, {
    {ComputerSystem_SystemType::Invalid, "Invalid"},
    {ComputerSystem_SystemType::Physical, "Physical"},
    {ComputerSystem_SystemType::Virtual, "Virtual"},
    {ComputerSystem_SystemType::OS, "OS"},
    {ComputerSystem_SystemType::PhysicallyPartitioned, "PhysicallyPartitioned"},
    {ComputerSystem_SystemType::VirtuallyPartitioned, "VirtuallyPartitioned"},
    {ComputerSystem_SystemType::Composed, "Composed"},
    {ComputerSystem_SystemType::DPU, "DPU"},
});

enum class ComputerSystem_AutomaticRetryConfig{
    Invalid,
    Disabled,
    RetryAttempts,
    RetryAlways,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ComputerSystem_AutomaticRetryConfig, {
    {ComputerSystem_AutomaticRetryConfig::Invalid, "Invalid"},
    {ComputerSystem_AutomaticRetryConfig::Disabled, "Disabled"},
    {ComputerSystem_AutomaticRetryConfig::RetryAttempts, "RetryAttempts"},
    {ComputerSystem_AutomaticRetryConfig::RetryAlways, "RetryAlways"},
});

enum class ComputerSystem_BootProgressTypes{
    Invalid,
    None,
    PrimaryProcessorInitializationStarted,
    BusInitializationStarted,
    MemoryInitializationStarted,
    SecondaryProcessorInitializationStarted,
    PCIResourceConfigStarted,
    SystemHardwareInitializationComplete,
    SetupEntered,
    OSBootStarted,
    OSRunning,
    OEM,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ComputerSystem_BootProgressTypes, {
    {ComputerSystem_BootProgressTypes::Invalid, "Invalid"},
    {ComputerSystem_BootProgressTypes::None, "None"},
    {ComputerSystem_BootProgressTypes::PrimaryProcessorInitializationStarted, "PrimaryProcessorInitializationStarted"},
    {ComputerSystem_BootProgressTypes::BusInitializationStarted, "BusInitializationStarted"},
    {ComputerSystem_BootProgressTypes::MemoryInitializationStarted, "MemoryInitializationStarted"},
    {ComputerSystem_BootProgressTypes::SecondaryProcessorInitializationStarted, "SecondaryProcessorInitializationStarted"},
    {ComputerSystem_BootProgressTypes::PCIResourceConfigStarted, "PCIResourceConfigStarted"},
    {ComputerSystem_BootProgressTypes::SystemHardwareInitializationComplete, "SystemHardwareInitializationComplete"},
    {ComputerSystem_BootProgressTypes::SetupEntered, "SetupEntered"},
    {ComputerSystem_BootProgressTypes::OSBootStarted, "OSBootStarted"},
    {ComputerSystem_BootProgressTypes::OSRunning, "OSRunning"},
    {ComputerSystem_BootProgressTypes::OEM, "OEM"},
});

enum class ComputerSystem_GraphicalConnectTypesSupported{
    Invalid,
    KVMIP,
    OEM,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ComputerSystem_GraphicalConnectTypesSupported, {
    {ComputerSystem_GraphicalConnectTypesSupported::Invalid, "Invalid"},
    {ComputerSystem_GraphicalConnectTypesSupported::KVMIP, "KVMIP"},
    {ComputerSystem_GraphicalConnectTypesSupported::OEM, "OEM"},
});

enum class ComputerSystem_TrustedModuleRequiredToBoot{
    Invalid,
    Disabled,
    Required,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ComputerSystem_TrustedModuleRequiredToBoot, {
    {ComputerSystem_TrustedModuleRequiredToBoot::Invalid, "Invalid"},
    {ComputerSystem_TrustedModuleRequiredToBoot::Disabled, "Disabled"},
    {ComputerSystem_TrustedModuleRequiredToBoot::Required, "Required"},
});

enum class ComputerSystem_PowerMode{
    Invalid,
    MaximumPerformance,
    BalancedPerformance,
    PowerSaving,
    Static,
    OSControlled,
    OEM,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ComputerSystem_PowerMode, {
    {ComputerSystem_PowerMode::Invalid, "Invalid"},
    {ComputerSystem_PowerMode::MaximumPerformance, "MaximumPerformance"},
    {ComputerSystem_PowerMode::BalancedPerformance, "BalancedPerformance"},
    {ComputerSystem_PowerMode::PowerSaving, "PowerSaving"},
    {ComputerSystem_PowerMode::Static, "Static"},
    {ComputerSystem_PowerMode::OSControlled, "OSControlled"},
    {ComputerSystem_PowerMode::OEM, "OEM"},
});

enum class ComputerSystem_StopBootOnFault{
    Invalid,
    Never,
    AnyFault,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ComputerSystem_StopBootOnFault, {
    {ComputerSystem_StopBootOnFault::Invalid, "Invalid"},
    {ComputerSystem_StopBootOnFault::Never, "Never"},
    {ComputerSystem_StopBootOnFault::AnyFault, "AnyFault"},
});

enum class ComputerSystem_BootSourceOverrideMode{
    Invalid,
    Legacy,
    UEFI,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ComputerSystem_BootSourceOverrideMode, {
    {ComputerSystem_BootSourceOverrideMode::Invalid, "Invalid"},
    {ComputerSystem_BootSourceOverrideMode::Legacy, "Legacy"},
    {ComputerSystem_BootSourceOverrideMode::UEFI, "UEFI"},
});

enum class ComputerSystem_InterfaceType{
    Invalid,
    TPM1_2,
    TPM2_0,
    TCM1_0,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ComputerSystem_InterfaceType, {
    {ComputerSystem_InterfaceType::Invalid, "Invalid"},
    {ComputerSystem_InterfaceType::TPM1_2, "TPM1_2"},
    {ComputerSystem_InterfaceType::TPM2_0, "TPM2_0"},
    {ComputerSystem_InterfaceType::TCM1_0, "TCM1_0"},
});

enum class ComputerSystem_MemoryMirroring{
    Invalid,
    System,
    DIMM,
    Hybrid,
    None,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ComputerSystem_MemoryMirroring, {
    {ComputerSystem_MemoryMirroring::Invalid, "Invalid"},
    {ComputerSystem_MemoryMirroring::System, "System"},
    {ComputerSystem_MemoryMirroring::DIMM, "DIMM"},
    {ComputerSystem_MemoryMirroring::Hybrid, "Hybrid"},
    {ComputerSystem_MemoryMirroring::None, "None"},
});

enum class ComputerSystem_HostingRole{
    Invalid,
    ApplicationServer,
    StorageServer,
    Switch,
    Appliance,
    BareMetalServer,
    VirtualMachineServer,
    ContainerServer,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ComputerSystem_HostingRole, {
    {ComputerSystem_HostingRole::Invalid, "Invalid"},
    {ComputerSystem_HostingRole::ApplicationServer, "ApplicationServer"},
    {ComputerSystem_HostingRole::StorageServer, "StorageServer"},
    {ComputerSystem_HostingRole::Switch, "Switch"},
    {ComputerSystem_HostingRole::Appliance, "Appliance"},
    {ComputerSystem_HostingRole::BareMetalServer, "BareMetalServer"},
    {ComputerSystem_HostingRole::VirtualMachineServer, "VirtualMachineServer"},
    {ComputerSystem_HostingRole::ContainerServer, "ContainerServer"},
});

enum class ComputerSystem_InterfaceTypeSelection{
    Invalid,
    None,
    FirmwareUpdate,
    BiosSetting,
    OemMethod,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ComputerSystem_InterfaceTypeSelection, {
    {ComputerSystem_InterfaceTypeSelection::Invalid, "Invalid"},
    {ComputerSystem_InterfaceTypeSelection::None, "None"},
    {ComputerSystem_InterfaceTypeSelection::FirmwareUpdate, "FirmwareUpdate"},
    {ComputerSystem_InterfaceTypeSelection::BiosSetting, "BiosSetting"},
    {ComputerSystem_InterfaceTypeSelection::OemMethod, "OemMethod"},
});

enum class ComputerSystem_WatchdogTimeoutActions{
    Invalid,
    None,
    ResetSystem,
    PowerCycle,
    PowerDown,
    OEM,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ComputerSystem_WatchdogTimeoutActions, {
    {ComputerSystem_WatchdogTimeoutActions::Invalid, "Invalid"},
    {ComputerSystem_WatchdogTimeoutActions::None, "None"},
    {ComputerSystem_WatchdogTimeoutActions::ResetSystem, "ResetSystem"},
    {ComputerSystem_WatchdogTimeoutActions::PowerCycle, "PowerCycle"},
    {ComputerSystem_WatchdogTimeoutActions::PowerDown, "PowerDown"},
    {ComputerSystem_WatchdogTimeoutActions::OEM, "OEM"},
});

enum class ComputerSystem_WatchdogWarningActions{
    Invalid,
    None,
    DiagnosticInterrupt,
    SMI,
    MessagingInterrupt,
    SCI,
    OEM,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ComputerSystem_WatchdogWarningActions, {
    {ComputerSystem_WatchdogWarningActions::Invalid, "Invalid"},
    {ComputerSystem_WatchdogWarningActions::None, "None"},
    {ComputerSystem_WatchdogWarningActions::DiagnosticInterrupt, "DiagnosticInterrupt"},
    {ComputerSystem_WatchdogWarningActions::SMI, "SMI"},
    {ComputerSystem_WatchdogWarningActions::MessagingInterrupt, "MessagingInterrupt"},
    {ComputerSystem_WatchdogWarningActions::SCI, "SCI"},
    {ComputerSystem_WatchdogWarningActions::OEM, "OEM"},
});

enum class ComputerSystem_BootOrderTypes{
    Invalid,
    BootOrder,
    AliasBootOrder,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ComputerSystem_BootOrderTypes, {
    {ComputerSystem_BootOrderTypes::Invalid, "Invalid"},
    {ComputerSystem_BootOrderTypes::BootOrder, "BootOrder"},
    {ComputerSystem_BootOrderTypes::AliasBootOrder, "AliasBootOrder"},
});

enum class ComputerSystem_PowerRestorePolicyTypes{
    Invalid,
    AlwaysOn,
    AlwaysOff,
    LastState,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ComputerSystem_PowerRestorePolicyTypes, {
    {ComputerSystem_PowerRestorePolicyTypes::Invalid, "Invalid"},
    {ComputerSystem_PowerRestorePolicyTypes::AlwaysOn, "AlwaysOn"},
    {ComputerSystem_PowerRestorePolicyTypes::AlwaysOff, "AlwaysOff"},
    {ComputerSystem_PowerRestorePolicyTypes::LastState, "LastState"},
});

enum class ComputerSystem_BootSource{
    Invalid,
    None,
    Pxe,
    Floppy,
    Cd,
    Usb,
    Hdd,
    BiosSetup,
    Utilities,
    Diags,
    UefiShell,
    UefiTarget,
    SDCard,
    UefiHttp,
    RemoteDrive,
    UefiBootNext,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ComputerSystem_BootSource, {
    {ComputerSystem_BootSource::Invalid, "Invalid"},
    {ComputerSystem_BootSource::None, "None"},
    {ComputerSystem_BootSource::Pxe, "Pxe"},
    {ComputerSystem_BootSource::Floppy, "Floppy"},
    {ComputerSystem_BootSource::Cd, "Cd"},
    {ComputerSystem_BootSource::Usb, "Usb"},
    {ComputerSystem_BootSource::Hdd, "Hdd"},
    {ComputerSystem_BootSource::BiosSetup, "BiosSetup"},
    {ComputerSystem_BootSource::Utilities, "Utilities"},
    {ComputerSystem_BootSource::Diags, "Diags"},
    {ComputerSystem_BootSource::UefiShell, "UefiShell"},
    {ComputerSystem_BootSource::UefiTarget, "UefiTarget"},
    {ComputerSystem_BootSource::SDCard, "SDCard"},
    {ComputerSystem_BootSource::UefiHttp, "UefiHttp"},
    {ComputerSystem_BootSource::RemoteDrive, "RemoteDrive"},
    {ComputerSystem_BootSource::UefiBootNext, "UefiBootNext"},
});

enum class Connection_AccessCapability{
    Invalid,
    Read,
    Write,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Connection_AccessCapability, {
    {Connection_AccessCapability::Invalid, "Invalid"},
    {Connection_AccessCapability::Read, "Read"},
    {Connection_AccessCapability::Write, "Write"},
});

enum class Connection_AccessState{
    Invalid,
    Optimized,
    NonOptimized,
    Standby,
    Unavailable,
    Transitioning,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Connection_AccessState, {
    {Connection_AccessState::Invalid, "Invalid"},
    {Connection_AccessState::Optimized, "Optimized"},
    {Connection_AccessState::NonOptimized, "NonOptimized"},
    {Connection_AccessState::Standby, "Standby"},
    {Connection_AccessState::Unavailable, "Unavailable"},
    {Connection_AccessState::Transitioning, "Transitioning"},
});

enum class Connection_ConnectionType{
    Invalid,
    Storage,
    Memory,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Connection_ConnectionType, {
    {Connection_ConnectionType::Invalid, "Invalid"},
    {Connection_ConnectionType::Storage, "Storage"},
    {Connection_ConnectionType::Memory, "Memory"},
});

enum class ConnectionMethod_ConnectionMethodType{
    Invalid,
    Redfish,
    SNMP,
    IPMI15,
    IPMI20,
    NETCONF,
    OEM,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ConnectionMethod_ConnectionMethodType, {
    {ConnectionMethod_ConnectionMethodType::Invalid, "Invalid"},
    {ConnectionMethod_ConnectionMethodType::Redfish, "Redfish"},
    {ConnectionMethod_ConnectionMethodType::SNMP, "SNMP"},
    {ConnectionMethod_ConnectionMethodType::IPMI15, "IPMI15"},
    {ConnectionMethod_ConnectionMethodType::IPMI20, "IPMI20"},
    {ConnectionMethod_ConnectionMethodType::NETCONF, "NETCONF"},
    {ConnectionMethod_ConnectionMethodType::OEM, "OEM"},
});

enum class ConsistencyGroup_ApplicationConsistencyMethod{
    Invalid,
    HotStandby,
    VASA,
    VDI,
    VSS,
    Other,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ConsistencyGroup_ApplicationConsistencyMethod, {
    {ConsistencyGroup_ApplicationConsistencyMethod::Invalid, "Invalid"},
    {ConsistencyGroup_ApplicationConsistencyMethod::HotStandby, "HotStandby"},
    {ConsistencyGroup_ApplicationConsistencyMethod::VASA, "VASA"},
    {ConsistencyGroup_ApplicationConsistencyMethod::VDI, "VDI"},
    {ConsistencyGroup_ApplicationConsistencyMethod::VSS, "VSS"},
    {ConsistencyGroup_ApplicationConsistencyMethod::Other, "Other"},
});

enum class ConsistencyGroup_ConsistencyType{
    Invalid,
    CrashConsistent,
    ApplicationConsistent,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ConsistencyGroup_ConsistencyType, {
    {ConsistencyGroup_ConsistencyType::Invalid, "Invalid"},
    {ConsistencyGroup_ConsistencyType::CrashConsistent, "CrashConsistent"},
    {ConsistencyGroup_ConsistencyType::ApplicationConsistent, "ApplicationConsistent"},
});

enum class Control_ControlMode{
    Invalid,
    Automatic,
    Override,
    Manual,
    Disabled,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Control_ControlMode, {
    {Control_ControlMode::Invalid, "Invalid"},
    {Control_ControlMode::Automatic, "Automatic"},
    {Control_ControlMode::Override, "Override"},
    {Control_ControlMode::Manual, "Manual"},
    {Control_ControlMode::Disabled, "Disabled"},
});

enum class Control_ControlType{
    Invalid,
    Temperature,
    Power,
    Frequency,
    FrequencyMHz,
    Pressure,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Control_ControlType, {
    {Control_ControlType::Invalid, "Invalid"},
    {Control_ControlType::Temperature, "Temperature"},
    {Control_ControlType::Power, "Power"},
    {Control_ControlType::Frequency, "Frequency"},
    {Control_ControlType::FrequencyMHz, "FrequencyMHz"},
    {Control_ControlType::Pressure, "Pressure"},
});

enum class Control_ImplementationType{
    Invalid,
    Programmable,
    Direct,
    Monitored,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Control_ImplementationType, {
    {Control_ImplementationType::Invalid, "Invalid"},
    {Control_ImplementationType::Programmable, "Programmable"},
    {Control_ImplementationType::Direct, "Direct"},
    {Control_ImplementationType::Monitored, "Monitored"},
});

enum class Control_SetPointType{
    Invalid,
    Single,
    Range,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Control_SetPointType, {
    {Control_SetPointType::Invalid, "Invalid"},
    {Control_SetPointType::Single, "Single"},
    {Control_SetPointType::Range, "Range"},
});

enum class DataProtectionLoSCapabilities_FailureDomainScope{
    Invalid,
    Server,
    Rack,
    RackGroup,
    Row,
    Datacenter,
    Region,
};

NLOHMANN_JSON_SERIALIZE_ENUM(DataProtectionLoSCapabilities_FailureDomainScope, {
    {DataProtectionLoSCapabilities_FailureDomainScope::Invalid, "Invalid"},
    {DataProtectionLoSCapabilities_FailureDomainScope::Server, "Server"},
    {DataProtectionLoSCapabilities_FailureDomainScope::Rack, "Rack"},
    {DataProtectionLoSCapabilities_FailureDomainScope::RackGroup, "RackGroup"},
    {DataProtectionLoSCapabilities_FailureDomainScope::Row, "Row"},
    {DataProtectionLoSCapabilities_FailureDomainScope::Datacenter, "Datacenter"},
    {DataProtectionLoSCapabilities_FailureDomainScope::Region, "Region"},
});

enum class DataProtectionLoSCapabilities_RecoveryAccessScope{
    Invalid,
    OnlineActive,
    OnlinePassive,
    Nearline,
    Offline,
};

NLOHMANN_JSON_SERIALIZE_ENUM(DataProtectionLoSCapabilities_RecoveryAccessScope, {
    {DataProtectionLoSCapabilities_RecoveryAccessScope::Invalid, "Invalid"},
    {DataProtectionLoSCapabilities_RecoveryAccessScope::OnlineActive, "OnlineActive"},
    {DataProtectionLoSCapabilities_RecoveryAccessScope::OnlinePassive, "OnlinePassive"},
    {DataProtectionLoSCapabilities_RecoveryAccessScope::Nearline, "Nearline"},
    {DataProtectionLoSCapabilities_RecoveryAccessScope::Offline, "Offline"},
});

enum class DataSecurityLoSCapabilities_AntiVirusScanTrigger{
    Invalid,
    None,
    OnFirstRead,
    OnPatternUpdate,
    OnUpdate,
    OnRename,
};

NLOHMANN_JSON_SERIALIZE_ENUM(DataSecurityLoSCapabilities_AntiVirusScanTrigger, {
    {DataSecurityLoSCapabilities_AntiVirusScanTrigger::Invalid, "Invalid"},
    {DataSecurityLoSCapabilities_AntiVirusScanTrigger::None, "None"},
    {DataSecurityLoSCapabilities_AntiVirusScanTrigger::OnFirstRead, "OnFirstRead"},
    {DataSecurityLoSCapabilities_AntiVirusScanTrigger::OnPatternUpdate, "OnPatternUpdate"},
    {DataSecurityLoSCapabilities_AntiVirusScanTrigger::OnUpdate, "OnUpdate"},
    {DataSecurityLoSCapabilities_AntiVirusScanTrigger::OnRename, "OnRename"},
});

enum class DataSecurityLoSCapabilities_AuthenticationType{
    Invalid,
    None,
    PKI,
    Ticket,
    Password,
};

NLOHMANN_JSON_SERIALIZE_ENUM(DataSecurityLoSCapabilities_AuthenticationType, {
    {DataSecurityLoSCapabilities_AuthenticationType::Invalid, "Invalid"},
    {DataSecurityLoSCapabilities_AuthenticationType::None, "None"},
    {DataSecurityLoSCapabilities_AuthenticationType::PKI, "PKI"},
    {DataSecurityLoSCapabilities_AuthenticationType::Ticket, "Ticket"},
    {DataSecurityLoSCapabilities_AuthenticationType::Password, "Password"},
});

enum class DataSecurityLoSCapabilities_DataSanitizationPolicy{
    Invalid,
    None,
    Clear,
    CryptographicErase,
};

NLOHMANN_JSON_SERIALIZE_ENUM(DataSecurityLoSCapabilities_DataSanitizationPolicy, {
    {DataSecurityLoSCapabilities_DataSanitizationPolicy::Invalid, "Invalid"},
    {DataSecurityLoSCapabilities_DataSanitizationPolicy::None, "None"},
    {DataSecurityLoSCapabilities_DataSanitizationPolicy::Clear, "Clear"},
    {DataSecurityLoSCapabilities_DataSanitizationPolicy::CryptographicErase, "CryptographicErase"},
});

enum class DataSecurityLoSCapabilities_KeySize{
    Invalid,
    Bits_0,
    Bits_112,
    Bits_128,
    Bits_192,
    Bits_256,
};

NLOHMANN_JSON_SERIALIZE_ENUM(DataSecurityLoSCapabilities_KeySize, {
    {DataSecurityLoSCapabilities_KeySize::Invalid, "Invalid"},
    {DataSecurityLoSCapabilities_KeySize::Bits_0, "Bits_0"},
    {DataSecurityLoSCapabilities_KeySize::Bits_112, "Bits_112"},
    {DataSecurityLoSCapabilities_KeySize::Bits_128, "Bits_128"},
    {DataSecurityLoSCapabilities_KeySize::Bits_192, "Bits_192"},
    {DataSecurityLoSCapabilities_KeySize::Bits_256, "Bits_256"},
});

enum class DataSecurityLoSCapabilities_SecureChannelProtocol{
    Invalid,
    None,
    TLS,
    IPsec,
    RPCSEC_GSS,
};

NLOHMANN_JSON_SERIALIZE_ENUM(DataSecurityLoSCapabilities_SecureChannelProtocol, {
    {DataSecurityLoSCapabilities_SecureChannelProtocol::Invalid, "Invalid"},
    {DataSecurityLoSCapabilities_SecureChannelProtocol::None, "None"},
    {DataSecurityLoSCapabilities_SecureChannelProtocol::TLS, "TLS"},
    {DataSecurityLoSCapabilities_SecureChannelProtocol::IPsec, "IPsec"},
    {DataSecurityLoSCapabilities_SecureChannelProtocol::RPCSEC_GSS, "RPCSEC_GSS"},
});

enum class DataStorageLoSCapabilities_ProvisioningPolicy{
    Invalid,
    Fixed,
    Thin,
};

NLOHMANN_JSON_SERIALIZE_ENUM(DataStorageLoSCapabilities_ProvisioningPolicy, {
    {DataStorageLoSCapabilities_ProvisioningPolicy::Invalid, "Invalid"},
    {DataStorageLoSCapabilities_ProvisioningPolicy::Fixed, "Fixed"},
    {DataStorageLoSCapabilities_ProvisioningPolicy::Thin, "Thin"},
});

enum class DataStorageLoSCapabilities_StorageAccessCapability{
    Invalid,
    Read,
    Write,
    WriteOnce,
    Append,
    Streaming,
    Execute,
};

NLOHMANN_JSON_SERIALIZE_ENUM(DataStorageLoSCapabilities_StorageAccessCapability, {
    {DataStorageLoSCapabilities_StorageAccessCapability::Invalid, "Invalid"},
    {DataStorageLoSCapabilities_StorageAccessCapability::Read, "Read"},
    {DataStorageLoSCapabilities_StorageAccessCapability::Write, "Write"},
    {DataStorageLoSCapabilities_StorageAccessCapability::WriteOnce, "WriteOnce"},
    {DataStorageLoSCapabilities_StorageAccessCapability::Append, "Append"},
    {DataStorageLoSCapabilities_StorageAccessCapability::Streaming, "Streaming"},
    {DataStorageLoSCapabilities_StorageAccessCapability::Execute, "Execute"},
});

enum class Drive_EncryptionAbility{
    Invalid,
    None,
    SelfEncryptingDrive,
    Other,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Drive_EncryptionAbility, {
    {Drive_EncryptionAbility::Invalid, "Invalid"},
    {Drive_EncryptionAbility::None, "None"},
    {Drive_EncryptionAbility::SelfEncryptingDrive, "SelfEncryptingDrive"},
    {Drive_EncryptionAbility::Other, "Other"},
});

enum class Drive_EncryptionStatus{
    Invalid,
    Unecrypted,
    Unlocked,
    Locked,
    Foreign,
    Unencrypted,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Drive_EncryptionStatus, {
    {Drive_EncryptionStatus::Invalid, "Invalid"},
    {Drive_EncryptionStatus::Unecrypted, "Unecrypted"},
    {Drive_EncryptionStatus::Unlocked, "Unlocked"},
    {Drive_EncryptionStatus::Locked, "Locked"},
    {Drive_EncryptionStatus::Foreign, "Foreign"},
    {Drive_EncryptionStatus::Unencrypted, "Unencrypted"},
});

enum class Drive_HotspareType{
    Invalid,
    None,
    Global,
    Chassis,
    Dedicated,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Drive_HotspareType, {
    {Drive_HotspareType::Invalid, "Invalid"},
    {Drive_HotspareType::None, "None"},
    {Drive_HotspareType::Global, "Global"},
    {Drive_HotspareType::Chassis, "Chassis"},
    {Drive_HotspareType::Dedicated, "Dedicated"},
});

enum class Drive_MediaType{
    Invalid,
    HDD,
    SSD,
    SMR,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Drive_MediaType, {
    {Drive_MediaType::Invalid, "Invalid"},
    {Drive_MediaType::HDD, "HDD"},
    {Drive_MediaType::SSD, "SSD"},
    {Drive_MediaType::SMR, "SMR"},
});

enum class Drive_StatusIndicator{
    Invalid,
    OK,
    Fail,
    Rebuild,
    PredictiveFailureAnalysis,
    Hotspare,
    InACriticalArray,
    InAFailedArray,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Drive_StatusIndicator, {
    {Drive_StatusIndicator::Invalid, "Invalid"},
    {Drive_StatusIndicator::OK, "OK"},
    {Drive_StatusIndicator::Fail, "Fail"},
    {Drive_StatusIndicator::Rebuild, "Rebuild"},
    {Drive_StatusIndicator::PredictiveFailureAnalysis, "PredictiveFailureAnalysis"},
    {Drive_StatusIndicator::Hotspare, "Hotspare"},
    {Drive_StatusIndicator::InACriticalArray, "InACriticalArray"},
    {Drive_StatusIndicator::InAFailedArray, "InAFailedArray"},
});

enum class Drive_HotspareReplacementModeType{
    Invalid,
    Revertible,
    NonRevertible,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Drive_HotspareReplacementModeType, {
    {Drive_HotspareReplacementModeType::Invalid, "Invalid"},
    {Drive_HotspareReplacementModeType::Revertible, "Revertible"},
    {Drive_HotspareReplacementModeType::NonRevertible, "NonRevertible"},
});

enum class Endpoint_EntityRole{
    Invalid,
    Initiator,
    Target,
    Both,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Endpoint_EntityRole, {
    {Endpoint_EntityRole::Invalid, "Invalid"},
    {Endpoint_EntityRole::Initiator, "Initiator"},
    {Endpoint_EntityRole::Target, "Target"},
    {Endpoint_EntityRole::Both, "Both"},
});

enum class Endpoint_EntityType{
    Invalid,
    StorageInitiator,
    RootComplex,
    NetworkController,
    Drive,
    StorageExpander,
    DisplayController,
    Bridge,
    Processor,
    Volume,
    AccelerationFunction,
    MediaController,
    MemoryChunk,
    Switch,
    FabricBridge,
    Manager,
    StorageSubsystem,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Endpoint_EntityType, {
    {Endpoint_EntityType::Invalid, "Invalid"},
    {Endpoint_EntityType::StorageInitiator, "StorageInitiator"},
    {Endpoint_EntityType::RootComplex, "RootComplex"},
    {Endpoint_EntityType::NetworkController, "NetworkController"},
    {Endpoint_EntityType::Drive, "Drive"},
    {Endpoint_EntityType::StorageExpander, "StorageExpander"},
    {Endpoint_EntityType::DisplayController, "DisplayController"},
    {Endpoint_EntityType::Bridge, "Bridge"},
    {Endpoint_EntityType::Processor, "Processor"},
    {Endpoint_EntityType::Volume, "Volume"},
    {Endpoint_EntityType::AccelerationFunction, "AccelerationFunction"},
    {Endpoint_EntityType::MediaController, "MediaController"},
    {Endpoint_EntityType::MemoryChunk, "MemoryChunk"},
    {Endpoint_EntityType::Switch, "Switch"},
    {Endpoint_EntityType::FabricBridge, "FabricBridge"},
    {Endpoint_EntityType::Manager, "Manager"},
    {Endpoint_EntityType::StorageSubsystem, "StorageSubsystem"},
});

enum class EndpointGroup_GroupType{
    Invalid,
    Client,
    Server,
    Initiator,
    Target,
};

NLOHMANN_JSON_SERIALIZE_ENUM(EndpointGroup_GroupType, {
    {EndpointGroup_GroupType::Invalid, "Invalid"},
    {EndpointGroup_GroupType::Client, "Client"},
    {EndpointGroup_GroupType::Server, "Server"},
    {EndpointGroup_GroupType::Initiator, "Initiator"},
    {EndpointGroup_GroupType::Target, "Target"},
});

enum class EndpointGroup_AccessState{
    Invalid,
    Optimized,
    NonOptimized,
    Standby,
    Unavailable,
    Transitioning,
};

NLOHMANN_JSON_SERIALIZE_ENUM(EndpointGroup_AccessState, {
    {EndpointGroup_AccessState::Invalid, "Invalid"},
    {EndpointGroup_AccessState::Optimized, "Optimized"},
    {EndpointGroup_AccessState::NonOptimized, "NonOptimized"},
    {EndpointGroup_AccessState::Standby, "Standby"},
    {EndpointGroup_AccessState::Unavailable, "Unavailable"},
    {EndpointGroup_AccessState::Transitioning, "Transitioning"},
});

enum class EthernetInterface_LinkStatus{
    Invalid,
    LinkUp,
    NoLink,
    LinkDown,
};

NLOHMANN_JSON_SERIALIZE_ENUM(EthernetInterface_LinkStatus, {
    {EthernetInterface_LinkStatus::Invalid, "Invalid"},
    {EthernetInterface_LinkStatus::LinkUp, "LinkUp"},
    {EthernetInterface_LinkStatus::NoLink, "NoLink"},
    {EthernetInterface_LinkStatus::LinkDown, "LinkDown"},
});

enum class EthernetInterface_DHCPv6OperatingMode{
    Invalid,
    Stateful,
    Stateless,
    Disabled,
    Enabled,
};

NLOHMANN_JSON_SERIALIZE_ENUM(EthernetInterface_DHCPv6OperatingMode, {
    {EthernetInterface_DHCPv6OperatingMode::Invalid, "Invalid"},
    {EthernetInterface_DHCPv6OperatingMode::Stateful, "Stateful"},
    {EthernetInterface_DHCPv6OperatingMode::Stateless, "Stateless"},
    {EthernetInterface_DHCPv6OperatingMode::Disabled, "Disabled"},
    {EthernetInterface_DHCPv6OperatingMode::Enabled, "Enabled"},
});

enum class EthernetInterface_DHCPFallback{
    Invalid,
    Static,
    AutoConfig,
    None,
};

NLOHMANN_JSON_SERIALIZE_ENUM(EthernetInterface_DHCPFallback, {
    {EthernetInterface_DHCPFallback::Invalid, "Invalid"},
    {EthernetInterface_DHCPFallback::Static, "Static"},
    {EthernetInterface_DHCPFallback::AutoConfig, "AutoConfig"},
    {EthernetInterface_DHCPFallback::None, "None"},
});

enum class EthernetInterface_EthernetDeviceType{
    Invalid,
    Physical,
    Virtual,
};

NLOHMANN_JSON_SERIALIZE_ENUM(EthernetInterface_EthernetDeviceType, {
    {EthernetInterface_EthernetDeviceType::Invalid, "Invalid"},
    {EthernetInterface_EthernetDeviceType::Physical, "Physical"},
    {EthernetInterface_EthernetDeviceType::Virtual, "Virtual"},
});

enum class EventDestination_EventDestinationProtocol{
    Invalid,
    Redfish,
    SNMPv1,
    SNMPv2c,
    SNMPv3,
    SMTP,
    SyslogTLS,
    SyslogTCP,
    SyslogUDP,
    SyslogRELP,
    OEM,
};

NLOHMANN_JSON_SERIALIZE_ENUM(EventDestination_EventDestinationProtocol, {
    {EventDestination_EventDestinationProtocol::Invalid, "Invalid"},
    {EventDestination_EventDestinationProtocol::Redfish, "Redfish"},
    {EventDestination_EventDestinationProtocol::SNMPv1, "SNMPv1"},
    {EventDestination_EventDestinationProtocol::SNMPv2c, "SNMPv2c"},
    {EventDestination_EventDestinationProtocol::SNMPv3, "SNMPv3"},
    {EventDestination_EventDestinationProtocol::SMTP, "SMTP"},
    {EventDestination_EventDestinationProtocol::SyslogTLS, "SyslogTLS"},
    {EventDestination_EventDestinationProtocol::SyslogTCP, "SyslogTCP"},
    {EventDestination_EventDestinationProtocol::SyslogUDP, "SyslogUDP"},
    {EventDestination_EventDestinationProtocol::SyslogRELP, "SyslogRELP"},
    {EventDestination_EventDestinationProtocol::OEM, "OEM"},
});

enum class EventDestination_SubscriptionType{
    Invalid,
    RedfishEvent,
    SSE,
    SNMPTrap,
    SNMPInform,
    Syslog,
    OEM,
};

NLOHMANN_JSON_SERIALIZE_ENUM(EventDestination_SubscriptionType, {
    {EventDestination_SubscriptionType::Invalid, "Invalid"},
    {EventDestination_SubscriptionType::RedfishEvent, "RedfishEvent"},
    {EventDestination_SubscriptionType::SSE, "SSE"},
    {EventDestination_SubscriptionType::SNMPTrap, "SNMPTrap"},
    {EventDestination_SubscriptionType::SNMPInform, "SNMPInform"},
    {EventDestination_SubscriptionType::Syslog, "Syslog"},
    {EventDestination_SubscriptionType::OEM, "OEM"},
});

enum class EventDestination_DeliveryRetryPolicy{
    Invalid,
    TerminateAfterRetries,
    SuspendRetries,
    RetryForever,
    RetryForeverWithBackoff,
};

NLOHMANN_JSON_SERIALIZE_ENUM(EventDestination_DeliveryRetryPolicy, {
    {EventDestination_DeliveryRetryPolicy::Invalid, "Invalid"},
    {EventDestination_DeliveryRetryPolicy::TerminateAfterRetries, "TerminateAfterRetries"},
    {EventDestination_DeliveryRetryPolicy::SuspendRetries, "SuspendRetries"},
    {EventDestination_DeliveryRetryPolicy::RetryForever, "RetryForever"},
    {EventDestination_DeliveryRetryPolicy::RetryForeverWithBackoff, "RetryForeverWithBackoff"},
});

enum class EventDestination_SNMPAuthenticationProtocols{
    Invalid,
    None,
    CommunityString,
    HMAC_MD5,
    HMAC_SHA96,
    HMAC128_SHA224,
    HMAC192_SHA256,
    HMAC256_SHA384,
    HMAC384_SHA512,
};

NLOHMANN_JSON_SERIALIZE_ENUM(EventDestination_SNMPAuthenticationProtocols, {
    {EventDestination_SNMPAuthenticationProtocols::Invalid, "Invalid"},
    {EventDestination_SNMPAuthenticationProtocols::None, "None"},
    {EventDestination_SNMPAuthenticationProtocols::CommunityString, "CommunityString"},
    {EventDestination_SNMPAuthenticationProtocols::HMAC_MD5, "HMAC_MD5"},
    {EventDestination_SNMPAuthenticationProtocols::HMAC_SHA96, "HMAC_SHA96"},
    {EventDestination_SNMPAuthenticationProtocols::HMAC128_SHA224, "HMAC128_SHA224"},
    {EventDestination_SNMPAuthenticationProtocols::HMAC192_SHA256, "HMAC192_SHA256"},
    {EventDestination_SNMPAuthenticationProtocols::HMAC256_SHA384, "HMAC256_SHA384"},
    {EventDestination_SNMPAuthenticationProtocols::HMAC384_SHA512, "HMAC384_SHA512"},
});

enum class EventDestination_SNMPEncryptionProtocols{
    Invalid,
    None,
    CBC_DES,
    CFB128_AES128,
};

NLOHMANN_JSON_SERIALIZE_ENUM(EventDestination_SNMPEncryptionProtocols, {
    {EventDestination_SNMPEncryptionProtocols::Invalid, "Invalid"},
    {EventDestination_SNMPEncryptionProtocols::None, "None"},
    {EventDestination_SNMPEncryptionProtocols::CBC_DES, "CBC_DES"},
    {EventDestination_SNMPEncryptionProtocols::CFB128_AES128, "CFB128_AES128"},
});

enum class EventDestination_SyslogFacility{
    Invalid,
    Kern,
    User,
    Mail,
    Daemon,
    Auth,
    Syslog,
    LPR,
    News,
    UUCP,
    Cron,
    Authpriv,
    FTP,
    NTP,
    Security,
    Console,
    SolarisCron,
    Local0,
    Local1,
    Local2,
    Local3,
    Local4,
    Local5,
    Local6,
    Local7,
};

NLOHMANN_JSON_SERIALIZE_ENUM(EventDestination_SyslogFacility, {
    {EventDestination_SyslogFacility::Invalid, "Invalid"},
    {EventDestination_SyslogFacility::Kern, "Kern"},
    {EventDestination_SyslogFacility::User, "User"},
    {EventDestination_SyslogFacility::Mail, "Mail"},
    {EventDestination_SyslogFacility::Daemon, "Daemon"},
    {EventDestination_SyslogFacility::Auth, "Auth"},
    {EventDestination_SyslogFacility::Syslog, "Syslog"},
    {EventDestination_SyslogFacility::LPR, "LPR"},
    {EventDestination_SyslogFacility::News, "News"},
    {EventDestination_SyslogFacility::UUCP, "UUCP"},
    {EventDestination_SyslogFacility::Cron, "Cron"},
    {EventDestination_SyslogFacility::Authpriv, "Authpriv"},
    {EventDestination_SyslogFacility::FTP, "FTP"},
    {EventDestination_SyslogFacility::NTP, "NTP"},
    {EventDestination_SyslogFacility::Security, "Security"},
    {EventDestination_SyslogFacility::Console, "Console"},
    {EventDestination_SyslogFacility::SolarisCron, "SolarisCron"},
    {EventDestination_SyslogFacility::Local0, "Local0"},
    {EventDestination_SyslogFacility::Local1, "Local1"},
    {EventDestination_SyslogFacility::Local2, "Local2"},
    {EventDestination_SyslogFacility::Local3, "Local3"},
    {EventDestination_SyslogFacility::Local4, "Local4"},
    {EventDestination_SyslogFacility::Local5, "Local5"},
    {EventDestination_SyslogFacility::Local6, "Local6"},
    {EventDestination_SyslogFacility::Local7, "Local7"},
});

enum class EventDestination_SyslogSeverity{
    Invalid,
    Emergency,
    Alert,
    Critical,
    Error,
    Warning,
    Notice,
    Informational,
    Debug,
    All,
};

NLOHMANN_JSON_SERIALIZE_ENUM(EventDestination_SyslogSeverity, {
    {EventDestination_SyslogSeverity::Invalid, "Invalid"},
    {EventDestination_SyslogSeverity::Emergency, "Emergency"},
    {EventDestination_SyslogSeverity::Alert, "Alert"},
    {EventDestination_SyslogSeverity::Critical, "Critical"},
    {EventDestination_SyslogSeverity::Error, "Error"},
    {EventDestination_SyslogSeverity::Warning, "Warning"},
    {EventDestination_SyslogSeverity::Notice, "Notice"},
    {EventDestination_SyslogSeverity::Informational, "Informational"},
    {EventDestination_SyslogSeverity::Debug, "Debug"},
    {EventDestination_SyslogSeverity::All, "All"},
});

enum class EventDestination_EventFormatType{
    Invalid,
    Event,
    MetricReport,
};

NLOHMANN_JSON_SERIALIZE_ENUM(EventDestination_EventFormatType, {
    {EventDestination_EventFormatType::Invalid, "Invalid"},
    {EventDestination_EventFormatType::Event, "Event"},
    {EventDestination_EventFormatType::MetricReport, "MetricReport"},
});

enum class Event_EventType{
    Invalid,
    StatusChange,
    ResourceUpdated,
    ResourceAdded,
    ResourceRemoved,
    Alert,
    MetricReport,
    Other,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Event_EventType, {
    {Event_EventType::Invalid, "Invalid"},
    {Event_EventType::StatusChange, "StatusChange"},
    {Event_EventType::ResourceUpdated, "ResourceUpdated"},
    {Event_EventType::ResourceAdded, "ResourceAdded"},
    {Event_EventType::ResourceRemoved, "ResourceRemoved"},
    {Event_EventType::Alert, "Alert"},
    {Event_EventType::MetricReport, "MetricReport"},
    {Event_EventType::Other, "Other"},
});

enum class EventService_SMTPAuthenticationMethods{
    Invalid,
    None,
    AutoDetect,
    Plain,
    Login,
    CRAM_MD5,
};

NLOHMANN_JSON_SERIALIZE_ENUM(EventService_SMTPAuthenticationMethods, {
    {EventService_SMTPAuthenticationMethods::Invalid, "Invalid"},
    {EventService_SMTPAuthenticationMethods::None, "None"},
    {EventService_SMTPAuthenticationMethods::AutoDetect, "AutoDetect"},
    {EventService_SMTPAuthenticationMethods::Plain, "Plain"},
    {EventService_SMTPAuthenticationMethods::Login, "Login"},
    {EventService_SMTPAuthenticationMethods::CRAM_MD5, "CRAM_MD5"},
});

enum class EventService_SMTPConnectionProtocol{
    Invalid,
    None,
    AutoDetect,
    StartTLS,
    TLS_SSL,
};

NLOHMANN_JSON_SERIALIZE_ENUM(EventService_SMTPConnectionProtocol, {
    {EventService_SMTPConnectionProtocol::Invalid, "Invalid"},
    {EventService_SMTPConnectionProtocol::None, "None"},
    {EventService_SMTPConnectionProtocol::AutoDetect, "AutoDetect"},
    {EventService_SMTPConnectionProtocol::StartTLS, "StartTLS"},
    {EventService_SMTPConnectionProtocol::TLS_SSL, "TLS_SSL"},
});

enum class ExternalAccountProvider_AccountProviderTypes{
    Invalid,
    RedfishService,
    ActiveDirectoryService,
    LDAPService,
    OEM,
    TACACSplus,
    OAuth2,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ExternalAccountProvider_AccountProviderTypes, {
    {ExternalAccountProvider_AccountProviderTypes::Invalid, "Invalid"},
    {ExternalAccountProvider_AccountProviderTypes::RedfishService, "RedfishService"},
    {ExternalAccountProvider_AccountProviderTypes::ActiveDirectoryService, "ActiveDirectoryService"},
    {ExternalAccountProvider_AccountProviderTypes::LDAPService, "LDAPService"},
    {ExternalAccountProvider_AccountProviderTypes::OEM, "OEM"},
    {ExternalAccountProvider_AccountProviderTypes::TACACSplus, "TACACSplus"},
    {ExternalAccountProvider_AccountProviderTypes::OAuth2, "OAuth2"},
});

enum class ExternalAccountProvider_AuthenticationTypes{
    Invalid,
    Token,
    KerberosKeytab,
    UsernameAndPassword,
    OEM,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ExternalAccountProvider_AuthenticationTypes, {
    {ExternalAccountProvider_AuthenticationTypes::Invalid, "Invalid"},
    {ExternalAccountProvider_AuthenticationTypes::Token, "Token"},
    {ExternalAccountProvider_AuthenticationTypes::KerberosKeytab, "KerberosKeytab"},
    {ExternalAccountProvider_AuthenticationTypes::UsernameAndPassword, "UsernameAndPassword"},
    {ExternalAccountProvider_AuthenticationTypes::OEM, "OEM"},
});

enum class ExternalAccountProvider_TACACSplusPasswordExchangeProtocol{
    Invalid,
    ASCII,
    PAP,
    CHAP,
    MSCHAPv1,
    MSCHAPv2,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ExternalAccountProvider_TACACSplusPasswordExchangeProtocol, {
    {ExternalAccountProvider_TACACSplusPasswordExchangeProtocol::Invalid, "Invalid"},
    {ExternalAccountProvider_TACACSplusPasswordExchangeProtocol::ASCII, "ASCII"},
    {ExternalAccountProvider_TACACSplusPasswordExchangeProtocol::PAP, "PAP"},
    {ExternalAccountProvider_TACACSplusPasswordExchangeProtocol::CHAP, "CHAP"},
    {ExternalAccountProvider_TACACSplusPasswordExchangeProtocol::MSCHAPv1, "MSCHAPv1"},
    {ExternalAccountProvider_TACACSplusPasswordExchangeProtocol::MSCHAPv2, "MSCHAPv2"},
});

enum class ExternalAccountProvider_OAuth2Mode{
    Invalid,
    Discovery,
    Offline,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ExternalAccountProvider_OAuth2Mode, {
    {ExternalAccountProvider_OAuth2Mode::Invalid, "Invalid"},
    {ExternalAccountProvider_OAuth2Mode::Discovery, "Discovery"},
    {ExternalAccountProvider_OAuth2Mode::Offline, "Offline"},
});

enum class Facility_FacilityType{
    Invalid,
    Room,
    Floor,
    Building,
    Site,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Facility_FacilityType, {
    {Facility_FacilityType::Invalid, "Invalid"},
    {Facility_FacilityType::Room, "Room"},
    {Facility_FacilityType::Floor, "Floor"},
    {Facility_FacilityType::Building, "Building"},
    {Facility_FacilityType::Site, "Site"},
});

enum class FileShare_QuotaType{
    Invalid,
    Soft,
    Hard,
};

NLOHMANN_JSON_SERIALIZE_ENUM(FileShare_QuotaType, {
    {FileShare_QuotaType::Invalid, "Invalid"},
    {FileShare_QuotaType::Soft, "Soft"},
    {FileShare_QuotaType::Hard, "Hard"},
});

enum class FileSystem_CharacterCodeSet{
    Invalid,
    ASCII,
    Unicode,
    ISO2022,
    ISO8859_1,
    ExtendedUNIXCode,
    UTF_8,
    UTF_16,
    UCS_2,
};

NLOHMANN_JSON_SERIALIZE_ENUM(FileSystem_CharacterCodeSet, {
    {FileSystem_CharacterCodeSet::Invalid, "Invalid"},
    {FileSystem_CharacterCodeSet::ASCII, "ASCII"},
    {FileSystem_CharacterCodeSet::Unicode, "Unicode"},
    {FileSystem_CharacterCodeSet::ISO2022, "ISO2022"},
    {FileSystem_CharacterCodeSet::ISO8859_1, "ISO8859_1"},
    {FileSystem_CharacterCodeSet::ExtendedUNIXCode, "ExtendedUNIXCode"},
    {FileSystem_CharacterCodeSet::UTF_8, "UTF_8"},
    {FileSystem_CharacterCodeSet::UTF_16, "UTF_16"},
    {FileSystem_CharacterCodeSet::UCS_2, "UCS_2"},
});

enum class FileSystem_FileProtocol{
    Invalid,
    NFSv3,
    NFSv4_0,
    NFSv4_1,
    SMBv2_0,
    SMBv2_1,
    SMBv3_0,
    SMBv3_0_2,
    SMBv3_1_1,
};

NLOHMANN_JSON_SERIALIZE_ENUM(FileSystem_FileProtocol, {
    {FileSystem_FileProtocol::Invalid, "Invalid"},
    {FileSystem_FileProtocol::NFSv3, "NFSv3"},
    {FileSystem_FileProtocol::NFSv4_0, "NFSv4_0"},
    {FileSystem_FileProtocol::NFSv4_1, "NFSv4_1"},
    {FileSystem_FileProtocol::SMBv2_0, "SMBv2_0"},
    {FileSystem_FileProtocol::SMBv2_1, "SMBv2_1"},
    {FileSystem_FileProtocol::SMBv3_0, "SMBv3_0"},
    {FileSystem_FileProtocol::SMBv3_0_2, "SMBv3_0_2"},
    {FileSystem_FileProtocol::SMBv3_1_1, "SMBv3_1_1"},
});

enum class HostInterface_AuthenticationMode{
    Invalid,
    AuthNone,
    BasicAuth,
    RedfishSessionAuth,
    OemAuth,
};

NLOHMANN_JSON_SERIALIZE_ENUM(HostInterface_AuthenticationMode, {
    {HostInterface_AuthenticationMode::Invalid, "Invalid"},
    {HostInterface_AuthenticationMode::AuthNone, "AuthNone"},
    {HostInterface_AuthenticationMode::BasicAuth, "BasicAuth"},
    {HostInterface_AuthenticationMode::RedfishSessionAuth, "RedfishSessionAuth"},
    {HostInterface_AuthenticationMode::OemAuth, "OemAuth"},
});

enum class HostInterface_HostInterfaceType{
    Invalid,
    NetworkHostInterface,
};

NLOHMANN_JSON_SERIALIZE_ENUM(HostInterface_HostInterfaceType, {
    {HostInterface_HostInterfaceType::Invalid, "Invalid"},
    {HostInterface_HostInterfaceType::NetworkHostInterface, "NetworkHostInterface"},
});

enum class IOPerformanceLoSCapabilities_IOAccessPattern{
    Invalid,
    ReadWrite,
    SequentialRead,
    SequentialWrite,
    RandomReadNew,
    RandomReadAgain,
};

NLOHMANN_JSON_SERIALIZE_ENUM(IOPerformanceLoSCapabilities_IOAccessPattern, {
    {IOPerformanceLoSCapabilities_IOAccessPattern::Invalid, "Invalid"},
    {IOPerformanceLoSCapabilities_IOAccessPattern::ReadWrite, "ReadWrite"},
    {IOPerformanceLoSCapabilities_IOAccessPattern::SequentialRead, "SequentialRead"},
    {IOPerformanceLoSCapabilities_IOAccessPattern::SequentialWrite, "SequentialWrite"},
    {IOPerformanceLoSCapabilities_IOAccessPattern::RandomReadNew, "RandomReadNew"},
    {IOPerformanceLoSCapabilities_IOAccessPattern::RandomReadAgain, "RandomReadAgain"},
});

enum class IPAddresses_AddressState{
    Invalid,
    Preferred,
    Deprecated,
    Tentative,
    Failed,
};

NLOHMANN_JSON_SERIALIZE_ENUM(IPAddresses_AddressState, {
    {IPAddresses_AddressState::Invalid, "Invalid"},
    {IPAddresses_AddressState::Preferred, "Preferred"},
    {IPAddresses_AddressState::Deprecated, "Deprecated"},
    {IPAddresses_AddressState::Tentative, "Tentative"},
    {IPAddresses_AddressState::Failed, "Failed"},
});

enum class IPAddresses_IPv4AddressOrigin{
    Invalid,
    Static,
    DHCP,
    BOOTP,
    IPv4LinkLocal,
};

NLOHMANN_JSON_SERIALIZE_ENUM(IPAddresses_IPv4AddressOrigin, {
    {IPAddresses_IPv4AddressOrigin::Invalid, "Invalid"},
    {IPAddresses_IPv4AddressOrigin::Static, "Static"},
    {IPAddresses_IPv4AddressOrigin::DHCP, "DHCP"},
    {IPAddresses_IPv4AddressOrigin::BOOTP, "BOOTP"},
    {IPAddresses_IPv4AddressOrigin::IPv4LinkLocal, "IPv4LinkLocal"},
});

enum class IPAddresses_IPv6AddressOrigin{
    Invalid,
    Static,
    DHCPv6,
    LinkLocal,
    SLAAC,
};

NLOHMANN_JSON_SERIALIZE_ENUM(IPAddresses_IPv6AddressOrigin, {
    {IPAddresses_IPv6AddressOrigin::Invalid, "Invalid"},
    {IPAddresses_IPv6AddressOrigin::Static, "Static"},
    {IPAddresses_IPv6AddressOrigin::DHCPv6, "DHCPv6"},
    {IPAddresses_IPv6AddressOrigin::LinkLocal, "LinkLocal"},
    {IPAddresses_IPv6AddressOrigin::SLAAC, "SLAAC"},
});

enum class Job_JobState{
    Invalid,
    New,
    Starting,
    Running,
    Suspended,
    Interrupted,
    Pending,
    Stopping,
    Completed,
    Cancelled,
    Exception,
    Service,
    UserIntervention,
    Continue,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Job_JobState, {
    {Job_JobState::Invalid, "Invalid"},
    {Job_JobState::New, "New"},
    {Job_JobState::Starting, "Starting"},
    {Job_JobState::Running, "Running"},
    {Job_JobState::Suspended, "Suspended"},
    {Job_JobState::Interrupted, "Interrupted"},
    {Job_JobState::Pending, "Pending"},
    {Job_JobState::Stopping, "Stopping"},
    {Job_JobState::Completed, "Completed"},
    {Job_JobState::Cancelled, "Cancelled"},
    {Job_JobState::Exception, "Exception"},
    {Job_JobState::Service, "Service"},
    {Job_JobState::UserIntervention, "UserIntervention"},
    {Job_JobState::Continue, "Continue"},
});

enum class Key_KeyType{
    Invalid,
    NVMeoF,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Key_KeyType, {
    {Key_KeyType::Invalid, "Invalid"},
    {Key_KeyType::NVMeoF, "NVMeoF"},
});

enum class Key_NVMeoFSecureHashType{
    Invalid,
    SHA256,
    SHA384,
    SHA512,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Key_NVMeoFSecureHashType, {
    {Key_NVMeoFSecureHashType::Invalid, "Invalid"},
    {Key_NVMeoFSecureHashType::SHA256, "SHA256"},
    {Key_NVMeoFSecureHashType::SHA384, "SHA384"},
    {Key_NVMeoFSecureHashType::SHA512, "SHA512"},
});

enum class Key_NVMeoFSecurityProtocolType{
    Invalid,
    DHHC,
    TLS_PSK,
    OEM,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Key_NVMeoFSecurityProtocolType, {
    {Key_NVMeoFSecurityProtocolType::Invalid, "Invalid"},
    {Key_NVMeoFSecurityProtocolType::DHHC, "DHHC"},
    {Key_NVMeoFSecurityProtocolType::TLS_PSK, "TLS_PSK"},
    {Key_NVMeoFSecurityProtocolType::OEM, "OEM"},
});

enum class KeyPolicy_KeyPolicyType{
    Invalid,
    NVMeoF,
};

NLOHMANN_JSON_SERIALIZE_ENUM(KeyPolicy_KeyPolicyType, {
    {KeyPolicy_KeyPolicyType::Invalid, "Invalid"},
    {KeyPolicy_KeyPolicyType::NVMeoF, "NVMeoF"},
});

enum class KeyPolicy_NVMeoFCipherSuiteType{
    Invalid,
    TLS_AES_128_GCM_SHA256,
    TLS_AES_256_GCM_SHA384,
};

NLOHMANN_JSON_SERIALIZE_ENUM(KeyPolicy_NVMeoFCipherSuiteType, {
    {KeyPolicy_NVMeoFCipherSuiteType::Invalid, "Invalid"},
    {KeyPolicy_NVMeoFCipherSuiteType::TLS_AES_128_GCM_SHA256, "TLS_AES_128_GCM_SHA256"},
    {KeyPolicy_NVMeoFCipherSuiteType::TLS_AES_256_GCM_SHA384, "TLS_AES_256_GCM_SHA384"},
});

enum class KeyPolicy_NVMeoFDHGroupType{
    Invalid,
    FFDHE2048,
    FFDHE3072,
    FFDHE4096,
    FFDHE6144,
    FFDHE8192,
};

NLOHMANN_JSON_SERIALIZE_ENUM(KeyPolicy_NVMeoFDHGroupType, {
    {KeyPolicy_NVMeoFDHGroupType::Invalid, "Invalid"},
    {KeyPolicy_NVMeoFDHGroupType::FFDHE2048, "FFDHE2048"},
    {KeyPolicy_NVMeoFDHGroupType::FFDHE3072, "FFDHE3072"},
    {KeyPolicy_NVMeoFDHGroupType::FFDHE4096, "FFDHE4096"},
    {KeyPolicy_NVMeoFDHGroupType::FFDHE6144, "FFDHE6144"},
    {KeyPolicy_NVMeoFDHGroupType::FFDHE8192, "FFDHE8192"},
});

enum class KeyPolicy_NVMeoFSecureHashType{
    Invalid,
    SHA256,
    SHA384,
    SHA512,
};

NLOHMANN_JSON_SERIALIZE_ENUM(KeyPolicy_NVMeoFSecureHashType, {
    {KeyPolicy_NVMeoFSecureHashType::Invalid, "Invalid"},
    {KeyPolicy_NVMeoFSecureHashType::SHA256, "SHA256"},
    {KeyPolicy_NVMeoFSecureHashType::SHA384, "SHA384"},
    {KeyPolicy_NVMeoFSecureHashType::SHA512, "SHA512"},
});

enum class KeyPolicy_NVMeoFSecurityProtocolType{
    Invalid,
    DHHC,
    TLS_PSK,
    OEM,
};

NLOHMANN_JSON_SERIALIZE_ENUM(KeyPolicy_NVMeoFSecurityProtocolType, {
    {KeyPolicy_NVMeoFSecurityProtocolType::Invalid, "Invalid"},
    {KeyPolicy_NVMeoFSecurityProtocolType::DHHC, "DHHC"},
    {KeyPolicy_NVMeoFSecurityProtocolType::TLS_PSK, "TLS_PSK"},
    {KeyPolicy_NVMeoFSecurityProtocolType::OEM, "OEM"},
});

enum class KeyPolicy_NVMeoFSecurityTransportType{
    Invalid,
    TLSv2,
    TLSv3,
};

NLOHMANN_JSON_SERIALIZE_ENUM(KeyPolicy_NVMeoFSecurityTransportType, {
    {KeyPolicy_NVMeoFSecurityTransportType::Invalid, "Invalid"},
    {KeyPolicy_NVMeoFSecurityTransportType::TLSv2, "TLSv2"},
    {KeyPolicy_NVMeoFSecurityTransportType::TLSv3, "TLSv3"},
});

enum class License_AuthorizationScope{
    Invalid,
    Device,
    Capacity,
    Service,
};

NLOHMANN_JSON_SERIALIZE_ENUM(License_AuthorizationScope, {
    {License_AuthorizationScope::Invalid, "Invalid"},
    {License_AuthorizationScope::Device, "Device"},
    {License_AuthorizationScope::Capacity, "Capacity"},
    {License_AuthorizationScope::Service, "Service"},
});

enum class License_LicenseOrigin{
    Invalid,
    BuiltIn,
    Installed,
};

NLOHMANN_JSON_SERIALIZE_ENUM(License_LicenseOrigin, {
    {License_LicenseOrigin::Invalid, "Invalid"},
    {License_LicenseOrigin::BuiltIn, "BuiltIn"},
    {License_LicenseOrigin::Installed, "Installed"},
});

enum class License_LicenseType{
    Invalid,
    Production,
    Prototype,
    Trial,
};

NLOHMANN_JSON_SERIALIZE_ENUM(License_LicenseType, {
    {License_LicenseType::Invalid, "Invalid"},
    {License_LicenseType::Production, "Production"},
    {License_LicenseType::Prototype, "Prototype"},
    {License_LicenseType::Trial, "Trial"},
});

enum class LicenseService_TransferProtocolType{
    Invalid,
    CIFS,
    FTP,
    SFTP,
    HTTP,
    HTTPS,
    SCP,
    TFTP,
    OEM,
    NFS,
};

NLOHMANN_JSON_SERIALIZE_ENUM(LicenseService_TransferProtocolType, {
    {LicenseService_TransferProtocolType::Invalid, "Invalid"},
    {LicenseService_TransferProtocolType::CIFS, "CIFS"},
    {LicenseService_TransferProtocolType::FTP, "FTP"},
    {LicenseService_TransferProtocolType::SFTP, "SFTP"},
    {LicenseService_TransferProtocolType::HTTP, "HTTP"},
    {LicenseService_TransferProtocolType::HTTPS, "HTTPS"},
    {LicenseService_TransferProtocolType::SCP, "SCP"},
    {LicenseService_TransferProtocolType::TFTP, "TFTP"},
    {LicenseService_TransferProtocolType::OEM, "OEM"},
    {LicenseService_TransferProtocolType::NFS, "NFS"},
});

enum class LogEntry_EventSeverity{
    Invalid,
    OK,
    Warning,
    Critical,
};

NLOHMANN_JSON_SERIALIZE_ENUM(LogEntry_EventSeverity, {
    {LogEntry_EventSeverity::Invalid, "Invalid"},
    {LogEntry_EventSeverity::OK, "OK"},
    {LogEntry_EventSeverity::Warning, "Warning"},
    {LogEntry_EventSeverity::Critical, "Critical"},
});

enum class LogEntry_LogEntryType{
    Invalid,
    Event,
    SEL,
    Oem,
};

NLOHMANN_JSON_SERIALIZE_ENUM(LogEntry_LogEntryType, {
    {LogEntry_LogEntryType::Invalid, "Invalid"},
    {LogEntry_LogEntryType::Event, "Event"},
    {LogEntry_LogEntryType::SEL, "SEL"},
    {LogEntry_LogEntryType::Oem, "Oem"},
});

enum class LogEntry_OriginatorTypes{
    Invalid,
    Client,
    Internal,
    SupportingService,
};

NLOHMANN_JSON_SERIALIZE_ENUM(LogEntry_OriginatorTypes, {
    {LogEntry_OriginatorTypes::Invalid, "Invalid"},
    {LogEntry_OriginatorTypes::Client, "Client"},
    {LogEntry_OriginatorTypes::Internal, "Internal"},
    {LogEntry_OriginatorTypes::SupportingService, "SupportingService"},
});

enum class LogEntry_LogDiagnosticDataTypes{
    Invalid,
    Manager,
    PreOS,
    OS,
    OEM,
    CPER,
    CPERSection,
};

NLOHMANN_JSON_SERIALIZE_ENUM(LogEntry_LogDiagnosticDataTypes, {
    {LogEntry_LogDiagnosticDataTypes::Invalid, "Invalid"},
    {LogEntry_LogDiagnosticDataTypes::Manager, "Manager"},
    {LogEntry_LogDiagnosticDataTypes::PreOS, "PreOS"},
    {LogEntry_LogDiagnosticDataTypes::OS, "OS"},
    {LogEntry_LogDiagnosticDataTypes::OEM, "OEM"},
    {LogEntry_LogDiagnosticDataTypes::CPER, "CPER"},
    {LogEntry_LogDiagnosticDataTypes::CPERSection, "CPERSection"},
});

enum class LogService_OverWritePolicy{
    Invalid,
    Unknown,
    WrapsWhenFull,
    NeverOverWrites,
};

NLOHMANN_JSON_SERIALIZE_ENUM(LogService_OverWritePolicy, {
    {LogService_OverWritePolicy::Invalid, "Invalid"},
    {LogService_OverWritePolicy::Unknown, "Unknown"},
    {LogService_OverWritePolicy::WrapsWhenFull, "WrapsWhenFull"},
    {LogService_OverWritePolicy::NeverOverWrites, "NeverOverWrites"},
});

enum class LogService_LogEntryTypes{
    Invalid,
    Event,
    SEL,
    Multiple,
    OEM,
};

NLOHMANN_JSON_SERIALIZE_ENUM(LogService_LogEntryTypes, {
    {LogService_LogEntryTypes::Invalid, "Invalid"},
    {LogService_LogEntryTypes::Event, "Event"},
    {LogService_LogEntryTypes::SEL, "SEL"},
    {LogService_LogEntryTypes::Multiple, "Multiple"},
    {LogService_LogEntryTypes::OEM, "OEM"},
});

enum class LogService_LogDiagnosticDataTypes{
    Invalid,
    Manager,
    PreOS,
    OS,
    OEM,
};

NLOHMANN_JSON_SERIALIZE_ENUM(LogService_LogDiagnosticDataTypes, {
    {LogService_LogDiagnosticDataTypes::Invalid, "Invalid"},
    {LogService_LogDiagnosticDataTypes::Manager, "Manager"},
    {LogService_LogDiagnosticDataTypes::PreOS, "PreOS"},
    {LogService_LogDiagnosticDataTypes::OS, "OS"},
    {LogService_LogDiagnosticDataTypes::OEM, "OEM"},
});

enum class LogService_SyslogFacility{
    Invalid,
    Kern,
    User,
    Mail,
    Daemon,
    Auth,
    Syslog,
    LPR,
    News,
    UUCP,
    Cron,
    Authpriv,
    FTP,
    NTP,
    Security,
    Console,
    SolarisCron,
    Local0,
    Local1,
    Local2,
    Local3,
    Local4,
    Local5,
    Local6,
    Local7,
};

NLOHMANN_JSON_SERIALIZE_ENUM(LogService_SyslogFacility, {
    {LogService_SyslogFacility::Invalid, "Invalid"},
    {LogService_SyslogFacility::Kern, "Kern"},
    {LogService_SyslogFacility::User, "User"},
    {LogService_SyslogFacility::Mail, "Mail"},
    {LogService_SyslogFacility::Daemon, "Daemon"},
    {LogService_SyslogFacility::Auth, "Auth"},
    {LogService_SyslogFacility::Syslog, "Syslog"},
    {LogService_SyslogFacility::LPR, "LPR"},
    {LogService_SyslogFacility::News, "News"},
    {LogService_SyslogFacility::UUCP, "UUCP"},
    {LogService_SyslogFacility::Cron, "Cron"},
    {LogService_SyslogFacility::Authpriv, "Authpriv"},
    {LogService_SyslogFacility::FTP, "FTP"},
    {LogService_SyslogFacility::NTP, "NTP"},
    {LogService_SyslogFacility::Security, "Security"},
    {LogService_SyslogFacility::Console, "Console"},
    {LogService_SyslogFacility::SolarisCron, "SolarisCron"},
    {LogService_SyslogFacility::Local0, "Local0"},
    {LogService_SyslogFacility::Local1, "Local1"},
    {LogService_SyslogFacility::Local2, "Local2"},
    {LogService_SyslogFacility::Local3, "Local3"},
    {LogService_SyslogFacility::Local4, "Local4"},
    {LogService_SyslogFacility::Local5, "Local5"},
    {LogService_SyslogFacility::Local6, "Local6"},
    {LogService_SyslogFacility::Local7, "Local7"},
});

enum class LogService_SyslogSeverity{
    Invalid,
    Emergency,
    Alert,
    Critical,
    Error,
    Warning,
    Notice,
    Informational,
    Debug,
    All,
};

NLOHMANN_JSON_SERIALIZE_ENUM(LogService_SyslogSeverity, {
    {LogService_SyslogSeverity::Invalid, "Invalid"},
    {LogService_SyslogSeverity::Emergency, "Emergency"},
    {LogService_SyslogSeverity::Alert, "Alert"},
    {LogService_SyslogSeverity::Critical, "Critical"},
    {LogService_SyslogSeverity::Error, "Error"},
    {LogService_SyslogSeverity::Warning, "Warning"},
    {LogService_SyslogSeverity::Notice, "Notice"},
    {LogService_SyslogSeverity::Informational, "Informational"},
    {LogService_SyslogSeverity::Debug, "Debug"},
    {LogService_SyslogSeverity::All, "All"},
});

enum class Manager_CommandConnectTypesSupported{
    Invalid,
    SSH,
    Telnet,
    IPMI,
    Oem,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Manager_CommandConnectTypesSupported, {
    {Manager_CommandConnectTypesSupported::Invalid, "Invalid"},
    {Manager_CommandConnectTypesSupported::SSH, "SSH"},
    {Manager_CommandConnectTypesSupported::Telnet, "Telnet"},
    {Manager_CommandConnectTypesSupported::IPMI, "IPMI"},
    {Manager_CommandConnectTypesSupported::Oem, "Oem"},
});

enum class Manager_GraphicalConnectTypesSupported{
    Invalid,
    KVMIP,
    Oem,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Manager_GraphicalConnectTypesSupported, {
    {Manager_GraphicalConnectTypesSupported::Invalid, "Invalid"},
    {Manager_GraphicalConnectTypesSupported::KVMIP, "KVMIP"},
    {Manager_GraphicalConnectTypesSupported::Oem, "Oem"},
});

enum class Manager_ManagerType{
    Invalid,
    ManagementController,
    EnclosureManager,
    BMC,
    RackManager,
    AuxiliaryController,
    Service,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Manager_ManagerType, {
    {Manager_ManagerType::Invalid, "Invalid"},
    {Manager_ManagerType::ManagementController, "ManagementController"},
    {Manager_ManagerType::EnclosureManager, "EnclosureManager"},
    {Manager_ManagerType::BMC, "BMC"},
    {Manager_ManagerType::RackManager, "RackManager"},
    {Manager_ManagerType::AuxiliaryController, "AuxiliaryController"},
    {Manager_ManagerType::Service, "Service"},
});

enum class Manager_SerialConnectTypesSupported{
    Invalid,
    SSH,
    Telnet,
    IPMI,
    Oem,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Manager_SerialConnectTypesSupported, {
    {Manager_SerialConnectTypesSupported::Invalid, "Invalid"},
    {Manager_SerialConnectTypesSupported::SSH, "SSH"},
    {Manager_SerialConnectTypesSupported::Telnet, "Telnet"},
    {Manager_SerialConnectTypesSupported::IPMI, "IPMI"},
    {Manager_SerialConnectTypesSupported::Oem, "Oem"},
});

enum class Manager_ResetToDefaultsType{
    Invalid,
    ResetAll,
    PreserveNetworkAndUsers,
    PreserveNetwork,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Manager_ResetToDefaultsType, {
    {Manager_ResetToDefaultsType::Invalid, "Invalid"},
    {Manager_ResetToDefaultsType::ResetAll, "ResetAll"},
    {Manager_ResetToDefaultsType::PreserveNetworkAndUsers, "PreserveNetworkAndUsers"},
    {Manager_ResetToDefaultsType::PreserveNetwork, "PreserveNetwork"},
});

enum class ManagerAccount_SNMPAuthenticationProtocols{
    Invalid,
    None,
    HMAC_MD5,
    HMAC_SHA96,
    HMAC128_SHA224,
    HMAC192_SHA256,
    HMAC256_SHA384,
    HMAC384_SHA512,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ManagerAccount_SNMPAuthenticationProtocols, {
    {ManagerAccount_SNMPAuthenticationProtocols::Invalid, "Invalid"},
    {ManagerAccount_SNMPAuthenticationProtocols::None, "None"},
    {ManagerAccount_SNMPAuthenticationProtocols::HMAC_MD5, "HMAC_MD5"},
    {ManagerAccount_SNMPAuthenticationProtocols::HMAC_SHA96, "HMAC_SHA96"},
    {ManagerAccount_SNMPAuthenticationProtocols::HMAC128_SHA224, "HMAC128_SHA224"},
    {ManagerAccount_SNMPAuthenticationProtocols::HMAC192_SHA256, "HMAC192_SHA256"},
    {ManagerAccount_SNMPAuthenticationProtocols::HMAC256_SHA384, "HMAC256_SHA384"},
    {ManagerAccount_SNMPAuthenticationProtocols::HMAC384_SHA512, "HMAC384_SHA512"},
});

enum class ManagerAccount_SNMPEncryptionProtocols{
    Invalid,
    None,
    CBC_DES,
    CFB128_AES128,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ManagerAccount_SNMPEncryptionProtocols, {
    {ManagerAccount_SNMPEncryptionProtocols::Invalid, "Invalid"},
    {ManagerAccount_SNMPEncryptionProtocols::None, "None"},
    {ManagerAccount_SNMPEncryptionProtocols::CBC_DES, "CBC_DES"},
    {ManagerAccount_SNMPEncryptionProtocols::CFB128_AES128, "CFB128_AES128"},
});

enum class ManagerAccount_AccountTypes{
    Invalid,
    Redfish,
    SNMP,
    OEM,
    HostConsole,
    ManagerConsole,
    IPMI,
    KVMIP,
    VirtualMedia,
    WebUI,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ManagerAccount_AccountTypes, {
    {ManagerAccount_AccountTypes::Invalid, "Invalid"},
    {ManagerAccount_AccountTypes::Redfish, "Redfish"},
    {ManagerAccount_AccountTypes::SNMP, "SNMP"},
    {ManagerAccount_AccountTypes::OEM, "OEM"},
    {ManagerAccount_AccountTypes::HostConsole, "HostConsole"},
    {ManagerAccount_AccountTypes::ManagerConsole, "ManagerConsole"},
    {ManagerAccount_AccountTypes::IPMI, "IPMI"},
    {ManagerAccount_AccountTypes::KVMIP, "KVMIP"},
    {ManagerAccount_AccountTypes::VirtualMedia, "VirtualMedia"},
    {ManagerAccount_AccountTypes::WebUI, "WebUI"},
});

enum class ManagerNetworkProtocol_NotifyIPv6Scope{
    Invalid,
    Link,
    Site,
    Organization,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ManagerNetworkProtocol_NotifyIPv6Scope, {
    {ManagerNetworkProtocol_NotifyIPv6Scope::Invalid, "Invalid"},
    {ManagerNetworkProtocol_NotifyIPv6Scope::Link, "Link"},
    {ManagerNetworkProtocol_NotifyIPv6Scope::Site, "Site"},
    {ManagerNetworkProtocol_NotifyIPv6Scope::Organization, "Organization"},
});

enum class ManagerNetworkProtocol_SNMPAuthenticationProtocols{
    Invalid,
    Account,
    CommunityString,
    HMAC_MD5,
    HMAC_SHA96,
    HMAC128_SHA224,
    HMAC192_SHA256,
    HMAC256_SHA384,
    HMAC384_SHA512,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ManagerNetworkProtocol_SNMPAuthenticationProtocols, {
    {ManagerNetworkProtocol_SNMPAuthenticationProtocols::Invalid, "Invalid"},
    {ManagerNetworkProtocol_SNMPAuthenticationProtocols::Account, "Account"},
    {ManagerNetworkProtocol_SNMPAuthenticationProtocols::CommunityString, "CommunityString"},
    {ManagerNetworkProtocol_SNMPAuthenticationProtocols::HMAC_MD5, "HMAC_MD5"},
    {ManagerNetworkProtocol_SNMPAuthenticationProtocols::HMAC_SHA96, "HMAC_SHA96"},
    {ManagerNetworkProtocol_SNMPAuthenticationProtocols::HMAC128_SHA224, "HMAC128_SHA224"},
    {ManagerNetworkProtocol_SNMPAuthenticationProtocols::HMAC192_SHA256, "HMAC192_SHA256"},
    {ManagerNetworkProtocol_SNMPAuthenticationProtocols::HMAC256_SHA384, "HMAC256_SHA384"},
    {ManagerNetworkProtocol_SNMPAuthenticationProtocols::HMAC384_SHA512, "HMAC384_SHA512"},
});

enum class ManagerNetworkProtocol_SNMPCommunityAccessMode{
    Invalid,
    Full,
    Limited,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ManagerNetworkProtocol_SNMPCommunityAccessMode, {
    {ManagerNetworkProtocol_SNMPCommunityAccessMode::Invalid, "Invalid"},
    {ManagerNetworkProtocol_SNMPCommunityAccessMode::Full, "Full"},
    {ManagerNetworkProtocol_SNMPCommunityAccessMode::Limited, "Limited"},
});

enum class ManagerNetworkProtocol_SNMPEncryptionProtocols{
    Invalid,
    None,
    Account,
    CBC_DES,
    CFB128_AES128,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ManagerNetworkProtocol_SNMPEncryptionProtocols, {
    {ManagerNetworkProtocol_SNMPEncryptionProtocols::Invalid, "Invalid"},
    {ManagerNetworkProtocol_SNMPEncryptionProtocols::None, "None"},
    {ManagerNetworkProtocol_SNMPEncryptionProtocols::Account, "Account"},
    {ManagerNetworkProtocol_SNMPEncryptionProtocols::CBC_DES, "CBC_DES"},
    {ManagerNetworkProtocol_SNMPEncryptionProtocols::CFB128_AES128, "CFB128_AES128"},
});

enum class Manifest_Expand{
    Invalid,
    None,
    All,
    Relevant,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Manifest_Expand, {
    {Manifest_Expand::Invalid, "Invalid"},
    {Manifest_Expand::None, "None"},
    {Manifest_Expand::All, "All"},
    {Manifest_Expand::Relevant, "Relevant"},
});

enum class Manifest_StanzaType{
    Invalid,
    ComposeSystem,
    DecomposeSystem,
    ComposeResource,
    DecomposeResource,
    OEM,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Manifest_StanzaType, {
    {Manifest_StanzaType::Invalid, "Invalid"},
    {Manifest_StanzaType::ComposeSystem, "ComposeSystem"},
    {Manifest_StanzaType::DecomposeSystem, "DecomposeSystem"},
    {Manifest_StanzaType::ComposeResource, "ComposeResource"},
    {Manifest_StanzaType::DecomposeResource, "DecomposeResource"},
    {Manifest_StanzaType::OEM, "OEM"},
});

enum class MediaController_MediaControllerType{
    Invalid,
    Memory,
};

NLOHMANN_JSON_SERIALIZE_ENUM(MediaController_MediaControllerType, {
    {MediaController_MediaControllerType::Invalid, "Invalid"},
    {MediaController_MediaControllerType::Memory, "Memory"},
});

enum class Memory_BaseModuleType{
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

NLOHMANN_JSON_SERIALIZE_ENUM(Memory_BaseModuleType, {
    {Memory_BaseModuleType::Invalid, "Invalid"},
    {Memory_BaseModuleType::RDIMM, "RDIMM"},
    {Memory_BaseModuleType::UDIMM, "UDIMM"},
    {Memory_BaseModuleType::SO_DIMM, "SO_DIMM"},
    {Memory_BaseModuleType::LRDIMM, "LRDIMM"},
    {Memory_BaseModuleType::Mini_RDIMM, "Mini_RDIMM"},
    {Memory_BaseModuleType::Mini_UDIMM, "Mini_UDIMM"},
    {Memory_BaseModuleType::SO_RDIMM_72b, "SO_RDIMM_72b"},
    {Memory_BaseModuleType::SO_UDIMM_72b, "SO_UDIMM_72b"},
    {Memory_BaseModuleType::SO_DIMM_16b, "SO_DIMM_16b"},
    {Memory_BaseModuleType::SO_DIMM_32b, "SO_DIMM_32b"},
    {Memory_BaseModuleType::Die, "Die"},
});

enum class Memory_ErrorCorrection{
    Invalid,
    NoECC,
    SingleBitECC,
    MultiBitECC,
    AddressParity,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Memory_ErrorCorrection, {
    {Memory_ErrorCorrection::Invalid, "Invalid"},
    {Memory_ErrorCorrection::NoECC, "NoECC"},
    {Memory_ErrorCorrection::SingleBitECC, "SingleBitECC"},
    {Memory_ErrorCorrection::MultiBitECC, "MultiBitECC"},
    {Memory_ErrorCorrection::AddressParity, "AddressParity"},
});

enum class Memory_MemoryClassification{
    Invalid,
    Volatile,
    ByteAccessiblePersistent,
    Block,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Memory_MemoryClassification, {
    {Memory_MemoryClassification::Invalid, "Invalid"},
    {Memory_MemoryClassification::Volatile, "Volatile"},
    {Memory_MemoryClassification::ByteAccessiblePersistent, "ByteAccessiblePersistent"},
    {Memory_MemoryClassification::Block, "Block"},
});

enum class Memory_MemoryDeviceType{
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

NLOHMANN_JSON_SERIALIZE_ENUM(Memory_MemoryDeviceType, {
    {Memory_MemoryDeviceType::Invalid, "Invalid"},
    {Memory_MemoryDeviceType::DDR, "DDR"},
    {Memory_MemoryDeviceType::DDR2, "DDR2"},
    {Memory_MemoryDeviceType::DDR3, "DDR3"},
    {Memory_MemoryDeviceType::DDR4, "DDR4"},
    {Memory_MemoryDeviceType::DDR4_SDRAM, "DDR4_SDRAM"},
    {Memory_MemoryDeviceType::DDR4E_SDRAM, "DDR4E_SDRAM"},
    {Memory_MemoryDeviceType::LPDDR4_SDRAM, "LPDDR4_SDRAM"},
    {Memory_MemoryDeviceType::DDR3_SDRAM, "DDR3_SDRAM"},
    {Memory_MemoryDeviceType::LPDDR3_SDRAM, "LPDDR3_SDRAM"},
    {Memory_MemoryDeviceType::DDR2_SDRAM, "DDR2_SDRAM"},
    {Memory_MemoryDeviceType::DDR2_SDRAM_FB_DIMM, "DDR2_SDRAM_FB_DIMM"},
    {Memory_MemoryDeviceType::DDR2_SDRAM_FB_DIMM_PROBE, "DDR2_SDRAM_FB_DIMM_PROBE"},
    {Memory_MemoryDeviceType::DDR_SGRAM, "DDR_SGRAM"},
    {Memory_MemoryDeviceType::DDR_SDRAM, "DDR_SDRAM"},
    {Memory_MemoryDeviceType::ROM, "ROM"},
    {Memory_MemoryDeviceType::SDRAM, "SDRAM"},
    {Memory_MemoryDeviceType::EDO, "EDO"},
    {Memory_MemoryDeviceType::FastPageMode, "FastPageMode"},
    {Memory_MemoryDeviceType::PipelinedNibble, "PipelinedNibble"},
    {Memory_MemoryDeviceType::Logical, "Logical"},
    {Memory_MemoryDeviceType::HBM, "HBM"},
    {Memory_MemoryDeviceType::HBM2, "HBM2"},
    {Memory_MemoryDeviceType::HBM3, "HBM3"},
    {Memory_MemoryDeviceType::GDDR, "GDDR"},
    {Memory_MemoryDeviceType::GDDR2, "GDDR2"},
    {Memory_MemoryDeviceType::GDDR3, "GDDR3"},
    {Memory_MemoryDeviceType::GDDR4, "GDDR4"},
    {Memory_MemoryDeviceType::GDDR5, "GDDR5"},
    {Memory_MemoryDeviceType::GDDR5X, "GDDR5X"},
    {Memory_MemoryDeviceType::GDDR6, "GDDR6"},
    {Memory_MemoryDeviceType::DDR5, "DDR5"},
    {Memory_MemoryDeviceType::OEM, "OEM"},
});

enum class Memory_MemoryMedia{
    Invalid,
    DRAM,
    NAND,
    Intel3DXPoint,
    Proprietary,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Memory_MemoryMedia, {
    {Memory_MemoryMedia::Invalid, "Invalid"},
    {Memory_MemoryMedia::DRAM, "DRAM"},
    {Memory_MemoryMedia::NAND, "NAND"},
    {Memory_MemoryMedia::Intel3DXPoint, "Intel3DXPoint"},
    {Memory_MemoryMedia::Proprietary, "Proprietary"},
});

enum class Memory_MemoryType{
    Invalid,
    DRAM,
    NVDIMM_N,
    NVDIMM_F,
    NVDIMM_P,
    IntelOptane,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Memory_MemoryType, {
    {Memory_MemoryType::Invalid, "Invalid"},
    {Memory_MemoryType::DRAM, "DRAM"},
    {Memory_MemoryType::NVDIMM_N, "NVDIMM_N"},
    {Memory_MemoryType::NVDIMM_F, "NVDIMM_F"},
    {Memory_MemoryType::NVDIMM_P, "NVDIMM_P"},
    {Memory_MemoryType::IntelOptane, "IntelOptane"},
});

enum class Memory_OperatingMemoryModes{
    Invalid,
    Volatile,
    PMEM,
    Block,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Memory_OperatingMemoryModes, {
    {Memory_OperatingMemoryModes::Invalid, "Invalid"},
    {Memory_OperatingMemoryModes::Volatile, "Volatile"},
    {Memory_OperatingMemoryModes::PMEM, "PMEM"},
    {Memory_OperatingMemoryModes::Block, "Block"},
});

enum class Memory_SecurityStates{
    Invalid,
    Enabled,
    Disabled,
    Unlocked,
    Locked,
    Frozen,
    Passphraselimit,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Memory_SecurityStates, {
    {Memory_SecurityStates::Invalid, "Invalid"},
    {Memory_SecurityStates::Enabled, "Enabled"},
    {Memory_SecurityStates::Disabled, "Disabled"},
    {Memory_SecurityStates::Unlocked, "Unlocked"},
    {Memory_SecurityStates::Locked, "Locked"},
    {Memory_SecurityStates::Frozen, "Frozen"},
    {Memory_SecurityStates::Passphraselimit, "Passphraselimit"},
});

enum class MemoryChunks_AddressRangeType{
    Invalid,
    Volatile,
    PMEM,
    Block,
};

NLOHMANN_JSON_SERIALIZE_ENUM(MemoryChunks_AddressRangeType, {
    {MemoryChunks_AddressRangeType::Invalid, "Invalid"},
    {MemoryChunks_AddressRangeType::Volatile, "Volatile"},
    {MemoryChunks_AddressRangeType::PMEM, "PMEM"},
    {MemoryChunks_AddressRangeType::Block, "Block"},
});

enum class MessageRegistry_ParamType{
    Invalid,
    string,
    number,
};

NLOHMANN_JSON_SERIALIZE_ENUM(MessageRegistry_ParamType, {
    {MessageRegistry_ParamType::Invalid, "Invalid"},
    {MessageRegistry_ParamType::string, "string"},
    {MessageRegistry_ParamType::number, "number"},
});

enum class MessageRegistry_ClearingType{
    Invalid,
    SameOriginOfCondition,
};

NLOHMANN_JSON_SERIALIZE_ENUM(MessageRegistry_ClearingType, {
    {MessageRegistry_ClearingType::Invalid, "Invalid"},
    {MessageRegistry_ClearingType::SameOriginOfCondition, "SameOriginOfCondition"},
});

enum class MetricDefinition_Calculable{
    Invalid,
    NonCalculatable,
    Summable,
    NonSummable,
};

NLOHMANN_JSON_SERIALIZE_ENUM(MetricDefinition_Calculable, {
    {MetricDefinition_Calculable::Invalid, "Invalid"},
    {MetricDefinition_Calculable::NonCalculatable, "NonCalculatable"},
    {MetricDefinition_Calculable::Summable, "Summable"},
    {MetricDefinition_Calculable::NonSummable, "NonSummable"},
});

enum class MetricDefinition_CalculationAlgorithmEnum{
    Invalid,
    Average,
    Maximum,
    Minimum,
    OEM,
};

NLOHMANN_JSON_SERIALIZE_ENUM(MetricDefinition_CalculationAlgorithmEnum, {
    {MetricDefinition_CalculationAlgorithmEnum::Invalid, "Invalid"},
    {MetricDefinition_CalculationAlgorithmEnum::Average, "Average"},
    {MetricDefinition_CalculationAlgorithmEnum::Maximum, "Maximum"},
    {MetricDefinition_CalculationAlgorithmEnum::Minimum, "Minimum"},
    {MetricDefinition_CalculationAlgorithmEnum::OEM, "OEM"},
});

enum class MetricDefinition_ImplementationType{
    Invalid,
    PhysicalSensor,
    Calculated,
    Synthesized,
    DigitalMeter,
};

NLOHMANN_JSON_SERIALIZE_ENUM(MetricDefinition_ImplementationType, {
    {MetricDefinition_ImplementationType::Invalid, "Invalid"},
    {MetricDefinition_ImplementationType::PhysicalSensor, "PhysicalSensor"},
    {MetricDefinition_ImplementationType::Calculated, "Calculated"},
    {MetricDefinition_ImplementationType::Synthesized, "Synthesized"},
    {MetricDefinition_ImplementationType::DigitalMeter, "DigitalMeter"},
});

enum class MetricDefinition_MetricDataType{
    Invalid,
    Boolean,
    DateTime,
    Decimal,
    Integer,
    String,
    Enumeration,
};

NLOHMANN_JSON_SERIALIZE_ENUM(MetricDefinition_MetricDataType, {
    {MetricDefinition_MetricDataType::Invalid, "Invalid"},
    {MetricDefinition_MetricDataType::Boolean, "Boolean"},
    {MetricDefinition_MetricDataType::DateTime, "DateTime"},
    {MetricDefinition_MetricDataType::Decimal, "Decimal"},
    {MetricDefinition_MetricDataType::Integer, "Integer"},
    {MetricDefinition_MetricDataType::String, "String"},
    {MetricDefinition_MetricDataType::Enumeration, "Enumeration"},
});

enum class MetricDefinition_MetricType{
    Invalid,
    Numeric,
    Discrete,
    Gauge,
    Counter,
    Countdown,
    String,
};

NLOHMANN_JSON_SERIALIZE_ENUM(MetricDefinition_MetricType, {
    {MetricDefinition_MetricType::Invalid, "Invalid"},
    {MetricDefinition_MetricType::Numeric, "Numeric"},
    {MetricDefinition_MetricType::Discrete, "Discrete"},
    {MetricDefinition_MetricType::Gauge, "Gauge"},
    {MetricDefinition_MetricType::Counter, "Counter"},
    {MetricDefinition_MetricType::Countdown, "Countdown"},
    {MetricDefinition_MetricType::String, "String"},
});

enum class MetricReportDefinition_CalculationAlgorithmEnum{
    Invalid,
    Average,
    Maximum,
    Minimum,
    Summation,
};

NLOHMANN_JSON_SERIALIZE_ENUM(MetricReportDefinition_CalculationAlgorithmEnum, {
    {MetricReportDefinition_CalculationAlgorithmEnum::Invalid, "Invalid"},
    {MetricReportDefinition_CalculationAlgorithmEnum::Average, "Average"},
    {MetricReportDefinition_CalculationAlgorithmEnum::Maximum, "Maximum"},
    {MetricReportDefinition_CalculationAlgorithmEnum::Minimum, "Minimum"},
    {MetricReportDefinition_CalculationAlgorithmEnum::Summation, "Summation"},
});

enum class MetricReportDefinition_CollectionTimeScope{
    Invalid,
    Point,
    Interval,
    StartupInterval,
};

NLOHMANN_JSON_SERIALIZE_ENUM(MetricReportDefinition_CollectionTimeScope, {
    {MetricReportDefinition_CollectionTimeScope::Invalid, "Invalid"},
    {MetricReportDefinition_CollectionTimeScope::Point, "Point"},
    {MetricReportDefinition_CollectionTimeScope::Interval, "Interval"},
    {MetricReportDefinition_CollectionTimeScope::StartupInterval, "StartupInterval"},
});

enum class MetricReportDefinition_MetricReportDefinitionType{
    Invalid,
    Periodic,
    OnChange,
    OnRequest,
};

NLOHMANN_JSON_SERIALIZE_ENUM(MetricReportDefinition_MetricReportDefinitionType, {
    {MetricReportDefinition_MetricReportDefinitionType::Invalid, "Invalid"},
    {MetricReportDefinition_MetricReportDefinitionType::Periodic, "Periodic"},
    {MetricReportDefinition_MetricReportDefinitionType::OnChange, "OnChange"},
    {MetricReportDefinition_MetricReportDefinitionType::OnRequest, "OnRequest"},
});

enum class MetricReportDefinition_ReportActionsEnum{
    Invalid,
    LogToMetricReportsCollection,
    RedfishEvent,
};

NLOHMANN_JSON_SERIALIZE_ENUM(MetricReportDefinition_ReportActionsEnum, {
    {MetricReportDefinition_ReportActionsEnum::Invalid, "Invalid"},
    {MetricReportDefinition_ReportActionsEnum::LogToMetricReportsCollection, "LogToMetricReportsCollection"},
    {MetricReportDefinition_ReportActionsEnum::RedfishEvent, "RedfishEvent"},
});

enum class MetricReportDefinition_ReportUpdatesEnum{
    Invalid,
    Overwrite,
    AppendWrapsWhenFull,
    AppendStopsWhenFull,
    NewReport,
};

NLOHMANN_JSON_SERIALIZE_ENUM(MetricReportDefinition_ReportUpdatesEnum, {
    {MetricReportDefinition_ReportUpdatesEnum::Invalid, "Invalid"},
    {MetricReportDefinition_ReportUpdatesEnum::Overwrite, "Overwrite"},
    {MetricReportDefinition_ReportUpdatesEnum::AppendWrapsWhenFull, "AppendWrapsWhenFull"},
    {MetricReportDefinition_ReportUpdatesEnum::AppendStopsWhenFull, "AppendStopsWhenFull"},
    {MetricReportDefinition_ReportUpdatesEnum::NewReport, "NewReport"},
});

enum class NVMeFirmwareImage_NVMeDeviceType{
    Invalid,
    Drive,
    JBOF,
    FabricAttachArray,
};

NLOHMANN_JSON_SERIALIZE_ENUM(NVMeFirmwareImage_NVMeDeviceType, {
    {NVMeFirmwareImage_NVMeDeviceType::Invalid, "Invalid"},
    {NVMeFirmwareImage_NVMeDeviceType::Drive, "Drive"},
    {NVMeFirmwareImage_NVMeDeviceType::JBOF, "JBOF"},
    {NVMeFirmwareImage_NVMeDeviceType::FabricAttachArray, "FabricAttachArray"},
});

enum class NetworkDeviceFunction_AuthenticationMethod{
    Invalid,
    None,
    CHAP,
    MutualCHAP,
};

NLOHMANN_JSON_SERIALIZE_ENUM(NetworkDeviceFunction_AuthenticationMethod, {
    {NetworkDeviceFunction_AuthenticationMethod::Invalid, "Invalid"},
    {NetworkDeviceFunction_AuthenticationMethod::None, "None"},
    {NetworkDeviceFunction_AuthenticationMethod::CHAP, "CHAP"},
    {NetworkDeviceFunction_AuthenticationMethod::MutualCHAP, "MutualCHAP"},
});

enum class NetworkDeviceFunction_BootMode{
    Invalid,
    Disabled,
    PXE,
    iSCSI,
    FibreChannel,
    FibreChannelOverEthernet,
};

NLOHMANN_JSON_SERIALIZE_ENUM(NetworkDeviceFunction_BootMode, {
    {NetworkDeviceFunction_BootMode::Invalid, "Invalid"},
    {NetworkDeviceFunction_BootMode::Disabled, "Disabled"},
    {NetworkDeviceFunction_BootMode::PXE, "PXE"},
    {NetworkDeviceFunction_BootMode::iSCSI, "iSCSI"},
    {NetworkDeviceFunction_BootMode::FibreChannel, "FibreChannel"},
    {NetworkDeviceFunction_BootMode::FibreChannelOverEthernet, "FibreChannelOverEthernet"},
});

enum class NetworkDeviceFunction_IPAddressType{
    Invalid,
    IPv4,
    IPv6,
};

NLOHMANN_JSON_SERIALIZE_ENUM(NetworkDeviceFunction_IPAddressType, {
    {NetworkDeviceFunction_IPAddressType::Invalid, "Invalid"},
    {NetworkDeviceFunction_IPAddressType::IPv4, "IPv4"},
    {NetworkDeviceFunction_IPAddressType::IPv6, "IPv6"},
});

enum class NetworkDeviceFunction_NetworkDeviceTechnology{
    Invalid,
    Disabled,
    Ethernet,
    FibreChannel,
    iSCSI,
    FibreChannelOverEthernet,
    InfiniBand,
};

NLOHMANN_JSON_SERIALIZE_ENUM(NetworkDeviceFunction_NetworkDeviceTechnology, {
    {NetworkDeviceFunction_NetworkDeviceTechnology::Invalid, "Invalid"},
    {NetworkDeviceFunction_NetworkDeviceTechnology::Disabled, "Disabled"},
    {NetworkDeviceFunction_NetworkDeviceTechnology::Ethernet, "Ethernet"},
    {NetworkDeviceFunction_NetworkDeviceTechnology::FibreChannel, "FibreChannel"},
    {NetworkDeviceFunction_NetworkDeviceTechnology::iSCSI, "iSCSI"},
    {NetworkDeviceFunction_NetworkDeviceTechnology::FibreChannelOverEthernet, "FibreChannelOverEthernet"},
    {NetworkDeviceFunction_NetworkDeviceTechnology::InfiniBand, "InfiniBand"},
});

enum class NetworkDeviceFunction_WWNSource{
    Invalid,
    ConfiguredLocally,
    ProvidedByFabric,
};

NLOHMANN_JSON_SERIALIZE_ENUM(NetworkDeviceFunction_WWNSource, {
    {NetworkDeviceFunction_WWNSource::Invalid, "Invalid"},
    {NetworkDeviceFunction_WWNSource::ConfiguredLocally, "ConfiguredLocally"},
    {NetworkDeviceFunction_WWNSource::ProvidedByFabric, "ProvidedByFabric"},
});

enum class NetworkDeviceFunction_DataDirection{
    Invalid,
    None,
    Ingress,
    Egress,
};

NLOHMANN_JSON_SERIALIZE_ENUM(NetworkDeviceFunction_DataDirection, {
    {NetworkDeviceFunction_DataDirection::Invalid, "Invalid"},
    {NetworkDeviceFunction_DataDirection::None, "None"},
    {NetworkDeviceFunction_DataDirection::Ingress, "Ingress"},
    {NetworkDeviceFunction_DataDirection::Egress, "Egress"},
});

enum class NetworkPort_FlowControl{
    Invalid,
    None,
    TX,
    RX,
    TX_RX,
};

NLOHMANN_JSON_SERIALIZE_ENUM(NetworkPort_FlowControl, {
    {NetworkPort_FlowControl::Invalid, "Invalid"},
    {NetworkPort_FlowControl::None, "None"},
    {NetworkPort_FlowControl::TX, "TX"},
    {NetworkPort_FlowControl::RX, "RX"},
    {NetworkPort_FlowControl::TX_RX, "TX_RX"},
});

enum class NetworkPort_LinkNetworkTechnology{
    Invalid,
    Ethernet,
    InfiniBand,
    FibreChannel,
};

NLOHMANN_JSON_SERIALIZE_ENUM(NetworkPort_LinkNetworkTechnology, {
    {NetworkPort_LinkNetworkTechnology::Invalid, "Invalid"},
    {NetworkPort_LinkNetworkTechnology::Ethernet, "Ethernet"},
    {NetworkPort_LinkNetworkTechnology::InfiniBand, "InfiniBand"},
    {NetworkPort_LinkNetworkTechnology::FibreChannel, "FibreChannel"},
});

enum class NetworkPort_LinkStatus{
    Invalid,
    Down,
    Up,
    Starting,
    Training,
};

NLOHMANN_JSON_SERIALIZE_ENUM(NetworkPort_LinkStatus, {
    {NetworkPort_LinkStatus::Invalid, "Invalid"},
    {NetworkPort_LinkStatus::Down, "Down"},
    {NetworkPort_LinkStatus::Up, "Up"},
    {NetworkPort_LinkStatus::Starting, "Starting"},
    {NetworkPort_LinkStatus::Training, "Training"},
});

enum class NetworkPort_SupportedEthernetCapabilities{
    Invalid,
    WakeOnLAN,
    EEE,
};

NLOHMANN_JSON_SERIALIZE_ENUM(NetworkPort_SupportedEthernetCapabilities, {
    {NetworkPort_SupportedEthernetCapabilities::Invalid, "Invalid"},
    {NetworkPort_SupportedEthernetCapabilities::WakeOnLAN, "WakeOnLAN"},
    {NetworkPort_SupportedEthernetCapabilities::EEE, "EEE"},
});

enum class NetworkPort_PortConnectionType{
    Invalid,
    NotConnected,
    NPort,
    PointToPoint,
    PrivateLoop,
    PublicLoop,
    Generic,
    ExtenderFabric,
};

NLOHMANN_JSON_SERIALIZE_ENUM(NetworkPort_PortConnectionType, {
    {NetworkPort_PortConnectionType::Invalid, "Invalid"},
    {NetworkPort_PortConnectionType::NotConnected, "NotConnected"},
    {NetworkPort_PortConnectionType::NPort, "NPort"},
    {NetworkPort_PortConnectionType::PointToPoint, "PointToPoint"},
    {NetworkPort_PortConnectionType::PrivateLoop, "PrivateLoop"},
    {NetworkPort_PortConnectionType::PublicLoop, "PublicLoop"},
    {NetworkPort_PortConnectionType::Generic, "Generic"},
    {NetworkPort_PortConnectionType::ExtenderFabric, "ExtenderFabric"},
});

enum class OemComputerSystem_FirmwareProvisioningStatus{
    Invalid,
    NotProvisioned,
    ProvisionedButNotLocked,
    ProvisionedAndLocked,
};

NLOHMANN_JSON_SERIALIZE_ENUM(OemComputerSystem_FirmwareProvisioningStatus, {
    {OemComputerSystem_FirmwareProvisioningStatus::Invalid, "Invalid"},
    {OemComputerSystem_FirmwareProvisioningStatus::NotProvisioned, "NotProvisioned"},
    {OemComputerSystem_FirmwareProvisioningStatus::ProvisionedButNotLocked, "ProvisionedButNotLocked"},
    {OemComputerSystem_FirmwareProvisioningStatus::ProvisionedAndLocked, "ProvisionedAndLocked"},
});

enum class Org_ConformanceLevelType{
    Invalid,
    Minimal,
    Intermediate,
    Advanced,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Org_ConformanceLevelType, {
    {Org_ConformanceLevelType::Invalid, "Invalid"},
    {Org_ConformanceLevelType::Minimal, "Minimal"},
    {Org_ConformanceLevelType::Intermediate, "Intermediate"},
    {Org_ConformanceLevelType::Advanced, "Advanced"},
});

enum class Org_FilterExpressionType{
    Invalid,
    SingleValue,
    MultiValue,
    SingleInterval,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Org_FilterExpressionType, {
    {Org_FilterExpressionType::Invalid, "Invalid"},
    {Org_FilterExpressionType::SingleValue, "SingleValue"},
    {Org_FilterExpressionType::MultiValue, "MultiValue"},
    {Org_FilterExpressionType::SingleInterval, "SingleInterval"},
});

enum class Org_IsolationLevel{
    Invalid,
    Snapshot,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Org_IsolationLevel, {
    {Org_IsolationLevel::Invalid, "Invalid"},
    {Org_IsolationLevel::Snapshot, "Snapshot"},
});

enum class Org_NavigationType{
    Invalid,
    Recursive,
    Single,
    None,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Org_NavigationType, {
    {Org_NavigationType::Invalid, "Invalid"},
    {Org_NavigationType::Recursive, "Recursive"},
    {Org_NavigationType::Single, "Single"},
    {Org_NavigationType::None, "None"},
});

enum class Org_SearchExpressions{
    Invalid,
    none,
    AND,
    OR,
    NOT,
    phrase,
    group,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Org_SearchExpressions, {
    {Org_SearchExpressions::Invalid, "Invalid"},
    {Org_SearchExpressions::none, "none"},
    {Org_SearchExpressions::AND, "AND"},
    {Org_SearchExpressions::OR, "OR"},
    {Org_SearchExpressions::NOT, "NOT"},
    {Org_SearchExpressions::phrase, "phrase"},
    {Org_SearchExpressions::group, "group"},
});

enum class Org_Permission{
    Invalid,
    None,
    Read,
    Write,
    ReadWrite,
    Invoke,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Org_Permission, {
    {Org_Permission::Invalid, "Invalid"},
    {Org_Permission::None, "None"},
    {Org_Permission::Read, "Read"},
    {Org_Permission::Write, "Write"},
    {Org_Permission::ReadWrite, "ReadWrite"},
    {Org_Permission::Invoke, "Invoke"},
});

enum class Outlet_VoltageType{
    Invalid,
    AC,
    DC,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Outlet_VoltageType, {
    {Outlet_VoltageType::Invalid, "Invalid"},
    {Outlet_VoltageType::AC, "AC"},
    {Outlet_VoltageType::DC, "DC"},
});

enum class OutletGroup_PowerState{
    Invalid,
    On,
    Off,
    PowerCycle,
};

NLOHMANN_JSON_SERIALIZE_ENUM(OutletGroup_PowerState, {
    {OutletGroup_PowerState::Invalid, "Invalid"},
    {OutletGroup_PowerState::On, "On"},
    {OutletGroup_PowerState::Off, "Off"},
    {OutletGroup_PowerState::PowerCycle, "PowerCycle"},
});

enum class Outlet_PowerState{
    Invalid,
    On,
    Off,
    PowerCycle,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Outlet_PowerState, {
    {Outlet_PowerState::Invalid, "Invalid"},
    {Outlet_PowerState::On, "On"},
    {Outlet_PowerState::Off, "Off"},
    {Outlet_PowerState::PowerCycle, "PowerCycle"},
});

enum class Outlet_ReceptacleType{
    Invalid,
    NEMA_5_15R,
    NEMA_5_20R,
    NEMA_L5_20R,
    NEMA_L5_30R,
    NEMA_L6_20R,
    NEMA_L6_30R,
    IEC_60320_C13,
    IEC_60320_C19,
    CEE_7_Type_E,
    CEE_7_Type_F,
    SEV_1011_TYPE_12,
    SEV_1011_TYPE_23,
    BS_1363_Type_G,
    BusConnection,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Outlet_ReceptacleType, {
    {Outlet_ReceptacleType::Invalid, "Invalid"},
    {Outlet_ReceptacleType::NEMA_5_15R, "NEMA_5_15R"},
    {Outlet_ReceptacleType::NEMA_5_20R, "NEMA_5_20R"},
    {Outlet_ReceptacleType::NEMA_L5_20R, "NEMA_L5_20R"},
    {Outlet_ReceptacleType::NEMA_L5_30R, "NEMA_L5_30R"},
    {Outlet_ReceptacleType::NEMA_L6_20R, "NEMA_L6_20R"},
    {Outlet_ReceptacleType::NEMA_L6_30R, "NEMA_L6_30R"},
    {Outlet_ReceptacleType::IEC_60320_C13, "IEC_60320_C13"},
    {Outlet_ReceptacleType::IEC_60320_C19, "IEC_60320_C19"},
    {Outlet_ReceptacleType::CEE_7_Type_E, "CEE_7_Type_E"},
    {Outlet_ReceptacleType::CEE_7_Type_F, "CEE_7_Type_F"},
    {Outlet_ReceptacleType::SEV_1011_TYPE_12, "SEV_1011_TYPE_12"},
    {Outlet_ReceptacleType::SEV_1011_TYPE_23, "SEV_1011_TYPE_23"},
    {Outlet_ReceptacleType::BS_1363_Type_G, "BS_1363_Type_G"},
    {Outlet_ReceptacleType::BusConnection, "BusConnection"},
});

enum class PCIeDevice_DeviceType{
    Invalid,
    SingleFunction,
    MultiFunction,
    Simulated,
};

NLOHMANN_JSON_SERIALIZE_ENUM(PCIeDevice_DeviceType, {
    {PCIeDevice_DeviceType::Invalid, "Invalid"},
    {PCIeDevice_DeviceType::SingleFunction, "SingleFunction"},
    {PCIeDevice_DeviceType::MultiFunction, "MultiFunction"},
    {PCIeDevice_DeviceType::Simulated, "Simulated"},
});

enum class PCIeDevice_LaneSplittingType{
    Invalid,
    None,
    Bridged,
    Bifurcated,
};

NLOHMANN_JSON_SERIALIZE_ENUM(PCIeDevice_LaneSplittingType, {
    {PCIeDevice_LaneSplittingType::Invalid, "Invalid"},
    {PCIeDevice_LaneSplittingType::None, "None"},
    {PCIeDevice_LaneSplittingType::Bridged, "Bridged"},
    {PCIeDevice_LaneSplittingType::Bifurcated, "Bifurcated"},
});

enum class PCIeDevice_SlotType{
    Invalid,
    FullLength,
    HalfLength,
    LowProfile,
    Mini,
    M2,
    OEM,
    OCP3Small,
    OCP3Large,
    U2,
};

NLOHMANN_JSON_SERIALIZE_ENUM(PCIeDevice_SlotType, {
    {PCIeDevice_SlotType::Invalid, "Invalid"},
    {PCIeDevice_SlotType::FullLength, "FullLength"},
    {PCIeDevice_SlotType::HalfLength, "HalfLength"},
    {PCIeDevice_SlotType::LowProfile, "LowProfile"},
    {PCIeDevice_SlotType::Mini, "Mini"},
    {PCIeDevice_SlotType::M2, "M2"},
    {PCIeDevice_SlotType::OEM, "OEM"},
    {PCIeDevice_SlotType::OCP3Small, "OCP3Small"},
    {PCIeDevice_SlotType::OCP3Large, "OCP3Large"},
    {PCIeDevice_SlotType::U2, "U2"},
});

enum class PCIeDevice_PCIeTypes{
    Invalid,
    Gen1,
    Gen2,
    Gen3,
    Gen4,
    Gen5,
};

NLOHMANN_JSON_SERIALIZE_ENUM(PCIeDevice_PCIeTypes, {
    {PCIeDevice_PCIeTypes::Invalid, "Invalid"},
    {PCIeDevice_PCIeTypes::Gen1, "Gen1"},
    {PCIeDevice_PCIeTypes::Gen2, "Gen2"},
    {PCIeDevice_PCIeTypes::Gen3, "Gen3"},
    {PCIeDevice_PCIeTypes::Gen4, "Gen4"},
    {PCIeDevice_PCIeTypes::Gen5, "Gen5"},
});

enum class PCIeFunction_DeviceClass{
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

NLOHMANN_JSON_SERIALIZE_ENUM(PCIeFunction_DeviceClass, {
    {PCIeFunction_DeviceClass::Invalid, "Invalid"},
    {PCIeFunction_DeviceClass::UnclassifiedDevice, "UnclassifiedDevice"},
    {PCIeFunction_DeviceClass::MassStorageController, "MassStorageController"},
    {PCIeFunction_DeviceClass::NetworkController, "NetworkController"},
    {PCIeFunction_DeviceClass::DisplayController, "DisplayController"},
    {PCIeFunction_DeviceClass::MultimediaController, "MultimediaController"},
    {PCIeFunction_DeviceClass::MemoryController, "MemoryController"},
    {PCIeFunction_DeviceClass::Bridge, "Bridge"},
    {PCIeFunction_DeviceClass::CommunicationController, "CommunicationController"},
    {PCIeFunction_DeviceClass::GenericSystemPeripheral, "GenericSystemPeripheral"},
    {PCIeFunction_DeviceClass::InputDeviceController, "InputDeviceController"},
    {PCIeFunction_DeviceClass::DockingStation, "DockingStation"},
    {PCIeFunction_DeviceClass::Processor, "Processor"},
    {PCIeFunction_DeviceClass::SerialBusController, "SerialBusController"},
    {PCIeFunction_DeviceClass::WirelessController, "WirelessController"},
    {PCIeFunction_DeviceClass::IntelligentController, "IntelligentController"},
    {PCIeFunction_DeviceClass::SatelliteCommunicationsController, "SatelliteCommunicationsController"},
    {PCIeFunction_DeviceClass::EncryptionController, "EncryptionController"},
    {PCIeFunction_DeviceClass::SignalProcessingController, "SignalProcessingController"},
    {PCIeFunction_DeviceClass::ProcessingAccelerators, "ProcessingAccelerators"},
    {PCIeFunction_DeviceClass::NonEssentialInstrumentation, "NonEssentialInstrumentation"},
    {PCIeFunction_DeviceClass::Coprocessor, "Coprocessor"},
    {PCIeFunction_DeviceClass::UnassignedClass, "UnassignedClass"},
    {PCIeFunction_DeviceClass::Other, "Other"},
});

enum class PCIeFunction_FunctionType{
    Invalid,
    Physical,
    Virtual,
};

NLOHMANN_JSON_SERIALIZE_ENUM(PCIeFunction_FunctionType, {
    {PCIeFunction_FunctionType::Invalid, "Invalid"},
    {PCIeFunction_FunctionType::Physical, "Physical"},
    {PCIeFunction_FunctionType::Virtual, "Virtual"},
});

enum class PCIeSlots_SlotTypes{
    Invalid,
    FullLength,
    HalfLength,
    LowProfile,
    Mini,
    M2,
    OEM,
    OCP3Small,
    OCP3Large,
    U2,
};

NLOHMANN_JSON_SERIALIZE_ENUM(PCIeSlots_SlotTypes, {
    {PCIeSlots_SlotTypes::Invalid, "Invalid"},
    {PCIeSlots_SlotTypes::FullLength, "FullLength"},
    {PCIeSlots_SlotTypes::HalfLength, "HalfLength"},
    {PCIeSlots_SlotTypes::LowProfile, "LowProfile"},
    {PCIeSlots_SlotTypes::Mini, "Mini"},
    {PCIeSlots_SlotTypes::M2, "M2"},
    {PCIeSlots_SlotTypes::OEM, "OEM"},
    {PCIeSlots_SlotTypes::OCP3Small, "OCP3Small"},
    {PCIeSlots_SlotTypes::OCP3Large, "OCP3Large"},
    {PCIeSlots_SlotTypes::U2, "U2"},
});

enum class PhysicalContext_PhysicalContext{
    Invalid,
    Room,
    Intake,
    Exhaust,
    LiquidInlet,
    LiquidOutlet,
    Front,
    Back,
    Upper,
    Lower,
    CPU,
    CPUSubsystem,
    GPU,
    GPUSubsystem,
    FPGA,
    Accelerator,
    ASIC,
    Backplane,
    SystemBoard,
    PowerSupply,
    PowerSubsystem,
    VoltageRegulator,
    Rectifier,
    StorageDevice,
    NetworkingDevice,
    ComputeBay,
    StorageBay,
    NetworkBay,
    ExpansionBay,
    PowerSupplyBay,
    Memory,
    MemorySubsystem,
    Chassis,
    Fan,
    CoolingSubsystem,
    Motor,
    Transformer,
    ACUtilityInput,
    ACStaticBypassInput,
    ACMaintenanceBypassInput,
    DCBus,
    ACOutput,
    ACInput,
    TrustedModule,
    Board,
    Transceiver,
    Battery,
    Pump,
};

NLOHMANN_JSON_SERIALIZE_ENUM(PhysicalContext_PhysicalContext, {
    {PhysicalContext_PhysicalContext::Invalid, "Invalid"},
    {PhysicalContext_PhysicalContext::Room, "Room"},
    {PhysicalContext_PhysicalContext::Intake, "Intake"},
    {PhysicalContext_PhysicalContext::Exhaust, "Exhaust"},
    {PhysicalContext_PhysicalContext::LiquidInlet, "LiquidInlet"},
    {PhysicalContext_PhysicalContext::LiquidOutlet, "LiquidOutlet"},
    {PhysicalContext_PhysicalContext::Front, "Front"},
    {PhysicalContext_PhysicalContext::Back, "Back"},
    {PhysicalContext_PhysicalContext::Upper, "Upper"},
    {PhysicalContext_PhysicalContext::Lower, "Lower"},
    {PhysicalContext_PhysicalContext::CPU, "CPU"},
    {PhysicalContext_PhysicalContext::CPUSubsystem, "CPUSubsystem"},
    {PhysicalContext_PhysicalContext::GPU, "GPU"},
    {PhysicalContext_PhysicalContext::GPUSubsystem, "GPUSubsystem"},
    {PhysicalContext_PhysicalContext::FPGA, "FPGA"},
    {PhysicalContext_PhysicalContext::Accelerator, "Accelerator"},
    {PhysicalContext_PhysicalContext::ASIC, "ASIC"},
    {PhysicalContext_PhysicalContext::Backplane, "Backplane"},
    {PhysicalContext_PhysicalContext::SystemBoard, "SystemBoard"},
    {PhysicalContext_PhysicalContext::PowerSupply, "PowerSupply"},
    {PhysicalContext_PhysicalContext::PowerSubsystem, "PowerSubsystem"},
    {PhysicalContext_PhysicalContext::VoltageRegulator, "VoltageRegulator"},
    {PhysicalContext_PhysicalContext::Rectifier, "Rectifier"},
    {PhysicalContext_PhysicalContext::StorageDevice, "StorageDevice"},
    {PhysicalContext_PhysicalContext::NetworkingDevice, "NetworkingDevice"},
    {PhysicalContext_PhysicalContext::ComputeBay, "ComputeBay"},
    {PhysicalContext_PhysicalContext::StorageBay, "StorageBay"},
    {PhysicalContext_PhysicalContext::NetworkBay, "NetworkBay"},
    {PhysicalContext_PhysicalContext::ExpansionBay, "ExpansionBay"},
    {PhysicalContext_PhysicalContext::PowerSupplyBay, "PowerSupplyBay"},
    {PhysicalContext_PhysicalContext::Memory, "Memory"},
    {PhysicalContext_PhysicalContext::MemorySubsystem, "MemorySubsystem"},
    {PhysicalContext_PhysicalContext::Chassis, "Chassis"},
    {PhysicalContext_PhysicalContext::Fan, "Fan"},
    {PhysicalContext_PhysicalContext::CoolingSubsystem, "CoolingSubsystem"},
    {PhysicalContext_PhysicalContext::Motor, "Motor"},
    {PhysicalContext_PhysicalContext::Transformer, "Transformer"},
    {PhysicalContext_PhysicalContext::ACUtilityInput, "ACUtilityInput"},
    {PhysicalContext_PhysicalContext::ACStaticBypassInput, "ACStaticBypassInput"},
    {PhysicalContext_PhysicalContext::ACMaintenanceBypassInput, "ACMaintenanceBypassInput"},
    {PhysicalContext_PhysicalContext::DCBus, "DCBus"},
    {PhysicalContext_PhysicalContext::ACOutput, "ACOutput"},
    {PhysicalContext_PhysicalContext::ACInput, "ACInput"},
    {PhysicalContext_PhysicalContext::TrustedModule, "TrustedModule"},
    {PhysicalContext_PhysicalContext::Board, "Board"},
    {PhysicalContext_PhysicalContext::Transceiver, "Transceiver"},
    {PhysicalContext_PhysicalContext::Battery, "Battery"},
    {PhysicalContext_PhysicalContext::Pump, "Pump"},
});

enum class PhysicalContext_PhysicalSubContext{
    Invalid,
    Input,
    Output,
};

NLOHMANN_JSON_SERIALIZE_ENUM(PhysicalContext_PhysicalSubContext, {
    {PhysicalContext_PhysicalSubContext::Invalid, "Invalid"},
    {PhysicalContext_PhysicalSubContext::Input, "Input"},
    {PhysicalContext_PhysicalSubContext::Output, "Output"},
});

enum class Port_PortType{
    Invalid,
    UpstreamPort,
    DownstreamPort,
    InterswitchPort,
    ManagementPort,
    BidirectionalPort,
    UnconfiguredPort,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Port_PortType, {
    {Port_PortType::Invalid, "Invalid"},
    {Port_PortType::UpstreamPort, "UpstreamPort"},
    {Port_PortType::DownstreamPort, "DownstreamPort"},
    {Port_PortType::InterswitchPort, "InterswitchPort"},
    {Port_PortType::ManagementPort, "ManagementPort"},
    {Port_PortType::BidirectionalPort, "BidirectionalPort"},
    {Port_PortType::UnconfiguredPort, "UnconfiguredPort"},
});

enum class Port_LinkNetworkTechnology{
    Invalid,
    Ethernet,
    InfiniBand,
    FibreChannel,
    GenZ,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Port_LinkNetworkTechnology, {
    {Port_LinkNetworkTechnology::Invalid, "Invalid"},
    {Port_LinkNetworkTechnology::Ethernet, "Ethernet"},
    {Port_LinkNetworkTechnology::InfiniBand, "InfiniBand"},
    {Port_LinkNetworkTechnology::FibreChannel, "FibreChannel"},
    {Port_LinkNetworkTechnology::GenZ, "GenZ"},
});

enum class Port_LinkState{
    Invalid,
    Enabled,
    Disabled,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Port_LinkState, {
    {Port_LinkState::Invalid, "Invalid"},
    {Port_LinkState::Enabled, "Enabled"},
    {Port_LinkState::Disabled, "Disabled"},
});

enum class Port_LinkStatus{
    Invalid,
    LinkUp,
    Starting,
    Training,
    LinkDown,
    NoLink,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Port_LinkStatus, {
    {Port_LinkStatus::Invalid, "Invalid"},
    {Port_LinkStatus::LinkUp, "LinkUp"},
    {Port_LinkStatus::Starting, "Starting"},
    {Port_LinkStatus::Training, "Training"},
    {Port_LinkStatus::LinkDown, "LinkDown"},
    {Port_LinkStatus::NoLink, "NoLink"},
});

enum class Port_PortMedium{
    Invalid,
    Electrical,
    Optical,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Port_PortMedium, {
    {Port_PortMedium::Invalid, "Invalid"},
    {Port_PortMedium::Electrical, "Electrical"},
    {Port_PortMedium::Optical, "Optical"},
});

enum class Port_FlowControl{
    Invalid,
    None,
    TX,
    RX,
    TX_RX,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Port_FlowControl, {
    {Port_FlowControl::Invalid, "Invalid"},
    {Port_FlowControl::None, "None"},
    {Port_FlowControl::TX, "TX"},
    {Port_FlowControl::RX, "RX"},
    {Port_FlowControl::TX_RX, "TX_RX"},
});

enum class Port_PortConnectionType{
    Invalid,
    NotConnected,
    NPort,
    PointToPoint,
    PrivateLoop,
    PublicLoop,
    Generic,
    ExtenderFabric,
    FPort,
    EPort,
    TEPort,
    NPPort,
    GPort,
    NLPort,
    FLPort,
    EXPort,
    UPort,
    DPort,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Port_PortConnectionType, {
    {Port_PortConnectionType::Invalid, "Invalid"},
    {Port_PortConnectionType::NotConnected, "NotConnected"},
    {Port_PortConnectionType::NPort, "NPort"},
    {Port_PortConnectionType::PointToPoint, "PointToPoint"},
    {Port_PortConnectionType::PrivateLoop, "PrivateLoop"},
    {Port_PortConnectionType::PublicLoop, "PublicLoop"},
    {Port_PortConnectionType::Generic, "Generic"},
    {Port_PortConnectionType::ExtenderFabric, "ExtenderFabric"},
    {Port_PortConnectionType::FPort, "FPort"},
    {Port_PortConnectionType::EPort, "EPort"},
    {Port_PortConnectionType::TEPort, "TEPort"},
    {Port_PortConnectionType::NPPort, "NPPort"},
    {Port_PortConnectionType::GPort, "GPort"},
    {Port_PortConnectionType::NLPort, "NLPort"},
    {Port_PortConnectionType::FLPort, "FLPort"},
    {Port_PortConnectionType::EXPort, "EXPort"},
    {Port_PortConnectionType::UPort, "UPort"},
    {Port_PortConnectionType::DPort, "DPort"},
});

enum class Port_SupportedEthernetCapabilities{
    Invalid,
    WakeOnLAN,
    EEE,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Port_SupportedEthernetCapabilities, {
    {Port_SupportedEthernetCapabilities::Invalid, "Invalid"},
    {Port_SupportedEthernetCapabilities::WakeOnLAN, "WakeOnLAN"},
    {Port_SupportedEthernetCapabilities::EEE, "EEE"},
});

enum class Port_FiberConnectionType{
    Invalid,
    SingleMode,
    MultiMode,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Port_FiberConnectionType, {
    {Port_FiberConnectionType::Invalid, "Invalid"},
    {Port_FiberConnectionType::SingleMode, "SingleMode"},
    {Port_FiberConnectionType::MultiMode, "MultiMode"},
});

enum class Port_IEEE802IdSubtype{
    Invalid,
    ChassisComp,
    IfAlias,
    PortComp,
    MacAddr,
    NetworkAddr,
    IfName,
    AgentId,
    LocalAssign,
    NotTransmitted,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Port_IEEE802IdSubtype, {
    {Port_IEEE802IdSubtype::Invalid, "Invalid"},
    {Port_IEEE802IdSubtype::ChassisComp, "ChassisComp"},
    {Port_IEEE802IdSubtype::IfAlias, "IfAlias"},
    {Port_IEEE802IdSubtype::PortComp, "PortComp"},
    {Port_IEEE802IdSubtype::MacAddr, "MacAddr"},
    {Port_IEEE802IdSubtype::NetworkAddr, "NetworkAddr"},
    {Port_IEEE802IdSubtype::IfName, "IfName"},
    {Port_IEEE802IdSubtype::AgentId, "AgentId"},
    {Port_IEEE802IdSubtype::LocalAssign, "LocalAssign"},
    {Port_IEEE802IdSubtype::NotTransmitted, "NotTransmitted"},
});

enum class Port_MediumType{
    Invalid,
    Copper,
    FiberOptic,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Port_MediumType, {
    {Port_MediumType::Invalid, "Invalid"},
    {Port_MediumType::Copper, "Copper"},
    {Port_MediumType::FiberOptic, "FiberOptic"},
});

enum class Port_SFPType{
    Invalid,
    SFP,
    SFPPlus,
    SFP28,
    cSFP,
    SFPDD,
    QSFP,
    QSFPPlus,
    QSFP14,
    QSFP28,
    QSFP56,
    MiniSASHD,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Port_SFPType, {
    {Port_SFPType::Invalid, "Invalid"},
    {Port_SFPType::SFP, "SFP"},
    {Port_SFPType::SFPPlus, "SFPPlus"},
    {Port_SFPType::SFP28, "SFP28"},
    {Port_SFPType::cSFP, "cSFP"},
    {Port_SFPType::SFPDD, "SFPDD"},
    {Port_SFPType::QSFP, "QSFP"},
    {Port_SFPType::QSFPPlus, "QSFPPlus"},
    {Port_SFPType::QSFP14, "QSFP14"},
    {Port_SFPType::QSFP28, "QSFP28"},
    {Port_SFPType::QSFP56, "QSFP56"},
    {Port_SFPType::MiniSASHD, "MiniSASHD"},
});

enum class Power_LineInputVoltageType{
    Invalid,
    Unknown,
    ACLowLine,
    ACMidLine,
    ACHighLine,
    DCNeg48V,
    DC380V,
    AC120V,
    AC240V,
    AC277V,
    ACandDCWideRange,
    ACWideRange,
    DC240V,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Power_LineInputVoltageType, {
    {Power_LineInputVoltageType::Invalid, "Invalid"},
    {Power_LineInputVoltageType::Unknown, "Unknown"},
    {Power_LineInputVoltageType::ACLowLine, "ACLowLine"},
    {Power_LineInputVoltageType::ACMidLine, "ACMidLine"},
    {Power_LineInputVoltageType::ACHighLine, "ACHighLine"},
    {Power_LineInputVoltageType::DCNeg48V, "DCNeg48V"},
    {Power_LineInputVoltageType::DC380V, "DC380V"},
    {Power_LineInputVoltageType::AC120V, "AC120V"},
    {Power_LineInputVoltageType::AC240V, "AC240V"},
    {Power_LineInputVoltageType::AC277V, "AC277V"},
    {Power_LineInputVoltageType::ACandDCWideRange, "ACandDCWideRange"},
    {Power_LineInputVoltageType::ACWideRange, "ACWideRange"},
    {Power_LineInputVoltageType::DC240V, "DC240V"},
});

enum class Power_PowerLimitException{
    Invalid,
    NoAction,
    HardPowerOff,
    LogEventOnly,
    Oem,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Power_PowerLimitException, {
    {Power_PowerLimitException::Invalid, "Invalid"},
    {Power_PowerLimitException::NoAction, "NoAction"},
    {Power_PowerLimitException::HardPowerOff, "HardPowerOff"},
    {Power_PowerLimitException::LogEventOnly, "LogEventOnly"},
    {Power_PowerLimitException::Oem, "Oem"},
});

enum class Power_PowerSupplyType{
    Invalid,
    Unknown,
    AC,
    DC,
    ACorDC,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Power_PowerSupplyType, {
    {Power_PowerSupplyType::Invalid, "Invalid"},
    {Power_PowerSupplyType::Unknown, "Unknown"},
    {Power_PowerSupplyType::AC, "AC"},
    {Power_PowerSupplyType::DC, "DC"},
    {Power_PowerSupplyType::ACorDC, "ACorDC"},
});

enum class Power_InputType{
    Invalid,
    AC,
    DC,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Power_InputType, {
    {Power_InputType::Invalid, "Invalid"},
    {Power_InputType::AC, "AC"},
    {Power_InputType::DC, "DC"},
});

enum class PowerDistribution_PowerEquipmentType{
    Invalid,
    RackPDU,
    FloorPDU,
    ManualTransferSwitch,
    AutomaticTransferSwitch,
    Switchgear,
    PowerShelf,
    Bus,
};

NLOHMANN_JSON_SERIALIZE_ENUM(PowerDistribution_PowerEquipmentType, {
    {PowerDistribution_PowerEquipmentType::Invalid, "Invalid"},
    {PowerDistribution_PowerEquipmentType::RackPDU, "RackPDU"},
    {PowerDistribution_PowerEquipmentType::FloorPDU, "FloorPDU"},
    {PowerDistribution_PowerEquipmentType::ManualTransferSwitch, "ManualTransferSwitch"},
    {PowerDistribution_PowerEquipmentType::AutomaticTransferSwitch, "AutomaticTransferSwitch"},
    {PowerDistribution_PowerEquipmentType::Switchgear, "Switchgear"},
    {PowerDistribution_PowerEquipmentType::PowerShelf, "PowerShelf"},
    {PowerDistribution_PowerEquipmentType::Bus, "Bus"},
});

enum class PowerDistribution_TransferSensitivityType{
    Invalid,
    High,
    Medium,
    Low,
};

NLOHMANN_JSON_SERIALIZE_ENUM(PowerDistribution_TransferSensitivityType, {
    {PowerDistribution_TransferSensitivityType::Invalid, "Invalid"},
    {PowerDistribution_TransferSensitivityType::High, "High"},
    {PowerDistribution_TransferSensitivityType::Medium, "Medium"},
    {PowerDistribution_TransferSensitivityType::Low, "Low"},
});

enum class PowerSupply_PowerSupplyType{
    Invalid,
    AC,
    DC,
    ACorDC,
};

NLOHMANN_JSON_SERIALIZE_ENUM(PowerSupply_PowerSupplyType, {
    {PowerSupply_PowerSupplyType::Invalid, "Invalid"},
    {PowerSupply_PowerSupplyType::AC, "AC"},
    {PowerSupply_PowerSupplyType::DC, "DC"},
    {PowerSupply_PowerSupplyType::ACorDC, "ACorDC"},
});

enum class PowerSupply_LineStatus{
    Invalid,
    Normal,
    LossOfInput,
    OutOfRange,
};

NLOHMANN_JSON_SERIALIZE_ENUM(PowerSupply_LineStatus, {
    {PowerSupply_LineStatus::Invalid, "Invalid"},
    {PowerSupply_LineStatus::Normal, "Normal"},
    {PowerSupply_LineStatus::LossOfInput, "LossOfInput"},
    {PowerSupply_LineStatus::OutOfRange, "OutOfRange"},
});

enum class Privileges_PrivilegeType{
    Invalid,
    Login,
    ConfigureManager,
    ConfigureUsers,
    ConfigureSelf,
    ConfigureComponents,
    NoAuth,
    ConfigureCompositionInfrastructure,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Privileges_PrivilegeType, {
    {Privileges_PrivilegeType::Invalid, "Invalid"},
    {Privileges_PrivilegeType::Login, "Login"},
    {Privileges_PrivilegeType::ConfigureManager, "ConfigureManager"},
    {Privileges_PrivilegeType::ConfigureUsers, "ConfigureUsers"},
    {Privileges_PrivilegeType::ConfigureSelf, "ConfigureSelf"},
    {Privileges_PrivilegeType::ConfigureComponents, "ConfigureComponents"},
    {Privileges_PrivilegeType::NoAuth, "NoAuth"},
    {Privileges_PrivilegeType::ConfigureCompositionInfrastructure, "ConfigureCompositionInfrastructure"},
});

enum class Processor_ProcessorType{
    Invalid,
    CPU,
    GPU,
    FPGA,
    DSP,
    Accelerator,
    Core,
    Thread,
    OEM,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Processor_ProcessorType, {
    {Processor_ProcessorType::Invalid, "Invalid"},
    {Processor_ProcessorType::CPU, "CPU"},
    {Processor_ProcessorType::GPU, "GPU"},
    {Processor_ProcessorType::FPGA, "FPGA"},
    {Processor_ProcessorType::DSP, "DSP"},
    {Processor_ProcessorType::Accelerator, "Accelerator"},
    {Processor_ProcessorType::Core, "Core"},
    {Processor_ProcessorType::Thread, "Thread"},
    {Processor_ProcessorType::OEM, "OEM"},
});

enum class Processor_FpgaType{
    Invalid,
    Integrated,
    Discrete,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Processor_FpgaType, {
    {Processor_FpgaType::Invalid, "Invalid"},
    {Processor_FpgaType::Integrated, "Integrated"},
    {Processor_FpgaType::Discrete, "Discrete"},
});

enum class Processor_ProcessorMemoryType{
    Invalid,
    L1Cache,
    L2Cache,
    L3Cache,
    L4Cache,
    L5Cache,
    L6Cache,
    L7Cache,
    HBM1,
    HBM2,
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

NLOHMANN_JSON_SERIALIZE_ENUM(Processor_ProcessorMemoryType, {
    {Processor_ProcessorMemoryType::Invalid, "Invalid"},
    {Processor_ProcessorMemoryType::L1Cache, "L1Cache"},
    {Processor_ProcessorMemoryType::L2Cache, "L2Cache"},
    {Processor_ProcessorMemoryType::L3Cache, "L3Cache"},
    {Processor_ProcessorMemoryType::L4Cache, "L4Cache"},
    {Processor_ProcessorMemoryType::L5Cache, "L5Cache"},
    {Processor_ProcessorMemoryType::L6Cache, "L6Cache"},
    {Processor_ProcessorMemoryType::L7Cache, "L7Cache"},
    {Processor_ProcessorMemoryType::HBM1, "HBM1"},
    {Processor_ProcessorMemoryType::HBM2, "HBM2"},
    {Processor_ProcessorMemoryType::HBM3, "HBM3"},
    {Processor_ProcessorMemoryType::SGRAM, "SGRAM"},
    {Processor_ProcessorMemoryType::GDDR, "GDDR"},
    {Processor_ProcessorMemoryType::GDDR2, "GDDR2"},
    {Processor_ProcessorMemoryType::GDDR3, "GDDR3"},
    {Processor_ProcessorMemoryType::GDDR4, "GDDR4"},
    {Processor_ProcessorMemoryType::GDDR5, "GDDR5"},
    {Processor_ProcessorMemoryType::GDDR5X, "GDDR5X"},
    {Processor_ProcessorMemoryType::GDDR6, "GDDR6"},
    {Processor_ProcessorMemoryType::DDR, "DDR"},
    {Processor_ProcessorMemoryType::DDR2, "DDR2"},
    {Processor_ProcessorMemoryType::DDR3, "DDR3"},
    {Processor_ProcessorMemoryType::DDR4, "DDR4"},
    {Processor_ProcessorMemoryType::DDR5, "DDR5"},
    {Processor_ProcessorMemoryType::SDRAM, "SDRAM"},
    {Processor_ProcessorMemoryType::SRAM, "SRAM"},
    {Processor_ProcessorMemoryType::Flash, "Flash"},
    {Processor_ProcessorMemoryType::OEM, "OEM"},
});

enum class Processor_SystemInterfaceType{
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

NLOHMANN_JSON_SERIALIZE_ENUM(Processor_SystemInterfaceType, {
    {Processor_SystemInterfaceType::Invalid, "Invalid"},
    {Processor_SystemInterfaceType::QPI, "QPI"},
    {Processor_SystemInterfaceType::UPI, "UPI"},
    {Processor_SystemInterfaceType::PCIe, "PCIe"},
    {Processor_SystemInterfaceType::Ethernet, "Ethernet"},
    {Processor_SystemInterfaceType::AMBA, "AMBA"},
    {Processor_SystemInterfaceType::CCIX, "CCIX"},
    {Processor_SystemInterfaceType::CXL, "CXL"},
    {Processor_SystemInterfaceType::OEM, "OEM"},
});

enum class Processor_BaseSpeedPriorityState{
    Invalid,
    Enabled,
    Disabled,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Processor_BaseSpeedPriorityState, {
    {Processor_BaseSpeedPriorityState::Invalid, "Invalid"},
    {Processor_BaseSpeedPriorityState::Enabled, "Enabled"},
    {Processor_BaseSpeedPriorityState::Disabled, "Disabled"},
});

enum class Processor_TurboState{
    Invalid,
    Enabled,
    Disabled,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Processor_TurboState, {
    {Processor_TurboState::Invalid, "Invalid"},
    {Processor_TurboState::Enabled, "Enabled"},
    {Processor_TurboState::Disabled, "Disabled"},
});

enum class Protocol_Protocol{
    Invalid,
    PCIe,
    AHCI,
    UHCI,
    SAS,
    SATA,
    USB,
    NVMe,
    FC,
    iSCSI,
    FCoE,
    FCP,
    FICON,
    NVMeOverFabrics,
    SMB,
    NFSv3,
    NFSv4,
    HTTP,
    HTTPS,
    FTP,
    SFTP,
    iWARP,
    RoCE,
    RoCEv2,
    I2C,
    TCP,
    UDP,
    TFTP,
    GenZ,
    MultiProtocol,
    InfiniBand,
    Ethernet,
    NVLink,
    OEM,
    DisplayPort,
    HDMI,
    VGA,
    DVI,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Protocol_Protocol, {
    {Protocol_Protocol::Invalid, "Invalid"},
    {Protocol_Protocol::PCIe, "PCIe"},
    {Protocol_Protocol::AHCI, "AHCI"},
    {Protocol_Protocol::UHCI, "UHCI"},
    {Protocol_Protocol::SAS, "SAS"},
    {Protocol_Protocol::SATA, "SATA"},
    {Protocol_Protocol::USB, "USB"},
    {Protocol_Protocol::NVMe, "NVMe"},
    {Protocol_Protocol::FC, "FC"},
    {Protocol_Protocol::iSCSI, "iSCSI"},
    {Protocol_Protocol::FCoE, "FCoE"},
    {Protocol_Protocol::FCP, "FCP"},
    {Protocol_Protocol::FICON, "FICON"},
    {Protocol_Protocol::NVMeOverFabrics, "NVMeOverFabrics"},
    {Protocol_Protocol::SMB, "SMB"},
    {Protocol_Protocol::NFSv3, "NFSv3"},
    {Protocol_Protocol::NFSv4, "NFSv4"},
    {Protocol_Protocol::HTTP, "HTTP"},
    {Protocol_Protocol::HTTPS, "HTTPS"},
    {Protocol_Protocol::FTP, "FTP"},
    {Protocol_Protocol::SFTP, "SFTP"},
    {Protocol_Protocol::iWARP, "iWARP"},
    {Protocol_Protocol::RoCE, "RoCE"},
    {Protocol_Protocol::RoCEv2, "RoCEv2"},
    {Protocol_Protocol::I2C, "I2C"},
    {Protocol_Protocol::TCP, "TCP"},
    {Protocol_Protocol::UDP, "UDP"},
    {Protocol_Protocol::TFTP, "TFTP"},
    {Protocol_Protocol::GenZ, "GenZ"},
    {Protocol_Protocol::MultiProtocol, "MultiProtocol"},
    {Protocol_Protocol::InfiniBand, "InfiniBand"},
    {Protocol_Protocol::Ethernet, "Ethernet"},
    {Protocol_Protocol::NVLink, "NVLink"},
    {Protocol_Protocol::OEM, "OEM"},
    {Protocol_Protocol::DisplayPort, "DisplayPort"},
    {Protocol_Protocol::HDMI, "HDMI"},
    {Protocol_Protocol::VGA, "VGA"},
    {Protocol_Protocol::DVI, "DVI"},
});

enum class RedfishExtensions_ReleaseStatusType{
    Invalid,
    Standard,
    Informational,
    WorkInProgress,
    InDevelopment,
};

NLOHMANN_JSON_SERIALIZE_ENUM(RedfishExtensions_ReleaseStatusType, {
    {RedfishExtensions_ReleaseStatusType::Invalid, "Invalid"},
    {RedfishExtensions_ReleaseStatusType::Standard, "Standard"},
    {RedfishExtensions_ReleaseStatusType::Informational, "Informational"},
    {RedfishExtensions_ReleaseStatusType::WorkInProgress, "WorkInProgress"},
    {RedfishExtensions_ReleaseStatusType::InDevelopment, "InDevelopment"},
});

enum class RedfishExtensions_RevisionKind{
    Invalid,
    Added,
    Modified,
    Deprecated,
};

NLOHMANN_JSON_SERIALIZE_ENUM(RedfishExtensions_RevisionKind, {
    {RedfishExtensions_RevisionKind::Invalid, "Invalid"},
    {RedfishExtensions_RevisionKind::Added, "Added"},
    {RedfishExtensions_RevisionKind::Modified, "Modified"},
    {RedfishExtensions_RevisionKind::Deprecated, "Deprecated"},
});

enum class Redundancy_RedundancyType{
    Invalid,
    Failover,
    NPlusM,
    Sharing,
    Sparing,
    NotRedundant,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Redundancy_RedundancyType, {
    {Redundancy_RedundancyType::Invalid, "Invalid"},
    {Redundancy_RedundancyType::Failover, "Failover"},
    {Redundancy_RedundancyType::NPlusM, "NPlusM"},
    {Redundancy_RedundancyType::Sharing, "Sharing"},
    {Redundancy_RedundancyType::Sparing, "Sparing"},
    {Redundancy_RedundancyType::NotRedundant, "NotRedundant"},
});

enum class RegisteredClient_ClientType{
    Invalid,
    Monitor,
    Configure,
};

NLOHMANN_JSON_SERIALIZE_ENUM(RegisteredClient_ClientType, {
    {RegisteredClient_ClientType::Invalid, "Invalid"},
    {RegisteredClient_ClientType::Monitor, "Monitor"},
    {RegisteredClient_ClientType::Configure, "Configure"},
});

enum class Resource_DurableNameFormat{
    Invalid,
    NAA,
    iQN,
    FC_WWN,
    UUID,
    EUI,
    NQN,
    NSID,
    NGUID,
    MACAddress,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Resource_DurableNameFormat, {
    {Resource_DurableNameFormat::Invalid, "Invalid"},
    {Resource_DurableNameFormat::NAA, "NAA"},
    {Resource_DurableNameFormat::iQN, "iQN"},
    {Resource_DurableNameFormat::FC_WWN, "FC_WWN"},
    {Resource_DurableNameFormat::UUID, "UUID"},
    {Resource_DurableNameFormat::EUI, "EUI"},
    {Resource_DurableNameFormat::NQN, "NQN"},
    {Resource_DurableNameFormat::NSID, "NSID"},
    {Resource_DurableNameFormat::NGUID, "NGUID"},
    {Resource_DurableNameFormat::MACAddress, "MACAddress"},
});

enum class Resource_RackUnits{
    Invalid,
    OpenU,
    EIA_310,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Resource_RackUnits, {
    {Resource_RackUnits::Invalid, "Invalid"},
    {Resource_RackUnits::OpenU, "OpenU"},
    {Resource_RackUnits::EIA_310, "EIA_310"},
});

enum class Resource_LocationType{
    Invalid,
    Slot,
    Bay,
    Connector,
    Socket,
    Backplane,
    Embedded,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Resource_LocationType, {
    {Resource_LocationType::Invalid, "Invalid"},
    {Resource_LocationType::Slot, "Slot"},
    {Resource_LocationType::Bay, "Bay"},
    {Resource_LocationType::Connector, "Connector"},
    {Resource_LocationType::Socket, "Socket"},
    {Resource_LocationType::Backplane, "Backplane"},
    {Resource_LocationType::Embedded, "Embedded"},
});

enum class Resource_Orientation{
    Invalid,
    FrontToBack,
    BackToFront,
    TopToBottom,
    BottomToTop,
    LeftToRight,
    RightToLeft,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Resource_Orientation, {
    {Resource_Orientation::Invalid, "Invalid"},
    {Resource_Orientation::FrontToBack, "FrontToBack"},
    {Resource_Orientation::BackToFront, "BackToFront"},
    {Resource_Orientation::TopToBottom, "TopToBottom"},
    {Resource_Orientation::BottomToTop, "BottomToTop"},
    {Resource_Orientation::LeftToRight, "LeftToRight"},
    {Resource_Orientation::RightToLeft, "RightToLeft"},
});

enum class Resource_Reference{
    Invalid,
    Top,
    Bottom,
    Front,
    Rear,
    Left,
    Right,
    Middle,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Resource_Reference, {
    {Resource_Reference::Invalid, "Invalid"},
    {Resource_Reference::Top, "Top"},
    {Resource_Reference::Bottom, "Bottom"},
    {Resource_Reference::Front, "Front"},
    {Resource_Reference::Rear, "Rear"},
    {Resource_Reference::Left, "Left"},
    {Resource_Reference::Right, "Right"},
    {Resource_Reference::Middle, "Middle"},
});

enum class ResourceBlock_CompositionState{
    Invalid,
    Composing,
    ComposedAndAvailable,
    Composed,
    Unused,
    Failed,
    Unavailable,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ResourceBlock_CompositionState, {
    {ResourceBlock_CompositionState::Invalid, "Invalid"},
    {ResourceBlock_CompositionState::Composing, "Composing"},
    {ResourceBlock_CompositionState::ComposedAndAvailable, "ComposedAndAvailable"},
    {ResourceBlock_CompositionState::Composed, "Composed"},
    {ResourceBlock_CompositionState::Unused, "Unused"},
    {ResourceBlock_CompositionState::Failed, "Failed"},
    {ResourceBlock_CompositionState::Unavailable, "Unavailable"},
});

enum class ResourceBlock_ResourceBlockType{
    Invalid,
    Compute,
    Processor,
    Memory,
    Network,
    Storage,
    ComputerSystem,
    Expansion,
    IndependentResource,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ResourceBlock_ResourceBlockType, {
    {ResourceBlock_ResourceBlockType::Invalid, "Invalid"},
    {ResourceBlock_ResourceBlockType::Compute, "Compute"},
    {ResourceBlock_ResourceBlockType::Processor, "Processor"},
    {ResourceBlock_ResourceBlockType::Memory, "Memory"},
    {ResourceBlock_ResourceBlockType::Network, "Network"},
    {ResourceBlock_ResourceBlockType::Storage, "Storage"},
    {ResourceBlock_ResourceBlockType::ComputerSystem, "ComputerSystem"},
    {ResourceBlock_ResourceBlockType::Expansion, "Expansion"},
    {ResourceBlock_ResourceBlockType::IndependentResource, "IndependentResource"},
});

enum class ResourceBlock_PoolType{
    Invalid,
    Free,
    Active,
    Unassigned,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ResourceBlock_PoolType, {
    {ResourceBlock_PoolType::Invalid, "Invalid"},
    {ResourceBlock_PoolType::Free, "Free"},
    {ResourceBlock_PoolType::Active, "Active"},
    {ResourceBlock_PoolType::Unassigned, "Unassigned"},
});

enum class Resource_Health{
    Invalid,
    OK,
    Warning,
    Critical,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Resource_Health, {
    {Resource_Health::Invalid, "Invalid"},
    {Resource_Health::OK, "OK"},
    {Resource_Health::Warning, "Warning"},
    {Resource_Health::Critical, "Critical"},
});

enum class Resource_IndicatorLED{
    Invalid,
    Lit,
    Blinking,
    Off,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Resource_IndicatorLED, {
    {Resource_IndicatorLED::Invalid, "Invalid"},
    {Resource_IndicatorLED::Lit, "Lit"},
    {Resource_IndicatorLED::Blinking, "Blinking"},
    {Resource_IndicatorLED::Off, "Off"},
});

enum class Resource_PowerState{
    Invalid,
    On,
    Off,
    PoweringOn,
    PoweringOff,
    Paused,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Resource_PowerState, {
    {Resource_PowerState::Invalid, "Invalid"},
    {Resource_PowerState::On, "On"},
    {Resource_PowerState::Off, "Off"},
    {Resource_PowerState::PoweringOn, "PoweringOn"},
    {Resource_PowerState::PoweringOff, "PoweringOff"},
    {Resource_PowerState::Paused, "Paused"},
});

enum class Resource_ResetType{
    Invalid,
    On,
    ForceOff,
    GracefulShutdown,
    GracefulRestart,
    ForceRestart,
    Nmi,
    ForceOn,
    PushPowerButton,
    PowerCycle,
    Suspend,
    Pause,
    Resume,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Resource_ResetType, {
    {Resource_ResetType::Invalid, "Invalid"},
    {Resource_ResetType::On, "On"},
    {Resource_ResetType::ForceOff, "ForceOff"},
    {Resource_ResetType::GracefulShutdown, "GracefulShutdown"},
    {Resource_ResetType::GracefulRestart, "GracefulRestart"},
    {Resource_ResetType::ForceRestart, "ForceRestart"},
    {Resource_ResetType::Nmi, "Nmi"},
    {Resource_ResetType::ForceOn, "ForceOn"},
    {Resource_ResetType::PushPowerButton, "PushPowerButton"},
    {Resource_ResetType::PowerCycle, "PowerCycle"},
    {Resource_ResetType::Suspend, "Suspend"},
    {Resource_ResetType::Pause, "Pause"},
    {Resource_ResetType::Resume, "Resume"},
});

enum class Resource_State{
    Invalid,
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

NLOHMANN_JSON_SERIALIZE_ENUM(Resource_State, {
    {Resource_State::Invalid, "Invalid"},
    {Resource_State::Enabled, "Enabled"},
    {Resource_State::Disabled, "Disabled"},
    {Resource_State::StandbyOffline, "StandbyOffline"},
    {Resource_State::StandbySpare, "StandbySpare"},
    {Resource_State::InTest, "InTest"},
    {Resource_State::Starting, "Starting"},
    {Resource_State::Absent, "Absent"},
    {Resource_State::UnavailableOffline, "UnavailableOffline"},
    {Resource_State::Deferring, "Deferring"},
    {Resource_State::Quiesced, "Quiesced"},
    {Resource_State::Updating, "Updating"},
    {Resource_State::Qualified, "Qualified"},
});

enum class Schedule_DayOfWeek{
    Invalid,
    Monday,
    Tuesday,
    Wednesday,
    Thursday,
    Friday,
    Saturday,
    Sunday,
    Every,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Schedule_DayOfWeek, {
    {Schedule_DayOfWeek::Invalid, "Invalid"},
    {Schedule_DayOfWeek::Monday, "Monday"},
    {Schedule_DayOfWeek::Tuesday, "Tuesday"},
    {Schedule_DayOfWeek::Wednesday, "Wednesday"},
    {Schedule_DayOfWeek::Thursday, "Thursday"},
    {Schedule_DayOfWeek::Friday, "Friday"},
    {Schedule_DayOfWeek::Saturday, "Saturday"},
    {Schedule_DayOfWeek::Sunday, "Sunday"},
    {Schedule_DayOfWeek::Every, "Every"},
});

enum class Schedule_MonthOfYear{
    Invalid,
    January,
    February,
    March,
    April,
    May,
    June,
    July,
    August,
    September,
    October,
    November,
    December,
    Every,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Schedule_MonthOfYear, {
    {Schedule_MonthOfYear::Invalid, "Invalid"},
    {Schedule_MonthOfYear::January, "January"},
    {Schedule_MonthOfYear::February, "February"},
    {Schedule_MonthOfYear::March, "March"},
    {Schedule_MonthOfYear::April, "April"},
    {Schedule_MonthOfYear::May, "May"},
    {Schedule_MonthOfYear::June, "June"},
    {Schedule_MonthOfYear::July, "July"},
    {Schedule_MonthOfYear::August, "August"},
    {Schedule_MonthOfYear::September, "September"},
    {Schedule_MonthOfYear::October, "October"},
    {Schedule_MonthOfYear::November, "November"},
    {Schedule_MonthOfYear::December, "December"},
    {Schedule_MonthOfYear::Every, "Every"},
});

enum class SecureBoot_ResetKeysType{
    Invalid,
    ResetAllKeysToDefault,
    DeleteAllKeys,
    DeletePK,
};

NLOHMANN_JSON_SERIALIZE_ENUM(SecureBoot_ResetKeysType, {
    {SecureBoot_ResetKeysType::Invalid, "Invalid"},
    {SecureBoot_ResetKeysType::ResetAllKeysToDefault, "ResetAllKeysToDefault"},
    {SecureBoot_ResetKeysType::DeleteAllKeys, "DeleteAllKeys"},
    {SecureBoot_ResetKeysType::DeletePK, "DeletePK"},
});

enum class SecureBoot_SecureBootCurrentBootType{
    Invalid,
    Enabled,
    Disabled,
};

NLOHMANN_JSON_SERIALIZE_ENUM(SecureBoot_SecureBootCurrentBootType, {
    {SecureBoot_SecureBootCurrentBootType::Invalid, "Invalid"},
    {SecureBoot_SecureBootCurrentBootType::Enabled, "Enabled"},
    {SecureBoot_SecureBootCurrentBootType::Disabled, "Disabled"},
});

enum class SecureBoot_SecureBootModeType{
    Invalid,
    SetupMode,
    UserMode,
    AuditMode,
    DeployedMode,
};

NLOHMANN_JSON_SERIALIZE_ENUM(SecureBoot_SecureBootModeType, {
    {SecureBoot_SecureBootModeType::Invalid, "Invalid"},
    {SecureBoot_SecureBootModeType::SetupMode, "SetupMode"},
    {SecureBoot_SecureBootModeType::UserMode, "UserMode"},
    {SecureBoot_SecureBootModeType::AuditMode, "AuditMode"},
    {SecureBoot_SecureBootModeType::DeployedMode, "DeployedMode"},
});

enum class SecureBootDatabase_ResetKeysType{
    Invalid,
    ResetAllKeysToDefault,
    DeleteAllKeys,
};

NLOHMANN_JSON_SERIALIZE_ENUM(SecureBootDatabase_ResetKeysType, {
    {SecureBootDatabase_ResetKeysType::Invalid, "Invalid"},
    {SecureBootDatabase_ResetKeysType::ResetAllKeysToDefault, "ResetAllKeysToDefault"},
    {SecureBootDatabase_ResetKeysType::DeleteAllKeys, "DeleteAllKeys"},
});

enum class Sensor_ReadingType{
    Invalid,
    Temperature,
    Humidity,
    Power,
    EnergykWh,
    EnergyJoules,
    EnergyWh,
    ChargeAh,
    Voltage,
    Current,
    Frequency,
    Pressure,
    PressurekPa,
    LiquidLevel,
    Rotational,
    AirFlow,
    LiquidFlow,
    Barometric,
    Altitude,
    Percent,
    AbsoluteHumidity,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Sensor_ReadingType, {
    {Sensor_ReadingType::Invalid, "Invalid"},
    {Sensor_ReadingType::Temperature, "Temperature"},
    {Sensor_ReadingType::Humidity, "Humidity"},
    {Sensor_ReadingType::Power, "Power"},
    {Sensor_ReadingType::EnergykWh, "EnergykWh"},
    {Sensor_ReadingType::EnergyJoules, "EnergyJoules"},
    {Sensor_ReadingType::EnergyWh, "EnergyWh"},
    {Sensor_ReadingType::ChargeAh, "ChargeAh"},
    {Sensor_ReadingType::Voltage, "Voltage"},
    {Sensor_ReadingType::Current, "Current"},
    {Sensor_ReadingType::Frequency, "Frequency"},
    {Sensor_ReadingType::Pressure, "Pressure"},
    {Sensor_ReadingType::PressurekPa, "PressurekPa"},
    {Sensor_ReadingType::LiquidLevel, "LiquidLevel"},
    {Sensor_ReadingType::Rotational, "Rotational"},
    {Sensor_ReadingType::AirFlow, "AirFlow"},
    {Sensor_ReadingType::LiquidFlow, "LiquidFlow"},
    {Sensor_ReadingType::Barometric, "Barometric"},
    {Sensor_ReadingType::Altitude, "Altitude"},
    {Sensor_ReadingType::Percent, "Percent"},
    {Sensor_ReadingType::AbsoluteHumidity, "AbsoluteHumidity"},
});

enum class Sensor_ThresholdActivation{
    Invalid,
    Increasing,
    Decreasing,
    Either,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Sensor_ThresholdActivation, {
    {Sensor_ThresholdActivation::Invalid, "Invalid"},
    {Sensor_ThresholdActivation::Increasing, "Increasing"},
    {Sensor_ThresholdActivation::Decreasing, "Decreasing"},
    {Sensor_ThresholdActivation::Either, "Either"},
});

enum class Sensor_ImplementationType{
    Invalid,
    PhysicalSensor,
    Synthesized,
    Reported,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Sensor_ImplementationType, {
    {Sensor_ImplementationType::Invalid, "Invalid"},
    {Sensor_ImplementationType::PhysicalSensor, "PhysicalSensor"},
    {Sensor_ImplementationType::Synthesized, "Synthesized"},
    {Sensor_ImplementationType::Reported, "Reported"},
});

enum class Sensor_ElectricalContext{
    Invalid,
    Line1,
    Line2,
    Line3,
    Neutral,
    LineToLine,
    Line1ToLine2,
    Line2ToLine3,
    Line3ToLine1,
    LineToNeutral,
    Line1ToNeutral,
    Line2ToNeutral,
    Line3ToNeutral,
    Line1ToNeutralAndL1L2,
    Line2ToNeutralAndL1L2,
    Line2ToNeutralAndL2L3,
    Line3ToNeutralAndL3L1,
    Total,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Sensor_ElectricalContext, {
    {Sensor_ElectricalContext::Invalid, "Invalid"},
    {Sensor_ElectricalContext::Line1, "Line1"},
    {Sensor_ElectricalContext::Line2, "Line2"},
    {Sensor_ElectricalContext::Line3, "Line3"},
    {Sensor_ElectricalContext::Neutral, "Neutral"},
    {Sensor_ElectricalContext::LineToLine, "LineToLine"},
    {Sensor_ElectricalContext::Line1ToLine2, "Line1ToLine2"},
    {Sensor_ElectricalContext::Line2ToLine3, "Line2ToLine3"},
    {Sensor_ElectricalContext::Line3ToLine1, "Line3ToLine1"},
    {Sensor_ElectricalContext::LineToNeutral, "LineToNeutral"},
    {Sensor_ElectricalContext::Line1ToNeutral, "Line1ToNeutral"},
    {Sensor_ElectricalContext::Line2ToNeutral, "Line2ToNeutral"},
    {Sensor_ElectricalContext::Line3ToNeutral, "Line3ToNeutral"},
    {Sensor_ElectricalContext::Line1ToNeutralAndL1L2, "Line1ToNeutralAndL1L2"},
    {Sensor_ElectricalContext::Line2ToNeutralAndL1L2, "Line2ToNeutralAndL1L2"},
    {Sensor_ElectricalContext::Line2ToNeutralAndL2L3, "Line2ToNeutralAndL2L3"},
    {Sensor_ElectricalContext::Line3ToNeutralAndL3L1, "Line3ToNeutralAndL3L1"},
    {Sensor_ElectricalContext::Total, "Total"},
});

enum class Sensor_VoltageType{
    Invalid,
    AC,
    DC,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Sensor_VoltageType, {
    {Sensor_VoltageType::Invalid, "Invalid"},
    {Sensor_VoltageType::AC, "AC"},
    {Sensor_VoltageType::DC, "DC"},
});

enum class SerialInterface_FlowControl{
    Invalid,
    None,
    Software,
    Hardware,
};

NLOHMANN_JSON_SERIALIZE_ENUM(SerialInterface_FlowControl, {
    {SerialInterface_FlowControl::Invalid, "Invalid"},
    {SerialInterface_FlowControl::None, "None"},
    {SerialInterface_FlowControl::Software, "Software"},
    {SerialInterface_FlowControl::Hardware, "Hardware"},
});

enum class SerialInterface_Parity{
    Invalid,
    None,
    Even,
    Odd,
    Mark,
    Space,
};

NLOHMANN_JSON_SERIALIZE_ENUM(SerialInterface_Parity, {
    {SerialInterface_Parity::Invalid, "Invalid"},
    {SerialInterface_Parity::None, "None"},
    {SerialInterface_Parity::Even, "Even"},
    {SerialInterface_Parity::Odd, "Odd"},
    {SerialInterface_Parity::Mark, "Mark"},
    {SerialInterface_Parity::Space, "Space"},
});

enum class SerialInterface_PinOut{
    Invalid,
    Cisco,
    Cyclades,
    Digi,
};

NLOHMANN_JSON_SERIALIZE_ENUM(SerialInterface_PinOut, {
    {SerialInterface_PinOut::Invalid, "Invalid"},
    {SerialInterface_PinOut::Cisco, "Cisco"},
    {SerialInterface_PinOut::Cyclades, "Cyclades"},
    {SerialInterface_PinOut::Digi, "Digi"},
});

enum class SerialInterface_SignalType{
    Invalid,
    Rs232,
    Rs485,
};

NLOHMANN_JSON_SERIALIZE_ENUM(SerialInterface_SignalType, {
    {SerialInterface_SignalType::Invalid, "Invalid"},
    {SerialInterface_SignalType::Rs232, "Rs232"},
    {SerialInterface_SignalType::Rs485, "Rs485"},
});

enum class Session_SessionTypes{
    Invalid,
    HostConsole,
    ManagerConsole,
    IPMI,
    KVMIP,
    OEM,
    Redfish,
    VirtualMedia,
    WebUI,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Session_SessionTypes, {
    {Session_SessionTypes::Invalid, "Invalid"},
    {Session_SessionTypes::HostConsole, "HostConsole"},
    {Session_SessionTypes::ManagerConsole, "ManagerConsole"},
    {Session_SessionTypes::IPMI, "IPMI"},
    {Session_SessionTypes::KVMIP, "KVMIP"},
    {Session_SessionTypes::OEM, "OEM"},
    {Session_SessionTypes::Redfish, "Redfish"},
    {Session_SessionTypes::VirtualMedia, "VirtualMedia"},
    {Session_SessionTypes::WebUI, "WebUI"},
});

enum class Settings_ApplyTime{
    Invalid,
    Immediate,
    OnReset,
    AtMaintenanceWindowStart,
    InMaintenanceWindowOnReset,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Settings_ApplyTime, {
    {Settings_ApplyTime::Invalid, "Invalid"},
    {Settings_ApplyTime::Immediate, "Immediate"},
    {Settings_ApplyTime::OnReset, "OnReset"},
    {Settings_ApplyTime::AtMaintenanceWindowStart, "AtMaintenanceWindowStart"},
    {Settings_ApplyTime::InMaintenanceWindowOnReset, "InMaintenanceWindowOnReset"},
});

enum class Settings_OperationApplyTime{
    Invalid,
    Immediate,
    OnReset,
    AtMaintenanceWindowStart,
    InMaintenanceWindowOnReset,
    OnStartUpdateRequest,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Settings_OperationApplyTime, {
    {Settings_OperationApplyTime::Invalid, "Invalid"},
    {Settings_OperationApplyTime::Immediate, "Immediate"},
    {Settings_OperationApplyTime::OnReset, "OnReset"},
    {Settings_OperationApplyTime::AtMaintenanceWindowStart, "AtMaintenanceWindowStart"},
    {Settings_OperationApplyTime::InMaintenanceWindowOnReset, "InMaintenanceWindowOnReset"},
    {Settings_OperationApplyTime::OnStartUpdateRequest, "OnStartUpdateRequest"},
});

enum class Signature_SignatureTypeRegistry{
    Invalid,
    UEFI,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Signature_SignatureTypeRegistry, {
    {Signature_SignatureTypeRegistry::Invalid, "Invalid"},
    {Signature_SignatureTypeRegistry::UEFI, "UEFI"},
});

enum class Storage_ResetToDefaultsType{
    Invalid,
    ResetAll,
    PreserveVolumes,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Storage_ResetToDefaultsType, {
    {Storage_ResetToDefaultsType::Invalid, "Invalid"},
    {Storage_ResetToDefaultsType::ResetAll, "ResetAll"},
    {Storage_ResetToDefaultsType::PreserveVolumes, "PreserveVolumes"},
});

enum class StorageController_ANAAccessState{
    Invalid,
    Optimized,
    NonOptimized,
    Inaccessible,
    PersistentLoss,
};

NLOHMANN_JSON_SERIALIZE_ENUM(StorageController_ANAAccessState, {
    {StorageController_ANAAccessState::Invalid, "Invalid"},
    {StorageController_ANAAccessState::Optimized, "Optimized"},
    {StorageController_ANAAccessState::NonOptimized, "NonOptimized"},
    {StorageController_ANAAccessState::Inaccessible, "Inaccessible"},
    {StorageController_ANAAccessState::PersistentLoss, "PersistentLoss"},
});

enum class StorageController_NVMeControllerType{
    Invalid,
    Admin,
    Discovery,
    IO,
};

NLOHMANN_JSON_SERIALIZE_ENUM(StorageController_NVMeControllerType, {
    {StorageController_NVMeControllerType::Invalid, "Invalid"},
    {StorageController_NVMeControllerType::Admin, "Admin"},
    {StorageController_NVMeControllerType::Discovery, "Discovery"},
    {StorageController_NVMeControllerType::IO, "IO"},
});

enum class StorageGroup_AuthenticationMethod{
    Invalid,
    None,
    CHAP,
    MutualCHAP,
    DHCHAP,
};

NLOHMANN_JSON_SERIALIZE_ENUM(StorageGroup_AuthenticationMethod, {
    {StorageGroup_AuthenticationMethod::Invalid, "Invalid"},
    {StorageGroup_AuthenticationMethod::None, "None"},
    {StorageGroup_AuthenticationMethod::CHAP, "CHAP"},
    {StorageGroup_AuthenticationMethod::MutualCHAP, "MutualCHAP"},
    {StorageGroup_AuthenticationMethod::DHCHAP, "DHCHAP"},
});

enum class StorageGroup_AccessCapability{
    Invalid,
    Read,
    ReadWrite,
};

NLOHMANN_JSON_SERIALIZE_ENUM(StorageGroup_AccessCapability, {
    {StorageGroup_AccessCapability::Invalid, "Invalid"},
    {StorageGroup_AccessCapability::Read, "Read"},
    {StorageGroup_AccessCapability::ReadWrite, "ReadWrite"},
});

enum class StoragePool_NVMePoolType{
    Invalid,
    EnduranceGroup,
    NVMSet,
};

NLOHMANN_JSON_SERIALIZE_ENUM(StoragePool_NVMePoolType, {
    {StoragePool_NVMePoolType::Invalid, "Invalid"},
    {StoragePool_NVMePoolType::EnduranceGroup, "EnduranceGroup"},
    {StoragePool_NVMePoolType::NVMSet, "NVMSet"},
});

enum class StoragePool_PoolType{
    Invalid,
    Block,
    File,
    Object,
    Pool,
};

NLOHMANN_JSON_SERIALIZE_ENUM(StoragePool_PoolType, {
    {StoragePool_PoolType::Invalid, "Invalid"},
    {StoragePool_PoolType::Block, "Block"},
    {StoragePool_PoolType::File, "File"},
    {StoragePool_PoolType::Object, "Object"},
    {StoragePool_PoolType::Pool, "Pool"},
});

enum class StorageReplicaInfo_ConsistencyState{
    Invalid,
    Consistent,
    Inconsistent,
};

NLOHMANN_JSON_SERIALIZE_ENUM(StorageReplicaInfo_ConsistencyState, {
    {StorageReplicaInfo_ConsistencyState::Invalid, "Invalid"},
    {StorageReplicaInfo_ConsistencyState::Consistent, "Consistent"},
    {StorageReplicaInfo_ConsistencyState::Inconsistent, "Inconsistent"},
});

enum class StorageReplicaInfo_ConsistencyStatus{
    Invalid,
    Consistent,
    InProgress,
    Disabled,
    InError,
};

NLOHMANN_JSON_SERIALIZE_ENUM(StorageReplicaInfo_ConsistencyStatus, {
    {StorageReplicaInfo_ConsistencyStatus::Invalid, "Invalid"},
    {StorageReplicaInfo_ConsistencyStatus::Consistent, "Consistent"},
    {StorageReplicaInfo_ConsistencyStatus::InProgress, "InProgress"},
    {StorageReplicaInfo_ConsistencyStatus::Disabled, "Disabled"},
    {StorageReplicaInfo_ConsistencyStatus::InError, "InError"},
});

enum class StorageReplicaInfo_ConsistencyType{
    Invalid,
    SequentiallyConsistent,
};

NLOHMANN_JSON_SERIALIZE_ENUM(StorageReplicaInfo_ConsistencyType, {
    {StorageReplicaInfo_ConsistencyType::Invalid, "Invalid"},
    {StorageReplicaInfo_ConsistencyType::SequentiallyConsistent, "SequentiallyConsistent"},
});

enum class StorageReplicaInfo_ReplicaPriority{
    Invalid,
    Low,
    Same,
    High,
    Urgent,
};

NLOHMANN_JSON_SERIALIZE_ENUM(StorageReplicaInfo_ReplicaPriority, {
    {StorageReplicaInfo_ReplicaPriority::Invalid, "Invalid"},
    {StorageReplicaInfo_ReplicaPriority::Low, "Low"},
    {StorageReplicaInfo_ReplicaPriority::Same, "Same"},
    {StorageReplicaInfo_ReplicaPriority::High, "High"},
    {StorageReplicaInfo_ReplicaPriority::Urgent, "Urgent"},
});

enum class StorageReplicaInfo_ReplicaProgressStatus{
    Invalid,
    Completed,
    Dormant,
    Initializing,
    Preparing,
    Synchronizing,
    Resyncing,
    Restoring,
    Fracturing,
    Splitting,
    FailingOver,
    FailingBack,
    Detaching,
    Aborting,
    Mixed,
    Suspending,
    RequiresFracture,
    RequiresResync,
    RequiresActivate,
    Pending,
    RequiresDetach,
    Terminating,
    RequiresSplit,
    RequiresResume,
};

NLOHMANN_JSON_SERIALIZE_ENUM(StorageReplicaInfo_ReplicaProgressStatus, {
    {StorageReplicaInfo_ReplicaProgressStatus::Invalid, "Invalid"},
    {StorageReplicaInfo_ReplicaProgressStatus::Completed, "Completed"},
    {StorageReplicaInfo_ReplicaProgressStatus::Dormant, "Dormant"},
    {StorageReplicaInfo_ReplicaProgressStatus::Initializing, "Initializing"},
    {StorageReplicaInfo_ReplicaProgressStatus::Preparing, "Preparing"},
    {StorageReplicaInfo_ReplicaProgressStatus::Synchronizing, "Synchronizing"},
    {StorageReplicaInfo_ReplicaProgressStatus::Resyncing, "Resyncing"},
    {StorageReplicaInfo_ReplicaProgressStatus::Restoring, "Restoring"},
    {StorageReplicaInfo_ReplicaProgressStatus::Fracturing, "Fracturing"},
    {StorageReplicaInfo_ReplicaProgressStatus::Splitting, "Splitting"},
    {StorageReplicaInfo_ReplicaProgressStatus::FailingOver, "FailingOver"},
    {StorageReplicaInfo_ReplicaProgressStatus::FailingBack, "FailingBack"},
    {StorageReplicaInfo_ReplicaProgressStatus::Detaching, "Detaching"},
    {StorageReplicaInfo_ReplicaProgressStatus::Aborting, "Aborting"},
    {StorageReplicaInfo_ReplicaProgressStatus::Mixed, "Mixed"},
    {StorageReplicaInfo_ReplicaProgressStatus::Suspending, "Suspending"},
    {StorageReplicaInfo_ReplicaProgressStatus::RequiresFracture, "RequiresFracture"},
    {StorageReplicaInfo_ReplicaProgressStatus::RequiresResync, "RequiresResync"},
    {StorageReplicaInfo_ReplicaProgressStatus::RequiresActivate, "RequiresActivate"},
    {StorageReplicaInfo_ReplicaProgressStatus::Pending, "Pending"},
    {StorageReplicaInfo_ReplicaProgressStatus::RequiresDetach, "RequiresDetach"},
    {StorageReplicaInfo_ReplicaProgressStatus::Terminating, "Terminating"},
    {StorageReplicaInfo_ReplicaProgressStatus::RequiresSplit, "RequiresSplit"},
    {StorageReplicaInfo_ReplicaProgressStatus::RequiresResume, "RequiresResume"},
});

enum class StorageReplicaInfo_ReplicaReadOnlyAccess{
    Invalid,
    SourceElement,
    ReplicaElement,
    Both,
};

NLOHMANN_JSON_SERIALIZE_ENUM(StorageReplicaInfo_ReplicaReadOnlyAccess, {
    {StorageReplicaInfo_ReplicaReadOnlyAccess::Invalid, "Invalid"},
    {StorageReplicaInfo_ReplicaReadOnlyAccess::SourceElement, "SourceElement"},
    {StorageReplicaInfo_ReplicaReadOnlyAccess::ReplicaElement, "ReplicaElement"},
    {StorageReplicaInfo_ReplicaReadOnlyAccess::Both, "Both"},
});

enum class StorageReplicaInfo_ReplicaRecoveryMode{
    Invalid,
    Automatic,
    Manual,
};

NLOHMANN_JSON_SERIALIZE_ENUM(StorageReplicaInfo_ReplicaRecoveryMode, {
    {StorageReplicaInfo_ReplicaRecoveryMode::Invalid, "Invalid"},
    {StorageReplicaInfo_ReplicaRecoveryMode::Automatic, "Automatic"},
    {StorageReplicaInfo_ReplicaRecoveryMode::Manual, "Manual"},
});

enum class StorageReplicaInfo_ReplicaRole{
    Invalid,
    Source,
    Target,
};

NLOHMANN_JSON_SERIALIZE_ENUM(StorageReplicaInfo_ReplicaRole, {
    {StorageReplicaInfo_ReplicaRole::Invalid, "Invalid"},
    {StorageReplicaInfo_ReplicaRole::Source, "Source"},
    {StorageReplicaInfo_ReplicaRole::Target, "Target"},
});

enum class StorageReplicaInfo_ReplicaState{
    Invalid,
    Source,
    Target,
};

NLOHMANN_JSON_SERIALIZE_ENUM(StorageReplicaInfo_ReplicaState, {
    {StorageReplicaInfo_ReplicaState::Invalid, "Invalid"},
    {StorageReplicaInfo_ReplicaState::Source, "Source"},
    {StorageReplicaInfo_ReplicaState::Target, "Target"},
});

enum class StorageReplicaInfo_UndiscoveredElement{
    Invalid,
    SourceElement,
    ReplicaElement,
};

NLOHMANN_JSON_SERIALIZE_ENUM(StorageReplicaInfo_UndiscoveredElement, {
    {StorageReplicaInfo_UndiscoveredElement::Invalid, "Invalid"},
    {StorageReplicaInfo_UndiscoveredElement::SourceElement, "SourceElement"},
    {StorageReplicaInfo_UndiscoveredElement::ReplicaElement, "ReplicaElement"},
});

enum class StorageReplicaInfo_ReplicaFaultDomain{
    Invalid,
    Local,
    Remote,
};

NLOHMANN_JSON_SERIALIZE_ENUM(StorageReplicaInfo_ReplicaFaultDomain, {
    {StorageReplicaInfo_ReplicaFaultDomain::Invalid, "Invalid"},
    {StorageReplicaInfo_ReplicaFaultDomain::Local, "Local"},
    {StorageReplicaInfo_ReplicaFaultDomain::Remote, "Remote"},
});

enum class StorageReplicaInfo_ReplicaType{
    Invalid,
    Mirror,
    Snapshot,
    Clone,
    TokenizedClone,
};

NLOHMANN_JSON_SERIALIZE_ENUM(StorageReplicaInfo_ReplicaType, {
    {StorageReplicaInfo_ReplicaType::Invalid, "Invalid"},
    {StorageReplicaInfo_ReplicaType::Mirror, "Mirror"},
    {StorageReplicaInfo_ReplicaType::Snapshot, "Snapshot"},
    {StorageReplicaInfo_ReplicaType::Clone, "Clone"},
    {StorageReplicaInfo_ReplicaType::TokenizedClone, "TokenizedClone"},
});

enum class StorageReplicaInfo_ReplicaUpdateMode{
    Invalid,
    Active,
    Synchronous,
    Asynchronous,
    Adaptive,
};

NLOHMANN_JSON_SERIALIZE_ENUM(StorageReplicaInfo_ReplicaUpdateMode, {
    {StorageReplicaInfo_ReplicaUpdateMode::Invalid, "Invalid"},
    {StorageReplicaInfo_ReplicaUpdateMode::Active, "Active"},
    {StorageReplicaInfo_ReplicaUpdateMode::Synchronous, "Synchronous"},
    {StorageReplicaInfo_ReplicaUpdateMode::Asynchronous, "Asynchronous"},
    {StorageReplicaInfo_ReplicaUpdateMode::Adaptive, "Adaptive"},
});

enum class Task_TaskState{
    Invalid,
    New,
    Starting,
    Running,
    Suspended,
    Interrupted,
    Pending,
    Stopping,
    Completed,
    Killed,
    Exception,
    Service,
    Cancelling,
    Cancelled,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Task_TaskState, {
    {Task_TaskState::Invalid, "Invalid"},
    {Task_TaskState::New, "New"},
    {Task_TaskState::Starting, "Starting"},
    {Task_TaskState::Running, "Running"},
    {Task_TaskState::Suspended, "Suspended"},
    {Task_TaskState::Interrupted, "Interrupted"},
    {Task_TaskState::Pending, "Pending"},
    {Task_TaskState::Stopping, "Stopping"},
    {Task_TaskState::Completed, "Completed"},
    {Task_TaskState::Killed, "Killed"},
    {Task_TaskState::Exception, "Exception"},
    {Task_TaskState::Service, "Service"},
    {Task_TaskState::Cancelling, "Cancelling"},
    {Task_TaskState::Cancelled, "Cancelled"},
});

enum class TaskService_OverWritePolicy{
    Invalid,
    Manual,
    Oldest,
};

NLOHMANN_JSON_SERIALIZE_ENUM(TaskService_OverWritePolicy, {
    {TaskService_OverWritePolicy::Invalid, "Invalid"},
    {TaskService_OverWritePolicy::Manual, "Manual"},
    {TaskService_OverWritePolicy::Oldest, "Oldest"},
});

enum class TelemetryService_CollectionFunction{
    Invalid,
    Average,
    Maximum,
    Minimum,
    Summation,
};

NLOHMANN_JSON_SERIALIZE_ENUM(TelemetryService_CollectionFunction, {
    {TelemetryService_CollectionFunction::Invalid, "Invalid"},
    {TelemetryService_CollectionFunction::Average, "Average"},
    {TelemetryService_CollectionFunction::Maximum, "Maximum"},
    {TelemetryService_CollectionFunction::Minimum, "Minimum"},
    {TelemetryService_CollectionFunction::Summation, "Summation"},
});

enum class Thermal_ReadingUnits{
    Invalid,
    RPM,
    Percent,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Thermal_ReadingUnits, {
    {Thermal_ReadingUnits::Invalid, "Invalid"},
    {Thermal_ReadingUnits::RPM, "RPM"},
    {Thermal_ReadingUnits::Percent, "Percent"},
});

enum class Triggers_DirectionOfCrossingEnum{
    Invalid,
    Increasing,
    Decreasing,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Triggers_DirectionOfCrossingEnum, {
    {Triggers_DirectionOfCrossingEnum::Invalid, "Invalid"},
    {Triggers_DirectionOfCrossingEnum::Increasing, "Increasing"},
    {Triggers_DirectionOfCrossingEnum::Decreasing, "Decreasing"},
});

enum class Triggers_DiscreteTriggerConditionEnum{
    Invalid,
    Specified,
    Changed,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Triggers_DiscreteTriggerConditionEnum, {
    {Triggers_DiscreteTriggerConditionEnum::Invalid, "Invalid"},
    {Triggers_DiscreteTriggerConditionEnum::Specified, "Specified"},
    {Triggers_DiscreteTriggerConditionEnum::Changed, "Changed"},
});

enum class Triggers_MetricTypeEnum{
    Invalid,
    Numeric,
    Discrete,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Triggers_MetricTypeEnum, {
    {Triggers_MetricTypeEnum::Invalid, "Invalid"},
    {Triggers_MetricTypeEnum::Numeric, "Numeric"},
    {Triggers_MetricTypeEnum::Discrete, "Discrete"},
});

enum class Triggers_ThresholdActivation{
    Invalid,
    Increasing,
    Decreasing,
    Either,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Triggers_ThresholdActivation, {
    {Triggers_ThresholdActivation::Invalid, "Invalid"},
    {Triggers_ThresholdActivation::Increasing, "Increasing"},
    {Triggers_ThresholdActivation::Decreasing, "Decreasing"},
    {Triggers_ThresholdActivation::Either, "Either"},
});

enum class Triggers_TriggerActionEnum{
    Invalid,
    LogToLogService,
    RedfishEvent,
    RedfishMetricReport,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Triggers_TriggerActionEnum, {
    {Triggers_TriggerActionEnum::Invalid, "Invalid"},
    {Triggers_TriggerActionEnum::LogToLogService, "LogToLogService"},
    {Triggers_TriggerActionEnum::RedfishEvent, "RedfishEvent"},
    {Triggers_TriggerActionEnum::RedfishMetricReport, "RedfishMetricReport"},
});

enum class UpdateService_TransferProtocolType{
    Invalid,
    CIFS,
    FTP,
    SFTP,
    HTTP,
    HTTPS,
    NSF,
    SCP,
    TFTP,
    OEM,
    NFS,
};

NLOHMANN_JSON_SERIALIZE_ENUM(UpdateService_TransferProtocolType, {
    {UpdateService_TransferProtocolType::Invalid, "Invalid"},
    {UpdateService_TransferProtocolType::CIFS, "CIFS"},
    {UpdateService_TransferProtocolType::FTP, "FTP"},
    {UpdateService_TransferProtocolType::SFTP, "SFTP"},
    {UpdateService_TransferProtocolType::HTTP, "HTTP"},
    {UpdateService_TransferProtocolType::HTTPS, "HTTPS"},
    {UpdateService_TransferProtocolType::NSF, "NSF"},
    {UpdateService_TransferProtocolType::SCP, "SCP"},
    {UpdateService_TransferProtocolType::TFTP, "TFTP"},
    {UpdateService_TransferProtocolType::OEM, "OEM"},
    {UpdateService_TransferProtocolType::NFS, "NFS"},
});

enum class UpdateService_ApplyTime{
    Invalid,
    Immediate,
    OnReset,
    AtMaintenanceWindowStart,
    InMaintenanceWindowOnReset,
    OnStartUpdateRequest,
};

NLOHMANN_JSON_SERIALIZE_ENUM(UpdateService_ApplyTime, {
    {UpdateService_ApplyTime::Invalid, "Invalid"},
    {UpdateService_ApplyTime::Immediate, "Immediate"},
    {UpdateService_ApplyTime::OnReset, "OnReset"},
    {UpdateService_ApplyTime::AtMaintenanceWindowStart, "AtMaintenanceWindowStart"},
    {UpdateService_ApplyTime::InMaintenanceWindowOnReset, "InMaintenanceWindowOnReset"},
    {UpdateService_ApplyTime::OnStartUpdateRequest, "OnStartUpdateRequest"},
});

enum class VirtualMedia_ConnectedVia{
    Invalid,
    NotConnected,
    URI,
    Applet,
    Oem,
};

NLOHMANN_JSON_SERIALIZE_ENUM(VirtualMedia_ConnectedVia, {
    {VirtualMedia_ConnectedVia::Invalid, "Invalid"},
    {VirtualMedia_ConnectedVia::NotConnected, "NotConnected"},
    {VirtualMedia_ConnectedVia::URI, "URI"},
    {VirtualMedia_ConnectedVia::Applet, "Applet"},
    {VirtualMedia_ConnectedVia::Oem, "Oem"},
});

enum class VirtualMedia_MediaType{
    Invalid,
    CD,
    Floppy,
    USBStick,
    DVD,
};

NLOHMANN_JSON_SERIALIZE_ENUM(VirtualMedia_MediaType, {
    {VirtualMedia_MediaType::Invalid, "Invalid"},
    {VirtualMedia_MediaType::CD, "CD"},
    {VirtualMedia_MediaType::Floppy, "Floppy"},
    {VirtualMedia_MediaType::USBStick, "USBStick"},
    {VirtualMedia_MediaType::DVD, "DVD"},
});

enum class VirtualMedia_TransferMethod{
    Invalid,
    Stream,
    Upload,
};

NLOHMANN_JSON_SERIALIZE_ENUM(VirtualMedia_TransferMethod, {
    {VirtualMedia_TransferMethod::Invalid, "Invalid"},
    {VirtualMedia_TransferMethod::Stream, "Stream"},
    {VirtualMedia_TransferMethod::Upload, "Upload"},
});

enum class VirtualMedia_TransferProtocolType{
    Invalid,
    CIFS,
    FTP,
    SFTP,
    HTTP,
    HTTPS,
    NFS,
    SCP,
    TFTP,
    OEM,
};

NLOHMANN_JSON_SERIALIZE_ENUM(VirtualMedia_TransferProtocolType, {
    {VirtualMedia_TransferProtocolType::Invalid, "Invalid"},
    {VirtualMedia_TransferProtocolType::CIFS, "CIFS"},
    {VirtualMedia_TransferProtocolType::FTP, "FTP"},
    {VirtualMedia_TransferProtocolType::SFTP, "SFTP"},
    {VirtualMedia_TransferProtocolType::HTTP, "HTTP"},
    {VirtualMedia_TransferProtocolType::HTTPS, "HTTPS"},
    {VirtualMedia_TransferProtocolType::NFS, "NFS"},
    {VirtualMedia_TransferProtocolType::SCP, "SCP"},
    {VirtualMedia_TransferProtocolType::TFTP, "TFTP"},
    {VirtualMedia_TransferProtocolType::OEM, "OEM"},
});

enum class Volume_EncryptionTypes{
    Invalid,
    NativeDriveEncryption,
    ControllerAssisted,
    SoftwareAssisted,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Volume_EncryptionTypes, {
    {Volume_EncryptionTypes::Invalid, "Invalid"},
    {Volume_EncryptionTypes::NativeDriveEncryption, "NativeDriveEncryption"},
    {Volume_EncryptionTypes::ControllerAssisted, "ControllerAssisted"},
    {Volume_EncryptionTypes::SoftwareAssisted, "SoftwareAssisted"},
});

enum class Volume_InitializeMethod{
    Invalid,
    Skip,
    Background,
    Foreground,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Volume_InitializeMethod, {
    {Volume_InitializeMethod::Invalid, "Invalid"},
    {Volume_InitializeMethod::Skip, "Skip"},
    {Volume_InitializeMethod::Background, "Background"},
    {Volume_InitializeMethod::Foreground, "Foreground"},
});

enum class Volume_InitializeType{
    Invalid,
    Fast,
    Slow,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Volume_InitializeType, {
    {Volume_InitializeType::Invalid, "Invalid"},
    {Volume_InitializeType::Fast, "Fast"},
    {Volume_InitializeType::Slow, "Slow"},
});

enum class Volume_RAIDType{
    Invalid,
    RAID0,
    RAID1,
    RAID3,
    RAID4,
    RAID5,
    RAID6,
    RAID10,
    RAID01,
    RAID6TP,
    RAID1E,
    RAID50,
    RAID60,
    RAID00,
    RAID10E,
    RAID1Triple,
    RAID10Triple,
    None,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Volume_RAIDType, {
    {Volume_RAIDType::Invalid, "Invalid"},
    {Volume_RAIDType::RAID0, "RAID0"},
    {Volume_RAIDType::RAID1, "RAID1"},
    {Volume_RAIDType::RAID3, "RAID3"},
    {Volume_RAIDType::RAID4, "RAID4"},
    {Volume_RAIDType::RAID5, "RAID5"},
    {Volume_RAIDType::RAID6, "RAID6"},
    {Volume_RAIDType::RAID10, "RAID10"},
    {Volume_RAIDType::RAID01, "RAID01"},
    {Volume_RAIDType::RAID6TP, "RAID6TP"},
    {Volume_RAIDType::RAID1E, "RAID1E"},
    {Volume_RAIDType::RAID50, "RAID50"},
    {Volume_RAIDType::RAID60, "RAID60"},
    {Volume_RAIDType::RAID00, "RAID00"},
    {Volume_RAIDType::RAID10E, "RAID10E"},
    {Volume_RAIDType::RAID1Triple, "RAID1Triple"},
    {Volume_RAIDType::RAID10Triple, "RAID10Triple"},
    {Volume_RAIDType::None, "None"},
});

enum class Volume_ReadCachePolicyType{
    Invalid,
    ReadAhead,
    AdaptiveReadAhead,
    Off,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Volume_ReadCachePolicyType, {
    {Volume_ReadCachePolicyType::Invalid, "Invalid"},
    {Volume_ReadCachePolicyType::ReadAhead, "ReadAhead"},
    {Volume_ReadCachePolicyType::AdaptiveReadAhead, "AdaptiveReadAhead"},
    {Volume_ReadCachePolicyType::Off, "Off"},
});

enum class Volume_VolumeType{
    Invalid,
    RawDevice,
    NonRedundant,
    Mirrored,
    StripedWithParity,
    SpannedMirrors,
    SpannedStripesWithParity,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Volume_VolumeType, {
    {Volume_VolumeType::Invalid, "Invalid"},
    {Volume_VolumeType::RawDevice, "RawDevice"},
    {Volume_VolumeType::NonRedundant, "NonRedundant"},
    {Volume_VolumeType::Mirrored, "Mirrored"},
    {Volume_VolumeType::StripedWithParity, "StripedWithParity"},
    {Volume_VolumeType::SpannedMirrors, "SpannedMirrors"},
    {Volume_VolumeType::SpannedStripesWithParity, "SpannedStripesWithParity"},
});

enum class Volume_VolumeUsageType{
    Invalid,
    Data,
    SystemData,
    CacheOnly,
    SystemReserve,
    ReplicationReserve,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Volume_VolumeUsageType, {
    {Volume_VolumeUsageType::Invalid, "Invalid"},
    {Volume_VolumeUsageType::Data, "Data"},
    {Volume_VolumeUsageType::SystemData, "SystemData"},
    {Volume_VolumeUsageType::CacheOnly, "CacheOnly"},
    {Volume_VolumeUsageType::SystemReserve, "SystemReserve"},
    {Volume_VolumeUsageType::ReplicationReserve, "ReplicationReserve"},
});

enum class Volume_WriteCachePolicyType{
    Invalid,
    WriteThrough,
    ProtectedWriteBack,
    UnprotectedWriteBack,
    Off,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Volume_WriteCachePolicyType, {
    {Volume_WriteCachePolicyType::Invalid, "Invalid"},
    {Volume_WriteCachePolicyType::WriteThrough, "WriteThrough"},
    {Volume_WriteCachePolicyType::ProtectedWriteBack, "ProtectedWriteBack"},
    {Volume_WriteCachePolicyType::UnprotectedWriteBack, "UnprotectedWriteBack"},
    {Volume_WriteCachePolicyType::Off, "Off"},
});

enum class Volume_WriteCacheStateType{
    Invalid,
    Unprotected,
    Protected,
    Degraded,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Volume_WriteCacheStateType, {
    {Volume_WriteCacheStateType::Invalid, "Invalid"},
    {Volume_WriteCacheStateType::Unprotected, "Unprotected"},
    {Volume_WriteCacheStateType::Protected, "Protected"},
    {Volume_WriteCacheStateType::Degraded, "Degraded"},
});

enum class Volume_WriteHoleProtectionPolicyType{
    Invalid,
    Off,
    Journaling,
    DistributedLog,
    Oem,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Volume_WriteHoleProtectionPolicyType, {
    {Volume_WriteHoleProtectionPolicyType::Invalid, "Invalid"},
    {Volume_WriteHoleProtectionPolicyType::Off, "Off"},
    {Volume_WriteHoleProtectionPolicyType::Journaling, "Journaling"},
    {Volume_WriteHoleProtectionPolicyType::DistributedLog, "DistributedLog"},
    {Volume_WriteHoleProtectionPolicyType::Oem, "Oem"},
});

enum class Zone_ExternalAccessibility{
    Invalid,
    GloballyAccessible,
    NonZonedAccessible,
    ZoneOnly,
    NoInternalRouting,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Zone_ExternalAccessibility, {
    {Zone_ExternalAccessibility::Invalid, "Invalid"},
    {Zone_ExternalAccessibility::GloballyAccessible, "GloballyAccessible"},
    {Zone_ExternalAccessibility::NonZonedAccessible, "NonZonedAccessible"},
    {Zone_ExternalAccessibility::ZoneOnly, "ZoneOnly"},
    {Zone_ExternalAccessibility::NoInternalRouting, "NoInternalRouting"},
});

enum class Zone_ZoneType{
    Invalid,
    Default,
    ZoneOfEndpoints,
    ZoneOfZones,
    ZoneOfResourceBlocks,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Zone_ZoneType, {
    {Zone_ZoneType::Invalid, "Invalid"},
    {Zone_ZoneType::Default, "Default"},
    {Zone_ZoneType::ZoneOfEndpoints, "ZoneOfEndpoints"},
    {Zone_ZoneType::ZoneOfZones, "ZoneOfZones"},
    {Zone_ZoneType::ZoneOfResourceBlocks, "ZoneOfResourceBlocks"},
});

// clang-format on
