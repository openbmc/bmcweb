#pragma once

#include <ast_video_types.hpp>
#include <array>
#include <aspeed/JTABLES.H>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

namespace AstVideo {

struct COLOR_CACHE {
  COLOR_CACHE()
      : Color{0x008080, 0xFF8080, 0x808080, 0xC08080}, Index{0, 1, 2, 3} {}

  unsigned long Color[4];
  unsigned char Index[4];
  unsigned char BitMapBits{};
};

struct RGB {
  unsigned char B;
  unsigned char G;
  unsigned char R;
  unsigned char Reserved;
};

enum class JpgBlock {
  JPEG_NO_SKIP_CODE = 0x00,
  JPEG_SKIP_CODE = 0x08,

  JPEG_PASS2_CODE = 0x02,
  JPEG_SKIP_PASS2_CODE = 0x0A,

  LOW_JPEG_NO_SKIP_CODE = 0x04,
  LOW_JPEG_SKIP_CODE = 0x0C,

  VQ_NO_SKIP_1_COLOR_CODE = 0x05,
  VQ_SKIP_1_COLOR_CODE = 0x0D,

  VQ_NO_SKIP_2_COLOR_CODE = 0x06,
  VQ_SKIP_2_COLOR_CODE = 0x0E,

  VQ_NO_SKIP_4_COLOR_CODE = 0x07,
  VQ_SKIP_4_COLOR_CODE = 0x0F,

  FRAME_END_CODE = 0x09,

};

class AstJpegDecoder {
 public:
  AstJpegDecoder() {
    // TODO(ed) figure out how to init this in the constructor
    YUVBuffer.resize(1920 * 1200);
    OutBuffer.resize(1920 * 1200);
    for (auto &r : OutBuffer) {
      r.R = 0x00;
      r.G = 0x00;
      r.B = 0x00;
      r.Reserved = 0xAA;
    }

    int qfactor = 16;

    SCALEFACTOR = qfactor;
    SCALEFACTORUV = qfactor;
    ADVANCESCALEFACTOR = 16;
    ADVANCESCALEFACTORUV = 16;
    init_jpg_table();
  }

  void load_quant_table(std::array<long, 64> &quant_table) {
    float scalefactor[8] = {1.0f, 1.387039845f, 1.306562965f, 1.175875602f,
                            1.0f, 0.785694958f, 0.541196100f, 0.275899379f};
    uint8_t j, row, col;
    std::array<uint8_t, 64> tempQT{};

    // Load quantization coefficients from JPG file, scale them for DCT and
    // reorder
    // from zig-zag order
    switch (Y_selector) {
      case 0:
        std_luminance_qt = Tbl_000Y;
        break;
      case 1:
        std_luminance_qt = Tbl_014Y;
        break;
      case 2:
        std_luminance_qt = Tbl_029Y;
        break;
      case 3:
        std_luminance_qt = Tbl_043Y;
        break;
      case 4:
        std_luminance_qt = Tbl_057Y;
        break;
      case 5:
        std_luminance_qt = Tbl_071Y;
        break;
      case 6:
        std_luminance_qt = Tbl_086Y;
        break;
      case 7:
        std_luminance_qt = Tbl_100Y;
        break;
    }
    set_quant_table(std_luminance_qt, static_cast<uint8_t>(SCALEFACTOR),
                    tempQT);

    for (j = 0; j <= 63; j++) {
      quant_table[j] = tempQT[zigzag[j]];
    }
    j = 0;
    for (row = 0; row <= 7; row++) {
      for (col = 0; col <= 7; col++) {
        quant_table[j] = static_cast<long>(
            (quant_table[j] * scalefactor[row] * scalefactor[col]) * 65536);
        j++;
      }
    }
    byte_pos += 64;
  }

  void load_quant_tableCb(std::array<long, 64> &quant_table) {
    float scalefactor[8] = {1.0f, 1.387039845f, 1.306562965f, 1.175875602f,
                            1.0f, 0.785694958f, 0.541196100f, 0.275899379f};
    uint8_t j, row, col;
    std::array<uint8_t, 64> tempQT{};

    // Load quantization coefficients from JPG file, scale them for DCT and
    // reorder from zig-zag order
    if (Mapping == 0) {
      switch (UV_selector) {
        case 0:
          std_chrominance_qt = Tbl_000Y;
          break;
        case 1:
          std_chrominance_qt = Tbl_014Y;
          break;
        case 2:
          std_chrominance_qt = Tbl_029Y;
          break;
        case 3:
          std_chrominance_qt = Tbl_043Y;
          break;
        case 4:
          std_chrominance_qt = Tbl_057Y;
          break;
        case 5:
          std_chrominance_qt = Tbl_071Y;
          break;
        case 6:
          std_chrominance_qt = Tbl_086Y;
          break;
        case 7:
          std_chrominance_qt = Tbl_100Y;
          break;
      }
    } else {
      switch (UV_selector) {
        case 0:
          std_chrominance_qt = Tbl_000UV;
          break;
        case 1:
          std_chrominance_qt = Tbl_014UV;
          break;
        case 2:
          std_chrominance_qt = Tbl_029UV;
          break;
        case 3:
          std_chrominance_qt = Tbl_043UV;
          break;
        case 4:
          std_chrominance_qt = Tbl_057UV;
          break;
        case 5:
          std_chrominance_qt = Tbl_071UV;
          break;
        case 6:
          std_chrominance_qt = Tbl_086UV;
          break;
        case 7:
          std_chrominance_qt = Tbl_100UV;
          break;
      }
    }
    set_quant_table(std_chrominance_qt, static_cast<uint8_t>(SCALEFACTORUV),
                    tempQT);

    for (j = 0; j <= 63; j++) {
      quant_table[j] = tempQT[zigzag[j]];
    }
    j = 0;
    for (row = 0; row <= 7; row++) {
      for (col = 0; col <= 7; col++) {
        quant_table[j] = static_cast<long>(
            (quant_table[j] * scalefactor[row] * scalefactor[col]) * 65536);
        j++;
      }
    }
    byte_pos += 64;
  }
  //  Note: Added for Dual_JPEG
  void load_advance_quant_table(std::array<long, 64> &quant_table) {
    float scalefactor[8] = {1.0f, 1.387039845f, 1.306562965f, 1.175875602f,
                            1.0f, 0.785694958f, 0.541196100f, 0.275899379f};
    uint8_t j, row, col;
    std::array<uint8_t, 64> tempQT{};

    // Load quantization coefficients from JPG file, scale them for DCT and
    // reorder
    // from zig-zag order
    switch (advance_selector) {
      case 0:
        std_luminance_qt = Tbl_000Y;
        break;
      case 1:
        std_luminance_qt = Tbl_014Y;
        break;
      case 2:
        std_luminance_qt = Tbl_029Y;
        break;
      case 3:
        std_luminance_qt = Tbl_043Y;
        break;
      case 4:
        std_luminance_qt = Tbl_057Y;
        break;
      case 5:
        std_luminance_qt = Tbl_071Y;
        break;
      case 6:
        std_luminance_qt = Tbl_086Y;
        break;
      case 7:
        std_luminance_qt = Tbl_100Y;
        break;
    }
    //  Note: pass ADVANCE SCALE FACTOR to sub-function in Dual-JPEG
    set_quant_table(std_luminance_qt, static_cast<uint8_t>(ADVANCESCALEFACTOR),
                    tempQT);

    for (j = 0; j <= 63; j++) {
      quant_table[j] = tempQT[zigzag[j]];
    }
    j = 0;
    for (row = 0; row <= 7; row++) {
      for (col = 0; col <= 7; col++) {
        quant_table[j] = static_cast<long>(
            (quant_table[j] * scalefactor[row] * scalefactor[col]) * 65536);
        j++;
      }
    }
    byte_pos += 64;
  }

