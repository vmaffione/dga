#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cmath>

using namespace std;


#define COLS    5
#define ROWS    100

static float coeff_matrix[ROWS][COLS];

float crazypoly(float x)
{
    unsigned int idx = static_cast<unsigned int>(abs(x)) % ROWS;
    float value = 0.0;

    for (unsigned int j = 0; j < COLS; j++) {
        value += pow(x, j) * coeff_matrix[idx][j];
    }

    return abs(value);
}

int main()
{
    srand(time(NULL));

    for (unsigned int i = 0; i < ROWS; i++) {
        for (unsigned int j = 0; j < COLS; j++) {
            coeff_matrix[i][j] = static_cast<float>((rand() % 1000) - 500);
        }
    }

#if 0
    for (unsigned int i = 0; i < ROWS; i++) {
        for (unsigned int j = 0; j < COLS; j++) {
            cout << coeff_matrix[i][j] << " ";
        }
        cout << endl;
    }
#endif

    cout << crazypoly(1.7) << endl;

    return 0;
}
