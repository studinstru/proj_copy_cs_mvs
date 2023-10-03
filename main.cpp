/*
 * main.cpp
 *
 *  Created on: Jul 17, 2023
 *      Author: ambatwar
 */
#if 1
#include <stdio.h>>

unsigned int mvs[20];
typedef unsigned short U16;
typedef unsigned int U32;
typedef unsigned long U64;

unsigned int VH264_buf_num = 0;

#define Pred_L0         0x01 // bitwise: [0] = L0, [1] = L1
#define Pred_L1         0x10
#define BiPred          (Pred_L0 | Pred_L1)

U32 VMB_vector_x[8];
U32 VMB_vector_y[8];
short mvLX[2][4][4][2];

// data types BIG ENDIAN
typedef struct {
    U32 MB_TYPE; // 0x00 (H) | DBUF ADDR | MB_TYPE |
    U32 SP_INFO; // 0x00 (L) |

    U32 MB_y; // 0x01 (H)
    U32 MB_x; // 0x01 (L)

    U32 FILTER_PARAM2; // 0x03 (H)
    U32 MV_FWD_UNIT;   // 0x03 (L)

    U32 CURR_MB_ADDR;      // 0x04 (H)
    U32 CODED_BLOCK_FLAGS; // 0x04 (L)

    U32 DQ_COEF_C; // 0x05 (H)
    U32 DQ_COEF_Y; // 0x05 (L)


    U32 PRED_MODE_L16; // 0x06 (H)
    union {
        U32 PRED_MODE_C8;  // 0x06 (L)
        U32 DQ_COEF_Y__PRED_MODE_C8_8x8;
    };

    U32 VMB_vector_x[8];
    U32 VMB_vector_y[8];

    union {
        struct {  //Structure for Intra Prediction mode in I slice
            U32 PREV1msb;
            U32 PREV1lsb;

            U32 REM1msb;
            U32 REM1lsb;

            U32 PREV2msb;
            U32 PREV2lsb;

            U32 REM2msb;
            U32 REM2lsb;
        };

        struct {
            U32 REF_PIC_PTR_0; // reference index for part0(8x8) in list 0
            U32 REF_PIC_PTR_1; // reference index for part1(8x8) in list 0
            U32 REF_PIC_PTR_2; // reference index for part2(8x8) in list 0
            U32 REF_PIC_PTR_3; // reference index for part3(8x8) in list 0

            U32 REF_PIC_PTR_b_0; // reference index for part0(8x8) in list 1
            U32 REF_PIC_PTR_b_1; // reference index for part1(8x8) in list 1
            U32 REF_PIC_PTR_b_2; // reference index for part2(8x8) in list 1
            U32 REF_PIC_PTR_b_3; // reference index for part3(8x8) in list 1
        };
    };

    union {
        U64 IDCT_DATA[24][4];
        short level8x8[4][64];
    };

    U32 ChromaFilter; // 0x05 (H)
    U32 LumaFilter;   // 0x05 (L)

    U32 LumaFilterTop;  // 0x06 (H)
    U32 LumaFilterLeft; // 0x06 (L)

    U32 ChromaFilterTop;  // 0x0c
    U32 ChromaFilterLeft; // 0x0c

    U32 WEIGHTED_PRED_P_W_0;
    U32 WEIGHTED_PRED_P_W_1;
    U32 WEIGHTED_PRED_P_W_2;
    U32 WEIGHTED_PRED_P_W_3;

    U32 WEIGHTED_PRED_P_O_0;
    U32 WEIGHTED_PRED_P_O_1;
    U32 WEIGHTED_PRED_P_O_2;
    U32 WEIGHTED_PRED_P_O_3;

    U32 notused_RGB_DATA_BASE_ADDR;
    U32 BS_DATA_BASE_ADDR;

    U32 notused_INTERMEDIATE_IMAGE_BASE_ADDR;
    U32 DISPLAY_BUFFER_ADDR;

    U32 MBS_Y;
    U32 MBS_X;

    U64 notused_IDCT_DC_DATA[4];

} CTRL_STRUCT;


/*********************************/
/* global control structure data */
/*********************************/
volatile CTRL_STRUCT VH264_cs[2];

void __MC_8x8_MODE() {

    int mbPartIdx;
    unsigned short *mvx_init_ptr = (unsigned short *)&mvs[0];
    unsigned short *mvy_init_ptr = (unsigned short *)&mvs[2];

    for (mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++) {
        *mvx_init_ptr++ = (unsigned short)mvLX[0][mbPartIdx][0][0];
        *mvy_init_ptr++ = (unsigned short)mvLX[0][mbPartIdx][0][1];
    }

    mvx_init_ptr = (unsigned short *)&mvs[4];
    mvy_init_ptr = (unsigned short *)&mvs[6];

    for (mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++) {
        *mvx_init_ptr++ = (unsigned short)mvLX[1][mbPartIdx][0][0];
        *mvy_init_ptr++ = (unsigned short)mvLX[1][mbPartIdx][0][1];
    }
}

void __MC_8x8_MODE_TO_CS()
{
    register U32 *src_ptr;
    register U32 *dst_ptr_8x8;
    register U32 *type_ptr;

    src_ptr = (U32 *)&mvs[0];
    dst_ptr_8x8 = (U32 *)&VH264_cs[VH264_buf_num].VMB_vector_x[0];
    type_ptr = (U32 *)&VH264_cs[VH264_buf_num].MV_FWD_UNIT;

    *type_ptr = 0x4; /* 8x8_MODE */

    // list 0
    *(dst_ptr_8x8 + 0) = *(src_ptr + 0);
    *(dst_ptr_8x8 + 1) = *(src_ptr + 1);
    *(dst_ptr_8x8 + 2) = *(src_ptr + 2);
    *(dst_ptr_8x8 + 3) = *(src_ptr + 3);

    // list 1
    *(dst_ptr_8x8 + 4) = *(src_ptr + 4);
    *(dst_ptr_8x8 + 5) = *(src_ptr + 5);
    *(dst_ptr_8x8 + 6) = *(src_ptr + 6);
    *(dst_ptr_8x8 + 7) = *(src_ptr + 7);
}

void __MC_8x8_MODE_TO_CS2()
{
    register U32 *dst_ptr_8x8;
    register U32 *type_ptr;

    dst_ptr_8x8 = (U32 *)&VH264_cs[VH264_buf_num].VMB_vector_x[0];
    type_ptr = (U32 *)&VH264_cs[VH264_buf_num].MV_FWD_UNIT;

    *type_ptr = 0x4; /* 8x8_MODE */
#if 1
    *(dst_ptr_8x8 + 0) = mvLX[0][1][0][0] << 16 | mvLX[0][0][0][0]; //list 0: X0X1
    *(dst_ptr_8x8 + 1) = mvLX[0][3][0][0] << 16 | mvLX[0][2][0][0]; //list 0: X2X3
    *(dst_ptr_8x8 + 2) = mvLX[0][1][0][1] << 16 | mvLX[0][0][0][1]; //list 0: Y0Y1
    *(dst_ptr_8x8 + 3) = mvLX[0][3][0][1] << 16 | mvLX[0][2][0][1]; //list 0: Y2Y3

    *(dst_ptr_8x8 + 4) = mvLX[1][1][0][0] << 16 | mvLX[1][0][0][0]; //list 1: X0X1
    *(dst_ptr_8x8 + 5) = mvLX[1][3][0][0] << 16 | mvLX[1][2][0][0]; //list 1: X2X3
    *(dst_ptr_8x8 + 6) = mvLX[1][1][0][1] << 16 | mvLX[1][0][0][1]; //list 1: Y0Y1
    *(dst_ptr_8x8 + 7) = mvLX[1][3][0][1] << 16 | mvLX[1][2][0][1]; //list 1: Y2Y3
#else
    *(dst_ptr_8x8 + 0) = *(src_ptr + 0);
    *(dst_ptr_8x8 + 1) = *(src_ptr + 1);
    *(dst_ptr_8x8 + 2) = *(src_ptr + 2);
    *(dst_ptr_8x8 + 3) = *(src_ptr + 3);
#endif
}

