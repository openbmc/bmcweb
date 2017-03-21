#define  IOCTL_IO_READ    0x1103
#define  IOCTL_IO_WRITE   0x1104
#define  IOCTL_REAL_IO_READ    0x1106
#define  IOCTL_REAL_IO_WRITE   0x1107
#define  IOCTL_BIT_STREAM_BASE 0x1108
#define  IOCTL_TX_BIT_STREAM   0x1109
#define  IOCTL_GET_SOCKET      0x1110
#define  IOCTL_AUTOMODE_TRIGGER         0x1111
#define  IOCTL_PASS3_TRIGGER   0x1112
#define  IOCTL_I2C_READ        0x1113
#define  IOCTL_I2C_WRITE       0x1114
#define  IOCTL_FLAG_BUFFER_CLEAR       0x1115
#define  IOCTL_REMAP      0x1105
#define  RELOCATE_OFFSET  0x380

typedef struct _IO_ACCESS_DATA {
    unsigned char Type;
    unsigned long Address;
    unsigned long Data;
    unsigned long Value;
    unsigned long I2CValue;
    int      kernel_socket;
//    struct sockaddr_in address_svr;
} IO_ACCESS_DATA, *PIO_ACCESS_DATA;
/*
typedef struct _MAP_DATA {
    unsigned long virt_add;
    unsigned long phys_add;
    unsigned long size;
    pgprot_t      prot;
} MAP_DATA;
*/