  //  Note: Added for Dual-JPEG
  void load_advance_quant_tableCb(std::array<long, 64> &quant_table) {
    float scalefactor[8] = {1.0f, 1.387039845f, 1.306562965f, 1.175875602f,
                            1.0f, 0.785694958f, 0.541196100f, 0.275899379f};
    uint8_t j, row, col;
    std::array<uint8_t, 64> tempQT{};

    // Load quantization coefficients from JPG file, scale them for DCT and
    // reorder
    // from zig-zag order
    if (Mapping == 1) {
      switch (advance_selector) {
        case 0:
          std_chrominance_qt = Tbl_000Y;
          break;
        case 1:
          std_chrominance_qt = Tbl_014Y;
          break;
        case 2:
          std_chrominance_qt = Tbl_029Y;
          break;
        case 3:
          std_chrominance_qt = Tbl_043Y;
          break;
        case 4:
          std_chrominance_qt = Tbl_057Y;
          break;
        case 5:
          std_chrominance_qt = Tbl_071Y;
          break;
        case 6:
          std_chrominance_qt = Tbl_086Y;
          break;
        case 7:
          std_chrominance_qt = Tbl_100Y;
          break;
      }
    } else {
      switch (advance_selector) {
        case 0:
          std_chrominance_qt = Tbl_000UV;
          break;
        case 1:
          std_chrominance_qt = Tbl_014UV;
          break;
        case 2:
          std_chrominance_qt = Tbl_029UV;
          break;
        case 3:
          std_chrominance_qt = Tbl_043UV;
          break;
        case 4:
          std_chrominance_qt = Tbl_057UV;
          break;
        case 5:
          std_chrominance_qt = Tbl_071UV;
          break;
        case 6:
          std_chrominance_qt = Tbl_086UV;
          break;
        case 7:
          std_chrominance_qt = Tbl_100UV;
          break;
      }
    }
    //  Note: pass ADVANCE SCALE FACTOR to sub-function in Dual-JPEG
    set_quant_table(std_chrominance_qt,
                    static_cast<uint8_t>(ADVANCESCALEFACTORUV), tempQT);

    for (j = 0; j <= 63; j++) {
      quant_table[j] = tempQT[zigzag[j]];
    }
    j = 0;
    for (row = 0; row <= 7; row++) {
      for (col = 0; col <= 7; col++) {
        quant_table[j] = static_cast<long>(
            (quant_table[j] * scalefactor[row] * scalefactor[col]) * 65536);
        j++;
      }
    }
    byte_pos += 64;
  }