void __MC_16x16_MODE_TO_CS() {

    unsigned *cs_ptr = (unsigned *)&VH264_cs[VH264_buf_num].VMB_vector_x[0];
    //The code extracts the lower 16 bits of the motion vector components  of mvLX0****
    unsigned mv_p0x0 = ((unsigned)mvLX[0][0][0][0]) & 0xffff;
    unsigned mv_p0y0 = ((unsigned)mvLX[0][0][0][1]) & 0xffff;

    //The code extracts the lower 16 bits of the motion vector components  of mvLX1****
    unsigned mv_p0x1 = ((unsigned)mvLX[1][0][0][0]) & 0xffff;
    unsigned mv_p0y1 = ((unsigned)mvLX[1][0][0][1]) & 0xffff;

    unsigned mv_p1x0, mv_p1x1;
    unsigned mv_p1y0, mv_p1y1;

    int mb_type = 2;

    //performs a bit shift operation to duplicate the lower 16 bits of mvx0 and mvy0 to
    //the upper 16 bits, effectively creating a 32-bit value with identical upper and lower 16-bit components.
    mv_p0x0 = (mv_p0x0 << 16) | mv_p0x0;
    mv_p0y0 = (mv_p0y0 << 16) | mv_p0y0;

    mv_p0x1 = (mv_p0x1 << 16) | mv_p0x1;
    mv_p0y1 = (mv_p0y1 << 16) | mv_p0y1;

    if (mb_type == 0) {                            /* 16x16 macroblock */
        VH264_cs[VH264_buf_num].MV_FWD_UNIT = 0x1; /* 16x16 MODE */
        *cs_ptr++ = mv_p0x0;
        *cs_ptr++ = mv_p0x0;
        *cs_ptr++ = mv_p0y0;
        *cs_ptr++ = mv_p0y0;

        *cs_ptr++ = mv_p0x1;
        *cs_ptr++ = mv_p0x1;
        *cs_ptr++ = mv_p0y1;
        *cs_ptr++ = mv_p0y1;

        /* | mvx0 | mvx0 | mvx0 | mvx0 | -> 64 bits (vector_x) */
        /* | mvx0 | mvx0 | mvx0 | mvx0 | -> 64 bits (vector_x) */
        /* | mvy0 | mvy0 | mvy0 | mvy0 | -> 64 bits (vector_y) */
        /* | mvy0 | mvy0 | mvy0 | mvy0 | -> 64 bits (vector_y) */

    } else if (mb_type == 1) {                     /* 16x8 macroblock */
        VH264_cs[VH264_buf_num].MV_FWD_UNIT = 0x2; /* 16x8 MODE */
        mv_p1x0 = ((unsigned)mvLX[0][1][0][0]) & 0xffff;
        mv_p1y0 = ((unsigned)mvLX[0][1][0][1]) & 0xffff;
        mv_p1x0 = (mv_p1x0 << 16) | mv_p1x0;
        mv_p1y0 = (mv_p1y0 << 16) | mv_p1y0;

        mv_p1x1 = ((unsigned)mvLX[1][1][0][0]) & 0xffff;
        mv_p1y1 = ((unsigned)mvLX[1][1][0][1]) & 0xffff;
        mv_p1x1 = (mv_p1x1 << 16) | mv_p1x1;
        mv_p1y1 = (mv_p1y1 << 16) | mv_p1y1;

        *cs_ptr++ = mv_p0x0;
        *cs_ptr++ = mv_p1x0;
        *cs_ptr++ = mv_p0y0;
        *cs_ptr++ = mv_p1y0;

        *cs_ptr++ = mv_p0x1;
        *cs_ptr++ = mv_p1x1;
        *cs_ptr++ = mv_p0y1;
        *cs_ptr++ = mv_p1y1;
    } else {                                       /* 8x16 macroblock */
        VH264_cs[VH264_buf_num].MV_FWD_UNIT = 0x3; /* 8x16 MODE */
        mv_p1x0 = ((unsigned)mvLX[0][1][0][0]) & 0xffff;
        mv_p1y0 = ((unsigned)mvLX[0][1][0][1]) & 0xffff;
        *cs_ptr++ = mv_p0x0 << 16 | mv_p1x0;
        *cs_ptr++ = mv_p0x0 << 16 | mv_p1x0;
        *cs_ptr++ = mv_p0y0 << 16 | mv_p1y0;
        *cs_ptr++ = mv_p0y0 << 16 | mv_p1y0;

        mv_p1x1 = ((unsigned)mvLX[1][1][0][0]) & 0xffff;
        mv_p1y1 = ((unsigned)mvLX[1][1][0][1]) & 0xffff;
        *cs_ptr++ = mv_p0x1 << 16 | mv_p1x1;
        *cs_ptr++ = mv_p0x1 << 16 | mv_p1x1;
        *cs_ptr++ = mv_p0y1 << 16 | mv_p1y1;
        *cs_ptr++ = mv_p0y1 << 16 | mv_p1y1;
    }
}

void update_MVL() {

    int mbPartIdx, mbsubPartIdx, listIdx;

    for (listIdx = 0; listIdx < 2; listIdx++) {
        for (mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++) {
            for (mbsubPartIdx = 0; mbsubPartIdx < 4; mbsubPartIdx++) {
                mvLX[listIdx][mbPartIdx][mbsubPartIdx][0] = (mbPartIdx*2 + listIdx) + mbsubPartIdx*2 + 4;
                mvLX[listIdx][mbPartIdx][mbsubPartIdx][1] = (mbPartIdx*5 + listIdx) + mbsubPartIdx*2 + 5;
            }
        }
    }
}

static int cabac_get_neighbouring_mvLX(short mv[], int list)
{

    int refidx = 5;

   *((int *)&(mv[0])) = *((int *)&(mvLX[list][0][2][0]));
   *((int *)&(mv[1])) = *((int *)&(mvLX[list][0][2][1]));

    return refidx;
}

