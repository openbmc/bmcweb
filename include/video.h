/*
* The definition and structure for AST 2500 Video Capture Driver
* Portions Copyright (C) 2015 Insyde Software Corp.
*
* This program is free software; you can redistribute it and/or modify it
* under the terms and conditions of the GNU General Public License,
* version 2, as published by the Free Software Foundation.
*
* This program is distributed in the hope it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
* more details.
*/

#ifndef _VIDEO_H_
#define _VIDEO_H_

#define DEF_Y_TBL             4
#define DEF_UV_TBL            (7 | 0x10)

#define VIDEO_IOC_MAGIC       'i'
#define VIDIOCMCAPTURE        _IOW(VIDEO_IOC_MAGIC,  1, struct video_mmap*)
#define VIDIOCGFBUF           _IOR(VIDEO_IOC_MAGIC,  2, struct video_buffer*)
#define VIDIOCCAPTURE         _IOW(VIDEO_IOC_MAGIC,  3, unsigned long )
#define VIDIOCGCAPTURE        _IOR(VIDEO_IOC_MAGIC,  4, subcapture_info*)
#define VIDIOCSCAPTURE        _IOW(VIDEO_IOC_MAGIC,  5, subcapture_info*)
#define VIDIOCGSEQ            _IOR(VIDEO_IOC_MAGIC,  6, vseq_info*)
#define VIDIOCSSEQ            _IOW(VIDEO_IOC_MAGIC,  7, vseq_info*)
#define VIDIOCGVIN            _IOR(VIDEO_IOC_MAGIC,  8, vin_info*)
#define VIDIOCSVIN            _IOW(VIDEO_IOC_MAGIC,  9, vin_info*)
#define VIDIOCGPROC           _IOR(VIDEO_IOC_MAGIC, 10, vproc_info*)
#define VIDIOCSPROC           _IOW(VIDEO_IOC_MAGIC, 11, vproc_info*)
#define VIDIOCGCOMP           _IOR(VIDEO_IOC_MAGIC, 12, vcomp_info*)
#define VIDIOCSCOMP           _IOW(VIDEO_IOC_MAGIC, 13, vcomp_info*)
#define VIDIOCDBG             _IOR(VIDEO_IOC_MAGIC, 14, unsigned int*)
#define VIDIOCIMGREFRESH      _IOW(VIDEO_IOC_MAGIC, 15, unsigned int*)

#define VIDEO_IOC_MAXNR       15

#define DUAL_COMP_BUFFER
#define V_MASK

#define AST2500_DEVICEID      0x25032402
#define AST2400_DEVICEID      0x00002402

// FLAG_FRAME_SIZE = (x/8) * (y/8) * 4
// 144K:(1920/8)*(1200/8)*4 (current set 1M)
#define FLAG_FRAME_SIZE       0x100000 // 1*1024*1024
// PROC_FRAME_SIZE = X * Y * 4
// 9M:1920*1200*4,  8M:1600*1200*4,  5M:1280*1024*4,  3M:1024*768*4
#define PROC_FRAME_SIZE       0x900000 // 9*1024*1024
// According to VREG(VR_STREAM_BUF_SZ) Video Stream Buffer Size Register
// bit[2:0] = 7 (128KB), bit[4:3] = 3 (32 packets), So max compression data
// will be 128KB * 32 = 4 MB,
// 4M: (128*1024)*32
#define COMP_FRAME_SIZE       0x400000 // 4*1024*1024
// Total Video Mem Buffer Size
// 2 PROC + 2 COMP + 1 FLAG for Capture Eng, 1 PROC + 1 COMP + 1 FLAG for Jpeg Capture Eng
#define VIDEO_MEM_BUF_SIZE    0x2900000
// Define Video Memory Start Address According to Memory & Kernel Size
// Total:128M, 80M for Kernel
#define SDRAM_MEM_OFFSET_ADDR 0x80000000

#define SYS_MEM_SZ_128M       0
#define SYS_MEM_SZ_256M       1
#define SYS_MEM_SZ_512M       2
#define SYS_MEM_SZ_1024M      3

#define V_MEM_SZ_8M           0
#define V_MEM_SZ_16M          1
#define V_MEM_SZ_32M          2
#define V_MEM_SZ_64M          3

