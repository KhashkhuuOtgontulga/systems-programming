/* Khashkhuu Otgontulga
 * A20379665
 *
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
    /* parameters (s,E,b)
     * s = 5 so 2^5 = 32 sets
     * E = 1 so it is a direct mapped cache
     * b = 5 so 2^5 = block size of 32 bytes
     * The naive approach has 3 issues:
     * (1) Bad spatial locality
     * (2) Working set size is high
     * (3) Bad eviction policy
     * 2 main ideas: blocking and looking at the miss patterns 
     * input is M X N so there are 3 cases:
     * (1) 32 x 32 (M=32, N=32)
     * (2) 64 x 64 (M=64, N=64)
     * (3) 61 x 67 (M=61, N=67)
     * 
     * Notes:
     * - We divide the matrix into 4 submatrices by the block_size that is specific to the
     * size of the matrix to have better locality and a less working set size.
     * - We want 4 for loops: 2 to iterate over the blocks and then another 2 to perform 
     * the transpose copy of a single block.
     * - Copying the elements into a temporary variable allows us to have less misses 
     * but at the cost that we have 2 memory accesses. However, this is okay because
     * we utilize the cache more.
     * - We have to be careful about the diagonal because we are using a direct mapped cache
     * so there are potential conflict misses that arise.
     * - The block size should be a multiple of the matrix size.
     * 
     * Hints:
     * - http://csapp.cs.cmu.edu/public/waside/waside-blocking.pdf
     * - run ./csim-ref -v -s 5 -E 1 -b 5 -t trace.f0
     */ 
     /* case 1 */
     int block_size;
     int block_row, block_column, i, j;
     int temp = 0, diagonal = 0;
     
     if (M == 32 && N == 32) {
     /* split up the matrix into 16 blocks */
     block_size = 8;
 	for (block_column = 0; block_column < N; block_column += block_size)
	{
    		for (block_row = 0; block_row < N; block_row += block_size)
    		{
			for (i = block_row; i < block_row + block_size; i++)
        		{
            			for (j = block_column; j < block_column + block_size; j++)
            			{
            				/* if the row number is not equal to the column number, we don't have 
 					 * conflict misses on the diagonal so we just transpose the element */
					if (i != j) {
						B[j][i] = A[i][j];
					}
					/* if the row number is equal to the column number, we have met a 
 					 * diagonal element so we must avoid conflict misses on the diagonal */
					else {
						/* store the element into a temporary variable so we don't 
 						 * access this element again and save the position of the diagonal
 						 * so that we can transpose the saved element of A into array B */
						temp = A[i][j];
						diagonal = i;
					}
				}
				/* if the block row and block column are equal, then that means that we are dealing
 				 * with a diagnol element so we must take the temporary element from A and transpose it 
 				 * to B */
				if (block_row == block_column) {
					B[diagonal][diagonal] = temp;
				}
       			}
    		}
	}
    }
     /* case 2 */
     else if (M == 64 && N == 64) {
     /* split up the matrix into 256 blocks */
     block_size = 4;
	for (block_column = 0; block_column < N; block_column += block_size)
	{
    		for (block_row = 0; block_row < N; block_row += block_size)
    		{
    			for (i = block_row; i < block_row + block_size; i++)
                        {
                                for (j = block_column; j < block_column + block_size; j++)
                                {
                                        if (i != j) {
                                                B[j][i] = A[i][j];
                                        }
                                        else {
                                                temp = A[i][j];
                                                diagonal = i;
                                        }
                                }
                                if (block_row == block_column) {
                                        B[diagonal][diagonal] = temp;
                                }
                        }
		}
	}

     }
     /* case 3 */
     else if (M == 61 && N == 67) {
     /* block size that gives the lowest amount of misses */
     block_size = 16;
	for (block_column = 0; block_column < N; block_column += block_size)
	{
    		for (block_row = 0; block_row < N; block_row += block_size)
    		{
			/* we also need to check if we are in the boundaries of the matrix size M and N 
 			 * to avoid invalid accesses because the matrix is odd and thus does not evenly form a square */
        		for (i = block_row; (i < block_row + block_size) && (i < N); i++)
        		{

            			for (j = block_column; (j < block_column + block_size) && (j < M); j++)
           			{
            				if (i != j) {
						B[j][i] = A[i][j];
					}
					else {
						temp = A[i][j];
						diagonal = i;
					}
				}
				
				if (block_row == block_column) {
					B[diagonal][diagonal] = temp;

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

