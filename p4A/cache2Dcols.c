////////////////////////////////////////////////////////////////////////////////
// Grade Group:      gg2
// Author:           Kaeya Kapoor
// CS Login:         nkapoor
////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#define ROWS 3000
#define COLS 500
int arr2D[ROWS][COLS];

int main() {
    for (int col = 0; col < COLS; col++) {
        for (int row = 0; row < ROWS; row++) {
            arr2D[row][col] = row + col;
        }
    }
    return 0;
}

