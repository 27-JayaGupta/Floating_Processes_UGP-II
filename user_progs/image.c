#include <stdio.h>
#include <math.h>
#include <unistd.h>

int main() {
    int integerResult;
    float floatResult;
    double doubleResult;

    int i= 0;
    while (1) {
        // Integer computation
        integerResult = 5 * 10 + 3;

        // Floating-point computation
        floatResult = sqrt(25.0) + 2.5;

        // Double precision computation
        doubleResult = pow(2.0, 10.0) - 1.0;

        // Print the results
        printf("[%d][PID: %d]: Integer Result: %d\n",  i, getpid(),integerResult);
        printf("[%d][PID: %d]: Float Result: %.2f\n",  i, getpid(),floatResult);
        printf("[%d][PID: %d]: Double Result: %.2lf\n",  i, getpid(),doubleResult);
        i++;

        // Introduce a delay to avoid excessive CPU usage in the infinite loop
        sleep(5);
    }

    return 0;
}
