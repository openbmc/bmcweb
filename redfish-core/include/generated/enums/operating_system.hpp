#pragma once
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

NLOHMANN_JSON_SERIALIZE_ENUM(ContainerEngineTypes, {
    {ContainerEngineTypes::Invalid, "Invalid"},
    {ContainerEngineTypes::Docker, "Docker"},
    {ContainerEngineTypes::containerd, "containerd"},
    {ContainerEngineTypes::CRIO, "CRIO"},
});

}
// clang-format on
