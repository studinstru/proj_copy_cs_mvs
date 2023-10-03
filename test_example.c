#include <stdio.h>>

unsigned int mvs[20];
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

    U32 VMB_vector_x[8];

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
} CTRL_STRUCT;

/*********************************/
/* global control structure data */
/*********************************/
volatile CTRL_STRUCT VH264_cs[2];


void UPDATE_CS_STRUCTURE8X16() {

    // 8x16
    int *dst_ptr_8x8, z;
    int refIdxLX[2][4] = {{4,0,0,0},{0,0,0,0}};

    int ref_pic_list[2][6] = {{0x60000000, 0x70000000, 0x80000000, 0x90000000, 0xa0000000,0xb0000000}, {0x22220000, 0x33330000, 0x44440000, 0x55550000, 0x66660000, 0x77770000}};

    int findSinglePred[9][2] = {{0x3, 0x0}, {0x0, 0x3}, {0x1, 0x2}, {0x2, 0x1}, {0x3, 0x2}, {0x2, 0x3}, {0x3, 0x1}, {0x1, 0x3}, {0x3, 0x3}};
    int PredMode[2] = {51, 51};

    int VH264_buf_num = 0;

    dst_ptr_8x8 = (int  *)&VH264_cs[0].VMB_vector_x[0];

    mvLX[0][0][0][0] = -4;
    mvLX[0][1][0][0] = -1;
    mvLX[0][2][0][0] = 0;
    mvLX[0][3][0][0] = 1;

    mvLX[0][0][0][1] = 2;
    mvLX[0][1][0][1] = 2;
    mvLX[0][2][0][1] = 4;
    mvLX[0][3][0][1] = 0;

    mvLX[1][0][0][0] = 1;
    mvLX[1][1][0][0] = 3;
    mvLX[1][2][0][0] = 0;
    mvLX[1][3][0][0] = 2;

    mvLX[1][0][0][1] = -4;
    mvLX[1][1][0][1] = -3;
    mvLX[1][2][0][1] = 5;
    mvLX[1][3][0][1] = 0;

    int idx0, idx1, offset, listIdx = 0;

    int partList00, partList01, partList02, partList03;
    int partList10, partList11, partList12, partList13;

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

    for(z = 0; z < 1; z++) {

        if(findSinglePred[z][0] == findSinglePred[z][1])
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

            partList00 = ((findSinglePred[z][0] & (1 << 0)) != 0) ? 0 : 1;
            partList01 = ((findSinglePred[z][0] & (1 << 1)) != 0) ? 0 : 1;
            partList10 = ((findSinglePred[z][1] & (1 << 0)) != 0) ? 1 : 0;
            partList11 = ((findSinglePred[z][1] & (1 << 1)) != 0) ? 1 : 0;


            mv_p0x0 = ((unsigned)mvLX[partList00][0][0][0]) & 0xffff;
            mv_p0y0 = ((unsigned)mvLX[partList00][0][0][1]) & 0xffff;

            mv_p0x0 = (mv_p0x0 << 16) | mv_p0x0;
            mv_p0y0 = (mv_p0y0 << 16) | mv_p0y0;

            mv_p1x0 = ((unsigned)mvLX[partList01][1][0][0]) & 0xffff;
            mv_p1y0 = ((unsigned)mvLX[partList01][1][0][1]) & 0xffff;

            *cs_ptr++ = mv_p0x0 << 16 | mv_p1x0; // test git add
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
    }

}
void initializeMotionVectors() {
    int i, j, k, l;
    int mvLXValues[2][4][4][2] = {
        {{{-4, 2}, {-1, 2}, {0, 4}, {1, 0}}, {{1, -4}, {3, -3}, {0, 5}, {2, 0}}},
        {{{0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}}}
    };

    for(i = 0; i < 2; i++) {
        for(j = 0; j < 4; j++) {
            for(k = 0; k < 4; k++) {
                for(l = 0; l < 2; l++) {
                    VH264_cs[i].VMB_vector_x[j*4 + k] = mvLXValues[i][j][k][l];
                }
            }
        }
    }
}

void setRefPicList(int listIdx, int refIdxLX[2][4], int ref_pic_list[2][6], unsigned int * ref_idx_ptr) {
    int i;
    int partList00, partList01, partList10, partList11;
    unsigned int  mv_p0x, mv_p0y, mv_p1x, mv_p1y;

    for(i = 0; i < 9; i++) {
        partList00 = (i & 1) ? 0 : 1;
        partList01 = (i & 2) ? 0 : 1;
        partList10 = (i & 4) ? 1 : 0;
        partList11 = (i & 8) ? 1 : 0;

        mv_p0x = VH264_cs[partList00].VMB_vector_x[0] & 0xFFFF;
        mv_p0y = VH264_cs[partList00].VMB_vector_x[1] & 0xFFFF;
        mv_p1x = VH264_cs[partList01].VMB_vector_x[2] & 0xFFFF;
        mv_p1y = VH264_cs[partList01].VMB_vector_x[3] & 0xFFFF;

        ref_idx_ptr[0] = ref_pic_list[listIdx][refIdxLX[listIdx][0]];
        ref_idx_ptr[1] = ref_pic_list[listIdx][refIdxLX[listIdx][1]];
        ref_idx_ptr[2] = ref_pic_list[listIdx][refIdxLX[listIdx][0]];
        ref_idx_ptr[3] = ref_pic_list[listIdx][refIdxLX[listIdx][1]];

        ref_idx_ptr[4] = ref_pic_list[1 - listIdx][refIdxLX[1 - listIdx][0]];
        ref_idx_ptr[5] = ref_pic_list[1 - listIdx][refIdxLX[1 - listIdx][1]];
        ref_idx_ptr[6] = ref_pic_list[1 - listIdx][refIdxLX[1 - listIdx][0]];
        ref_idx_ptr[7] = ref_pic_list[1 - listIdx][refIdxLX[1 - listIdx][1]];

        ref_idx_ptr += 8;
    }
}
void OPT_CS_STRUCTURE8X16() {

    int refIdxLX[2][4] = {{4, 0, 0, 0}, {0, 0, 0, 0}};
    int ref_pic_list[2][6] = {{0x60000000, 0x70000000, 0x80000000, 0x90000000, 0xa0000000, 0xb0000000},
                             {0x22220000, 0x33330000, 0x44440000, 0x55550000, 0x66660000, 0x77770000}};

    initializeMotionVectors();

    setRefPicList(0, refIdxLX, ref_pic_list, (unsigned int  *)&VH264_cs[VH264_buf_num].REF_PIC_PTR_0);
    setRefPicList(1, refIdxLX, ref_pic_list, (unsigned int  *)&VH264_cs[VH264_buf_num].REF_PIC_PTR_b_0);

}
int main() {

    UPDATE_CS_STRUCTURE8X16();
    OPT_CS_STRUCTURE8X16();
    return 0;
}
