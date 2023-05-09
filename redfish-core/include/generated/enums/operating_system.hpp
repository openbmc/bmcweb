#pragma once
#include <boost/describe/enum.hpp>
#include <nlohmann/json.hpp>

namespace operating_system
{
// clang-format off

enum class OperatingSystemTypes{
    Invalid,
    Linux,
    Windows,
    Solaris,
    HPUX,
    AIX,
    BSD,
    macOS,
    IBMi,
    Hypervisor,
};

enum class VirtualMachineEngineTypes{
    Invalid,
    VMwareESX,
    HyperV,
    Xen,
    KVM,
    QEMU,
    VirtualBox,
    PowerVM,
};

enum class VirtualMachineImageTypes{
    Invalid,
    Raw,
    OVF,
    OVA,
    VHD,
    VMDK,
    VDI,
    QCOW,
    QCOW2,
};

enum class ContainerEngineTypes{
    Invalid,
    Docker,
    containerd,
    CRIO,
};

NLOHMANN_JSON_SERIALIZE_ENUM(OperatingSystemTypes, {
    {OperatingSystemTypes::Invalid, "Invalid"},
    {OperatingSystemTypes::Linux, "Linux"},
    {OperatingSystemTypes::Windows, "Windows"},
    {OperatingSystemTypes::Solaris, "Solaris"},
    {OperatingSystemTypes::HPUX, "HPUX"},
    {OperatingSystemTypes::AIX, "AIX"},
    {OperatingSystemTypes::BSD, "BSD"},
    {OperatingSystemTypes::macOS, "macOS"},
    {OperatingSystemTypes::IBMi, "IBMi"},
    {OperatingSystemTypes::Hypervisor, "Hypervisor"},
});

BOOST_DESCRIBE_ENUM(OperatingSystemTypes,

    Invalid,
    Linux,
    Windows,
    Solaris,
    HPUX,
    AIX,
    BSD,
    macOS,
    IBMi,
    Hypervisor,
);

NLOHMANN_JSON_SERIALIZE_ENUM(VirtualMachineEngineTypes, {
    {VirtualMachineEngineTypes::Invalid, "Invalid"},
    {VirtualMachineEngineTypes::VMwareESX, "VMwareESX"},
    {VirtualMachineEngineTypes::HyperV, "HyperV"},
    {VirtualMachineEngineTypes::Xen, "Xen"},
    {VirtualMachineEngineTypes::KVM, "KVM"},
    {VirtualMachineEngineTypes::QEMU, "QEMU"},
    {VirtualMachineEngineTypes::VirtualBox, "VirtualBox"},
    {VirtualMachineEngineTypes::PowerVM, "PowerVM"},
});

BOOST_DESCRIBE_ENUM(VirtualMachineEngineTypes,

    Invalid,
    VMwareESX,
    HyperV,
    Xen,
    KVM,
    QEMU,
    VirtualBox,
    PowerVM,
);

NLOHMANN_JSON_SERIALIZE_ENUM(VirtualMachineImageTypes, {
    {VirtualMachineImageTypes::Invalid, "Invalid"},
    {VirtualMachineImageTypes::Raw, "Raw"},
    {VirtualMachineImageTypes::OVF, "OVF"},
    {VirtualMachineImageTypes::OVA, "OVA"},
    {VirtualMachineImageTypes::VHD, "VHD"},
    {VirtualMachineImageTypes::VMDK, "VMDK"},
    {VirtualMachineImageTypes::VDI, "VDI"},
    {VirtualMachineImageTypes::QCOW, "QCOW"},
    {VirtualMachineImageTypes::QCOW2, "QCOW2"},
});

BOOST_DESCRIBE_ENUM(VirtualMachineImageTypes,

    Invalid,
    Raw,
    OVF,
    OVA,
    VHD,
    VMDK,
    VDI,
    QCOW,
    QCOW2,
);

NLOHMANN_JSON_SERIALIZE_ENUM(ContainerEngineTypes, {
    {ContainerEngineTypes::Invalid, "Invalid"},
    {ContainerEngineTypes::Docker, "Docker"},
    {ContainerEngineTypes::containerd, "containerd"},
    {ContainerEngineTypes::CRIO, "CRIO"},
});

BOOST_DESCRIBE_ENUM(ContainerEngineTypes,

    Invalid,
    Docker,
    containerd,
    CRIO,
);

}
// clang-format on