#define MODE_DETECT           0
#define MODE_CHANGE           1
#define MAX_NO_SYNC_CNT       100
#define V_BUSY_TIME_OUT       6           //time tick

// Register Offset
#define SDRAM_PHY_BASE        0x1E6E0000  //1E6E:0000-1E6E:1FFF
#define SCU_PHY_BASE          0x1E6E2000  //1E6E:2000-1E6E:2FFF
#define VIDEO_PHY_BASE        0x1E700000  //1E70:0000-1E7F:FFFF

// IRQ NUMBER
#define VIDEO_IRQ             7           // for video
#define CURSOR_IRQ            21          // for quick cursor

// Register Definition
#define VREG(x)               (*(volatile unsigned int *)(IO_ADDRESS(x + VIDEO_PHY_BASE)))
#define SDRAMREG(x)           (*(volatile unsigned int *)(IO_ADDRESS(x + SDRAM_PHY_BASE)))
#define SCUREG(x)             (*(volatile unsigned int *)(IO_ADDRESS(x + SCU_PHY_BASE)))

// Register Protection Key
#define SCU_UNLOCK_KEY        0x1688A8A8
#define MCR_UNLOCK_KEY        0xFC600309
#define VR_UNLOCK_KEY         0x1A038AA8

// SDRAM Memory Controller Register Offset
#define MCR_PROTECTION_KEY    0x000
#define MCR_CONFIGURATION     0x004
#define BACKWARD_SCU_MPLL     0x120

// System Control Unit Register Offset
#define SCU_PROTECTION_KEY    0x00
#define SCU_SYSTEM_RESET_CTL  0x04
#define SCU_CLOCK_SELECTION   0x08
#define SCU_CLK_STOP_CTL      0x0C
#define SCU_INTERRUPT_CTL     0x18
#define SCU_MPLL_PARAMETER    0x20
#define SCU_MISC1_CTL         0x2C
#define SCU_SOC_SCRATCH1      0x40
#define SCU_VGA_SCRATCH1      0x50
#define SCU_VGA_SCRATCH2      0x54
#define SCU_HW_STRAPPING      0x70
#define SCU_SILICON_REV_ID    0x7C
#define SCU_DEVICE_ID         0x1A4

// Video Register Offset
#define VR_PROTECT_KEY                0x000
#define VIDEO_SEQ_CTL                 0x004
#define VIDEO_PASS1_CTL               0x008
#define TIMING_GEN_SETTING1           0x00C // if VIDEO_CTL1[5] = 0
#define TIMING_GEN_SETTING2           0x010 // if VIDEO_CTL1[5] = 0
#define SCALING_FACTOR                0x014
#define SCALING_FILTER_PARAMETER0     0x018
#define SCALING_FILTER_PARAMETER1     0x01C
#define SCALING_FILTER_PARAMETER2     0x020
#define SCALING_FILTER_PARAMETER3     0x024
#define BCD_CTL                       0x02C
#define CAPTURING_WINDOW_SETTING      0x030
#define COMP_WINDOW_SETTING           0x034
#define COMP_STREAM_BUF_PROC_OFFSET   0x038
#define COMP_STREAM_BUF_READ_OFFSET   0x03C
#define CRC_BUF_BASE_ADDR             0x040
#define VIDEO_SOURCE_BUF1_BASE_ADDR   0x044
#define SOURCE_BUF_SCANLINE_OFFSET    0x048
#define VIDEO_SOURCE_BUF2_BASE_ADDR   0x04C
#define BCD_FLAG_BUF_BASE_ADDR        0x050
#define COMP_STREAM_BUF_BASE_ADDR     0x054
#define VIDEO_STREAM_BUF_SIZE         0x058
#define COMP_STREAM_BUF_WRITE_OFFSET  0x05C
#define VIDEO_COMP_CTL                0x060
#define JPEG_BIT_CTRL                 0x064
#define QUANTIZATION_VALUE            0x068
#define COPY_BUF_BASE_ADDR            0x06C
#define COMP_STREAM_SIZE              0x070
#define COMP_BLOCK_NUM                0x074
#define COMP_STREAM_BUF_END_OFFSET    0x078
#define COMP_FRAME_COUNTER            0x07C
#define USER_HDR_PARAM                0x080
#define SOURCE_L_R_EDGE_DETECT        0x090
#define SOURCE_T_B_EDGE_DETECT        0x094
#define MODE_DETECT_STATUS            0x098