void UPDATE_CS_STRUCTURE16X16 () {

    int *dst_ptr_8x8;
    int refIdxLX[2][4] = {{1,0,0,0},{0,0,0,0}};

    unsigned int ref_pic_list[2][6] = {{0x60000000, 0x70000000, 0x80000000, 0x90000000, 0xa0000000,0xb0000000}, {0x22220000, 0x33330000, 0x44440000, 0x55550000, 0x66660000, 0x77770000}};

    int findSinglePred[2] = {0x1, 0x0};
    int PredMode[2] = {1, 1};

    int VH264_buf_num = 0;

    dst_ptr_8x8 = (int  *)&VH264_cs[0].VMB_vector_x[0];

    mvLX[0][0][0][0] = -2;
    mvLX[0][1][0][0] = 0;
    mvLX[0][2][0][0] = 0;
    mvLX[0][3][0][0] = 0;

    mvLX[0][0][0][1] = 4;
    mvLX[0][1][0][1] = 0;
    mvLX[0][2][0][1] = 0;
    mvLX[0][3][0][1] = 0;

    mvLX[1][0][0][0] = 0;
    mvLX[1][1][0][0] = 0;
    mvLX[1][2][0][0] = 0;
    mvLX[1][3][0][0] = 0;

    mvLX[1][0][0][1] = 0;
    mvLX[1][1][0][1] = 0;
    mvLX[1][2][0][1] = 0;
    mvLX[1][3][0][1] = 0;

    int idx0, idx1, offset, listIdx = 0;

    int partList00, partList01, partList02, partList03;
    int partList10, partList11, partList12, partList13;

    int findSinglePred00, findSinglePred01, findSinglePred02, findSinglePred03;
    int findSinglePred10, findSinglePred11, findSinglePred12, findSinglePred13;

    unsigned mv_p0x0, mv_p0x1;
    unsigned mv_p0y0, mv_p0y1;

    unsigned mv_p1x0, mv_p1x1;
    unsigned mv_p1y0, mv_p1y1;

    volatile unsigned *cs_ptr = (volatile unsigned *)&VH264_cs[VH264_buf_num].VMB_vector_x[0];
    //The code extracts the lower 16 bits of the motion vector components  of mvLX0****
    mv_p0x0 = ((unsigned)mvLX[0][0][0][0]) & 0xffff;
    mv_p0y0 = ((unsigned)mvLX[0][0][0][1]) & 0xffff;

    mv_p0x0 = (mv_p0x0 << 16) | mv_p0x0;
    mv_p0y0 = (mv_p0y0 << 16) | mv_p0y0;

    //The code extracts the lower 16 bits of the motion vector components  of mvLX1****
    mv_p0x1 = ((unsigned)mvLX[1][0][0][0]) & 0xffff;
    mv_p0y1 = ((unsigned)mvLX[1][0][0][1]) & 0xffff;

    mv_p0x1 = (mv_p0x1 << 16) | mv_p0x1;
    mv_p0y1 = (mv_p0y1 << 16) | mv_p0y1;

    mv_p1x0 = ((unsigned)mvLX[0][1][0][0]) & 0xffff;
    mv_p1y0 = ((unsigned)mvLX[0][1][0][1]) & 0xffff;

    mv_p1x1 = ((unsigned)mvLX[1][1][0][0]) & 0xffff;
    mv_p1y1 = ((unsigned)mvLX[1][1][0][1]) & 0xffff;

    volatile unsigned *ref_idx_ptr0  = (volatile unsigned *)&VH264_cs[VH264_buf_num].REF_PIC_PTR_0;
    volatile unsigned *ref_idx_ptr1  = (volatile unsigned *)&VH264_cs[VH264_buf_num].REF_PIC_PTR_b_0;

    unsigned *ref_idx_ptr = ( unsigned *)&VH264_cs[VH264_buf_num].REF_PIC_PTR_0;

    printf("FOR U 16X16: PredMode[0]:%d\n",PredMode[0] );

    if((PredMode[0] & (Pred_L0)) || ((PredMode[0] & (Pred_L0 << 1)))) {
        printf("SGGS1 \n");
        *cs_ptr++ = mv_p0x0;
        *cs_ptr++ = mv_p0x0;
        *cs_ptr++ = mv_p0y0;
        *cs_ptr++ = mv_p0y0;

        ref_idx_ptr  = (unsigned *)&VH264_cs[VH264_buf_num].REF_PIC_PTR_0;

        *ref_idx_ptr++ = ref_pic_list[0][refIdxLX[0][0]];
        *ref_idx_ptr++ = ref_pic_list[0][refIdxLX[0][0]];
        *ref_idx_ptr++ = ref_pic_list[0][refIdxLX[0][0]];
        *ref_idx_ptr++ = ref_pic_list[0][refIdxLX[0][0]];

        if(findSinglePred[1] == 0) {
            ref_idx_ptr  = (unsigned *)&VH264_cs[VH264_buf_num].REF_PIC_PTR_b_0;

            *ref_idx_ptr++ = 0;
            *ref_idx_ptr++ = 0;
            *ref_idx_ptr++ = 0;
            *ref_idx_ptr++ = 0;

            *cs_ptr++ = 0;
            *cs_ptr++ = 0;
            *cs_ptr++ = 0;
            *cs_ptr++ = 0;
        }
    }

    if((PredMode[1] & (Pred_L1)) || ((PredMode[1] & (Pred_L1 << 1)))) {
        printf("SGGS2 \n");
        *cs_ptr++ = mv_p0x1;
        *cs_ptr++ = mv_p0x1;
        *cs_ptr++ = mv_p0y1;
        *cs_ptr++ = mv_p0y1;

        if(findSinglePred[0] != 0) {
            ref_idx_ptr  = (unsigned *)&VH264_cs[VH264_buf_num].REF_PIC_PTR_b_0;
        } else {
            ref_idx_ptr  = (unsigned *)&VH264_cs[VH264_buf_num].REF_PIC_PTR_0;
        }

        *ref_idx_ptr++ = ref_pic_list[1][refIdxLX[1][0]];
        *ref_idx_ptr++ = ref_pic_list[1][refIdxLX[1][0]];
        *ref_idx_ptr++ = ref_pic_list[1][refIdxLX[1][0]];
        *ref_idx_ptr++ = ref_pic_list[1][refIdxLX[1][0]];

        if(findSinglePred[0] == 0) {

            ref_idx_ptr  = (unsigned *)&VH264_cs[VH264_buf_num].REF_PIC_PTR_b_0;

            *ref_idx_ptr++ = 0;
            *ref_idx_ptr++ = 0;
            *ref_idx_ptr++ = 0;
            *ref_idx_ptr++ = 0;

            *cs_ptr++ = 0;
            *cs_ptr++ = 0;
            *cs_ptr++ = 0;
            *cs_ptr++ = 0;
        }
    }

    printf("%04x ", *(dst_ptr_8x8 + 0));
    printf("%04x\n",*(dst_ptr_8x8 + 1));
    printf("%08x ", *(dst_ptr_8x8 + 2));
    printf("%08x\n",*(dst_ptr_8x8 + 3));

    printf("%08x ", *(dst_ptr_8x8 + 4));
    printf("%08x\n",*(dst_ptr_8x8 + 5));
    printf("%08x ", *(dst_ptr_8x8 + 6));
    printf("%08x\n",*(dst_ptr_8x8 + 7));
    printf("End here\n");

}

