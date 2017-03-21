#pragma once


#define PASS 0
#define TRUE 0
#define FALSE 1
#define FAIL 1
#define FIX 0
#define STEP 1
#define SEQ_ADDRESS_REGISTER 0x3C4
#define SEQ_DATA_REGISTER 0x3C5
#define CRTC_ADDRESS_REGISTER 0x3D4
#define CRTC_DATA_REGISTER 0x3D5
#define DAC_INDEX_REGISTER 0x3C8
#define DAC_DATA_REGISTER 0x3C9

#define VIDEOBASE_OFFSET 0x1E700000
#define KEY_CONTROL 0x00 + VIDEOBASE_OFFSET
#define VIDEOENGINE_SEQUENCE_CONTROL 0x04 + VIDEOBASE_OFFSET
#define VIDEOENGINE_PASS1_CONTROL 0x08 + VIDEOBASE_OFFSET
#define VIDEOENGINE_MODEDETECTIONSETTING_H 0x0C + VIDEOBASE_OFFSET
#define VIDEOENGINE_MODEDETECTIONSETTING_V 0x10 + VIDEOBASE_OFFSET
#define SCALE_FACTOR_REGISTER 0x14 + VIDEOBASE_OFFSET
#define SCALING_FILTER_PARAMETERS_1 0x18 + VIDEOBASE_OFFSET
#define SCALING_FILTER_PARAMETERS_2 0x1C + VIDEOBASE_OFFSET
#define SCALING_FILTER_PARAMETERS_3 0x20 + VIDEOBASE_OFFSET
#define SCALING_FILTER_PARAMETERS_4 0x24 + VIDEOBASE_OFFSET
#define MODEDETECTION_STATUS_READBACK 0x98 + VIDEOBASE_OFFSET
#define VIDEOPROCESSING_CONTROL 0x2C + VIDEOBASE_OFFSET
#define VIDEO_CAPTURE_WINDOW_SETTING 0x30 + VIDEOBASE_OFFSET
#define VIDEO_COMPRESS_WINDOW_SETTING 0x34 + VIDEOBASE_OFFSET
#define VIDEO_COMPRESS_READ 0x3C + VIDEOBASE_OFFSET
#define VIDEO_IN_BUFFER_BASEADDRESS 0x44 + VIDEOBASE_OFFSET
#define VIDEO_IN_BUFFER_OFFSET 0x48 + VIDEOBASE_OFFSET
#define VIDEOPROCESS_BUFFER_BASEADDRESS 0x4C + VIDEOBASE_OFFSET
#define VIDEOCOMPRESS_SOURCE_BUFFER_BASEADDRESS 0x44 + VIDEOBASE_OFFSET
#define VIDEOPROCESS_OFFSET 0x48 + VIDEOBASE_OFFSET
#define VIDEOPROCESS_REFERENCE_BUFFER_BASEADDRESS 0x4C + VIDEOBASE_OFFSET
#define FLAG_BUFFER_BASEADDRESS 0x50 + VIDEOBASE_OFFSET
#define VIDEO_COMPRESS_DESTINATION_BASEADDRESS 0x54 + VIDEOBASE_OFFSET
#define STREAM_BUFFER_SIZE_REGISTER 0x58 + VIDEOBASE_OFFSET
#define VIDEO_CAPTURE_BOUND_REGISTER 0x5C + VIDEOBASE_OFFSET
#define VIDEO_COMPRESS_CONTROL 0x60 + VIDEOBASE_OFFSET
#define VIDEO_QUANTIZATION_TABLE_REGISTER 0x64 + VIDEOBASE_OFFSET
#define BLOCK_SHARPNESS_DETECTION_CONTROL 0x6C + VIDEOBASE_OFFSET
#define POST_WRITE_BUFFER_DRAM_THRESHOLD 0x68 + VIDEOBASE_OFFSET
#define DETECTION_STATUS_REGISTER 0x98 + VIDEOBASE_OFFSET
#define H_DETECTION_STATUS 0x90 + VIDEOBASE_OFFSET
#define V_DETECTION_STATUS 0x94 + VIDEOBASE_OFFSET
#define VIDEO_CONTROL_REGISTER 0x300 + VIDEOBASE_OFFSET
#define VIDEO_INTERRUPT_CONTROL 0x304 + VIDEOBASE_OFFSET
#define VIDEO_INTERRUPT_STATUS 0x308 + VIDEOBASE_OFFSET
#define MODE_DETECTION_REGISTER 0x30C + VIDEOBASE_OFFSET