  void IDCT_transform(short *coef, uint8_t *data, uint8_t nBlock) {
#define FIX_1_082392200 ((int)277) /* FIX(1.082392200) */
#define FIX_1_414213562 ((int)362) /* FIX(1.414213562) */
#define FIX_1_847759065 ((int)473) /* FIX(1.847759065) */
#define FIX_2_613125930 ((int)669) /* FIX(2.613125930) */

#define MULTIPLY(var, cons) ((int)((var) * (cons)) >> 8)

    int tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
    int tmp10, tmp11, tmp12, tmp13;
    int z5, z10, z11, z12, z13;
    int workspace[64]; /* buffers data between passes */

    short *inptr = coef;
    long *quantptr;
    int *wsptr = workspace;
    unsigned char *outptr;
    unsigned char *r_limit = rlimit_table + 128;
    int ctr, dcval, DCTSIZE = 8;

    quantptr = &QT[nBlock][0];

    // Pass 1: process columns from input (inptr), store into work array(wsptr)

    for (ctr = 8; ctr > 0; ctr--) {
      /* Due to quantization, we will usually find that many of the input
          * coefficients are zero, especially the AC terms.  We can exploit this
          * by short-circuiting the IDCT calculation for any column in which all
          * the AC terms are zero.  In that case each output is equal to the
          * DC coefficient (with scale factor as needed).
          * With typical images and quantization tables, half or more of the
          * column DCT calculations can be simplified this way.
          */

      if ((inptr[DCTSIZE * 1] | inptr[DCTSIZE * 2] | inptr[DCTSIZE * 3] |
           inptr[DCTSIZE * 4] | inptr[DCTSIZE * 5] | inptr[DCTSIZE * 6] |
           inptr[DCTSIZE * 7]) == 0) {
        /* AC terms all zero */
        dcval = static_cast<int>((inptr[DCTSIZE * 0] * quantptr[DCTSIZE * 0]) >>
                                 16);

        wsptr[DCTSIZE * 0] = dcval;
        wsptr[DCTSIZE * 1] = dcval;
        wsptr[DCTSIZE * 2] = dcval;
        wsptr[DCTSIZE * 3] = dcval;
        wsptr[DCTSIZE * 4] = dcval;
        wsptr[DCTSIZE * 5] = dcval;
        wsptr[DCTSIZE * 6] = dcval;
        wsptr[DCTSIZE * 7] = dcval;

        inptr++; /* advance pointers to next column */
        quantptr++;
        wsptr++;
        continue;
      }

      /* Even part */

      tmp0 = (inptr[DCTSIZE * 0] * quantptr[DCTSIZE * 0]) >> 16;
      tmp1 = (inptr[DCTSIZE * 2] * quantptr[DCTSIZE * 2]) >> 16;
      tmp2 = (inptr[DCTSIZE * 4] * quantptr[DCTSIZE * 4]) >> 16;
      tmp3 = (inptr[DCTSIZE * 6] * quantptr[DCTSIZE * 6]) >> 16;

      tmp10 = tmp0 + tmp2; /* phase 3 */
      tmp11 = tmp0 - tmp2;

      tmp13 = tmp1 + tmp3;                                    /* phases 5-3 */
      tmp12 = MULTIPLY(tmp1 - tmp3, FIX_1_414213562) - tmp13; /* 2*c4 */

      tmp0 = tmp10 + tmp13; /* phase 2 */
      tmp3 = tmp10 - tmp13;
      tmp1 = tmp11 + tmp12;
      tmp2 = tmp11 - tmp12;

      /* Odd part */

      tmp4 = (inptr[DCTSIZE * 1] * quantptr[DCTSIZE * 1]) >> 16;
      tmp5 = (inptr[DCTSIZE * 3] * quantptr[DCTSIZE * 3]) >> 16;
      tmp6 = (inptr[DCTSIZE * 5] * quantptr[DCTSIZE * 5]) >> 16;
      tmp7 = (inptr[DCTSIZE * 7] * quantptr[DCTSIZE * 7]) >> 16;

      z13 = tmp6 + tmp5; /* phase 6 */
      z10 = tmp6 - tmp5;
      z11 = tmp4 + tmp7;
      z12 = tmp4 - tmp7;

      tmp7 = z11 + z13;                             /* phase 5 */
      tmp11 = MULTIPLY(z11 - z13, FIX_1_414213562); /* 2*c4 */

      z5 = MULTIPLY(z10 + z12, FIX_1_847759065);    /* 2*c2 */
      tmp10 = MULTIPLY(z12, FIX_1_082392200) - z5;  /* 2*(c2-c6) */
      tmp12 = MULTIPLY(z10, -FIX_2_613125930) + z5; /* -2*(c2+c6) */

      tmp6 = tmp12 - tmp7; /* phase 2 */
      tmp5 = tmp11 - tmp6;
      tmp4 = tmp10 + tmp5;

      wsptr[DCTSIZE * 0] = (tmp0 + tmp7);
      wsptr[DCTSIZE * 7] = (tmp0 - tmp7);
      wsptr[DCTSIZE * 1] = (tmp1 + tmp6);
      wsptr[DCTSIZE * 6] = (tmp1 - tmp6);
      wsptr[DCTSIZE * 2] = (tmp2 + tmp5);
      wsptr[DCTSIZE * 5] = (tmp2 - tmp5);
      wsptr[DCTSIZE * 4] = (tmp3 + tmp4);
      wsptr[DCTSIZE * 3] = (tmp3 - tmp4);

      inptr++; /* advance pointers to next column */
      quantptr++;
      wsptr++;
    }

/* Pass 2: process rows from work array, store into output array. */
/* Note that we must descale the results by a factor of 8 == 2**3, */
/* and also undo the PASS1_BITS scaling. */

//#define RANGE_MASK 1023; //2 bits wider than legal samples
#define PASS1_BITS 0
#define IDESCALE(x, n) ((int)((x) >> (n)))

    wsptr = workspace;
    for (ctr = 0; ctr < DCTSIZE; ctr++) {
      outptr = data + ctr * 8;

      /* Rows of zeroes can be exploited in the same way as we did with columns.
      * However, the column calculation has created many nonzero AC terms, so
      * the simplification applies less often (typically 5% to 10% of the time).
      * On machines with very fast multiplication, it's possible that the
      * test takes more time than it's worth.  In that case this section
      * may be commented out.
      */
      /* Even part */

      tmp10 = (wsptr[0] + wsptr[4]);
      tmp11 = (wsptr[0] - wsptr[4]);

      tmp13 = (wsptr[2] + wsptr[6]);
      tmp12 = MULTIPLY((int)wsptr[2] - (int)wsptr[6], FIX_1_414213562) - tmp13;

      tmp0 = tmp10 + tmp13;
      tmp3 = tmp10 - tmp13;
      tmp1 = tmp11 + tmp12;
      tmp2 = tmp11 - tmp12;

      /* Odd part */

      z13 = wsptr[5] + wsptr[3];
      z10 = wsptr[5] - wsptr[3];
      z11 = wsptr[1] + wsptr[7];
      z12 = wsptr[1] - wsptr[7];

      tmp7 = z11 + z13;                             /* phase 5 */
      tmp11 = MULTIPLY(z11 - z13, FIX_1_414213562); /* 2*c4 */

      z5 = MULTIPLY(z10 + z12, FIX_1_847759065);    /* 2*c2 */
      tmp10 = MULTIPLY(z12, FIX_1_082392200) - z5;  /* 2*(c2-c6) */
      tmp12 = MULTIPLY(z10, -FIX_2_613125930) + z5; /* -2*(c2+c6) */

      tmp6 = tmp12 - tmp7; /* phase 2 */
      tmp5 = tmp11 - tmp6;
      tmp4 = tmp10 + tmp5;

      /* Final output stage: scale down by a factor of 8 and range-limit */

      outptr[0] = r_limit[IDESCALE((tmp0 + tmp7), (PASS1_BITS + 3)) & 1023L];
      outptr[7] = r_limit[IDESCALE((tmp0 - tmp7), (PASS1_BITS + 3)) & 1023L];
      outptr[1] = r_limit[IDESCALE((tmp1 + tmp6), (PASS1_BITS + 3)) & 1023L];
      outptr[6] = r_limit[IDESCALE((tmp1 - tmp6), (PASS1_BITS + 3)) & 1023L];
      outptr[2] = r_limit[IDESCALE((tmp2 + tmp5), (PASS1_BITS + 3)) & 1023L];
      outptr[5] = r_limit[IDESCALE((tmp2 - tmp5), (PASS1_BITS + 3)) & 1023L];
      outptr[4] = r_limit[IDESCALE((tmp3 + tmp4), (PASS1_BITS + 3)) & 1023L];
      outptr[3] = r_limit[IDESCALE((tmp3 - tmp4), (PASS1_BITS + 3)) & 1023L];

      wsptr += DCTSIZE; /* advance pointer to next row */
    }
  }
  void YUVToRGB(
      int txb, int tyb,
      unsigned char
          *pYCbCr,       // in, Y: 256 or 64 bytes; Cb: 64 bytes; Cr: 64 bytes
      struct RGB *pYUV,  // in, Y: 256 or 64 bytes; Cb: 64 bytes; Cr: 64 bytes
      unsigned char
          *pBgr  // out, BGR format, 16*16*3 = 768 bytes; or 8*8*3=192 bytes
      ) {
    int i, j, pos, m, n;
    unsigned char cb, cr, *py, *pcb, *pcr, *py420[4];
    int y;
    struct RGB *pByte;
    int nBlocksInMcu = 6;
    unsigned int pixel_x, pixel_y;

    pByte = reinterpret_cast<struct RGB *>(pBgr);
    if (yuvmode == YuvMode::YUV444) {
      py = pYCbCr;
      pcb = pYCbCr + 64;
      pcr = pcb + 64;

      pixel_x = txb * 8;
      pixel_y = tyb * 8;
      pos = (pixel_y * WIDTH) + pixel_x;

      for (j = 0; j < 8; j++) {
        for (i = 0; i < 8; i++) {
          m = ((j << 3) + i);
          y = py[m];
          cb = pcb[m];
          cr = pcr[m];
          n = pos + i;
          // For 2Pass. Save the YUV value
          pYUV[n].B = cb;
          pYUV[n].G = y;
          pYUV[n].R = cr;
          pByte[n].B = rlimit_table[m_Y[y] + m_CbToB[cb]];
          pByte[n].G = rlimit_table[m_Y[y] + m_CbToG[cb] + m_CrToG[cr]];
          pByte[n].R = rlimit_table[m_Y[y] + m_CrToR[cr]];
        }
        pos += WIDTH;
      }
    } else {
      for (i = 0; i < nBlocksInMcu - 2; i++) {
        py420[i] = pYCbCr + i * 64;
      }
      pcb = pYCbCr + (nBlocksInMcu - 2) * 64;
      pcr = pcb + 64;

      pixel_x = txb * 16;
      pixel_y = tyb * 16;
      pos = (pixel_y * WIDTH) + pixel_x;

      for (j = 0; j < 16; j++) {
        for (i = 0; i < 16; i++) {
          //	block number is ((j/8) * 2 + i/8)={0, 1, 2, 3}
          y = *(py420[(j >> 3) * 2 + (i >> 3)]++);
          m = ((j >> 1) << 3) + (i >> 1);
          cb = pcb[m];
          cr = pcr[m];
          n = pos + i;
          pByte[n].B = rlimit_table[m_Y[y] + m_CbToB[cb]];
          pByte[n].G = rlimit_table[m_Y[y] + m_CbToG[cb] + m_CrToG[cr]];
          pByte[n].R = rlimit_table[m_Y[y] + m_CrToR[cr]];
        }
        pos += WIDTH;
      }
    }
  }
  void YUVToBuffer(
      int txb, int tyb,
      unsigned char
          *pYCbCr,  // in, Y: 256 or 64 bytes; Cb: 64 bytes; Cr: 64 bytes
      struct RGB
          *pYUV,  // out, BGR format, 16*16*3 = 768 bytes; or 8*8*3=192 bytes
      unsigned char
          *pBgr  // out, BGR format, 16*16*3 = 768 bytes; or 8*8*3=192 bytes
      ) {
    int i, j, pos, m, n;
    unsigned char cb, cr, *py, *pcb, *pcr, *py420[4];
    int y;
    struct RGB *pByte;
    int nBlocksInMcu = 6;
    unsigned int pixel_x, pixel_y;

    pByte = reinterpret_cast<struct RGB *>(pBgr);
    if (yuvmode == YuvMode::YUV444) {
      py = pYCbCr;
      pcb = pYCbCr + 64;
      pcr = pcb + 64;

      pixel_x = txb * 8;
      pixel_y = tyb * 8;
      pos = (pixel_y * WIDTH) + pixel_x;

      for (j = 0; j < 8; j++) {
        for (i = 0; i < 8; i++) {
          m = ((j << 3) + i);
          n = pos + i;
          y = pYUV[n].G + (py[m] - 128);
          cb = pYUV[n].B + (pcb[m] - 128);
          cr = pYUV[n].R + (pcr[m] - 128);
          pYUV[n].B = cb;
          pYUV[n].G = y;
          pYUV[n].R = cr;
          pByte[n].B = rlimit_table[m_Y[y] + m_CbToB[cb]];
          pByte[n].G = rlimit_table[m_Y[y] + m_CbToG[cb] + m_CrToG[cr]];
          pByte[n].R = rlimit_table[m_Y[y] + m_CrToR[cr]];
        }
        pos += WIDTH;
      }
    } else {
      for (i = 0; i < nBlocksInMcu - 2; i++) {
        py420[i] = pYCbCr + i * 64;
      }
      pcb = pYCbCr + (nBlocksInMcu - 2) * 64;
      pcr = pcb + 64;

      pixel_x = txb * 16;
      pixel_y = tyb * 16;
      pos = (pixel_y * WIDTH) + pixel_x;

      for (j = 0; j < 16; j++) {
        for (i = 0; i < 16; i++) {
          //	block number is ((j/8) * 2 + i/8)={0, 1, 2, 3}
          y = *(py420[(j >> 3) * 2 + (i >> 3)]++);
          m = ((j >> 1) << 3) + (i >> 1);
          cb = pcb[m];
          cr = pcr[m];
          n = pos + i;
          pByte[n].B = rlimit_table[m_Y[y] + m_CbToB[cb]];
          pByte[n].G = rlimit_table[m_Y[y] + m_CbToG[cb] + m_CrToG[cr]];
          pByte[n].R = rlimit_table[m_Y[y] + m_CrToR[cr]];
        }
        pos += WIDTH;
      }
    }
  }
  void Decompress(int txb, int tyb, char *outBuf, uint8_t QT_TableSelection) {
    unsigned char *ptr;
    unsigned char byTileYuv[768] = {};

    memset(DCT_coeff, 0, 384 * 2);
    ptr = byTileYuv;
    process_Huffman_data_unit(YDC_nr, YAC_nr, &DCY, 0);
    IDCT_transform(DCT_coeff, ptr, QT_TableSelection);
    ptr += 64;

    if (yuvmode == YuvMode::YUV420) {
      process_Huffman_data_unit(YDC_nr, YAC_nr, &DCY, 64);
      IDCT_transform(DCT_coeff + 64, ptr, QT_TableSelection);
      ptr += 64;

      process_Huffman_data_unit(YDC_nr, YAC_nr, &DCY, 128);
      IDCT_transform(DCT_coeff + 128, ptr, QT_TableSelection);
      ptr += 64;

      process_Huffman_data_unit(YDC_nr, YAC_nr, &DCY, 192);
      IDCT_transform(DCT_coeff + 192, ptr, QT_TableSelection);
      ptr += 64;

      process_Huffman_data_unit(CbDC_nr, CbAC_nr, &DCCb, 256);
      IDCT_transform(DCT_coeff + 256, ptr, QT_TableSelection + 1);
      ptr += 64;

      process_Huffman_data_unit(CrDC_nr, CrAC_nr, &DCCr, 320);
      IDCT_transform(DCT_coeff + 320, ptr, QT_TableSelection + 1);
    } else {
      process_Huffman_data_unit(CbDC_nr, CbAC_nr, &DCCb, 64);
      IDCT_transform(DCT_coeff + 64, ptr, QT_TableSelection + 1);
      ptr += 64;

      process_Huffman_data_unit(CrDC_nr, CrAC_nr, &DCCr, 128);
      IDCT_transform(DCT_coeff + 128, ptr, QT_TableSelection + 1);
    }

    //    YUVToRGB (txb, tyb, byTileYuv, (unsigned char *)outBuf);
    //  YUVBuffer for YUV record
    YUVToRGB(txb, tyb, byTileYuv, YUVBuffer.data(),
             reinterpret_cast<unsigned char *>(outBuf));
  }