void UPDATE_CS_STRUCTURE16X8 () {

    int *dst_ptr_8x8;
    int refIdxLX[2][4] = {{0,0,0,0},{0,0,0,0}};

    unsigned int ref_pic_list[2][6] = {{0x60000000, 0x70000000, 0x80000000, 0x90000000, 0xa0000000,0xb0000000}, {0x22220000, 0x33330000, 0x44440000, 0x55550000, 0x66660000, 0x77770000}};

    int findSinglePred[2] = {0x1, 0x3};
    int PredMode[2] = {48, 48};

    int VH264_buf_num = 0;

    dst_ptr_8x8 = (int  *)&VH264_cs[0].VMB_vector_x[0];

    mvLX[0][0][0][0] = 0;
    mvLX[0][1][0][0] = 0;
    mvLX[0][2][0][0] = 0;
    mvLX[0][3][0][0] = 0;

    mvLX[0][0][0][1] = 0;
    mvLX[0][1][0][1] = 0;
    mvLX[0][2][0][1] = 0;
    mvLX[0][3][0][1] = 0;

    mvLX[1][0][0][0] = 0;
    mvLX[1][1][0][0] = 1;
    mvLX[1][2][0][0] = 0;
    mvLX[1][3][0][0] = 0;

    mvLX[1][0][0][1] = 0;
    mvLX[1][1][0][1] = 0;
    mvLX[1][2][0][1] = 0;
    mvLX[1][3][0][1] = 0;

    int idx0, idx1, offset, listIdx = 0;

    int partList00, partList01, partList02, partList03;
    int partList10, partList11, partList12, partList13;

    int findSinglePred00, findSinglePred01, findSinglePred02, findSinglePred03;
    int findSinglePred10, findSinglePred11, findSinglePred12, findSinglePred13;

    unsigned mv_p0x0, mv_p0x1;
    unsigned mv_p0y0, mv_p0y1;

    unsigned mv_p1x0, mv_p1x1;
    unsigned mv_p1y0, mv_p1y1;

    volatile unsigned *cs_ptr = (volatile unsigned *)&VH264_cs[VH264_buf_num].VMB_vector_x[0];
    //The code extracts the lower 16 bits of the motion vector components  of mvLX0****
    mv_p0x0 = ((unsigned)mvLX[0][0][0][0]) & 0xffff;
    mv_p0y0 = ((unsigned)mvLX[0][0][0][1]) & 0xffff;

    mv_p0x0 = (mv_p0x0 << 16) | mv_p0x0;
    mv_p0y0 = (mv_p0y0 << 16) | mv_p0y0;

    //The code extracts the lower 16 bits of the motion vector components  of mvLX1****
    mv_p0x1 = ((unsigned)mvLX[1][0][0][0]) & 0xffff;
    mv_p0y1 = ((unsigned)mvLX[1][0][0][1]) & 0xffff;

    mv_p0x1 = (mv_p0x1 << 16) | mv_p0x1;
    mv_p0y1 = (mv_p0y1 << 16) | mv_p0y1;

    mv_p1x0 = ((unsigned)mvLX[0][1][0][0]) & 0xffff;
    mv_p1y0 = ((unsigned)mvLX[0][1][0][1]) & 0xffff;

    mv_p1x1 = ((unsigned)mvLX[1][1][0][0]) & 0xffff;
    mv_p1y1 = ((unsigned)mvLX[1][1][0][1]) & 0xffff;

    volatile unsigned *ref_idx_ptr0  = (volatile unsigned *)&VH264_cs[VH264_buf_num].REF_PIC_PTR_0;
    volatile unsigned *ref_idx_ptr1  = (volatile unsigned *)&VH264_cs[VH264_buf_num].REF_PIC_PTR_b_0;

    if(findSinglePred[0] == findSinglePred[1]) {

        volatile unsigned *ref_idx_ptr;

        if((PredMode[0] & (Pred_L0)) || ((PredMode[0] & (Pred_L0 << 1)))) {

            mv_p1x0 = (mv_p1x0 << 16) | mv_p1x0;
            mv_p1y0 = (mv_p1y0 << 16) | mv_p1y0;

            *cs_ptr++ = mv_p0x0;
            *cs_ptr++ = mv_p1x0;
            *cs_ptr++ = mv_p0y0;
            *cs_ptr++ = mv_p1y0;

            ref_idx_ptr  = (volatile unsigned *)&VH264_cs[VH264_buf_num].REF_PIC_PTR_0;

            *ref_idx_ptr++ = ref_pic_list[0][refIdxLX[0][0]];
            *ref_idx_ptr++ = ref_pic_list[0][refIdxLX[0][0]];
            *ref_idx_ptr++ = ref_pic_list[0][refIdxLX[0][1]];
            *ref_idx_ptr++ = ref_pic_list[0][refIdxLX[0][1]];

        }

        if((PredMode[1] & (Pred_L1)) || ((PredMode[1] & (Pred_L1 << 1)))) {

            mv_p1x1 = (mv_p1x1 << 16) | mv_p1x1;
            mv_p1y1 = (mv_p1y1 << 16) | mv_p1y1;

            *cs_ptr++ = mv_p0x1;
            *cs_ptr++ = mv_p1x1;
            *cs_ptr++ = mv_p0y1;
            *cs_ptr++ = mv_p1y1;

            ref_idx_ptr  = (volatile unsigned *)&VH264_cs[VH264_buf_num].REF_PIC_PTR_b_0;

            *ref_idx_ptr++ = ref_pic_list[1][refIdxLX[1][0]];
            *ref_idx_ptr++ = ref_pic_list[1][refIdxLX[1][0]];
            *ref_idx_ptr++ = ref_pic_list[1][refIdxLX[1][1]];
            *ref_idx_ptr++ = ref_pic_list[1][refIdxLX[1][1]];
        }
    } else {
        int partList00, partList01, partList10, partList11;
        volatile unsigned *ref_idx_ptr;

        if((findSinglePred[0] == 0) || findSinglePred[1] == 0) {
            int checkPartition, idx, listIdx0, listIdx1;
            if(findSinglePred[1] == 0) {
                idx = 0;
                checkPartition = findSinglePred[0];
            } else {
                idx = 1;
                checkPartition = findSinglePred[1];
            }
            ref_idx_ptr  = (volatile unsigned *)&VH264_cs[VH264_buf_num].REF_PIC_PTR_0;

            listIdx0 =  ((checkPartition & (1 << 0)) != 0) ? idx : !idx;
            listIdx1 =  ((checkPartition & (1 << 1)) != 0) ? idx : !idx;

            mv_p0x0 = ((unsigned)mvLX[idx][0][0][0]) & 0xffff;
            mv_p0y0 = ((unsigned)mvLX[idx][0][0][1]) & 0xffff;

            mv_p0x0 = (mv_p0x0 << 16) | mv_p0x0;
            mv_p0y0 = (mv_p0y0 << 16) | mv_p0y0;

            mv_p1x0 = ((unsigned)mvLX[idx][1][0][0]) & 0xffff;
            mv_p1y0 = ((unsigned)mvLX[idx][1][0][1]) & 0xffff;

            mv_p1x0 = (mv_p1x0 << 16) | mv_p1x0;
            mv_p1y0 = (mv_p1y0 << 16) | mv_p1y0;

            *ref_idx_ptr++ = ref_pic_list[idx][refIdxLX[idx][0]];
            *ref_idx_ptr++ = ref_pic_list[idx][refIdxLX[idx][0]];
            *ref_idx_ptr++ = ref_pic_list[idx][refIdxLX[idx][1]];
            *ref_idx_ptr++ = ref_pic_list[idx][refIdxLX[idx][1]];

            //*ref_idx_ptr++ = 0;
            //*ref_idx_ptr++ = 0;
            //*ref_idx_ptr++ = 0;
            //*ref_idx_ptr++ = 0;

            *cs_ptr++ = mv_p0x0 << 16 | mv_p0x0;
            *cs_ptr++ = mv_p1x0 << 16 | mv_p1x0;
            *cs_ptr++ = mv_p0y0 << 16 | mv_p0y0;
            *cs_ptr++ = mv_p1y0 << 16 | mv_p1y0;

            *cs_ptr++ = 0;
            *cs_ptr++ = 0;
            *cs_ptr++ = 0;
            *cs_ptr++ = 0;

        } else {
            partList00 = ((findSinglePred[0] & (1 << 0)) != 0) ? 0 : 1;
            partList01 = ((findSinglePred[0] & (1 << 1)) != 0) ? 0 : 1;
            partList10 = ((findSinglePred[1] & (1 << 0)) != 0) ? 1 : 0;
            partList11 = ((findSinglePred[1] & (1 << 1)) != 0) ? 1 : 0;


            mv_p0x0 = ((unsigned)mvLX[partList00][0][0][0]) & 0xffff;
            mv_p0y0 = ((unsigned)mvLX[partList00][0][0][1]) & 0xffff;

            //mv_p0x0 = (mv_p0x0 << 16) | mv_p0x0;
            //mv_p0y0 = (mv_p0y0 << 16) | mv_p0y0;

            mv_p1x0 = ((unsigned)mvLX[partList01][1][0][0]) & 0xffff;
            mv_p1y0 = ((unsigned)mvLX[partList01][1][0][1]) & 0xffff;

            *cs_ptr++ = mv_p0x0 << 16 | mv_p0x0;
            *cs_ptr++ = mv_p1x0 << 16 | mv_p1x0;
            *cs_ptr++ = mv_p0y0 << 16 | mv_p0y0;
            *cs_ptr++ = mv_p1y0 << 16 | mv_p1y0;

            mv_p0x1 = ((unsigned)mvLX[partList10][0][0][0]) & 0xffff;
            mv_p0y1 = ((unsigned)mvLX[partList10][0][0][1]) & 0xffff;

            //mv_p0x1 = (mv_p0x1 << 16) | mv_p0x1;
            //mv_p0y1 = (mv_p0y1 << 16) | mv_p0y1;

            mv_p1x1 = ((unsigned)mvLX[partList11][1][0][0]) & 0xffff;
            mv_p1y1 = ((unsigned)mvLX[partList11][1][0][1]) & 0xffff;

            *cs_ptr++ = mv_p0x1 << 16 | mv_p0x1;
            *cs_ptr++ = mv_p1x1 << 16 | mv_p1x1;
            *cs_ptr++ = mv_p0y1 << 16 | mv_p0y1;
            *cs_ptr++ = mv_p1y1 << 16 | mv_p1y1;


            *ref_idx_ptr0++ = ref_pic_list[partList00][refIdxLX[partList00][0]];
            *ref_idx_ptr0++ = ref_pic_list[partList00][refIdxLX[partList00][0]];
            *ref_idx_ptr0++ = ref_pic_list[partList01][refIdxLX[partList01][1]];
            *ref_idx_ptr0++ = ref_pic_list[partList01][refIdxLX[partList01][1]];

            *ref_idx_ptr1++ = ref_pic_list[partList10][refIdxLX[partList10][0]];
            *ref_idx_ptr1++ = ref_pic_list[partList10][refIdxLX[partList10][0]];
            *ref_idx_ptr1++ = ref_pic_list[partList11][refIdxLX[partList11][1]];
            *ref_idx_ptr1++ = ref_pic_list[partList11][refIdxLX[partList11][1]];
        }
    }

    printf("%04x ", *(dst_ptr_8x8 + 0));
    printf("%04x\n",*(dst_ptr_8x8 + 1));
    printf("%08x ", *(dst_ptr_8x8 + 2));
    printf("%08x\n",*(dst_ptr_8x8 + 3));

    printf("%08x ", *(dst_ptr_8x8 + 4));
    printf("%08x\n",*(dst_ptr_8x8 + 5));
    printf("%08x ", *(dst_ptr_8x8 + 6));
    printf("%08x\n",*(dst_ptr_8x8 + 7));
    printf("End here\n");

}

