/**
 * Test utilities for ndmonlib tests
 */

#include <stdio.h>
#include <stdlib.h>

static int tests_passed = 0;
static int tests_failed = 0;

void test_assert(int condition, const char* message) {
    if (condition) {
        tests_passed++;
        printf("✓ %s\n", message);
    } else {
        tests_failed++;
        printf("✗ %s\n", message);
    }
}

void test_summary(void) {
    printf("\n=== Test Summary ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    if (tests_failed > 0) {
        exit(1);
    }
}

int test_utils_placeholder = 0;  // Silence unused warning
