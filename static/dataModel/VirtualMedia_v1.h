#ifndef VIRTUALMEDIA_V1
#define VIRTUALMEDIA_V1

#include "CertificateCollection_v1.h"
#include "Resource_v1.h"
#include "VirtualMedia_v1.h"

enum class VirtualMediaV1ConnectedVia
{
    NotConnected,
    URI,
    Applet,
    Oem,
};
enum class VirtualMediaV1MediaType
{
    CD,
    Floppy,
    USBStick,
    DVD,
};
enum class VirtualMediaV1TransferMethod
{
    Stream,
    Upload,
};
enum class VirtualMediaV1TransferProtocolType
{
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
struct VirtualMediaV1OemActions
{};
struct VirtualMediaV1Actions
{
    VirtualMediaV1OemActions oem;
};
struct VirtualMediaV1VirtualMedia
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string imageName;
    std::string image;
    VirtualMediaV1MediaType mediaTypes;
    VirtualMediaV1ConnectedVia connectedVia;
    bool inserted;
    bool writeProtected;
    VirtualMediaV1Actions actions;
    std::string userName;
    std::string password;
    VirtualMediaV1TransferProtocolType transferProtocolType;
    VirtualMediaV1TransferMethod transferMethod;
    ResourceV1Resource status;
    CertificateCollectionV1CertificateCollection certificates;
    bool verifyCertificate;
};
#endif
