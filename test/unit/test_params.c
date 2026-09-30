/*
 * Parameter access tests - src/core/mon_params.c.
 *
 * Every handler reads its arguments and writes its results through these
 * helpers, so a fault here shows up as a fault in whichever MON call happens
 * to be under test. The expected values below are taken from mon_params.c
 * itself; where that code makes a choice that has not been checked against
 * the real machine (the dword word order) the test says so.
 */

#include "unit/test_utils.h"
#include <stdio.h>
#include <string.h>

/* Point argument `idx` at guest address `addr`. */
static void set_arg(MonContext* ctx, int idx, uint32_t addr) {
    ctx->arg_addresses[idx] = addr;
    if ((uint32_t)idx + 1 > ctx->arg_count) ctx->arg_count = (uint32_t)idx + 1;
}

static void fresh(MonContext* ctx) {
    memset(mock_mem, 0, sizeof(mock_mem));
    mock_init_ctx(ctx);
}

/* ---- word / halfword / byte / dword ------------------------------------ */

static void test_word_halfword_byte_round_trip(void) {
    MonContext ctx;
    fresh(&ctx);
    set_arg(&ctx, 0, 0x100);
    set_arg(&ctx, 1, 0x110);
    set_arg(&ctx, 2, 0x120);

    mon_write_param_word(&ctx, 0, 0x12345678u);
    check(mon_read_param_word(&ctx, 0) == 0x12345678u, "word: write then read gives the same value");
    check(mock_mem[0x100] == 0x12 && mock_mem[0x103] == 0x78,
          "word: written at the argument address, through the write_word callback");

    mon_write_param_halfword(&ctx, 1, 0xBEEF);
    check(mon_read_param_halfword(&ctx, 1) == 0xBEEF, "halfword: write then read gives the same value");
    check(mock_mem[0x112] == 0, "halfword: does not touch the bytes after it");

    mon_write_param_byte(&ctx, 2, 0xA5);
    check(mon_read_param_byte(&ctx, 2) == 0xA5, "byte: write then read gives the same value");
    check(mock_mem[0x121] == 0, "byte: does not touch the byte after it");
}

static void test_dword(void) {
    MonContext ctx;
    fresh(&ctx);
    set_arg(&ctx, 0, 0x100);

    mon_write_param_dword(&ctx, 0, 0x1122334455667788ull);
    check(mon_read_param_dword(&ctx, 0) == 0x1122334455667788ull,
          "dword: write then read gives the same value");
    /* Word order as coded in mon_params.c: low word first, high word at +4.
     * Not verified against the ND-500; this pins the current behaviour so a
     * change to it is noticed. */
    check(mock_read_word(NULL, 0x100) == 0x55667788u && mock_read_word(NULL, 0x104) == 0x11223344u,
          "dword: low word at the argument address, high word at +4 (current mon_params.c order)");
}

static void test_bad_index_and_missing_callback(void) {
    MonContext ctx;
    fresh(&ctx);
    set_arg(&ctx, 0, 0x100);
    mock_write_word(NULL, 0x100, 0xCAFEBABEu);

    check(mon_read_param_word(&ctx, 1) == 0, "read with idx == arg_count returns 0");
    check(mon_read_param_word(&ctx, -1) == 0, "read with negative idx returns 0");

    mon_write_param_word(&ctx, 1, 0x11111111u);
    mon_write_param_word(&ctx, -1, 0x11111111u);
    check(mock_read_word(NULL, 0x100) == 0xCAFEBABEu, "write with a bad idx changes nothing");

    ctx.read_word = NULL;
    check(mon_read_param_word(&ctx, 0) == 0, "read with no read_word callback returns 0");
}

/* ---- strings ------------------------------------------------------------ */

static void put_bytes(uint32_t addr, const char* s, size_t n) {
    memcpy(&mock_mem[addr], s, n);
}

static void test_sintran_string(void) {
    MonContext ctx;
    char buf[32];

    fresh(&ctx);
    set_arg(&ctx, 0, 0x200);
    put_bytes(0x200, "HELLO'XYZ", 9);
    check(mon_read_sintran_string(&ctx, 0, buf, sizeof(buf)) == 5 && strcmp(buf, "HELLO") == 0,
          "SINTRAN string stops at 0x27 (apostrophe)");

    fresh(&ctx);
    set_arg(&ctx, 0, 0x200);
    put_bytes(0x200, "AB\0CD", 5);
    check(mon_read_sintran_string(&ctx, 0, buf, sizeof(buf)) == 2 && strcmp(buf, "AB") == 0,
          "SINTRAN string stops at 0x00");

    fresh(&ctx);
    set_arg(&ctx, 0, 0x200);
    put_bytes(0x200, "XY\xFFZ", 4);
    check(mon_read_sintran_string(&ctx, 0, buf, sizeof(buf)) == 2 && strcmp(buf, "XY") == 0,
          "SINTRAN string stops at 0xFF");

    fresh(&ctx);
    set_arg(&ctx, 0, 0x200);
    put_bytes(0x200, "ABCDEFG'", 8);
    check(mon_read_sintran_string(&ctx, 0, buf, 4) == 3 && strcmp(buf, "ABC") == 0,
          "SINTRAN string is cut to max-1 characters and still 0-terminated");

    check(mon_read_sintran_string(&ctx, 5, buf, sizeof(buf)) == 0 && buf[0] == '\0',
          "SINTRAN string with a bad idx returns 0 and an empty buffer");
}

