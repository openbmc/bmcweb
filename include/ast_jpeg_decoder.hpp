#pragma once

#include <aspeed/JTABLES.H>

#include <array>
#include <ast_video_types.hpp>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

namespace ast_video
{

struct ColorCache
{
    ColorCache() :
        color{0x008080, 0xFF8080, 0x808080, 0xC08080}, index{0, 1, 2, 3}
    {
    }

    unsigned long color[4];
    unsigned char index[4];
    unsigned char bitMapBits{};
};

struct RGB
{
    unsigned char b;
    unsigned char g;
    unsigned char r;
    unsigned char reserved;
};

enum class JpgBlock
{
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

class AstJpegDecoder
{
  public:
    AstJpegDecoder()
    {
        // TODO(ed) figure out how to init this in the constructor
        yuvBuffer.resize(1920 * 1200);
        outBuffer.resize(1920 * 1200);
        for (auto &r : outBuffer)
        {
            r.r = 0x00;
            r.g = 0x00;
            r.b = 0x00;
            r.reserved = 0xAA;
        }

        int qfactor = 16;

        scalefactor = qfactor;
        scalefactoruv = qfactor;
        advancescalefactor = 16;
        advancescalefactoruv = 16;
        initJpgTable();
    }

    void loadQuantTable(std::array<long, 64> &quant_table)
    {
        float scalefactorF[8] = {1.0f,         1.387039845f, 1.306562965f,
                                 1.175875602f, 1.0f,         0.785694958f,
                                 0.541196100f, 0.275899379f};
        uint8_t j, row, col;
        std::array<uint8_t, 64> tempQT{};

        // Load quantization coefficients from JPG file, scale them for DCT and
        // reorder
        // from zig-zag order
        switch (ySelector)
        {
            case 0:
                stdLuminanceQt = tbl000Y;
                break;
            case 1:
                stdLuminanceQt = tbl014Y;
                break;
            case 2:
                stdLuminanceQt = tbl029Y;
                break;
            case 3:
                stdLuminanceQt = tbl043Y;
                break;
            case 4:
                stdLuminanceQt = tbl057Y;
                break;
            case 5:
                stdLuminanceQt = tbl071Y;
                break;
            case 6:
                stdLuminanceQt = tbl086Y;
                break;
            case 7:
                stdLuminanceQt = tbl100Y;
                break;
        }
        setQuantTable(stdLuminanceQt, static_cast<uint8_t>(scalefactor),
                      tempQT);

        for (j = 0; j <= 63; j++)
        {
            quant_table[j] = tempQT[zigzag[j]];
        }
        j = 0;
        for (row = 0; row <= 7; row++)
        {
            for (col = 0; col <= 7; col++)
            {
                quant_table[j] = static_cast<long>(
                    (quant_table[j] * scalefactorF[row] * scalefactorF[col]) *
                    65536);
                j++;
            }
        }
        bytePos += 64;
    }

    void loadQuantTableCb(std::array<long, 64> &quant_table)
    {
        float scalefactor[8] = {1.0f, 1.387039845f, 1.306562965f, 1.175875602f,
                                1.0f, 0.785694958f, 0.541196100f, 0.275899379f};
        uint8_t j, row, col;
        std::array<uint8_t, 64> tempQT{};

        // Load quantization coefficients from JPG file, scale them for DCT and
        // reorder from zig-zag order
        if (mapping == 0)
        {
            switch (uvSelector)
            {
                case 0:
                    stdChrominanceQt = tbl000Y;
                    break;
                case 1:
                    stdChrominanceQt = tbl014Y;
                    break;
                case 2:
                    stdChrominanceQt = tbl029Y;
                    break;
                case 3:
                    stdChrominanceQt = tbl043Y;
                    break;
                case 4:
                    stdChrominanceQt = tbl057Y;
                    break;
                case 5:
                    stdChrominanceQt = tbl071Y;
                    break;
                case 6:
                    stdChrominanceQt = tbl086Y;
                    break;
                case 7:
                    stdChrominanceQt = tbl100Y;
                    break;
            }
        }
        else
        {
            switch (uvSelector)
            {
                case 0:
                    stdChrominanceQt = tbl000Uv;
                    break;
                case 1:
                    stdChrominanceQt = tbl014Uv;
                    break;
                case 2:
                    stdChrominanceQt = tbl029Uv;
                    break;
                case 3:
                    stdChrominanceQt = tbl043Uv;
                    break;
                case 4:
                    stdChrominanceQt = tbl057Uv;
                    break;
                case 5:
                    stdChrominanceQt = tbl071Uv;
                    break;
                case 6:
                    stdChrominanceQt = tbl086Uv;
                    break;
                case 7:
                    stdChrominanceQt = tbl100Uv;
                    break;
            }
        }
        setQuantTable(stdChrominanceQt, static_cast<uint8_t>(scalefactoruv),
                      tempQT);

        for (j = 0; j <= 63; j++)
        {
            quant_table[j] = tempQT[zigzag[j]];
        }
        j = 0;
        for (row = 0; row <= 7; row++)
        {
            for (col = 0; col <= 7; col++)
            {
                quant_table[j] = static_cast<long>(
                    (quant_table[j] * scalefactor[row] * scalefactor[col]) *
                    65536);
                j++;
            }
        }
        bytePos += 64;
    }
    //  Note: Added for Dual_JPEG
    void loadAdvanceQuantTable(std::array<long, 64> &quant_table)
    {
        float scalefactor[8] = {1.0f, 1.387039845f, 1.306562965f, 1.175875602f,
                                1.0f, 0.785694958f, 0.541196100f, 0.275899379f};
        uint8_t j, row, col;
        std::array<uint8_t, 64> tempQT{};

        // Load quantization coefficients from JPG file, scale them for DCT and
        // reorder
        // from zig-zag order
        switch (advanceSelector)
        {
            case 0:
                stdLuminanceQt = tbl000Y;
                break;
            case 1:
                stdLuminanceQt = tbl014Y;
                break;
            case 2:
                stdLuminanceQt = tbl029Y;
                break;
            case 3:
                stdLuminanceQt = tbl043Y;
                break;
            case 4:
                stdLuminanceQt = tbl057Y;
                break;
            case 5:
                stdLuminanceQt = tbl071Y;
                break;
            case 6:
                stdLuminanceQt = tbl086Y;
                break;
            case 7:
                stdLuminanceQt = tbl100Y;
                break;
        }
        //  Note: pass ADVANCE SCALE FACTOR to sub-function in Dual-JPEG
        setQuantTable(stdLuminanceQt, static_cast<uint8_t>(advancescalefactor),
                      tempQT);

        for (j = 0; j <= 63; j++)
        {
            quant_table[j] = tempQT[zigzag[j]];
        }
        j = 0;
        for (row = 0; row <= 7; row++)
        {
            for (col = 0; col <= 7; col++)
            {
                quant_table[j] = static_cast<long>(
                    (quant_table[j] * scalefactor[row] * scalefactor[col]) *
                    65536);
                j++;
            }
        }
        bytePos += 64;
    }

