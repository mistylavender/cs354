////////////////////////////////////////////////////////////////////////////////
// Grade Group:      gg2
// Author:           Kaeya Kapoor
// CS Login:         nkapoor
////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#define ROWS 128
#define COLS 8
int arr2D[ROWS][COLS];

int main() {
    for (int iteration = 0; iteration < 100; iteration++) {
        for (int row = 0; row < ROWS; row += 64) {
            for (int col = 0; col < COLS; col++) {
                arr2D[row][col] = iteration + row + col;
            }
        }
    }
    return 0;
}
