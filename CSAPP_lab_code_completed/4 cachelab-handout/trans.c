/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    // block size is 32 bytes --> 8 integers
    // set number is also 32

    /*  when it is 32 x 32, the addresses of the element at the same position in the matrix A and matrix B are algined 
     *  (or in the same set number which will cause the eviction)
     *  the increment is 32*4=2**7=0x80=100 00000 and there are 32 sets (10000 00000). 100->2**2, 10000->2**5
     *  Therefore, every 8-rows is a loop of new set number, where 2**5/2**2=2**3=8.
     */ 

    /* when it is 64 x 63, the increment is 64*4=2**8=1000 00000
     * therefore, it is every 4-rows a loop for a new set number causing a confliction/eviction.
     * however, when we set the block size as 4, 
     * the number of the misses is 1795 which is still larger than the requiement
     * we need to take care of the submatrices whih are off-diagonal for very 8 x 8 block
     */

    // when it is 61 x 67, the increment is 67*4= 1000 01100, which is not a whole number (or not aligned)
    // a random thing,
    // just try different block_sizes to find one satisify the requirement, which # of misses is less than 2000


    int block_size;
    int i, j, k, l;
    int tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;

    if (N==32) {
        block_size = 8;
        for (i = 0; i < N; i += block_size) {
            for (j = 0; j < M; j += block_size) {

                // block[i][j]: row first-->inner loop
                for (k = i; k < i + block_size && k < N; k++) {
                    for (l = j; l < j + block_size && l < M; l++) {
                        if (i == j && k == l) {
                            tmp0 = A[k][l];
                        } 
                        else {
                            B[l][k] = A[k][l];
                        }
                    }
                    // left the confliction (where B has the same set number with A) to implete at last
                    if (i==j) {
                        B[k][k] = tmp0;
                        }
                    
                }
            }
        }
    }

    else if (N==64) {
        block_size = 8;
        for (i = 0; i < N; i += block_size) {
            for (j = 0; j < M; j += block_size) {

                // implement for upper half (4 x 8) in matrix A
                // because block size is 32 bytes
                // we extract all the integers in one cache block and stored in tmp
                /*  matrix A                               matrix B (each sub_matrix is 4*4, here I show 16*16 matrix)
                    --------|--------|--------|--------    |--------|--------|--------|--------
                   |--rowA1-|--rowA2-|--rowA3-|--rowA4-    | | | | || | | | ||        |        
                   |--rowA1-|--rowA2-|--rowA3-|--rowA4-    |columnA1|columnA2|        |
                   |--rowA1-|--rowA2-|--rowA3-|--rowA4-    | | | | || | | | ||        |        
                   |--rowA1-|--rowA2-|--rowA3-|--rowA4-    | | | | || | | | ||        |        
                    --------|--------|--------|--------     --------|--------|--------|--------
                   |        |        |        |            |        |        |        |        
                   |        |        |        |            |        |        |        |            
                   |        |        |        |            |        |        |        |        
                   |        |        |        |            |        |        |        |        
                    --------|--------|--------|--------     --------|--------|--------|--------
                   |        |        |        |            | | | | || | | | ||        |        
                   |        |        |        |            |columnA3|columnA4|        |        
                   |        |        |        |            | | | | || | | | ||        |        
                   |        |        |        |            | | | | || | | | ||        |        
                    --------|--------|--------|--------     --------|--------|--------|--------
                   |        |        |        |            |        |        |        |        
                   |        |        |        |            |        |        |        |        
                   |        |        |        |            |        |        |        |        
                   |        |        |        |            |        |        |        |        
                    --------|--------|--------|--------    --------|--------|--------|--------
                */
                for (k=i; k < i + block_size/2; k++) {
                    //for both diagonal and off-diagonal, one eviction per iteration for loading
                    tmp0 = A [k][j];
                    tmp1 = A [k][j+1];
                    tmp2 = A [k][j+2];
                    tmp3 = A [k][j+3];
                    tmp4 = A [k][j+4];
                    tmp5 = A [k][j+5];
                    tmp6 = A [k][j+6];
                    tmp7 = A [k][j+7];

                    // for first iteration, it has 4 evictions
                    // for diagonal, others have 1 evictions per iteration because the loading one cache line of A need to restore back to B
                    // for off-diagonal, 0 eviction
                    B[j][k] = tmp0;
                    B[j+1][k] = tmp1;
                    B[j+2][k] = tmp2;
                    B[j+3][k] = tmp3;

                    B[j][k+4] = tmp4;
                    B[j+1][k+4] = tmp5;
                    B[j+2][k+4] = tmp6;
                    B[j+3][k+4] = tmp7;

                }
                
                /* capital A&B is for i=j=0, lower case a&b is for i=0,j=8
                    matrix A                                matrix B
                    --------|--------|--------|--------     --------|--------|--------|--------
                   |        |        |        |            |        |B B B B1|        |        
                   |        |        |        |            |        |        |        |            
                   |        |        |        |            |        |        |        |        
                   |        |        |        |            |        |        |        |    
                    --------|--------|--------|--------     --------|--------|--------|--------
                   |A1      |A2      |a1      |            |B B B B2|B B B B3|        |        
                   |A1      |A2      |a1      |            |        |        |        |            
                   |A1      |A2      |a1      |            |        |        |        |        
                   |A1      |A2      |a1      |            |        |        |        |        
                    --------|--------|--------|--------     --------|--------|--------|--------
                   |        |        |        |            |        |b b b b1|        |        
                   |        |        |        |            |        |        |        |        
                   |        |        |        |            |        |        |        |        
                   |        |        |        |            |        |        |        |        
                    --------|--------|--------|--------     --------|--------|--------|--------
                   |        |        |        |            |b b b b2|b b b b3|        |        
                   |        |        |        |            |        |        |        |        
                   |        |        |        |            |        |        |        |        
                   |        |        |        |            |        |        |        |        
                    --------|--------|--------|--------    --------|--------|--------|--------
                    */
                for (k = 0; k < block_size/2; k++) {
                    
                    // 4 evictions per iteration for first iteration
                    // for diagonal, others is one eviction
                    // for off-diagonal, others is 0 eviction
                    tmp0 = A[i+4][k+j];
                    tmp1 = A[i+5][k+j];
                    tmp2 = A[i+6][k+j];
                    tmp3 = A[i+7][k+j];

                    // for diagonal, 1 eviction per iteration
                    // for off-diagonal, it already loaded from previous loop
                    tmp4 = B[k+j][i+4];
                    tmp5 = B[k+j][i+5];
                    tmp6 = B[k+j][i+6];
                    tmp7 = B[k+j][i+7];

                    // 0 eviction
                    B[k+j][i+4] = tmp0;
                    B[k+j][i+5] = tmp1;
                    B[k+j][i+6] = tmp2;
                    B[k+j][i+7] = tmp3;

                    // for both diagonal and off-diagonal, 1 eviction periteration
                    B[k+j+4][i] = tmp4;
                    B[k+j+4][i+1] = tmp5;
                    B[k+j+4][i+2] = tmp6;
                    B[k+j+4][i+3] = tmp7;

                    // 2 evictions for diagonal submatrix
                    // 0 evictions for off-diagonal
                    //-- store the last block straight from A
                    for (l = 0; l < block_size/2; l++) {
                        B[k+j+4][i+4+l] = A[i+4+l][k+j+4];
                    }
                }
                
            }

        }
        // summary: one submatrix on diagonal is 11+23 misses, one submatrix on off-diagonal is 8+8 misses
    }
    else {
        block_size = 16;
        for (i = 0; i < N; i += block_size) {
            for (j = 0; j < M; j += block_size) {

                // block[i][j]: row first-->inner loop
                for (k = i; k < i + block_size && k < N; k++) {
                    for (l = j; l < j + block_size && l < M; l++) {
                        if (i == j && k == l) {
                            tmp0 = A[k][l];
                        } 
                        else {
                            B[l][k] = A[k][l];
                        }
                    }
                    // left the confliction (where B has the same set number with A) to implete at last
                    if (i==j) {
                        B[k][k] = tmp0;
                        }
                    
                }
            }
        }
    }
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 


    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