    //  Note: Added for Dual-JPEG
    void loadAdvanceQuantTableCb(std::array<long, 64> &quant_table)
    {
        float scalefactor[8] = {1.0f, 1.387039845f, 1.306562965f, 1.175875602f,
                                1.0f, 0.785694958f, 0.541196100f, 0.275899379f};
        uint8_t j, row, col;
        std::array<uint8_t, 64> tempQT{};

        // Load quantization coefficients from JPG file, scale them for DCT and
        // reorder
        // from zig-zag order
        if (mapping == 1)
        {
            switch (advanceSelector)
            {
                case 0:
                    stdChrominanceQt = tbl000Y;
                    break;
                case 1:
                    stdChrominanceQt = tbl014Y;
                    break;
                case 2:
                    stdChrominanceQt = tbl029Y;
                    break;
                case 3:
                    stdChrominanceQt = tbl043Y;
                    break;
                case 4:
                    stdChrominanceQt = tbl057Y;
                    break;
                case 5:
                    stdChrominanceQt = tbl071Y;
                    break;
                case 6:
                    stdChrominanceQt = tbl086Y;
                    break;
                case 7:
                    stdChrominanceQt = tbl100Y;
                    break;
            }
        }
        else
        {
            switch (advanceSelector)
            {
                case 0:
                    stdChrominanceQt = tbl000Uv;
                    break;
                case 1:
                    stdChrominanceQt = tbl014Uv;
                    break;
                case 2:
                    stdChrominanceQt = tbl029Uv;
                    break;
                case 3:
                    stdChrominanceQt = tbl043Uv;
                    break;
                case 4:
                    stdChrominanceQt = tbl057Uv;
                    break;
                case 5:
                    stdChrominanceQt = tbl071Uv;
                    break;
                case 6:
                    stdChrominanceQt = tbl086Uv;
                    break;
                case 7:
                    stdChrominanceQt = tbl100Uv;
                    break;
            }
        }
        //  Note: pass ADVANCE SCALE FACTOR to sub-function in Dual-JPEG
        setQuantTable(stdChrominanceQt,
                      static_cast<uint8_t>(advancescalefactoruv), tempQT);

        for (j = 0; j <= 63; j++)
        {
            quant_table[j] = tempQT[zigzag[j]];
        }
        j = 0;
        for (row = 0; row <= 7; row++)
        {
            for (col = 0; col <= 7; col++)
            {
                quant_table[j] = static_cast<long>(
                    (quant_table[j] * scalefactor[row] * scalefactor[col]) *
                    65536);
                j++;
            }
        }
        bytePos += 64;
    }

