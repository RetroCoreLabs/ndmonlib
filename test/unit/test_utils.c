/*
 * Shared helpers for the ndmonlib tests - see test_utils.h.
 */

#include "unit/test_utils.h"
#include <stdio.h>
#include <string.h>

int test_failures = 0;

void check(int cond, const char* what) {
    printf("%s: %s\n", cond ? "PASS" : "FAIL", what);
    if (!cond) test_failures++;
}

int test_finish(const char* suite) {
    if (test_failures == 0) {
        printf("\nAll %s tests passed\n", suite);
        return 0;
    }
    printf("\n%s: %d FAILURE(S)\n", suite, test_failures);
    return 1;
}

/* ---- mock guest memory ------------------------------------------------- */

uint8_t mock_mem[MOCK_MEM_SIZE];

uint32_t mock_read_word(void* cpu, uint32_t addr) {
    (void)cpu;
    if (addr + 3 >= MOCK_MEM_SIZE) return 0;
    return ((uint32_t)mock_mem[addr] << 24) | ((uint32_t)mock_mem[addr + 1] << 16) |
           ((uint32_t)mock_mem[addr + 2] << 8) | mock_mem[addr + 3];
}

void mock_write_word(void* cpu, uint32_t addr, uint32_t val) {
    (void)cpu;
    if (addr + 3 >= MOCK_MEM_SIZE) return;
    mock_mem[addr]     = (uint8_t)(val >> 24);
    mock_mem[addr + 1] = (uint8_t)(val >> 16);
    mock_mem[addr + 2] = (uint8_t)(val >> 8);
    mock_mem[addr + 3] = (uint8_t)val;
}

uint16_t mock_read_halfword(void* cpu, uint32_t addr) {
    (void)cpu;
    if (addr + 1 >= MOCK_MEM_SIZE) return 0;
    return (uint16_t)(((uint16_t)mock_mem[addr] << 8) | mock_mem[addr + 1]);
}

void mock_write_halfword(void* cpu, uint32_t addr, uint16_t val) {
    (void)cpu;
    if (addr + 1 >= MOCK_MEM_SIZE) return;
    mock_mem[addr]     = (uint8_t)(val >> 8);
    mock_mem[addr + 1] = (uint8_t)val;
}

uint8_t mock_read_byte(void* cpu, uint32_t addr) {
    (void)cpu;
    return (addr < MOCK_MEM_SIZE) ? mock_mem[addr] : 0;
}

void mock_write_byte(void* cpu, uint32_t addr, uint8_t val) {
    (void)cpu;
    if (addr < MOCK_MEM_SIZE) mock_mem[addr] = val;
}

void mock_init_ctx(MonContext* ctx) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->read_word      = mock_read_word;
    ctx->write_word     = mock_write_word;
    ctx->read_halfword  = mock_read_halfword;
    ctx->write_halfword = mock_write_halfword;
    ctx->read_byte      = mock_read_byte;
    ctx->write_byte     = mock_write_byte;
}