  void Decompress_2PASS(int txb, int tyb, char *outBuf,
                        uint8_t QT_TableSelection) {
    unsigned char *ptr;
    unsigned char byTileYuv[768];
    memset(DCT_coeff, 0, 384 * 2);

    ptr = byTileYuv;
    process_Huffman_data_unit(YDC_nr, YAC_nr, &DCY, 0);
    IDCT_transform(DCT_coeff, ptr, QT_TableSelection);
    ptr += 64;

    process_Huffman_data_unit(CbDC_nr, CbAC_nr, &DCCb, 64);
    IDCT_transform(DCT_coeff + 64, ptr, QT_TableSelection + 1);
    ptr += 64;

    process_Huffman_data_unit(CrDC_nr, CrAC_nr, &DCCr, 128);
    IDCT_transform(DCT_coeff + 128, ptr, QT_TableSelection + 1);

    YUVToBuffer(txb, tyb, byTileYuv, YUVBuffer.data(),
                reinterpret_cast<unsigned char *>(outBuf));
    //    YUVToRGB (txb, tyb, byTileYuv, (unsigned char *)outBuf);
  }

  void VQ_Decompress(int txb, int tyb, char *outBuf, uint8_t QT_TableSelection,
                     struct COLOR_CACHE *VQ) {
    unsigned char *ptr, i;
    unsigned char byTileYuv[192];
    int Data;

