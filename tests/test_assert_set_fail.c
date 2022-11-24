#include "config.h"

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <cmocka_private.h>

static void test_assert_int_in_set_fail(void **state)
{
    int32_t set[] = {1, 2, 3, INT32_MIN, INT32_MAX};

    (void)state; /* unused */

    assert_int_in_set(4, set, ARRAY_SIZE(set));
}

int main(void) {
    const struct CMUnitTest set_fail_tests[] = {
        cmocka_unit_test(test_assert_int_in_set_fail),
    };

    return cmocka_run_group_tests(set_fail_tests, NULL, NULL);
}

