#include "test.h"

int test_failures;

void test_brain(void);
void test_gfx(void);
void test_render(void);
void test_species(void);
void test_assets(void);
void test_tank(void);
void test_tank_finger(void);
void test_app(void);

int main(void)
{
    test_brain();
    test_gfx();
    test_species();
    test_assets();
    test_tank();
    test_tank_finger();
    test_app();
    test_render();

    if (test_failures) {
        printf("%d check(s) FAILED\n", test_failures);
        return 1;
    }
    printf("all tests passed\n");
    return 0;
}