    ptr = byTileYuv;
    if (VQ->BitMapBits == 0) {
      for (i = 0; i < 64; i++) {
        ptr[0] = (VQ->Color[VQ->Index[0]] & 0xFF0000) >> 16;
        ptr[64] = (VQ->Color[VQ->Index[0]] & 0x00FF00) >> 8;
        ptr[128] = VQ->Color[VQ->Index[0]] & 0x0000FF;
        ptr += 1;
      }
    } else {
      for (i = 0; i < 64; i++) {
        Data = static_cast<int>(lookKbits(VQ->BitMapBits));
        ptr[0] = (VQ->Color[VQ->Index[Data]] & 0xFF0000) >> 16;
        ptr[64] = (VQ->Color[VQ->Index[Data]] & 0x00FF00) >> 8;
        ptr[128] = VQ->Color[VQ->Index[Data]] & 0x0000FF;
        ptr += 1;
        skipKbits(VQ->BitMapBits);
      }
    }
    //    YUVToRGB (txb, tyb, byTileYuv, (unsigned char *)outBuf);
    YUVToRGB(txb, tyb, byTileYuv, YUVBuffer.data(),
             reinterpret_cast<unsigned char *>(outBuf));
  }

  void MoveBlockIndex() {
    if (yuvmode == YuvMode::YUV444) {
      txb++;
      if (txb >= static_cast<int>(WIDTH / 8)) {
        tyb++;
        if (tyb >= static_cast<int>(HEIGHT / 8)) {
          tyb = 0;
        }
        txb = 0;
      }
    } else {
      txb++;
      if (txb >= static_cast<int>(WIDTH / 16)) {
        tyb++;
        if (tyb >= static_cast<int>(HEIGHT / 16)) {
          tyb = 0;
        }
        txb = 0;
      }
    }
  }

  void Init_Color_Table() {
    int i, x;
    int nScale = 1L << 16;  // equal to power(2,16)
    int nHalf = nScale >> 1;

#define FIX(x) ((int)((x)*nScale + 0.5))

    /* i is the actual input pixel value, in the range 0..MAXJSAMPLE */
    /* The Cb or Cr value we are thinking of is x = i - CENTERJSAMPLE */
    /* Cr=>R value is nearest int to 1.597656 * x */
    /* Cb=>B value is nearest int to 2.015625 * x */
    /* Cr=>G value is scaled-up -0.8125 * x */
    /* Cb=>G value is scaled-up -0.390625 * x */
    for (i = 0, x = -128; i < 256; i++, x++) {
      m_CrToR[i] = (FIX(1.597656) * x + nHalf) >> 16;
      m_CbToB[i] = (FIX(2.015625) * x + nHalf) >> 16;
      m_CrToG[i] = (-FIX(0.8125) * x + nHalf) >> 16;
      m_CbToG[i] = (-FIX(0.390625) * x + nHalf) >> 16;
    }
    for (i = 0, x = -16; i < 256; i++, x++) {
      m_Y[i] = (FIX(1.164) * x + nHalf) >> 16;
    }
    // For Color Text Enchance Y Re-map. Recommend to disable in default
    /*
            for (i = 0; i < (VideoEngineInfo->INFData.Gamma1_Gamma2_Seperate);
       i++) {
                    temp = (double)i /
       VideoEngineInfo->INFData.Gamma1_Gamma2_Seperate;
                    temp1 = 1.0 / VideoEngineInfo->INFData.Gamma1Parameter;
                    m_Y[i] =
       (BYTE)(VideoEngineInfo->INFData.Gamma1_Gamma2_Seperate * pow (temp,
       temp1));
                    if (m_Y[i] > 255) m_Y[i] = 255;
            }
            for (i = (VideoEngineInfo->INFData.Gamma1_Gamma2_Seperate); i < 256;
       i++) {
                    m_Y[i] =
       (BYTE)((VideoEngineInfo->INFData.Gamma1_Gamma2_Seperate) + (256 -
       VideoEngineInfo->INFData.Gamma1_Gamma2_Seperate) * ( pow((double)((i -
       VideoEngineInfo->INFData.Gamma1_Gamma2_Seperate) / (256 -
       (VideoEngineInfo->INFData.Gamma1_Gamma2_Seperate))), (1.0 /
       VideoEngineInfo->INFData.Gamma2Parameter)) ));
                    if (m_Y[i] > 255) m_Y[i] = 255;
            }
    */
  }
  void load_Huffman_table(Huffman_table *HT, const unsigned char *nrcode,
                          const unsigned char *value,
                          const unsigned short int *Huff_code) {
    unsigned char k, j, i;
    unsigned int code, code_index;

    for (j = 1; j <= 16; j++) {
      HT->Length[j] = nrcode[j];
    }
    for (i = 0, k = 1; k <= 16; k++) {
      for (j = 0; j < HT->Length[k]; j++) {
        HT->V[WORD_hi_lo(k, j)] = value[i];
        i++;
      }
    }

    code = 0;
    for (k = 1; k <= 16; k++) {
      HT->minor_code[k] = static_cast<unsigned short int>(code);
      for (j = 1; j <= HT->Length[k]; j++) {
        code++;
      }
      HT->major_code[k] = static_cast<unsigned short int>(code - 1);
      code *= 2;
      if (HT->Length[k] == 0) {
        HT->minor_code[k] = 0xFFFF;
        HT->major_code[k] = 0;
      }
    }

    HT->Len[0] = 2;
    i = 2;

    for (code_index = 1; code_index < 65535; code_index++) {
      if (code_index < Huff_code[i]) {
        HT->Len[code_index] = static_cast<unsigned char>(Huff_code[i + 1]);
      } else {
        i = i + 2;
        HT->Len[code_index] = static_cast<unsigned char>(Huff_code[i + 1]);
      }
    }
  }
  void init_jpg_table() {
    Init_Color_Table();
    prepare_range_limit_table();
    load_Huffman_table(&HTDC[0], std_dc_luminance_nrcodes,
                       std_dc_luminance_values, DC_LUMINANCE_HUFFMANCODE);
    load_Huffman_table(&HTAC[0], std_ac_luminance_nrcodes,
                       std_ac_luminance_values, AC_LUMINANCE_HUFFMANCODE);
    load_Huffman_table(&HTDC[1], std_dc_chrominance_nrcodes,
                       std_dc_chrominance_values, DC_CHROMINANCE_HUFFMANCODE);
    load_Huffman_table(&HTAC[1], std_ac_chrominance_nrcodes,
                       std_ac_chrominance_values, AC_CHROMINANCE_HUFFMANCODE);
  }

