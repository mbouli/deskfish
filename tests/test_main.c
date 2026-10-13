#include "test.h"

int test_failures;

void test_gfx(void);
void test_species(void);
void test_assets(void);

int main(void)
{
    test_gfx();
    test_species();
    test_assets();

    if (test_failures) {
        printf("%d check(s) FAILED\n", test_failures);
        return 1;
    }
    printf("all tests passed\n");
    return 0;
}