/* Video Management Engine, i.e. 2nd Set Video Engine */
#define VM_SEQ_CTRL                   0x204
#define VM_PASS_CTRL                  0x208
#define VM_SCALING_FACTOR             0x214
#define VM_CAP_WINDOW_SETTING         0x230
#define VM_COMP_WINDOW_SETTING        0x234
#define VM_COMP_BUF_PROC_OFFSET       0x238
#define VM_COMP_BUF_READ_OFFSET       0x23C
#define VM_JPEG_HEADER_BUFF           0x240
#define VM_SOURCE_BUFF0               0x244
#define VM_SRC_BUF_SCANLINE_OFFSET    0x248
#define VM_COMPRESS_BUFF              0x254
#define VM_STREAM_SIZE                0x258
#define VM_COMPRESS_CTRL              0x260
#define VM_JPEG_BIT_CTRL              0x264
#define VM_QUANTIZATION_VALUE         0x268
#define VM_COPY_BUF_BASE_ADDR         0x26C
#define VM_COMP_STREAM_SIZE           0x070
#define VM_COMP_BLOCK_NUM             0x074
#define VM_COMP_STREAM_BUF_END_OFFSET 0x278
#define VM_USER_HDR_PARAM             0x280

#define VIDEO_PASS3_CTRL              0x300
#define INTERRUPT_CTL                 0x304
#define INTERRUPT_STATUS              0x308
#define MODE_DETECT_PARAMETER         0x30C
#define MEM_RESTRICT_START_ADDR       0x310
#define MEM_RESTRICT_END_ADDR         0x314
#define PRI_CRC_PARAMETER             0x320
#define SEC_CRC_PARAMETER             0x324
#define DATA_TRUNCATION               0x328
#define VGA_SCRATCH_REMAP1            0x340
#define VGA_SCRATCH_REMAP2            0x344
#define VGA_SCRATCH_REMAP3            0x348
#define VGA_SCRATCH_REMAP4            0x34C
#define VGA_SCRATCH_REMAP5            0x350
#define VGA_SCRATCH_REMAP6            0x354
#define VGA_SCRATCH_REMAP7            0x358
#define VGA_SCRATCH_REMAP8            0x35C
#define RC4KEYS_REGISTER              0x400 //0x400~0x4FC  RC4 Encryption Key Register #0~#63

//
// Cursor struct is used in User Mode
//
typedef struct _cursor_attribution_tag {
  unsigned int posX;
  unsigned int posY;
  unsigned int cur_width;
  unsigned int cur_height;
  unsigned int cur_type;      //0:mono 1:color 2:disappear cursor
  unsigned int cur_change_flag;
} AST_CUR_ATTRIBUTION_TAG;

//
// For storing Cursor Information
//
typedef struct _cursor_tag {
  AST_CUR_ATTRIBUTION_TAG attr;
  //unsigned char     icon[MAX_CUR_OFFSETX*MAX_CUR_OFFSETY*2];
  unsigned char *icon;       //[64*64*2];
} AST_CURSOR_TAG;

//
// For select image format, i.e. 422 JPG420, 444 JPG444, lumin/chrom table, 0 ~ 11, low to high
//
typedef struct _video_features {
  short jpg_fmt;             //422:JPG420, 444:JPG444
  short lumin_tbl;
  short chrom_tbl;
  short tolerance_noise;
  int w;
  int h;
  unsigned char *buf;
} FEATURES_TAG;

//
// For configure video engine control registers
//
typedef struct _image_info {
  short do_image_refresh;    // Action 0:motion 1:fullframe 2:quick cursor
  char qc_valid;             // quick cursor enable/disable
  unsigned int len;
  int crypttype;
  char cryptkey[16];
  union {
    FEATURES_TAG features;
    AST_CURSOR_TAG cursor_info;
  } parameter;
} IMAGE_INFO;

typedef struct _video_set_chk {
  short get_qc_info;         // Quick Cursor Info
  short do_image_refresh;
  FEATURES_TAG features;
} video_set_chk;