  void prepare_range_limit_table()
  /* Allocate and fill in the sample_range_limit table */
  {
    int j;
    rlimit_table = reinterpret_cast<unsigned char *>(malloc(5 * 256L + 128));
    /* First segment of "simple" table: limit[x] = 0 for x < 0 */
    memset((void *)rlimit_table, 0, 256);
    rlimit_table += 256; /* allow negative subscripts of simple table */
    /* Main part of "simple" table: limit[x] = x */
    for (j = 0; j < 256; j++) {
      rlimit_table[j] = j;
    }
    /* End of simple table, rest of first half of post-IDCT table */
    for (j = 256; j < 640; j++) {
      rlimit_table[j] = 255;
    }

    /* Second half of post-IDCT table */
    memset((void *)(rlimit_table + 640), 0, 384);
    for (j = 0; j < 128; j++) {
      rlimit_table[j + 1024] = j;
    }
  }

  inline unsigned short int WORD_hi_lo(uint8_t byte_high, uint8_t byte_low) {
    return (byte_high << 8) + byte_low;
  }

  // river
  void process_Huffman_data_unit(uint8_t DC_nr, uint8_t AC_nr,
                                 signed short int *previous_DC,
                                 unsigned short int position) {
    uint8_t nr = 0;
    uint8_t k;
    unsigned short int tmp_Hcode;
    uint8_t size_val, count_0;
    unsigned short int *min_code;
    uint8_t *huff_values;
    uint8_t byte_temp;

    min_code = HTDC[DC_nr].minor_code;
    //   maj_code=HTDC[DC_nr].major_code;
    huff_values = HTDC[DC_nr].V;

    // DC
    k = HTDC[DC_nr].Len[static_cast<unsigned short int>(codebuf >> 16)];
    // river
    //	 tmp_Hcode=lookKbits(k);
    tmp_Hcode = static_cast<unsigned short int>(codebuf >> (32 - k));
    skipKbits(k);
    size_val = huff_values[WORD_hi_lo(
        k, static_cast<uint8_t>(tmp_Hcode - min_code[k]))];
    if (size_val == 0) {
      DCT_coeff[position + 0] = *previous_DC;
    } else {
      DCT_coeff[position + 0] = *previous_DC + getKbits(size_val);
      *previous_DC = DCT_coeff[position + 0];
    }

    // Second, AC coefficient decoding
    min_code = HTAC[AC_nr].minor_code;
    //   maj_code=HTAC[AC_nr].major_code;
    huff_values = HTAC[AC_nr].V;

    nr = 1;  // AC coefficient
    do {
      k = HTAC[AC_nr].Len[static_cast<unsigned short int>(codebuf >> 16)];
      tmp_Hcode = static_cast<unsigned short int>(codebuf >> (32 - k));
      skipKbits(k);

      byte_temp = huff_values[WORD_hi_lo(
          k, static_cast<uint8_t>(tmp_Hcode - min_code[k]))];
      size_val = byte_temp & 0xF;
      count_0 = byte_temp >> 4;
      if (size_val == 0) {
        if (count_0 != 0xF) {
          break;
        }
        nr += 16;
      } else {
        nr += count_0;  // skip count_0 zeroes
        DCT_coeff[position + dezigzag[nr++]] = getKbits(size_val);
      }
    } while (nr < 64);
  }

  unsigned short int lookKbits(uint8_t k) {
    unsigned short int revcode;

    revcode = static_cast<unsigned short int>(codebuf >> (32 - k));

    return (revcode);
  }

  void skipKbits(uint8_t k) {
    unsigned long readbuf;

    if ((newbits - k) <= 0) {
      readbuf = Buffer[buffer_index];
      buffer_index++;
      codebuf =
          (codebuf << k) | ((newbuf | (readbuf >> (newbits))) >> (32 - k));
      newbuf = readbuf << (k - newbits);
      newbits = 32 + newbits - k;
    } else {
      codebuf = (codebuf << k) | (newbuf >> (32 - k));
      newbuf = newbuf << k;
      newbits -= k;
    }
  }

  signed short int getKbits(uint8_t k) {
    signed short int signed_wordvalue;

    // river
    // signed_wordvalue=lookKbits(k);
    signed_wordvalue = static_cast<unsigned short int>(codebuf >> (32 - k));
    if (((1L << (k - 1)) & signed_wordvalue) == 0) {
      // neg_pow2 was previously defined as the below.  It seemed silly to keep
      // a table of values around for something
      // THat's relatively easy to compute, so it was replaced with the
      // appropriate math
      // signed_wordvalue = signed_wordvalue - (0xFFFF >> (16 - k));
      std::array<signed short int, 17> neg_pow2 = {
          0,    -1,   -3,    -7,    -15,   -31,   -63,    -127,
          -255, -511, -1023, -2047, -4095, -8191, -16383, -32767};

      signed_wordvalue = signed_wordvalue + neg_pow2[k];
    }
    skipKbits(k);
    return signed_wordvalue;
  }
  int init_JPG_decoding() {
    byte_pos = 0;
    load_quant_table(QT[0]);
    load_quant_tableCb(QT[1]);
    //  Note: Added for Dual-JPEG
    load_advance_quant_table(QT[2]);
    load_advance_quant_tableCb(QT[3]);
    return 1;
  }

  void set_quant_table(const uint8_t *basic_table, uint8_t scale_factor,
                       std::array<uint8_t, 64>& newtable)
  // Set quantization table and zigzag reorder it
  {
    uint8_t i;
    long temp;
    for (i = 0; i < 64; i++) {
      temp = (static_cast<long>(basic_table[i] * 16) / scale_factor);
      /* limit the values to the valid range */
      if (temp <= 0L) {
        temp = 1L;
      }
      if (temp > 255L) {
        temp = 255L; /* limit to baseline range if requested */
      }
      newtable[zigzag[i]] = static_cast<uint8_t>(temp);
    }
  }

  void updatereadbuf(uint32_t *codebuf, uint32_t *newbuf, int walks,
                     int *newbits, std::vector<uint32_t> &Buffer) {
    unsigned long readbuf;

    if ((*newbits - walks) <= 0) {
      readbuf = Buffer[buffer_index];
      buffer_index++;
      *codebuf = (*codebuf << walks) |
                 ((*newbuf | (readbuf >> (*newbits))) >> (32 - walks));
      *newbuf = readbuf << (walks - *newbits);
      *newbits = 32 + *newbits - walks;
    } else {
      *codebuf = (*codebuf << walks) | (*newbuf >> (32 - walks));
      *newbuf = *newbuf << walks;
      *newbits -= walks;
    }
  }