    void idctTransform(short *coef, uint8_t *data, uint8_t nBlock)
    {
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
        unsigned char *rLimit = rlimitTable + 128;
        int ctr, dcval, dctsize = 8;

        quantptr = &qt[nBlock][0];

        // Pass 1: process columns from input (inptr), store into work
        // array(wsptr)

        for (ctr = 8; ctr > 0; ctr--)
        {
            /* Due to quantization, we will usually find that many of the input
             * coefficients are zero, especially the AC terms.  We can exploit
             * this by short-circuiting the IDCT calculation for any column in
             * which all the AC terms are zero.  In that case each output is
             * equal to the DC coefficient (with scale factor as needed). With
             * typical images and quantization tables, half or more of the
             * column DCT calculations can be simplified this way.
             */

            if ((inptr[dctsize * 1] | inptr[dctsize * 2] | inptr[dctsize * 3] |
                 inptr[dctsize * 4] | inptr[dctsize * 5] | inptr[dctsize * 6] |
                 inptr[dctsize * 7]) == 0)
            {
                /* AC terms all zero */
                dcval = static_cast<int>(
                    (inptr[dctsize * 0] * quantptr[dctsize * 0]) >> 16);

                wsptr[dctsize * 0] = dcval;
                wsptr[dctsize * 1] = dcval;
                wsptr[dctsize * 2] = dcval;
                wsptr[dctsize * 3] = dcval;
                wsptr[dctsize * 4] = dcval;
                wsptr[dctsize * 5] = dcval;
                wsptr[dctsize * 6] = dcval;
                wsptr[dctsize * 7] = dcval;

                inptr++; /* advance pointers to next column */
                quantptr++;
                wsptr++;
                continue;
            }

            /* Even part */

            tmp0 = (inptr[dctsize * 0] * quantptr[dctsize * 0]) >> 16;
            tmp1 = (inptr[dctsize * 2] * quantptr[dctsize * 2]) >> 16;
            tmp2 = (inptr[dctsize * 4] * quantptr[dctsize * 4]) >> 16;
            tmp3 = (inptr[dctsize * 6] * quantptr[dctsize * 6]) >> 16;

            tmp10 = tmp0 + tmp2; /* phase 3 */
            tmp11 = tmp0 - tmp2;

            tmp13 = tmp1 + tmp3; /* phases 5-3 */
            tmp12 = MULTIPLY(tmp1 - tmp3, FIX_1_414213562) - tmp13; /* 2*c4 */

            tmp0 = tmp10 + tmp13; /* phase 2 */
            tmp3 = tmp10 - tmp13;
            tmp1 = tmp11 + tmp12;
            tmp2 = tmp11 - tmp12;

            /* Odd part */

            tmp4 = (inptr[dctsize * 1] * quantptr[dctsize * 1]) >> 16;
            tmp5 = (inptr[dctsize * 3] * quantptr[dctsize * 3]) >> 16;
            tmp6 = (inptr[dctsize * 5] * quantptr[dctsize * 5]) >> 16;
            tmp7 = (inptr[dctsize * 7] * quantptr[dctsize * 7]) >> 16;

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

            wsptr[dctsize * 0] = (tmp0 + tmp7);
            wsptr[dctsize * 7] = (tmp0 - tmp7);
            wsptr[dctsize * 1] = (tmp1 + tmp6);
            wsptr[dctsize * 6] = (tmp1 - tmp6);
            wsptr[dctsize * 2] = (tmp2 + tmp5);
            wsptr[dctsize * 5] = (tmp2 - tmp5);
            wsptr[dctsize * 4] = (tmp3 + tmp4);
            wsptr[dctsize * 3] = (tmp3 - tmp4);

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
        for (ctr = 0; ctr < dctsize; ctr++)
        {
            outptr = data + ctr * 8;

            /* Rows of zeroes can be exploited in the same way as we did with
             * columns. However, the column calculation has created many nonzero
             * AC terms, so the simplification applies less often (typically 5%
             * to 10% of the time). On machines with very fast multiplication,
             * it's possible that the test takes more time than it's worth.  In
             * that case this section may be commented out.
             */
            /* Even part */

            tmp10 = (wsptr[0] + wsptr[4]);
            tmp11 = (wsptr[0] - wsptr[4]);

            tmp13 = (wsptr[2] + wsptr[6]);
            tmp12 = MULTIPLY((int)wsptr[2] - (int)wsptr[6], FIX_1_414213562) -
                    tmp13;

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

            /* Final output stage: scale down by a factor of 8 and range-limit
             */

            outptr[0] =
                rLimit[IDESCALE((tmp0 + tmp7), (PASS1_BITS + 3)) & 1023L];
            outptr[7] =
                rLimit[IDESCALE((tmp0 - tmp7), (PASS1_BITS + 3)) & 1023L];
            outptr[1] =
                rLimit[IDESCALE((tmp1 + tmp6), (PASS1_BITS + 3)) & 1023L];
            outptr[6] =
                rLimit[IDESCALE((tmp1 - tmp6), (PASS1_BITS + 3)) & 1023L];
            outptr[2] =
                rLimit[IDESCALE((tmp2 + tmp5), (PASS1_BITS + 3)) & 1023L];
            outptr[5] =
                rLimit[IDESCALE((tmp2 - tmp5), (PASS1_BITS + 3)) & 1023L];
            outptr[4] =
                rLimit[IDESCALE((tmp3 + tmp4), (PASS1_BITS + 3)) & 1023L];
            outptr[3] =
                rLimit[IDESCALE((tmp3 - tmp4), (PASS1_BITS + 3)) & 1023L];

            wsptr += dctsize; /* advance pointer to next row */
        }
    }
    void yuvToRgb(
        int txb, int tyb,
        unsigned char
            *pYCbCr,      // in, Y: 256 or 64 bytes; Cb: 64 bytes; Cr: 64 bytes
        struct RGB *pYUV, // in, Y: 256 or 64 bytes; Cb: 64 bytes; Cr: 64 bytes
        unsigned char
            *pBgr // out, BGR format, 16*16*3 = 768 bytes; or 8*8*3=192 bytes
    )
    {
        int i, j, pos, m, n;
        unsigned char cb, cr, *py, *pcb, *pcr, *py420[4];
        int y;
        struct RGB *pByte;
        int nBlocksInMcu = 6;
        unsigned int pixelX, pixelY;

        pByte = reinterpret_cast<struct RGB *>(pBgr);
        if (yuvmode == YuvMode::YUV444)
        {
            py = pYCbCr;
            pcb = pYCbCr + 64;
            pcr = pcb + 64;

            pixelX = txb * 8;
            pixelY = tyb * 8;
            pos = (pixelY * width) + pixelX;

            for (j = 0; j < 8; j++)
            {
                for (i = 0; i < 8; i++)
                {
                    m = ((j << 3) + i);
                    y = py[m];
                    cb = pcb[m];
                    cr = pcr[m];
                    n = pos + i;
                    // For 2Pass. Save the YUV value
                    pYUV[n].b = cb;
                    pYUV[n].g = y;
                    pYUV[n].r = cr;
                    pByte[n].b = rlimitTable[mY[y] + mCbToB[cb]];
                    pByte[n].g = rlimitTable[mY[y] + mCbToG[cb] + mCrToG[cr]];
                    pByte[n].r = rlimitTable[mY[y] + mCrToR[cr]];
                }
                pos += width;
            }
        }
        else
        {
            for (i = 0; i < nBlocksInMcu - 2; i++)
            {
                py420[i] = pYCbCr + i * 64;
            }
            pcb = pYCbCr + (nBlocksInMcu - 2) * 64;
            pcr = pcb + 64;

            pixelX = txb * 16;
            pixelY = tyb * 16;
            pos = (pixelY * width) + pixelX;

            for (j = 0; j < 16; j++)
            {
                for (i = 0; i < 16; i++)
                {
                    //	block number is ((j/8) * 2 + i/8)={0, 1, 2, 3}
                    y = *(py420[(j >> 3) * 2 + (i >> 3)]++);
                    m = ((j >> 1) << 3) + (i >> 1);
                    cb = pcb[m];
                    cr = pcr[m];
                    n = pos + i;
                    pByte[n].b = rlimitTable[mY[y] + mCbToB[cb]];
                    pByte[n].g = rlimitTable[mY[y] + mCbToG[cb] + mCrToG[cr]];
                    pByte[n].r = rlimitTable[mY[y] + mCrToR[cr]];
                }
                pos += width;
            }
        }
    }
    void yuvToBuffer(
        int txb, int tyb,
        unsigned char
            *pYCbCr, // in, Y: 256 or 64 bytes; Cb: 64 bytes; Cr: 64 bytes
        struct RGB
            *pYUV, // out, BGR format, 16*16*3 = 768 bytes; or 8*8*3=192 bytes
        unsigned char
            *pBgr // out, BGR format, 16*16*3 = 768 bytes; or 8*8*3=192 bytes
    )
    {
        int i, j, pos, m, n;
        unsigned char cb, cr, *py, *pcb, *pcr, *py420[4];
        int y;
        struct RGB *pByte;
        int nBlocksInMcu = 6;
        unsigned int pixelX, pixelY;

        pByte = reinterpret_cast<struct RGB *>(pBgr);
        if (yuvmode == YuvMode::YUV444)
        {
            py = pYCbCr;
            pcb = pYCbCr + 64;
            pcr = pcb + 64;

            pixelX = txb * 8;
            pixelY = tyb * 8;
            pos = (pixelY * width) + pixelX;

            for (j = 0; j < 8; j++)
            {
                for (i = 0; i < 8; i++)
                {
                    m = ((j << 3) + i);
                    n = pos + i;
                    y = pYUV[n].g + (py[m] - 128);
                    cb = pYUV[n].b + (pcb[m] - 128);
                    cr = pYUV[n].r + (pcr[m] - 128);
                    pYUV[n].b = cb;
                    pYUV[n].g = y;
                    pYUV[n].r = cr;
                    pByte[n].b = rlimitTable[mY[y] + mCbToB[cb]];
                    pByte[n].g = rlimitTable[mY[y] + mCbToG[cb] + mCrToG[cr]];
                    pByte[n].r = rlimitTable[mY[y] + mCrToR[cr]];
                }
                pos += width;
            }
        }
        else
        {
            for (i = 0; i < nBlocksInMcu - 2; i++)
            {
                py420[i] = pYCbCr + i * 64;
            }
            pcb = pYCbCr + (nBlocksInMcu - 2) * 64;
            pcr = pcb + 64;

            pixelX = txb * 16;
            pixelY = tyb * 16;
            pos = (pixelY * width) + pixelX;

            for (j = 0; j < 16; j++)
            {
                for (i = 0; i < 16; i++)
                {
                    //	block number is ((j/8) * 2 + i/8)={0, 1, 2, 3}
                    y = *(py420[(j >> 3) * 2 + (i >> 3)]++);
                    m = ((j >> 1) << 3) + (i >> 1);
                    cb = pcb[m];
                    cr = pcr[m];
                    n = pos + i;
                    pByte[n].b = rlimitTable[mY[y] + mCbToB[cb]];
                    pByte[n].g = rlimitTable[mY[y] + mCbToG[cb] + mCrToG[cr]];
                    pByte[n].r = rlimitTable[mY[y] + mCrToR[cr]];
                }
                pos += width;
            }
        }
    }
    void decompress(int txb, int tyb, char *outBuf, uint8_t QT_TableSelection)
    {
        unsigned char *ptr;
        unsigned char byTileYuv[768] = {};

        memset(dctCoeff, 0, 384 * 2);
        ptr = byTileYuv;
        processHuffmanDataUnit(ydcNr, yacNr, &dcy, 0);
        idctTransform(dctCoeff, ptr, QT_TableSelection);
        ptr += 64;

        if (yuvmode == YuvMode::YUV420)
        {
            processHuffmanDataUnit(ydcNr, yacNr, &dcy, 64);
            idctTransform(dctCoeff + 64, ptr, QT_TableSelection);
            ptr += 64;

            processHuffmanDataUnit(ydcNr, yacNr, &dcy, 128);
            idctTransform(dctCoeff + 128, ptr, QT_TableSelection);
            ptr += 64;

            processHuffmanDataUnit(ydcNr, yacNr, &dcy, 192);
            idctTransform(dctCoeff + 192, ptr, QT_TableSelection);
            ptr += 64;

            processHuffmanDataUnit(cbDcNr, cbAcNr, &dcCb, 256);
            idctTransform(dctCoeff + 256, ptr, QT_TableSelection + 1);
            ptr += 64;

            processHuffmanDataUnit(crDcNr, crAcNr, &dcCr, 320);
            idctTransform(dctCoeff + 320, ptr, QT_TableSelection + 1);
        }
        else
        {
            processHuffmanDataUnit(cbDcNr, cbAcNr, &dcCb, 64);
            idctTransform(dctCoeff + 64, ptr, QT_TableSelection + 1);
            ptr += 64;

            processHuffmanDataUnit(crDcNr, crAcNr, &dcCr, 128);
            idctTransform(dctCoeff + 128, ptr, QT_TableSelection + 1);
        }

        //    yuvToRgb (txb, tyb, byTileYuv, (unsigned char *)outBuf);
        //  yuvBuffer for YUV record
        yuvToRgb(txb, tyb, byTileYuv, yuvBuffer.data(),
                 reinterpret_cast<unsigned char *>(outBuf));
    }

    void decompress2Pass(int txb, int tyb, char *outBuf,
                         uint8_t QT_TableSelection)
    {
        unsigned char *ptr;
        unsigned char byTileYuv[768];
        memset(dctCoeff, 0, 384 * 2);

        ptr = byTileYuv;
        processHuffmanDataUnit(ydcNr, yacNr, &dcy, 0);
        idctTransform(dctCoeff, ptr, QT_TableSelection);
        ptr += 64;

        processHuffmanDataUnit(cbDcNr, cbAcNr, &dcCb, 64);
        idctTransform(dctCoeff + 64, ptr, QT_TableSelection + 1);
        ptr += 64;

        processHuffmanDataUnit(crDcNr, crAcNr, &dcCr, 128);
        idctTransform(dctCoeff + 128, ptr, QT_TableSelection + 1);

        yuvToBuffer(txb, tyb, byTileYuv, yuvBuffer.data(),
                    reinterpret_cast<unsigned char *>(outBuf));
        //    yuvToRgb (txb, tyb, byTileYuv, (unsigned char *)outBuf);
    }

    void vqDecompress(int txb, int tyb, char *outBuf, uint8_t QT_TableSelection,
                      struct ColorCache *VQ)
    {
        unsigned char *ptr, i;
        unsigned char byTileYuv[192];
        int data;

        ptr = byTileYuv;
        if (VQ->bitMapBits == 0)
        {
            for (i = 0; i < 64; i++)
            {
                ptr[0] = (VQ->color[VQ->index[0]] & 0xFF0000) >> 16;
                ptr[64] = (VQ->color[VQ->index[0]] & 0x00FF00) >> 8;
                ptr[128] = VQ->color[VQ->index[0]] & 0x0000FF;
                ptr += 1;
            }
        }
        else
        {
            for (i = 0; i < 64; i++)
            {
                data = static_cast<int>(lookKbits(VQ->bitMapBits));
                ptr[0] = (VQ->color[VQ->index[data]] & 0xFF0000) >> 16;
                ptr[64] = (VQ->color[VQ->index[data]] & 0x00FF00) >> 8;
                ptr[128] = VQ->color[VQ->index[data]] & 0x0000FF;
                ptr += 1;
                skipKbits(VQ->bitMapBits);
            }
        }
        //    yuvToRgb (txb, tyb, byTileYuv, (unsigned char *)outBuf);
        yuvToRgb(txb, tyb, byTileYuv, yuvBuffer.data(),
                 reinterpret_cast<unsigned char *>(outBuf));
    }

    void moveBlockIndex()
    {
        if (yuvmode == YuvMode::YUV444)
        {
            txb++;
            if (txb >= static_cast<int>(width / 8))
            {
                tyb++;
                if (tyb >= static_cast<int>(height / 8))
                {
                    tyb = 0;
                }
                txb = 0;
            }
        }
        else
        {
            txb++;
            if (txb >= static_cast<int>(width / 16))
            {
                tyb++;
                if (tyb >= static_cast<int>(height / 16))
                {
                    tyb = 0;
                }
                txb = 0;
            }
        }
    }

    void initColorTable()
    {
        int i, x;
        int nScale = 1L << 16; // equal to power(2,16)
        int nHalf = nScale >> 1;

#define FIX(x) ((int)((x)*nScale + 0.5))

        /* i is the actual input pixel value, in the range 0..MAXJSAMPLE */
        /* The Cb or Cr value we are thinking of is x = i - CENTERJSAMPLE */
        /* Cr=>r value is nearest int to 1.597656 * x */
        /* Cb=>b value is nearest int to 2.015625 * x */
        /* Cr=>g value is scaled-up -0.8125 * x */
        /* Cb=>g value is scaled-up -0.390625 * x */
        for (i = 0, x = -128; i < 256; i++, x++)
        {
            mCrToR[i] = (FIX(1.597656) * x + nHalf) >> 16;
            mCbToB[i] = (FIX(2.015625) * x + nHalf) >> 16;
            mCrToG[i] = (-FIX(0.8125) * x + nHalf) >> 16;
            mCbToG[i] = (-FIX(0.390625) * x + nHalf) >> 16;
        }
        for (i = 0, x = -16; i < 256; i++, x++)
        {
            mY[i] = (FIX(1.164) * x + nHalf) >> 16;
        }
        // For color Text Enchance Y Re-map. Recommend to disable in default
        /*
                for (i = 0; i <
           (VideoEngineInfo->INFData.Gamma1_Gamma2_Seperate); i++) { temp =
           (double)i / VideoEngineInfo->INFData.Gamma1_Gamma2_Seperate; temp1
           = 1.0 / VideoEngineInfo->INFData.Gamma1Parameter; mY[i] =
           (BYTE)(VideoEngineInfo->INFData.Gamma1_Gamma2_Seperate * pow (temp,
           temp1));
                        if (mY[i] > 255) mY[i] = 255;
                }
                for (i = (VideoEngineInfo->INFData.Gamma1_Gamma2_Seperate); i <
           256; i++) { mY[i] =
           (BYTE)((VideoEngineInfo->INFData.Gamma1_Gamma2_Seperate) + (256 -
           VideoEngineInfo->INFData.Gamma1_Gamma2_Seperate) * ( pow((double)((i
           - VideoEngineInfo->INFData.Gamma1_Gamma2_Seperate) / (256 -
           (VideoEngineInfo->INFData.Gamma1_Gamma2_Seperate))), (1.0 /
           VideoEngineInfo->INFData.Gamma2Parameter)) ));
                        if (mY[i] > 255) mY[i] = 255;
                }
        */
    }
    void loadHuffmanTable(HuffmanTable *HT, const unsigned char *nrcode,
                          const unsigned char *value,
                          const unsigned short int *Huff_code)
    {
        unsigned char k, j, i;
        unsigned int code, codeIndex;

        for (j = 1; j <= 16; j++)
        {
            HT->length[j] = nrcode[j];
        }
        for (i = 0, k = 1; k <= 16; k++)
        {
            for (j = 0; j < HT->length[k]; j++)
            {
                HT->v[wordHiLo(k, j)] = value[i];
                i++;
            }
        }

        code = 0;
        for (k = 1; k <= 16; k++)
        {
            HT->minorCode[k] = static_cast<unsigned short int>(code);
            for (j = 1; j <= HT->length[k]; j++)
            {
                code++;
            }
            HT->majorCode[k] = static_cast<unsigned short int>(code - 1);
            code *= 2;
            if (HT->length[k] == 0)
            {
                HT->minorCode[k] = 0xFFFF;
                HT->majorCode[k] = 0;
            }
        }

        HT->len[0] = 2;
        i = 2;

        for (codeIndex = 1; codeIndex < 65535; codeIndex++)
        {
            if (codeIndex < Huff_code[i])
            {
                HT->len[codeIndex] =
                    static_cast<unsigned char>(Huff_code[i + 1]);
            }
            else
            {
                i = i + 2;
                HT->len[codeIndex] =
                    static_cast<unsigned char>(Huff_code[i + 1]);
            }
        }
    }
    void initJpgTable()
    {
        initColorTable();
        prepareRangeLimitTable();
        loadHuffmanTable(&htdc[0], stdDcLuminanceNrcodes, stdDcLuminanceValues,
                         dcLuminanceHuffmancode);
        loadHuffmanTable(&htac[0], stdAcLuminanceNrcodes, stdAcLuminanceValues,
                         acLuminanceHuffmancode);
        loadHuffmanTable(&htdc[1], stdDcChrominanceNrcodes,
                         stdDcChrominanceValues, dcChrominanceHuffmancode);
        loadHuffmanTable(&htac[1], stdAcChrominanceNrcodes,
                         stdAcChrominanceValues, acChrominanceHuffmancode);
    }

    void prepareRangeLimitTable()
    /* Allocate and fill in the sample_range_limit table */
    {
        int j;
        rlimitTable = reinterpret_cast<unsigned char *>(malloc(5 * 256L + 128));
        /* First segment of "simple" table: limit[x] = 0 for x < 0 */
        memset((void *)rlimitTable, 0, 256);
        rlimitTable += 256; /* allow negative subscripts of simple table */
        /* Main part of "simple" table: limit[x] = x */
        for (j = 0; j < 256; j++)
        {
            rlimitTable[j] = j;
        }
        /* End of simple table, rest of first half of post-IDCT table */
        for (j = 256; j < 640; j++)
        {
            rlimitTable[j] = 255;
        }

        /* Second half of post-IDCT table */
        memset((void *)(rlimitTable + 640), 0, 384);
        for (j = 0; j < 128; j++)
        {
            rlimitTable[j + 1024] = j;
        }
    }

    inline unsigned short int wordHiLo(uint8_t byte_high, uint8_t byte_low)
    {
        return (byte_high << 8) + byte_low;
    }

    // river
    void processHuffmanDataUnit(uint8_t DC_nr, uint8_t AC_nr,
                                signed short int *previous_DC,
                                unsigned short int position)
    {
        uint8_t nr = 0;
        uint8_t k;
        unsigned short int tmpHcode;
        uint8_t sizeVal, count0;
        unsigned short int *minCode;
        uint8_t *huffValues;
        uint8_t byteTemp;

        minCode = htdc[DC_nr].minorCode;
        //   maj_code=htdc[DC_nr].majorCode;
        huffValues = htdc[DC_nr].v;

        // DC
        k = htdc[DC_nr].len[static_cast<unsigned short int>(codebuf >> 16)];
        // river
        //	 tmp_Hcode=lookKbits(k);
        tmpHcode = static_cast<unsigned short int>(codebuf >> (32 - k));
        skipKbits(k);
        sizeVal = huffValues[wordHiLo(
            k, static_cast<uint8_t>(tmpHcode - minCode[k]))];
        if (sizeVal == 0)
        {
            dctCoeff[position + 0] = *previous_DC;
        }
        else
        {
            dctCoeff[position + 0] = *previous_DC + getKbits(sizeVal);
            *previous_DC = dctCoeff[position + 0];
        }

        // Second, AC coefficient decoding
        minCode = htac[AC_nr].minorCode;
        //   maj_code=htac[AC_nr].majorCode;
        huffValues = htac[AC_nr].v;

        nr = 1; // AC coefficient
        do
        {
            k = htac[AC_nr].len[static_cast<unsigned short int>(codebuf >> 16)];
            tmpHcode = static_cast<unsigned short int>(codebuf >> (32 - k));
            skipKbits(k);

            byteTemp = huffValues[wordHiLo(
                k, static_cast<uint8_t>(tmpHcode - minCode[k]))];
            sizeVal = byteTemp & 0xF;
            count0 = byteTemp >> 4;
            if (sizeVal == 0)
            {
                if (count0 != 0xF)
                {
                    break;
                }
                nr += 16;
            }
            else
            {
                nr += count0; // skip count_0 zeroes
                dctCoeff[position + dezigzag[nr++]] = getKbits(sizeVal);
            }
        } while (nr < 64);
    }

    unsigned short int lookKbits(uint8_t k)
    {
        unsigned short int revcode;

        revcode = static_cast<unsigned short int>(codebuf >> (32 - k));

        return (revcode);
    }

    void skipKbits(uint8_t k)
    {
        unsigned long readbuf;

        if ((newbits - k) <= 0)
        {
            readbuf = buffer[bufferIndex];
            bufferIndex++;
            codebuf = (codebuf << k) |
                      ((newbuf | (readbuf >> (newbits))) >> (32 - k));
            newbuf = readbuf << (k - newbits);
            newbits = 32 + newbits - k;
        }
        else
        {
            codebuf = (codebuf << k) | (newbuf >> (32 - k));
            newbuf = newbuf << k;
            newbits -= k;
        }
    }

    signed short int getKbits(uint8_t k)
    {
        signed short int signedWordvalue;

        // river
        // signed_wordvalue=lookKbits(k);
        signedWordvalue = static_cast<unsigned short int>(codebuf >> (32 - k));
        if (((1L << (k - 1)) & signedWordvalue) == 0)
        {
            // neg_pow2 was previously defined as the below.  It seemed silly to
            // keep a table of values around for something THat's relatively
            // easy to compute, so it was replaced with the appropriate math
            // signed_wordvalue = signed_wordvalue - (0xFFFF >> (16 - k));
            std::array<signed short int, 17> negPow2 = {
                0,    -1,   -3,    -7,    -15,   -31,   -63,    -127,
                -255, -511, -1023, -2047, -4095, -8191, -16383, -32767};

            signedWordvalue = signedWordvalue + negPow2[k];
        }
        skipKbits(k);
        return signedWordvalue;
    }
    int initJpgDecoding()
    {
        bytePos = 0;
        loadQuantTable(qt[0]);
        loadQuantTableCb(qt[1]);
        //  Note: Added for Dual-JPEG
        loadAdvanceQuantTable(qt[2]);
        loadAdvanceQuantTableCb(qt[3]);
        return 1;
    }

    void setQuantTable(const uint8_t *basic_table, uint8_t scale_factor,
                       std::array<uint8_t, 64> &newtable)
    // Set quantization table and zigzag reorder it
    {
        uint8_t i;
        long temp;
        for (i = 0; i < 64; i++)
        {
            temp = (static_cast<long>(basic_table[i] * 16) / scale_factor);
            /* limit the values to the valid range */
            if (temp <= 0L)
            {
                temp = 1L;
            }
            if (temp > 255L)
            {
                temp = 255L; /* limit to baseline range if requested */
            }
            newtable[zigzag[i]] = static_cast<uint8_t>(temp);
        }
    }

    void updatereadbuf(uint32_t *codebuf, uint32_t *newbuf, int walks,
                       int *newbits, std::vector<uint32_t> &buffer)
    {
        unsigned long readbuf;

        if ((*newbits - walks) <= 0)
        {
            readbuf = buffer[bufferIndex];
            bufferIndex++;
            *codebuf = (*codebuf << walks) |
                       ((*newbuf | (readbuf >> (*newbits))) >> (32 - walks));
            *newbuf = readbuf << (walks - *newbits);
            *newbits = 32 + *newbits - walks;
        }
        else
        {
            *codebuf = (*codebuf << walks) | (*newbuf >> (32 - walks));
            *newbuf = *newbuf << walks;
            *newbits -= walks;
        }
    }

    uint32_t decode(std::vector<uint32_t> &bufferVector, unsigned long width,
                    unsigned long height, YuvMode yuvmode_in, int ySelector,
                    int uvSelector)
    {
        ColorCache decodeColor;
        if (width != userWidth || height != userHeight ||
            yuvmode_in != yuvmode || ySelector != ySelector ||
            uvSelector != uvSelector)
        {
            yuvmode = yuvmode_in;
            ySelector = ySelector;   // 0-7
            uvSelector = uvSelector; // 0-7
            userHeight = height;
            userWidth = width;
            width = width;
            height = height;

            // TODO(ed) Magic number section.  Document appropriately
            advanceSelector = 0; // 0-7
            mapping = 0;         // 0 or 1

            if (yuvmode == YuvMode::YUV420)
            {
                if ((width % 16) != 0u)
                {
                    width = width + 16 - (width % 16);
                }
                if ((height % 16) != 0u)
                {
                    height = height + 16 - (height % 16);
                }
            }
            else
            {
                if ((width % 8) != 0u)
                {
                    width = width + 8 - (width % 8);
                }
                if ((height % 8) != 0u)
                {
                    height = height + 8 - (height % 8);
                }
            }

            initJpgDecoding();
        }
        // TODO(ed) cleanup cruft
        buffer = bufferVector.data();

        codebuf = bufferVector[0];
        newbuf = bufferVector[1];
        bufferIndex = 2;

        txb = tyb = 0;
        newbits = 32;
        dcy = dcCb = dcCr = 0;

        static const uint32_t vqHeaderMask = 0x01;
        static const uint32_t vqNoUpdateHeader = 0x00;
        static const uint32_t vqUpdateHeader = 0x01;
        static const int vqNoUpdateLength = 0x03;
        static const int vqUpdateLength = 0x1B;
        static const uint32_t vqIndexMask = 0x03;
        static const uint32_t vqColorMask = 0xFFFFFF;

        static const int blockAsT2100StartLength = 0x04;
        static const int blockAsT2100SkipLength = 20; // S:1 H:3 X:8 Y:8

        do
        {
            auto blockHeader = static_cast<JpgBlock>((codebuf >> 28) & 0xFF);
            switch (blockHeader)
            {
                case JpgBlock::JPEG_NO_SKIP_CODE:
                    updatereadbuf(&codebuf, &newbuf, blockAsT2100StartLength,
                                  &newbits, bufferVector);
                    decompress(txb, tyb,
                               reinterpret_cast<char *>(outBuffer.data()), 0);
                    break;
                case JpgBlock::FRAME_END_CODE:
                    return 0;
                    break;
                case JpgBlock::JPEG_SKIP_CODE:

                    txb = (codebuf & 0x0FF00000) >> 20;
                    tyb = (codebuf & 0x0FF000) >> 12;

                    updatereadbuf(&codebuf, &newbuf, blockAsT2100SkipLength,
                                  &newbits, bufferVector);
                    decompress(txb, tyb,
                               reinterpret_cast<char *>(outBuffer.data()), 0);
                    break;
                case JpgBlock::VQ_NO_SKIP_1_COLOR_CODE:
                    updatereadbuf(&codebuf, &newbuf, blockAsT2100StartLength,
                                  &newbits, bufferVector);
                    decodeColor.bitMapBits = 0;

                    for (int i = 0; i < 1; i++)
                    {
                        decodeColor.index[i] = ((codebuf >> 29) & vqIndexMask);
                        if (((codebuf >> 31) & vqHeaderMask) ==
                            vqNoUpdateHeader)
                        {
                            updatereadbuf(&codebuf, &newbuf, vqNoUpdateLength,
                                          &newbits, bufferVector);
                        }
                        else
                        {
                            decodeColor.color[decodeColor.index[i]] =
                                ((codebuf >> 5) & vqColorMask);
                            updatereadbuf(&codebuf, &newbuf, vqUpdateLength,
                                          &newbits, bufferVector);
                        }
                    }
                    vqDecompress(txb, tyb,
                                 reinterpret_cast<char *>(outBuffer.data()), 0,
                                 &decodeColor);
                    break;
                case JpgBlock::VQ_SKIP_1_COLOR_CODE:
                    txb = (codebuf & 0x0FF00000) >> 20;
                    tyb = (codebuf & 0x0FF000) >> 12;

                    updatereadbuf(&codebuf, &newbuf, blockAsT2100SkipLength,
                                  &newbits, bufferVector);
                    decodeColor.bitMapBits = 0;

                    for (int i = 0; i < 1; i++)
                    {
                        decodeColor.index[i] = ((codebuf >> 29) & vqIndexMask);
                        if (((codebuf >> 31) & vqHeaderMask) ==
                            vqNoUpdateHeader)
                        {
                            updatereadbuf(&codebuf, &newbuf, vqNoUpdateLength,
                                          &newbits, bufferVector);
                        }
                        else
                        {
                            decodeColor.color[decodeColor.index[i]] =
                                ((codebuf >> 5) & vqColorMask);
                            updatereadbuf(&codebuf, &newbuf, vqUpdateLength,
                                          &newbits, bufferVector);
                        }
                    }
                    vqDecompress(txb, tyb,
                                 reinterpret_cast<char *>(outBuffer.data()), 0,
                                 &decodeColor);
                    break;

                case JpgBlock::VQ_NO_SKIP_2_COLOR_CODE:
                    updatereadbuf(&codebuf, &newbuf, blockAsT2100StartLength,
                                  &newbits, bufferVector);
                    decodeColor.bitMapBits = 1;

                    for (int i = 0; i < 2; i++)
                    {
                        decodeColor.index[i] = ((codebuf >> 29) & vqIndexMask);
                        if (((codebuf >> 31) & vqHeaderMask) ==
                            vqNoUpdateHeader)
                        {
                            updatereadbuf(&codebuf, &newbuf, vqNoUpdateLength,
                                          &newbits, bufferVector);
                        }
                        else
                        {
                            decodeColor.color[decodeColor.index[i]] =
                                ((codebuf >> 5) & vqColorMask);
                            updatereadbuf(&codebuf, &newbuf, vqUpdateLength,
                                          &newbits, bufferVector);
                        }
                    }
                    vqDecompress(txb, tyb,
                                 reinterpret_cast<char *>(outBuffer.data()), 0,
                                 &decodeColor);
                    break;
                case JpgBlock::VQ_SKIP_2_COLOR_CODE:
                    txb = (codebuf & 0x0FF00000) >> 20;
                    tyb = (codebuf & 0x0FF000) >> 12;

                    updatereadbuf(&codebuf, &newbuf, blockAsT2100SkipLength,
                                  &newbits, bufferVector);
                    decodeColor.bitMapBits = 1;

                    for (int i = 0; i < 2; i++)
                    {
                        decodeColor.index[i] = ((codebuf >> 29) & vqIndexMask);
                        if (((codebuf >> 31) & vqHeaderMask) ==
                            vqNoUpdateHeader)
                        {
                            updatereadbuf(&codebuf, &newbuf, vqNoUpdateLength,
                                          &newbits, bufferVector);
                        }
                        else
                        {
                            decodeColor.color[decodeColor.index[i]] =
                                ((codebuf >> 5) & vqColorMask);
                            updatereadbuf(&codebuf, &newbuf, vqUpdateLength,
                                          &newbits, bufferVector);
                        }
                    }
                    vqDecompress(txb, tyb,
                                 reinterpret_cast<char *>(outBuffer.data()), 0,
                                 &decodeColor);

                    break;
                case JpgBlock::VQ_NO_SKIP_4_COLOR_CODE:
                    updatereadbuf(&codebuf, &newbuf, blockAsT2100StartLength,
                                  &newbits, bufferVector);
                    decodeColor.bitMapBits = 2;

                    for (unsigned char &i : decodeColor.index)
                    {
                        i = ((codebuf >> 29) & vqIndexMask);
                        if (((codebuf >> 31) & vqHeaderMask) ==
                            vqNoUpdateHeader)
                        {
                            updatereadbuf(&codebuf, &newbuf, vqNoUpdateLength,
                                          &newbits, bufferVector);
                        }
                        else
                        {
                            decodeColor.color[i] =
                                ((codebuf >> 5) & vqColorMask);
                            updatereadbuf(&codebuf, &newbuf, vqUpdateLength,
                                          &newbits, bufferVector);
                        }
                    }
                    vqDecompress(txb, tyb,
                                 reinterpret_cast<char *>(outBuffer.data()), 0,
                                 &decodeColor);

                    break;

                case JpgBlock::VQ_SKIP_4_COLOR_CODE:
                    txb = (codebuf & 0x0FF00000) >> 20;
                    tyb = (codebuf & 0x0FF000) >> 12;

                    updatereadbuf(&codebuf, &newbuf, blockAsT2100SkipLength,
                                  &newbits, bufferVector);
                    decodeColor.bitMapBits = 2;

                    for (unsigned char &i : decodeColor.index)
                    {
                        i = ((codebuf >> 29) & vqIndexMask);
                        if (((codebuf >> 31) & vqHeaderMask) ==
                            vqNoUpdateHeader)
                        {
                            updatereadbuf(&codebuf, &newbuf, vqNoUpdateLength,
                                          &newbits, bufferVector);
                        }
                        else
                        {
                            decodeColor.color[i] =
                                ((codebuf >> 5) & vqColorMask);
                            updatereadbuf(&codebuf, &newbuf, vqUpdateLength,
                                          &newbits, bufferVector);
                        }
                    }
                    vqDecompress(txb, tyb,
                                 reinterpret_cast<char *>(outBuffer.data()), 0,
                                 &decodeColor);

                    break;
                case JpgBlock::JPEG_SKIP_PASS2_CODE:
                    txb = (codebuf & 0x0FF00000) >> 20;
                    tyb = (codebuf & 0x0FF000) >> 12;

                    updatereadbuf(&codebuf, &newbuf, blockAsT2100SkipLength,
                                  &newbits, bufferVector);
                    decompress2Pass(txb, tyb,
                                    reinterpret_cast<char *>(outBuffer.data()),
                                    2);

                    break;
                default:
                    // TODO(ed) propogate errors upstream
                    return -1;
                    break;
            }
            moveBlockIndex();

        } while (bufferIndex <= bufferVector.size());

        return -1;
    }

#ifdef cimg_version
    void dump_to_bitmap_file()
    {
        cimg_library::CImg<unsigned char> image(width, height, 1, 3);
        for (int y = 0; y < width; y++)
        {
            for (int x = 0; x < height; x++)
            {
                auto pixel = outBuffer[x + (y * width)];
                image(x, y, 0) = pixel.r;
                image(x, y, 1) = pixel.g;
                image(x, y, 2) = pixel.b;
            }
        }
        image.save("/tmp/file2.bmp");
    }
#endif

  private:
    YuvMode yuvmode{};
    // width and height are the modes your display used
    unsigned long width{};
    unsigned long height{};
    unsigned long userWidth{};
    unsigned long userHeight{};
    unsigned char ySelector{};
    int scalefactor;
    int scalefactoruv;
    int advancescalefactor;
    int advancescalefactoruv;
    int mapping{};
    unsigned char uvSelector{};
    unsigned char advanceSelector{};
    int bytePos{}; // current byte position

    // quantization tables, no more than 4 quantization tables
    std::array<std::array<long, 64>, 4> qt{};

    // DC huffman tables , no more than 4 (0..3)
    std::array<HuffmanTable, 4> htdc{};
    // AC huffman tables (0..3)
    std::array<HuffmanTable, 4> htac{};
    std::array<int, 256> mCrToR{};
    std::array<int, 256> mCbToB{};
    std::array<int, 256> mCrToG{};
    std::array<int, 256> mCbToG{};
    std::array<int, 256> mY{};
    unsigned long bufferIndex{};
    uint32_t codebuf{}, newbuf{}, readbuf{};
    const unsigned char *stdLuminanceQt{};
    const uint8_t *stdChrominanceQt{};

    signed short int dcy{}, dcCb{}, dcCr{}; // Coeficientii DC pentru Y,Cb,Cr
    signed short int dctCoeff[384]{};
    // std::vector<signed short int> dctCoeff;  // Current DCT_coefficients
    // quantization table number for Y, Cb, Cr
    uint8_t yqNr = 0, cbQNr = 1, crQNr = 1;
    // DC Huffman table number for Y,Cb, Cr
    uint8_t ydcNr = 0, cbDcNr = 1, crDcNr = 1;
    // AC Huffman table number for Y,Cb, Cr
    uint8_t yacNr = 0, cbAcNr = 1, crAcNr = 1;
    int txb = 0;
    int tyb = 0;
    int newbits{};
    uint8_t *rlimitTable{};
    std::vector<RGB> yuvBuffer;
    // TODO(ed) this shouldn't exist.  It is cruft that needs cleaning up
    uint32_t *buffer{};

  public:
    std::vector<RGB> outBuffer;
};
} // namespace ast_video