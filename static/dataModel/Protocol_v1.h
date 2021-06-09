#ifndef PROTOCOL_V1
#define PROTOCOL_V1


enum class Protocol_v1_Protocol {
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
    OEM,
};
#endif