  uint32_t decode(std::vector<uint32_t> &buffer, unsigned long width,
                  unsigned long height, YuvMode yuvmode_in, int y_selector,
                  int uv_selector) {
    COLOR_CACHE Decode_Color;
    if (width != USER_WIDTH || height != USER_HEIGHT || yuvmode_in != yuvmode ||
        y_selector != Y_selector || uv_selector != UV_selector) {
      yuvmode = yuvmode_in;
      Y_selector = y_selector;    // 0-7
      UV_selector = uv_selector;  // 0-7
      USER_HEIGHT = height;
      USER_WIDTH = width;
      WIDTH = width;
      HEIGHT = height;

      // TODO(ed) Magic number section.  Document appropriately
      advance_selector = 0;  // 0-7
      Mapping = 0;           // 0 or 1

      if (yuvmode == YuvMode::YUV420) {
        if ((WIDTH % 16) != 0u) {
          WIDTH = WIDTH + 16 - (WIDTH % 16);
        }
        if ((HEIGHT % 16) != 0u) {
          HEIGHT = HEIGHT + 16 - (HEIGHT % 16);
        }
      } else {
        if ((WIDTH % 8) != 0u) {
          WIDTH = WIDTH + 8 - (WIDTH % 8);
        }
        if ((HEIGHT % 8) != 0u) {
          HEIGHT = HEIGHT + 8 - (HEIGHT % 8);
        }
      }

      init_JPG_decoding();
    }
    // TODO(ed) cleanup cruft
    Buffer = buffer.data();

    codebuf = buffer[0];
    newbuf = buffer[1];
    buffer_index = 2;

    txb = tyb = 0;
    newbits = 32;
    DCY = DCCb = DCCr = 0;

    static const uint32_t VQ_HEADER_MASK = 0x01;
    static const uint32_t VQ_NO_UPDATE_HEADER = 0x00;
    static const uint32_t VQ_UPDATE_HEADER = 0x01;
    static const int VQ_NO_UPDATE_LENGTH = 0x03;
    static const int VQ_UPDATE_LENGTH = 0x1B;
    static const uint32_t VQ_INDEX_MASK = 0x03;
    static const uint32_t VQ_COLOR_MASK = 0xFFFFFF;

    static const int BLOCK_AST2100_START_LENGTH = 0x04;
    static const int BLOCK_AST2100_SKIP_LENGTH = 20;  // S:1 H:3 X:8 Y:8

    do {
      auto block_header = static_cast<JpgBlock>((codebuf >> 28) & 0xFF);
      switch (block_header) {
        case JpgBlock::JPEG_NO_SKIP_CODE:
          updatereadbuf(&codebuf, &newbuf, BLOCK_AST2100_START_LENGTH, &newbits,
                        buffer);
          Decompress(txb, tyb, reinterpret_cast<char *>(OutBuffer.data()), 0);
          break;
        case JpgBlock::FRAME_END_CODE:
          return 0;
          break;
        case JpgBlock::JPEG_SKIP_CODE:

          txb = (codebuf & 0x0FF00000) >> 20;
          tyb = (codebuf & 0x0FF000) >> 12;

          updatereadbuf(&codebuf, &newbuf, BLOCK_AST2100_SKIP_LENGTH, &newbits,
                        buffer);
          Decompress(txb, tyb, reinterpret_cast<char *>(OutBuffer.data()), 0);
          break;
        case JpgBlock::VQ_NO_SKIP_1_COLOR_CODE:
          updatereadbuf(&codebuf, &newbuf, BLOCK_AST2100_START_LENGTH, &newbits,
                        buffer);
          Decode_Color.BitMapBits = 0;

          for (int i = 0; i < 1; i++) {
            Decode_Color.Index[i] = ((codebuf >> 29) & VQ_INDEX_MASK);
            if (((codebuf >> 31) & VQ_HEADER_MASK) == VQ_NO_UPDATE_HEADER) {
              updatereadbuf(&codebuf, &newbuf, VQ_NO_UPDATE_LENGTH, &newbits,
                            buffer);
            } else {
              Decode_Color.Color[Decode_Color.Index[i]] =
                  ((codebuf >> 5) & VQ_COLOR_MASK);
              updatereadbuf(&codebuf, &newbuf, VQ_UPDATE_LENGTH, &newbits,
                            buffer);
            }
          }
          VQ_Decompress(txb, tyb, reinterpret_cast<char *>(OutBuffer.data()), 0,
                        &Decode_Color);
          break;
        case JpgBlock::VQ_SKIP_1_COLOR_CODE:
          txb = (codebuf & 0x0FF00000) >> 20;
          tyb = (codebuf & 0x0FF000) >> 12;

          updatereadbuf(&codebuf, &newbuf, BLOCK_AST2100_SKIP_LENGTH, &newbits,
                        buffer);
          Decode_Color.BitMapBits = 0;

          for (int i = 0; i < 1; i++) {
            Decode_Color.Index[i] = ((codebuf >> 29) & VQ_INDEX_MASK);
            if (((codebuf >> 31) & VQ_HEADER_MASK) == VQ_NO_UPDATE_HEADER) {
              updatereadbuf(&codebuf, &newbuf, VQ_NO_UPDATE_LENGTH, &newbits,
                            buffer);
            } else {
              Decode_Color.Color[Decode_Color.Index[i]] =
                  ((codebuf >> 5) & VQ_COLOR_MASK);
              updatereadbuf(&codebuf, &newbuf, VQ_UPDATE_LENGTH, &newbits,
                            buffer);
            }
          }
          VQ_Decompress(txb, tyb, reinterpret_cast<char *>(OutBuffer.data()), 0,
                        &Decode_Color);
          break;

        case JpgBlock::VQ_NO_SKIP_2_COLOR_CODE:
          updatereadbuf(&codebuf, &newbuf, BLOCK_AST2100_START_LENGTH, &newbits,
                        buffer);
          Decode_Color.BitMapBits = 1;

          for (int i = 0; i < 2; i++) {
            Decode_Color.Index[i] = ((codebuf >> 29) & VQ_INDEX_MASK);
            if (((codebuf >> 31) & VQ_HEADER_MASK) == VQ_NO_UPDATE_HEADER) {
              updatereadbuf(&codebuf, &newbuf, VQ_NO_UPDATE_LENGTH, &newbits,
                            buffer);
            } else {
              Decode_Color.Color[Decode_Color.Index[i]] =
                  ((codebuf >> 5) & VQ_COLOR_MASK);
              updatereadbuf(&codebuf, &newbuf, VQ_UPDATE_LENGTH, &newbits,
                            buffer);
            }
          }
          VQ_Decompress(txb, tyb, reinterpret_cast<char *>(OutBuffer.data()), 0,
                        &Decode_Color);
          break;
        case JpgBlock::VQ_SKIP_2_COLOR_CODE:
          txb = (codebuf & 0x0FF00000) >> 20;
          tyb = (codebuf & 0x0FF000) >> 12;

          updatereadbuf(&codebuf, &newbuf, BLOCK_AST2100_SKIP_LENGTH, &newbits,
                        buffer);
          Decode_Color.BitMapBits = 1;

          for (int i = 0; i < 2; i++) {
            Decode_Color.Index[i] = ((codebuf >> 29) & VQ_INDEX_MASK);
            if (((codebuf >> 31) & VQ_HEADER_MASK) == VQ_NO_UPDATE_HEADER) {
              updatereadbuf(&codebuf, &newbuf, VQ_NO_UPDATE_LENGTH, &newbits,
                            buffer);
            } else {
              Decode_Color.Color[Decode_Color.Index[i]] =
                  ((codebuf >> 5) & VQ_COLOR_MASK);
              updatereadbuf(&codebuf, &newbuf, VQ_UPDATE_LENGTH, &newbits,
                            buffer);
            }
          }
          VQ_Decompress(txb, tyb, reinterpret_cast<char *>(OutBuffer.data()), 0,
                        &Decode_Color);

          break;
        case JpgBlock::VQ_NO_SKIP_4_COLOR_CODE:
          updatereadbuf(&codebuf, &newbuf, BLOCK_AST2100_START_LENGTH, &newbits,
                        buffer);
          Decode_Color.BitMapBits = 2;

          for (unsigned char &i : Decode_Color.Index) {
            i = ((codebuf >> 29) & VQ_INDEX_MASK);
            if (((codebuf >> 31) & VQ_HEADER_MASK) == VQ_NO_UPDATE_HEADER) {
              updatereadbuf(&codebuf, &newbuf, VQ_NO_UPDATE_LENGTH, &newbits,
                            buffer);
            } else {
              Decode_Color.Color[i] = ((codebuf >> 5) & VQ_COLOR_MASK);
              updatereadbuf(&codebuf, &newbuf, VQ_UPDATE_LENGTH, &newbits,
                            buffer);
            }
          }
          VQ_Decompress(txb, tyb, reinterpret_cast<char *>(OutBuffer.data()), 0,
                        &Decode_Color);

          break;

        case JpgBlock::VQ_SKIP_4_COLOR_CODE:
          txb = (codebuf & 0x0FF00000) >> 20;
          tyb = (codebuf & 0x0FF000) >> 12;

          updatereadbuf(&codebuf, &newbuf, BLOCK_AST2100_SKIP_LENGTH, &newbits,
                        buffer);
          Decode_Color.BitMapBits = 2;

          for (unsigned char &i : Decode_Color.Index) {
            i = ((codebuf >> 29) & VQ_INDEX_MASK);
            if (((codebuf >> 31) & VQ_HEADER_MASK) == VQ_NO_UPDATE_HEADER) {
              updatereadbuf(&codebuf, &newbuf, VQ_NO_UPDATE_LENGTH, &newbits,
                            buffer);
            } else {
              Decode_Color.Color[i] = ((codebuf >> 5) & VQ_COLOR_MASK);
              updatereadbuf(&codebuf, &newbuf, VQ_UPDATE_LENGTH, &newbits,
                            buffer);
            }
          }
          VQ_Decompress(txb, tyb, reinterpret_cast<char *>(OutBuffer.data()), 0,
                        &Decode_Color);

          break;
        case JpgBlock::JPEG_SKIP_PASS2_CODE:
          txb = (codebuf & 0x0FF00000) >> 20;
          tyb = (codebuf & 0x0FF000) >> 12;

          updatereadbuf(&codebuf, &newbuf, BLOCK_AST2100_SKIP_LENGTH, &newbits,
                        buffer);
          Decompress_2PASS(txb, tyb, reinterpret_cast<char *>(OutBuffer.data()),
                           2);

          break;
        default:
          // TODO(ed) propogate errors upstream
          return -1;
          break;
      }
      MoveBlockIndex();

    } while (buffer_index <= buffer.size());

    return -1;
  }

#ifdef cimg_version
  void dump_to_bitmap_file() {
    cimg_library::CImg<unsigned char> image(WIDTH, HEIGHT, 1, 3);
    for (int y = 0; y < WIDTH; y++) {
      for (int x = 0; x < HEIGHT; x++) {
        auto pixel = OutBuffer[x + (y * WIDTH)];
        image(x, y, 0) = pixel.R;
        image(x, y, 1) = pixel.G;
        image(x, y, 2) = pixel.B;
      }
    }
    image.save("/tmp/file2.bmp");
  }
#endif

