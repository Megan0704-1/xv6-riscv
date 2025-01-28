#include "kernel/types.h"
#include "user/user.h"

// Multiply-and-Accummulate function - software implementation
// int mac(int a, int b, int c) {
//     return a * b + c;
// }


static inline __attribute__((always_inline)) int mac(int a, int b, int c) {
    int res;
    asm volatile(
        "mv a0, %1        \n"  // Load a
        "mv a1, %2        \n"  // Load b
        "mv a2, %3        \n"  // Load c
        ".word 0x316a50b  \n"  // Custom instruction
        "mv %0, a0        \n"  // Fetch result
        :
        "=r"(res)
        :
        "r"(a), "r"(b), "r"(c)
        : "a0", "a1", "a2"
    );

    return res;
}

int main() {
    //fprintf(2, "Basic matmul test: %d\n", mac(1,2,3));

    // Example matrices A (2x3) and B (3x2)
    int A[2][3] = {{1, 2, 3}, {4, 5, 6}};  // 2x3 matrix
    int B[3][2] = {{7, 8}, {9, 10}, {11, 12}};  // 3x2 matrix
    
    // Resulting matrix C (2x2)
    int C[2][2] = {0};  // Initialize with zeroes


    // Matrix multiplication using MAC function
    for (int i = 0; i < 2; i++) {  // Loop over rows of A
        for (int j = 0; j < 2; j++) {  // Loop over columns of B
            int result = 0;  // Initialize the accumulation variable
            for (int k = 0; k < 3; k++) {  // Loop over columns of A and rows of B
                result = mac(A[i][k], B[k][j], result);
            }
            C[i][j] = result;  // Store the result in the corresponding position in C
        }
    }

    // Output the resulting matrix C
    fprintf(2, "Resulting Matrix C (A * B):\n");
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            fprintf(2, "%d ", C[i][j]);
        }
        fprintf(2, "\n");
    }

    return 0;
}