void UPDATE_CS_STRUCTURE8X16 () {

    int *dst_ptr_8x8;
    int refIdxLX[2][4] = {{4,0,0,0},{0,0,0,0}};

    unsigned int ref_pic_list[2][6] = {{0x60000000, 0x70000000, 0x80000000, 0x90000000, 0xa0000000,0xb0000000}, {0x22220000, 0x33330000, 0x44440000, 0x55550000, 0x66660000, 0x77770000}};

    int findSinglePred[2] = {0x3, 0x3};
    int PredMode[2] = {51, 51};

    int VH264_buf_num = 0;

    dst_ptr_8x8 = (int  *)&VH264_cs[0].VMB_vector_x[0];

    mvLX[0][0][0][0] = -4;
    mvLX[0][1][0][0] = -1;
    mvLX[0][2][0][0] = 0;
    mvLX[0][3][0][0] = 0;

    mvLX[0][0][0][1] = 2;
    mvLX[0][1][0][1] = 2;
    mvLX[0][2][0][1] = 0;
    mvLX[0][3][0][1] = 0;

    mvLX[1][0][0][0] = 0;
    mvLX[1][1][0][0] = 0;
    mvLX[1][2][0][0] = 0;
    mvLX[1][3][0][0] = 0;

    mvLX[1][0][0][1] = -4;
    mvLX[1][1][0][1] = -3;
    mvLX[1][2][0][1] = 0;
    mvLX[1][3][0][1] = 0;

    int idx0, idx1, offset, listIdx = 0;

    int partList00, partList01, partList02, partList03;
    int partList10, partList11, partList12, partList13;

    int findSinglePred00, findSinglePred01, findSinglePred02, findSinglePred03;
    int findSinglePred10, findSinglePred11, findSinglePred12, findSinglePred13;

    unsigned mv_p0x0, mv_p0x1;
    unsigned mv_p0y0, mv_p0y1;

    unsigned mv_p1x0, mv_p1x1;
    unsigned mv_p1y0, mv_p1y1;

    volatile unsigned *cs_ptr = (volatile unsigned *)&VH264_cs[VH264_buf_num].VMB_vector_x[0];
    //The code extracts the lower 16 bits of the motion vector components  of mvLX0****
    mv_p0x0 = ((unsigned)mvLX[0][0][0][0]) & 0xffff;
    mv_p0y0 = ((unsigned)mvLX[0][0][0][1]) & 0xffff;

    mv_p0x0 = (mv_p0x0 << 16) | mv_p0x0;
    mv_p0y0 = (mv_p0y0 << 16) | mv_p0y0;

    //The code extracts the lower 16 bits of the motion vector components  of mvLX1****
    mv_p0x1 = ((unsigned)mvLX[1][0][0][0]) & 0xffff;
    mv_p0y1 = ((unsigned)mvLX[1][0][0][1]) & 0xffff;

    mv_p0x1 = (mv_p0x1 << 16) | mv_p0x1;
    mv_p0y1 = (mv_p0y1 << 16) | mv_p0y1;

    mv_p1x0 = ((unsigned)mvLX[0][1][0][0]) & 0xffff;
    mv_p1y0 = ((unsigned)mvLX[0][1][0][1]) & 0xffff;

    mv_p1x1 = ((unsigned)mvLX[1][1][0][0]) & 0xffff;
    mv_p1y1 = ((unsigned)mvLX[1][1][0][1]) & 0xffff;

    volatile unsigned *ref_idx_ptr0  = (volatile unsigned *)&VH264_cs[VH264_buf_num].REF_PIC_PTR_0;
    volatile unsigned *ref_idx_ptr1  = (volatile unsigned *)&VH264_cs[VH264_buf_num].REF_PIC_PTR_b_0;

    if(findSinglePred[0] == findSinglePred[1])
    {
        if((PredMode[0] & (Pred_L0)) || ((PredMode[0] & (Pred_L0 << 1)))) {

             *cs_ptr++ = mv_p0x0 << 16 | mv_p1x0;
             *cs_ptr++ = mv_p0x0 << 16 | mv_p1x0;
             *cs_ptr++ = mv_p0y0 << 16 | mv_p1y0;
             *cs_ptr++ = mv_p0y0 << 16 | mv_p1y0;

             volatile unsigned *ref_idx_ptr  = (volatile unsigned *)&VH264_cs[VH264_buf_num].REF_PIC_PTR_0;

             *ref_idx_ptr++ = ref_pic_list[0][refIdxLX[0][0]];
             *ref_idx_ptr++ = ref_pic_list[0][refIdxLX[0][1]];
             *ref_idx_ptr++ = ref_pic_list[0][refIdxLX[0][0]];
             *ref_idx_ptr++ = ref_pic_list[0][refIdxLX[0][1]];

         }

         if((PredMode[1] & (Pred_L1)) || ((PredMode[1] & (Pred_L1 << 1)))) {

             *cs_ptr++ = mv_p0x1 << 16 | mv_p1x1;
             *cs_ptr++ = mv_p0x1 << 16 | mv_p1x1;
             *cs_ptr++ = mv_p0y1 << 16 | mv_p1y1;
             *cs_ptr++ = mv_p0y1 << 16 | mv_p1y1;

             volatile unsigned *ref_idx_ptr  = (volatile unsigned *)&VH264_cs[VH264_buf_num].REF_PIC_PTR_b_0;

             /* bframes */
             *ref_idx_ptr++ = ref_pic_list[1][refIdxLX[1][0]];
             *ref_idx_ptr++ = ref_pic_list[1][refIdxLX[1][1]];
             *ref_idx_ptr++ = ref_pic_list[1][refIdxLX[1][0]];
             *ref_idx_ptr++ = ref_pic_list[1][refIdxLX[1][1]];
         }

    } else {

        int partList00, partList01, partList10, partList11;

        partList00 = ((findSinglePred[0] & (1 << 0)) != 0) ? 0 : 1;
        partList01 = ((findSinglePred[0] & (1 << 1)) != 0) ? 0 : 1;
        partList10 = ((findSinglePred[1] & (1 << 0)) != 0) ? 1 : 0;
        partList11 = ((findSinglePred[1] & (1 << 1)) != 0) ? 1 : 0;


        mv_p0x0 = ((unsigned)mvLX[partList00][0][0][0]) & 0xffff;
        mv_p0y0 = ((unsigned)mvLX[partList00][0][0][1]) & 0xffff;

        mv_p0x0 = (mv_p0x0 << 16) | mv_p0x0;
        mv_p0y0 = (mv_p0y0 << 16) | mv_p0y0;

        mv_p1x0 = ((unsigned)mvLX[partList01][1][0][0]) & 0xffff;
        mv_p1y0 = ((unsigned)mvLX[partList01][1][0][1]) & 0xffff;

        *cs_ptr++ = mv_p0x0 << 16 | mv_p1x0;
        *cs_ptr++ = mv_p0x0 << 16 | mv_p1x0;
        *cs_ptr++ = mv_p0y0 << 16 | mv_p1y0;
        *cs_ptr++ = mv_p0y0 << 16 | mv_p1y0;

        mv_p0x1 = ((unsigned)mvLX[partList10][0][0][0]) & 0xffff;
        mv_p0y1 = ((unsigned)mvLX[partList10][0][0][1]) & 0xffff;

        mv_p0x1 = (mv_p0x1 << 16) | mv_p0x1;
        mv_p0y1 = (mv_p0y1 << 16) | mv_p0y1;

        mv_p1x1 = ((unsigned)mvLX[partList11][1][0][0]) & 0xffff;
        mv_p1y1 = ((unsigned)mvLX[partList11][1][0][1]) & 0xffff;

        *cs_ptr++ = mv_p0x1 << 16 | mv_p1x1;
        *cs_ptr++ = mv_p0x1 << 16 | mv_p1x1;
        *cs_ptr++ = mv_p0y1 << 16 | mv_p1y1;
        *cs_ptr++ = mv_p0y1 << 16 | mv_p1y1;


        *ref_idx_ptr0++ = ref_pic_list[partList00][refIdxLX[partList00][0]];
        *ref_idx_ptr0++ = ref_pic_list[partList01][refIdxLX[partList01][1]];
        *ref_idx_ptr0++ = ref_pic_list[partList00][refIdxLX[partList00][0]];
        *ref_idx_ptr0++ = ref_pic_list[partList01][refIdxLX[partList01][1]];

        *ref_idx_ptr1++ = ref_pic_list[partList10][refIdxLX[partList10][0]];
        *ref_idx_ptr1++ = ref_pic_list[partList11][refIdxLX[partList11][1]];
        *ref_idx_ptr1++ = ref_pic_list[partList10][refIdxLX[partList10][0]];
        *ref_idx_ptr1++ = ref_pic_list[partList11][refIdxLX[partList11][1]];

    }

    printf("%04x ", *(dst_ptr_8x8 + 0));
    printf("%04x\n",*(dst_ptr_8x8 + 1));
    printf("%08x ", *(dst_ptr_8x8 + 2));
    printf("%08x\n",*(dst_ptr_8x8 + 3));

    printf("%08x ", *(dst_ptr_8x8 + 4));
    printf("%08x\n",*(dst_ptr_8x8 + 5));
    printf("%08x ", *(dst_ptr_8x8 + 6));
    printf("%08x\n",*(dst_ptr_8x8 + 7));
    printf("End here\n");

}

