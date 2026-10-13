#include "test.h"

int test_failures;

void test_gfx(void);

int main(void)
{
    test_gfx();

    if (test_failures) {
        printf("%d check(s) FAILED\n", test_failures);
        return 1;
    }
    printf("all tests passed\n");
    return 0;
}