#define FRONT_BOUND_REGISTER 0x310 + VIDEOBASE_OFFSET
#define END_BOUND_REGISTER 0x314 + VIDEOBASE_OFFSET
#define CRC_1_REGISTER 0x320 + VIDEOBASE_OFFSET
#define CRC_2_REGISTER 0x324 + VIDEOBASE_OFFSET
#define REDUCE_BIT_REGISTER 0x328 + VIDEOBASE_OFFSET
#define BIOS_SCRATCH_REGISTER 0x34C + VIDEOBASE_OFFSET
#define COMPRESS_DATA_COUNT_REGISTER 0x70 + VIDEOBASE_OFFSET
#define COMPRESS_BLOCK_COUNT_REGISTER 0x74 + VIDEOBASE_OFFSET
#define VIDEO_SCRATCH_REGISTER_34C 0x34C + VIDEOBASE_OFFSET
#define VIDEO_SCRATCH_REGISTER_35C 0x35C + VIDEOBASE_OFFSET
#define RC4KEYS_REGISTER 0x400 + VIDEOBASE_OFFSET
#define VQHUFFMAN_TABLE_REGISTER 0x300 + VIDEOBASE_OFFSET

//  Parameters
#define SAMPLE_RATE 24000000.0
#define MODEDETECTION_VERTICAL_STABLE_MAXIMUM 0x6
#define MODEDETECTION_HORIZONTAL_STABLE_MAXIMUM 0x6
#define MODEDETECTION_VERTICAL_STABLE_THRESHOLD 0x2
#define MODEDETECTION_HORIZONTAL_STABLE_THRESHOLD 0x2
#define HORIZONTAL_SCALING_FILTER_PARAMETERS_LOW 0xFFFFFFFF
#define HORIZONTAL_SCALING_FILTER_PARAMETERS_HIGH 0xFFFFFFFF
#define VIDEO_WRITE_BACK_BUFFER_THRESHOLD_LOW 0x08
#define VIDEO_WRITE_BACK_BUFFER_THRESHOLD_HIGH 0x04
#define VQ_Y_LEVELS 0x10
#define VQ_UV_LEVELS 0x05
#define EXTERNAL_VIDEO_HSYNC_POLARITY 0x01
#define EXTERNAL_VIDEO_VSYNC_POLARITY 0x01
#define VIDEO_SOURCE_FROM 0x01
#define EXTERNAL_ANALOG_SOURCE 0x01
#define USE_intERNAL_TIMING_GENERATOR 0x01
#define WRITE_DATA_FORMAT 0x00
#define SET_BCD_TO_WHOLE_FRAME 0x01
#define ENABLE_VERTICAL_DOWN_SCALING 0x01
#define BCD_TOLERENCE 0xFF
#define BCD_START_BLOCK_XY 0x0
#define BCD_END_BLOCK_XY 0x3FFF
#define COLOR_DEPTH 16
#define BLOCK_SHARPNESS_DETECTION_HIGH_THRESHOLD 0xFF
#define BLOCK_SHARPNESS_DETECTION_LOE_THRESHOLD 0xFF
#define BLOCK_SHARPNESS_DETECTION_HIGH_COUNTS_THRESHOLD 0x3F
#define BLOCK_SHARPNESS_DETECTION_LOW_COUNTS_THRESHOLD 0x1F
#define VQTABLE_AUTO_GENERATE_BY_HARDWARE 0x0
#define VQTABLE_SELECTION 0x0
#define JPEG_COMPRESS_ONLY 0x0
#define DUAL_MODE_COMPRESS 0x1
#define BSD_H_AND_V 0x0
#define ENABLE_RC4_ENCRYPTION 0x1
#define BSD_ENABLE_HIGH_THRESHOLD_CHECK 0x0
#define VIDEO_PROCESS_AUTO_TRIGGER 0x0
#define VIDEO_COMPRESS_AUTO_TRIGGER 0x0
#define DIGITAL_SIGNAL 0x0
#define ANALOG_SIGNAL 0x1

/* AST_VIDEO_SCRATCH_35C	0x35C		Video Scratch Remap Read Back */
#define SCRATCH_VGA_PWR_STS_HSYNC (1 << 31)
#define SCRATCH_VGA_PWR_STS_VSYNC (1 << 30)
#define SCRATCH_VGA_ATTRIBTE_INDEX_BIT5 (1 << 29)
#define SCRATCH_VGA_MASK_REG (1 << 28)
#define SCRATCH_VGA_CRT_RST (1 << 27)
#define SCRATCH_VGA_SCREEN_OFF (1 << 26)
#define SCRATCH_VGA_RESET (1 << 25)
#define SCRATCH_VGA_ENABLE (1 << 24)

typedef struct _VIDEO_MODE_INFO {
  unsigned short X;
  unsigned short Y;
  unsigned short ColorDepth;
  unsigned short RefreshRate;
  unsigned char ModeIndex;
} VIDEO_MODE_INFO, *PVIDEO_MODE_INFO;

typedef struct _VQ_INFO {
  unsigned char Y[16];
  unsigned char U[32];
  unsigned char V[32];
  unsigned char NumberOfY;
  unsigned char NumberOfUV;
  unsigned char NumberOfInner;
  unsigned char NumberOfOuter;
} VQ_INFO, *PVQ_INFO;