 private:
  YuvMode yuvmode{};
  // WIDTH and HEIGHT are the modes your display used
  unsigned long WIDTH{};
  unsigned long HEIGHT{};
  unsigned long USER_WIDTH{};
  unsigned long USER_HEIGHT{};
  unsigned char Y_selector{};
  int SCALEFACTOR;
  int SCALEFACTORUV;
  int ADVANCESCALEFACTOR;
  int ADVANCESCALEFACTORUV;
  int Mapping{};
  unsigned char UV_selector{};
  unsigned char advance_selector{};
  int byte_pos{};  // current byte position

  // quantization tables, no more than 4 quantization tables
  std::array<std::array<long, 64>, 4> QT{};

  // DC huffman tables , no more than 4 (0..3)
  std::array<Huffman_table, 4> HTDC{};
  // AC huffman tables (0..3)
  std::array<Huffman_table, 4> HTAC{};
  std::array<int, 256> m_CrToR{};
  std::array<int, 256> m_CbToB{};
  std::array<int, 256> m_CrToG{};
  std::array<int, 256> m_CbToG{};
  std::array<int, 256> m_Y{};
  unsigned long buffer_index{};
  uint32_t codebuf{}, newbuf{}, readbuf{};
  const unsigned char *std_luminance_qt{};
  const uint8_t *std_chrominance_qt{};

  signed short int DCY{}, DCCb{}, DCCr{};  // Coeficientii DC pentru Y,Cb,Cr
  signed short int DCT_coeff[384]{};
  // std::vector<signed short int> DCT_coeff;  // Current DCT_coefficients
  // quantization table number for Y, Cb, Cr
  uint8_t YQ_nr = 0, CbQ_nr = 1, CrQ_nr = 1;
  // DC Huffman table number for Y,Cb, Cr
  uint8_t YDC_nr = 0, CbDC_nr = 1, CrDC_nr = 1;
  // AC Huffman table number for Y,Cb, Cr
  uint8_t YAC_nr = 0, CbAC_nr = 1, CrAC_nr = 1;
  int txb = 0;
  int tyb = 0;
  int newbits{};
  uint8_t *rlimit_table{};
  std::vector<RGB> YUVBuffer;
  // TODO(ed) this shouldn't exist.  It is cruft that needs cleaning up
  uint32_t *Buffer{};

 public:
  std::vector<RGB> OutBuffer;
};
}  // namespace AstVideo