/* Descriptor at `desc`: [length word][pointer word], characters at `text`. */
static void put_descriptor(uint32_t desc, uint32_t len, uint32_t text) {
    mock_write_word(NULL, desc, len);
    mock_write_word(NULL, desc + 4, text);
}

static void test_descriptor_string(void) {
    MonContext ctx;
    char buf[32];

    fresh(&ctx);
    set_arg(&ctx, 0, 0x100);
    put_descriptor(0x100, 6, 0x300);
    put_bytes(0x300, "MYFILE", 6);
    check(mon_read_descriptor_string(&ctx, 0, buf, sizeof(buf)) == 6 && strcmp(buf, "MYFILE") == 0,
          "descriptor string: reads `length` characters from the pointer");

    fresh(&ctx);
    set_arg(&ctx, 0, 0x100);
    put_descriptor(0x100, 3, 0x300);
    put_bytes(0x300, "ABCDEF", 6);
    check(mon_read_descriptor_string(&ctx, 0, buf, sizeof(buf)) == 3 && strcmp(buf, "ABC") == 0,
          "descriptor string: stops after `length` characters");

    fresh(&ctx);
    set_arg(&ctx, 0, 0x100);
    put_descriptor(0x100, 8, 0x300);
    put_bytes(0x300, "NAME    ", 8);
    check(mon_read_descriptor_string(&ctx, 0, buf, sizeof(buf)) == 4 && strcmp(buf, "NAME") == 0,
          "descriptor string: trailing spaces are removed");

    fresh(&ctx);
    set_arg(&ctx, 0, 0x100);
    put_descriptor(0x100, 10, 0x300);
    put_bytes(0x300, "AB'CDEFGHI", 10);
    check(mon_read_descriptor_string(&ctx, 0, buf, sizeof(buf)) == 2 && strcmp(buf, "AB") == 0,
          "descriptor string: 0x27 inside the length still ends the string");

    fresh(&ctx);
    set_arg(&ctx, 0, 0x100);
    put_descriptor(0x100, 10, 0x300);
    put_bytes(0x300, "ABCDEFGHIJ", 10);
    check(mon_read_descriptor_string(&ctx, 0, buf, 5) == 4 && strcmp(buf, "ABCD") == 0,
          "descriptor string: cut to max-1 characters");

    fresh(&ctx);
    set_arg(&ctx, 0, 0x100);
    put_descriptor(0x100, 0, 0x300);
    check(mon_read_descriptor_string(&ctx, 0, buf, sizeof(buf)) == -1 && buf[0] == '\0',
          "descriptor string: length 0 is rejected with -1");

    put_descriptor(0x100, 5, 0);
    check(mon_read_descriptor_string(&ctx, 0, buf, sizeof(buf)) == -1,
          "descriptor string: pointer 0 is rejected with -1");

    put_descriptor(0x100, 10001, 0x300);
    check(mon_read_descriptor_string(&ctx, 0, buf, sizeof(buf)) == -1,
          "descriptor string: length above 10000 is rejected with -1");
}

static void test_write_string(void) {
    MonContext ctx;
    fresh(&ctx);
    set_arg(&ctx, 0, 0x400);
    memset(&mock_mem[0x400], 0x55, 8);

    check(mon_write_string(&ctx, 0, "ABC") == 3, "write_string returns the number of characters");
    check(memcmp(&mock_mem[0x400], "ABC", 3) == 0 && mock_mem[0x403] == 0x00,
          "write_string writes the characters and a 0x00 after them");
    check(mock_mem[0x404] == 0x55, "write_string writes nothing after the 0x00");
}

/* ---- error / success / halt --------------------------------------------- */

static int k_flag_seen = -1;
static int32_t error_code_seen = 0;

static void spy_set_k_flag(void* cpu, int value) { (void)cpu; k_flag_seen = value; }
static void spy_set_error_code(void* cpu, int32_t code) { (void)cpu; error_code_seen = code; }

static void test_error_success_halt(void) {
    MonContext ctx;
    fresh(&ctx);
    ctx.set_k_flag = spy_set_k_flag;
    ctx.set_error_code = spy_set_error_code;

    mon_set_error(&ctx, 056);
    check(ctx.error_flag == 1 && ctx.error_code == 056, "set_error stores flag 1 and the code in the context");
    check(k_flag_seen == 1 && error_code_seen == 056, "set_error sets K and passes the code to the callbacks");

    mon_set_success(&ctx);
    check(ctx.error_flag == 0 && ctx.error_code == 0, "set_success clears flag and code in the context");
    check(k_flag_seen == 0, "set_success clears K through the callback");

    mon_request_halt(&ctx, "test reason");
    check(ctx.halt_requested == 1 && strcmp(ctx.halt_reason, "test reason") == 0,
          "request_halt sets halt_requested and the reason");
}

int main(void) {
    printf("Parameter access tests\n\n");
    test_word_halfword_byte_round_trip();
    test_dword();
    test_bad_index_and_missing_callback();
    test_sintran_string();
    test_descriptor_string();
    test_write_string();
    test_error_success_halt();
    return test_finish("parameter");
}