void UPDATE_CS_STRUCTURE8X8 () {

    int *dst_ptr_8x8;
    int refIdxLX[2][4] = {{0,0,0,0}, {0,0,0,0}};;
    unsigned int ref_pic_list[2][6] = {{0x60000000, 0x70000000, 0x80000000, 0x90000000, 0xa0000000, 0xb0000000},
                              {0x22220000, 0x33330000, 0x44440000, 0x55550000, 0x66660000, 0x77770000}};

    int findSinglePred[2] = {0x1, 0x0}; //{0xE, 0x8};

    dst_ptr_8x8 = (int  *)&VH264_cs[0].VMB_vector_x[0];

    mvLX[0][0][0][0] = -12;
    mvLX[0][1][0][0] = -7;
    mvLX[0][2][0][0] = -16;
    mvLX[0][3][0][0] = -5;

    mvLX[0][0][0][1] = 2;
    mvLX[0][1][0][1] = 2;
    mvLX[0][2][0][1] = 1;
    mvLX[0][3][0][1] = 2;

    mvLX[1][0][0][0] =  0;
    mvLX[1][1][0][0] =  7;
    mvLX[1][2][0][0] = 17;
    mvLX[1][3][0][0] =  6;

    mvLX[1][0][0][1] =  0;
    mvLX[1][1][0][1] = -2;
    mvLX[1][2][0][1] = -1;
    mvLX[1][3][0][1] = -1;

    int idx0, idx1, offset, listIdx = 0;

    int partList00, partList01, partList02, partList03;
    int partList10, partList11, partList12, partList13;

    int findSinglePred00, findSinglePred01, findSinglePred02, findSinglePred03;
    int findSinglePred10, findSinglePred11, findSinglePred12, findSinglePred13;

    if(findSinglePred[0] != findSinglePred[1]) {

        register unsigned *ref_idx_ptr, *ref_idx_ptr1;

        register U32 *dst_ptr_8x8;
        register U32 *type_ptr;

        dst_ptr_8x8 = (U32 *)&VH264_cs[VH264_buf_num].VMB_vector_x[0];
        type_ptr = (U32 *)&VH264_cs[VH264_buf_num].MV_FWD_UNIT;

        *type_ptr = 0x4; /* 8x8_MODE */


#if 0
        if (findSinglePred[1] == 0) {
            listIdx = 0;
            ref_idx_ptr = (unsigned *)&VH264_cs[VH264_buf_num].REF_PIC_PTR_0;
        } else if (findSinglePred[0] == 0) {
            listIdx = 1;
            ref_idx_ptr = (unsigned *)&VH264_cs[VH264_buf_num].REF_PIC_PTR_b_0;
        } else {
            ref_idx_ptr = (unsigned *)&VH264_cs[VH264_buf_num].REF_PIC_PTR_0;
        }
#else
        ref_idx_ptr = (unsigned *)&VH264_cs[VH264_buf_num].REF_PIC_PTR_0;



        findSinglePred00 = (findSinglePred[0] & (1 << 0)) >> 0;
        findSinglePred01 = (findSinglePred[0] & (1 << 1)) >> 1;
        findSinglePred02 = (findSinglePred[0] & (1 << 2)) >> 2;
        findSinglePred03 = (findSinglePred[0] & (1 << 3)) >> 3;

        findSinglePred10 = (findSinglePred[1] & (1 << 0)) >> 0;
        findSinglePred11 = (findSinglePred[1] & (1 << 1)) >> 1;
        findSinglePred12 = (findSinglePred[1] & (1 << 2)) >> 2;
        findSinglePred13 = (findSinglePred[1] & (1 << 3)) >> 3;

        partList00 = (findSinglePred00 == findSinglePred10) ? 0: (findSinglePred00 != 0) ? 0 : 1;
        partList01 = (findSinglePred01 == findSinglePred11) ? 0: (findSinglePred01 != 0) ? 0 : 1;
        partList02 = (findSinglePred02 == findSinglePred12) ? 0: (findSinglePred02 != 0) ? 0 : 1;
        partList03 = (findSinglePred03 == findSinglePred13) ? 0: (findSinglePred03 != 0) ? 0 : 1;

        partList10 = (findSinglePred00 == findSinglePred10) ? 1: (findSinglePred10 != 0) ? 1 : 0;
        partList11 = (findSinglePred01 == findSinglePred11) ? 1: (findSinglePred11 != 0) ? 1 : 0;
        partList12 = (findSinglePred02 == findSinglePred12) ? 1: (findSinglePred12 != 0) ? 1 : 0;
        partList13 = (findSinglePred03 == findSinglePred13) ? 1: (findSinglePred13 != 0) ? 1 : 0;
#endif

        *ref_idx_ptr++ = ref_pic_list[partList00][refIdxLX[partList00][0]];
        *ref_idx_ptr++ = ref_pic_list[partList01][refIdxLX[partList01][1]];
        *ref_idx_ptr++ = ref_pic_list[partList02][refIdxLX[partList02][2]];
        *ref_idx_ptr++ = ref_pic_list[partList03][refIdxLX[partList03][3]];

        *ref_idx_ptr++ = ref_pic_list[partList10][refIdxLX[partList10][0]];
        *ref_idx_ptr++ = ref_pic_list[partList11][refIdxLX[partList11][1]];
        *ref_idx_ptr++ = ref_pic_list[partList12][refIdxLX[partList12][2]];
        *ref_idx_ptr++ = ref_pic_list[partList13][refIdxLX[partList13][3]];


        *(dst_ptr_8x8 + 0 ) = (mvLX[partList00][0][0][0] & 0xFFFF)<< 16 | (mvLX[partList01][1][0][0] & 0xFFFF);
        *(dst_ptr_8x8 + 1 ) = (mvLX[partList02][2][0][0] & 0xFFFF)<< 16 | (mvLX[partList03][3][0][0] & 0xFFFF);
        *(dst_ptr_8x8 + 2 ) = (mvLX[partList00][0][0][1] & 0xFFFF)<< 16 | (mvLX[partList01][1][0][1] & 0xFFFF);
        *(dst_ptr_8x8 + 3 ) = (mvLX[partList02][2][0][1] & 0xFFFF)<< 16 | (mvLX[partList03][3][0][1] & 0xFFFF);

        *(dst_ptr_8x8 + 4 ) = (mvLX[partList10][0][0][0] & 0xFFFF)<< 16 | (mvLX[partList11][1][0][0] & 0xFFFF);
        *(dst_ptr_8x8 + 5 ) = (mvLX[partList12][2][0][0] & 0xFFFF)<< 16 | (mvLX[partList13][3][0][0] & 0xFFFF);
        *(dst_ptr_8x8 + 6 ) = (mvLX[partList10][0][0][1] & 0xFFFF)<< 16 | (mvLX[partList11][1][0][1] & 0xFFFF);
        *(dst_ptr_8x8 + 7 ) = (mvLX[partList12][2][0][1] & 0xFFFF)<< 16 | (mvLX[partList13][3][0][1] & 0xFFFF);

#if 0
        // copy the remaining refIndx in cs structure
        if (findSinglePred[1] == 0) {
            ref_idx_ptr = (unsigned *)&VH264_cs[VH264_buf_num].REF_PIC_PTR_b_0;
        } else {
            ref_idx_ptr = (unsigned *)&VH264_cs[VH264_buf_num].REF_PIC_PTR_0;
        }
        offset = !listIdx;

        for(int i = 0; i< 4; i++) {
            idx1 = ((findSinglePred[listIdx]) & (1 << i)) == 0 ? (!listIdx): (listIdx);
            *ref_idx_ptr = ref_pic_list[idx1][0];

            ref_idx_ptr++;

        }

        int k = 0, l= 0;
        for(int j = 0; j< 4; j+=2) {
            int p0 = ((findSinglePred[listIdx]) & (1 << j+0)) == 0 ? (!listIdx): (listIdx);
            int p1 = ((findSinglePred[listIdx]) & (1 << j+1)) == 0 ? (!listIdx): (listIdx);

            *(dst_ptr_8x8 + (4*offset + (l + 0))) = (mvLX[p0][k][0][0] & 0xFFFF)<< 16 | (mvLX[p1][k+1][0][0] & 0xFFFF);
            *(dst_ptr_8x8 + (4*offset + (l + 2))) = (mvLX[p0][k][0][1] & 0xFFFF)<< 16 | (mvLX[p1][k+1][0][1] & 0xFFFF);

            k +=2;
            l +=1;
        }
#endif
    }

#if 0
    else {

        *(dst_ptr_8x8 + 0) = mvLX[0][0][0][0] << 16 | (mvLX[0][1][0][0] & 0xFFFF);
        *(dst_ptr_8x8 + 1) = mvLX[0][2][0][0] << 16 | (mvLX[0][3][0][0] & 0xFFFF);
        *(dst_ptr_8x8 + 2) = mvLX[0][0][0][1] << 16 | (mvLX[0][1][0][1] & 0xFFFF);
        *(dst_ptr_8x8 + 3) = mvLX[0][2][0][1] << 16 | (mvLX[0][3][0][1] & 0xFFFF);
        *(dst_ptr_8x8 + 4) = mvLX[1][0][0][0] << 16 | (mvLX[1][1][0][0] & 0xFFFF);
        *(dst_ptr_8x8 + 5) = mvLX[1][2][0][0] << 16 | (mvLX[1][3][0][0] & 0xFFFF);
        *(dst_ptr_8x8 + 6) = mvLX[1][0][0][1] << 16 | (mvLX[1][1][0][1] & 0xFFFF);
        *(dst_ptr_8x8 + 7) = mvLX[1][2][0][1] << 16 | (mvLX[1][3][0][1] & 0xFFFF);
    }
#endif

    printf("%04x ", *(dst_ptr_8x8 + 0));
    printf("%04x\n",*(dst_ptr_8x8 + 1));
    printf("%08x ", *(dst_ptr_8x8 + 2));
    printf("%08x\n",*(dst_ptr_8x8 + 3));

    printf("%08x ", *(dst_ptr_8x8 + 4));
    printf("%08x\n",*(dst_ptr_8x8 + 5));
    printf("%08x ", *(dst_ptr_8x8 + 6));
    printf("%08x\n",*(dst_ptr_8x8 + 7));
    printf("End here\n");

}
typedef struct {
    U32 MB_TYPE; // 0x00 (H) | DBUF ADDR | MB_TYPE |
    U32 SP_INFO; // 0x00 (L) |

    U32 MB_y; // 0x01 (H)
    U32 MB_x; // 0x01 (L)

    U32 FILTER_PARAM2; // 0x03 (H)
    U32 MV_FWD_UNIT;   // 0x03 (L)

    U32 CURR_MB_ADDR;      // 0x04 (H)
    U32 CODED_BLOCK_FLAGS; // 0x04 (L)

    U32 DQ_COEF_C; // 0x05 (H)
    U32 DQ_COEF_Y; // 0x05 (L)


    U32 PRED_MODE_L16; // 0x06 (H)
    union {
        U32 PRED_MODE_C8;  // 0x06 (L)
        U32 DQ_COEF_Y__PRED_MODE_C8_8x8;
    }testUnion;

    union {
        struct {
            unsigned short VMB_mvx_L0[4]; // P16x16...P8x8 mapped to 8x8 blocks
            unsigned short VMB_mvy_L0[4]; // P16x16...P8x8 mapped to 8x8 blocks

            unsigned short VMB_mvx_L1[4]; // B-Slice L1 motion vectors
            unsigned short VMB_mvy_L1[4]; // B-Slice L1 motion vectors
        };

        unsigned short VMB_vector[8u];       //!< P16x16...P8x8 mapped to 8x8 blocks (old nomenclature, still in use for CABAC)
        unsigned int VMB_vector_x[8u];     //!< only used for P4x4
        unsigned short VMB_4x4vector_x[16u]; //!< proposal to be used when CALVLC support for P4x4 is added

        // the following struct stores motion vector components
        // all array elements must be initialized with 0
        // depending on the partitioning, either 1, 2, or 4 elements are filled
        // 16x16 -> motion vector  in array index 0
        // 16x8  -> motion vectors in array indices 0, 1
        //  8x16 -> motion vectors in array indices 0, 1
        //  8x8  -> motion vectors in array indices 0, 1, 2, 3
        struct {
            unsigned short VMP_mvxL0[4u]; //!< up to 4 motion vectors L0 x-component
            unsigned short VMP_mvyL0[4u]; //!< up to 4 motion vectors L0 y-component
            unsigned short VMP_mvxL1[4u]; //!< up to 4 motion vectors L1 x-component
            unsigned short VMP_mvyL1[4u]; //!< up to 4 motion vectors L1 y-component
        };
    }mvsNon4x4MBUnion;

    union {
        U32 VMB_vector_y[8u];     //!< only used for P4x4
        U16 VMB_4x4vector_y[16u]; //!< proposal to be used when CALVLC support for P4x4 is added
    }mvs4x4MBUnion;

    union {
        struct {  //Structure for Intra Prediction mode in I slice
            U32 PREV1msb;
            U32 PREV1lsb;

            U32 REM1msb;
            U32 REM1lsb;

            U32 PREV2msb;
            U32 PREV2lsb;

            U32 REM2msb;
            U32 REM2lsb;
        };

        struct {
            U32 REF_PIC_PTR_0; // reference index for part0(8x8) in list 0
            U32 REF_PIC_PTR_1; // reference index for part1(8x8) in list 0
            U32 REF_PIC_PTR_2; // reference index for part2(8x8) in list 0
            U32 REF_PIC_PTR_3; // reference index for part3(8x8) in list 0

            U32 REF_PIC_PTR_b_0; // reference index for part0(8x8) in list 1
            U32 REF_PIC_PTR_b_1; // reference index for part1(8x8) in list 1
            U32 REF_PIC_PTR_b_2; // reference index for part2(8x8) in list 1
            U32 REF_PIC_PTR_b_3; // reference index for part3(8x8) in list 1
        };

        // the following struct stores the external frame address
        // corresponding to the reference index valid for a particular 8x8 partition
        // all array elements must be initialized with 0
        // L0 elements are to be filled if L0 motion compensation is used for that sub-partition
        // L1 elements are to be filled if L1 motion compensation is used for that sub-partition
        struct {
            U32 refidxAddrL0[4u]; //!< address for L0 reference frame (8x8 partition 0...3)
            U32 refidxAddrL1[4u]; //!< address for L1 reference frame (8x8 partition 0...3)
        };

    }refIdxUnion;

} cmb;

