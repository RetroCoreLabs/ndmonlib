/*
 * Shared helpers for the ndmonlib tests.
 *
 * check()      - print PASS/FAIL for one condition and count failures.
 * mock memory  - a flat MOCK_MEM_SIZE byte array standing in for guest
 *                memory, with word/halfword/byte callbacks for MonContext.
 *                Words and halfwords are stored most significant byte first.
 *                That is the order this mock uses; the tests only rely on
 *                it being consistent, not on it matching real hardware.
 */

#ifndef NDMON_TEST_UTILS_H
#define NDMON_TEST_UTILS_H

#include "mon.h"
#include <stdint.h>

extern int test_failures;

void check(int cond, const char* what);

/* Print the summary line and return the process exit code (0 = all passed). */
int test_finish(const char* suite);

#define MOCK_MEM_SIZE 4096
extern uint8_t mock_mem[MOCK_MEM_SIZE];

uint32_t mock_read_word(void* cpu, uint32_t addr);
void mock_write_word(void* cpu, uint32_t addr, uint32_t val);
uint16_t mock_read_halfword(void* cpu, uint32_t addr);
void mock_write_halfword(void* cpu, uint32_t addr, uint16_t val);
uint8_t mock_read_byte(void* cpu, uint32_t addr);
void mock_write_byte(void* cpu, uint32_t addr, uint8_t val);

/* Zero the context and hook up the mock memory callbacks. */
void mock_init_ctx(MonContext* ctx);

#endif /* NDMON_TEST_UTILS_H */
