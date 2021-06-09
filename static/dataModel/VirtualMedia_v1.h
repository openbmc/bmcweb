#ifndef VIRTUALMEDIA_V1
#define VIRTUALMEDIA_V1

#include "Resource_v1.h"
#include "VirtualMedia_v1.h"

enum class VirtualMedia_v1_ConnectedVia {
    NotConnected,
    URI,
    Applet,
    Oem,
};
enum class VirtualMedia_v1_MediaType {
    CD,
    Floppy,
    USBStick,
    DVD,
};
enum class VirtualMedia_v1_TransferMethod {
    Stream,
    Upload,
};
enum class VirtualMedia_v1_TransferProtocolType {
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
struct VirtualMedia_v1_Actions
{
    VirtualMedia_v1_OemActions oem;
};
struct VirtualMedia_v1_OemActions
{
};
struct VirtualMedia_v1_VirtualMedia
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string imageName;
    std::string image;
    VirtualMedia_v1_MediaType mediaTypes;
    VirtualMedia_v1_ConnectedVia connectedVia;
    bool inserted;
    bool writeProtected;
    VirtualMedia_v1_Actions actions;
    std::string userName;
    std::string password;
    VirtualMedia_v1_TransferProtocolType transferProtocolType;
    VirtualMedia_v1_TransferMethod transferMethod;
};
#endif