typedef struct {
    U32 MB_TYPE; // 0x00 (H) | DBUF ADDR | MB_TYPE |
    U32 SP_INFO; // 0x00 (L) |

    U32 MB_y; // 0x01 (H)
    U32 MB_x; // 0x01 (L)

    U32 FILTER_PARAM2; // 0x03 (H)
    U32 MV_FWD_UNIT;   // 0x03 (L)

    U32 CURR_MB_ADDR;      // 0x04 (H)
    U32 CODED_BLOCK_FLAGS; // 0x04 (L)

    U32 DQ_COEF_C; // 0x05 (H)
    U32 DQ_COEF_Y; // 0x05 (L)


    U32 PRED_MODE_L16; // 0x06 (H)

    union {
        U32 PRED_MODE_C8;  // 0x06 (L)
        U32 DQ_COEF_Y__PRED_MODE_C8_8x8;
    };

    union {
        struct {
            unsigned short VMB_mvx_L0[4]; // P16x16...P8x8 mapped to 8x8 blocks
            unsigned short VMB_mvy_L0[4]; // P16x16...P8x8 mapped to 8x8 blocks

            unsigned short VMB_mvx_L1[4]; // B-Slice L1 motion vectors
            unsigned short VMB_mvy_L1[4]; // B-Slice L1 motion vectors
        };

        unsigned short VMB_vector[8u];       //!< P16x16...P8x8 mapped to 8x8 blocks (old nomenclature, still in use for CABAC)
        unsigned int VMB_vector_x[8u];     //!< only used for P4x4
        unsigned short VMB_4x4vector_x[16u]; //!< proposal to be used when CALVLC support for P4x4 is added

        // the following struct stores motion vector components
        // all array elements must be initialized with 0
        // depending on the partitioning, either 1, 2, or 4 elements are filled
        // 16x16 -> motion vector  in array index 0
        // 16x8  -> motion vectors in array indices 0, 1
        //  8x16 -> motion vectors in array indices 0, 1
        //  8x8  -> motion vectors in array indices 0, 1, 2, 3
        struct {
            unsigned short VMP_mvxL0[4u]; //!< up to 4 motion vectors L0 x-component
            unsigned short VMP_mvyL0[4u]; //!< up to 4 motion vectors L0 y-component
            unsigned short VMP_mvxL1[4u]; //!< up to 4 motion vectors L1 x-component
            unsigned short VMP_mvyL1[4u]; //!< up to 4 motion vectors L1 y-component
        };
    };

    union {
        U32 VMB_vector_y[8u];     //!< only used for P4x4
        U16 VMB_4x4vector_y[16u]; //!< proposal to be used when CALVLC support for P4x4 is added
    };

    union {
        struct {  //Structure for Intra Prediction mode in I slice
            U32 PREV1msb;
            U32 PREV1lsb;

            U32 REM1msb;
            U32 REM1lsb;

            U32 PREV2msb;
            U32 PREV2lsb;

            U32 REM2msb;
            U32 REM2lsb;
        };

        struct {
            U32 REF_PIC_PTR_0; // reference index for part0(8x8) in list 0
            U32 REF_PIC_PTR_1; // reference index for part1(8x8) in list 0
            U32 REF_PIC_PTR_2; // reference index for part2(8x8) in list 0
            U32 REF_PIC_PTR_3; // reference index for part3(8x8) in list 0

            U32 REF_PIC_PTR_b_0; // reference index for part0(8x8) in list 1
            U32 REF_PIC_PTR_b_1; // reference index for part1(8x8) in list 1
            U32 REF_PIC_PTR_b_2; // reference index for part2(8x8) in list 1
            U32 REF_PIC_PTR_b_3; // reference index for part3(8x8) in list 1
        };

        // the following struct stores the external frame address
        // corresponding to the reference index valid for a particular 8x8 partition
        // all array elements must be initialized with 0
        // L0 elements are to be filled if L0 motion compensation is used for that sub-partition
        // L1 elements are to be filled if L1 motion compensation is used for that sub-partition
        struct {
            U32 refidxAddrL0[4u]; //!< address for L0 reference frame (8x8 partition 0...3)
            U32 refidxAddrL1[4u]; //!< address for L1 reference frame (8x8 partition 0...3)
        };

    };

} cmb1;