struct video_buffer {
  void *base;
  int height, width;
  int depth;
  int bytesperline;
};

//
// Data Structure for Video Buffer Layout
// kept information about video buffer layout, used in memory initialization.
//
typedef struct _buf_layout {
  int c_proc;
  int c_comp;
  unsigned int in;           // video in buffer
  unsigned int proc [2];     // old and current proc buffer
  unsigned int comp [2];     // compressed data buffer
  unsigned int flag;         // flag buffer
  unsigned int jpgproc;       // old and current proc buffer for jpeg
  unsigned int jpgcomp;       // compressed data buffer for jpeg
  unsigned int jpgflag;       // flag buffer for jpeg
} buf_layout;

//
// subcapture_info:
// linestep - byte per line after scaling, but 32 byte align
// step - scaling step, decrease 8 pixels a step after scaling.
// scaled_h - height after scaling.
// scaled_w - width after scaling.
// hor_factor, ver_factor - horizontal and vertical scaling factor
// init - initialized = 0, not initialized = 1, set by mode_detect
//
typedef struct _subcature_info {
  int linestep;
  short x, y, width, height, scaled_h, scaled_w;
  unsigned short hor_factor, ver_factor;
  short step, init;
} subcapture_info;

//
// video_hw_info:
// cap_seq - capture orders, CAP_PASSX and CAP_INIT_FRAME
// cap_int - interrupt to enable
// cur_idx - current idx of resolution table content.
//
// mode_change must be
// set by interrupt handler, and clear
// when capture start.
//

//FK Comment :
//interrupt handler for mode_chage
//capture start, mode_chage, clear
//cur_idx resolution state, Ex: 800x600 80Hz, ...
//cap_int interrupt enable
//cap_seq captor orders i.e. CAP_PASS1, CAP_PASS2..., or CAP_INIT_FRAME
#define MCAP_FRAME  2
typedef struct _mcap_info {
  int offset;
  int size;
} mcap_info;

typedef struct _video_hw_info {
  int cap_seq;
  mcap_info mcap [MCAP_FRAME];       // record each frame size of mcapture
  //volatile short width, height, fix_x, fix_y, cur_idx;
  short width, height, fix_x, fix_y, cur_idx;
  short max_h, max_w, min_h, min_w;
  short cap_int;

  char mode_changed;                 //1130 modify
  //volatile char mode_changed;      //1130 modify
  char cap_done, capturing, disconnect;  //, mode_changed; //, cur_idx;
  char can_capture;                  // prevent hardware error.
  subcapture_info subcap;
//  VIDEO_ENGINE_INFO ve_info;
} video_hw_info;

struct video_mmap {
  unsigned int frame; // Frame (0 - n) for double buffer
  int height, width;
  unsigned int format; // should be VIDEO_PALETTE_*
};

// Internal_Mode Table for resolution checking
typedef struct {
  unsigned short HorizontalActive;
  unsigned short VerticalActive;
  unsigned short RefreshRateIndex;
  unsigned int PixelClock;
} INTERNAL_MODE;

INTERNAL_MODE Internal_Mode [] = {
    // 1024x768
    { 1024, 768, 0, 65 },
    { 1024, 768, 1, 65 },
    { 1024, 768, 2, 75 },
    { 1024, 768, 3, 79 },
    { 1024, 768, 4, 95 },

    // 1280x1024
    { 1280, 1024, 0, 108 },
    { 1280, 1024, 1, 108 },
    { 1280, 1024, 2, 135 },
    { 1280, 1024, 3, 158 },

    // 1600x1200
    { 1600, 1200, 0, 162 },
    { 1600, 1200, 1, 162 },
    { 1600, 1200, 2, 176 },
    { 1600, 1200, 3, 189 },
    { 1600, 1200, 4, 203 },
    { 1600, 1200, 5, 230 },

    // 1920x1200 reduce blank
    { 1920, 1200, 0, 157 },
    { 1920, 1200, 1, 157 },
};

typedef enum vga_color_mode {
  VGA_NO_SIGNAL = 0,
  EGA_MODE,
  VGA_MODE,
  VGA_15BPP_MODE,
  VGA_16BPP_MODE,
  VGA_32BPP_MODE,
} color_mode;
#endif //_VIDEO_H_