typedef struct _HUFFMAN_TABLE {
  unsigned long HuffmanCode[32];
} HUFFMAN_TABLE, *PHUFFMAN_TABLE;

typedef struct _FRAME_HEADER {
  unsigned long StartCode;   // 0
  unsigned long FrameNumber; /// 4
  unsigned short HSize;      // 8
  unsigned short VSize;
  unsigned long Reserved[2];         // 12 13 14
  unsigned char DirectMode;          // 15
  unsigned char CompressionMode;     // 15
  unsigned char JPEGScaleFactor;     // 16
  unsigned char Y_JPEGTableSelector; // 18 [[[[
  unsigned char JPEGYUVTableMapping;
  unsigned char SharpModeSelection;
  unsigned char AdvanceTableSelector;
  unsigned char AdvanceScaleFactor;
  unsigned long NumberOfMB;
  unsigned char VQ_YLevel;
  unsigned char VQ_UVLevel;
  VQ_INFO VQVectors;
  unsigned char Mode420;
  unsigned char Visual_Lossless;
} FRAME_HEADER, *PFRAME_HEADER;

typedef struct _INF_DATA {
  unsigned char AST2500;
  unsigned char Input_Signale; // 0: internel vga, 1, ext digital, 2, ext analog
  unsigned char Trigger_Mode;  // 0: capture, 1, ext digital, 2, ext analog
  unsigned char DownScalingEnable;
  unsigned char DifferentialSetting;
  unsigned short AnalogDifferentialThreshold;
  unsigned short DigitalDifferentialThreshold;
  unsigned char AutoMode;
  unsigned char DirectMode; // 0: force sync mode 1: auto direct mode
  unsigned short DelayControl;
  unsigned char VQMode;
  unsigned char JPEG_FILE;
} INF_DATA, *PINF_DATA;

typedef struct _COMPRESS_DATA {
  unsigned long SourceFrameSize;
  unsigned long CompressSize;
  unsigned long HDebug;
  unsigned long VDebug;
} COMPRESS_DATA, *PCOMPRESS_DATA;

// VIDEO Engine Info
typedef struct _VIDEO_ENGINE_INFO {
  INF_DATA INFData;
  VIDEO_MODE_INFO SourceModeInfo;
  VIDEO_MODE_INFO DestinationModeInfo;
  VQ_INFO VQInfo;
  FRAME_HEADER FrameHeader;
  COMPRESS_DATA CompressData;
  unsigned char ChipVersion;
  unsigned char NoSignal;
} VIDEO_ENGINE_INFO, *PVIDEO_ENGINE_INFO;

typedef struct {
  unsigned short HorizontalActive;
  unsigned short VerticalActive;
  unsigned short RefreshRate;
  unsigned char ADCIndex1;
  unsigned char ADCIndex2;
  unsigned char ADCIndex3;
  unsigned char ADCIndex5;
  unsigned char ADCIndex6;
  unsigned char ADCIndex7;
  unsigned char ADCIndex8;
  unsigned char ADCIndex9;
  unsigned char ADCIndexA;
  unsigned char ADCIndexF;
  unsigned char ADCIndex15;
  int HorizontalShift;
  int VerticalShift;
} ADC_MODE;

typedef struct {
  unsigned short HorizontalTotal;
  unsigned short VerticalTotal;
  unsigned short HorizontalActive;
  unsigned short VerticalActive;
  unsigned char RefreshRate;
  double HorizontalFrequency;
  unsigned short HSyncTime;
  unsigned short HBackPorch;
  unsigned short VSyncTime;
  unsigned short VBackPorch;
  unsigned short HLeftBorder;
  unsigned short HRightBorder;
  unsigned short VBottomBorder;
  unsigned short VTopBorder;
  ADC_MODE AdcMode;
} VESA_MODE;



typedef struct {
  unsigned short HorizontalActive;
  unsigned short VerticalActive;
  unsigned short RefreshRateIndex;
  double PixelClock;
} INTERNAL_MODE;

typedef struct _TRANSFER_HEADER {
  unsigned long Data_Length;
  unsigned long Blocks_Changed;
  unsigned short User_Width;
  unsigned short User_Height;
  unsigned char Frist_frame;   // 1: first frame
  unsigned char Compress_type; // 0:aspeed mode, 1:jpeg mode
  unsigned char Trigger_mode;  // 0:capture, 1: compression, 2: buffer
  unsigned char Data_format;   // 0:DCT, 1:DCTwVQ2 color, 2:DCTwVQ4 color
  unsigned char RC4_Enable;
  unsigned char RC4_Reset; // no use
  unsigned char Y_Table;
  unsigned char UV_Table;
  unsigned char Mode_420;
  unsigned char Direct_Mode;
  unsigned char VQ_Mode;
  unsigned char Disable_VGA;
  unsigned char Differential_Enable;
  unsigned char Auto_Mode;
  unsigned char VGA_Status;
  unsigned char RC4State;
  unsigned char Advance_Table;
} TRANSFER_HEADER, *PTRANSFER_HEADER;