int main() {

    printf("sizeof checksize is:%d\n", sizeof(cmb));
    // Get the size of the first union (mv4x4Union)
    size_t sizeOftestUnion = sizeof(((cmb*)0)->testUnion);
    size_t sizeOfmvsNon4x4MBUnion = sizeof(((cmb*)0)->mvsNon4x4MBUnion);
    size_t sizeOfmvs4x4MBUnion = sizeof(((cmb*)0)->mvs4x4MBUnion);
    size_t sizeOfrefIdxUnion = sizeof(((cmb*)0)->refIdxUnion);

    printf("sizeof(((cmb*)0)->testUnion):%d\n", sizeof(((cmb*)0)->testUnion));
    printf("sizeof(((cmb*)0)->mvsNon4x4MBUnion):%d\n", sizeof(((cmb*)0)->mvsNon4x4MBUnion));
    printf("sizeof(((cmb*)0)->mvs4x4MBUnion):%d\n", sizeof(((cmb*)0)->mvs4x4MBUnion));
    printf("sizeof(((cmb*)0)->refIdxUnion):%d\n----\n", sizeof(((cmb*)0)->refIdxUnion));

    printf("sizeof(((cmb1*)0)->testUnion):%d\n", sizeof(((cmb1*)0)->DQ_COEF_Y__PRED_MODE_C8_8x8));
    printf("sizeof(((cmb1*)0)->mvsNon4x4MBUnion):%d\n", sizeof(((cmb1*)0)->VMB_4x4vector_x));
    printf("sizeof(((cmb1*)0)->mvs4x4MBUnion):%d\n", sizeof(((cmb1*)0)->VMB_4x4vector_y));
    printf("sizeof(((cmb1*)0)->PREV1msb):%d\n----\n", sizeof(((cmb1*)0)->PREV1msb));

    int findPredList0 = 0x1011;
    int findPredList1 = 0xFF0F;

    int findSinglePred00, findSinglePred01, findSinglePred02, findSinglePred03;
    int findSinglePred10, findSinglePred11, findSinglePred12, findSinglePred13;

    findSinglePred00 = (findPredList0 & (1 << 0)) >> 0;
    findSinglePred01 = (findPredList0 & (1 << 4)) >> 4;
    findSinglePred02 = (findPredList0 & (1 << 8)) >> 8;
    findSinglePred03 = (findPredList0 & (1 << 12)) >> 12;

    findSinglePred10 = (findPredList1 & (1 <<0))  >> 0;
    findSinglePred11 = (findPredList1 & (1 <<4))  >> 1;
    findSinglePred12 = (findPredList1 & (1 <<8))  >> 2;
    findSinglePred13 = (findPredList1 & (1 <<12)) >> 3;

    update_MVL();

    UPDATE_CS_STRUCTURE8X8();
    //UPDATE_CS_STRUCTURE8X16();
    //UPDATE_CS_STRUCTURE16X8();
    //UPDATE_CS_STRUCTURE16X16();

#if 0
    __MC_8x8_MODE();
    __MC_8x8_MODE_TO_CS();
#endif
    //__MC_8x8_MODE_TO_CS2();
    __MC_16x16_MODE_TO_CS();

    //__MC_8x8_MODE_TO_CS2();

    return 0;
}

#else
#include <stdio.h>

typedef unsigned int U32;
typedef unsigned long long U64;

typedef struct {
    U32 MB_TYPE; // 0x00 (H) | DBUF ADDR | MB_TYPE |
    U32 SP_INFO; // 0x00 (L) |

    U32 MB_y; // 0x01 (H)
    U32 MB_x; // 0x01 (L)

    U32 FILTER_PARAM2; // 0x03 (H)
    U32 MV_FWD_UNIT;   // 0x03 (L)

    U32 CURR_MB_ADDR;      // 0x04 (H)
    U32 CODED_BLOCK_FLAGS; // 0x04 (L)

    U32 DQ_COEF_C; // 0x05 (H)
    U32 DQ_COEF_Y; // 0x05 (L)


    U32 PRED_MODE_L16; // 0x06 (H)
    union {
        U32 PRED_MODE_C8;  // 0x06 (L)
        U32 DQ_COEF_Y__PRED_MODE_C8_8x8;
    } mv4x4Union;

    union {
        struct {
            unsigned short VMB_mvx_L0[4]; // P16x16...P8x8 mapped to 8x8 blocks
            unsigned short VMB_mvy_L0[4]; // P16x16...P8x8 mapped to 8x8 blocks

            unsigned short VMB_mvx_L1[4]; // B-Slice L1 motion vectors
            unsigned short VMB_mvy_L1[4]; // B-Slice L1 motion vectors
        };

        unsigned short VMB_vector[8u];       //!< P16x16...P8x8 mapped to 8x8 blocks (old nomenclature, still in use for CABAC)
        unsigned int VMB_vector_x[8u];     //!< only used for P4x4
        unsigned short VMB_4x4vector_x[16u]; //!< proposal to be used when CALVLC support for P4x4 is added

        // the following struct stores motion vector components
        // all array elements must be initialized with 0
        // depending on the partitioning, either 1, 2, or 4 elements are filled
        // 16x16 -> motion vector  in array index 0
        // 16x8  -> motion vectors in array indices 0, 1
        //  8x16 -> motion vectors in array indices 0, 1
        //  8x8  -> motion vectors in array indices 0, 1, 2, 3
        struct {
            unsigned short VMP_mvxL0[4u]; //!< up to 4 motion vectors L0 x-component
            unsigned short VMP_mvyL0[4u]; //!< up to 4 motion vectors L0 y-component
            unsigned short VMP_mvxL1[4u]; //!< up to 4 motion vectors L1 x-component
            unsigned short VMP_mvyL1[4u]; //!< up to 4 motion vectors L1 y-component
        };
    }mvsNon4x4MBUnion;

    union {
        // Third union definition
    } mv4x4Union2;

    union {
        // Fourth union definition
    } refIdxUnion;
} cmb;

int main() {
    // Get the size of cmb
    size_t sizeOfCmb = sizeof(cmb);

    printf("Size of cmb: %zu bytes\n", sizeOfCmb);

    // Get the size of the first union (mv4x4Union)
    size_t sizeOfFirstUnion = sizeof(((cmb*)0)->mvsNon4x4MBUnion);

    printf("Size of mv4x4Union: %zu bytes\n", sizeOfFirstUnion);

    // Get the size of the second union (IDCT_DATA)
    //size_t sizeOfSecondUnion = sizeof(((cmb*)0)->IDCT_DATA);

    //printf("Size of IDCT_DATA: %zu bytes\n", sizeOfSecondUnion);

    // Get the size of the third union (mv4x4Union2)
    size_t sizeOfThirdUnion = sizeof(((cmb*)0)->mv4x4Union2);

    printf("Size of mv4x4Union2: %zu bytes\n", sizeOfThirdUnion);

    // Get the size of the fourth union (refIdxUnion)
    size_t sizeOfFourthUnion = sizeof(((cmb*)0)->refIdxUnion);

    printf("Size of refIdxUnion: %zu bytes\n", sizeOfFourthUnion);

    return 0;
}

#endif
